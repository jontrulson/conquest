#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/*                               C O N Q U T I L */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "global.h"


/*##  acdist - figure distance traveled while changing velocities */
/*  SYNOPSIS */
/*    real dis, curvel, newvel, acc */
/*    dis = acdist( curvel, newvel, acc ) */
/*  DESCRIPTION */
/*    These formulas works for de-acceleration only. */
real acdist( real curvel, real newvel, real acc )
{
  real t;
  /*    The following is a box approximation that takes into account */
  /*    the way the driver moves ships and so gives the *exact* distance. */
  /*    v = curvel - newvel */
  /*    n = v / ( ITER_SECONDS * acc ) + 1.0 */
  /*    d = ( n * ITER_SECONDS + ( v / 2.0 + newvel ) * MM_PER_SEC_PER_WARP ) */
  /*    A "faster", but less accurate formula (when compared to the */
  /*    way the driver moves ships) is as follows: */
  t = ( curvel - newvel ) / acc;
  return ( ( curvel * t - 0.5 * acc * t * t ) * MM_PER_SEC_PER_WARP );
  
}


/*##  angle - compute the angle between two points */
/*  SYNOPSIS */
/*    real ang, angle, fromx, fromy, tox, toy */
/*    ang = angle( fromx, fromy, tox, toy ) */
real angle( real fromx, real fromy, real tox, real toy )
{
  if ( fromx == tox && fromy == toy )
    return ( 0.0 );
  
  return ( mod360( rtod( atan2( toy - fromy, tox - fromx ) ) ) );
  
}


/*##  appint - append an int to a string */
/*  SYNOPSIS */
/*    int int */
/*    char str() */
/*    appint( i, str ) */
void appint( int i, char *str )
{
  char buf[20];
  
  itoc( i, buf, 20 );
  appstr( buf, str );
  
  return;
  
}


/*##  appnum - append a number in English */
/*  SYNOPSIS */
/*    int num */
/*    char buf() */
/*    appnum( num, buf ) */
/* Note: This routine only works for the number less than 100. */
void appnum( int num, char *buf )
{
  int i, j;
  
  i = num;
  if ( i >= 100 )
    {
      appstr( "big num", buf );
      return;
    }
  if ( i >= 20 )
    {
      j = i/10;
      switch ( j )
	{
	case 2:
	  appstr( "twenty", buf );
	  break;
	case 3:
	  appstr( "thirty", buf );
	  break;
	case 4:
	  appstr( "forty", buf );
	  break;
	case 5:
	  appstr( "fifty", buf );
	  break;
	case 6:
	  appstr( "sixty", buf );
	  break;
	case 7:
	  appstr( "seventy", buf );
	  break;
	case 8:
	  appstr( "eighty", buf );
	  break;
	case 9:
	  appstr( "ninety", buf );
	  break;
	default:
	  appstr( "???", buf );
	  break;
	}
      i = i - j*10;
      if ( i == 0 )
	return;
      appchr( '-', buf );
    }
  
  switch ( i )
    {
    case 0:
      appstr( "zero", buf );
      break;
    case 1:
      appstr( "one", buf );
      break;
    case 2:
      appstr( "two", buf );
      break;
    case 3:
      appstr( "three", buf );
      break;
    case 4:
      appstr( "four", buf );
      break;
    case 5:
      appstr( "five", buf );
      break;
    case 6:
      appstr( "six", buf );
      break;
    case 7:
      appstr( "seven", buf );
      break;
    case 8:
      appstr( "eight", buf );
      break;
    case 9:
      appstr( "nine", buf );
      break;
    case 10:
      appstr( "ten", buf );
      break;
    case 11:
      appstr( "eleven", buf );
      break;
    case 12:
      appstr( "twelve", buf );
      break;
    case 13:
      appstr( "thirteen", buf );
      break;
    case 14:
      appstr( "fourteen", buf );
      break;
    case 15:
      appstr( "fifteen", buf );
      break;
    case 16:
      appstr( "sixteen", buf );
      break;
    case 17:
      appstr( "seventeen", buf );
      break;
    case 18:
      appstr( "eighteen", buf );
      break;
    case 19:
      appstr( "nineteen", buf );
      break;
    }
  
  return;
  
}


