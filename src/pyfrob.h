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

#include "./ptrace.h"
#include "./symbol.h"
#include "./thread.h"

// This abstracts the representation of py2/py3
namespace pyflame {

// Get the threads. Each thread stack will be in reverse order (most recent
// frame first).
typedef std::vector<Thread> (*get_threads_t)(pid_t, PyAddresses, bool);

// Frobber to get python stack stuff; this encapsulates all of the Python
// interpreter logic.
class PyFrob {
 public:
  PyFrob(pid_t pid, bool enable_threads)
      : pid_(pid), enable_threads_(enable_threads) {}
  ~PyFrob() { PtraceCleanup(pid_); }

  // Must be called before GetThreads() to detect the Python ABI.
  int DetectABI(PyABI abi);

  // Get the current frame list.
  std::vector<Thread> GetThreads(void) const;

  // Useful when debugging.
  std::string Status() const;

 private:
  pid_t pid_;
  PyAddresses addrs_;
  bool enable_threads_;
  get_threads_t get_threads_;

  // Fill the addrs_ member
  int set_addrs_(PyABI *abi);
};

}  // namespace pyflame
