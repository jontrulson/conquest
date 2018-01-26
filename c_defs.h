//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


/************************************************************************
 * c_defs.h - C macro defines to ease ratfor to C conversion.  Included
 *               in all files.
 ************************************************************************/

#ifndef _C_DEFS_H
#define _C_DEFS_H
/* Get some valuable info... */
#include "config.h"


#if defined(HAVE_LIMITS_H)
# include <limits.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
# include <sys/types.h>
#endif

#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif

#if defined(HAVE_CRYPT_H)
# include <crypt.h>
#else
# if !defined(DARWIN)
/* DARWIN does not need this, crypt is in unistd.h */
extern char *crypt(char *, char *);
# endif
#endif

#if defined(STDC_HEADERS)
# include <stdlib.h>
# include <stdio.h>
# include <ctype.h>
# include <errno.h>
# include <signal.h>
# include <stdarg.h>
# include <stdbool.h>
# include <stdint.h>
#endif

#ifndef SIGCLD
#define SIGCLD SIGCHLD
#endif

#if !defined(MINGW)
/* JET - need checks here? */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#else
#define __USE_W32_SOCKETS 1
#include <windows.h>
#endif

#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif


#if defined(HAVE_STRING_H)
# include <string.h>
#endif

#if defined(HAVE_STRINGS_H)
# include <strings.h>
#endif

#ifndef _POSIX_C_SOURCE
// We need at least 199309L for nanosleep()
# define _POSIX_C_SOURCE 200809L
#endif

#if defined(TIME_WITH_SYS_TIME)
# include <sys/time.h>
# include <time.h>
#else
# if defined(HAVE_SYS_TIME_H)
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if defined(HAVE_SYS_TIMES_H)
# include <sys/times.h>
#endif

#if defined(HAVE_TERMIOS_H)
# include <termios.h>
#endif

#include <math.h>

#if defined(HAVE_FCNTL_H)
# include <fcntl.h>
#endif

/* if we have curses, we'll use it, else ncurses */
#if !defined(MINGW)
# if defined(HAVE_CURSES_H)
#  include <curses.h>
# elif defined(HAVE_NCURSES_H)
#  include <ncurses.h>
# else
#  error "You need the System V curses or ncurses library and headers."
# endif
#endif

#if defined(HAVE_TERMIO_H)
# include <termio.h>
#endif

#if defined(HAVE_TERM_H)
# include <term.h>
#endif

#include <memory.h>

#include <sys/stat.h>

#if !defined(MINGW)
#include <pwd.h>
#include <grp.h>
#endif

#include "defs.h"		/* conquest behavior modification */
#include "conqdef.h"

#if defined(TIME_WITH_SYS_TIME)
# include <sys/time.h>
#endif

#if !defined(MINGW)
/* We'll use select by default if it's there */
# if defined(HAVE_SELECT)
#  if defined(HAVE_SYS_SELECT_H)
#   include <sys/select.h>
#  endif
#  define USE_SELECT
# elif defined(HAVE_POLL) && defined(HAVE_POLL_H)
#  include <stropts.h>
#  include <poll.h>
#  undef USE_SELECT
# else
#  error "Must have select() or poll()"
# endif
#endif  /* MINGW */

#define BUFFER_SIZE_1024 1024
#define BUFFER_SIZE_256  256
#define BUFFER_SIZE_128  128

/* Type Fakes */

typedef double real;

/* Function Fakes */


#define mod(x, y) ((x) % (y))

#endif /* _C_DEFS_H */
