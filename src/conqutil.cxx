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

#include "c_defs.h"

#include <algorithm>
using namespace std;
#include "format.h"

#include "conqdef.h"
#include "cb.h"
#include "global.h"
#include "context.h"
#include "color.h"
#include "conqlb.h"
#include "conqutil.h"


/* if set, utLog uses the system log, else it is created in HOME */
static int systemlog   = true;
static int echo2stderr = false;  /* whether to echo utLog to stderr */
                                 /* this will be ignored if systemlog is
                                    true */

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
    std::string buffer;

    utAppendKilledBy(buffer, kb, detail);

    strcat(buf, buffer.c_str());

    return;
}

void utAppendKilledBy(std::string& buf, killedBy_t kb, uint16_t detail)
{

    switch ( kb )
    {
    case KB_SELF:
        buf += "self";
        break;
    case KB_NEGENB:
        buf += "negenb";
        break;
    case KB_CONQUER:
        buf += "conquer";
        break;
    case KB_NEWGAME:
        buf += "newgame";
        break;
    case KB_EVICT:
        buf += "evict";
        break;
    case KB_SHIT:
        buf += "shit";
        break;
    case KB_DOOMSDAY:
        buf += "doomsday";
        break;
    case KB_GOTDOOMSDAY:
        buf += "gotdoomsday";
        break;
    case KB_GOD:
        buf += "GOD";
        break;
    case KB_SHIP:
        if (detail < cbLimits.maxShips())
            utAppendShip(buf, detail);
        break;
    case KB_PLANET:
        if (detail < cbLimits.maxPlanets())
            buf += cbPlanets[detail].name ;
        break;
    default:
        buf += std::to_string((int)kb);
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
    std::string buffer;

    utAppendShip(buffer, snum);

    strcat(str, buffer.c_str());
    return;
}


void utAppendShip(std::string& str, int snum)
{
    int i;
    char ch = 'S';

    if ( snum >= 0 && snum < cbLimits.maxShips() )
    {
        i = cbShips[snum].team;
        if ( i >= 0 && i < NUMPLAYERTEAMS )
            ch = cbTeams[i].teamchar;
    }

    str += fmt::format("{}{}", ch, snum);

    return;
}


// return a ship string of the form "<team char><snum>"
const std::string utShipStr(int snum)
{
    std::string buf;

    utAppendShip(buf, snum);
    return buf;
}

/*  utAppendShipStatus - append ship status string */
/*  SYNOPSIS */
/*    int status */
/*    char buf() */
/*    utAppendShipStatus(buf , status)  */
void utAppendShipStatus(char *buf, int status)
{
    std::string buffer;

    utAppendShipStatus(buffer, status);

    strcat(buf, buffer.c_str());

    return;
}

void utAppendShipStatus(std::string& buf, int status)
{
    switch ( status )
    {
    case SS_OFF:
        buf += "off";
        break;
    case SS_ENTERING:
        buf += "entering";
        break;
    case SS_LIVE:
        buf += "live";
        break;
    case SS_DYING:
        buf += "dying";
        break;
    case SS_DEAD:
        buf += "dead";
        break;
    case SS_RESERVED:
        buf += "reserved";
        break;
    default:
        buf += std::to_string(status);
        break;
    }
    return;

}


/*  utAppendTitle - append a team oriented title */
/*  SYNOPSIS */
/*    int team */
/*    char buf() */
/*    utAppendTitle(buf , team)  */
void utAppendTitle(std::string& buf, int team)
{
    switch ( team )
    {
        case TEAM_FEDERATION:
            buf += "Captain";
            break;
        case TEAM_ROMULAN:
            buf += "Centurion";
            break;
        case TEAM_ORION:
            buf += "Commander";
            break;
        case TEAM_KLINGON:
            buf += "Kommander";
            break;
        default:
            // shouldn't happen, but...
            buf += "Pirate";
    }

    return;
}


/*  utArrowsToDir - interpret arrow keys */
int utArrowsToDir( const char *str, real *dir )
{
    int i, n;
    real thedir, ndir, ndir1, ndir2;
    // the '*' is a placeholder for not found/invalid (0)
    const char *arrs="*dewqazxc";

    /* Special hack preventing "ea" and "da" from being recognized as arrows. */
    /* "ea" is reserved for Earth and "da" for Dakel. */
    if ( (char)tolower(str[0]) == 'e' && (char)tolower(str[1]) == 'a' )
        return ( false );
    if ( (char)tolower(str[0]) == 'd' && (char)tolower(str[1]) == 'a' )
        return ( false );

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
            return ( false );

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

    return ( true );

}

void utSetLogConfig(int usesys, int echostderr)
{
    if (usesys)
    {
        systemlog = true;
        echo2stderr = false;
    }
    else
    {
        systemlog = false;
        echo2stderr = echostderr;
    }

    return;
}

void utLog(const char *fmt, ...)
{
    va_list ap;
    static int nowarn = false;     /* if set, ignore logging */
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
            snprintf(errfile, PATH_MAX, "%s/%s/%s", CONQSTATE,
                     gameSubdirectory.get().c_str(), C_CONQ_ERRLOG);
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
                    nowarn = true;
                }

                if (!systemlog)
                {
                    /* if the logfile could not be created for a user,
                       keep trying (the .conquest/ dir may not exist
                       yet), but only log an error once. */
                    nowarn = true;
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
                // don't whine on EPERM.  Many systems don't allow
                // ordinary users to chown anymore
                if (errno != EPERM)
                    perror("utLog():chown()");
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
    if (!str)
        return;

    int i, j;

    for ( i = 0; str[i] != 0; )
        if ( str[i] == ' ' )
            for ( j = i; str[j] != 0; j++ )
                str[j] = str[j+1];
        else
            i++;

    return;

}

void utDeleteBlanks( std::string& str )
{
    if (str.empty())
        return;

    str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
    return;
}


/*  utDeltaGrand - delta time for thousands */
/*  SYNOPSIS */
/*    int i, utDeltaGrand, s, n */
/*    i = utDeltaGrand( s, n ) */
uint64_t utDeltaGrand( uint64_t s, uint64_t *n )
{
    uint64_t tn, ts;

    /* Save s in case it and n are the same variable. */
    ts = s;

    /* Get thousands since midnight. */
    utGrand( &tn );
    *n = tn;

    /* Calculate the difference. */
    if ( tn < ts )
        return (ts - tn);
    else
        return (tn - ts);
}


/*  utDeltaSecs - delta time for seconds */
/*  SYNOPSIS */
/*    int i, utDeltaSecs, s, n */
/*    i = utDeltaSecs( s, n ) */

time_t utDeltaSecs( time_t s, time_t *n )
{
    time_t tn, ts;

    /* Save s in case it and n are the same variable. */
    ts = s;

    /* Get seconds since midnight. */
    utGetSecs( &tn );
    *n = tn;

    /* Calculate the difference. */
    if ( tn < ts )
        return (ts - tn);
    else
        return (tn - ts);
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
    std::string tbuf;

    utFormatMinutes(itime, tbuf);
    strcpy(buf, tbuf.c_str());
    return;
}

void utFormatMinutes( int itime, std::string& buf )
{
    int i, days, hours, minutes;
    int minus;

    if ( itime < 0 )
    {
        minus = true;
        i = -itime;
    }
    else
    {
        minus = false;
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

    std::string tmp;
    if (days)
        tmp = fmt::format("{} {:2d}:{:02d}", days, hours, minutes);
    else if (hours)
        tmp = fmt::format("{:2d}:{:02d}", hours, minutes);
    else
        tmp = fmt::format("{:02d}", minutes);

    buf = tmp;

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
        minus = true;
        i = -itime;
    }
    else
    {
        minus = false;
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
    while ( *msg != cbConqInfo->lastmsg )
    {
        *msg = mod( *msg + 1, cbLimits.maxMsgs() );
        if ( clbCanRead( snum, *msg ) )
            return(true);

    }
    return ( false );
}


/*  utFormatTime - get the date and time into a string */
/*  SYNOPSIS */
/*    char buf() */
/*    utFormatTime( buf ) */
void utFormatTime( char *buf, time_t thetime )
{
    std::string buffer;

    utFormatTime(buffer, thetime);
    strcpy(buf, buffer.c_str());

    return;
}

void utFormatTime( std::string& buf, time_t thetime )
{
    char junk[5];

    if (!thetime)
        thetime = time(0);

    struct tm *thetm = localtime(&thetime);

    strftime(junk, 5, "%b", thetm);

    buf = fmt::format("{:2d}:{:02d}:{:02d} {:02d}{}{:02d}",
                      thetm->tm_hour, thetm->tm_min,
                      thetm->tm_min, thetm->tm_mday,
                      junk,
                      mod(thetm->tm_year + 1900, 100));

    return;
}

// millisecs since the epoch
void utGrand( uint64_t *h )
{
    struct timeval now;

    GETTIMEOFDAY(&now);

    *h = uint64_t((now.tv_sec * 1000)
                  + (now.tv_usec / 1000));

    return;
}


// secs since the epoch
void utGetSecs( time_t *s )
{
    struct timeval now;

    GETTIMEOFDAY(&now);

    *s = now.tv_sec;

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

/*  utSafeCToI - char to int conversion with overflow protection */
/*  SYNOPSIS */
/*    int flag, utSafeCToI */
/*    int num, ptr */
/*    char buf() */
/*    flag = utSafeCToI( num, buf ptr ) */
bool utSafeCToI( int *num, const std::string& buf, int offset )
{
    if (buf.empty() || !num)
        return false;

    if (offset >= buf.size())
    {
        // some code depends on this behavior
        *num = 0;
        return false;
    }

    long rv = strtol(&buf[offset], NULL, 10);

    // check for validity
    if ( rv >= INT_MAX || rv == LONG_MIN || rv == LONG_MAX)
    {
        *num = INT_MAX;
        return false;
    }

    *num = int(rv);

    return ( true );
}


/*  special - check if a string is a valid "special" specifier */
bool utIsSpecial( const std::string& str, int *what, int *token, int *count )
{
    int i;
    std::string buf;

    *what = NEAR_ERROR;
    *token = SPECIAL_NOTSPECIAL;
    *count = 0;

    /* Reject obvious losers. */
    if ( str[0] != 'n' && str[0] != 'w' && str[0] != 'h' )
        return ( false );

    buf = str;                  /* need a private copy */

    // find threshold count if present
    size_t digits = buf.find_first_of("0123456789");

    if (digits != std::string::npos)
    {
        utSafeCToI(count, buf.substr(digits), 0);
        // remove the digits
        buf.erase(digits);
    }

    if ( utStringMatch( buf, "nes", false ) )	/* this one must be first */
    {
        *what = NEAR_SHIP;
        *token = SPECIAL_ENEMYSHIP;
    }
    else if ( utStringMatch( buf, "nfp", false ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_FUELPLANET;
    }
    else if ( utStringMatch( buf, "nep", false ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_ENEMYPLANET;
    }
    else if ( utStringMatch( buf, "ns", false ) )
    {
        *what = NEAR_SHIP;
        *token = SPECIAL_SHIP;
    }
    else if ( utStringMatch( buf, "np", false ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_PLANET;
    }
    else if ( utStringMatch( buf, "nts", false ) )
    {
        *what = NEAR_SHIP;
        *token = SPECIAL_TEAMSHIP;
    }
    else if ( utStringMatch( buf, "nap", false ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_ARMYPLANET;
        if ( *count <= 0 )
            *count = 1;
    }
    else if ( utStringMatch( buf, "wp", false ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_WEAKPLANET;
    }
    else if ( utStringMatch( buf, "ntp", false ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_TEAMPLANET;
    }
    else if ( utStringMatch( buf, "nrp", false ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_REPAIRPLANET;
    }
    else if ( utStringMatch( buf, "hp", false ) )
    {
        *what = NEAR_PLANET;
        *token = SPECIAL_HOMEPLANET;
    }
    else
        return false;       // string simply isn't special :(

    return true;
}


/*  utStrncpy - copy a string with a size limit, ensuring the last
 *  byte is 0 */
void utStrncpy( char *to, const char *from, unsigned int tosize )
{
    strncpy(to, from, tosize - 1);
    to[tosize - 1] = '\0';
}

/*  utStrncat - concatenate a string with a size limit, ensuring the
 *  last byte is 0 */
void utStrncat( char *to, const char *from, unsigned int tosize )
{
    size_t toLen = strlen(to);
    strncat(to, from, tosize - toLen - 1);
    to[tosize - 1] = '\0';
}

static bool _strcompare_pred(unsigned char a, unsigned char b)
{
    return std::tolower(a) == std::tolower(b);
}

/*  utStringMatch - check whether two strings match or not */
bool utStringMatch(const std::string& str1, const std::string& str2,
                   bool casesensitive )
{
    if (str1.empty() && str2.empty())
        return true;
    else if (str1.empty() || str2.empty())
        return false;

    if (casesensitive)
    {
        // this is never actually used in Conquest, but we'll keep the
        // support for it just in case...
        return str1 == str2;
    }

    // we only check matching characters up to the length of str1
    return std::equal(str1.begin(), str1.end(),
                      str2.begin(), _strcompare_pred);
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
    utStrncpy(retpath, thepath, PATH_MAX);
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
        snprintf(retpath, PATH_MAX, "%s%s/etc", theroot, defaultConq);
    }
    else if (!strcmp(thepath, CONQSHARE))
    {                           /* share */
        snprintf(retpath, PATH_MAX, "%s%s", theroot, defaultConq);
    }
    else
    {                           /* default, just return original path */
        snprintf(retpath, PATH_MAX, "%s", thepath);
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

bool utIsDigits(const std::string& str)
{
    return std::all_of(str.begin(), str.end(), ::isdigit);
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
