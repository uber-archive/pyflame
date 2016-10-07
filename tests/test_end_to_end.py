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
def proc(argv, wait_for_pid=True):
    # start the process and wait for it to print its pid... we explicitly do
    # this instead of using the pid attribute so we can ensure that the process
    # is initialized
    proc = subprocess.Popen(argv, stdout=subprocess.PIPE)
    if wait_for_pid:
        proc.stdout.readline()

    try:
        yield proc
    finally:
        proc.kill()


def python_proc(test_file):
    return proc(['python', './tests/%s' % (test_file,)])


@pytest.yield_fixture
def dijkstra():
    with python_proc('dijkstra.py') as p:
        yield p


@pytest.yield_fixture
def sleeper():
    with python_proc('sleeper.py') as p:
        yield p


@pytest.yield_fixture
def exit_early():
    with python_proc('exit_early.py') as p:
        yield p


@pytest.yield_fixture
def not_python():
    with proc(['./tests/sleep.sh'], wait_for_pid=False) as p:
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


def test_exit_early(exit_early):
    proc = subprocess.Popen(['./src/pyflame', '-s', '10', str(exit_early.pid)],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert FLAMEGRAPH_RE.match(line) or IDLE_RE.match(line)


def test_sample_not_python(not_python):
    proc = subprocess.Popen(['./src/pyflame', str(not_python.pid)],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not out
    assert err.startswith(b'Failed to locate libpython')
    assert proc.returncode == 1


def test_trace():
    proc = subprocess.Popen(['./src/pyflame', '-t',
                             'python', 'tests/exit_early.py', '-s'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert FLAMEGRAPH_RE.match(line) or IDLE_RE.match(line)


def test_trace_not_python():
    proc = subprocess.Popen(['./src/pyflame', '-t', './tests/sleep.sh'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not out
    assert err.startswith(b'Failed to locate libpython')
    assert proc.returncode == 1


def test_pyflame_a_pyflame():
    proc = subprocess.Popen(['./src/pyflame', '-t', './src/pyflame'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not out
    assert err.startswith(b'You tried to pyflame a pyflame')
    assert proc.returncode == 1


def test_pyflame_nonexistent_file():
    proc = subprocess.Popen(['./src/pyflame', '-t', '/no/such/file'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not out
    assert b'Child process exited with status' in err
    assert proc.returncode == 1


def test_trace_no_arg():
    proc = subprocess.Popen(['./src/pyflame', '-t'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not out
    assert err.startswith(b'Usage: ')
    assert proc.returncode == 1


def test_sample_no_arg():
    proc = subprocess.Popen(['./src/pyflame'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not out
    assert err.startswith(b'Usage: ')
    assert proc.returncode == 1


def test_sample_extra_args():
    proc = subprocess.Popen(['./src/pyflame', 'foo', 'bar'],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not out
    assert err.startswith(b'Usage: ')
    assert proc.returncode == 1


@pytest.mark.parametrize('pid', [(1,), (0,)])
def test_permission_error(pid):
    # pid 1 = EPERM
    # pid 0 = ESRCH
    proc = subprocess.Popen(['./src/pyflame', str(pid)],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)
    out, err = proc.communicate()
    assert not out
    assert err.startswith(b'Failed to attach to PID')
    assert proc.returncode == 1
