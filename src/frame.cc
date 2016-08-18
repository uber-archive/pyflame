// Copyright 2016 Uber Technologies, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "./frame.h"

#include <sys/types.h>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

// only needed for the struct offsets
#include <Python.h>
#include <frameobject.h>

#include "./aslr.h"
#include "./exc.h"
#include "./posix.h"
#include "./ptrace.h"
#include "./pystring.h"
#include "./symbol.h"

// why would this not be true idk
static_assert(sizeof(long) == sizeof(void *), "wat platform r u on");

namespace pyflame {
namespace {
// Extract the line number from the code object. Python uses a compressed table
// data structure to store line numbers. See:
//
// https://svn.python.org/projects/python/trunk/Objects/lnotab_notes.txt
//
// This is essentially an implementation of PyFrame_GetLineNumber /
// PyCode_Addr2Line.
size_t GetLine(pid_t pid, unsigned long frame, unsigned long f_code) {
  const long f_trace = PtracePeek(pid, frame + offsetof(_frame, f_trace));
  if (f_trace) {
    return static_cast<size_t>(
        PtracePeek(pid, frame + offsetof(_frame, f_lineno)) &
        std::numeric_limits<decltype(_frame::f_lineno)>::max());
  }

  const int f_lasti = PtracePeek(pid, frame + offsetof(_frame, f_lasti)) &
                      std::numeric_limits<int>::max();
  const long co_lnotab =
      PtracePeek(pid, f_code + offsetof(PyCodeObject, co_lnotab));

  int size =
      PtracePeek(pid, StringSize(co_lnotab)) & std::numeric_limits<int>::max();
  int line = PtracePeek(pid, f_code + offsetof(PyCodeObject, co_firstlineno)) &
             std::numeric_limits<int>::max();
  const std::unique_ptr<uint8_t[]> tbl =
      PtracePeekBytes(pid, StringData(co_lnotab), size);
  size /= 2;  // since we increment twice in each loop iteration
  const uint8_t *p = tbl.get();
  int addr = 0;
  while (--size >= 0) {
    addr += *p++;
    if (addr > f_lasti) {
      break;
    }
    line += *p++;
  }
  return static_cast<size_t>(line);
}

// This method will fill the stack trace. Normally in the C API there are some
// methods that you can use to extract the filename and line number from a frame
// object. We implement the same logic here just using PTRACE_PEEKDATA. In
// principle we could also execute code in the context of the process, but this
// approach is harder to mess up.
void FollowFrame(pid_t pid, unsigned long frame, std::vector<Frame> *stack) {
  const long f_code = PtracePeek(pid, frame + offsetof(_frame, f_code));
  const long co_filename =
      PtracePeek(pid, f_code + offsetof(PyCodeObject, co_filename));
  const std::string filename = PtracePeekString(pid, StringData(co_filename));
  const long co_name =
      PtracePeek(pid, f_code + offsetof(PyCodeObject, co_name));
  const std::string name = PtracePeekString(pid, StringData(co_name));
  stack->push_back({filename, name, GetLine(pid, frame, f_code)});

  const long f_back = PtracePeek(pid, frame + offsetof(_frame, f_back));
  if (f_back != 0) {
    FollowFrame(pid, f_back, stack);
  }
}

// Locate _PyThreadState_Current within libpython
unsigned long ThreadStateFromLibPython(pid_t pid, const std::string &libpython,
                                       Namespace *ns) {
  std::string elf_path;
  const size_t offset = LocateLibPython(pid, libpython, &elf_path);
  if (offset == 0) {
    std::ostringstream ss;
    ss << "Failed to locate libpython named " << libpython;
    FatalException(ss.str());
  }

  ELF pyelf;
  pyelf.Open(elf_path, ns);
  pyelf.Parse();
  const unsigned long threadstate = pyelf.GetThreadState();
  if (threadstate == 0) {
    throw FatalException("Failed to locate _PyThreadState_Current");
  }
  return threadstate + offset;
}

}  // namespace

std::ostream &operator<<(std::ostream &os, const Frame &frame) {
  os << frame.file() << ':' << frame.name() << ':' << frame.line();
  return os;
}

unsigned long ThreadStateAddr(pid_t pid, Namespace *ns) {
  std::ostringstream ss;
  ss << "/proc/" << pid << "/exe";
  ELF target;
  target.Open(ReadLink(ss.str().c_str()), ns);
  target.Parse();

  // There's two different cases here. The default way Python is compiled you
  // get a "static" build which means that you get a big several-megabytes
  // Python executable that has all of the symbols statically built in. For
  // instance, this is how Python is built on Debian and Ubuntu. This is the
  // easiest case to handle, since in this case there are no tricks, we just
  // need to find the symbol in the ELF file.
  //
  // There's also a configure option called --enable-shared where you get a
  // small several-kilobytes Python executable that links against a
  // several-megabytes libpython2.7.so. This is how Python is built on Fedora.
  // If that's the case we need to do some fiddly things to find the true symbol
  // location.
  //
  // The code here attempts to detect if the executable links against
  // libpython2.7.so, and if it does the libpython variable will be filled with
  // the full soname. That determines where we need to look to find our symbol
  // table.
  std::string libpython;
  for (const auto &lib : target.NeededLibs()) {
    if (lib.find("libpython") != std::string::npos) {
      libpython = lib;
      break;
    }
  }
  if (!libpython.empty()) {
    return ThreadStateFromLibPython(pid, libpython, ns);
  }
  // Appears to be statically linked, find the symbols in the binary
  unsigned long threadstate = target.GetThreadState();
  if (threadstate == 0) {
    // A process like uwsgi may use dlopen() to load libpython... let's just
    // guess that the DSO is called libpython2.7.so
    //
    // XXX: this won't work if the embedding language is Python 3
    threadstate = ThreadStateFromLibPython(pid, "libpython2.7.so", ns);
  }
  return threadstate;
}

std::vector<Frame> GetStack(pid_t pid, unsigned long addr) {
  // dereference _PyThreadState_Current
  const long state = PtracePeek(pid, addr);
  if (state == 0) {
    throw NonFatalException("No active frame for the Python interpreter.");
  }

  // dereference the current frame
  const long frame = PtracePeek(pid, state + offsetof(PyThreadState, frame));

  // get the stack trace
  std::vector<Frame> stack;
  FollowFrame(pid, frame, &stack);
  return stack;
}
}  // namespace pyflame
