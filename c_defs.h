/************************************************************************
 *
 * c_defs.h - C macro defines to ease ratfor to C conversion.  Included
 *               in all files.
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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

#define creal(x) ((real) (x))
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
#define min0(x, y) min((x), (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define max0(x, y) max((x), (y))
#define fold(x) /* Who knows what this does... */
#define glname(x, y) {\
                      strncpy(x, cuserid(NULL), y - 1); \
                      x[y - 1] = EOS; \
		     }
      
#define ifix(x) ((int) (x))

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

/* conquest.c */

void command(int ch);
int welcome( int *unum );
void doalloc( int snum);
void doautopilot( int snum );
void dobeam( int snum );
void dobomb( int snum );
void doburst( int snum );
void docloak( int snum );
void dorefit( int snum, int dodisplay );
void docoup( int snum );
void docourse( int snum );
void dodet( int snum );
void dodistress( int snum );
void dohelp( void );
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
int play(void);
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
int confirm(void);
int askyn(char *question, int lin, int col);
void delblanks( char *str );
int dsecs( int s, int *n );
real explosion( real basehits, real dis );
void fmtminutes( int itime, char *buf );
void fmtseconds( int itime, char *buf );
int getamsg( int snum, int *msg );
void getdandt( char *buf, time_t thetime );
int gettarget( char *pmt, int lin, int col, real *dir, real cdefault );
char getcx( char *pmt, int lin, int offset, char *terms, char *buf, int len );
int dgrand(int s, int *n);
void grand( int *h );
void gsecs( int *s );
real mod360( real r );
int modp1( int i, int modulus );
int more( char *pmt );
void pagefile( char *file, char *errmsg );
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
int enemydet( int snum );
void hit( int snum, real ht, int kb );
void ikill( int snum, int kb );
void infoplanet( char *str, int pnum, int snum );
char *ETAstr(real warp, real distance);
void infoship( int snum, int scanner );
void killship( int snum, int kb );
int launch( int snum, real dir, int number, int ltype );
void orbit( int snum, int pnum );
int phaser( int snum, real dir );
real phaserhit( int snum, real dis );
void pseudo( int unum, int snum );
void planlist( int team, int snum );
void playlist( int godlike, int doall, int snum );
void resign( int unum, int isoper );
int review( int snum, int slm );
void takeplanet( int pnum, int snum );
void teamlist( int team );
void userline( int unum, int snum, char *buf, int showgods, int showteam );
void userlist( int godlike, int snum );   /* dwp */
void userstats( int godlike , int snum ); /* dwp */
void statline( int unum, char *buf );
void zeroplanet( int pnum, int snum );
int IsRemoteUser(void);

/** conqmisc.c */

int cmpplanet(void *cmp1, void *cmp2);
void sortplanets(int sv[]);
int cmpuser(void *cmp1, void *cmp2);
void sortusers(int sv[], int numentries);
int spwar( int snum, int pnum );
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
int gunum( int *unum, char *lname, int ltype );
void histlist( int godlike );
void initeverything(void);
void initgame(void);
void initmsgs(void);
void initplanets(void);
void initrobots(void);
void initship( int snum, int unum );
void inituniverse(void);
void intrude( int snum, int pnum );
int loghist( int unum );
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
int KP2DirKey(int *ch);

/* cd2lb.c */

void cdbeep(void);
void cdbox ( int lin1, int col1, int lin2, int col2 );
void cdcput ( char ch );
void cdclear(void);
void cdclra ( int l1, int c1, int l2, int c2 );
void cdclrl ( int f, int n );
int cdcols (void);
void cdend(void);
void cdfill ( char ch, char buf[], int count );
int cdgetn ( char pmt[], int lin, int col, int *num );
int cdgets ( char pmt[], int lin, int col, char str[], int maxlen );
int cdgetx ( char pmt[], int lin, int col, char terms[], char str[], 
	     int maxlen, int doecho );
int cdgetp ( char pmt[], int lin, int col, char terms[], char str[], 
	     int maxlen, int *append_flg, int do_append_flg, int doecho );
void cdinit(void);
void cdline ( int lin1, int col1, int lin2, int col2 );
int cdlins ( void );
void cdmove ( int lin, int col );
void cdmovtc ( char ibuf[], char obuf[], char trntbl[], int count );
void cdrefresh ( void );
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
int gplanmatch( char str[], int *pnum );
void kiss(int snum, int prompt_flg);
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
int prompt_ship(char buf[], int *snum, int *normal);
void dowatchhelp(void);
void setdheader(int show_header);
void toggle_line(int snum, int old_snum);
char *build_toggle_str(char snum_str[], int snum);
void menu_item( char *option, char *msg_line, int lin, int col );
int DoInit(char InitChar, int cmdline);

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

void iterdrive( int *ship );
void secdrive( int *ship );
void submindrive(void);
void mindrive(void);
void fivemindrive(void);
void SigTerminate(int sig);

/* conqsvr42.c */

int GetConquestUID(void);
int GetConquestGID(void);
void astoff(void);
void aston(void);
void EnableConquestSignalHandler(void);
void EnableConqoperSignalHandler(void);
void DoConquestSig(int sig);
void DoConqoperSig(int sig);
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
int isagod( int unum );
void news(void);
int mailimps( char *subject, char *msg);
int CheckPid(int pidnum);

void settimer(void);
void setopertimer(void);
void stoptimer(void);

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

/* iolb.c */

int iochav( void );
int iogchar ( void );
int iogtimed ( int *ch, int seconds );
void ioeat(void);

/* conqcm.c */
char *mymalloc(int size);
void map_common(void);
void lock_common(void);
void flush_common(void);
int check_cblock(char *fname, int fmode, int sizeofcb);
#ifndef USE_PVLOCK
# define PVLOCK(x)
# define PVUNLOCK(x)
#else
 void PVLOCK(int *);
 void PVUNLOCK(int *);
#endif
void zero_common(void);


/* ibuf.c */

void iBufInit(void);
int iBufEmpty(void);
void iBufPut(char *thestr);
void iBufPutc(char thechar);
char iBufGetCh(void);
int DoMacro(int ch);

/* conf.c */

int GetSysConf(int checkonly);
int GetConf(int isremote, int usernum);
int MakeSysConf(void);
int SaveUserConfig(int unum);
int SaveSysConfig(void);
char *Str2Macro(char *str);
char *Macro2Str(char *str);

/* sem.c */

char *getsemtxt(int what);
int GetSem(void);
void Lock(int what);
void Unlock(int what);
char *GetSemVal(int thesem);

/* display.c */

void display( int snum, int display_info );
void display_headers(int snum);
void do_bottomborder(void);
void do_border(void);
int alertcolor(int alert);
void draw_alertborder(int alert);

/* userauth.c */
int Logon(char *username, char *password);
void ChangePassword(int unum, int godlike);

/* options.c */
void SysOptsMenu(void);
void UserOptsMenu(int unum);


