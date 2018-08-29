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

#include <Python.h>
#include <frameobject.h>

#if PYFLAME_PY_VERSION >= 34
#include <unicodeobject.h>
#endif

#include <sys/types.h>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#include "./config.h"
#include "./exc.h"
#include "./ptrace.h"
#include "./pyfrob.h"
#include "./symbol.h"

// why would this not be true idk
static_assert(sizeof(long) == sizeof(void *), "wat platform r u on");

namespace pyflame {

#if PYFLAME_PY_VERSION == 26
namespace py26 {
unsigned long StringSize(unsigned long addr) {
  return addr + offsetof(PyStringObject, ob_size);
}

unsigned long ByteData(unsigned long addr) {
  return addr + offsetof(PyStringObject, ob_sval);
}

std::string StringData(pid_t pid, unsigned long addr) {
  return PtracePeekString(pid, ByteData(addr));
}

#elif PYFLAME_PY_VERSION == 34
namespace py34 {
std::string StringDataPython3(pid_t pid, unsigned long addr);

unsigned long StringSize(unsigned long addr) {
  return addr + offsetof(PyVarObject, ob_size);
}

std::string StringData(pid_t pid, unsigned long addr) {
  return StringDataPython3(pid, addr);
}

unsigned long ByteData(unsigned long addr) {
  return addr + offsetof(PyBytesObject, ob_sval);
}

#elif PYFLAME_PY_VERSION == 36
namespace py36 {
std::string StringDataPython3(pid_t pid, unsigned long addr);

unsigned long StringSize(unsigned long addr) {
  return addr + offsetof(PyVarObject, ob_size);
}

std::string StringData(pid_t pid, unsigned long addr) {
  return StringDataPython3(pid, addr);
}

unsigned long ByteData(unsigned long addr) {
  return addr + offsetof(PyBytesObject, ob_sval);
}

#elif PYFLAME_PY_VERSION == 37
namespace py37 {
std::string StringDataPython3(pid_t pid, unsigned long addr);

unsigned long StringSize(unsigned long addr) {
  return addr + offsetof(PyVarObject, ob_size);
}

std::string StringData(pid_t pid, unsigned long addr) {
  return StringDataPython3(pid, addr);
}

unsigned long ByteData(unsigned long addr) {
  return addr + offsetof(PyBytesObject, ob_sval);
}

#else
static_assert(false, "uh oh, bad PYFLAME_PY_VERSION");
#endif

#if PYFLAME_PY_VERSION >= 34
std::string StringDataPython3(pid_t pid, unsigned long addr) {
  // TODO: This function only works for Python >= 3.3. Is it also possible to
  // support older versions of Python 3?

  // TODO: Can we guarantee that the same padding is used for the bitfield?
  const std::unique_ptr<uint8_t[]> unicode_bytes =
      PtracePeekBytes(pid, addr, sizeof(PyASCIIObject));
  PyASCIIObject *unicode =
      reinterpret_cast<PyASCIIObject *>(unicode_bytes.get());

  // Because both the filename and function name string objects are made by the
  // Python interpreter itself, we can probably assume they are compact. This
  // means that the data immediately follows the object, and is of type {ASCII,
  // Latin-1, UCS-2, UCS-4}.
  assert(unicode->state.compact);

  const long str_offset = unicode->state.ascii ? sizeof(PyASCIIObject)
                                               : sizeof(PyCompactUnicodeObject);

  // NOTE: From CPython commit c47adb04 onwards the kind matches directly to
  // character size. This is different from the unicode format specification
  // outlined in PEP 393, which still had only two bits allocated to the kind
  // field.
  const unsigned int ch_size = unicode->state.kind;
  const ssize_t str_length = ch_size * unicode->length;
  const std::unique_ptr<uint8_t[]> bytes =
      PtracePeekBytes(pid, addr + str_offset, str_length);

  std::ostringstream dump;

  for (int i = 0; i < str_length; i += ch_size) {
    Py_UCS4 ch = 0;

    switch (unicode->state.kind) {
      case PyUnicode_1BYTE_KIND:
        ch = bytes[i];
        break;
      case PyUnicode_2BYTE_KIND: {
        Py_UCS2 *data_2 = reinterpret_cast<Py_UCS2 *>(&bytes.get()[i]);
        ch = *data_2;
        break;
      }
      case PyUnicode_4BYTE_KIND: {
        Py_UCS4 *data_4 = reinterpret_cast<Py_UCS4 *>(&bytes.get()[i]);
        ch = *data_4;
        break;
      }
      default:
        // We are not supposed to come here, as the WCHAR kind is not supported
        // when the object is compact.
        assert(false);
        break;
    }

    // TODO: Is it alright to assume a lack of surrogates. They might be present
    // in the UCS-2 representation if the UTF-16 approach is used. We currently
    // assume that CPython will instead use UCS-4 for such characters, instead
    // of using surrogates.

    // Below section is taken from CPython's STRINGLIB(utf8_encoder) routine.
    // The differences are that (1) we use a string builder instead of a char
    // buffer, and (2) that we skip the surrogate handling entirely.
    if (ch < 0x80) {
      /* Encode ASCII */
      dump << (char)ch;
    } else if (ch < 0x0800) {
      /* Encode Latin-1 */
      dump << (char)(0xc0 | (ch >> 6));
      dump << (char)(0x80 | (ch & 0x3f));
    } else if (ch < 0x10000) {
      dump << (char)(0xe0 | (ch >> 12));
      dump << (char)(0x80 | ((ch >> 6) & 0x3f));
      dump << (char)(0x80 | (ch & 0x3f));
    } else {
      /* ch >= 0x10000 */
      assert(ch <= 0x10ffff);  // Maximum code point of Unicode 6.0

      /* Encode UCS4 Unicode ordinals */
      dump << (char)(0xf0 | (ch >> 18));
      dump << (char)(0x80 | ((ch >> 12) & 0x3f));
      dump << (char)(0x80 | ((ch >> 6) & 0x3f));
      dump << (char)(0x80 | (ch & 0x3f));
    }
  }

  return dump.str();
}
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
  const std::string filename = StringData(pid, co_filename);
  const long co_name =
      PtracePeek(pid, f_code + offsetof(PyCodeObject, co_name));
  const std::string name = StringData(pid, co_name);

