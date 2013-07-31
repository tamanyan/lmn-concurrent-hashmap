#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([LmnConcurrentHashMap], [1.0.0], [tamanyan.sss@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdio.h],[AC_DEFINE(HAVE_STDIO_H)],[AC_MSG_ERROR([Sorry, cantnot do anything for you])])
AC_CHECK_HEADERS([string.h],[AC_DEFINE(HAVE_STRING_H)],[AC_MSG_ERROR([Sorry, cannot do anything for you])])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.

AC_OUTPUT(Makefile \
          src/map/Makefile
          src/Makefile)
