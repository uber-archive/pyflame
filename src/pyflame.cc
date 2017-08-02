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
#include <sys/ptrace.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>

#include "./config.h"
#include "./exc.h"
#include "./ptrace.h"
#include "./pyfrob.h"
#include "./symbol.h"
#include "./thread.h"

// Maximum number of times to retry checking for Python symbols when -p is used.
#define MAX_ATTACH_RETRIES 1

// Maximum number of times to retry checking for Python symbols when -t is used.
#define MAX_TRACE_RETRIES 50

// Microseconds in a second.
#define MICROSECONDS_PER_SEC 1000000

using namespace pyflame;

typedef std::unordered_map<frames_t, size_t, FrameHash> buckets_t;

static pid_t ParsePid(const char *pid_str) {
  long pid = std::strtol(pid_str, nullptr, 10);
  if (pid <= 0 || pid > std::numeric_limits<pid_t>::max()) {
    std::cerr << "PID " << pid << " is out of valid PID range.\n";
    return -1;
  }
  return static_cast<pid_t>(pid);
}

// Prints all stack traces
static void PrintFrames(std::ostream &out,
                        const std::vector<FrameTS> &call_stacks,
                        size_t idle_count) {
  if (idle_count) {
    out << "(idle) " << idle_count << "\n";
  }
  // Put the call stacks into buckets
  buckets_t buckets;
  for (const auto &call_stack : call_stacks) {
    auto bucket = buckets.find(call_stack.frames);
    if (bucket == buckets.end()) {
      buckets.insert(bucket, {call_stack.frames, 1});
    } else {
      bucket->second++;
    }
  }
  // process the frames
  for (const auto &kv : buckets) {
    if (kv.first.empty()) {
      std::cerr << "fatal error\n";
      return;
    }
    auto last = kv.first.rend();
    last--;
    for (auto it = kv.first.rbegin(); it != last; ++it) {
      out << *it << ";";
    }
    out << *last << " " << kv.second << "\n";
  }
}

// Prints all stack traces with timestamps
static void PrintFramesTS(std::ostream &out,
                          const std::vector<FrameTS> &call_stacks) {
  for (const auto &call_stack : call_stacks) {
    out << std::chrono::duration_cast<std::chrono::microseconds>(
               call_stack.ts.time_since_epoch())
               .count()
        << "\n";
    // Handle idle
    if (call_stack.frames.empty()) {
      out << "(idle)\n";
      continue;
    }
    // Print the call stack
    for (auto it = call_stack.frames.rbegin(); it != call_stack.frames.rend();
         ++it) {
      out << *it << ";";
    }
    out << "\n";
  }
}

static inline bool IsPyflame(const std::string &str) {
  return str.find("pyflame") != std::string::npos;
}

