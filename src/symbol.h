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

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "./exc.h"

namespace pyflame {
// The type of ELF file. This will be used to implement 32-bit support.
enum class ElfType { Unknown, Elf32, Elf64 };

// Representation of a 64-bit ELF file.
//
// TODO: support 32-bit ELF files. One easiest way to do this would be to have
// this class templated on the architecture where the 64-bit version uses the
// Elf64_* structs and the 32-bit version uses the Elf32_* structs. Then another
// function can inspect the ELF header and tell the caller which class they
// should use.
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

  inline const Elf64_Ehdr *hdr() const {
    return reinterpret_cast<const Elf64_Ehdr *>(addr_);
  }

  inline const Elf64_Shdr *shdr(int idx) const {
    if (idx < 0) {
      std::ostringstream ss;
      ss << "Illegal shdr index: " << idx;
      throw FatalException(ss.str());
    }
    return reinterpret_cast<const Elf64_Shdr *>(p() + hdr()->e_shoff +
                                                idx * hdr()->e_shentsize);
  }

  inline unsigned long p() const {
    return reinterpret_cast<unsigned long>(addr_);
  }

  inline const char *strtab(int offset) const {
    const Elf64_Shdr *strings = shdr(hdr()->e_shstrndx);
    return reinterpret_cast<const char *>(p() + strings->sh_offset + offset);
  }

  inline const char *dynstr(int offset) const {
    const Elf64_Shdr *strings = shdr(dynstr_);
    return reinterpret_cast<const char *>(p() + strings->sh_offset + offset);
  }

  // Detect the file type (32-bit or 64-bit) and return a file descriptor for
  // the file.
  ElfType DetectType(const std::string &target, int *fd);
};
}  // namespace pyflame
