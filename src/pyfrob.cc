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

#include "./pyfrob.h"

#include <fstream>
#include <sstream>

#include "./aslr.h"
#include "./config.h"
#include "./exc.h"
#include "./namespace.h"
#include "./posix.h"
#include "./ptrace.h"
#include "./symbol.h"

#define FROB_FUNCS                                            \
  std::vector<Thread> GetThreads(pid_t pid, PyAddresses addr, \
                                 bool enable_threads);

namespace pyflame {
namespace {
// locate within libpython
PyAddresses AddressesFromLibPython(pid_t pid, const std::string &libpython,
                                   Namespace *ns, PyABI *abi) {
  std::string elf_path;
  const size_t offset = LocateLibPython(pid, libpython, &elf_path);
  if (offset == 0) {
    std::ostringstream ss;
    ss << "Failed to locate libpython named " << libpython;
    throw SymbolException(ss.str());
  }

  ELF pyelf;
  pyelf.Open(elf_path, ns);
  pyelf.Parse();
  const PyAddresses addrs = pyelf.GetAddresses(abi);
  if (addrs.empty()) {
    throw SymbolException("Failed to locate addresses");
  }
  return addrs + offset;
}

PyAddresses Addrs(pid_t pid, Namespace *ns, PyABI *abi) {
  std::ostringstream ss;
  ss << "/proc/" << pid << "/exe";
  ELF target;
  std::string exe = ReadLink(ss.str().c_str());
  target.Open(exe, ns);
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

  PyAddresses addrs = target.GetAddresses(abi);
  if (addrs) {
    if (addrs.pie) {
      // If Python executable is PIE, add offsets
      std::string elf_path;
      const size_t offset = LocateLibPython(pid, exe, &elf_path);
      return addrs + offset;
    } else {
      return addrs;
    }
  }

  std::string libpython;
  for (const auto &lib : target.NeededLibs()) {
    if (lib.find("libpython") != std::string::npos) {
      libpython = lib;
      break;
    }
  }
  if (!libpython.empty()) {
    return AddressesFromLibPython(pid, libpython, ns, abi);
  }
  // A process like uwsgi may use dlopen() to load libpython... let's just guess
  // that the DSO is called libpython2.7.so
  //
  // XXX: this won't work if the embedding language is Python 3
  return AddressesFromLibPython(pid, "libpython2.7.so", ns, abi);
}
}  // namespace

#ifdef ENABLE_PY26
namespace py26 {
FROB_FUNCS
}
#endif

#ifdef ENABLE_PY34
namespace py34 {
FROB_FUNCS
}
#endif

#ifdef ENABLE_PY36
namespace py36 {
FROB_FUNCS
}
#endif

#ifdef ENABLE_PY37
namespace py37 {
FROB_FUNCS
}
#endif

// Fill the addrs_ member
int PyFrob::set_addrs_(PyABI *abi) {
  Namespace ns(pid_);
  try {
    addrs_ = Addrs(pid_, &ns, abi);
  } catch (const SymbolException &exc) {
    return 1;
  }
#if ENABLE_THREADS
  // If we didn't find the interp_head address, but we did find the public
  // PyInterpreterState_Head
  // function, use evil non-portable ptrace tricks to call the function
  if (enable_threads_ && addrs_.interp_head_addr == 0 &&
      addrs_.interp_head_hint == 0 && addrs_.interp_head_fn_addr != 0) {
    addrs_.interp_head_hint =
        PtraceCallFunction(pid_, addrs_.interp_head_fn_addr);
  }
#endif
  return 0;
}

int PyFrob::DetectABI(PyABI abi) {
  // Set up the function pointers. By default, we auto-detect the ABI. If an ABI
  // is explicitly passed to us, then use that one (even though it could be
  // wrong)!
  if (set_addrs_(abi == PyABI::Unknown ? &abi : nullptr)) {
    return 1;
  }
  switch (abi) {
    case PyABI::Unknown:
      throw FatalException("Failed to detect a Python ABI.");
      break;
#ifdef ENABLE_PY26
    case PyABI::Py26:
      get_threads_ = py26::GetThreads;
      break;
#endif
#ifdef ENABLE_PY34
    case PyABI::Py34:
      get_threads_ = py34::GetThreads;
      break;
#endif
#ifdef ENABLE_PY36
    case PyABI::Py36:
      get_threads_ = py36::GetThreads;
      break;
#endif
#ifdef ENABLE_PY37
    case PyABI::Py37:
      get_threads_ = py37::GetThreads;
      break;
#endif
    default:
      std::ostringstream os;
      os << "Target has Python ABI " << static_cast<int>(abi)
         << ", which is not supported by this pyflame build.";
      throw FatalException(os.str());
  }

  if (addrs_.empty()) {
    throw FatalException("DetectABI(): addrs_ is unexpectedly empty.");
  }
  return 0;
}

std::string PyFrob::Status() const {
  std::ostringstream os;
  os << "/proc/" << pid_ << "/stat";
  std::ifstream statfile(os.str());
  std::string line;
  std::getline(statfile, line);
  return line;
}

std::vector<Thread> PyFrob::GetThreads(void) const {
  return get_threads_(pid_, addrs_, enable_threads_);
}
}  // namespace pyflame
