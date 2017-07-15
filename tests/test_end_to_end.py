# -*- coding: utf-8 -*-

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
import re
import subprocess
import sys

IDLE_RE = re.compile(r'^\(idle\) \d+$')
FLAMEGRAPH_RE = re.compile(r'^(.+) (\d+)$')
TS_IDLE_RE = re.compile(r'\(idle\)')
# Matches strings of the form
# './tests/sleeper.py:<module>:31;./tests/sleeper.py:main:26;'
TS_FLAMEGRAPH_RE = re.compile(r'[^[^\d]+\d+;]*')
TS_RE = re.compile(r'\d+')
SLEEP_A_RE = re.compile(r'.*:sleep_a:.*')
SLEEP_B_RE = re.compile(r'.*:sleep_b:.*')


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


def python_command():
    return 'python%d' % (sys.version_info[0], )


def python_proc(test_file):
    return proc([python_command(), './tests/%s' % (test_file, )])


@pytest.yield_fixture
def dijkstra():
    with python_proc('dijkstra.py') as p:
        yield p


@pytest.yield_fixture
def sleeper():
    with python_proc('sleeper.py') as p:
        yield p

if sys.version_info >= (3,3):
    @pytest.yield_fixture
    def unicode_sleeper():
        with python_proc('sleeper_ユニコード.py') as p:
            yield p

@pytest.yield_fixture
def threaded_sleeper():
    with python_proc('threaded_sleeper.py') as p:
        yield p


@pytest.yield_fixture
def threaded_busy():
    with python_proc('threaded_busy.py') as p:
        yield p


@pytest.yield_fixture
def exit_early():
    with python_proc('exit_early.py') as p:
        yield p


@pytest.yield_fixture
def not_python():
    with proc(['./tests/sleep.sh'], wait_for_pid=False) as p:
        yield p


def assert_flamegraph(line, allow_idle=False):
    if allow_idle and IDLE_RE.match(line):
        return
    m = FLAMEGRAPH_RE.match(line)
    assert m is not None
    parts, count = m.groups()
    count = int(count, 10)
    assert count >= 1
    for part in parts.split(';'):
        fname, func, line_num = part.split(':')
        line_num = int(line_num, 10)

        # Make a best effort to sanity check the line number. This logic could
        # definitely be improved, since right now an off-by-one error wouldn't
        # be caught by the test suite.
        if fname.startswith('./tests/'):
            assert 1 <= line_num < 300


def communicate(proc):
    out, err = proc.communicate()
    if isinstance(out, bytes):
        out = out.decode('utf-8')
    if isinstance(err, bytes):
        err = err.decode('utf-8')
    return out, err


