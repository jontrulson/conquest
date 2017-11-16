#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q U T I L */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */


#include "conqdef.h"
#include "conqcom.h"
#include "global.h"
#include "context.h"
#include "color.h"
#include "conqlb.h"
#include "conqutil.h"

/* if set, utLog uses the system log, else it is created in HOME */
static int systemlog   = TRUE;
static int echo2stderr = FALSE;  /* whether to echo utLog to stderr */
                                 /* this will be ignored if systemlog is
                                    TRUE */

/*  utAccurateDist - figure distance traveled while changing velocities */
/*  SYNOPSIS */
/*    real dis, curvel, newvel, acc */
/*    dis = utAccurateDist( curvel, newvel, acc ) */
/*  DESCRIPTION */
/*    These formulas works for de-acceleration only. */
real utAccurateDist( real curvel, real newvel, real acc )
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


/*  angle - compute the angle between two points */
/*  SYNOPSIS */
/*    real ang, angle, fromx, fromy, tox, toy */
/*    ang = utAngle( fromx, fromy, tox, toy ) */
real utAngle( real fromx, real fromy, real tox, real toy )
{
    if ( fromx == tox && fromy == toy )
        return ( 0.0 );

    return ( utMod360( rtod( atan2( toy - fromy, tox - fromx ) ) ) );

}


/*  utAppendInt - append an int to a string */
/*  SYNOPSIS */
/*    int int */
/*    char str() */
/*    utAppendInt(str , i)  */
void utAppendInt(char *str, int i)
{
    char buf[BUFFER_SIZE_256];

    buf[0] = 0;
    sprintf(buf, "%d", i);
    strcat(str , buf) ;

    return;

}


/*  utAppendKilledBy - append killed by string */
/*  SYNOPSIS */
/*    int kb */
/*    char buf() */
/*    utAppendKilledBy(buf , kb)  */
void utAppendKilledBy(char *buf, killedBy_t kb, uint16_t detail)
{

    switch ( kb )
    {
    case KB_SELF:
        strcat(buf , "self") ;
        break;
    case KB_NEGENB:
        strcat(buf , "negenb") ;
        break;
    case KB_CONQUER:
        strcat(buf , "conquer") ;
        break;
    case KB_NEWGAME:
        strcat(buf , "newgame") ;
        break;
    case KB_EVICT:
        strcat(buf , "evict") ;
        break;
    case KB_SHIT:
        strcat(buf , "shit") ;
        break;
    case KB_DOOMSDAY:
        strcat(buf , "doomsday") ;
        break;
    case KB_GOTDOOMSDAY:
        strcat(buf , "gotdoomsday") ;
        break;
    case KB_GOD:
        strcat(buf , "GOD") ;
        break;
    case KB_SHIP:
        if (detail < MAXSHIPS)
            utAppendShip(buf, detail) ;
        break;
    case KB_PLANET:
        if (detail < MAXPLANETS )
            strcat(buf , Planets[detail].name) ;
        break;
    default:
        utAppendInt(buf, (int)kb) ;
        break;
    }

    return;

}

/*  utAppendShip - append a ship number to a string */
/*  SYNOPSIS */
/*    int snum */
/*    char str() */
/*    utAppendShip(str , snum)  */
void utAppendShip(char *str, int snum)
{
    int i;
    char ch;

    ch = 'S';
    if ( snum >= 0 && snum < MAXSHIPS )
    {
        i = Ships[snum].team;
        if ( i >= 0 && i < NUMPLAYERTEAMS )
            ch = Teams[i].teamchar;
    }

    utAppendChar(str, ch) ;
    utAppendInt(str, snum) ;

    return;
}


