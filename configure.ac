#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([SlimHashMap], [1.0.0], [tamanyan.sss@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/benchmark.cpp])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h],[AC_DEFINE(HAVE_STDIO_H)],[AC_MSG_ERROR([Sorry, cantnot do anything for you])])
AC_CHECK_HEADERS([string.h],[AC_DEFINE(HAVE_STRING_H)],[AC_MSG_ERROR([Sorry, cannot do anything for you])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.

AC_OUTPUT(Makefile \
          src/Makefile)
