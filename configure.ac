AC_PREREQ([2.68])
AC_INIT([pyflame], [1.3.1], [evan@uber.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/pyflame.cc])

AM_INIT_AUTOMAKE([dist-bzip2 foreign subdir-objects -Wall -Werror])

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

# Fail early if the user tries to build on OS X
AC_CHECK_HEADERS([linux/ptrace.h], [], [AC_MSG_ERROR([Pyflame only supports Linux hosts])])

AC_LANG([C++])

# This is commented out because it doesn't work right on Ubuntu 12.04 (Precise)
# AX_CXX_COMPILE_STDCXX_11
AX_CHECK_COMPILE_FLAG(["-std=c++11"],
                      [AX_APPEND_FLAG(["-std=c++11"], [CXXFLAGS])],
                      [AX_CHECK_COMPILE_FLAG(["-std=c++0x"],
                                             [AX_APPEND_FLAG(["-std=c++0x"], [CXXFLAGS])],
                                             [AC_MSG_ERROR([failed to detect C++11 support])])])

AX_CHECK_COMPILE_FLAG([-Wall],
                      [AX_APPEND_FLAG([-Wall], [CXXFLAGS])])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h limits.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
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

PKG_CHECK_MODULES([PY2], [python2], [enable_py2="yes"], [AC_MSG_WARN([Building without Python 2 support])])
AM_CONDITIONAL([ENABLE_PY2], [test x"$enable_py2" = xyes])
AM_COND_IF([ENABLE_PY2], [AC_DEFINE([ENABLE_PY2], [1], [Python2 is enabled])])

PKG_CHECK_MODULES([PY34], [python-3.4], [enable_py34="yes"], 
                          [PKG_CHECK_MODULES([PY35], [python-3.5], [enable_py34="yes"], [AC_MSG_WARN([Building without Python 3.4/3.5 support])])])
AM_CONDITIONAL([ENABLE_PY34], [test x"$enable_py34" = xyes])
AM_COND_IF([ENABLE_PY34], [AC_DEFINE([ENABLE_PY34], [1], [Python3 is enabled])])

PKG_CHECK_MODULES([PY36], [python-3.6], [enable_py36="yes"], [AC_MSG_WARN([Building without Python 3.6 support])])
AM_CONDITIONAL([ENABLE_PY36], [test x"$enable_py36" = xyes])
AM_COND_IF([ENABLE_PY36], [AC_DEFINE([ENABLE_PY36], [1], [Python36 is enabled])])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_REVISION([m4_esyscmd_s([git describe --always])])
AC_OUTPUT
