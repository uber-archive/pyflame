% PYFLAME(1)
% Evan Klitzke <evan@eklitzke.org>

# NAME

pyflame - A Ptracing Python Profiler

# SYNOPSIS

**pyflame** [**options**] [**-p**|**--pid**] *PID*

**pyflame** [**options**] [**-t**|**--trace**] *command* [*args*...]

# DESCRIPTION

**pyflame** is a Python profiler that created flame graphs. It uses
**ptrace**(2) to extract stack information. The output of **pyflame** is
intended to be used with Brendan Gregg's *flamegraph.pl* script, which can be
found on GitHub at <https://github.com/brendangregg/FlameGraph>.

# GENERAL OPTIONS

There are two invocation forms. When **-p** *PID* is used, pyflame will attach
to the running process specified by *PID* to collect profiling data. The meaning
of this option is analogous to its meaning in commands like **strace**(1) or
**gdb**(1).

When **-t** is given, pyflame will instead go into "trace mode". In this mode,
it interprets the rest of the command line as a command to run, and traces the
command to completion. This is analogous to how **strace**(1) works when a PID
is not specified.

**-h**, **--help**
:   Display a friendly help message.

**-o**, **--output**=*FILENAME*
:   Write profiling output to *FILENAME* (otherwise stdout is used).

**-p**, **--pid**=*PID*
:   Specify which *PID* to trace.

    Older versions of pyflame received *PID* as a positional argument, where
    *PID* was interpreted as the last argument. This usage mode still works, but
    is considered deprecated. You should use **-p** or **--pid** when specifying
    *PID*.

**-s**, **--seconds**=*SECONDS*
:   Profile the process for duration *SECONDS* before detaching. The default is
    to profile for 1 second. This option is not compatible with trace mode.

**-r**, **--rate**=*RATE*
:   Sample the process at this frequency. The argument *RATE* is interpreted as
    a fractional value, measured in seconds. For example, **-r 0.01** would mean
    to sample the process every 0.01 seconds (i.e. every 10 milliseconds). The
    default value for *RATE* is 0.001, which samples every millisecond.

    Note that setting a low value for rate will increase the accuracy of
    profiles, but it also increases the overhead introduced by pyflame. The
    default frequency used by pyflame is relatively aggressive; a less
    aggressive value like **-r 0.01** may be more appropriate if you are
    profiling processes in production.

**-t**, **--trace** *command* [*args*...]
:   Run pyflame in trace mode, which traces the child process until completion.
    If used, this must be the final argument (the rest of the arguments will be
    interpreted as a command plus arguments to the command). This is analogous
    to **strace**(1) in its default mode.

**-v**, **--version**
:   Print the version.

**-x**, **--exclude-idle**
:   Exclude "idle" time from output.

**--threads**
:   Enable profiling multi-threaded Python apps.

## ADVANCED OPTIONS

The following options are less commonly used.

**--abi**=*VERSION*
:   Force a particular Python ABI. This option should only be needed in edge
    cases when profiling embedded Python builds (e.g. uWSGI), and only if
    pyflame doesn't automatically detect the correct ABI. *VERSION* should be a
    two digit integer consisting of the Python major and minor version, e.g. 27
    for Python 2.7 or 36 for Python 3.6.

**--flamechart**
:   Print the timestamp for each stack. This is useful for generating "flame
    chart" profiles. Generally regular flame graphs are encouraged, since the
    timestamp flame charts are harder to use.

# ONLINE DOCUMENTATION

You can find the complete documentation online
at: <https://pyflame.readthedocs.io/>. The online documentation is more
comprehensive than this man page, and includes usage examples.

# REPORTING BUGS

If you find any bugs, please create a new issue on
GitHub: <https://github.com/uber/pyflame>
