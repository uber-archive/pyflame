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

#include "./tstate.h"

#include <cstddef>
#include <sstream>
#include <string>

// only needed for the struct offsets
#include <Python.h>

#include "./aslr.h"
#include "./exc.h"
#include "./posix.h"
#include "./ptrace.h"
#include "./symbol.h"

namespace pyflame {
namespace {
// locate _PyThreadState_Current within libpython
unsigned long ThreadStateFromLibPython(pid_t pid, const std::string &libpython,
                                       Namespace *ns) {
  std::string elf_path;
  const size_t offset = LocateLibPython(pid, libpython, &elf_path);
  if (offset == 0) {
    std::ostringstream ss;
    ss << "Failed to locate libpython named " << libpython;
    throw FatalException(ss.str());
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

  unsigned long threadstate = target.GetThreadState();
  if (threadstate != 0) {
    return threadstate;
  }

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
  // A process like uwsgi may use dlopen() to load libpython... let's just guess
  // that the DSO is called libpython2.7.so
  //
  // XXX: this won't work if the embedding language is Python 3
  return ThreadStateFromLibPython(pid, "libpython2.7.so", ns);
}

unsigned long FirstFrameAddr(pid_t pid, unsigned long tstate_addr) {
  // dereference _PyThreadState_Current
  const long state = PtracePeek(pid, tstate_addr);
  if (state == 0) {
    return 0;
  }

  // dereference the frame
  return static_cast<unsigned long>(
      PtracePeek(pid, state + offsetof(PyThreadState, frame)));
}
}  // namespace pyflame
