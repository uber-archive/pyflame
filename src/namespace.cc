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

#include "./namespace.h"

#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>

#include "./exc.h"
#include "./posix.h"

namespace {
const char kOurMnt[] = "/proc/self/ns/mnt";
}

namespace pyflame {
Namespace::Namespace(pid_t pid) : out_(-1), in_(-1) {
  struct stat in_st;
  std::ostringstream os;
  os << "/proc/" << pid << "/ns/mnt";
  const std::string their_mnt = os.str();

  struct stat out_st;

  // In the case of no namespace support (ie ancient boxen), still make an attempt to work
  if (lstat(kOurMnt, &out_st) < 0) {
    std::ostringstream ss;
    ss << "Failed to lstat path " << path << ": " << strerror(errno);
    std::cerr ss.str();
    out_ = in_ = -1;
    return;
  }

  // Since Linux 3.8 symbolic links are used.
  if (S_ISLNK(out_st.st_mode)) {
    char our_name[PATH_MAX];
    ssize_t ourlen = readlink(kOurMnt, our_name, sizeof(our_name));
    if (ourlen < 0) {
        std::ostringstream ss;
        ss << "Failed to readlink " << kOurMnt << ": " << strerror(errno);
        throw FatalException(ss.str());
    }
    our_name[ourlen] = '\0';

    char their_name[PATH_MAX];
    ssize_t theirlen = readlink(their_mnt.c_str(), their_name, 
                                            sizeof(their_name));
    if (theirlen < 0) {
        std::ostringstream ss;
        ss << "Failed to readlink " << their_mnt.c_str() << ": " 
           << strerror(errno);
        throw FatalException(ss.str());
    }
    their_name[theirlen] = '\0';

    if (strcmp(our_name, their_name) != 0) {
      out_ = OpenRdonly(kOurMnt);
      in_ = OpenRdonly(their_mnt.c_str());
    }
  } else {
    // Before Linux 3.8 these are hard links.
    out_ = OpenRdonly(kOurMnt);
    Fstat(out_, &out_st);

    in_ = OpenRdonly(os.str().c_str());
    Fstat(in_, &in_st);
    if (out_st.st_ino == in_st.st_ino) {
      Close(out_);
      Close(in_);
      out_ = in_ = -1;
    }
  }
}

int Namespace::Open(const char *path) {
  if (in_ != -1) {
    SetNs(in_);
    int fd = open(path, O_RDONLY);
    SetNs(out_);
    return fd;
  } else {
    return open(path, O_RDONLY);
  }
}

Namespace::~Namespace() {
  if (out_) {
    Close(out_);
    Close(in_);
  }
}
}  // namespace pyflame
