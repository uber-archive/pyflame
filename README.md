# Pyflame: A Ptracing Profiler For Python

[![Build Status](https://api.travis-ci.org/uber/pyflame.svg?branch=master)](https://travis-ci.org/uber/pyflame) [![Docs Status](https://readthedocs.org/projects/pyflame/badge/?version=latest)](http://pyflame.readthedocs.io/en/latest/?badge=latest) [![COPR Status](https://copr.fedorainfracloud.org/coprs/eklitzke/pyflame/package/pyflame/status_image/last_build.png)](https://copr.fedorainfracloud.org/coprs/eklitzke/pyflame/)

Pyflame is a high performance profiling tool that
generates [flame graphs](http://www.brendangregg.com/flamegraphs.html) for
Python. Pyflame is implemented in C++, and uses the
Linux [ptrace(2)](http://man7.org/linux/man-pages/man2/ptrace.2.html) system
call to collect profiling information. It can take snapshots of the Python call
stack without explicit instrumentation, meaning you can profile a program
without modifying its source code. Pyflame is capable of profiling embedded
Python interpreters like [uWSGI](https://uwsgi-docs.readthedocs.io/en/latest/).
It fully supports profiling multi-threaded Python programs.

Pyflame usually introduces significantly less overhead than the builtin
`profile` (or `cProfile`) modules, and emits richer profiling data. The
profiling overhead is low enough that you can use it to profile live processes
in production.

**Full Documentation:** https://pyflame.readthedocs.io

![pyflame](https://cloud.githubusercontent.com/assets/2734/17949703/8ef7d08c-6a0b-11e6-8bbd-41f82086d862.png)

## Quickstart

### Building And Installing

For Debian/Ubuntu, install the following:

```bash
# Install build dependencies on Debian or Ubuntu.
sudo apt-get install autoconf automake autotools-dev g++ pkg-config python-dev python3-dev libtool make
```

Once you have the build dependencies installed:

```bash
./autogen.sh
./configure
make
```

The `make` command will produce an executable at `src/pyflame` that you can run
and use.

Or you can use docker to build pyflame
```base```
sudo docker build --tag pyflame .
sudo docker run -it -v $(pwd):/root/pyflame pyflame /bin/bash -c "cd /root/pyflame;./autogen.sh;./configure;make"
```
This will also produce the executable at `src/pyflame`, which support py2.6/2.7/3.4/3.5/3.6/3.7

Optionally, if you have `virtualenv` installed, you can test the executable you
produced using `make check`.

### Using Pyflame

The full documentation for using Pyflame
is [here](https://pyflame.readthedocs.io/en/latest/usage.html). But
here's a quick guide:

```bash
# Attach to PID 12345 and profile it for 1 second
pyflame -p 12345

# Attach to PID 768 and profile it for 5 seconds, sampling every 0.01 seconds
pyflame -s 5 -r 0.01 -p 768

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
tl;dr: use the `-x` flag to suppress (idle) output.

### What About These Ptrace Errors?

See [here](https://pyflame.readthedocs.io/en/latest/faq.html#what-are-these-ptrace-permissions-errors).

### How Do I Profile Threaded Applications?

Use the `--threads` option.

### Is There A Way To Just Dump Stack Traces?

Yes, use the `-d` option.
