/************************************************************************
*
* c_defs.h - C macro defines to ease ratfor to C conversion.  Included
*               in all files.
*
* Author: Jon Trulson
************************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/12/08 05:05:37  jon
 * Initial revision
 *
 * Revision 1.18  1996/12/08  05:05:11  jon
 * - added parameter 'snum' to doselfdest()
 *
 * Revision 1.17  1996/11/23  06:08:00  jon
 * - changes required to accomodate autoconf.
 *   Now builds on Linux and FreeBSD 2.2x (with ncurses 1.9.9e).
 *
 * Revision 1.16  1996/07/02  02:52:22  jon
 * - mods to support linux
 * - removed old/uness macros
 * - changed launch() prototype to support new arg.
 * - removed USETIMER stuff.  you always USETIMER now.
 * - removed proto's for dead functions
 *
 * Revision 1.15  1996/05/25  00:27:11  jon
 * - modified to support system config file, added GetSysConf()
 *   prototype
 * - changed isagod() prototype to reflect new usage
 * - added getsemtxt() prototype - conqoper sem info support
 *
 * Revision 1.14  1996/05/02  01:03:34  jon
 * - added funtion protoypes for the semaphore module - sem.c
 *
 * Revision 1.13  1996/04/28  06:30:26  jon
 * - fixed typo in MakeConf() specification
 *
 * Revision 1.12  1996/04/28  05:33:52  jon
 * - removed dead (unused) function prototypes
 * - added process_bool() and MakeConf() prototypes
 *
 * Revision 1.11  1996/03/31  23:32:14  jon
 * - added function prototypes for new functions: lower, GetConquestUID,
 *   GetConquestGID, iBufInit, iBufEmpty, iBufPut, iBufGetCh, and DoMacro.
 *
 * Revision 1.10  1996/03/16  21:45:42  jon
 * removed sc*() dead functions...Yeah! removed en*() dead func too.
 *
 * Revision 1.9  1995/02/08  05:05:11  jon
 * minor fixes, replaced all _And_ and _Or_ macros with && and
 * || respectively.
 *
 * Revision 1.8  1995/01/30  06:04:58  jon
 * appchr() - fixed indexing bug.
 * Added prototypes for DoSig() and EnableSignalHandler().
 *
 * Revision 1.7  1995/01/29  05:59:10  jon
 * Added new function KPAngle() to return an angle depending on which
 * keypad key was pressed.
 * command() - changed to use KPAngle().
 *
 * Revision 1.6  1995/01/27  04:37:40  jon
 * *() - another attempt at using the curses input routines. Changed
 * input routine return values to int's so can decode terminfo data on
 * keypad and arrow keys.  Changed team effectiveness values back to
 * their original settings. 
 *
 * Revision 1.5  1995/01/15  07:25:14  jon
 * added ETA calculation ability to infoplanet() and infoship(). Enabled
 * by defining DO_ETASTATS.
 *
 * Revision 1.4  1995/01/14  22:52:40  jon
 * minor fixes, and formatting
 *
 * Revision 1.3  1995/01/09  00:46:44  jon
 * Added PVLOCK/UNLOCK capability, fixed some minor bugs, changed cloak
 * code so that a cloaked, warp 0 ship is invisible to alert scanners.
 * Increase torp fuse to 50 seconds, thereby increasing range.  Added
 * msync() to implement the upchuck() function.
 *
 * Revision 1.2  1995/01/02  00:58:58  jon
 * Changed references to NUMPLANETS and MAXSHIPS so that valid
 * values would be between 1 and NUMPLANETS/MAXSHIPS.
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 *
 *
 ***********************************************************************/

/* Get some valuable info... */
#include "config.h"


#if defined(HAVE_LIMITS_H)
# include <limits.h>
#endif

#if defined(HAVE_UNISTD_H)
# include <sys/types.h>
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



#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if defined(HAVE_SYS_TIMES_H)
# include <sys/times.h>
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


/* externs */
extern int errno;

#if !defined(SYS_ERRLIST_DECLARED)
extern char *sys_errlist[];
#endif

#define DIGIT   0
#define LETTER  1

