#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT(chameleon, 1.0)
AM_INIT_AUTOMAKE(chameleon, 1.0)
AC_CONFIG_SRCDIR([./])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
${CXXFLAGS=""}
AC_PROG_CC
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([pthread.h time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.
AC_CHECK_FUNCS([memcpy memset])

AC_OUTPUT(Makefile)