int main(int argc, char **argv) {
  PyABI abi{};
  pid_t pid = -1;
  long abi_version;
  bool trace = false;
  bool include_idle = true;
  bool include_ts = false;
  bool enable_threads = false;
  double seconds = 1;
  double sample_rate = 0.001;
  std::ofstream output_file;

  static const char usage_str[] =
      ("Usage: pyflame [options] -p PID\n"
       "       pyflame [options] -t command arg1 arg2...\n"
       "\n"
       "General Options:\n"
       "      --abi            Force a particular Python ABI (26, 34, 36)\n"
       "      --threads        Enable multi-threading support\n"
       "  -h, --help           Show help\n"
       "  -p, --pid=PID        The PID to trace\n"
       "  -s, --seconds=SECS   How many seconds to run for (default 1)\n"
       "  -r, --rate=RATE      Sample rate, as a fractional value of seconds "
       "(default 0.001)\n"
       "  -o, --output=PATH    Output to file path\n"
       "  -t, --trace          Trace a child process\n"
       "  -T, --timestamp      Include timestamps for each stacktrace\n"
       "  -v, --version        Show the version\n"
       "  -x, --exclude-idle   Exclude idle time from statistics\n");

  static const char short_opts[] = "ho:p:r:s:tTvx";
  static struct option long_opts[] = {
    {"abi", required_argument, 0, 'a'},
    {"help", no_argument, 0, 'h'},
    {"rate", required_argument, 0, 'r'},
    {"seconds", required_argument, 0, 's'},
#if ENABLE_THREADS
    {"threads", no_argument, 0, 'L'},
#endif
    {"output", required_argument, 0, 'o'},
    {"pid", required_argument, 0, 'p'},
    {"trace", no_argument, 0, 't'},
    {"timestamp", no_argument, 0, 'T'},
    {"version", no_argument, 0, 'v'},
    {"exclude-idle", no_argument, 0, 'x'},
    {0, 0, 0, 0}
  };

  for (;;) {
    int c = getopt_long(argc, argv, short_opts, long_opts, nullptr);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 'a':
        abi_version = std::strtol(optarg, nullptr, 10);
        switch (abi_version) {
          case 26:
          case 27:
            abi = PyABI::Py26;
            break;
          case 34:
          case 35:
            abi = PyABI::Py34;
            break;
          case 36:
            abi = PyABI::Py36;
            break;
          default:
            std::cerr << "Unknown or unsupported ABI version: " << abi_version
                      << "\n";
            return 1;
            break;
        }
        break;
      case 'h':
        std::cout << PYFLAME_VERSION_STR << "\n\n" << usage_str;
        return 0;
        break;
#ifdef ENABLE_THREADS
      case 'L':
        enable_threads = true;
        break;
#endif
      case 'p':
        if ((pid = ParsePid(optarg)) == -1) {
          return 1;
        }
        break;
      case 'r':
        sample_rate = std::stod(optarg);
        break;
      case 's':
        seconds = std::stod(optarg);
        break;
      case 't':
        trace = true;
        seconds = -1;
        goto finish_arg_parse;
        break;
      case 'T':
        include_ts = true;
        break;
      case 'v':
        std::cout << PYFLAME_VERSION_STR << "\n";
        return 0;
        break;
      case 'x':
        include_idle = false;
        break;
      case 'o':
        output_file.open(optarg, std::ios::out | std::ios::trunc);
        if (!output_file.is_open()) {
          std::cerr << "cannot open file \"" << optarg << "\" as output\n";
          return 1;
        }
        break;
      case '?':
        // getopt_long should already have printed an error message
        break;
      default:
        abort();
    }
  }
