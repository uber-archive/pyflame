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

#include <cstring>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <sys/ptrace.h>
#include <sys/wait.h>

#include "./exc.h"

namespace pyflame {
void PtraceAttach(pid_t pid) {
  if (ptrace(PTRACE_ATTACH, pid, 0, 0)) {
    std::ostringstream ss;
    ss << "Failed to attach to PID " << pid << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
  if (wait(nullptr) == -1) {
    std::ostringstream ss;
    ss << "Failed to wait on PID " << pid << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
}

void PtraceDetach(pid_t pid) {
  if (ptrace(PTRACE_DETACH, pid, 0, 0)) {
    std::ostringstream ss;
    ss << "Failed to detach PID " << pid << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
}

long PtracePeek(pid_t pid, unsigned long addr) {
  errno = 0;
  const long data = ptrace(PTRACE_PEEKDATA, pid, addr, 0);
  if (data == -1 && errno != 0) {
    std::ostringstream ss;
    ss << "Failed to PTRACE_PEEKDATA at " << reinterpret_cast<void *>(addr)
       << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
  return data;
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
  return std::move(bytes);
}
}  // namespace pyflame
