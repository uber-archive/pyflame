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

#include "./symbol.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

namespace pyflame {
void ELF::Close() {
  if (addr_ != nullptr) {
    munmap(addr_, length_);
    addr_ = nullptr;
  }
}

// mmap the file
void ELF::Open(const std::string &target) {
  Close();
  int fd;
  ElfType type = DetectType(target, &fd);
  if (type != ElfType::Elf64) {
    throw FatalException("Currently only 64-bit ELF files are supported");
  }

  length_ = lseek(fd, 0, SEEK_END);
  addr_ = mmap(nullptr, length_, PROT_READ, MAP_SHARED, fd, 0);
  while (close(fd) == -1) {
    ;
  }
  if (addr_ == MAP_FAILED) {
    std::ostringstream ss;
    ss << "Failed to mmap " << target << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
}

void ELF::Parse() {
  // skip the first section since it must be of type SHT_NULL
  for (uint16_t i = 1; i < hdr()->e_shnum; i++) {
    const Elf64_Shdr *s = shdr(i);
    switch (s->sh_type) {
      case SHT_STRTAB:
        if (strcmp(strtab(s->sh_name), ".dynstr") == 0) {
          dynstr_ = i;
        }
        break;
      case SHT_DYNSYM:
        dynsym_ = i;
        break;
      case SHT_DYNAMIC:
        dynamic_ = i;
        break;
    }
  }
  if (dynamic_ == -1) {
    throw FatalException("Failed to find section .dynamic");
  } else if (dynstr_ == -1) {
    throw FatalException("Failed to find section .dynstr");
  } else if (dynsym_ == -1) {
    throw FatalException("Failed to find section .dynsym");
  }
}

std::vector<std::string> ELF::NeededLibs() {
  // Get all of the strings
  std::vector<std::string> needed;
  const Elf64_Shdr *s = shdr(dynamic_);
  const Elf64_Shdr *d = shdr(dynstr_);
  for (uint16_t i = 0; i < s->sh_size / s->sh_entsize; i++) {
    const Elf64_Dyn *dyn = reinterpret_cast<const Elf64_Dyn *>(
        p() + s->sh_offset + i * s->sh_entsize);
    if (dyn->d_tag == DT_NEEDED) {
      needed.push_back(
          reinterpret_cast<const char *>(p() + d->sh_offset + dyn->d_un.d_val));
    }
  }
  return needed;
}

unsigned long ELF::GetThreadState() {
  const Elf64_Shdr *s = shdr(dynsym_);
  const Elf64_Shdr *d = shdr(dynstr_);
  for (uint16_t i = 0; i < s->sh_size / s->sh_entsize; i++) {
    const Elf64_Sym *sym = reinterpret_cast<const Elf64_Sym *>(
        p() + s->sh_offset + i * s->sh_entsize);
    const char *name =
        reinterpret_cast<const char *>(p() + d->sh_offset + sym->st_name);
    if (strcmp(name, "_PyThreadState_Current") == 0) {
      return static_cast<unsigned long>(sym->st_value);
    }
  }
  return 0;
}

ElfType ELF::DetectType(const std::string &target, int *fd) {
  *fd = open(target.c_str(), O_RDONLY);
  if (*fd == -1) {
    std::ostringstream ss;
    ss << "Failed to open target " << target << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
  ElfType type = ElfType::Unknown;
  unsigned char buf[5];
  if (read(*fd, buf, sizeof(buf)) != sizeof(buf)) {
    std::ostringstream ss;
    ss << "Failed to read " << target << ": " << strerror(errno);
    throw FatalException(ss.str());
  };
  if (buf[EI_MAG0] != ELFMAG0 || buf[EI_MAG1] != ELFMAG1 ||
      buf[EI_MAG2] != ELFMAG2 || buf[EI_MAG3] != ELFMAG3) {
    std::ostringstream ss;
    ss << "File " << target << " does not have correct ELF magic header";
    throw FatalException(ss.str());
  }
  switch (buf[EI_CLASS]) {
    case ELFCLASS32:
      type = ElfType::Elf32;
      break;
    case ELFCLASS64:
      type = ElfType::Elf64;
      break;
  }
  lseek(*fd, SEEK_SET, 0);  // rewind the fd
  return type;
}

}  // namespace pyflame
