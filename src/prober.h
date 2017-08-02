// Copyright 2017 Evan Klitzke <evan@eklitzke.org>
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

#include <chrono>
#include <string>

#include "./pyfrob.h"
#include "./symbol.h"

// Maximum number of times to retry checking for Python symbols when -p is used.
#define MAX_ATTACH_RETRIES 1

// Maximum number of times to retry checking for Python symbols when -t is used.
#define MAX_TRACE_RETRIES 50

namespace pyflame {

class Prober {
 public:
  Prober()
      : abi_(PyABI::Unknown),
        pid_(-1),
        trace_(false),
        include_idle_(true),
        include_ts_(false),
        enable_threads_(false),
        seconds_(1),
        sample_rate_(0.001) {}
  Prober(const Prober &other) = delete;

  int ParseOpts(int argc, char **argv);

  int InitiatePtrace(int optind, char **argv);

  int FindSymbols(PyFrob *frobber);

  int ProbeLoop(const PyFrob &frobber);

  inline bool enable_threads() const { return enable_threads_; }
  inline pid_t pid() const { return pid_; }

 private:
  PyABI abi_;
  pid_t pid_;
  bool trace_;
  bool include_idle_;
  bool include_ts_;
  bool enable_threads_;
  double seconds_;
  double sample_rate_;
  std::chrono::microseconds interval_;
  std::string output_file_;
  std::string trace_target_;

  pid_t ParsePid(const char *pid_str);

  inline size_t MaxRetries() const {
    return trace_ ? MAX_TRACE_RETRIES : MAX_ATTACH_RETRIES;
  }
};
}  // namespace pyflame