/*##  appnumtim - append English formated time and date */
/*  SYNOPSIS */
/*   appnumtim( now, buf ) */
void appnumtim( int now[], char *buf )
{
  int hour;
  int am;
  
  am = TRUE;				/* assume morning */
  hour = now[4];
  if ( hour == 0 )
    hour = 12;			/* midnight */
  else if ( hour == 12 )
    am = FALSE;			/* afternoon */
  else if ( hour > 12 )
    {
      hour = hour - 12;
      am = FALSE;			/* afternoon */
    }
  switch ( wkday( now[2], now[3], now[1] ) )
    {
    case 1:
      appstr( "Sunday", buf );
      break;
    case 2:
      appstr( "Monday", buf );
      break;
    case 3:
      appstr( "Tuesday", buf );
      break;
    case 4:
      appstr( "Wednesday", buf );
      break;
    case 5:
      appstr( "Thursday", buf );
      break;
    case 6:
      appstr( "Friday", buf );
      break;
    case 7:
      appstr( "Saturday", buf );
      break;
    default:
      appstr( "???", buf );
      break;
    }
  appstr( ", ", buf );
  switch ( now[2] )
    {
    case 1:
      appstr( "January", buf );
      break;
    case 2:
      appstr( "February", buf );
      break;
    case 3:
      appstr( "March", buf );
      break;
    case 4:
      appstr( "April", buf );
      break;
    case 5:
      appstr( "May", buf );
      break;
    case 6:
      appstr( "June", buf );
      break;
    case 7:
      appstr( "July", buf );
      break;
    case 8:
      appstr( "August", buf );
      break;
    case 9:
      appstr( "September", buf );
      break;
    case 10:
      appstr( "October", buf );
      break;
    case 11:
      appstr( "November", buf );
      break;
    case 12:
      appstr( "December", buf );
      break;
    default:
      appstr( "???", buf );
      break;
    }
  appchr( ' ', buf );
  appint( now[3], buf );		/* day of month */
  appstr( ", at ", buf );
  appnum( hour, buf );		/* hour */
  appchr( ' ', buf );
  if ( now[5] == 0 )			/* minute */
    appstr( "o'clock", buf );
  else
    {
      if ( now[5] < 10 )
	appstr( "o ", buf );
      appnum( now[5], buf );
    }
  appchr( ' ', buf );
  if ( am )
    appstr( "ante", buf );
  else
    appstr( "post", buf );
  appstr( " meridiem", buf );
  
  return;
  
}


/*##  appsstatus - append ship status string */
/*  SYNOPSIS */
/*    int status */
/*    char buf() */
/*    appsstatus( status, buf ) */
void appsstatus( int status, char *buf )
{
  switch ( status )
    {
    case SS_OFF:
      appstr( "off", buf );
      break;
    case SS_ENTERING:
      appstr( "entering", buf );
      break;
    case SS_LIVE:
      appstr( "live", buf );
      break;
    case SS_DYING:
      appstr( "dying", buf );
      break;
    case SS_DEAD:
      appstr( "dead", buf );
      break;
    case SS_RESERVED:
      appstr( "reserved", buf );
      break;
    default:
      appint( status, buf );
      break;
    }
  return;
  
}


/*##  apptitle - append a team oriented title */
/*  SYNOPSIS */
/*    int team */
/*    char buf() */
/*    apptitle( team, buf ) */
void apptitle( int team, char *buf )
{
  switch ( team )
    {
    case TEAM_FEDERATION:
      appstr( "Captain", buf );
      break;
    case TEAM_ROMULAN:
      appstr( "Centurion", buf );
      break;
    case TEAM_ORION:
      appstr( "Commander", buf );
      break;
    case TEAM_KLINGON:
      appstr( "Kommander", buf );
      break;
    }
  
  return;
  
}


/*##  arrows - interpret arrow keys */
/*  SYNOPSIS */
/*    int flag, arrows */
/*    char str() */
/*    real dir */
/*    flag = arrows( str, dir ) */
int arrows( char *str, real *dir )
{
  int i, n, idx, num; 
  real thedir, ndir, ndir1, ndir2;
  string arrs="*dewqazxc";	/* the '*' is to fill arrs[0] - JET */
  
  /* Special hack preventing "ea" and "da" from being recognized as arrows. */
  /* "ea" is reserved for Earth and "da" for Dakel. */
  if ( tolower(str[0]) == 'e' && tolower(str[1]) == 'a' )
    return ( FALSE );
  if ( tolower(str[0]) == 'd' && tolower(str[1]) == 'a' )
    return ( FALSE );
  
  thedir = 0.0;
  
  for ( i = 0; str[i] != EOS; i = i + 1 )
    {
      n = i + 1;
      idx = c_index( arrs, tolower(str[i]) );
      if ( idx == ERR || idx == 0)
	return ( FALSE );
      
      ndir1 = ((real)idx - 1.0) * 45.0;
      ndir2 = (real)ndir1 - 360.0;
      
      if ( (real)fabsf( thedir - ndir1 ) < (real)fabsf( thedir - ndir2 ) )
	ndir = ndir1;
      else
	ndir = ndir2;
      
      thedir = (((thedir*((real)n - 1)) + ndir ) / (real)n);
      
    }
  
  
  *dir = mod360( thedir );
  
  return ( TRUE );
  
}