/*  utAppendShipStatus - append ship status string */
/*  SYNOPSIS */
/*    int status */
/*    char buf() */
/*    utAppendShipStatus(buf , status)  */
void utAppendShipStatus(char *buf, int status)
{
    switch ( status )
    {
    case SS_OFF:
        strcat(buf , "off") ;
        break;
    case SS_ENTERING:
        strcat(buf , "entering") ;
        break;
    case SS_LIVE:
        strcat(buf , "live") ;
        break;
    case SS_DYING:
        strcat(buf , "dying") ;
        break;
    case SS_DEAD:
        strcat(buf , "dead") ;
        break;
    case SS_RESERVED:
        strcat(buf , "reserved") ;
        break;
    default:
        utAppendInt(buf, status) ;
        break;
    }
    return;

}


/*  utAppendTitle - append a team oriented title */
/*  SYNOPSIS */
/*    int team */
/*    char buf() */
/*    utAppendTitle(buf , team)  */
void utAppendTitle(char *buf, int team)
{
    switch ( team )
    {
    case TEAM_FEDERATION:
        strcat(buf , "Captain") ;
        break;
    case TEAM_ROMULAN:
        strcat(buf , "Centurion") ;
        break;
    case TEAM_ORION:
        strcat(buf , "Commander") ;
        break;
    case TEAM_KLINGON:
        strcat(buf , "Kommander") ;
        break;
    }

    return;

}


/*  utArrowsToDir - interpret arrow keys */
int utArrowsToDir( char *str, real *dir )
{
    int i, n;
    real thedir, ndir, ndir1, ndir2;
    // the '*' is a placeholder for not found/invalid (0)
    const char *arrs="*dewqazxc";

    /* Special hack preventing "ea" and "da" from being recognized as arrows. */
    /* "ea" is reserved for Earth and "da" for Dakel. */
    if ( (char)tolower(str[0]) == 'e' && (char)tolower(str[1]) == 'a' )
        return ( FALSE );
    if ( (char)tolower(str[0]) == 'd' && (char)tolower(str[1]) == 'a' )
        return ( FALSE );

    thedir = 0.0;

    for ( i = 0; str[i] != 0; i = i + 1 )
    {
        int idx = 0;

        // look for the 'arrow' in the string and return it's index
        const char *s = arrs;
        while (*s)
        {
            if (*s == tolower(str[i]))
                break;
            s++;
            idx++;
        }

        if (*s == 0) // not found
            return ( FALSE );

        ndir1 = (real)(idx - 1) * 45.0;
        ndir2 = (real)ndir1 - 360.0;

        if ( (real)fabs( thedir - ndir1 ) < (real)fabs( thedir - ndir2 ) )
            ndir = ndir1;
        else
            ndir = ndir2;

        n = i + 1;
        thedir = (((thedir*((real)n - 1)) + ndir ) / (real)n);
    }


    *dir = utMod360( thedir );

    return ( TRUE );

}

void utSetLogConfig(int usesys, int echostderr)
{
    if (usesys)
    {
        systemlog = TRUE;
        echo2stderr = FALSE;
    }
    else
    {
        systemlog = FALSE;
        echo2stderr = echostderr;
    }

    return;
}

