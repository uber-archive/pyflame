AC_PREREQ([2.68])
AC_INIT([pyflame], [1.6.6], [evan@eklitzke.org])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/pyflame.cc])

# Detect errors with pkg-tools macro expansion
m4_pattern_forbid([^PKG_])

# Fail early if the user tries to build for BSD/OS X
AC_CANONICAL_HOST
AS_CASE([$host_os],
        [linux-gnu*], [],
        [AC_MSG_ERROR([Pyflame can only be built for Linux hosts])])

enable_threads=no
AS_IF([test x"$host_cpu" = xx86_64],
      [AC_MSG_NOTICE([x86-64 system, threads will be supported])
       AC_DEFINE([ENABLE_THREADS], [1], [Threads are enabled.])
       AC_DEFINE([USE_ELF64], [1], [Expect 64-bit ELF symbols.])
       enable_threads=yes;
      ],
      [AC_MSG_NOTICE([Threading support will be disabled (only works on x86-64)])
       AC_DEFINE([ENABLE_THREADS], [0], [Threads are enabled.])
       AC_DEFINE([USE_ELF64], [0], [Expect 64-bit ELF symbols.])
      ])

AC_DEFINE_UNQUOTED([HOST_CPU], ["$host_cpu"], [CPU of target architecture.])

AM_INIT_AUTOMAKE([dist-bzip2 foreign subdir-objects])

dnl make the compilation flags quiet unless V=1 is used
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

AM_PROG_AR

LT_INIT

# Ensure Linux kernel headers are present
AC_CHECK_HEADERS([linux/ptrace.h], [], [AC_MSG_ERROR([Linux kernel headers missing])])

AC_LANG_PUSH([C++])

# Minimum C++ version is C++11
AX_CHECK_COMPILE_FLAG(["-std=c++11"],
                      [AX_APPEND_FLAG(["-std=c++11"], [CXXFLAGS])],
                      [AC_MSG_ERROR([failed to detect C++11 support])])

# Yes please.
AX_APPEND_COMPILE_FLAGS([-Wall], [CXXFLAGS])

# Turn warnings into errors
AC_ARG_ENABLE([werror],
              [AS_HELP_STRING([--enable-werror],
              [Treat certain compiler warnings as errors (default is no)])],
              [enable_werror=$enableval],
              [enable_werror=no])

if test "x$enable_werror" = "xyes"; then
  AX_APPEND_COMPILE_FLAGS([-Werror], [CXXFLAGS])
fi

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MMAP
AC_CHECK_FUNCS([getpagesize memmove munmap strerror strtol strtoul])

AC_DEFINE_UNQUOTED(
  [PYFLAME_VERSION_STR],
  ["pyflame $PACKAGE_VERSION $host_os $host_cpu"],
  [A string containing build information.])

enable_py26=no
PKG_CHECK_MODULES([PY26], [python-2.6], [enable_py26="yes"],
                          [PKG_CHECK_MODULES([PY26], [python-2.7], [enable_py26="yes"], [AC_MSG_WARN([Building without Python 2.6/2.7 support])])])
AM_CONDITIONAL([ENABLE_PY26], [test x"$enable_py26" = xyes])
AM_COND_IF([ENABLE_PY26], [AC_DEFINE([ENABLE_PY26], [1], [Python 2.6/2.7 will be enabled])])

enable_py34=no
PKG_CHECK_MODULES([PY34], [python-3.4], [enable_py34="yes"],
                          [PKG_CHECK_MODULES([PY34], [python-3.5], [enable_py34="yes"], [AC_MSG_WARN([Building without Python 3.4/3.5 support])])])
AM_CONDITIONAL([ENABLE_PY34], [test x"$enable_py34" = xyes])
AM_COND_IF([ENABLE_PY34], [AC_DEFINE([ENABLE_PY34], [1], [Python 3.4/3.5 will be enabled])])

enable_py36=no
PKG_CHECK_MODULES([PY36], [python-3.6], [enable_py36="yes"], [AC_MSG_WARN([Building without Python 3.6 support])])
AM_CONDITIONAL([ENABLE_PY36], [test x"$enable_py36" = xyes])
AM_COND_IF([ENABLE_PY36], [AC_DEFINE([ENABLE_PY36], [1], [Python 3.6 will be enabled])])

AS_IF([test x"$enable_py26" = xyes -o x"$enable_py34" = xyes -o x"$enable_py36" = xyes],
      [AC_MSG_NOTICE([Found at least one copy of Python.h])],
      [AC_MSG_ERROR([Failed to find a supported Python.h])]
)

AC_LANG_POP

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

echo
echo "Options used to compile and link:"
echo
echo "  with threads        = $enable_threads"
echo "  with Python 2.6/7   = $enable_py26"
echo "  with Python 3.4/5   = $enable_py34"
echo "  with Python 3.6+    = $enable_py36"
echo
echo "  CXX                 = $CXX"
echo "  CXXFLAGS            = $CXXFLAGS"
echo
