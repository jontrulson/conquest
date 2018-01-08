dnl Process this file with autoconf to produce a configure script.
dnl Jon Trulson 9/21/96 - with some borrowed code from various sources...;-)
dnl
dnl

AC_INIT([conquest],[8],[jon@radscan.com])
AC_CONFIG_HEADER(config.h)

AC_CONFIG_MACRO_DIRS([m4])

AC_LANG(C++)

dnl checks the system
dnl AC_CANONICAL_TARGET([])
dnl Checks for programs.
dnl get the system type and set accordingly.  first init some defaults.

GLCLIENT=""
ISMINGW=""
conquest_group=""

AC_ARG_WITH(conquest-group,
  [  --with-conquest-group=groupname  The conquest group name (Default: conquest)],
  conquest_group="$withval", conquest_group="")

AC_ARG_ENABLE(gl,
  [  --disable-gl  Don't build the OpenGL bits.],
  GLCLIENT="", GLCLIENT="yes")

AC_ARG_ENABLE(sound,
  [  --disable-sound  Don't enable sound support.],
  SNDCLIENT="", SNDCLIENT="yes")

AC_ARG_ENABLE(tcpwrappers,
  [  --enable-tcpwrappers  Enable TCP Wrappers support.],
  TCPW="yes", TCPW="")

dnl can't have sound without gl
if ! test -n "$GLCLIENT"
then
  SNDCLIENT=""
fi

AC_CANONICAL_SYSTEM
AC_MSG_CHECKING(system type)
uname=`uname -s`
urev=`uname -r`
uvers=`uname -v`

case "$host" in
  *linux*)
	AC_DEFINE(LINUX, 1, [Linux])

	AC_MSG_RESULT(Linux)
	;;
  *darwin*)
	AC_DEFINE(DARWIN, 1, [Darwin])
        ocflags="$CFLAGS"
        CFLAGS="$ocflags -I/usr/X11/include -I/opt/X11/include"
        CXXFLAGS="$ocflags -I/usr/X11/include -I/opt/X11/include"

	AC_MSG_RESULT(Darwin)
	;;
  *freebsd*)
	AC_DEFINE(FREEBSD, 1, [FreeBSD])

	LIBS="$LIBS -lcompat"

	AC_MSG_RESULT(FreeBSD)
	;;
  *solaris*)
	AC_DEFINE(SOLARIS, 1, [Solaris])

	LIBS="$LIBS -lsocket -lnsl"

	AC_MSG_RESULT(Solaris)
	;;
  *sysv4.2uw*)
	AC_DEFINE(UNIXWARE, 1, [UnixWare])

	AC_MSG_RESULT(UnixWare)
	;;
  *mingw*)
        # for MinGW systems - we only want to build the GL client (only)
	AC_DEFINE(MINGW, 1, [MinGW])
        ISMINGW="yes"

	AC_MSG_RESULT(MinGW)
	;;
  *dec-osf1*)

	case "$uvers" in
		?4.*)
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

  *cygwin*)
        AC_DEFINE(CYGWIN, 1, [Cygwin])

        if test ! -n "$conquest_group"
        then
                conquest_group="Users"
        fi

        AC_MSG_RESULT(Cygwin)
        ;;

  *)
	# assume something safe, linux
	AC_DEFINE(LINUX, 1, [Linux])

	AC_MSG_RESULT(UNKNOWN defaulting to LINUX)
	;;
esac

dnl disable static libs by default (except for mingw)
AM_DISABLE_SHARED

AM_INIT_AUTOMAKE
AC_PROG_LIBTOOL
AC_PROG_YACC
AC_PROG_LEX

AC_PREFIX_DEFAULT(/opt)

if test ! -n "$ISMINGW"
then
AC_PATH_X
AC_PATH_XTRA
fi

AC_PROG_CXX
AC_PROG_LIBTOOL

if test ! -n "$conquest_group"
then
        conquest_group="conquest"