void utLog(char *fmt, ...)
{
    va_list ap;
    static int nowarn = FALSE;     /* if set, ignore logging */
    static char buf[BUFFER_SIZE_256] = {};
    static char errfile[PATH_MAX] = {};
    static FILE *errfd = NULL;
    char *homevar;
    int tmp;

    va_start(ap, fmt);
    (void)vsnprintf(buf, BUFFER_SIZE_256, fmt, ap);

    va_end(ap);

    if (errfd == NULL)
    {

        if (systemlog)
	{
            umask(007);
            snprintf(errfile, PATH_MAX, "%s/%s", CONQSTATE, C_CONQ_ERRLOG);
            if (ConquestGID == ERR)
	    {
                fprintf(stderr, "conqutil: utLog():  ConquestGID == -1!\n");
                exit(1);
	    }
	}
        else
	{			/* local logfile */
            if ((homevar = getenv(CQ_USERHOMEDIR)) != NULL)
            {
                snprintf(errfile, PATH_MAX, "%s/%s/%s",
                         homevar, CQ_USERCONFDIR, C_CONQ_ERRLOG);
            }
            else
                snprintf(errfile, PATH_MAX, "%s", C_CONQ_ERRLOG);
	}

        if ((errfd = fopen(errfile, "a+")) == NULL)
	{
            if ((tmp = creat(errfile, 0660)) == -1)
	    {

                if (!nowarn)
                {
                    fprintf(stderr, "utLog(): creat(%s): %s\n",
                            errfile,
                            strerror(errno));
                    nowarn = TRUE;
                }

                if (!systemlog)
                {
                    /* if the logfile could not be created for a user,
                       keep trying (the .conquest/ dir may not exist
                       yet), but only log an error once. */
                    nowarn = TRUE;
                    return;
                }
                else
                    exit(1);
	    }
            else
	    {
                close(tmp);

#if !defined(MINGW)
                if (systemlog)
                    if (chmod(errfile,
                              (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)) == -1)
                    {
                        perror("utLog():chmod()");
                        exit(1);
                    }
#endif
	    }
	}
        else
	{
            fclose(errfd);
	}

#if !defined(MINGW)
        if (systemlog)
            if (chown(errfile, 0, ConquestGID) == -1)
            {
                perror("utLog():chown()");
                //            exit(1);
            }
#endif

        if ((errfd = fopen(errfile, "a+")) == NULL)
	{
            perror("utLog(): can't open logfile");
	}
    }

    if (errfd != NULL)
    {
        fprintf(errfd, "%ld:%d:%s\n", time(0), (int)getpid(), buf);
        fflush(errfd);
    }

    if (echo2stderr)
    {
        fprintf(stderr, "%s\n", buf);
        fflush(stderr);
    }


    return;

}

/*  utDeleteBlanks - remove all blanks from a string */
/*  SYNOPSIS */
/*    char str() */
/*    utDeleteBlanks( str ) */
void utDeleteBlanks( char *str )
{
    int i, j;

    for ( i = 0; str[i] != 0; )
        if ( str[i] == ' ' )
            for ( j = i; str[j] != 0; j = j + 1 )
                str[j] = str[j+1];
        else
            i = i + 1;

    return;

}


/*  utDeltaGrand - delta time for thousands */
/*  SYNOPSIS */
/*    int i, utDeltaGrand, s, n */
/*    i = utDeltaGrand( s, n ) */
int utDeltaGrand( int s, int *n )
{
    int tn, ts;

    /* Save s in case it and n are the same variable. */
    ts = s;

    /* Get thousands since midnight. */
    utGrand( &tn );
    *n = tn;

    /* Calculate the difference. */
    if ( tn < ts )
        tn = tn + 24 * 60 * 60 * 1000;		/* crossed midnight */

    return ( tn - ts );

}


/*  utDeltaSecs - delta time for seconds */
/*  SYNOPSIS */
/*    int i, utDeltaSecs, s, n */
/*    i = utDeltaSecs( s, n ) */

int utDeltaSecs( int s, int *n )
{
    int tn, ts;

    /* Save s in case it and n are the same variable. */
    ts = s;

    /* Get seconds since midnight. */
    utGetSecs( &tn );
    *n = tn;

    /* Calculate the difference. */
    if ( tn < ts )
        tn = tn + ((24 * 60) * 60);		/* crossed midnight */

    return ( tn - ts );

}


/*  explosion - hits based on distance */
/*  SYNOPSIS */
/*    real newhits, utExplosionHits, basehits, dis */
/*    newhits = utExplosionHits( basehits, dis ) */
real utExplosionHits( real basehits, real dis )
{
    if ( dis > PHASER_DIST )
        return ( 0.0 );
    return ( basehits / ( ( EXPLOSION_FALLOFF - 1.0 ) *
                          max( dis - EXPLOSION_RADIUS,
                               0.0 ) / PHASER_DIST + 1.0 ) -
             basehits / EXPLOSION_FALLOFF * dis / PHASER_DIST );

}


