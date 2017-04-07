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

#include "./posix.h"

#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <sstream>

#include "./exc.h"
#include "./netns.h"

namespace pyflame {
int OpenRdonly(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd < 0) {
    std::ostringstream ss;
    ss << "Failed to open " << path << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
  return fd;
}

void Close(int fd) {
  if (fd < 0) {
    return;
  }
  while (close(fd) == -1)
    ;
}

void Fstat(int fd, struct stat *buf) {
  if (fstat(fd, buf) < 0) {
    std::ostringstream ss;
    ss << "Failed to fstat file descriptor " << fd << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
}

void Lstat(const char *path, struct stat *buf) {
  if (lstat(path, buf) < 0) {
    std::ostringstream ss;
    ss << "Failed to lstat path " << path << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
}

void SetNs(int fd) {
  if (setns(fd, 0)) {
    std::ostringstream ss;
    ss << "Failed to setns " << fd << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
}

std::string ReadLink(const char *path) {
  char buf[PATH_MAX];
  ssize_t nbytes = readlink(path, buf, sizeof(buf));
  if (nbytes < 0) {
    std::ostringstream ss;
    ss << "Failed to read symlink " << path << ": " << strerror(errno);
    throw FatalException(ss.str());
  }
  buf[nbytes] = '\0';
  return {buf, static_cast<std::string::size_type>(nbytes)};
}
}  // namespace pyflame
