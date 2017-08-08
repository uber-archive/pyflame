Installing Pyflame
==================

To build Pyflame you will need a C++ compiler with basic C++11 support. Pyflame
is known to compile on versions of GCC as old as GCC 4.6.

Build Dependencies
------------------

Generally you'll need autotools, automake, libtool, pkg-config, and the Python
headers. If you have headers for both Python 2 and Python 3 installed you'll get
a Pyflame build that can target either version of Python.

Debian/Ubuntu
~~~~~~~~~~~~~

Install the following packages if you are building for Debian or Ubuntu.
Note that you technically only need one of ``python-dev`` or
``python3-dev``, but if you have both installed then you can use Pyflame
to profile both Python 2 and Python 3 processes.

.. code:: bash

    # Install build dependencies on Debian or Ubuntu.
    sudo apt-get install autoconf automake autotools-dev g++ pkg-config python-dev python3-dev libtool make

Fedora
~~~~~~

Again, you technically only need one of ``python-devel`` and
``python3-devel``, although installing both is recommended.

.. code:: bash

    # Install build dependencies on Fedora.
    sudo dnf install autoconf automake gcc-c++ python-devel python3-devel libtool

Compiling
---------

Once you've installed the appropriate build dependencies, you can compile
Pyflame like so:

.. code:: bash

    ./autogen.sh
    ./configure      # Plus any options like --prefix.
    make
    make check       # Optional, test the build! Should take < 1 minute.
    make install     # Optional, install into the configure prefix.

The Pyflame executable produced by the ``make`` command will be located at
``src/pyflame``. Note that the ``make check`` command requires that you have the
``virtualenv`` command installed. You can also sanity check your build with a
command like:

.. code:: bash

    # Or use -t python3, as appropriate.
    pyflame -t python -c 'print(sum(i for i in range(100000)))'

Creating A Debian Package
~~~~~~~~~~~~~~~~~~~~~~~~~

If you'd like to build a Debian package, run the following from the root
of your Pyflame git checkout:

.. code:: bash

    # Install additional dependencies required for packaging.
    sudo apt-get install debhelper dh-autoreconf dpkg-dev

    # This create a file named something like ../pyflame_1.3.1_amd64.deb
    dpkg-buildpackage -uc -us

Pre-Built Packages
------------------

Several Pyflame users have created unofficial pre-built packages for different
distros. Uploads of these packages tend to lag the official Pyflame releases, so
you are **strongly encouraged to check the pre-built version** to ensure that it
is not too old. If you want the newest version of Pyflame, build from source.

Ubuntu PPA
~~~~~~~~~~

`Trevor Joynson <https://github.com/akatrevorjay>`__ has set up an unofficial
PPA for all current Ubuntu releases: `ppa:trevorjay/pyflame
<https://launchpad.net/~trevorjay/+archive/ubuntu/pyflame>`__.

.. code:: bash

    sudo apt-add-repository ppa:trevorjay/pyflame
    sudo apt-get update
    sudo apt-get install pyflame

Note also that you can build your own Debian package easily, using the one
provided in the ``debian/`` directory of this project.

Arch Linux
~~~~~~~~~~

`Oleg Senin <https://github.com/RealFatCat>`__ has added an Arch Linux package
to `AUR <https://aur.archlinux.org/packages/pyflame-git/>`__.
