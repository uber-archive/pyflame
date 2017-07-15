[![Build Status](https://api.travis-ci.org/uber/pyflame.svg?branch=master)](https://travis-ci.org/uber/pyflame) [![Docs Status](https://readthedocs.org/projects/pyflame/badge/?version=latest)](http://pyflame.readthedocs.io/en/latest/?badge=latest)

# Pyflame: A Ptracing Profiler For Python

Pyflame is a unique profiling tool that
generates [flame graphs](http://www.brendangregg.com/flamegraphs.html) for
Python. Pyflame is the only Python profiler based on the
Linux [ptrace(2)](http://man7.org/linux/man-pages/man2/ptrace.2.html) system
call. This allows it to take snapshots of the Python call stack without explicit
instrumentation, meaning you can profile a program without modifying its source
code! Pyflame is capable of profiling embedded Python interpreters
like [uWSGI](https://uwsgi-docs.readthedocs.io/en/latest/). It fully supports
profiling multi-threaded Python programs.

Pyflame is written in C++, with attention to speed and performance. Pyflame
usually introduces less overhead than the builtin `profile` (or `cProfile`)
modules, and also emits richer profiling data. The profiling overhead is low
enough that you can use it to profile live processes in production.

![pyflame](https://cloud.githubusercontent.com/assets/2734/17949703/8ef7d08c-6a0b-11e6-8bbd-41f82086d862.png)

## Instructions

Please find the **full documentation** at: https://pyflame.readthedocs.io/

### Building And Installing

The full documentation for building Pyflame
is [here](https://pyflame.readthedocs.io/en/latest/installation.html). But
here's a quick guide for those who know what they're doing.

For Debian/Ubuntu, install the following:

```bash
# Install build dependencies on Debian or Ubuntu.
sudo apt-get install autoconf automake autotools-dev g++ pkg-config python-dev python3-dev libtool make
```

Once you've got the build dependencies installed:

```bash
./autogen.sh
./configure
make
```

The `make` command will produce an executable at `src/pyflame` that you can run
and use.

### Using Pyflame

The full documentation for using Pyflame
is [here](https://pyflame.readthedocs.io/en/latest/usage.html). But
here's a quick recap:

```bash
# Attach to PID 12345 and profile it for 1 second
pyflame 12345

# Attach to PID 768 and profile it for 5 seconds, sampling every 0.01 seconds
pyflame -s 5 -r 0.01 768

# Run py.test against tests/, emitting sample data to prof.txt
pyflame -o prof.txt -t py.test tests/
```

In all of these cases you will get flame graph data on stdout (or to a file if
you used `-o`). This data is in the format expected by `flamegraph.pl`, which
you can find [here](https://github.com/brendangregg/FlameGraph).

## FAQ

The full FAQ is [here](https://pyflame.readthedocs.io/en/latest/faq.html).

### What's The Deal With (idle) Time?

Full
answer
[here](https://pyflame.readthedocs.io/en/latest/faq.html#what-is-idle-time).
**tl;dr** use the `-x` option if you find the (idle) output annoying.

### What About These Ptrace Errors?

See [here](https://pyflame.readthedocs.io/en/latest/faq.html#what-are-these-ptrace-permissions-errors).

### How Do I Profile Threaded Applications?

Use the `--threads` option.