/*##  cerror - conquest error message */
/*  SYNOPSIS */
/*    int to, status */
/*    char fmt() */
/*    cerror( to, fmt, status ) */
/*void cerror( int to, char *fmt, int status )*/
void cerror(char *fmt, ...)
{
  va_list ap;
  char buf[MSGMAXLINE];
  char ebuf[128];
  static FILE *errfd = NULL;
  int tmp;
  
  va_start(ap, fmt);
  (void)vsprintf(buf, fmt, ap);
  
  va_end(ap);
  
  stormsg( MSG_OUTSIDE, MSG_GOD, buf );
  
  return;
  
}

void clog(char *fmt, ...)
{
  va_list ap;
  char buf[MSGMAXLINE];
  char ebuf[128];
  static char errfile[BUFFER_SIZE];
  static FILE *errfd = NULL;
  extern char *c_conq_errlog;
  int tmp;
  
  va_start(ap, fmt);
  (void)vsprintf(buf, fmt, ap);
  
  va_end(ap);

  if (errfd == NULL)
    {
      umask(007);
      sprintf(errfile, "%s/%s", CONQHOME, c_conq_errlog);
      if (ConquestGID == ERR)
	{
	  fprintf(stderr, "conqutil: clog():  ConquestUID == ERR!\n");
	  exit(1);
	}

      if ((errfd = fopen(errfile, "a+")) == NULL)
	{
	  if ((tmp = creat(errfile, 0660)) == -1)
	    {
	      fprintf(stderr, "clog(): creat(%s): %s\n",
		      errfile,
		      sys_errlist[errno]);
	      
	      exit(1);
	    }
	  else
	    {
	      close(tmp);

	      if (chmod(errfile, 
			(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) == -1)
		{
		  perror("clog():chmod()");
		  exit(1);
		}
	    }
	}
      else
	{
	  fclose(errfd);
	}
      
      chown(errfile, 0, ConquestGID);

      if ((errfd = fopen(errfile, "a+")) == NULL)
	{
	  perror("cerror(): can't open logfile");
	}
    }
  
  if (errfd != NULL)
    {
      fprintf(errfd, "%ld:%d:%s\n", time(0), getpid(), buf);
      fflush(errfd);
    }
  
  return;
  
}



/*##  confirm - ask the user to confirm a dangerous action */
/*  SYNOPSIS */
/*    int ok, confirm */
/*    ok = confirm( 0 ) */
int confirm( int dummy )
{
  char ch, buf[MSGMAXLINE];
  
  cdclrl( MSG_LIN2, 1 );
  ch = getcx( "Are you sure? ", MSG_LIN2, 0, TERMS, buf, MSGMAXLINE );
  cdclrl( MSG_LIN2, 1 );
  cdrefresh(TRUE);
  if ( ch == TERM_ABORT )
    return ( FALSE );
  if ( buf[0] == 'y' || buf[0] == 'Y' )
    return ( TRUE );
  
  return ( FALSE );
  
}


/*##  delblanks - remove all blanks from a string */
/*  SYNOPSIS */
/*    char str() */
/*    delblanks( str ) */
void delblanks( char *str )
{
  int i, j;
  
  for ( i = 0; str[i] != EOS; )
    if ( str[i] == ' ' )
      for ( j = i; str[j] != EOS; j = j + 1 )
	str[j] = str[j+1];
    else
      i = i + 1;
  
  return;
  
}


/*##  dgrand - delta time for thousands */
/*  SYNOPSIS */
/*    int i, dgrand, s, n */
/*    i = dgrand( s, n ) */
int dgrand( int s, int *n )
{
  int tn, ts;
  
  /* Save s in case it and n are the same variable. */
  ts = s;
  
  /* Get thousands since midnight. */
  grand( &tn );
  *n = tn;
  
  /* Calculate the difference. */
  if ( tn < ts )
    tn = tn + 24 * 60 * 60 * 1000;		/* crossed midnight */
  
  return ( tn - ts );
  
}


/*##  dsecs - delta time for seconds */
/*  SYNOPSIS */
/*    int i, dsecs, s, n */
/*    i = dsecs( s, n ) */

int dsecs( int s, int *n )
{
  int tn, ts;
  
  /* Save s in case it and n are the same variable. */
  ts = s;
  
  /* Get seconds since midnight. */
  gsecs( &tn );
  *n = tn;
  
  /* Calculate the difference. */
  if ( tn < ts )
    tn = tn + ((24 * 60) * 60);		/* crossed midnight */
  
  return ( tn - ts );
  
}


/*##  explosion - hits based on distance */
/*  SYNOPSIS */
/*    real newhits, explosion, basehits, dis */
/*    newhits = explosion( basehits, dis ) */
real explosion( real basehits, real dis )
{
  if ( dis > PHASER_DIST )
    return ( 0.0 );
  return ( basehits / ( ( EXPLOSION_FALLOFF - 1.0 ) *
		       max( dis - EXPLOSION_RADIUS, 0.0 ) / PHASER_DIST + 1.0 ) -
	  basehits / EXPLOSION_FALLOFF * dis / PHASER_DIST );
  
}


/*##  fmtminutes - format a minutes string */
/*  SYNOPSIS */
/*   fmtminutes( itime, buf ) */
void fmtminutes( int itime, char *buf )
{
  int i, days, hours, minutes;
  char junk[32];
  int minus;
  
  if ( itime < 0 )
    {
      minus = TRUE;
      i = -itime;
    }
  else
    {
      minus = FALSE;
      i = itime;
    }
  
  minutes = mod( i, 60 );		/* minutes */
  i = i / 60;
  hours = mod( i, 24 );		/* hours */
  days = i / 24;			/* days */
  
  if ( minus )
    if ( days > 0 )
      days = -days;
    else if ( hours > 0 )
      hours = -hours;
    else
      minutes = -minutes;
  
  /* Format time. */
  sprintf( junk, "%d %2d:%02d", days, hours, minutes );
  
  /* Skip the junk and find the beginning. */
  for ( i = 0; junk[i] == ' ' || junk[i] == ':' || junk[i] == '0'; i = i + 1 )
    ;
  
  /* Store in return buffer. */
  /*    scopy( junk, i, buf, 1 );*/
  strcpy(buf, &junk[i]);
  
  return;
  
}


/*##  fmtseconds - format a seconds string */
/*  SYNOPSIS */
/*   fmtseconds( itime, buf ) */
void fmtseconds( int itime, char *buf )
{
  int i, days, hours, minutes, seconds;
  char junk[32];
  int minus;
  
  if ( itime < 0 )
    {
      minus = TRUE;
      i = -itime;
    }
  else
    {
      minus = FALSE;
      i = itime;
    }
  
  seconds = mod( i, 60 );		/* seconds */
  i = i / 60;
  minutes = mod( i, 60 );		/* minutes */
  i = i / 60;
  hours = mod( i, 24 );		/* hours */
  days = i / 24;			/* days */
  
  if ( minus )
    if ( days > 0 )
      days = -days;
    else if ( hours > 0 )
      hours = -hours;
    else if ( minutes > 0 )
      minutes = -minutes;
    else
      seconds = -seconds;
  
  /* Format time. */
  sprintf( junk, "%d %2d:%02d:%02d", days, hours, minutes, seconds );
  
  strcpy(buf, junk);
  
  return;
  
}


/*##  getamsg - find the next readable message */
/*  SYNOPSIS */
/*    int gotone, getamsg */
/*    int snum, msg */
/*    gotone = getamsg( snum, msg ) */
int getamsg( int snum, int *msg )
{
  
  while ( *msg != *lastmsg )
    {
      *msg = modp1( *msg + 1, MAXMESSAGES );
      
      /* If we can read it, only do so if it's not from us to GOD. */
      
      if ( canread( snum, *msg ) )
	return(MSG_GOD != msgto[*msg]);
	
      /*      	return ( snum != msgfrom[*msg] || MSG_GOD != msgto[*msg] );*/
    }
  
  return ( FALSE );
  
}


/*##  getcx - prompt for a string, centered */
/*  SYNOPSIS */
/*    char pmt(), */
/*    int lin, offset */
/*    char terms(), buf() */
/*    int len */
/*    tch = getcx( pmt, lin, offset, terms, buf, len ) */
char getcx( char *pmt, int lin, int offset, char *terms, char *buf, int len )
{
  int i;
  
  i = ( cdcols(0) - strlen( pmt ) ) / 2 + offset;
  if ( i <= 0 )
    i = 1;
  move(lin, 0);
  clrtoeol();
  return ( cdgetx( pmt, lin, i, terms, buf, len ) );
  
}


/*##  getdandt - get the date and time into a string */
/*  SYNOPSIS */
/*    char buf() */
/*    getdandt( buf ) */
void getdandt( char *buf )
{
  int now[8];
  char junk[5];
  
  getnow( now );
  switch ( now[2] )
    {
    case 1:
      c_strcpy( "Jan", junk );
      break;
    case 2:
      c_strcpy( "Feb", junk );
      break;
    case 3:
      c_strcpy( "Mar", junk );
      break;
    case 4:
      c_strcpy( "Apr", junk );
      break;
    case 5:
      c_strcpy( "May", junk );
      break;
    case 6:
      c_strcpy( "Jun", junk );
      break;
    case 7:
      c_strcpy( "Jul", junk );
      break;
    case 8:
      c_strcpy( "Aug", junk );
      break;
    case 9:
      c_strcpy( "Sep", junk );
      break;
    case 10:
      c_strcpy( "Oct", junk );
      break;
    case 11:
      c_strcpy( "Nov", junk );
      break;
    case 12:
      c_strcpy( "Dec", junk );
      break;
    default:
      c_strcpy( "???", junk );
      break;
    }
  sprintf( buf, "%2d:%02d:%02d %02d%s%02d",
	 now[4], now[5], now[6], now[3], junk, mod( now[1], 100 ) );
  
  return;
  
}


/*##  gettarget - get a target angle from the user */
/*  SYNOPSIS */
/*    char pmt() */
/*    int lin, col */
/*    real dir */
/*    int flag, gettarget */
/*    flag = gettarget( pmt, lin, col, dir ) */
int gettarget( char *pmt, int lin, int col, real *dir, real cdefault )
{
  int i, j; 
  char ch, buf[MSGMAXLINE];
  
  cdclrl( lin, 1 );
  ch = cdgetx( pmt, lin, col, TERMS, buf, MSGMAXLINE );
  if ( ch == TERM_ABORT )
    return ( FALSE );
  
  delblanks( buf );
  fold( buf );
  if ( buf[0] == EOS )
    {
      /* Default. */
      *dir = cdefault;
      return ( TRUE );
    }
  if ( alldig( buf ) == YES )
    {
      i = 0;
      if ( ! safectoi( &j, buf, i ) )
	return ( FALSE );
      *dir = mod360( (real) j );
      return ( TRUE );
    }
  if ( arrows( buf, dir ) )
    return ( TRUE );
  
  return ( FALSE );
  
}


/*##  grand - thousands since midnight */
/*  SYNOPSIS */
/*    int h */
/*    grand( h ) */
void grand( int *h )
{
  int now[8];
  
  getnow( now );
  *h = ( ( ( now[4] * 60 ) + now[5] ) * 60 + now[6] ) * 1000 + now[7];
  
  return;
  
}


/*##  gsecs - seconds since midnight */
/*  SYNOPSIS */
/*    int s */
/*    gsecs( s ) */
void gsecs( int *s )
{
  int now[8];
  
  getnow( now );
  *s = ( ( now[4] * 60 ) + now[5] ) * 60 + now[6];
  
  return;
  
}


/*##  mod360 - modularize a real number to 0.0 <= r < 360.0 */
/*  SYNOPSIS */
/*    real mr, mod360, r */
/*    mr = mod360( r ) */
real mod360( real r )
{
  real mr;
  
  mr = r;
  
  while ( mr < 0.0 )
    mr += 360.0;
  
  return((real) fmodf(mr, 360.0));
  
}


/*##  modp1 - modulus plus one */
/*  SYNOPSIS */
/*    int mi, modp1, i, modulus */
/*    mi = modp1( i, modulus ) */
int modp1( int i, int modulus )
{
  int m, n;
  
  m = i;
  
  while ( m < 0 )
    m += modulus;
  
  n = mod(--m, modulus) + 1;
  return((n >= modulus) ? 0 : n);
  
  /*    return((n == 0) ? 1 : n);*/
  
  /*    while ( m < 1 )
	m = m + modulus;
	
	return ( mod( m-1, modulus ) + 1 );
	*/
}

/*##  more - wait for the user to type a space */
/*  SYNOPSIS */
/*    char pmt() */
/*    int spacetyped, more */
/*    spacetyped = more( pmt ) */
int more( char *pmt )
{
  int ch; 
  string pml="--- press space for more ---";
  
  if ( pmt[0] != EOS )
    putpmt( pmt, MSG_LIN2 );
  else
    putpmt( pml, MSG_LIN2 );
  
  cdrefresh( TRUE );
  ch = iogchar( ch );
  return ( ch == ' ' );
  
}


/*##  pagefile - page through a file */
/*  SYNOPSIS */
/*    char file(), errmsg() */
/*    int ignorecontroll, eatblanklines */
/*    pagefile( file, errmsg, ignorecontroll, eatblanklines ) */
void pagefile( char *file, char *errmsg, int ignorecontroll, int eatblanklines )
{
  
  int plins = 1;
  FILE *pfd;
  string sdone="--- press any key to return ---";
  char buffer[BUFFER_SIZE];
  int buflen;
  
  if ((pfd = fopen(file, "r")) == NULL)
    {
      clog("pagefile(): fopen(%s) failed: %s",
	   file,
	   sys_errlist[errno]);
      
      cdclear();
      cdredo();
      cdputc( errmsg, MSG_LIN2/2 );
      more( sdone );
      
      return;
    }
  
  cdclear();
  cdrefresh(TRUE);
  cdmove(0, 0);
  
  plins = 0;
  
  while (fgets(buffer, BUFFER_SIZE - 1, pfd) != NULL)
    { 
      /* get one at a time */
      buflen = strlen(buffer);
      
      buffer[buflen - 1] = EOS; /* remove trailing LF */
      buflen--;
      
      if (buffer[0] == 0x0c)	/* formfeed */
	{
	  plins = DISPLAY_LINS + 1; /* force new page */
	}
      else
	{
    	  cdputs(buffer, plins, 0);
	  plins++;
	}
      
      if (plins >= DISPLAY_LINS)
	{
	  if (!more("--- press space for more ---"))
	    break;		/* bail if space not hit */
	  
	  cdclear();
	  plins = 1;
	}
    }
  
  fclose(pfd);
  
  more(sdone);
  
  return;
  
}


/*##  putmsg - display a message on the bottom of the user's screen */
/*  SYNOPSIS */
/*    char msg() */
/*    int line */
/*    c_putmsg( msg, line ) */
void c_putmsg( char *msg, int line )
{
  cdclrl( line, 1 );
  cdputs( msg, line, 1 );
  
  return;
  
}


/*##  putpmt - display a prompt */
/*  SYNOPSIS */
/*    char pmt() */
/*    int line */
/*    putpmt( pmt, line ) */
void putpmt( char *pmt, int line )
{
  int i, dcol, pcol;
  
  i = strlen( pmt );
  dcol = ( cdcols(0) - i ) / 2;
  pcol = dcol + i;
  cdclrl( line, 1 );
  cdputs( pmt, line, dcol );
  cdmove( line, pcol );
  
  return;
  
}


/*##  safectoi - char to int conversion with overflow protection */
/*  SYNOPSIS */
/*    int flag, safectoi */
/*    int num, ptr */
/*    char buf() */
/*    flag = safectoi( num, buf ptr ) */
int safectoi( int *num, char *buf, int ptr )
{
  int retval;

  retval = FALSE;
  /* If the number is the same size as the biggest integer, */
  /*  assume that it is too big. */

  if (buf[ptr] == EOS)
    {		
      *num = 0;
      retval = FALSE;
    }

  *num = ctoi( &buf[ptr]);
  retval = TRUE;
  
  if ( *num >= INT_MAX )
    {
      *num = INT_MAX;
      retval = FALSE;
    }
  
  return ( retval );
  
}


/*##  special - check if a string is a valid "special" specifier */
/*  SYNOPSIS */
/*    char str() */
/*    int what, token, count */
/*    int flag, special */
/*    flag = special( str, what, token, count ) */
int special( char *str, int *what, int *token, int *count )
{
  int i; 
  int l; 
  char buf[20];
  
  *what = NEAR_ERROR;
  *token = SPECIAL_NOTSPECIAL;
  *count = 0;
  
  /* Reject obvious losers. */
  if ( str[0] != 'n' && str[0] != 'w' && str[0] != 'h' )
    return ( FALSE );
  
  stcpn( str, buf, 20 );			/* need a private copy */
  
  /* Find threshold count; cleverly, the default will be zero when using ctoi. */
  for ( i = 0; buf[i] != EOS && c_type( buf[i] ) != DIGIT; i = i + 1 )
    ;
  buf[i] = EOS;				/* ditch numeric part */
  l =  safectoi( count, str, i );		/* ignore status */
  
  if ( stmatch( buf, "nes", FALSE ) )	/* this one must be first */
    {
      *what = NEAR_SHIP;
      *token = SPECIAL_ENEMYSHIP;
    }
  else if ( stmatch( buf, "nfp", FALSE ) )
    {
      *what = NEAR_PLANET;
      *token = SPECIAL_FUELPLANET;
    }
  else if ( stmatch( buf, "nep", FALSE ) )
    {
      *what = NEAR_PLANET;
      *token = SPECIAL_ENEMYPLANET;
    }
  else if ( stmatch( buf, "ns", FALSE ) )
    {
      *what = NEAR_SHIP;
      *token = SPECIAL_SHIP;
    }
  else if ( stmatch( buf, "np", FALSE ) )
    {
      *what = NEAR_PLANET;
      *token = SPECIAL_PLANET;
    }
  else if ( stmatch( buf, "nts", FALSE ) )
    {
      *what = NEAR_SHIP;
      *token = SPECIAL_TEAMSHIP;
    }
  else if ( stmatch( buf, "nap", FALSE ) )
    {
      *what = NEAR_PLANET;
      *token = SPECIAL_ARMYPLANET;
      if ( *count <= 0 )
	*count = 1;
    }
  else if ( stmatch( buf, "wp", FALSE ) )
    {
      *what = NEAR_PLANET;
      *token = SPECIAL_WEAKPLANET;
    }
  else if ( stmatch( buf, "ntp", FALSE ) )
    {
      *what = NEAR_PLANET;
      *token = SPECIAL_TEAMPLANET;
    }
  else if ( stmatch( buf, "nrp", FALSE ) )
    {
      *what = NEAR_PLANET;
      *token = SPECIAL_REPAIRPLANET;
    }
  else if ( stmatch( buf, "hp", FALSE ) )
    {
      *what = NEAR_PLANET;
      *token = SPECIAL_HOMEPLANET;
    }
  else
    return ( FALSE );		/* string simply isn't special */
  
  return ( TRUE );
  
}


/*##  stcpn - copy a string with a size limit */
/*  SYNOPSIS */
/*    char from(), to() */
/*    int tosize */
/*    stcpn( from, to, tosize ) */
void stcpn( char *from, char *to, int tosize )
{
  strncpy(to, from, tosize);
  to[tosize - 1] = '\0';
  
}


/*##  stmatch - check whether two strings match or not */
/*  SYNOPSIS */
/*    int matched, stmatch, casesensitive */
/*    char str1(), str2() */
/*    matched = stmatch( str1, str2, casesensitive ) */
int stmatch( char *str1, char *str2, int casesensitive )
{
  int i;
  
  if ( casesensitive )
    for ( i = 0; str1[i] == str2[i] && str1[i] != EOS; i = i + 1 )
      ;
  else
    for ( i = 0;
	 clower(str1[i]) == clower(str2[i]) && str1[i] != EOS;
	 i = i + 1 )
      ;
  
  if ( i == 0 )
    {
      if ( str1[0] == EOS && str2[0] == EOS )
	return ( TRUE );
      else
	return ( FALSE );
    }
  else if ( str1[i] == EOS || str2[i] == EOS )
    return ( TRUE );
  
  return ( FALSE );
  
}


/*##  subang - find smallest difference between angles. */
/*  SYNOPSIS */
/*    real h, subang, a1, a2 */
/*    h = subang( a1, a2 ) */
real subang( real a1, real a2 )
{
  real x;
  
  x = a1 - a2;
  while ( x > 180.0 )
    x = x - 360.0;
  while ( x < -180.0 )
    x = x + 360.0;
  
  return ( x );
  
}