def test_monitor(dijkstra):
    """Basic test for the monitor mode."""
    proc = subprocess.Popen(
        ['./src/pyflame', str(dijkstra.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True)
    out, err = communicate(proc)
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert_flamegraph(line)


def test_non_gil(sleeper):
    """Basic test for non-GIL/native code processes."""
    proc = subprocess.Popen(
        ['./src/pyflame', str(sleeper.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True)
    out, err = communicate(proc)
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert_flamegraph(line, allow_idle=True)


def test_threaded(threaded_sleeper):
    """Basic test for non-GIL/native code processes."""
    proc = subprocess.Popen(
        ['./src/pyflame', '--threads',
         str(threaded_sleeper.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True)
    out, err = communicate(proc)
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    a_count = 0
    b_count = 0
    for line in lines:
        if SLEEP_A_RE.match(line):
            assert_flamegraph(line)
            a_count += 1
        elif SLEEP_B_RE.match(line):
            assert_flamegraph(line)
            b_count += 1

    # We must see both threads.
    assert a_count > 0
    assert b_count > 0

    # We should see them both *about* the same number of times.
    small = float(min(a_count, b_count))
    big = float(max(a_count, b_count))
    assert (small / big) >= 0.5


def test_unthreaded(threaded_busy):
    """Test only one process is profiled by default."""
    proc = subprocess.Popen(
        ['./src/pyflame', '-s', '0',
         str(threaded_busy.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True)
    out, err = communicate(proc)
    assert not err
    assert proc.returncode == 0
    lines = out.strip().split('\n')
    assert len(lines) == 1


def test_exclude_idle(sleeper):
    """Basic test for idle processes."""
    proc = subprocess.Popen(
        ['./src/pyflame', '-x', str(sleeper.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True)
    out, err = communicate(proc)
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert_flamegraph(line)


if sys.version_info >= (3,3):
    def test_utf8_output(unicode_sleeper):
        proc = subprocess.Popen(
            ['./src/pyflame', '-x', str(unicode_sleeper.pid)],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            universal_newlines=True)
        out, err = communicate(proc)
        assert not err
        assert proc.returncode == 0

        # The output is decoded assuming UTF-8. So here we check if we can
        # find our function names again.
        func_names = ["låtìÑ1", "وظيفة", "日本語はどうですか", "មុខងារ", "ฟังก์ชัน"]

        for f in func_names:
            assert f in out, "Could not find function '{}' in output".format(f)

        lines = out.split('\n')
        assert lines.pop(-1) == ''  # output should end in a newline
        for line in lines:
            assert_flamegraph(line)


def test_exit_early(exit_early):
    proc = subprocess.Popen(
        ['./src/pyflame', '-s', '10',
         str(exit_early.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert_flamegraph(line, allow_idle=True)


def test_sample_not_python(not_python):
    proc = subprocess.Popen(
        ['./src/pyflame', str(not_python.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not out
    assert err.startswith('Failed to locate libpython')
    assert proc.returncode == 1


@pytest.mark.parametrize('force_abi', [(False, ), (True, )])
def test_trace(force_abi):
    args = ['./src/pyflame']
    if force_abi:
        abi_string = '%d%d' % sys.version_info[:2]
        args.extend(['--abi', abi_string])
    args.extend(['-t', python_command(), 'tests/exit_early.py', '-s'])

    proc = subprocess.Popen(
        args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert_flamegraph(line, allow_idle=True)


def test_trace_not_python():
    proc = subprocess.Popen(
        ['./src/pyflame', '-t', './tests/sleep.sh'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not out
    assert err.startswith('Failed to locate libpython')
    assert proc.returncode == 1


def test_pyflame_a_pyflame():
    proc = subprocess.Popen(
        ['./src/pyflame', '-t', './src/pyflame'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not out
    assert err.startswith('You tried to pyflame a pyflame')
    assert proc.returncode == 1


def test_pyflame_nonexistent_file():
    proc = subprocess.Popen(
        ['./src/pyflame', '-t', '/no/such/file'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not out
    assert 'Child process exited with status' in err
    assert proc.returncode == 1


def test_trace_no_arg():
    proc = subprocess.Popen(
        ['./src/pyflame', '-t'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not out
    assert err.startswith('Usage: ')
    assert proc.returncode == 1


def test_sample_no_arg():
    proc = subprocess.Popen(
        ['./src/pyflame'], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not out
    assert err.startswith('Usage: ')
    assert proc.returncode == 1


def test_sample_extra_args():
    proc = subprocess.Popen(
        ['./src/pyflame', 'foo', 'bar'],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not out
    assert err.startswith('Usage: ')
    assert proc.returncode == 1


@pytest.mark.parametrize('pid', [(1, ), (0, )])
def test_permission_error(pid):
    # pid 1 = EPERM
    # pid 0 = ESRCH
    proc = subprocess.Popen(
        ['./src/pyflame', str(pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE)
    out, err = communicate(proc)
    assert not out
    assert err.startswith('Failed to attach to PID')
    assert proc.returncode == 1


def test_include_ts(sleeper):
    """Basic test for timestamp processes."""
    proc = subprocess.Popen(
        ['./src/pyflame', '-T', str(sleeper.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True)
    out, err = proc.communicate()
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert TS_FLAMEGRAPH_RE.match(line) or TS_RE.match(
            line) or TS_IDLE_RE.match(line)


def test_include_ts_exclude_idle(sleeper):
    """Basic test for timestamp processes."""
    proc = subprocess.Popen(
        ['./src/pyflame', '-T', '-x',
         str(sleeper.pid)],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True)
    out, err = proc.communicate()
    assert not err
    assert proc.returncode == 0
    lines = out.split('\n')
    assert lines.pop(-1) == ''  # output should end in a newline
    for line in lines:
        assert not TS_IDLE_RE.match(line)
        assert TS_FLAMEGRAPH_RE.match(line) or TS_RE.match(line)
