Contributing
============

We love getting pull requests and bug reports! This section outlines some ways
you can contribute to Pyflame.

Hacking
-------

This section will explain the Pyflame code for people who are interested in
contributing source code patches.

A good way to start understanding the code is to read the two blog posts (linked
on the main docs page) written by Evan Klitzke. They cover the basics about how
Pyflame works, and have some helpful information about how the code is
organized.

The code style in Pyflame (mostly) conforms to the `Google C++ Style Guide
<https://google.github.io/styleguide/cppguide.html>`__. Additionally, all of the
source code is formatted with `clang-format
<http://clang.llvm.org/docs/ClangFormat.html>`__. There's a ``.clang-format``
file checked into the root of this repository which will make ``clang-format``
do the right thing. Different clang releases may format the source code slightly
differently, as the formatting rules are updated within clang itself. Therefore
you should eyeball the changes made when formatting, especially if you have an
older version of clang.

If you are changing any of the low-level C++ bits, and end up with a broken
build, you may want to try by getting the following command working before
testing with the full test suite:

.. code:: bash

    # Sanity check Pyflame.
    pyflame -t python -c 'print(sum(i for i in range(100000)))'

To run the full test suite locally:

.. code:: bash

    # Run the Pyflame test suite.
    make test

If you change any of the Python files in the ``tests/`` directory, please run
your changes through `YAPF <https://github.com/google/yapf>`__ before submitting
a pull request.

How Else Can I Help?
--------------------

Patches are not the only way to contribute to Pyflame! Bug reports are very
useful as well. If you file a bug, make sure you tell us the exact version of
Python you're using, and how to reproduce the issue.
