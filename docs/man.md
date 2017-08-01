% PYFLAME(1)
% Evan Klitzke <evan@eklitzke.org>

# NAME

pyflame - A Ptracing Python Profiler

# SYNOPSIS

**pyflame** [**options**] *PID*

**pyflame** [**options**] [**-t**|**--trace**] *command* [*args*...]

# DESCRIPTION

**pyflame** profiles Python processes using **ptrace**(2) to extract stack
information. There are two modes. In the default mode pyflame will attach to a
running process to get profiling data. If, instead, the **-t** or **--trace**
options are given, pyflame will instead interpret the rest of the command line
as a command to run, and trace it to completion.

The output of **pyflame** is intended to be used with Brendan Gregg's
*flamegraph.pl* script, which can be found on GitHub
at <https://github.com/brendangregg/FlameGraph>.

# GENERAL OPTIONS

**--abi**=*VERSION*
:   Force a particular Python ABI. This is an advanced option, only needed in
    edge cases when profiling embedded Python builds (e.g. uWSGI). The version

**--threads**
:   Enable profiling multi-threaded Python apps.

**-h**, **--help**
:   Display a friendly help message.

**-o**, **--output**=*FILENAME*
:   Write profiling output to *FILENAME* (otherwise stdout is used).

**-s**, **--seconds**=*SECONDS*
:   Profile the process for duration *SECONDS* before detaching. The default is
    to profile for 1 second. This option is not compatible with trace mode.

**-r**, **--rate**=*RATE*
:   Sample the process at this frequency. The argument *RATE* is interpreted as
    a fractional value, measured in seconds. For example, **-r 0.01** would mean
    to sample the process every 0.01 seconds (i.e. every 10 milliseconds). The
    default value for *RATE* is 0.001, which samples every millisecond.

    Note that setting a low value for rate will increase the accuracy of
    profiles, but it also increases the overhead introduced by Pyflame. The
    default frequency used by Pyflame is relatively aggressive; a less
    aggressive value like **-r 0.01** may be more appropriate if you are
    profiling processes in production.

**-t**, **--trace** *command* [*args*...]
:   Run pyflame in trace mode, which traces the child process until completion.
    If used, this must be the final argument (the rest of the arguments will be
    interpreted as a command plus arguments to the command). This is analogous
    to **strace**(1) in its default mode.

**-T**, **--timestamp**
:   Print the timestamp for each stack. This is useful for generating "flame
    chart" profiles.

**-v**, **--version**
:   Print the version.

**-x**, **--exclude-idle**
:   Exclude "idle" time from output.

# ONLINE DOCUMENTATION

You can find the complete documentation online
at: <https://pyflame.readthedocs.io/>. The online documentation is more
comprehensive than this man page, and includes usage examples.

# REPORTING BUGS

If you find any bugs, please create a new issue on
GitHub: <https://github.com/uber/pyflame>
