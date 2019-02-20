# Here be dragons.
#
# The way this code is structured is the code that know about Python interpreter
# internals is in frob.cc. This file isn't compiled directly, instead there are
# files frob{26,34,36}.cc that define preprocessor macros to compile frob.cc for
# different Python ABIs.
#
# The libtool magic here makes it so that frob26.cc is compiled with python2
# flags, and frob3{4,6}.cc are compiled with python3.4/3.6 flags.

bin_PROGRAMS = pyflame
pyflame_SOURCES = aslr.cc frame.cc thread.cc namespace.cc posix.cc prober.cc ptrace.cc pyflame.cc pyfrob.cc symbol.cc
pyflame_LDADD =

noinst_LTLIBRARIES =

if ENABLE_PY26
libfrob26_la_SOURCES = frob26.cc
libfrob26_la_CXXFLAGS = $(PY26_CFLAGS)
noinst_LTLIBRARIES += libfrob26.la
pyflame_LDADD += libfrob26.la
endif

if ENABLE_PY34
libfrob34_la_SOURCES = frob34.cc
libfrob34_la_CXXFLAGS = $(PY34_CFLAGS)
noinst_LTLIBRARIES += libfrob34.la
pyflame_LDADD += libfrob34.la
endif

if ENABLE_PY36
libfrob36_la_SOURCES = frob36.cc
libfrob36_la_CXXFLAGS = $(PY36_CFLAGS)
noinst_LTLIBRARIES += libfrob36.la
pyflame_LDADD += libfrob36.la
endif

if ENABLE_PY37
libfrob37_la_SOURCES = frob37.cc
libfrob37_la_CXXFLAGS = $(PY37_CFLAGS)
noinst_LTLIBRARIES += libfrob37.la
pyflame_LDADD += libfrob37.la
endif