finish_arg_parse:
  if (trace && pid != -1) {
    std::cerr << "Options -t and -p are not mutually compatible.\n";
    return 1;
  }
  std::ostream *output = &std::cout;
  if (output_file.is_open()) {
    output = &output_file;
  }
  if (trace) {
    if (optind == argc) {
      std::cerr << usage_str;
      return 1;
    }
    if (IsPyflame(argv[optind])) {
      std::cerr << "You tried to pyflame a pyflame, naughty!\n";
      return 1;
    }
    // In trace mode, all of the remaining arguments are a command to run. We
    // fork and have the child run the command; the parent traces.
    pid = fork();
    if (pid == -1) {
      perror("fork()");
      return 1;
    } else if (pid == 0) {
      // Child: request to be traced.
      PtraceTraceme();
      if (execvp(argv[optind], argv + optind)) {
        std::cerr << "execvp() failed for: " << argv[optind]
                  << ", err = " << strerror(errno) << "\n";
        return 1;
      }
    } else {
      // Parent: we trace the child until it's exec'ed the new process before
      // proceeding. For a dynamically linked Python build, there's still a race
      // condition between when the exec() happens and when symbols are
      // available. But there's no point in polling the child until it's at
      // least had a chance to run exec.
      pid_t child = waitpid(0, nullptr, 0);
      assert(child == pid);
      PtraceSetOptions(pid, PTRACE_O_TRACEEXEC);
      PtraceCont(pid);
      int status = 0;
      while (!SawEventExec(status)) {
        pid_t p = waitpid(-1, &status, 0);
        if (p == -1) {
          perror("waitpid()");
          return 1;
        }
        if (WIFEXITED(status)) {
          std::cerr << "Child process exited with status: "
                    << WEXITSTATUS(status) << "\n";
          return 1;
        }
      }
      // We can only use PtraceInterrupt, used later in the main loop, if the
      // process was seized. So we reattach and seize.
      PtraceDetach(pid);
      PtraceSeize(pid);
    }
  } else {
    if (pid == -1) {
      // Users should use -p to supply the PID to trace. However, older versions
      // of Pyflame used a convention where the PID to trace was the final
      // argument to the pyflame command. This code path handles this legacy use
      // case, to preserve backward compatibility.
      if (optind != argc - 1 || (pid = ParsePid(argv[optind])) == -1) {
        std::cerr << usage_str;
        return 1;
      }
      std::cerr
          << "WARNING: Specifying a PID to trace without -p is deprecated; "
             "see Pyflame issue #99 for details.\n ";
    }
    try {
      PtraceSeize(pid);
    } catch (const PtraceException &err) {
      std::cerr << "Failed to seize PID " << pid << "\n";
      return 1;
    }
  }
  PtraceInterrupt(pid);

  std::vector<FrameTS> call_stacks;
  size_t idle_count = 0;
  const std::chrono::microseconds interval{
      static_cast<long>(sample_rate * MICROSECONDS_PER_SEC)};
  try {
    PyFrob frobber(pid, enable_threads);

    // When tracing a dynamically linked Python build, it may take a while for
    // ld.so to actually load symbols into the process. Therefore we retry
    // probing in a loop, until the symbols are loaded. A more reliable way of
    // doing this would be to break at entry to a known static function (e.g.
    // Py_Main), but this isn't reliable in all cases. For instance,
    // /usr/bin/python{,3} will start at Py_Main, but uWSGI will not.
    try {
      const size_t max_retries = trace ? MAX_TRACE_RETRIES : MAX_ATTACH_RETRIES;
      for (size_t i = 0;;) {
        if (frobber.DetectABI(abi)) {
          if (++i >= max_retries) {
            goto fail;
          }
          PtraceCont(pid);
          std::this_thread::sleep_for(interval);
          PtraceInterrupt(pid);
          continue;
        }
        goto success;
      }
    fail:
      std::cerr << "Failed to locate libpython within timeout period.\n";
      return 1;
    } catch (const FatalException &exc) {
      std::cerr << exc.what() << "\n";
      return 1;
    }
  success:

    const std::chrono::microseconds interval{
        static_cast<long>(sample_rate * 1000000)};
    bool check_end = seconds >= 0;
    auto end = std::chrono::system_clock::now() +
               std::chrono::microseconds(static_cast<long>(seconds * 1000000));
    for (;;) {
      auto now = std::chrono::system_clock::now();
      std::vector<Thread> threads = frobber.GetThreads();

      // Only true for non-GIL stacks that we couldn't find a way to profile
      // Currently this means stripped builds on non-AMD64 archs
      if (threads.empty() && include_idle) {
        idle_count++;
        // Time stamp empty call stacks only if required. Since lots of time
        // the process will be idle, this is a good optimization to have
        if (include_ts) {
          call_stacks.push_back({now, {}});
        }
      }

      for (const auto &thread : threads) {
        call_stacks.push_back({now, thread.frames()});
      }

      if ((check_end) && (now + interval >= end)) {
        break;
      }
      PtraceCont(pid);
      std::this_thread::sleep_for(interval);
      PtraceInterrupt(pid);
    }
    if (!include_ts) {
      PrintFrames(*output, call_stacks, idle_count);
    } else {
      PrintFramesTS(*output, call_stacks);
    }
  } catch (const PtraceException &exc) {
    // If the process terminates early then we just print the stack traces up
    // until that point in time.
    if (!call_stacks.empty() || idle_count) {
      if (!include_ts) {
        PrintFrames(*output, call_stacks, idle_count);
      } else {
        PrintFramesTS(*output, call_stacks);
      }
    } else {
      std::cerr << exc.what() << std::endl;
      return 1;
    }
  } catch (const std::exception &exc) {
    std::cerr << exc.what() << std::endl;
    return 1;
  }
  return 0;
}
