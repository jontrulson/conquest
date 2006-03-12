dnl Process this file with autoconf to produce a configure script.
dnl Jon Trulson 9/21/96 - with some borrowed code from various sources...;-)
dnl
dnl $Id$
dnl

AC_INIT([conquest],[8.1],[jon@radscan.com])
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE
AC_PROG_LIBTOOL
AC_PROG_YACC
AC_PROG_LEX
AC_PATH_X
AC_PATH_XTRA

AC_PREFIX_DEFAULT(/opt)

dnl checks the system
dnl AC_CANONICAL_TARGET([])
dnl Checks for programs.
dnl get the system type and set accordingly.  first init some defaults.

GLCLIENT=""
conquest_group=""
AC_ARG_WITH(conquest-group,
  [  --with-conquest-group=groupname  The conquest group name (Default: conquest)],
  conquest_group="$withval", conquest_group="")

AC_ARG_ENABLE(gl,
  [  --disable-gl  Don't build the OpenGL bits.],
  GLCLIENT="", GLCLIENT="yes")

AC_MSG_CHECKING(system type)
uname=`uname -s`
urev=`uname -r`
uvers=`uname -v`

FILEPRIV="@-echo filepriv"

case "$uname" in
  Linux*)
	AC_DEFINE(LINUX, 1, [Linux])    

	AC_MSG_RESULT(Linux)
	;;
  FreeBSD*)
	AC_DEFINE(FREEBSD, 1, [FreeBSD])

	LIBS="$LIBS -lcompat"

	AC_MSG_RESULT(FreeBSD)
	;;
  SunOS*)
	AC_DEFINE(SOLARIS, 1, [Solaris])

	AC_MSG_RESULT(Solaris)
	;;
  UNIX_SV*|UnixWare)
	AC_DEFINE(UNIXWARE, 1, [UnixWare])

	FILEPRIV="/sbin/filepriv -f plock,tshar,core"
	AC_MSG_RESULT(UnixWare)
	;;
  OSF1*)

	case "$uvers" in
		V4.*)	
			true
			;;
		*)
			AC_MSG_WARN([This port is only known to work on DU 4.x])
			AC_MSG_WARN([Assuming OSF V4.x])
			;;
	esac
			
	AC_DEFINE(OSF, 1, [OSF])

	AC_MSG_RESULT(Digital Unix 4.x+)
	;;

  CYGWIN*)
        AC_DEFINE(CYGWIN, 1, [Cygwin])

        LIBS="$LIBS -lcygipc"
        if test ! -n "$conquest_group"
        then
                conquest_group="Users"
        fi
        
        AC_MSG_RESULT(Cygwin)
        ;;

  *)	
	# assume something safe...
	AC_DEFINE(UNKNOWN, 1, [Unknown])     

	AC_MSG_RESULT(UNKNOWN)
	;;
esac

AC_SUBST(FILEPRIV)
AC_PROG_CC

if test ! -n "$conquest_group"
then
        conquest_group="conquest"
fi

CONQGROUP="$conquest_group"
AC_SUBST(CONQGROUP)

if test -n "$GCC"
then
	CONQ_CFLAGS="$CFLAGS -D__NO_MATH_INLINES -Wall -fPIC -fno-builtin -DNDEBUG"
else
	CONQ_CFLAGS="$CFLAGS -DNDEBUG"
fi

AC_SUBST(CONQ_CFLAGS)

AC_PROG_MAKE_SET

dnl Checks for libraries.
dnl Replace `main' with a function in -lm:

AC_CHECK_LIB(m, main)

dnl check for ncurses
olibs="$LIBS"
AC_CHECK_LIB(ncurses, initscr, LIBS="-lncurses $olibs",
  AC_CHECK_LIB(curses, initscr, LIBS="-lcurses $olibs",
    AC_MSG_WARN(I cannot find initscr.  you might need to help me)))

dnl check for libcrypt
olibs="$LIBS"
AC_CHECK_LIB(crypt, crypt, LIBS="$olibs -lcrypt")

dnl check for libgen
olibs="$LIBS"
AC_CHECK_LIB(gen, crypt, LIBS="$olibs -lgen")

dnl check for libz
olibs="$LIBS"
AC_CHECK_LIB(z, gzread)

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h limits.h sys/ioctl.h sys/times.h termio.h unistd.h)
AC_CHECK_HEADERS(ncurses.h term.h sys/time.h sys/select.h stropts.h poll.h)
AC_CHECK_HEADERS(curses.h time.h string.h strings.h termios.h zlib.h)
AC_CHECK_HEADERS(stdint.h dirent.h)
AC_CHECK_HEADERS(GL/glut.h GL/gl.h GL/glu.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

dnl Checks for library functions.
CONQ_FUNC_MMAP
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(select strstr fileno setpgrp daemon)
AC_CHECK_FUNCS(getmsg poll setitimer strerror)

AC_FUNC_SETPGRP
AC_FUNC_SNPRINTF

dnl check for sys_errlist decl

AC_MSG_CHECKING(for sys_errlist declaration)
AC_RUN_IFELSE([AC_LANG_SOURCE([[
#include <sys/types.h>
#include <stdio.h>

#include <errno.h>
main()
{
  char *s = sys_errlist[0];
  exit(0);
}
]])],[AC_MSG_RESULT(yes)
  AC_DEFINE(SYS_ERRLIST_DECLARED, 1, [sys_errlist declared])],[AC_MSG_RESULT(no)],[:])

AM_CONDITIONAL(GLCLIENT, test $GLCLIENT)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT



