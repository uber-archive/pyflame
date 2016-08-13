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
// Representation of an ELF file.
class ELF {
 public:
  ELF() : addr_(nullptr), length_(0), dynamic_(-1), dynstr_(-1), dynsym_(-1) {}
  ~ELF() { Close(); }

  // Open a file
  void Open(const std::string &target);

  // Close the file; normally the destructor will do this for you.
  void Close();

  // Parse the ELF sections.
  void Parse();

  // Find the DT_NEEDED fields. This is similar to the ldd(1) command.
  std::vector<std::string> NeededLibs();

  // Get the address of _PyThreadState_Current
  unsigned long GetThreadState();

 private:
  void *addr_;
  size_t length_;
  int dynamic_, dynstr_, dynsym_;

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
};
}  // namespace pyflame
