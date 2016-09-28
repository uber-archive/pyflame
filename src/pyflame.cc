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

#include <getopt.h>

#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "./config.h"
#include "./exc.h"
#include "./frame.h"
#include "./namespace.h"
#include "./ptrace.h"
#include "./tstate.h"
#include "./version.h"

using namespace pyflame;

namespace {
const char usage_str[] =
    ("Usage: pyflame [options] <pid>\n"
     "\n"
     "General Options:\n"
     "  -h, --help           Show help\n"
     "  -s, --seconds=SECS   How many seconds to run for (default 1)\n"
     "  -r, --rate=RATE      Sample rate, as a fractional value of seconds "
     "(default 0.001)\n"
     "  -v, --version        Show the version\n"
     "  -x, --exclude-idle   Exclude idle time from statistics\n");
}  // namespace

int main(int argc, char **argv) {
  bool include_idle = true;
  double seconds = 1;
  double sample_rate = 0.001;
  for (;;) {
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"rate", required_argument, 0, 'r'},
        {"seconds", required_argument, 0, 's'},
        {"version", no_argument, 0, 'v'},
        {"exclude-idle", no_argument, 0, 'x'},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long(argc, argv, "hr:s:vx", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 0:
        if (long_options[option_index].flag != 0) {
          // if the option set a flag, do nothing
          break;
        }
        break;
      case 'h':
        std::cout << usage_str;
        return 0;
        break;
      case 'r':
        sample_rate = std::stod(optarg);
        break;
      case 's':
        seconds = std::stod(optarg);
        break;
      case 'v':
        std::cout << PACKAGE_STRING << "\n\n";
        std::cout << kBuildNote << "\n";
        return 0;
        break;
      case 'x':
        include_idle = false;
        break;
      case '?':
        // getopt_long should already have printed an error message
        break;
      default:
        abort();
    }
  }
  if (optind != argc - 1) {
    std::cerr << usage_str;
    return 1;
  }
  long pid = std::strtol(argv[argc - 1], nullptr, 10);
  if (pid > std::numeric_limits<pid_t>::max() ||
      pid < std::numeric_limits<pid_t>::min()) {
    std::cerr << "PID " << pid << " is out of valid PID range.\n";
    return 1;
  }
  try {
    PtraceAttach(pid);
    Namespace ns(pid);
    const unsigned long tstate_addr = ThreadStateAddr(pid, &ns);
    if (seconds) {
      const std::chrono::microseconds interval{
          static_cast<long>(sample_rate * 1000000)};
      std::unordered_map<frames_t, size_t, FrameHash> buckets;
      size_t idle = 0;
      auto end =
          std::chrono::system_clock::now() +
          std::chrono::microseconds(static_cast<long>(seconds * 1000000));
      for (;;) {
        const unsigned long frame_addr = FirstFrameAddr(pid, tstate_addr);
        if (frame_addr == 0) {
          if (include_idle) {
            idle++;
          }
        } else {
          frames_t frames = GetStack(pid, frame_addr);
          auto it = buckets.find(frames);
          if (it == buckets.end()) {
            buckets.insert(it, {frames, 1});
          } else {
            it->second++;
          }
        }
        auto now = std::chrono::system_clock::now();
        if (now + interval >= end) {
          break;
        }
        PtraceDetach(pid);
        std::this_thread::sleep_for(interval);
        PtraceAttach(pid);
      }
      if (idle) {
        std::cout << "(idle) " << idle << "\n";
      }
      // process the frames
      for (const auto &kv : buckets) {
        if (kv.first.empty()) {
          std::cerr << "uh oh\n";
          return 1;
        }
        auto last = kv.first.rend();
        last--;
        for (auto it = kv.first.rbegin(); it != last; ++it) {
          std::cout << *it << ";";
        }
        std::cout << *last << " " << kv.second << "\n";
      }
    } else {
      const unsigned long frame_addr = FirstFrameAddr(pid, tstate_addr);
      if (frame_addr) {
        std::vector<Frame> stack = GetStack(pid, frame_addr);
        for (auto it = stack.rbegin(); it != stack.rend(); it++) {
          std::cout << *it << "\n";
        }
      } else {
        std::cout << "(idle)\n";
      }
    }
  } catch (const FatalException &exc) {
    std::cerr << exc.what() << std::endl;
    return 1;
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << std::endl;
    return 1;
  }
  return 0;
}
