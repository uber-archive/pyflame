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

#include "./ptrace.h"

#include <dirent.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/wait.h>

#include "./exc.h"

namespace pyflame {
void PtraceAttach(pid_t pid) {
  if (ptrace(PTRACE_ATTACH, pid, 0, 0)) {
    std::ostringstream ss;
    ss << "Failed to attach to PID " << pid << ": " << strerror(errno);
    throw PtraceException(ss.str());
  }
  if (wait(nullptr) == -1) {
    std::ostringstream ss;
    ss << "Failed to wait on PID " << pid << ": " << strerror(errno);
    throw PtraceException(ss.str());
  }
}

void PtraceDetach(pid_t pid) {
  if (ptrace(PTRACE_DETACH, pid, 0, 0)) {
    std::ostringstream ss;
    ss << "Failed to detach PID " << pid << ": " << strerror(errno);
    throw PtraceException(ss.str());
  }
}

struct user_regs_struct PtraceGetRegs(pid_t pid) {
  struct user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, pid, 0, &regs)) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_GETREGS: " << strerror(errno);
    throw PtraceException(ss.str());
  }
  return regs;
}

void PtraceSetRegs(pid_t pid, struct user_regs_struct regs) {
  if (ptrace(PTRACE_SETREGS, pid, 0, &regs)) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_SETREGS: " << strerror(errno);
    throw PtraceException(ss.str());
  }
}

void PtracePoke(pid_t pid, unsigned long addr, long data) {
  if (ptrace(PTRACE_POKEDATA, pid, addr, (void *)data)) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_POKEDATA at " << reinterpret_cast<void *>(addr)
       << ": " << strerror(errno);
    throw PtraceException(ss.str());
  }
}

long PtracePeek(pid_t pid, unsigned long addr) {
  errno = 0;
  const long data = ptrace(PTRACE_PEEKDATA, pid, addr, 0);
  if (data == -1 && errno != 0) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_PEEKDATA at " << reinterpret_cast<void *>(addr)
       << ": " << strerror(errno);
    throw PtraceException(ss.str());
  }
  return data;
}

void do_wait() {
  int status;
  if (wait(&status) == -1) {
    throw PtraceException("Failed to PTRACE_CONT");
  }
  if (WIFSTOPPED(status)) {
    if (WSTOPSIG(status) != SIGTRAP) {
      std::ostringstream ss;
      ss << "Failed to PTRACE_CONT - unexpectedly got status  "
         << strsignal(status);
      throw PtraceException(ss.str());
    }
  } else {
    std::ostringstream ss;
    ss << "Failed to PTRACE_CONT - unexpectedly got status  " << status;
    throw PtraceException(ss.str());
  }
}

void PtraceCont(pid_t pid) {
  ptrace(PTRACE_CONT, pid, 0, 0);
  do_wait();
}

void PtraceSingleStep(pid_t pid) {
  ptrace(PTRACE_SINGLESTEP, pid, 0, 0);
  do_wait();
}

std::string PtracePeekString(pid_t pid, unsigned long addr) {
  std::ostringstream dump;
  unsigned long off = 0;
  while (true) {
    const long val = PtracePeek(pid, addr + off);

    // XXX: this can be micro-optimized, c.f.
    // https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
    const std::string chunk(reinterpret_cast<const char *>(&val), sizeof(val));
    dump << chunk.c_str();
    if (chunk.find_first_of('\0') != std::string::npos) {
      break;
    }
    off += sizeof(val);
  }
  return dump.str();
}

std::unique_ptr<uint8_t[]> PtracePeekBytes(pid_t pid, unsigned long addr,
                                           size_t nbytes) {
  // align the buffer to a word size
  if (nbytes % sizeof(long)) {
    nbytes = (nbytes / sizeof(long) + 1) * sizeof(long);
  }
  std::unique_ptr<uint8_t[]> bytes(new uint8_t[nbytes]);

  size_t off = 0;
  while (off < nbytes) {
    const long val = PtracePeek(pid, addr + off);
    memmove(bytes.get() + off, &val, sizeof(val));
    off += sizeof(val);
  }
  return bytes;
}

#ifdef __amd64__

static unsigned long probe_ = 0;

static unsigned long AllocPage(pid_t pid) {
  user_regs_struct oldregs = PtraceGetRegs(pid);
  long code = 0x050f;  // syscall
  long orig_code = PtracePeek(pid, oldregs.rip);
  PtracePoke(pid, oldregs.rip, code);

  user_regs_struct newregs = oldregs;
  newregs.rax = SYS_mmap;
  newregs.rdi = 0;                                   // addr
  newregs.rsi = getpagesize();                       // len
  newregs.rdx = PROT_READ | PROT_WRITE | PROT_EXEC;  // prot
  newregs.r10 = MAP_PRIVATE | MAP_ANONYMOUS;         // flags
  newregs.r8 = -1;                                   // fd
  newregs.r9 = 0;                                    // offset
  PtraceSetRegs(pid, newregs);
  PtraceSingleStep(pid);
  unsigned long result = PtraceGetRegs(pid).rax;

  PtraceSetRegs(pid, oldregs);
  PtracePoke(pid, oldregs.rip, orig_code);

  return result;
}

static std::vector<pid_t> ListThreads(pid_t pid) {
  std::vector<pid_t> result;
  std::ostringstream dirname;
  dirname << "/proc/" << pid << "/task";
  DIR *dir = opendir(dirname.str().c_str());
  if (dir == nullptr) {
    throw PtraceException("Failed to list threads");
  }
  dirent *entry;
  while ((entry = readdir(dir)) != nullptr) {
    std::string name = entry->d_name;
    if (name != "." && name != "..") {
      result.push_back(static_cast<pid_t>(std::stoi(name)));
    }
  }
  return result;
}

static void PauseChildThreads(pid_t pid) {
  for (auto tid : ListThreads(pid)) {
    if (tid != pid) PtraceAttach(tid);
  }
}

static void ResumeChildThreads(pid_t pid) {
  for (auto tid : ListThreads(pid)) {
    if (tid != pid) PtraceDetach(tid);
  }
}

long PtraceCallFunction(pid_t pid, unsigned long addr) {
  if (probe_ == 0) {
    PauseChildThreads(pid);
    probe_ = AllocPage(pid);
    ResumeChildThreads(pid);
    if (probe_ == (unsigned long)MAP_FAILED) {
      return -1;
    }

    // std::cerr << "probe point is at " << reinterpret_cast<void *>(probe_)
    //           << "\n";
    long code = 0;
    uint8_t *new_code_bytes = (uint8_t *)&code;
    new_code_bytes[0] = 0xff;  // CALL
    new_code_bytes[1] = 0xd0;  // rax
    new_code_bytes[2] = 0xcc;  // TRAP
    PtracePoke(pid, probe_, code);
  }

  user_regs_struct oldregs = PtraceGetRegs(pid);
  user_regs_struct newregs = oldregs;
  newregs.rax = addr;
  newregs.rip = probe_;

  PtraceSetRegs(pid, newregs);
  PtraceCont(pid);

  newregs = PtraceGetRegs(pid);
  PtraceSetRegs(pid, oldregs);
  return newregs.rax;
};
#endif
}  // namespace pyflame
