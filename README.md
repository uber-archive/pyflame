# Pyflame

Pyflame is a tool for generating
[flame graphs](https://github.com/brendangregg/FlameGraph) for Python processes.
Pyflame works by using the
[ptrace(2)](http://man7.org/linux/man-pages/man2/ptrace.2.html) system call to
analyze the currently-executing stack trace for a Python process.

## Installing

To build Pyflame you will need a C++ compiler with C++11 support, and GNU
Autotools ([GNU Autoconf](https://www.gnu.org/software/autoconf/autoconf.html)
and [GNU Automake](https://www.gnu.org/software/automake/automake.html)). Then
you can build it with an invocation like:

```bash
./autogen.sh
./configure      # plus any options like --prefix
make
make install
```

If you'd like to build a Debian package there's already a `debian/` directory at
the root of this project. We'd like to remove this, as per the
[upstream Debian packaging guidelines](https://wiki.debian.org/UpstreamGuide).
If you can help get this project packaged in Debian please let us know.

## Usage

After compiling Pyflame you'll get a small executable called `pyflame`. The most
basic usage is:

```bash
# profile a process for 1s, sampling every 1ms
pyflame PID
```

The `pyflame` command will send data to stdout that is suitable for using with
Brendan Gregg's `flamegraph.pl` command (which you can get
[here](https://github.com/brendangregg/FlameGraph)). Therefore a typical command
pipeline might be like this:

```bash
# assuming flamegraph.pl is in your $PATH
pyflame 12345 | flamegraph.pl > myprofile.svg
```

You can also change the sample time and sampling frequency:

```bash
# profile for 60 seconds, sampling every 10ms
pyflame -s 60 -r 0.10 PID
```

### Ptrace Permissions Errors

To run Pyflame you'll need appropriate permissions to `PTRACE_ATTACH` the
process. Typically this means that you'll need to invoke `pyflame` as root, or
as the same user as the process you're trying to profile. If you have errors
running it as the correct user then you probably have `ptrace_scope` set to a
value that's too restrictive.

Debian Jessie ships with a default restrictive `ptrace_scope`. This will also
manifest by being unable to use `gdb -p` as an unprivileged user by default.

To see the current value:

```bash
sysctl kernel.yama.ptrace_scope
```

If you see a value other than 0 you may need to change it. Note that by doing
this you'll weaken the security of your system. Please read
[the relevant kernel documentation](https://www.kernel.org/doc/Documentation/security/Yama.txt)
for a comprehensive discussion of the possible settings and how they work. If
you want to completely disable ptrace restrictions you can run:

```bash
sudo sysctl kernel.yama.ptrace_scope=0
```

## License

Pyflame is free software distributed under the terms of the
[Apache License, version 2.0](http://www.apache.org/licenses/LICENSE-2.0). You
should receive a copy of this license along with Pyflame.
