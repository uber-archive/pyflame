[![Build Status](https://travis-ci.com/uber/pyflame.svg?token=PuYq6s4ssh4VyDYs6Qex&branch=master)](https://travis-ci.com/uber/pyflame)

# Pyflame

Pyflame is a tool for generating
[flame graphs](https://github.com/brendangregg/FlameGraph) for Python processes.
Pyflame works by using the
[ptrace(2)](http://man7.org/linux/man-pages/man2/ptrace.2.html) system call to
analyze the currently-executing stack trace for a Python process.

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

    sudo apt-get install autotools-dev g++ pkg-config python-dev

If you'd like to build a Debian package there's already a `debian/` directory at
the root of this project. We'd like to remove this, as per the
[upstream Debian packaging guidelines](https://wiki.debian.org/UpstreamGuide).
If you can help get this project packaged in Debian please let us know.

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

### Ptrace Permissions Errors

To run Pyflame you'll need appropriate permissions to `PTRACE_ATTACH` the
process. Typically this means that you'll need to invoke `pyflame` as root, or
as the same user as the process you're trying to profile. If you have errors
running it as the correct user then you probably have `ptrace_scope` set to a
value that's too restrictive.

Debian Jessie ships with `ptrace_scope` set to 1 by default, which will prevent
unprivileged users from attaching to already running processes. This will also
manifest by being unable to use `gdb -p` as an unprivileged user by default.

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
# use this if you want "classic" ptrace permissions
sudo sysctl kernel.yama.ptrace_scope=0
```

## Python 3 Support

There is very basic Python 3 support, which you can get by compiling using:

```bash
./configure --with-python=python3
```

This *should* work as long as none of your files have non-ASCII characters in
their names. If you are interested in supporting Unicode file names please
assist us with pull requests.

## Legal and Licensing

Pyflame is [free software](https://www.gnu.org/philosophy/free-sw.en.html)
licensed under the
[Apache License, Version 2.0][].

[Apache License, Version 2.0]: LICENSE
