#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([kyfd], [0.0.5], [kyfd@phontron.com])
AC_CONFIG_SRCDIR([src/lib/decoder.cc])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/bin/Makefile
    src/lib/Makefile
    src/include/Makefile
    src/include/kyfd/Makefile
])

# disable shared libraries
AM_DISABLE_SHARED
AC_PROG_LIBTOOL


# Intialize automake
AM_INIT_AUTOMAKE([-Wall])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([fst/fst.h xerces/dom/DOM.hpp])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([floor])

AC_OUTPUT
