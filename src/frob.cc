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

// XXX: This file isn't compiled directly. It's included by frob2.cc or
// frob3.cc, which define PYFLAME_PY_VERSION. Since Makefile.am for more
// information.

#include <sys/types.h>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#include <Python.h>
#include <frameobject.h>

#include "./config.h"
#include "./exc.h"
#include "./ptrace.h"
#include "./pyfrob.h"

// why would this not be true idk
static_assert(sizeof(long) == sizeof(void *), "wat platform r u on");

namespace pyflame {

#if PYFLAME_PY_VERSION == 2
namespace py2 {
unsigned long StringSize(unsigned long addr) {
  return addr + offsetof(PyStringObject, ob_size);
}

unsigned long StringData(unsigned long addr) {
  return addr + offsetof(PyStringObject, ob_sval);
}
#elif PYFLAME_PY_VERSION == 3
namespace py3 {
unsigned long StringSize(unsigned long addr) {
  return addr + offsetof(PyVarObject, ob_size);
}

unsigned long StringData(unsigned long addr) {
  // this works only if the filename is all ascii *fingers crossed*
  return addr + sizeof(PyASCIIObject);
}
#else
static_assert(false, "uh oh, bad PYFLAME_PY_VERSION");
#endif

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

std::vector<Frame> GetStack(pid_t pid, unsigned long frame_addr) {
  std::vector<Frame> stack;
  FollowFrame(pid, frame_addr, &stack);
  return stack;
}
}  // namespace py2/py3
}  // namespace pyflame
