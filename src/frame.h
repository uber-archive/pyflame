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
#include <iostream>
#include <string>
#include <vector>

#include "./namespace.h"

namespace pyflame {

class Frame {
 public:
  Frame() = delete;
  Frame(const Frame &other)
      : file_(other.file_), name_(other.name_), line_(other.line_) {}
  Frame(const std::string &file, const std::string &name, size_t line)
      : file_(file), name_(name), line_(line) {}

  inline const std::string &file() const { return file_; }
  inline const std::string &name() const { return name_; }
  inline size_t line() const { return line_; }

  inline bool operator==(const Frame &other) const {
    return file_ == other.file_ && line_ == other.line_;
  }

 private:
  std::string file_;
  std::string name_;
  size_t line_;
};

std::ostream &operator<<(std::ostream &os, const Frame &frame);

// Get the stack. The stack will be in reverse order (most recent frame first).
std::vector<Frame> GetStack(pid_t pid, unsigned long frame_addr);

typedef std::vector<Frame> frames_t;

struct FrameHash {
  size_t operator()(const frames_t &frames) const {
    size_t hash = 0;
    for (size_t i = 0; i < frames.size(); i++) {
      hash ^= std::hash<size_t>()(i);
      hash ^= std::hash<std::string>()(frames[i].file());
    }
    return hash;
  }
};

struct FrameTS {
  std::chrono::system_clock::time_point ts;
  frames_t frames;
};
}  // namespace pyflame
