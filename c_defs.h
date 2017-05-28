/************************************************************************
 *
 * c_defs.h - C macro defines to ease ratfor to C conversion.  Included
 *               in all files.
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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

#if !defined(MINGW)
# if defined(HAVE_MMAP)
#  include <sys/mman.h>
# else
#  error "You need mman.h - mmap()"
# endif
#endif

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

#ifndef FALSE
# define FALSE   (0)
#endif
#ifndef TRUE
# define TRUE    (1)
#endif

#define FILENAMESIZE 512
#define MAXLINE 78
#define ARGBUFSIZE 512
#define BIG_BUFFER_SIZE 4096
#define MID_BUFFER_SIZE 2048
#define BUFFER_SIZE 256

/* Type Fakes */

typedef double real;

/* Function Fakes */


#define mod(x, y) ((x) % (y))
#define equal(x, y)  ((strcmp(x, y) == 0) ? TRUE : FALSE)
#define c_strcpy(x, y) strcpy(y, x)
#define appstr(x, y) strcat(y, x)
#define appchr(x, y)     {                      \
        static int yyzi;                        \
        yyzi = strlen(y);                       \
        y[yyzi++] = x;                          \
        y[yyzi] = '\0';                         \
    }
#ifndef min
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef max
#define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#define fold(x) /* Who knows what this does... */

/* Function prototypes for everything else */

/* conqai.c */
void defend( int attacker, int pnum );
int newrob( int *snum, int unum );
void robotloop(void);
void robreply( char buf[] );
void robstr( int token, char buf[] );
void robotai( int snum );

/* conqsvr42.c */

int getUID(char *name);
int getConquestGID(void);
void comsize( unsigned long *size );
void conqinit(void);
void conqstats( int snum );
void drcheck(void);
void drcreate(void);
void drkill(void);
void drpexit(void);
void drstart(void);
void gcputime( int *cpu );
void initstats( int *ctemp, int *etemp );
int isagod( int unum );
int mailimps( char *subject, char *msg);
int checkPID(int pidnum);
void upchuck(void);
void upstats( int *ctemp, int *etemp, int *caccum, int *eaccum, int *ctime, int *etime );

/* rndlb.c */

void rndini ( int seed1, int seed2 );
void rndseq ( int *value, int multiplier, int increment, int modulus );
real rnd ( void );
real rnduni ( real rlow, real rhigh );
int rndint ( int ilow, int ihigh );
real rndnor ( real mean, real stddev );
real rndexp ( real mean );
real rndchi ( int v );
real rndF ( int v1, int v2 );
real rndt ( int v );
int rndgeo ( real prob );
int rndbin ( int trials, real prob );
int rndpoi ( real mean );

#endif /* _C_DEFS_H */
