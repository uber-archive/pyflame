# Copyright 2016 Uber Technologies, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import contextlib
import pytest
import subprocess
import re


IDLE_RE = re.compile(r'^\(idle\) \d+$')
FLAMEGRAPH_RE = re.compile(r'^\S+ \d+$')


@contextlib.contextmanager
def proc(test_file):
    # start the process and wait for it to print its pid... we explicitly do
    # this instead of using the pid attribute so we can ensure that the process
    # is initialized
    proc = subprocess.Popen(
        ['python', './tests/%s' % (test_file,)], stdout=subprocess.PIPE)
    proc.stdout.readline()

    try:
        yield proc
    finally:
        proc.kill()


@pytest.yield_fixture
def dijkstra():
    with proc('dijkstra.py') as p:
        yield p


@pytest.yield_fixture
def sleeper():
    with proc('sleeper.py') as p:
        yield p


def test_monitor(dijkstra):
    """Basic test for the monitor mode."""
    proc = subprocess.Popen(['./src/pyflame', str(dijkstra.pid)],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True)
    out, err = proc.communicate()
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert FLAMEGRAPH_RE.match(line) is not None


def test_idle(sleeper):
    """Basic test for idle processes."""
    proc = subprocess.Popen(['./src/pyflame', str(sleeper.pid)],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True)
    out, err = proc.communicate()
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    has_idle = False
    for line in lines:
        assert FLAMEGRAPH_RE.match(line) is not None
        if IDLE_RE.match(line):
            has_idle = True
    assert has_idle


def test_exclude_idle(sleeper):
    """Basic test for idle processes."""
    proc = subprocess.Popen(['./src/pyflame', '-x', str(sleeper.pid)],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            universal_newlines=True)
    out, err = proc.communicate()
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert FLAMEGRAPH_RE.match(line) is not None
        assert not IDLE_RE.match(line)
