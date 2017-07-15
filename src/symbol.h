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

#pragma once

#include <elf.h>

#include <limits.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "./exc.h"
#include "./namespace.h"

#if (__WORDSIZE == 64)
#define ehdr_t Elf64_Ehdr
#define shdr_t Elf64_Shdr
#define dyn_t Elf64_Dyn
#define sym_t Elf64_Sym
#define ARCH_ELFCLASS ELFCLASS64
#elif (__WORDSIZE == 32)
#define ehdr_t Elf32_Ehdr
#define shdr_t Elf32_Shdr
#define dyn_t Elf32_Dyn
#define sym_t Elf32_Sym
#define ARCH_ELFCLASS ELFCLASS32
#else
static_assert(false, "unknown build environment");
#endif

namespace pyflame {

// The Python interpreter ABI. Some ABIs span multiple Python versions. In that
// case, the convention is to name the ABI after the first Python release to
// introduce the ABI.
enum class PyABI {
  Unknown = 0,  // Unknown Python ABI
  Py26 = 26,    // ABI for Python 2.6/2.7
  Py34 = 34,    // ABI for Python 3.4/3.5
  Py36 = 36     // ABI for Python 3.6
};

// Symbols
struct PyAddresses {
  unsigned long tstate_addr;
  unsigned long interp_head_addr;
  unsigned long interp_head_fn_addr;
  unsigned long interp_head_hint;
  bool pie;

  PyAddresses()
      : tstate_addr(0),
        interp_head_addr(0),
        interp_head_fn_addr(0),
        interp_head_hint(0),
        pie(false) {}

  PyAddresses operator+(const unsigned long base) const {
    PyAddresses res;
    res.tstate_addr = this->tstate_addr == 0 ? 0 : this->tstate_addr + base;
    res.interp_head_addr =
        this->interp_head_addr == 0 ? 0 : this->interp_head_addr + base;
    res.interp_head_fn_addr =
        this->interp_head_fn_addr == 0 ? 0 : this->interp_head_fn_addr + base;
    return res;
  }

  // True indicates a non-empty struct.
  explicit operator bool() const { return !empty(); }

  // Empty means the struct hasn't been initialized.
  inline bool empty() const { return this->tstate_addr == 0; }
};

// Representation of an ELF file.
class ELF {
 public:
  ELF()
      : addr_(nullptr),
        length_(0),
        dynamic_(-1),
        dynstr_(-1),
        dynsym_(-1),
        strtab_(-1),
        symtab_(-1) {}
  ~ELF() { Close(); }

  // Open a file
  void Open(const std::string &target, Namespace *ns);

  // Close the file; normally the destructor will do this automatically.
  void Close();

  // Parse the ELF sections.
  void Parse();

  // Find the DT_NEEDED fields. This is similar to the ldd(1) command.
  std::vector<std::string> NeededLibs();

  // Get the address of _PyThreadState_Current & interp_head, and set the Python
  // ABI.
  PyAddresses GetAddresses(PyABI *abi);

 private:
  void *addr_;
  size_t length_;
  int dynamic_, dynstr_, dynsym_, strtab_, symtab_;

  inline const ehdr_t *hdr() const {
    return reinterpret_cast<const ehdr_t *>(addr_);
  }

  inline const shdr_t *shdr(int idx) const {
    if (idx < 0) {
      std::ostringstream ss;
      ss << "Illegal shdr index: " << idx;
      throw FatalException(ss.str());
    }
    return reinterpret_cast<const shdr_t *>(p() + hdr()->e_shoff +
                                            idx * hdr()->e_shentsize);
  }

  inline unsigned long p() const {
    return reinterpret_cast<unsigned long>(addr_);
  }

  inline const char *strtab(int offset) const {
    const shdr_t *strings = shdr(hdr()->e_shstrndx);
    return reinterpret_cast<const char *>(p() + strings->sh_offset + offset);
  }

  inline const char *dynstr(int offset) const {
    const shdr_t *strings = shdr(dynstr_);
    return reinterpret_cast<const char *>(p() + strings->sh_offset + offset);
  }

  // Walk the symbol table, and return the detected ABI.
  PyABI WalkTable(int sym, int str, PyAddresses *addrs);
};
}  // namespace pyflame
