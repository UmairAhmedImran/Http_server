AC_INIT([http-loadbalancer], [1.0.0], [your-email@example.com])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
AC_PROG_INSTALL

AC_CHECK_LIB([pthread], [pthread_create], [], [
    AC_MSG_ERROR([pthread library not found])
])

AC_ARG_WITH([prefix],
    [AS_HELP_STRING([--with-prefix=PREFIX],
        [Installation prefix @<:@default=/usr/local@:>@])],
    [prefix=$withval],
    [prefix=/usr/local])

AC_ARG_WITH([sysconfdir],
    [AS_HELP_STRING([--with-sysconfdir=DIR],
        [System configuration directory @<:@default=PREFIX/etc@:>@])],
    [sysconfdir=$withval],
    [sysconfdir=$prefix/etc])

AC_SUBST([prefix])
AC_SUBST([sysconfdir])
AC_SUBST([bindir], [$prefix/bin])
AC_SUBST([localstatedir], [$prefix/var])

CFLAGS="${CFLAGS:--Wall -Wextra}"
AC_SUBST([CFLAGS])
AC_SUBST([LDFLAGS])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