/*  utFormatMinutes - format a minutes string */
/*  SYNOPSIS */
/*   utFormatMinutes( itime, buf ) */
void utFormatMinutes( int itime, char *buf )
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
    {
        if ( days > 0 )
            days = -days;
        else if ( hours > 0 )
            hours = -hours;
        else
            minutes = -minutes;
    }

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


/*  utFormatSeconds - format a seconds string */
/*  SYNOPSIS */
/*   utFormatSeconds( itime, buf ) */
void utFormatSeconds( int itime, char *buf )
{
    int i, days, hours, minutes, seconds;
    char junk[BUFFER_SIZE_256];
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
    {
        if ( days > 0 )
            days = -days;
        else if ( hours > 0 )
            hours = -hours;
        else if ( minutes > 0 )
            minutes = -minutes;
        else
            seconds = -seconds;
    }

    /* Format time. */
    sprintf( junk, "%d %2d:%02d:%02d", days, hours, minutes, seconds );

    strcpy(buf, junk);

    return;

}


/*  utGetMsg - find the next readable message */
/*  SYNOPSIS */
/*    int gotone, utGetMsg */
/*    int snum, msg */
/*    gotone = utGetMsg( snum, msg ) */
int utGetMsg( int snum, int *msg )
{
    while ( *msg != ConqInfo->lastmsg )
    {
        *msg = utModPlusOne( *msg + 1, MAXMESSAGES );
        if ( clbCanRead( snum, *msg ) )
            return(TRUE);

    }
    return ( FALSE );
}


/*  utFormatTime - get the date and time into a string */
/*  SYNOPSIS */
/*    char buf() */
/*    utFormatTime( buf ) */
void utFormatTime( char *buf, time_t thetime )
{
    char junk[5] = {};

    if (!thetime)
        thetime = time(0);

    struct tm *thetm = localtime(&thetime);

    strftime(junk, 5, "%b", thetm);

    sprintf(buf, "%2d:%02d:%02d %02d%s%02d",
            thetm->tm_hour, thetm->tm_min, thetm->tm_min, thetm->tm_mday,
            junk,
            mod(thetm->tm_year + 1900, 100));

    return;

}


/*  grand - thousands since midnight */
/*  SYNOPSIS */
/*    int h */
/*    utGrand( h ) */
void utGrand( int *h )
{
    time_t thetime = time(0);
    struct tm *thetm = localtime(&thetime);

    *h = ( ( ( thetm->tm_hour * 60 )
             + thetm->tm_min ) * 60
           + thetm->tm_sec ) * 1000;

    return;

}


/*  utGetSecs - seconds since midnight */
/*  SYNOPSIS */
/*    int s */
/*    utGetSecs( s ) */
void utGetSecs( int *s )
{
    time_t thetime = time(0);
    struct tm *thetm = localtime(&thetime);

    *s = ( ( thetm->tm_hour * 60 ) + thetm->tm_min ) * 60 + thetm->tm_sec;

    return;

}


/*  utMod360 - modularize a real number to 0.0 <= r < 360.0 */
/*  SYNOPSIS */
/*    real mr, utMod360, r */
/*    mr = utMod360( r ) */
real utMod360( real r )
{
    real mr;

    mr = r;

    while ( mr < 0.0 )
        mr += 360.0;

    return((real) fmod(mr, 360.0));

}


/*  utModPlusOne - modulus plus one */
/*  SYNOPSIS */
/*    int mi, utModPlusOne, i, modulus */
/*    mi = utModPlusOne( i, modulus ) */
int utModPlusOne( int i, int modulus )
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


