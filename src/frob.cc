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
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#include <Python.h>
#include <frameobject.h>

#if PYFLAME_PY_VERSION == 3
#include <unicodeobject.h>
#endif

#include "./config.h"
#include "./exc.h"
#include "./ptrace.h"
#include "./pyfrob.h"
#include "./symbol.h"
#include "./symbol.h"

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

unsigned long ByteData(unsigned long addr) {
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

unsigned long ByteData(unsigned long addr) {
  return addr + offsetof(PyBytesObject, ob_sval);
}

#else
static_assert(false, "uh oh, bad PYFLAME_PY_VERSION");
#endif

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
      PtracePeekBytes(pid, ByteData(co_lnotab), size);
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

std::vector<Thread> GetThreads(pid_t pid, PyAddresses addrs,
                               bool enable_threads) {
  unsigned long istate = 0;

  // First try to get interpreter state via dereferencing
  // _PyThreadState_Current. This won't work if the main thread doesn't hold
  // the GIL (_Current will be null).
  unsigned long tstate = PtracePeek(pid, addrs.tstate_addr);
  unsigned long current_tstate = tstate;
  if (enable_threads) {
    if (tstate != 0) {
      istate = static_cast<unsigned long>(
          PtracePeek(pid, tstate + offsetof(PyThreadState, interp)));
      // Secondly try to get it via the static interp_head symbol, if we managed
      // to find it:
      //  - interp_head is not strictly speaking part of the public API so it
      //    might get removed!
      //  - interp_head is not part of the dynamic symbol table, so e.g. strip
      //    will drop it
    } else if (addrs.interp_head_addr != 0) {
      istate =
          static_cast<unsigned long>(PtracePeek(pid, addrs.interp_head_addr));
    } else if (addrs.interp_head_hint != 0) {
      // Finally. check if we have already put a hint into interp_head_hint -
      // currently this can only happen if we called PyInterpreterState_Head.
      istate = addrs.interp_head_hint;
    }
    if (istate != 0) {
      tstate = static_cast<unsigned long>(
          PtracePeek(pid, istate + offsetof(PyInterpreterState, tstate_head)));
    }
  }

  std::vector<Thread> threads;
  while (tstate != 0) {
    const long id =
        PtracePeek(pid, tstate + offsetof(PyThreadState, thread_id));
    const bool is_current = tstate == current_tstate;

    // dereference the frame
    const unsigned long frame_addr = static_cast<unsigned long>(
        PtracePeek(pid, tstate + offsetof(PyThreadState, frame)));

    std::vector<Frame> stack;
    if (frame_addr != 0) {
      FollowFrame(pid, frame_addr, &stack);
      threads.push_back(Thread(id, is_current, stack));
    }

    if (enable_threads) {
      tstate = PtracePeek(pid, tstate + offsetof(PyThreadState, next));
    } else {
      tstate = 0;
    }
  };

  return threads;
}
}  // namespace py2/py3
}  // namespace pyflame