#ifndef FALSE
# define FALSE   (0)
#endif
#ifndef TRUE
# define TRUE    (1)
#endif


#define YES     TRUE
#define NO      FALSE
#define yes     YES
#define no      NO
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
#define BUFFER_SIZE 256

/* Type Fakes */
/*
#define string char *
#define real float
*/

typedef char * string;
typedef float real;

/* Function Fakes */

#if defined(LINUX)
#define powf(x, y)  (real)(pow((double) (x),(double) (y)))
#define sinf(x)     (real)(sin((double) (x)))
#define cosf(x)     (real)(cos((double) (x)))
#define fmodf(x, y)  (real)(fmod((double) (x),(double) (y)))
#define fabsf(x)     (real)(fabs((double) (x)))
#endif

#define float(x) ((real) (x))
#define mod(x, y) ((x) % (y))
#define equal(x, y)  ((strcmp(x, y) == 0) ? YES : NO)
#define c_strcpy(x, y) strcpy(y, x)
#define appstr(x, y) strcat(y, x)
#define appchr(x, y)     {\
			    static int yyzi; \
			    yyzi = strlen(y); \
			    y[yyzi++] = x; \
			    y[yyzi] = '\0'; \
			 }
#define chcopy(from, to, off) to[off] = from; to[(off) + 1] = EOS
#define clower(x) (char)tolower((int)(x))
#define cupper(x) (char)toupper((int)(x))
#define itoc(i, buf, len) sprintf(buf, "%d", (i)) /* hope buf is big enough */
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define min0(x, y) min((x), (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define max0(x, y) max((x), (y))
#define alog(x) ((real)log((double)(x)))
#define fold(x) /* Who knows what this does... */
#define glname(x) (void) cuserid(x)
#define iand(x, y) ((x) & (y))
#define ifix(x) ((int) (x))
#define termin(x) strcpy(x, ttyname(0))	/* assume stdin */

/* Function prototypes for ratfor.c */

int alldig(char *buf);
int c_type(char c);
void concat(char str1[], char str2[], char buf[]);
int c_index(char *buf, char ch);
void lower(char *buf);
void upper(char *buf);
void getnow (int now[8]);
int wkday (int a, int b, int c);
char c_getch(char c, int fd);
int lib_ffs(int start, int len, int bits, int *rule);
void c_sleep(real sleeptime);
int ctoi(char *buf);
real ctor(char *buf);

/* Function prototypes for everything else */

/* conquest.c */

void command(int ch);
void doalloc( int snum);
void doautopilot( int snum );
void dobeam( int snum );
void dobomb( int snum );
void doburst( int snum );
void docloak( int snum );
void docoup( int snum );
void docourse( int snum );
void dodet( int snum );
void dodistress( int snum );
void dohelp( int subdcl );
void doinfo( int snum );
void dolastphase( int snum );
void domydet( int snum );
void dooption( int snum, int dodisplay );
void doorbit( int snum );
void dophase( int snum );
void doplanlist( int snum );
void doreview( int snum );
void doselfdest(int snum);
void doshields( int snum, int up );
void doteamlist( int team );
void dotorp( int snum );
void dotow( int snum );
void dountow( int snum );
void dowar( int snum );
void dowarp( int snum, real warp );
int getoption( char ch, int *tok );
void menu(void);
void play(void);
real phaserhit( int snum, real dis );
void statline( int unum, char *buf );
int capentry( int snum, int *system );
int newship( int unum, int *snum );
void dead( int snum, int leave );

/** conqutil.c */

real acdist( real curvel, real newvel, real acc );
void putpmt( char *pmt, int line );
void stcpn( char *from, char *to, int tosize );
int safectoi(int *num, char *buf, int useless);
real subang( real a1, real a2);
real angle( real fromx, real fromy, real tox, real toy );
void appint( int i, char *str );
void appnum( int num, char *buf );
void appnumtim( int now[], char *buf );
void appsstatus( int status, char *buf );
void apptitle( int team, char *buf );
int arrows( char *str, real *dir );
void cerror(char *fmt, ...);
void clog(char *fmt, ...);
int confirm( int dummy );
void delblanks( char *str );
int dsecs( int s, int *n );
real explosion( real basehits, real dis );
void fmtminutes( int itime, char *buf );
void fmtseconds( int itime, char *buf );
int getamsg( int snum, int *msg );
void getdandt( char *buf );
int gettarget( char *pmt, int lin, int col, real *dir, real cdefault );
char getcx( char *pmt, int lin, int offset, char *terms, char *buf, int len );
int dgrand(int s, int *n);
void grand( int *h );
void gsecs( int *s );
real mod360( real r );
int modp1( int i, int modulus );
int more( char *pmt );
void pagefile( char *file, char *errmsg, int ignorecontroll, int eatblanklines );
void c_putmsg( char *msg, int line );
void putpmt( char *pmt, int line );
int special( char *str, int *what, int *token, int *count );
void stcpn( char *from, char *to, int tosize );
int stmatch( char *str1, char *str2, int casesensitive );


/* conqlb.c */

int c_register( char *lname, char *rname, int team, int *unum );
void chalkup( int snum );
int cloak( int snum );
void damage( int snum, real dam, int kb );
void detonate( int snum, int tnum );
void display( int snum );
int enemydet( int snum );
void hit( int snum, real ht, int kb );
void ikill( int snum, int kb );
void infoplanet( char *str, int pnum, int snum );
char *ETAstr(real warp, real distance);
void infoship( int snum, int scanner );
void killship( int snum, int kb );
int launch( int snum, real dir, int number );
void orbit( int snum, int pnum );
int phaser( int snum, real dir );
real phaserhit( int snum, real dis );
void pseudo( int unum, int snum );
void planlist( int team );
void playlist( int godlike, int doall );
void resign( int unum );
int review( int snum, int slm );
void takeplanet( int pnum, int snum );
void teamlist( int team );
void userline( int unum, int snum, char *buf, int showgods, int showteam );
void userlist( int godlike );
void userstats( int godlike );
void statline( int unum, char *buf );
void zeroplanet( int pnum, int snum );
void do_border(void);
void draw_alertborder(int alert);

/** conqmisc.c */

int cmpplanet(void *cmp1, void *cmp2);
void sortplanets(int sv[]);
void appkb( int kb, char *buf );
void appship( int snum, char *str );
int canread( int snum, int msgnum );
void clearships(void);
int cvtcoords( real cenx, real ceny, real x, real y, real scale, 
	      int *lin, int *col );
void doomfind(void);
void doomsday(void);
int findorbit( int snum, int *pnum );
int findship( int *snum );
void fixdeltas( int snum );
int gunum( int *unum, char *lname );
void histlist( int godlike );
void initeverything(void);
void initgame(void);
void initmsgs(void);
void initplanets(void);
void initrobots(void);
void initship( int snum, int unum );
void inituniverse(void);
void intrude( int snum, int pnum );
void loghist( int unum );
real newarp( int snum, real dwarp );
int phoon( int pnum );
void puthing( int what, int lin, int col );
void putship( int snum, real basex, real basey );
void sendmsg( int from, int terse );
int readmsg( int snum, int msgnum, int dsplin );
int stillalive( int snum );
void zeroeverything(void);
void stormsg( int from, int to, char *msg );
void zeroship( int snum );
int planmatch(char *str, int *pnum, int godlike);
int usefuel( int snum, real fuel, int weapon );
int findspecial( int snum, int token, int count, int *sorpnum, int *xsorpnum );
int KPAngle(int ch, real *angle);

/* cd2lb.c */

void cdbeep(void);
void cdbox ( int lin1, int col1, int lin2, int col2 );
void cdcput ( char ch, char f );
void cdclear(void);
void cdclra ( int l1, int c1, int l2, int c2 );
void cdclrl ( int f, int n );
int cdcols ( int dummy );
void cdend(void);
void cdfill ( char ch, char buf[], int count );
int cdgetn ( char pmt[], int lin, int col, int *num );
int cdgets ( char pmt[], int lin, int col, char str[], int maxlen );
int cdgetx ( char pmt[], int lin, int col, char terms[], char str[], 
	     int maxlen );
int cdgetp ( char pmt[], int lin, int col, char terms[], char str[], 
	     int maxlen );
void cdgoto ( int lin, int col );
void cdinit(void);
void cdline ( int lin1, int col1, int lin2, int col2 );
int cdlins ( int dummy );
void cdmove ( int lin, int col );
void cdmovtc ( char ibuf[], char obuf[], char trntbl[], int count );
void cdrefresh ( int stoponinput );
void cdput ( char ch, int lin, int col );
void cdputc ( char str[], int lin );
void cdputn ( int iint, int wid, int lin, int col );
void cdputr ( real x, int wid, int lin, int col );
void cdputs ( char str[], int lin, int col );
void cdredo(void);

/* conqoper.c */
void bigbang(void);
void debugdisplay( int snum );
void debugplan(void);
void doomdisplay(void);
int gplanmatch( char str[], int *pnum );
void kiss(void);
void opback( int lastrev, int *savelin );
void operate(void);
void opinfo( int snum );
void opinit(void);
void opplanlist(void);
void opresign(void);
void oprobot(void);
void opstats(void);
void opteamlist(void);
void opuadd(void);
void oppedit(void);
void opuedit(void);
void watch(void);

/* conqai.c */
void buildai( int snum, int vars[], int *bnenum, real *bdne, real *bane );
void defend( int attacker, int pnum );
void displayai( int snum, int token, int vars[] );
void executeai( int snum, int token );
void exitai(void);
int newrob( int *snum, int unum );
void robotai( int snum );
void trobotai( int snum );
void robotloop(void);
void robreply( char buf[] );
void robstr( int token, char buf[] );
int tableai( int vars[] );

/* conqdriv.c */

void iterdrive( int ship[] );
void secdrive( int ship[] );
void submindrive(void);
void mindrive(void);
void fivemindrive(void);
void SigTerminate(int sig);

/* conqsvr42.c */

int GetConquestUID(void);
int GetConquestGID(void);
void astoff(void);
void aston(void);
void EnableSignalHandler(void);
void DoSig(int sig);
void astservice(int sig);	/* our SIGALARM handler */
void comsize( unsigned long *size );
void conqend(void);
void conqinit(void);
void conqstats( int snum );
void drcheck(void);
void drcreate(void);
void drkill(void);
void drpexit(void);
void drstart(void);
void gcputime( int *cpu );
void helplesson(void);
void initstats( int *ctemp, int *etemp );
int isagod( char *name );
int mail( char names[], char subject[], char msg[] );
void news(void);

void settimer(void);
void stoptimer(void);

void upchuck(void);
void upstats( int *ctemp, int *etemp, int *caccum, int *eaccum, int *ctime, int *etime );

/* gamlb.c */

int gamcheck( int dummy );
int gamcmp( int num, char buf[] );
void gamcronfile( char file[] );
int gamdialup( int dummy );
void gamend(void);
void gaminit( char truename[] );
void gamlinit( int fdial, int fprio, int fcron, int despri, char *badlist, 
	      char *badttylist, char *truename, char *cronfile );
void gamimage( char image[] );
int gamtname( char name[], char list[], int leadingmatch );

/* rndlb.c */

void rndini ( int seed1, int seed2 );
void rndseq ( int value, int multiplier, int increment, int modulus );
real rnd ( int dummy );
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

/* ioxlb.c */

int iogchar ( int ch );
int iogtimed ( int *ch, int seconds );

/* ioplb.c */

int iogquick( char ch );
void ioeat(void);

/* ionlb.c */

int iochav( int dummy );

/* conqcm.c */
char *mymalloc(int size);
void map_common(void);
void lock_common(void);
void flush_common(void);
#ifndef USE_PVLOCK
# define PVLOCK(x)
# define PVUNLOCK(x)
#else
 void PVLOCK(int *);
 void PVUNLOCK(int *);
#endif


/* ibuf.c */

void iBufInit(void);
int iBufEmpty(void);
void iBufPut(char *thestr);
char iBufGetCh(void);
int DoMacro(int ch);

/* conf.c */

int GetSysConf(int checkonly);
int GetConf(void);
int MakeSysConf(void);

/* sem.c */

char *getsemtxt(int what);
int GetSem(void);
void Lock(int what);
void Unlock(int what);
char *GetSemVal(int thesem);

