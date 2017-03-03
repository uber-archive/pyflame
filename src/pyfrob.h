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

#include "./frame.h"
#include "./symbol.h"

// This abstracts the representation of py2/py3
namespace pyflame {

// Locate the address of the first frame
typedef unsigned long (*first_frame_addr_t)(pid_t, unsigned long);

// Get the stack. The stack will be in reverse order (most recent frame first).
typedef std::vector<Frame> (*get_stack_t)(pid_t, unsigned long);

// Frobber to get python stack stuff; this encapsulates all of the Python
// interpreter logic.
class PyFrob {
 public:
  PyFrob(pid_t pid) : pid_(pid), thread_state_addr_(0) {}

  // Must be called before GetStack() to set/auto-detect the Python version
  void DetectPython();
  void SetPython(PyVersion);

  // Get the current frame list.
  std::vector<Frame> GetStack();

 private:
  pid_t pid_;
  unsigned long thread_state_addr_;
  first_frame_addr_t first_frame_addr_;
  get_stack_t get_stack_;
};

}  // namespace pyflame
