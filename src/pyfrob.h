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

#include "./symbol.h"
#include "./thread.h"


// This abstracts the representation of py2/py3
namespace pyflame {

// Get the threads. Each thread stack will be in reverse order (most recent
// frame first).
typedef std::vector<Thread> (*get_threads_t)(pid_t, PyAddresses);

// Frobber to get python stack stuff; this encapsulates all of the Python
// interpreter logic.
class PyFrob {
 public:
  PyFrob(pid_t pid) : pid_(pid), addrs_() {}

  // Must be called before GetThreads() to set/auto-detect the Python version
  void DetectPython();
  void SetPython(PyVersion);

  // Get the current frame list.
  std::vector<Thread> GetThreads();

 private:
  pid_t pid_;
  PyAddresses addrs_;
  get_threads_t get_threads_;
};

}  // namespace pyflame
