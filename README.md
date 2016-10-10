[![Build Status](https://api.travis-ci.org/uber/pyflame.svg)](https://travis-ci.org/uber/pyflame)

# Pyflame

Pyflame is a tool for
generating [flame graphs](https://github.com/brendangregg/FlameGraph) for Python
processes. Pyflame is different from existing Python profilers because it
doesn't require explicit instrumentation -- it will work with any running Python
process! Pyflame works by using
the [ptrace(2)](http://man7.org/linux/man-pages/man2/ptrace.2.html) system call
to analyze the currently-executing stack trace for a Python process.

Read the Uber Engineering blog post about Pyflame [here](http://eng.uber.com/pyflame/).

![pyflame](https://cloud.githubusercontent.com/assets/2734/17949703/8ef7d08c-6a0b-11e6-8bbd-41f82086d862.png)

## Installing

To build Pyflame you will need a C++ compiler with basic C++11 support. Pyflame
is known to compile on versions of GCC as old as GCC 4.6. You'll also need GNU
Autotools ([GNU Autoconf](https://www.gnu.org/software/autoconf/autoconf.html)
and [GNU Automake](https://www.gnu.org/software/automake/automake.html)) if
you're building from the git repository.

From git you would compile like so:

```bash
./autogen.sh
./configure      # plus any options like --prefix
make
make install
```

### Fedora

The following command should install the necessary packages to build on Fedora:

    sudo dnf install autoconf automake gcc-c++ python-devel

### Debian

The following command should install the necessary packages to build on Debian
(or Ubuntu):

    sudo apt-get install autoconf autotools-dev g++ pkg-config python-dev

If you'd like to build a Debian package there's already a `debian/` directory at
the root of this project. We'd like to remove this, as per the
[upstream Debian packaging guidelines](https://wiki.debian.org/UpstreamGuide).
If you can help get this project packaged in Debian please let us know.

### Arch Linux

You can install pyflame from [AUR](https://aur.archlinux.org/packages/pyflame-git/).

## Usage

After compiling Pyflame you'll get a small executable called `pyflame`. The most
basic usage is:

```bash
# profile PID for 1s, sampling every 1ms
pyflame PID
```

The `pyflame` command will send data to stdout that is suitable for using with
Brendan Gregg's `flamegraph.pl` command (which you can get
[here](https://github.com/brendangregg/FlameGraph)). Therefore a typical command
pipeline might be like this:

```bash
# generate flame graph for pid 12345; assumes flamegraph.pl is in your $PATH
pyflame 12345 | flamegraph.pl > myprofile.svg
```

You can also change the sample time and sampling frequency:

```bash
# profile PID for 60 seconds, sampling every 10ms
pyflame -s 60 -r 0.10 PID
```

### Trace Mode

Sometimes you want to trace a process from start to finish. An example would be
tracing the run of a test suite. Pyflame supports this use case. To use it, you
invoke Pyflame like this:

    pyflame [regular options] -t command arg1 arg2...

Frequently the value of `command` will actually be `python`, but it could be
something else like `uwsgi` or `py.test`. For instance, here's how Pyflame can
be used to trace its own test suite (a.k.a. "pyflameception"):

    pyflame -t py.test tests/

Beware that when using the trace mode the stdout/stderr of the pyflame process
and the traced process will be mixed. This means if the traced process sends
data to stdout you may need to filter it somehow before sending the output to
`flamegraph.pl`.

### Timestamp ("Flame Chart") Mode

Pyflame can also generate data with timestamps which can be used to
generate ["flame charts"](https://addyosmani.com/blog/devtools-flame-charts/)
that can be viewed in Chrome. This is controlled with the `-T` option.

**TODO**: Document how to load these in a browser.

## FAQ

### What Is "(idle)" Time?

From time to time the Python interpreter will have nothing to do other than wait
for I/O to complete. This will typically happen when the Python interpreter is
waiting for network operations to finish. When that happens Pyflame will report
the time as "idle".

If you don't want to include this time you can use the invocation `pyflame -x`.

### What Are These Ptrace Permissions Errors?

The short version is that the `ptrace(2)` system call is locked down by default
in certain situations. In order to use ptrace two conditions need to be met:

 * You must have the `SYS_PTRACE` capability (which is denied by default within
   Docker images).
 * The kernel must not have `kernel.yama.ptrace_scope` set to a value that is
   too restrictive.

In both scenarios you'll also find that `strace` and `gdb` do not work as
expected.

#### Ptrace Errors Within Docker Containers

By default Docker images do not have
the
[`SYS_PTRACE` capability](http://man7.org/linux/man-pages/man7/capabilities.7.html).
When you invoke `docker run` try using this option:

```bash
docker run --cap-add SYS_PTRACE ...
```

You can also use [capsh(1)](http://man7.org/linux/man-pages/man1/capsh.1.html)
to list your current capabilities:

```bash
# You should see cap_sys_ptrace in the "Bounding set".
capsh --print
```

Further note that by design you do not need to run Pyflame from within a Docker
container. If you have sufficient permissions (i.e. you are root, or the same
UID as the Docker process) Pyflame can be run from outside of the container and
inspect a process inside the container. That said, Pyflame will certainly work
within containers if that's how you want to use it.

#### Ptrace Errors Outside Docker Containers Or When Not Using Docker

If you're not in a Docker container, or you're not using Docker at all, ptrace
permissions errors are likely related to you having too restrictive a value set
for the `kernel.yama.ptrace_scope` sysfs knob.

Debian Jessie ships with `ptrace_scope` set to 1 by default, which will prevent
unprivileged users from attaching to already running processes.

To see the current value of this setting:

```bash
sysctl kernel.yama.ptrace_scope
```

If you see a value other than 0 you may want to change it. Note that by doing
this you'll affect the security of your system. Please read
[the relevant kernel documentation](https://www.kernel.org/doc/Documentation/security/Yama.txt)
for a comprehensive discussion of the possible settings and what you're
changing. If you want to completely disable the ptrace settings and get
"classic" permissions (i.e. root can ptrace anything, unprivileged users can
ptrace processes with the same user id) then use:

```bash
# Use this if you want "classic" ptrace permissions.
sudo sysctl kernel.yama.ptrace_scope=0
```

### Are BSD / OS X / macOS Supported?

No, these aren't supported. Someone who is proficient with low-level C
programming can probably get BSD to work, as described in issue #3. It is
probably *extremely* difficult to adapt this code to work on OS X/macOS since
the current code assumes that the host
uses [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) files.

## Python 3 Support

Pyflame will work with Python 3 as long as your file names contain only ASCII
characters (which will be the case for most people). To build with Python 3
support, compile using:

```bash
./configure --with-python=python3
```

The Travis CI test suite is also configured to test Pyflame under Python 3.

## Hacking

This section will explain the Pyflame code for people who are interested in
contributing source code patches.

The code style in Pyflame (mostly) conforms to
the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
Additionally, all of the source code is formatted
with [clang-format](http://clang.llvm.org/docs/ClangFormat.html). There's a
`.clang-format` file checked into the root of this repository which will make
`clang-format` do the right thing.

The Linux-specific code is be mostly restricted to the files `src/aslr.*`,
`src/namespace.*`, and `src/ptrace.*`. If you want to port Pyflame to another
Unix you will probably only need to modify these files. In principle you can
probably port Pyflame to macOS (née OS X) if you modify `src/symbol.*` to work
with [Mach-O](https://en.wikipedia.org/wiki/Mach-O) executables, but this is
probably pretty challenging.

You can run the test suite locally like this:

    make test

## Legal and Licensing

Pyflame is [free software](https://www.gnu.org/philosophy/free-sw.en.html)
licensed under the
[Apache License, Version 2.0][].

[Apache License, Version 2.0]: LICENSE
