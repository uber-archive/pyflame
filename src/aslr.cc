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

#include "./aslr.h"
#include "./exc.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace pyflame {
// Find libpython2.7.so and its offset for an ASLR process
size_t LocateLibPython(pid_t pid, const std::string &hint, std::string *path) {
  std::ostringstream ss;
  ss << "/proc/" << pid << "/maps";
  std::ifstream fp(ss.str());
  std::string line;
  std::string elf_path;
  while (std::getline(fp, line)) {
    if (line.find(hint) != std::string::npos) {
      size_t pos = line.find('/');
      if (pos == std::string::npos) {
        throw FatalException("Did not find libpython absolute path");
      }
      *path = line.substr(pos);
      pos = line.find('-');
      if (pos == std::string::npos) {
        throw FatalException("Did not find libpython virtual memory address");
      }
      return std::strtoul(line.substr(0, pos).c_str(), nullptr, 16);
    }
  }
  return 0;
}
}  // namespace pyflame
