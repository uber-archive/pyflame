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

#define Py_BUILD_CORE
#include <stddef.h>
#include <stdatomic.h>
#include <Python.h>
#include <internal/pystate.h>

// We needd to include pystate.h which needs to include pyatomic.h
// which needs to include stdatomic.h which currently fails to work in C++
// with GCC. We do this in C instead.
//
size_t PYFLAME_PYTHON37_OFFSET = offsetof(_PyRuntimeState, gilstate) +
                                 offsetof(struct _gilstate_runtime_state, tstate_current);
