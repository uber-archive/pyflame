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
#include <cassert>
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
int DoWait(pid_t pid, int options) {
  int status;
  std::ostringstream ss;
  for (;;) {
    pid_t progeny = waitpid(pid, &status, options);
    if (progeny == -1) {
      ss << "Failed to waitpid(): " << strerror(errno);
      throw PtraceException(ss.str());
    }
    assert(progeny == pid);
    if (WIFSTOPPED(status)) {
      int signum = WSTOPSIG(status);
      if (signum == SIGTRAP) {
        break;
      } else if (signum == SIGCHLD) {
        PtraceCont(pid);  // see issue #122
        continue;
      }
      ss << "waitpid() indicated a WIFSTOPPED process, but got unexpected "
            "signal "
         << signum;
      throw PtraceException(ss.str());
    } else if (WIFEXITED(status)) {
      ss << "Child process " << pid << " exited with status "
         << WEXITSTATUS(status);
      throw TerminateException(ss.str());
    } else {
      ss << "Child process " << pid
         << " returned an unexpected waitpid() code: " << status;
      throw PtraceException(ss.str());
    }
  }
  return status;
}

bool SawEventExec(int status) {
  return status >> 8 == (SIGTRAP | (PTRACE_EVENT_EXEC << 8));
}

void PtraceTraceme() {
  if (ptrace(PTRACE_TRACEME, getpid(), 0, 0)) {
    throw PtraceException("Failed to PTRACE_TRACEME");
  }
  raise(SIGSTOP);
}

void PtraceAttach(pid_t pid) {
  if (ptrace(PTRACE_ATTACH, pid, 0, 0)) {
    std::ostringstream ss;
    ss << "Failed to attach to PID " << pid << ": " << strerror(errno);
    throw PtraceException(ss.str());
  }
  int status;
  if (waitpid(pid, &status, __WALL) != pid || !WIFSTOPPED(status)) {
    std::ostringstream ss;
    ss << "Failed to wait on PID " << pid << ": " << strerror(errno);
    throw PtraceException(ss.str());
  }
}

void PtraceSeize(pid_t pid) {
  if (ptrace(PTRACE_SEIZE, pid, 0, 0)) {
    std::ostringstream ss;
    ss << "Failed to seize PID " << pid << ": " << strerror(errno);
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

// Like PtraceDetach(), but ignore errors.
static inline void SafeDetach(pid_t pid) noexcept {
  ptrace(PTRACE_DETACH, pid, 0, 0);
}

void PtraceInterrupt(pid_t pid) {
  if (ptrace(PTRACE_INTERRUPT, pid, 0, 0)) {
    throw PtraceException("Failed to PTRACE_INTERRUPT");
  }
  DoWait(pid);
}

user_regs_struct PtraceGetRegs(pid_t pid) {
  user_regs_struct regs;
  if (ptrace(PTRACE_GETREGS, pid, 0, &regs)) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_GETREGS: " << strerror(errno);
    throw PtraceException(ss.str());
  }
  return regs;
}

void PtraceSetRegs(pid_t pid, user_regs_struct regs) {
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
    ss << "Failed to PTRACE_PEEKDATA (pid " << pid << ", addr "
       << reinterpret_cast<void *>(addr) << "): " << strerror(errno);
    throw PtraceException(ss.str());
  }
  return data;
}

void PtraceSetOptions(pid_t pid, long options) {
  if (ptrace(PTRACE_SETOPTIONS, pid, 0, options)) {
    throw PtraceException("Failed to PTRACE_SETOPTIONS");
  }
}

void PtraceCont(pid_t pid) {
  if (ptrace(PTRACE_CONT, pid, 0, 0) == -1) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_CONT: " << strerror(errno);
    throw PtraceException(ss.str());
  }
}

void PtraceSingleStep(pid_t pid) {
  if (ptrace(PTRACE_SINGLESTEP, pid, 0, 0) == -1) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_SINGLESTEP: " << strerror(errno);
    throw PtraceException(ss.str());
  }
  DoWait(pid);
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

#if defined(__amd64__) && ENABLE_THREADS
static const long syscall_x86 = 0x050f;  // x86 code for SYSCALL

static unsigned long probe_ = 0;

static unsigned long AllocPage(pid_t pid) {
  user_regs_struct oldregs = PtraceGetRegs(pid);
  long orig_code = PtracePeek(pid, oldregs.rip);
  PtracePoke(pid, oldregs.rip, syscall_x86);

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
  DoWait(pid);

  newregs = PtraceGetRegs(pid);
  PtraceSetRegs(pid, oldregs);
  return newregs.rax;
};

void PtraceCleanup(pid_t pid) noexcept {
  // Clean up the memory area allocated by AllocPage().
  if (probe_ != 0) {
    try {
      const user_regs_struct oldregs = PtraceGetRegs(pid);
      const long orig_code = PtracePeek(pid, oldregs.rip);

      user_regs_struct newregs = oldregs;
      newregs.rax = SYS_munmap;
      newregs.rdi = probe_;         // addr
      newregs.rsi = getpagesize();  // len

      // Prepare to run munmap(2) syscall.
      PauseChildThreads(pid);
      PtracePoke(pid, oldregs.rip, syscall_x86);
    do_munmap:
      PtraceSetRegs(pid, newregs);

      // Actually call munmap(2), and check the return value.
      PtraceSingleStep(pid);
      const long rax = PtraceGetRegs(pid).rax;
      switch (rax) {
        case 0:
          probe_ = 0;
          break;
        case EAGAIN:
          goto do_munmap;
          break;
        default:
          std::cerr << "Warning: failed to munmap(2) trampoline page, %rax = "
                    << rax << "\n";
          break;
      }

      // Clean up and resume the child threads.
      PtracePoke(pid, oldregs.rip, orig_code);
      PtraceSetRegs(pid, oldregs);
      ResumeChildThreads(pid);
    } catch (...) {
      // If the process has already exited, then we'll get a ptrace error, which
      // can be safely ignored. This *should* happen at the initial
      // PtraceGetRegs() call, but we wrap the entire block to be safe.
    }
  }

  SafeDetach(pid);
}
#else
void PtraceCleanup(pid_t pid) noexcept { SafeDetach(pid); }
#endif

}  // namespace pyflame