/*  utSafeCToI - char to int conversion with overflow protection */
/*  SYNOPSIS */
/*    int flag, utSafeCToI */
/*    int num, ptr */
/*    char buf() */
/*    flag = utSafeCToI( num, buf ptr ) */
int utSafeCToI( int *num, char *buf, int offset )
{
    int retval;
    retval = FALSE;

    if (!buf)
    {
        *num = 0;
        retval = FALSE;
    }

    int slen = strlen(buf);

    if (offset >= slen)
    {
        *num = 0;
        retval = FALSE;
    }

    *num = atoi( &buf[offset]);
    retval = TRUE;

    /* If the number is the same size as the biggest integer, */
    /*  assume that it is too big. */

    if ( *num >= INT_MAX )
    {
        *num = INT_MAX;
        retval = FALSE;
    }

    return ( retval );

}


/*  special - check if a string is a valid "special" specifier */
/*  SYNOPSIS */
/*    char str() */
/*    int what, token, count */
/*    int flag, special */
/*    flag = utIsSpecial( str, what, token, count ) */
int utIsSpecial( char *str, int *what, int *token, int *count )
{
    int i;
    char buf[20];

    *what = NEAR_ERROR;
    *token = SPECIAL_NOTSPECIAL;
    *count = 0;

    /* Reject obvious losers. */
    if ( str[0] != 'n' && str[0] != 'w' && str[0] != 'h' )
        return ( FALSE );

    utStcpn( str, buf, 20 );			/* need a private copy */

    /* Find threshold count; cleverly, the default will be zero when using ctoi. */
    for (i = 0; buf[i] != 0 && !isdigit(buf[i]); i++)
        ;

    buf[i] = 0;				/* ditch numeric part */
    utSafeCToI( count, str, i );		/* ignore status */

    if ( utStringMatch( buf, "nes", FALSE ) )	/* this one must be first */
    {
        *what = NEAR_SHIP;
        *token = SPECIAL_ENEMYSHIP;
    }
    else if ( utStringMatch( buf, "nfp", FALSE ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_FUELPLANET;
    }
    else if ( utStringMatch( buf, "nep", FALSE ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_ENEMYPLANET;
    }
    else if ( utStringMatch( buf, "ns", FALSE ) )
    {
        *what = NEAR_SHIP;
        *token = SPECIAL_SHIP;
    }
    else if ( utStringMatch( buf, "np", FALSE ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_PLANET;
    }
    else if ( utStringMatch( buf, "nts", FALSE ) )
    {
        *what = NEAR_SHIP;
        *token = SPECIAL_TEAMSHIP;
    }
    else if ( utStringMatch( buf, "nap", FALSE ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_ARMYPLANET;
        if ( *count <= 0 )
            *count = 1;
    }
    else if ( utStringMatch( buf, "wp", FALSE ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_WEAKPLANET;
    }
    else if ( utStringMatch( buf, "ntp", FALSE ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_TEAMPLANET;
    }
    else if ( utStringMatch( buf, "nrp", FALSE ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_REPAIRPLANET;
    }
    else if ( utStringMatch( buf, "hp", FALSE ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_HOMEPLANET;
    }
    else
        return ( FALSE );		/* string simply isn't special */

    return ( TRUE );

}


/*  utStcpn - copy a string with a size limit */
/*  SYNOPSIS */
/*    char from(), to() */
/*    int tosize */
/*    utStcpn( from, to, tosize ) */
void utStcpn( char *from, char *to, int tosize )
{
    strncpy(to, from, tosize);
    to[tosize - 1] = '\0';

}


/*  utStringMatch - check whether two strings match or not */
/*  SYNOPSIS */
/*    int matched, utStringMatch, casesensitive */
/*    char str1(), str2() */
/*    matched = utStringMatch( str1, str2, casesensitive ) */
int utStringMatch( char *str1, char *str2, int casesensitive )
{
    int i;

    if ( casesensitive )
        for ( i = 0; str1[i] == str2[i] && str1[i] != 0; i = i + 1 )
            ;
    else
        for ( i = 0;
              (char)tolower(str1[i]) == (char)tolower(str2[i]) && str1[i] != 0;
              i = i + 1 )
            ;

    if ( i == 0 )
    {
        if ( str1[0] == 0 && str2[0] == 0 )
            return ( TRUE );
        else
            return ( FALSE );
    }
    else if ( str1[i] == 0 || str2[i] == 0 )
        return ( TRUE );

    return ( FALSE );

}


/*  utSubAngle - find smallest difference between angles. */
/*  SYNOPSIS */
/*    real h, utSubAngle, a1, a2 */
/*    h = utSubAngle( a1, a2 ) */
real utSubAngle( real a1, real a2 )
{
    real x;

    x = a1 - a2;
    while ( x > 180.0 )
        x = x - 360.0;
    while ( x < -180.0 )
        x = x + 360.0;

    return ( x );

}

/* Under windows, we play a few more games to figure out where
 * system-related stuff can be found, like souncd/img/etc... For non
 * windows (!MINGW) systems we simply return the path arg that was
 * passed in.
 *
 * For windows, we will look for an env var: CONQUEST_ROOT, which if
 * present will be used to locate the other dirs.  If this env var is
 * not set, we will use Windows $COMMONPROGRAMFILES/Conquest/.  If
 * that fails, we will use just '/Conquest'.
 *
 * At present, we only care about CONQETC and CONQSHARE.
 */

char *utGetPath(const char *thepath)
{
    static char retpath[PATH_MAX];

#if !defined(MINGW)
    /* the non-windows case just returns thepath, but we copy it
     * to avoid an annoying compiler warning regarding const.
     */
    strncpy(retpath, thepath, PATH_MAX - 1);
    return retpath;
#else

    char *theroot = NULL;
    char *defaultConq = "";

    if (!(theroot = getenv("CONQUEST_ROOT")))
    {
        /* next check COMMONPROGRAMFILES */
        if (!(theroot = getenv("COMMONPROGRAMFILES")))
        {
            theroot = "/Conquest";
        }
        else
        {                       /* Add /Conquest to COMMONPROGRAMFILES */
            defaultConq = "/Conquest";
        }
    }

    /* now determine the path requested and do our magic. */

    if (!strcmp(thepath, CONQETC))
    {                           /* etc */
        snprintf(retpath, PATH_MAX - 1, "%s%s/etc", theroot, defaultConq);
    }
    else if (!strcmp(thepath, CONQSHARE))
    {                           /* share */
        snprintf(retpath, PATH_MAX - 1, "%s%s", theroot, defaultConq);
    }
    else
    {                           /* default, just return original path */
        snprintf(retpath, PATH_MAX - 1, "%s", thepath);
    }

    return retpath;

#endif  /* MINGW */
}

void utSleep(real seconds)
{
    // convert to integer milliseconds
    int msecs = (int)(seconds * 1000.0f);

#if defined(MINGW)

    /* For windows, use WINAPI Sleep(millis) */
    Sleep((DWORD)(msecs));

#else
    // hope you have nanosleep()
    struct timespec delay_time;

    delay_time.tv_sec  = msecs / 1000;
    delay_time.tv_nsec = (msecs % 1000) * 1000000;
    // here we spin until the delay is complete - detecting signals
    // and continuing where we left off
    while (nanosleep(&delay_time, &delay_time) && errno == EINTR)
        ; // loop
#endif
}

bool utIsDigits(const char *buf)
{
    const char *s = buf;

    while (*s)
    {
        if (!isdigit(*s))
            return false;
        s++;
    }

    return true;
}

/* upcase a string */
void utToUpperCase(char *buf)
{
    char *s = buf;
    while(*s)
    {
        *s = (char)toupper(*s);
        s++;
    }
}

// append a character to a string
void utAppendChar(char *buf, char ch)
{
    int len = strlen(buf);
    buf[len++] = ch;
    buf[len] = 0;
}
