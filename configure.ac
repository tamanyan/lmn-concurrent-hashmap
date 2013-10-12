#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([LmnConcurrentHashMap], [1.0.0], [tamanyan.sss@gmail.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([config.h])


#if test -z $CFLAGS; then
#    CXFLAGS='-O2'
#fi
OPTIMIZER_CFLAGS="-O2"
OPTIMIZER_CXXFLAGS="-O2"

CFLAGS="$CFLAGS -std=gnu++0x"

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AM_PROG_CC_C_O
AC_PROG_RANLIB

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

default_enable_tcmalloc=no

AC_ARG_ENABLE([tcmalloc],                                                                                                                               
              [AS_HELP_STRING([--enable-tcmalloc],
                              [turn on using google perftools tcmalloc (linux only)])],
              [], 
              [enable_tcmalloc="$default_enable_tcmalloc"])

AM_CONDITIONAL(ENABLE_TCMALLOC, test "$enable_tcmalloc" = "yes")

# Checks for library functions.

AC_OUTPUT(Makefile \
          src/map/Makefile
          src/lmntal/concurrent/Makefile
          src/Makefile)