  stack->push_back({filename, name, GetLine(pid, frame, f_code)});

  const long f_back = PtracePeek(pid, frame + offsetof(_frame, f_back));
  if (f_back != 0) {
    FollowFrame(pid, f_back, stack);
  }
}

// N.B. To better understand how this method works, read the implementation of
// pystate.c in the CPython source code.
std::vector<Thread> GetThreads(pid_t pid, PyAddresses addrs,
                               bool enable_threads) {
  // Pointer to the current interpreter state. Python has a very rarely used
  // feature called "sub-interpreters", Pyflame only supports profiling a single
  // sub-interpreter.
  unsigned long istate = 0;

  // First try to get interpreter state via dereferencing
  // _PyThreadState_Current. This won't work if the main thread doesn't hold
  // the GIL (_Current will be null).
  unsigned long tstate = 0;
  if (addrs.tstate_addr) {
    tstate = PtracePeek(pid, addrs.tstate_addr);
  }

  if (tstate == 0 && addrs.tstate_get_addr != 0) {
    // If we are Python 3.7, there will be no global reference to current thread
    // state, and the gilstate's ThreadState will be null if during memory
    // probing the child was not executing Python code. We need to run this
    // function to get the current running ThreadState
    tstate = PtraceCallFunction(pid, addrs.tstate_get_addr);
  }

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

  // Walk the thread list.
  std::vector<Thread> threads;
  while (tstate != 0) {
    const unsigned long id =
        PtracePeek(pid, tstate + offsetof(PyThreadState, thread_id));
    const bool is_current = tstate == current_tstate;

    // Dereference the thread's current frame.
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
}  // namespace py*
}  // namespace pyflame
