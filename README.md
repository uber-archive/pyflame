# Pyflame: A Ptracing Profiler For Python

[![Build Status](https://api.travis-ci.org/uber/pyflame.svg?branch=master)](https://travis-ci.org/uber/pyflame)

Pyflame is a profiling tool that
generates [flame graphs](http://www.brendangregg.com/flamegraphs.html) for
Python. Pyflame is unique among Python profilers, because it does not require
explicit instrumentation: you can use it with any running Python process, no
modifications needed! Pyflame is able to achieve this using the
Linux [ptrace(2)](http://man7.org/linux/man-pages/man2/ptrace.2.html) system
call to grab snapshots of the Python stack. Pyflame is also capable of profiling
embedded Python interpreters, such
as [uWSGI](https://uwsgi-docs.readthedocs.io/en/latest/). Pyflame also fully
supports profiling multi-threaded Python programs.

Pyflame is written in C++, with attention to speed and performance. Pyflame
usually introduces less overhead than the builtin `profile` (or `cProfile`)
modules, and also emits richer profiling data than `profile`. The profiling
overhead of Pyflame is low enough that you can use it to profile live processes
in production.

![pyflame](https://cloud.githubusercontent.com/assets/2734/17949703/8ef7d08c-6a0b-11e6-8bbd-41f82086d862.png)

<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->
**Table of Contents**

- [Pyflame: A Ptracing Profiler For Python](#pyflame-a-ptracing-profiler-for-python)
    - [Installing From Source](#installing-from-source)
        - [Build Dependencies](#build-dependencies)
            - [Debian/Ubuntu](#debianubuntu)
            - [Fedora](#fedora)
        - [Compiling](#compiling)
            - [Creating A Debian Package](#creating-a-debian-package)
        - [Python 3 Support](#python-3-support)
    - [Installing A Pre-Built Package](#installing-a-pre-built-package)
        - [Ubuntu PPA](#ubuntu-ppa)
        - [Arch Linux](#arch-linux)
    - [Usage](#usage)
        - [Attaching To A Running Python Process](#attaching-to-a-running-python-process)
        - [Tracing Python Commands](#tracing-python-commands)
            - [Tracing Programs That Print To Stdout](#tracing-programs-that-print-to-stdout)
        - [Timestamp ("Flame Chart") Mode](#timestamp-flame-chart-mode)
    - [FAQ](#faq)
        - [What Is "(idle)" Time?](#what-is-idle-time)
        - [Are BSD / OS X / macOS Supported?](#are-bsd--os-x--macos-supported)
        - [What Are These Ptrace Permissions Errors?](#what-are-these-ptrace-permissions-errors)
            - [Ptrace Errors Within Docker Containers](#ptrace-errors-within-docker-containers)
            - [Ptrace Errors Outside Docker Containers Or When Not Using Docker](#ptrace-errors-outside-docker-containers-or-when-not-using-docker)
            - [Ptrace With SELinux](#ptrace-with-selinux)
    - [Blog Posts](#blog-posts)
    - [Contributing](#contributing)
        - [Hacking](#hacking)
        - [How Else Can I Help?](#how-else-can-i-help)
    - [Legal and Licensing](#legal-and-licensing)

<!-- markdown-toc end -->

## Installing From Source

To build Pyflame you will need a C++ compiler with basic C++11 support. Pyflame
is known to compile on versions of GCC as old as GCC 4.6. You'll also need GNU
Autotools ([GNU Autoconf](https://www.gnu.org/software/autoconf/autoconf.html)
and [GNU Automake](https://www.gnu.org/software/automake/automake.html)) if
you're building from the Git repository.

### Build Dependencies

#### Debian/Ubuntu

Install the following packages if you are building for Debian or Ubuntu. Note
that you technically only need one of `python-dev` or `python3-dev`, but if you have both
installed then you can use Pyflame to profile both Python 2 and Python 3
processes.

```bash
# Install build dependencies on Debian or Ubuntu.
sudo apt-get install autoconf automake autotools-dev g++ pkg-config python-dev python3-dev libtool make
```

#### Fedora

Again, you technically only need one of `python-devel` and `python3-devel`,
although installing both is recommended.

```bash
# Install build dependencies on Fedora.
sudo dnf install autoconf automake gcc-c++ python-devel python3-devel libtool
```

### Compiling

Once you've installed the appropriate build dependencies (see below), you can
compile Pyflame like so:

```bash
./autogen.sh
./configure      # Plus any options like --prefix.
make
make test        # Optional, test the build! Should take < 1 minute.
make install     # Optional, install into the configure prefix.
```

The Pyflame executable produced by the `make` command will be located at
`src/pyflame`. Note that the `make test` command requires that you have
`virtualenv` installed.

#### Creating A Debian Package

If you'd like to build a Debian package, run the following from the root of your
Pyflame git checkout:

```bash
# Install additional dependencies required for packaging.
sudo apt-get install debhelper dh-autoreconf dpkg-dev

# This create a file named something like ../pyflame_1.3.1_amd64.deb
dpkg-buildpackage -uc -us
```

### Python 3 Support

Pyflame will detect Python 3 headers at build time, and will be compiled with
Python 3 support if these headers are detected. Python 3.4 and 3.5 are known to
work. [Issue #69](https://github.com/uber/pyflame/issues/69) tracks Python 3.6
support. [Issue #77](https://github.com/uber/pyflame/issues/77) tracks
supporting earlier Python 3 releases.

There is one known bug specific to Python
3. [Issue #2](https://github.com/uber/pyflame/issues/2) describes the problem:
Pyflame assumes that Python 3 file names are encoded using ASCII. This is will
only affect you if you actually use non-ASCII code points in your `.py` file
names, which is probably quite uncommon. In principle it is possible to fix this
although a bit tricky; see the linked issue for details, if you're interested in
contributing a patch.

## Installing A Pre-Built Package

Several Pyflame users have created unofficial pre-built packages for different
distros. Uploads of these packages tend to lag the official Pyflame releases, so
you are **strongly encouraged to check the pre-built version** to ensure that it
is not too old. If you want the newest version of Pyflame, build from source.

### Ubuntu PPA

[Trevor Joynson](https://github.com/akatrevorjay) has set up an unofficial PPA
for all current Ubuntu
releases:
[ppa:trevorjay/pyflame](https://launchpad.net/~trevorjay/+archive/ubuntu/pyflame).

```bash
sudo apt-add-repository ppa:trevorjay/pyflame
sudo apt-get update
sudo apt-get install pyflame
```

Note also that you can build your own Debian package easily, using the one
provided in the `debian/` directory of this project.

### Arch Linux

[Oleg Senin](https://github.com/RealFatCat) has added an Arch Linux package
to [AUR](https://aur.archlinux.org/packages/pyflame-git/).

## Usage

Pyflame has two distinct modes: you can attach to a running process, or you can
trace a command from start to finish.

### Attaching To A Running Python Process

The default behavior of Pyflame is to attach to an existing Python process. The
target process is specified via its PID:

```bash
# Profile PID for 1s, sampling every 1ms.
pyflame PID
```

This will print data to stdout in a format that is suitable for usage with
Brendan Gregg's `flamegraph.pl` tool (which you can
get [here](https://github.com/brendangregg/FlameGraph)). A typical command
pipeline might be like this:

```bash
# Generate flame graph for pid 12345; assumes flamegraph.pl is in your $PATH.
pyflame 12345 | flamegraph.pl > myprofile.svg
```

You can also change the sample time with `-s`, and the sampling frequency with
`-r`. Both units are measured in seconds.

```bash
# Profile PID for 60 seconds, sampling every 100ms.
pyflame -s 60 -r 0.1 PID
```

### Tracing Python Commands

Sometimes you want to trace a command from start to finish. An example would be
tracing the run of a test suite or batch job. Pass `-t` as the **last** Pyflame
flag to run in trace mode. Anything after the `-t` flag is interpreted literally
as part of the command to run:

```bash
# Trace a given command until completion.
pyflame [regular pyflame options] -t command arg1 arg2...
```


Often `command` will be `python` or `python3`, but it could be something else,
like `uwsgi` or `py.test`. For instance, here's how Pyflame can be used to trace
its own test suite:

```bash
# Trace the Pyflame test suite, a.k.a. pyflameception!
pyflame -t py.test tests/
```

As described in the docs for attach mode, you can use `-r` to control the
sampling frequency.

#### Tracing Programs That Print To Stdout

By default, Pyflame will send flame graph data to stdout. If the profiled
program is also sending data to stdout, then `flamegraph.pl` will see the output
from both programs, and will get confused. To solve this, use the `-o` option:

```bash
# Trace a process, sending profiling information to profile.txt
pyflame -o profile.txt -t python -c 'for x in range(1000): print(x)'

# Convert profile.txt to a flame graph named profile.svg
flamegraph.pl <profile.txt >profile.svg
```

### Timestamp ("Flame Chart") Mode

Generally we recommend using regular flame graphs, generated by `flamegraph.pl`.
However, Pyflame can also generate data with a special time stamp output format,
useful for
generating ["flame charts"](https://addyosmani.com/blog/devtools-flame-charts/)
(somewhat like an inverted flame graph) that are viewable in Chrome. In some
cases, the flame chart format is easier to
understand.

To generate a flame chart, use `pyflame -T`, and then pass the output to
`utils/flame-chart-json` to convert the output into the JSON format required by
the Chrome CPU profiler:

```bash
# Generate flame chart data viewable in Chrome.
pyflame -T [other pyflame options] | flame-chart-json > foo.cpuprofile
```

Read the
following
[Chrome DevTools article](https://developers.google.com/web/updates/2016/12/devtools-javascript-cpu-profile-migration) for
instructions on loading a `.cpuprofile` file in Chrome 58+.

## FAQ

### What Is "(idle)" Time?

In Python, only one thread can execute Python code at any one time, due to the
Global Interpreter Lock, or GIL. The exception to this rule is that threads can
execute non-Python code (such as IO, or some native libraries such as NumPy)
without the GIL.

By default Pyflame will only profile code that holds the Global Interpreter
Lock. Since this is the only thread that can run Python code, in some sense this
is a more accurate representation of the profile of an application, even when it
is multithreaded. If nothing holds the GIL (so no Python code is executing)
Pyflame will report the time as "idle".

If you don't want to include this time you can use the invocation `pyflame -x`.

If instead you invoke Pyflame with the `--threads` option, Pyflame will take a
snapshot of each thread's stack each time it samples the target process. At the
end of the invocation, the profiling data for each thread will be printed to
stdout sequentially. This gives you a more accurate profile in the sense that
you will see what each thread was trying to do, even if it wasn't actually
scheduled to run.

**Pyflame may "freeze" the target process if you use this option with older
versions of the Linux kernel.** In particular, for this option to work you need
a kernel built with [waitid() ptrace support](https://lwn.net/Articles/688624/).
This change was landed for Linux kernel 4.7. Most Linux distros also backported
this change to older kernels, e.g. this change was backported to the 3.16 kernel
series in 3.16.37 (which is in Debian Jessie's kernel patches). For more
extensive discussion,
see [issue #55](https://github.com/uber/pyflame/issues/55).

One interesting use of this feature is to get a point-in-time snapshot of what
each thread is doing, like so:

```bash
# Get a point-in-time snapshot of what each thread is currently running.
pyflame -s 0 --threads PID
```

### Are BSD / OS X / macOS Supported?

Pyflame uses a few Linux-specific interfaces, so unfortunately it is the only
platform supported right now. Pull requests to add support for other platforms
are very much wanted.

Someone who is proficient with low-level C systems programming can probably get
BSD to work without *too much* difficulty. The necessary work to adapt the code
is described in [Issue #3](https://github.com/uber/pyflame/issues/3).

By comparison, it is probably *much more* work to get Pyflame working on macOS.
The current code assumes that the host
uses [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)
object/executable files. Apple uses a different object file format,
called [Mach-O](https://en.wikipedia.org/wiki/Mach-O), so porting Pyflame to
macOS would entail doing all of the work to port Pyflame to BSD, *plus*
additional work to parse Mach-O object files. That said, the Mach-O format is
documented online (e.g. [here](https://lowlevelbits.org/parsing-mach-o-files/)),
so a sufficiently motivated person could get macOS support working.

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

## Blog Posts

If you write a blog post about Pyflame, we may include it here. Some existing
blog posts on Pyflame include:

 * [Pyflame: Uber Engineering's Ptracing Profiler For Python](http://eng.uber.com/pyflame/) by
   Evan Klitzke (2016-09)
 * [Pyflame Dual Interpreter Mode](https://eklitzke.org/pyflame-dual-interpreter-mode) by
   Evan Klitzke (2016-10)
 * [Using Uber's Pyflame and Logs to Tackle Scaling Issues](https://benbernardblog.com/using-ubers-pyflame-and-logs-to-tackle-scaling-issues/) by
   Benoit Bernard (2017-02)

## Contributing

We love getting pull requests and bug reports! This section outlines some ways
you can contribute to Pyflame.

### Hacking

This section will explain the Pyflame code for people who are interested in
contributing source code patches.

The code style in Pyflame (mostly) conforms to
the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
Additionally, all of the source code is formatted
with [clang-format](http://clang.llvm.org/docs/ClangFormat.html). There's a
`.clang-format` file checked into the root of this repository which will make
`clang-format` do the right thing. Different clang releases may format the
source code slightly differently, as the formatting rules are updated within
clang itself. Therefore you should eyeball the changes made when formatting,
especially if you have an older version of clang.

The Linux-specific code is be mostly restricted to the files `src/aslr.*`,
`src/namespace.*`, and `src/ptrace.*`. If you want to port Pyflame to another
Unix, you will probably only need to modify these files.

You can run the test suite locally like this:

```bash
# Run the Pyflame test suite.
make test
```

### How Else Can I Help?

Patches are not the only way to contribute to Pyflame! Bug reports are very
useful as well. If you file a bug, make sure you tell us the exact version of
Python you're using, and how to reproduce the issue.

## Legal and Licensing

Pyflame is [free software](https://www.gnu.org/philosophy/free-sw.en.html)
licensed under the
[Apache License, Version 2.0][].

[Apache License, Version 2.0]: LICENSE