fi

CONQGROUP="$conquest_group"
AC_SUBST(CONQGROUP)

if test -n "$GXX"
then
	CONQ_CFLAGS="$CXXFLAGS -std=c++11 -D__NO_MATH_INLINES -fno-builtin -DNDEBUG -fno-strict-aliasing"
else
	CONQ_CFLAGS="$CXXFLAGS -std=c++11 -DNDEBUG"
fi

AC_SUBST(CONQ_CFLAGS)
dnl no idea why this is neccessary now.
AC_SUBST(YFLAGS)

AC_PROG_MAKE_SET

dnl Checks for libraries.
dnl Replace `main' with a function in -lm:

AC_CHECK_LIB(m, main)

dnl check for curses, we give preference to a native curses
olibs="$LIBS"
AC_CHECK_LIB(curses, initscr, LIBS="-lcurses $olibs",
  AC_CHECK_LIB(ncurses, initscr, LIBS="-lncurses $olibs",
    AC_MSG_WARN(I cannot find initscr (curses).  You might need to help me)))

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
AC_CHECK_HEADERS(stdint.h dirent.h crypt.h)

AC_CHECK_HEADERS(GL/glut.h GL/gl.h GL/glu.h, :, GLCLIENT=""; SNDCLIENT="")

dnl tcpd checks.  What a pain in the ass.
AC_CHECK_HEADERS(tcpd.h, :, TCPW="")

NSLLIB=""
AC_CHECK_LIB(nsl, yp_get_default_domain, NSLLIB="-lnsl")
TCPWLIB=""
olibs="$LIBS"
LIBS="$olibs $NSLLIB"
AC_CHECK_LIB(wrap, hosts_ctl, TCPWLIB="-lwrap")
LIBS="$olibs"

if test -n "$TCPW"
then
  CONQ_CFLAGS="$CONQ_CFLAGS -DHAVE_TCPW"
  AC_SUBST(TCPWLIB)
  AC_SUBST(NSLLIB)
fi


dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

AC_FUNC_VPRINTF
AC_CHECK_FUNCS(select strstr fileno setpgrp daemon)
AC_CHECK_FUNCS(getmsg poll setitimer strerror)

if test ! -n "$ISMINGW"
then
AC_FUNC_SETPGRP
fi


dnl see if we should check for SDL and friends
if test -n "$SNDCLIENT"
then

dnl Check for SDL
SDL_VERSION=1.2.0
AM_PATH_SDL($SDL_VERSION,
            CONQ_CFLAGS="$CONQ_CFLAGS -DHAVE_SDL=1",
            SNDCLIENT=""; AC_MSG_WARN([*** SDL version $SDL_VERSION not found!])
)
CONQ_CFLAGS="$CONQ_CFLAGS $SDL_CFLAGS"

dnl Check for SDL_mixer
AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio, CONQ_CFLAGS="$CONQ_CFLAGS -DHAVE_SDL_MIXER=1"; LIBS="-lSDL_mixer $LIBS", SNDCLIENT=""; AC_MSG_WARN([*** SDL_mixer not found!]))

fi

AM_CONDITIONAL(GLCLIENT, test $GLCLIENT)
AM_CONDITIONAL(ISMINGW, test $ISMINGW)
AM_CONDITIONAL(SNDCLIENT, test $SNDCLIENT)
AM_CONDITIONAL(TCPW, test $TCPW)


AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo
if ! test -n "$GLCLIENT"
then
        echo "### OpenGL support is disabled, the client will NOT be built"
else
        echo "### OpenGL support is enabled, will build the client"
fi

if ! test -n "$SNDCLIENT"
then
        echo "### SDL sound support is disabled"
else
        echo "### SDL sound support is enabled"
fi

if ! test -n "$TCPW"
then
        echo "### TCP Wrappers support is disabled"
else
        echo "### TCP Wrappers support is enabled"
fi
