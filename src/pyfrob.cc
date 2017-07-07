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

#include <ostream>

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
                                   Namespace *ns, PyVersion *version) {
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
  const PyAddresses addrs = pyelf.GetAddresses(version);
  if (!addrs.is_valid()) {
    throw FatalException("Failed to locate addresses");
  }
  return addrs + offset;
}

PyAddresses Addrs(pid_t pid, Namespace *ns, PyVersion *version) {
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

  PyAddresses addrs = target.GetAddresses(version);
  if (addrs.is_valid()) {
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
    return AddressesFromLibPython(pid, libpython, ns, version);
  }
  // A process like uwsgi may use dlopen() to load libpython... let's just guess
  // that the DSO is called libpython2.7.so
  //
  // XXX: this won't work if the embedding language is Python 3
  return AddressesFromLibPython(pid, "libpython2.7.so", ns, version);
}
}  // namespace

#ifdef ENABLE_PY2
namespace py2 {
FROB_FUNCS
}
#endif

#ifdef ENABLE_PY3
namespace py3 {
FROB_FUNCS
}
#endif

#ifdef ENABLE_PY36
namespace py36 {
FROB_FUNCS
}
#endif

void PyFrob::set_addrs_(PyVersion *version) {
  Namespace ns(pid_);
  addrs_ = Addrs(pid_, &ns, version);
#ifdef __amd64__
  // If we didn't find the interp_head address, but we did find the public
  // PyInterpreterState_Head
  // function, use evil non-portable ptrace tricks to call the function
  if (enable_threads_ && addrs_.interp_head_addr == 0 &&
      addrs_.interp_head_hint == 0 && addrs_.interp_head_fn_addr != 0) {
    addrs_.interp_head_hint =
        PtraceCallFunction(pid_, addrs_.interp_head_fn_addr);
  }
#endif
}

void PyFrob::SetPython(PyVersion version) {
  switch (version) {
#ifdef ENABLE_PY2
    case PyVersion::Py2:
      get_threads_ = py2::GetThreads;
      break;
#endif
#ifdef ENABLE_PY
    case PyVersion::Py3:
      get_threads_ = py3::GetThreads;
      break;
#endif
#ifdef ENABLE_PY36
    case PyVersion::Py36:
      get_threads_ = py36::GetThreads;
      break;
#endif
    default:
      std::ostringstream os;
      os << "Target is Python " << static_cast<int>(version)
         << ", which is not supported by this pyflame build.";
      throw FatalException(os.str());
  }
  set_addrs_(&version);
}

void PyFrob::DetectPython() {
  PyVersion version = PyVersion::Unknown;
  set_addrs_(&version);
  SetPython(version);
}

std::vector<Thread> PyFrob::GetThreads() {
  return get_threads_(pid_, addrs_, enable_threads_);
}
}  // namespace pyflame
