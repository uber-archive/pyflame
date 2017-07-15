.. Pyflame documentation master file, created by
   sphinx-quickstart on Fri Jul 14 14:13:03 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Pyflame: A Ptracing Profiler For Python
=======================================

Pyflame is a unique profiling tool that generates `flame graphs
<http://www.brendangregg.com/flamegraphs.html>`__ for Python. Pyflame is the
only Python profiler based on the Linux `ptrace(2)
<http://man7.org/linux/man-pages/man2/ptrace.2.html>`__ system call. This allows
it to take snapshots of the Python call stack without explicit instrumentation,
meaning you can profile a program without modifying its source code! Pyflame is
capable of profiling embedded Python interpreters like `uWSGI
<https://uwsgi-docs.readthedocs.io/en/latest/>`__. It fully supports profiling
multi-threaded Python programs.

Pyflame is written in C++, with attention to speed and performance. Pyflame
usually introduces less overhead than the builtin ``profile`` (or ``cProfile``)
modules, and also emits richer profiling data. The profiling overhead is low
enough that you can use it to profile live processes in production.

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   installation
   versions
   usage
   faq
   contributing

Websites
--------

- `Project homepage
  <http://pyflame.readthedocs.org/>`_ (this documentation)
- `Source code at Github
  <https://github.com/uber/pyflame>`_


Blog Posts
----------

Some existing articles and blog posts on Pyflame include:

-  `Pyflame: Uber Engineering's Ptracing Profiler For
   Python <http://eng.uber.com/pyflame/>`__ by Evan Klitzke (2016-09)
-  `Pyflame Dual Interpreter
   Mode <https://eklitzke.org/pyflame-dual-interpreter-mode>`__ by Evan
   Klitzke (2016-10)
-  `Using Uber's Pyflame and Logs to Tackle Scaling
   Issues <https://benbernardblog.com/using-ubers-pyflame-and-logs-to-tackle-scaling-issues/>`__
   by Benoit Bernard (2017-02)
-  `Building Pyflame on Centos
   6 <http://blog.motitan.com/2017/04/15/python%E6%80%A7%E8%83%BD%E5%88%86%E6%9E%90%E5%B7%A5%E5%85%B7%E4%B9%8Bpyflame/>`__
   (Chinese) by Faicker Mo (2017-04)

If you write a new post about Pyflame, please let us know and we'll add it here!
