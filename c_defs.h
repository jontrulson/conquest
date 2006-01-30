/************************************************************************
 *
 * c_defs.h - C macro defines to ease ratfor to C conversion.  Included
 *               in all files.
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ************************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

/* Get some valuable info... */
#include "config.h"


#if defined(HAVE_LIMITS_H)
# include <limits.h>
#endif

#if defined(HAVE_SYS_TYPES_H)
# include <sys/types.h>
#endif

#if defined(HAVE_UNISTD_H)
# if defined(LINUX)
#  define _XOPEN_SOURCE
#  define __USE_XOPEN		/* Why? */
# endif
# include <unistd.h>
#endif 

#if defined(STDC_HEADERS)
# include <stdlib.h>
# include <stdio.h>
# include <ctype.h>
# include <errno.h>
# include <signal.h>
# include <stdarg.h>
#endif 

#ifndef SIGCLD                                                     
#define SIGCLD SIGCHLD
#endif 

/* JET - need checks here? */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>


#if defined(HAVE_STRING_H)
# include <string.h>
#endif 

#if defined(HAVE_STRINGS_H)
# include <strings.h>
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

/* if we have ncurses_lib, we'll use it, else curses, else error */

#if defined(HAVE_NCURSES_H)
# include <ncurses.h>
#elif defined(HAVE_CURSES_H)
# include <curses.h>
#else
# error "You need System V curses or ncurses 1.9.9e. Badly."
#endif

#if defined(HAVE_TERM_H)
# include <term.h>
#endif

#include <memory.h>
#include <sys/ioctl.h>

#if defined(HAVE_TERMIO_H)
# include <termio.h>
#endif

#if defined(HAVE_MMAP)
# include <sys/mman.h>
#else
# error "You need mman.h - mmap()"
#endif

#include <sys/stat.h>

#include <pwd.h>
#include <grp.h>

#include "defs.h"		/* conquest behavior modification */
#include "conqdef.h"

#if defined(TIME_WITH_SYS_TIME)
# include <sys/time.h>
#endif

/* We'll use select by default if it's there */
#if defined(HAVE_SELECT)
# if defined(HAVE_SYS_SELECT_H)
#  include <sys/select.h>
# endif
# define USE_SELECT
#elif defined(HAVE_POLL) && defined(HAVE_POLL_H)
# include <stropts.h>
# include <poll.h>
# undef USE_SELECT
#else
# error "Must have select() or poll()"
#endif

#if !defined(HAVE_SPRINTF) || defined(PREFER_PORTABLE_SNPRINTF)
# include "snprintf.h"
#endif


/* externs */
extern int errno;

#ifndef HAVE_STRERROR
# if !defined(SYS_ERRLIST_DECLARED)
extern char *sys_errlist[];
# endif
# define strerror(X) sys_errlist[(X)]
#endif

#define DIGIT   0
#define LETTER  1

#ifndef FALSE
# define FALSE   (0)
#endif
#ifndef TRUE
# define TRUE    (1)
#endif

#ifndef ERR
# define ERR     (-1)
#endif
#ifndef OK
# define OK      TRUE
#endif

#define EOS	'\0'

#define FILENAMESIZE 512
#define MAXLINE 78
#define ARGBUFSIZE 512
#define BIG_BUFFER_SIZE 4096
#define MID_BUFFER_SIZE 2048
#define BUFFER_SIZE 256

/* Type Fakes */

typedef char * string;
typedef double real;

/* Function Fakes */


#define mod(x, y) ((x) % (y))
#define equal(x, y)  ((strcmp(x, y) == 0) ? TRUE : FALSE)
#define c_strcpy(x, y) strcpy(y, x)
#define appstr(x, y) strcat(y, x)
#define appchr(x, y)     {\
			    static int yyzi; \
			    yyzi = strlen(y); \
			    y[yyzi++] = x; \
			    y[yyzi] = '\0'; \
			 }
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define fold(x) /* Who knows what this does... */
#define ifix(x) ((int) (x))
#define rfix(x) ((real) (x))

/* Function prototypes for ratfor.c */

int alldig(char *buf);
int c_type(char c);
void concat(char str1[], char str2[], char buf[]);
int c_index(char *buf, char ch);
void lower(char *buf);
void upper(char *buf);
time_t getnow (int now[NOWSIZE], time_t thetime);
int wkday (void);
int lib_ffs(int start, int len, int bits, int *rule);
void c_sleep(real sleeptime);
real ctor(char *buf);
void error(char *str);


/* Function prototypes for everything else */

/** conqutil.c */

real acdist( real curvel, real newvel, real acc );
void stcpn( char *from, char *to, int tosize );
int safectoi(int *num, char *buf, int useless);
real subang( real a1, real a2);
real angle( real fromx, real fromy, real tox, real toy );
void appint( int i, char *str );
void appnum( int num, char *buf );
void appnumtim( int now[], char *buf );
void appsstatus( int status, char *buf );
void apptitle( int team, char *buf );
void appship( int snum, char *str );
void appkb( int kb, char *buf );
int arrows( char *str, real *dir );
void setSystemLog(int usesys);
void cerror(char *fmt, ...);
void clog(char *fmt, ...);
void delblanks( char *str );
int dsecs( int s, int *n );
real explosion( real basehits, real dis );
void fmtminutes( int itime, char *buf );
void fmtseconds( int itime, char *buf );
int getamsg( int snum, int *msg );
void getdandt( char *buf, time_t thetime );
int dgrand(int s, int *n);
void grand( int *h );
void gsecs( int *s );
real mod360( real r );
int modp1( int i, int modulus );
int special( char *str, int *what, int *token, int *count );
void stcpn( char *from, char *to, int tosize );
int stmatch( char *str1, char *str2, int casesensitive );


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


