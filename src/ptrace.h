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

#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>

#include <memory>
#include <string>

#include "./config.h"

namespace pyflame {

int DoWait(pid_t pid, int options = 0);

bool SawEventExec(int status);

void PtraceTraceme();

// detach a process
void PtraceAttach(pid_t pid);
void PtraceDetach(pid_t pid);

void PtraceSeize(pid_t pid);
void PtraceInterrupt(pid_t pid);

// get regs from a process
struct user_regs_struct PtraceGetRegs(pid_t pid);

// set regs in a process
void PtraceSetRegs(pid_t pid, struct user_regs_struct regs);

// poke a long word into an address
void PtracePoke(pid_t pid, unsigned long addr, long data);

// read the long word at an address
long PtracePeek(pid_t pid, unsigned long addr);

void PtraceSetOptions(pid_t pid, long options);

// peek a null-terminated string
std::string PtracePeekString(pid_t pid, unsigned long addr);

// peek some number of bytes
std::unique_ptr<uint8_t[]> PtracePeekBytes(pid_t pid, unsigned long addr,
                                           size_t nbytes);

// Continue a traced process
void PtraceCont(pid_t pid);

// Execute a single instruction in a traced process
void PtraceSingleStep(pid_t pid);

#if ENABLE_THREADS
// Call a function pointer.
long PtraceCallFunction(pid_t pid, unsigned long addr);
#endif

// Detach, and maybe dealloc the page allocated in PtraceCallFunction();
void PtraceCleanup(pid_t pid) noexcept;
}  // namespace pyflame
