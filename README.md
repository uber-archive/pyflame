[![Build Status](https://api.travis-ci.org/uber/pyflame.svg?branch=master)](https://travis-ci.org/uber/pyflame)

# Pyflame: A Ptracing Profiler For Python

Pyflame is a tool for
generating [flame graphs](https://github.com/brendangregg/FlameGraph) for Python
processes. Pyflame is different from existing Python profilers because it
doesn't require explicit instrumentation: it will work with any running Python
process! Pyflame works by using
the [ptrace(2)](http://man7.org/linux/man-pages/man2/ptrace.2.html) system call
to analyze the currently-executing stack trace for a Python process.

Learn more by reading
[the Uber Engineering blog post about Pyflame](http://eng.uber.com/pyflame/).

![pyflame](https://cloud.githubusercontent.com/assets/2734/17949703/8ef7d08c-6a0b-11e6-8bbd-41f82086d862.png)

## Installing

Pick your poison. Build from source or if available install a prebuilt release.

### Building from source

To build Pyflame you will need a C++ compiler with basic C++11 support. Pyflame
is known to compile on versions of GCC as old as GCC 4.6. You'll also need GNU
Autotools ([GNU Autoconf](https://www.gnu.org/software/autoconf/autoconf.html)
and [GNU Automake](https://www.gnu.org/software/automake/automake.html)) if
you're building from the Git repository.

#### Install build-time dependencies

* Fedora

```bash
# Install build dependencies on Fedora.
sudo dnf install autoconf automake gcc-c++ python-devel libtool
```

* Debian/Ubuntu

```bash
# Install build dependencies on Debian or Ubuntu.
sudo apt-get install autoconf automake autotools-dev g++ pkg-config python-dev libtool
```

#### Compilation

From git you would then compile like so:

```bash
./autogen.sh
./configure      # Plus any options like --prefix.
make
make install
```

If you'd like to build a Debian package there's already a `debian/` directory at
the root of this project. We'd like to remove this, as per the
[upstream Debian packaging guidelines](https://wiki.debian.org/UpstreamGuide).
If you can help get this project packaged in Debian please let us know.

### Installing a pre-built package

#### Ubuntu PPA

The community has setup a PPA for all current Ubuntu releases:
[PPA](https://launchpad.net/~trevorjay/+archive/ubuntu/pyflame).

```bash
sudo apt-add-repository ppa:trevorjay/pyflame
sudo apt-get update
sudo apt-get install pyflame
```

#### Arch Linux

You can install pyflame from [AUR](https://aur.archlinux.org/packages/pyflame-git/).

## Usage

After compiling Pyflame you'll get a small executable called `pyflame` (which
will be in the `src/` directory if you haven't run `make install`). The most
basic usage is:

```bash
# Profile PID for 1s, sampling every 1ms.
pyflame PID
```

The `pyflame` command will send data to stdout that is suitable for using with
Brendan Gregg's `flamegraph.pl` tool (which you can
get [here](https://github.com/brendangregg/FlameGraph)). Therefore a typical
command pipeline might be like this:

```bash
# Generate flame graph for pid 12345; assumes flamegraph.pl is in your $PATH.
pyflame 12345 | flamegraph.pl > myprofile.svg
```

You can also change the sample time and sampling frequency:

```bash
# Profile PID for 60 seconds, sampling every 10ms.
pyflame -s 60 -r 0.10 PID
```

### Trace Mode

Sometimes you want to trace a process from start to finish. An example would be
tracing the run of a test suite. Pyflame supports this use case. To use it, you
invoke Pyflame like this:

```bash
# Trace a given command until completion.
pyflame [regular pyflame options] -t command arg1 arg2...
```

Frequently the value of `command` will actually be `python`, but it could be
something else like `uwsgi` or `py.test`. For instance, here's how Pyflame can
be used to trace its own test suite:

```bash
# Trace the Pyflame test suite, a.k.a. pyflameception!
pyflame -t py.test tests/
```

Beware that when using the trace mode the stdout/stderr of the pyflame process
and the traced process will be mixed. This means if the traced process sends
data to stdout you may need to filter it somehow before sending the output to
`flamegraph.pl`.

### Timestamp ("Flame Chart") Mode

Pyflame can also generate data with timestamps which can be used to
generate ["flame charts"](https://addyosmani.com/blog/devtools-flame-charts/)
that can be viewed in Chrome. This is controlled with the `-T` option.

Use `utils/flame-chart-json` to generate the JSON data required for viewing
Flame Charts using the Chrome CPU profiler.

```bash
Usage: cat <pyflame_output_file> | flame-chart-json > <fc_output>.cpuprofile
(or) pyflame [regular pyflame options] | flame-chart-json > <fc_output>.cpuprofile
```

Then load the resulting .cpuprofile file from chrome CPU profiler to view Flame Chart.

## FAQ

### What Is "(idle)" Time?

From time to time the Python interpreter will have nothing to do other than wait
for I/O to complete. This will typically happen when the Python interpreter is
waiting for network operations to finish. In this scenario Pyflame will report
the time as "idle".

If you don't want to include this time you can use the invocation `pyflame -x`.

### Are BSD / OS X / macOS Supported?

No, these aren't supported. Someone who is proficient with low-level C
programming can probably get BSD to work, as described in issue #3. It is
probably much more difficult to adapt this code to work on OS X/macOS since the
current code assumes that the host
uses [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) files
as the executable file format for the Python interpreter.

### What Are These Ptrace Permissions Errors?

Because it's so powerful, the `ptrace(2)` system call is locked down by default
in various situations by different Linux distributions. In order to use ptrace
these conditions must be met:

 * You must have the
   [`SYS_PTRACE` capability](http://man7.org/linux/man-pages/man7/capabilities.7.html) (which
   is denied by default within Docker images).
 * The kernel must not have `kernel.yama.ptrace_scope` set to a value that is
   too restrictive.

In both scenarios you'll also find that `strace` and `gdb` do not work as
expected.

#### Ptrace Errors Within Docker Containers

By default Docker images do not have the `SYS_PTRACE` capability. When you
invoke `docker run` try using the `--cap-add SYS_PTRACE` option:

```bash
# Allows processes within the Docker container to use ptrace.
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
# Prints the current value for the ptrace_scope setting.
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

#### Ptrace With SELinux

If you're using SELinux,
[you may have problems with ptrace](https://fedoraproject.org/wiki/Features/SELinuxDenyPtrace).
To check if ptrace is disabled:

```bash
# Check if SELinux is denying ptrace.
getsebool deny_ptrace
```

If you'd like to enable it:

```bash
# Enable ptrace under SELinux.
setsebool -P deny_ptrace 0
```

## Python 3 Support

This mostly works: if you have the Python 3 headers installed on your system,
the configure script should detect the presence of Python 3 and use it. Please
report any bugs related to Python 3 detection if you find them (particularly if
you have Python 3 headers installed, but the build system isn't finding them).

There is one known
bug:
[Pyflame can only decode ASCII filenames in Python 3](https://github.com/uber/pyflame/issues/2).
The issue has more details, if you want to help fix it.

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
Unix you will probably only need to modify these files.

You can run the test suite locally like this:

```bash
# Run the Pyflame test suite.
make test
```

## Legal and Licensing

Pyflame is [free software](https://www.gnu.org/philosophy/free-sw.en.html)
licensed under the
[Apache License, Version 2.0][].

[Apache License, Version 2.0]: LICENSE
