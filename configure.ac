AC_PREREQ([2.60])
AC_INIT([ubjansson], [0.1], [luke-jr+jansson@utopios.org])

AM_INIT_AUTOMAKE([1.10 foreign])

AC_CONFIG_SRCDIR([src/load.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL
AM_CONDITIONAL([GCC], [test x$GCC = xyes])

# Checks for libraries.
PKG_CHECK_MODULES([jansson], [jansson])
AC_SEARCH_LIBS([pow], [m])

AC_CONFIG_FILES([
        ubjansson.pc
        Makefile
        src/Makefile
        test/Makefile
        test/bin/Makefile
])
AC_OUTPUT
