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

namespace pyflame {
// Implementation of a Linux filesystem namespace
class Namespace {
 public:
  Namespace() = delete;
  explicit Namespace(pid_t pid);
  ~Namespace();

  // Get a file descriptor in the namespace
  int Open(const char *path);

 private:
  int out_;  // file descriptor that lets us return to our original namespace
  int in_;   // file descriptor that lets us enter the target namespace
};
}  // namespace pyflame
