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

#include <chrono>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

#include "./frame.h"

namespace pyflame {

class Thread {
 public:
  Thread() = delete;
  Thread(const Thread &other)
      : id_(other.id_),
        is_current_(other.is_current_),
        frames_(other.frames_) {}
  Thread(const unsigned long id, const bool is_current,
         const std::vector<Frame> frames)
      : id_(id), is_current_(is_current), frames_(frames) {}

  inline const unsigned long id() const { return id_; }
  inline const bool is_current() const { return is_current_; }
  inline const std::vector<Frame> &frames() const { return frames_; }

  inline bool operator==(const Thread &other) const {
    return id_ == other.id_ && is_current_ == other.is_current_ &&
           frames_ == other.frames_;
  }

 private:
  unsigned long id_;
  bool is_current_;
  std::vector<Frame> frames_;
};

std::ostream &operator<<(std::ostream &os, const Thread &thread);
}  // namespace pyflame
