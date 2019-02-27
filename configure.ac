#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pid2pgsql], [0.1], [pid2pgsql@volumehost.com])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
LT_INIT
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_PROG_LIBTOOL(libtool)
AC_SEARCH_LIBS([PQconnectdb], [pq])
#AC_SEARCH_LIBS([boost::program_options], [boost_program_options])
BOOST_PROGRAM_OPTIONS_LIBS=""
saved_LIBS="${LIBS}"
LIBS=""
AC_SEARCH_LIBS(main, boost_program_options-mt, [],
    [AC_SEARCH_LIBS(main, boost_program_options, [],
        AC_MSG_FAILURE(["Boost program options library not found."]))])
BOOST_PROGRAM_OPTIONS_LIBS="${LIBS}"
LIBS="${saved_LIBS}"
AC_SUBST(BOOST_PROGRAM_OPTIONS_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h])
#AC_CXX_CHECK_LIB([boost_program_options], [boost::program_options])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_PID_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

# Checks for library functions.
AC_CHECK_FUNCS([uname])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
