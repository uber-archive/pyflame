FAQ
===

What Python Versions Are Supported?
-----------------------------------

Python 2 is tested with Python 2.6 and 2.7. Earlier versions of Python 2 are
likely to work as well, but have not been tested.

Python 3 is tested with Python 3.4, 3.5, and 3.6. Python 3.6 introduces a new
ABI for the ``PyCodeObject`` type, so Pyflame only supports the Python 3
versions that header files were available for when Pyflame was compiled.

It's possible for Pyflame to get confused about what Python version the target
process is when profiling an embedded Python build, such as uWSGI. If you run
into this issue, use the ``--abi`` option to force a particular Python ABI.

What Is "(idle)" Time?
----------------------

In Python, only one thread can execute Python code at any one time, due to the
Global Interpreter Lock, or GIL. The exception to this rule is that threads can
execute non-Python code (such as IO, or some native libraries such as NumPy)
without the GIL.

By default Pyflame will only profile code that holds the Global Interpreter
Lock. Since this is the only thread that can run Python code, in some sense this
is a more accurate representation of the profile of an application, even when it
is multithreaded. If nothing holds the GIL (so no Python code is executing)
Pyflame will report the time as "idle".

If you don't want to include this time you can use the invocation ``pyflame
-x``.

If instead you invoke Pyflame with the ``--threads`` option, Pyflame will take a
snapshot of each thread's stack each time it samples the target process. At the
end of the invocation, the profiling data for each thread will be printed to
stdout sequentially. This gives you a more accurate profile in the sense that
you will see what each thread was trying to do, even if it wasn't actually
scheduled to run.

**Pyflame may "freeze" the target process if you use this option with older
versions of the Linux kernel.** In particular, for this option to work you need
a kernel built with `waitid() ptrace support
<https://lwn.net/Articles/688624/>`__. This change was landed for Linux kernel
4.7. Most Linux distros also backported this change to older kernels, e.g. this
change was backported to the 3.16 kernel series in 3.16.37 (which is in Debian
Jessie's kernel patches). For more extensive discussion, see `issue #55
<https://github.com/uber/pyflame/issues/55>`__.

One interesting use of this feature is to get a point-in-time snapshot of what
each thread is doing, like so:

.. code:: bash

    # Get a point-in-time snapshot of what each thread is currently running.
    pyflame -s 0 --threads PID

Are BSD / OS X / macOS Supported?
---------------------------------

Pyflame uses a few Linux-specific interfaces, so unfortunately it is the only
platform supported right now. Pull requests to add support for other platforms
are very much wanted.

Someone who is proficient with low-level C systems programming can probably get
BSD to work without *too much* difficulty. The necessary work to adapt the code
is described in `Issue #3 <https://github.com/uber/pyflame/issues/3>`__.

By comparison, it is probably *much more* work to get Pyflame working on macOS.
The current code assumes that the host uses `ELF
<https://en.wikipedia.org/wiki/Executable_and_Linkable_Format>`__
object/executable files. Apple uses a different object file format, called
`Mach-O <https://en.wikipedia.org/wiki/Mach-O>`__, so porting Pyflame to macOS
would entail doing all of the work to port Pyflame to BSD, *plus* additional
work to parse Mach-O object files. That said, the Mach-O format is documented
online (e.g. `here <https://lowlevelbits.org/parsing-mach-o-files/>`__), so a
sufficiently motivated person could get macOS support working.

What Are These Ptrace Permissions Errors?
-----------------------------------------

Because it's so powerful, the ``ptrace(2)`` system call is often disabled or
severely restricted. In order to use ptrace, these conditions must be met:

-  You must have the ``SYS_PTRACE``
   `capability <http://man7.org/linux/man-pages/man7/capabilities.7.html>`__
   (which is denied by default within Docker images).
-  The kernel must not have ``kernel.yama.ptrace_scope`` set to a value
   that is too restrictive.

In both scenarios you'll also find that ``strace`` and ``gdb`` do not work as
expected.

Ptrace Errors Within Docker Containers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

By default Docker images do not have the ``SYS_PTRACE`` capability. If you want
it enabled, invoke ``docker run`` using the ``--cap-add SYS_PTRACE`` option:

.. code:: bash

    # Allows processes within the Docker container to use ptrace.
    docker run --cap-add SYS_PTRACE ...

You can also use `capsh(1)
<http://man7.org/linux/man-pages/man1/capsh.1.html>`__ to list your current
capabilities:

.. code:: bash

    # You should see cap_sys_ptrace in the "Bounding set".
    capsh --print

You do not need to run Pyflame from within a Docker container. If you have
sufficient permissions (i.e. you are root, or the same UID as the Docker
process) Pyflame can be run from outside a container to inspect a process inside
a container. This is better for security, since you can keep ptrace disabled in
the container.

Ptrace Errors Outside Docker Containers Or When Not Using Docker
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you're not in a Docker container, or you're not using Docker at all, ptrace
permissions errors are likely related to you having too restrictive a value set
for the ``kernel.yama.ptrace_scope`` sysfs knob.

Debian Jessie ships with ``ptrace_scope`` set to 1 by default, which will
prevent unprivileged users from attaching to already running processes.

To see the current value of this setting:

.. code:: bash

    # Prints the current value for the ptrace_scope setting.
    sysctl kernel.yama.ptrace_scope

If you see a value other than 0 you may want to change it. Note that by doing
this you'll affect the security of your system. Please read `the relevant kernel
documentation <https://www.kernel.org/doc/Documentation/security/Yama.txt>`__
for a comprehensive discussion of the possible settings and what you're
changing. If you want to completely disable the ptrace settings and get
"classic" permissions (i.e. root can ptrace anything, unprivileged users can
ptrace processes with the same user id) then use:

.. code:: bash

    # Use this if you want "classic" ptrace permissions.
    sudo sysctl kernel.yama.ptrace_scope=0

Ptrace With SELinux
~~~~~~~~~~~~~~~~~~~

If you're using SELinux, `you may have problems with ptrace
<https://fedoraproject.org/wiki/Features/SELinuxDenyPtrace>`__. To check if
ptrace is disabled:

.. code:: bash

    # Check if SELinux is denying ptrace.
    getsebool deny_ptrace

If you'd like to enable it:

.. code:: bash

    # Enable ptrace under SELinux.
    setsebool -P deny_ptrace 0
