# Here be dragons.
#
# The way this code is structured is the code that know about Python interpreter
# internals is in frob.cc. This file isn't compiled directly, instead there are
# two files frob{2,3}.cc that define preprocessor macros to compile frob.cc for
# python2 and python3, respectively.
#
# The libtool magic here makes it so that frob2.cc is compiled with python2
# flags and frob3.cc is compiled with python3 flags.

bin_PROGRAMS = pyflame
pyflame_SOURCES = aslr.cc frame.cc thread.cc namespace.cc posix.cc ptrace.cc pyflame.cc pyfrob.cc symbol.cc
pyflame_LDADD =

noinst_LTLIBRARIES =

if ENABLE_PY2
libfrob2_la_SOURCES = frob2.cc
libfrob2_la_CXXFLAGS = $(PY2_CFLAGS)
noinst_LTLIBRARIES += libfrob2.la
pyflame_LDADD += libfrob2.la
endif

if ENABLE_PY34
libfrob3_la_SOURCES = frob3.cc
libfrob3_la_CXXFLAGS = $(PY34_CFLAGS)
noinst_LTLIBRARIES += libfrob3.la
pyflame_LDADD += libfrob3.la
endif

if ENABLE_PY36
libfrob36_la_SOURCES = frob36.cc
libfrob36_la_CXXFLAGS = $(PY36_CFLAGS)
noinst_LTLIBRARIES += libfrob36.la
pyflame_LDADD += libfrob36.la
endif
