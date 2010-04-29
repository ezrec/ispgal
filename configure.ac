#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([ispgal], [0.0], [jason.mcmullan@gmail.com])
AM_INIT_AUTOMAKE

AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_LIB([ftdi], [ftdi_init],
	        [],
	     	AC_MSG_ERROR([libftdi is required]))

AC_OUTPUT([
	Makefile
	src/Makefile
])
