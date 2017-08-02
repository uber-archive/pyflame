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

#include <iostream>
#include <thread>

#include "./prober.h"

using namespace pyflame;

int main(int argc, char **argv) {
  Prober prober;
  int ret = prober.ParseOpts(argc, argv);
  if (ret != -1) {
    return ret;
  }
  if (prober.InitiatePtrace(argv)) {
    return 1;
  }
  PyFrob frobber(prober.pid(), prober.enable_threads());
  if (prober.FindSymbols(&frobber)) {
    return 1;
  }

  // Probe in a loop.
  return prober.ProbeLoop(frobber);
}
