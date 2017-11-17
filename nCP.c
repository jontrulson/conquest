/*
 * cockpit node
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "conqcom.h"
#include "conqlb.h"
#include "rndlb.h"
#include "conqutil.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "record.h"
#include "ibuf.h"
#include "prm.h"
#include "cqkeys.h"

#include "nDead.h"
#include "nCPHelp.h"
#include "nShipl.h"
#include "nPlanetl.h"
#include "nUserl.h"
#include "nHistl.h"
#include "nTeaml.h"
#include "nOptions.h"

#include <assert.h>

#include "glmisc.h"
#include "glfont.h"
#include "render.h"
#include "anim.h"
#include "GL.h"
#include "nCP.h"

#include "hud.h"

#include "cqsound.h"

/* node specific states */
#define S_NONE         0
#define S_COURSE       1        /* setting course */
#define S_DOINFO       2
#define S_TARGET       3        /* targeting info for torp/phase */
#define S_CLOAK        4
#define S_ALLOC        5
#define S_DISTRESS     6
#define S_REFIT        7
#define S_REFITING     8
#define S_COUP         9
#define S_TOW          10
#define S_MSGTO        11       /* getting msg target */
#define S_MSG          12       /* getting msg  */
#define S_DOAUTOPILOT  13
#define S_AUTOPILOT    14
#define S_REVIEW       15       /* review messages */
#define S_DEAD         16       /* the abyss */
#define S_DESTRUCT     17
#define S_DESTRUCTING  18
#define S_BOMB         19
#define S_BOMBING      20
#define S_BEAM         21
#define S_BEAMDIR      22
#define S_BEAMNUM      23
#define S_BEAMING      24
#define S_WAR          25
#define S_WARRING      26
#define S_PSEUDO       27       /* name change */
#define S_GHOST        28       /* died, but still watching */
#define S_GHOSTING     29       /* died, in the dead node */

static int state;

#define T_PHASER       0        /* S_TARGET */
#define T_BURST        1
#define T_TORP         2
static int desttarg;

/* timer vars */
static int entertime;

/* glut timer vars */
static int rftime;              /* last recording frame */

/* refit vars */
static int refitst = 0;         /* currently selected shiptype */

/* msg vars */
static msgTo_t msgto = MSG_TO_NOONE;
static uint16_t msgtoDetail = 0;

/* review msg vars */
static int lstmsg;              /* saved last msg */
static int lastone, msg;        /* saved for scrolling */

/* beaming vars */
static int dirup, upmax, downmax, beamax;

/* war vars */
static int twar[NUMPLAYERTEAMS];

/* the current prompt */
static prm_t prm;
static int prompting = FALSE;

/* save lastblast and always use a local copy for both torp and phasering */
static real lastblast;
static real lastphase;

/* misc buffers */
static char cbuf[BUFFER_SIZE_1024];
static char pbuf[BUFFER_SIZE_1024];

static char *abt = "...aborted...";

static int dostats = FALSE;     /* whether to display rendering stats */

extern hudData_t hudData;

/* Ping status */
static uint32_t pingStart = 0;
static int pingPending = FALSE;

/* bombing effect */
static int bombingfx;
static cqsHandle bombingHandle;    /* so we can stop it */

/* ack alert klaxon with <ESC> - Cataboligne */
extern cqsHandle alertHandle;

/* storage for the beam up/down sound handles */
static cqsHandle beamHandle = CQS_INVHANDLE;

/* current SR and LR magnification factors. (-5-5) */
int ncpLRMagFactor = 0;
int ncpSRMagFactor = 0;

extern void setWarp(real warp); /* FIXME - GL.c */


/* common output */
#define cp_putmsg(str, lin)  hudSetPrompt(lin, NULL, NoColor, str, NoColor)

#if 0                           /* for debugging */
void cp_putmsg(char *str, int lin)
{
    hudSetPrompt(lin, NULL, NoColor, str, NoColor);
    utLog("PUTMSG: %s", str);
}
#endif

static int nCPDisplay(dspConfig_t *);
static int nCPIdle(void);
static int nCPInput(int ch);
static int nCPMInput(mouseData_t *mdata);

static scrNode_t nCPNode = {
    nCPDisplay,                   /* display */
    nCPIdle,                      /* idle */
    nCPInput,                     /* input */
    nCPMInput,                    /* minput */
    NULL                          /* animQue */
};

static animQue_t animQue;

/* team torp anim states (exported) */
animStateRec_t ncpTorpAnims[NUMPLAYERTEAMS];

/* convert a KP key into an angle */
static int _KPAngle(int ch, real *angle)
{
    int rv;

    switch (ch)
    {
    case CQ_KEY_HOME:                /* KP upper left */
        *angle = 135.0;
        rv = TRUE;
        break;
    case CQ_KEY_PAGE_UP:                /* KP upper right */
        *angle = 45.0;
        rv = TRUE;
        break;
    case CQ_KEY_END:                /* KP lower left */
        *angle = 225.0;
        rv = TRUE;
        break;
    case CQ_KEY_PAGE_DOWN:                /* KP lower right */
        *angle = 315.0;
        rv = TRUE;
        break;
    case CQ_KEY_UP:                /* up arrow */
        *angle = 90.0;
        rv = TRUE;
        break;
    case CQ_KEY_DOWN:              /* down arrow */
        *angle = 270.0;
        rv = TRUE;
        break;
    case CQ_KEY_LEFT:              /* left arrow */
        *angle = 180.0;
        rv = TRUE;
        break;
    case CQ_KEY_RIGHT:             /* right arrow */
        *angle = 0.0;
        rv = TRUE;
        break;
    default:
        rv = FALSE;
        break;
    }

    return(rv);
}



static void _infoship( int snum, int scanner )
{
    int i, status;
    char junk[MSGMAXLINE];
    real x, y, dis, kills, appx, appy;
    int godlike, canscan;
    static char tmpstr[BUFFER_SIZE_256];
    /* ETA related vars */
    real pwarp, diffdis, close_rate;
    time_t difftime, curtime;
    static time_t oldtime = 0;
    static real avgclose_rate, olddis = 0.0, oldclose_rate = 0.0;
    static int oldsnum = 0;

    godlike = ( scanner < 0 || scanner >= MAXSHIPS );

    hudClearPrompt(MSG_LIN1);
    if ( snum < 0 || snum >= MAXSHIPS )
    {
        cp_putmsg( "No such ship.", MSG_LIN1 );
        return;
    }
    status = Ships[snum].status;
    if ( ! godlike && status != SS_LIVE )
    {
        cp_putmsg( "Not found.", MSG_LIN1 );
        return;
    }

    cbuf[0] = Context.lasttarg[0] = 0;
    utAppendShip(cbuf , snum) ;
    strcpy(Context.lasttarg, cbuf); /* save for hudInfo */
    hudSetInfoTarget(snum, true);

    if ( snum == scanner )
    {
        /* Silly Captain... */
        strcat(cbuf, ": That's us, silly!");
        cp_putmsg( cbuf, MSG_LIN1 );
        return;
    }
    /* Scan another ship. */
    if ( godlike )
    {
        x = 0.0;
        y = 0.0;
    }
    else
    {
        x = Ships[scanner].x;
        y = Ships[scanner].y;
    }
    if ( SCLOAKED(snum) )
    {
        if (godlike)
	{
            appx = rndnor(Ships[snum].x, CLOAK_SMEAR_DIST);
            appy = rndnor(Ships[snum].y, CLOAK_SMEAR_DIST);
	}
        else			/* client */
	{			/* for clients, these have already been
				   smeared */
            appx = Ships[snum].x;
            appy = Ships[snum].y;
	}
    }
    else
    {
        appx = Ships[snum].x;
        appy = Ships[snum].y;
    }
    dis = dist( x, y, appx, appy );
    if ( godlike )
        canscan = TRUE;
    else
    {

        /* Decide if we can do an acurate scan. */
        canscan = ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) ||
                    ( (Ships[snum].scanned[ Ships[scanner].team] > 0) && ! selfwar(scanner) ) );
    }

    strcat(cbuf , ": ");
    if ( Ships[snum].alias[0] != 0 )
    {
        strcat(cbuf , Ships[snum].alias);
        strcat(cbuf, ", ");
    }
    kills = (Ships[snum].kills + Ships[snum].strkills);
    if ( kills == 0.0 )
        strcat(cbuf , "no");
    else
    {
        sprintf( junk, "%.1f", kills );
        strcat(cbuf , junk);
    }
    strcat(cbuf , " kill");
    if ( kills != 1.0 )
        utAppendChar(cbuf , 's');
    if ( SCLOAKED(snum) && ( godlike || SSCANDIST(snum) ) )
        strcat(cbuf, " (CLOAKED) ");
    else
        strcat(cbuf, ", ");

    strcat(cbuf, "a ");
    strcat(cbuf, ShipTypes[Ships[snum].shiptype].name);
    strcat(cbuf, ", ");

    if ( godlike )
    {
        utAppendShipStatus(cbuf , status) ;
        utAppendChar(cbuf , '.');
    }
    else
    {
        if ( Ships[snum].war[Ships[scanner].team] )
            strcat(cbuf , "at WAR.");
        else
            strcat(cbuf , "at peace.");
    }

    cp_putmsg( cbuf, MSG_LIN1 );

    if ( ! SCLOAKED(snum) || Ships[snum].warp != 0.0 )
    {
        Context.lasttdist = round( dis ); /* save these puppies for hud info */
        Context.lasttang = round( utAngle( x, y, appx, appy ) );

        sprintf( cbuf, "Range %d, direction %d",
                 Context.lasttdist, Context.lasttang );

        if (UserConf.DoETAStats)
	{
            if (Ships[scanner].warp > 0.0 || Ships[snum].warp > 0.0)
	    {
                curtime = time(0);

                if (snum == oldsnum)
		{		/* then we can get better eta
				   by calculating closure rate and
				   extrapolate from there the apparent warp
				   giving a better estimate. */
                    difftime = curtime - oldtime;

                    /* we still need to compute diffdis  */
                    diffdis = olddis - dis;
                    olddis = dis;

                    if (difftime <= 0)
		    {		/* not enough time passed for a guess
				   use last closerate, and don't set
				   oldtime so it will eventually work */
                        close_rate = oldclose_rate;
		    }
                    else
		    {		/* we can make an estimate of closure rate in
				   MM's per second */
                        oldtime = curtime;

                        close_rate = diffdis / (real) difftime;
		    }

                    /* give a 'smoother' est. by avg'ing with
                       last close rate.*/
                    avgclose_rate = (close_rate + oldclose_rate) / 2.0;
                    oldclose_rate = close_rate;

#ifdef DEBUG_ETA
                    utLog("_infoship: close_rate(%.1f) = diffdis(%.1f) / difftime(%d), avgclose_rate = %.1f",
                          close_rate,
                          diffdis,
                          difftime,
                          avgclose_rate);
#endif

                    if (avgclose_rate <= 0.0)
		    {		/* dist is increasing or no change,
				   - can't ever catchup = ETA never */
                        sprintf(tmpstr, ", ETA %s",
                                clbETAStr(0.0, dis));
                        strcat(cbuf, tmpstr);
		    }
                    else
		    {		/* we are indeed closing... */

				/* calc psuedo-warp */
                        /* pwarp = dis / (avgclose_rate (in MM/sec) /
                           MM_PER_SEC_PER_WARP(18)) */
                        pwarp = (avgclose_rate / (real) MM_PER_SEC_PER_WARP);

#ifdef DEBUG_ETA
                        utLog("_infoship:\tdis(%.1f) pwarp(%.1f) = (close_rate(%.1f) / MM_PER_SEC_PER_WARP(%.1f)", dis, pwarp, close_rate, MM_PER_SEC_PER_WARP);
#endif

                        sprintf(tmpstr, ", ETA %s",
                                clbETAStr(pwarp, dis));
                        strcat(cbuf, tmpstr);
		    }
		}
                else
		{		/* scanning a new ship - assume ships
				   heading directly at each other */

				/* init old* vars */
                    oldtime = curtime;
                    oldsnum = snum;
                    olddis = dis;

                    pwarp =
                        (((Ships[scanner].warp > 0.0) ?
                          Ships[scanner].warp :
                          0.0) +
                         ((Ships[snum].warp > 0.0) ?
                          Ships[snum].warp
                          : 0.0));

                    sprintf(tmpstr, ", ETA %s",
                            clbETAStr(pwarp, dis));
                    strcat(cbuf, tmpstr);
		}
	    }
	} /* if do ETA stats */
    }
    else				/* else cloaked and at w0 */
    {
        Context.lasttdist = Context.lasttang = 0;
        cbuf[0] = 0;
    }

    if ( canscan )
    {
        if ( cbuf[0] != 0 )
            strcat(cbuf,  ", ");
        strcat(cbuf , "shields ");
        if ( SSHUP(snum) && ! SREPAIR(snum) )
            utAppendInt(cbuf, round( Ships[snum].shields ));
        else
            strcat(cbuf , "DOWN");
        i = round( Ships[snum].damage );
        if ( i > 0 )
	{
            if ( cbuf[0] != 0 )
                strcat(cbuf, ", ");
            sprintf( junk, "damage %d", i );
            strcat(cbuf , junk);
	}
        i = Ships[snum].armies;
        if ( i > 0 )
	{
            sprintf( junk, ", with %d arm", i );
            strcat(cbuf , junk);
            if ( i == 1 )
	    {
                utAppendChar(cbuf , 'y');
	    }
            else
                strcat(cbuf , "ies");
	}
    }
    if ( cbuf[0] != 0 )
    {
        cbuf[0] = (char)toupper( cbuf[0] );
        utAppendChar(cbuf , '.');
        cp_putmsg( cbuf, MSG_LIN2 );
    }

    return;

}



static void _infoplanet( char *str, int pnum, int snum )
{
    int i, j;
    int godlike, canscan;
    char buf[MSGMAXLINE*2], junk[MSGMAXLINE];
    real x, y;

    /* Check range of the passed planet number. */
    if ( pnum < 0 || pnum >= MAXPLANETS )
    {
        cp_putmsg( "No such planet.", MSG_LIN1 );
        hudClearPrompt(MSG_LIN2);
        utLog("_infoplanet: Called with invalid pnum (%d).",
              pnum );
        return;
    }

    /* GOD is too clever. */
    godlike = ( snum < 0 || snum >= MAXSHIPS );

    /* In some cases, report hostilities. */
    junk[0] = 0;
    if ( Planets[pnum].type == PLANET_CLASSM || Planets[pnum].type == PLANET_DEAD )
        if ( ! godlike )
            if ( Planets[pnum].scanned[Ships[snum].team] && clbSPWar( snum, pnum ) )
                strcat(junk, " (hostile)");

    if ( godlike )
    {
        x = 0.0;
        y = 0.0;
    }
    else
    {
        x = Ships[snum].x;
        y = Ships[snum].y;
    }

    Context.lasttdist = round(dist( x, y, Planets[pnum].x, Planets[pnum].y));
    Context.lasttang = round(utAngle( x, y, Planets[pnum].x, Planets[pnum].y ));

    if (UserConf.DoETAStats)
    {
        static char tmpstr[64];

        if (Ships[snum].warp > 0.0)
	{
            sprintf(tmpstr, ", ETA %s",
                    clbETAStr(Ships[snum].warp,
                              Context.lasttdist));
	}
        else
            tmpstr[0] = '\0';

        sprintf( buf, "%s%s, a %s%s, range %d, direction %d%s",
                 str,
                 Planets[pnum].name,
                 ConqInfo->ptname[Planets[pnum].type],
                 junk,
                 Context.lasttdist,
                 Context.lasttang,
                 tmpstr);


    }
    else
        sprintf( buf, "%s%s, a %s%s, range %d, direction %d",
                 str,
                 Planets[pnum].name,
                 ConqInfo->ptname[Planets[pnum].type],
                 junk,
                 Context.lasttdist,
                 Context.lasttang);

    /* save for the hudInfo */
    strncpy(Context.lasttarg, Planets[pnum].name, 3);
    Context.lasttarg[3] = 0;
    hudSetInfoTarget(pnum, false);

    if ( godlike )
        canscan = TRUE;
    else
        canscan = Planets[pnum].scanned[Ships[snum].team];

    junk[0] = 0;
    if ( Planets[pnum].type != PLANET_SUN && Planets[pnum].type != PLANET_MOON )
    {
        if ( ! canscan )
            strcpy(junk , "with unknown occupational forces") ;
        else
	{
            i = Planets[pnum].armies;
            if ( i == 0 )
	    {
                j = Planets[pnum].uninhabtime;
                if ( j > 0 )
                    sprintf( junk, "uninhabitable for %d more minutes", j );
                else
                    strcpy(junk , "with NO armies") ;
	    }
            else
	    {
                sprintf( junk, "with %d %s arm", i,
                         Teams[Planets[pnum].team].name );
                if ( i == 1 )
                    strcat(junk , "y");
                else
                    strcat(junk , "ies");
	    }
	}

        /* Now see if we can tell about coup time. */
        if ( godlike )
            canscan = FALSE;	/* GOD can use teaminfo instead */
        else
            canscan = ( pnum == Teams[Ships[snum].team].homeplanet &&
                        Teams[Ships[snum].team].coupinfo );
        if ( canscan )
	{
            j = Teams[Ships[snum].team].couptime;
            if ( j > 0 )
	    {
                if ( junk[0] != 0 )
                    strcat(junk, ", ");
                utAppendInt(junk , j) ;
                strcat(junk , " minutes until coup time");
	    }
	}
    }

    if ( junk[0] == 0 )
    {
        utAppendChar(buf , '.');
    }
    else
    {
        utAppendChar(buf, ',');
        utAppendChar(junk, '.');
    }

    /* Now output the info. Break the stuff in buf across two lines */
    /*  (if necessary) and force the stuff in junk (the number of */
    /*  armies for planets) to be all on the second line. */
    i = strlen( buf );				/* strlen of first part */
    j = 69;					/* desired maximum length */
    if ( i <= j )
    {
        /* The first part is small enough. */
        cp_putmsg( buf, MSG_LIN1 );
        if ( junk[0] != 0 )
            cp_putmsg(junk, MSG_LIN2);
        else
            hudClearPrompt(MSG_LIN2);
    }
    else
    {
        /* Break it into two lines. */
        i = j + 1;
        while ( buf[i] != ' ' && i > 1 )
            i = i - 1;
        utAppendChar(buf , ' ');
        strcat(buf , junk);
        buf[i] = 0;				/* terminate at blank */
        cp_putmsg( buf, MSG_LIN1 );
        cp_putmsg( &buf[i+1], MSG_LIN2 );
    }

    return;

}


static void _dowarp( int snum, real warp )
{
    hudClearPrompt(MSG_LIN2);

    /* Handle ship limitations. */

    warp = min( warp, ShipTypes[Ships[snum].shiptype].warplim );
    if (!sendCommand(CPCMD_SETWARP, (uint16_t)warp))
        return;

    sprintf( cbuf, "Warp %d.", (int) warp );
    cp_putmsg( cbuf, MSG_LIN1 );

    /* we set it locally since the server won't send it to us */
    Ships[snum].dwarp = warp;

    return;

}

/* get a target */
static int _gettarget(char *buf, real cdefault, real *dir, char ch)
{
    int i, j;

    if ( ch == TERM_ABORT )
        return ( FALSE );

    utDeleteBlanks( buf );
    if ( buf[0] == 0 )
    {
        /* Default. */
        *dir = cdefault;
        return ( TRUE );
    }
    if (utIsDigits(buf))
    {
        i = 0;
        if ( ! utSafeCToI( &j, buf, i ) )
            return ( FALSE );
        *dir = utMod360( (real) j );
        return ( TRUE );
    }
    if ( utArrowsToDir( buf, dir ) )
        return ( TRUE );

    return ( FALSE );

}

static void _dophase( real dir )
{
    /* we used to do some pre-checks here, but depending on lag,
       these checks can be racy.  Now we will send the command regardless
       of what things look like locally, and let the server deal with it. */

    /*  Cataboligne - sound code 10.16.6 */
    if ( Ships[Context.snum].pfuse == 0 &&
         clbUseFuel( Context.snum, PHASER_FUEL, TRUE, FALSE ) )
        cqsEffectPlay(cqsTeamEffects[Ships[Context.snum].team].phaser, NULL,
                      0, 0, 0);

    cp_putmsg( "Firing phasers...", MSG_LIN2 );

    sendCommand(CPCMD_FIREPHASER, (uint16_t)(dir * 100.0));

    return;

}

static void _dotorp(real dir, int num)
{
    /* we used to do some pre-checks here, but depending on lag,
       these checks can be racy.  Now we will send the command regardless
       of what things look like locally, and let the server deal with it. */

    if ( clbCheckLaunch( Context.snum, num ) )
    {
        /* Cat - torp fired sound */

        if (num > 1)
            cqsEffectPlay(cqsTeamEffects[Ships[Context.snum].team].torp3, NULL,
                          0, 0, 0);
        else
            cqsEffectPlay(cqsTeamEffects[Ships[Context.snum].team].torp, NULL,
                          0, 0, 0);
    }

    sendFireTorps(num, dir);
    hudClearPrompt(MSG_LIN1);

    return;
}


/*  doinfo - do an info command */
/*  SYNOPSIS */
/*    int snum */
/*    doinfo( snum ) */
static void _doinfo( char *buf, char ch )
{
    int snum = Context.snum;
    int j, what, sorpnum, xsorpnum, count, token;
    int extra;

    if ( ch == TERM_ABORT )
    {
        hudClearPrompt(MSG_LIN1);
        return;
    }
    extra = ( ch == TERM_EXTRA );

    /* Default to what we did last time. */
    utDeleteBlanks( buf );
    if ( buf[0] == 0 )
    {
        strcpy(buf , Context.lastinfostr) ;
        if ( buf[0] == 0 )
	{
            hudClearPrompt(MSG_LIN1);
            return;
	}
    }
    else
        strcpy(Context.lastinfostr , buf) ;

    if ( utIsSpecial( buf, &what, &token, &count ) )
    {
        if ( ! clbFindSpecial( snum, token, count, &sorpnum, &xsorpnum ) )
            what = NEAR_NONE;
        else if ( extra )
	{
            if ( xsorpnum == -1 )
                what = NEAR_NONE;
            else
                sorpnum = xsorpnum;
	}

        if ( what == NEAR_SHIP )
            _infoship( sorpnum, snum );
        else if ( what == NEAR_PLANET )
            _infoplanet( "", sorpnum, snum );
        else
            cp_putmsg( "Not found.", MSG_LIN2 );
    }
    else if ( buf[0] == 's' && utIsDigits(&buf[1]) )
    {
        utSafeCToI( &j, buf, 1 );		/* ignore status */
        _infoship( j, snum );
    }
    else if (utIsDigits(buf))
    {
        utSafeCToI( &j, buf, 0 );		/* ignore status */
        _infoship( j, snum );
    }
    else if ( clbPlanetMatch( buf, &j, FALSE ) )
        _infoplanet( "", j, snum );
    else
    {
        cp_putmsg( "I don't understand.", MSG_LIN2 );
        return;
    }

/* Cataboligne - Spocks viewer sound */
    if (rnd() < 0.3)
        cqsEffectPlay(cqsTeamEffects[Ships[Context.snum].team].info, NULL,
                      0, 0, 0);
}


/* 'read' a msg */
static void rmesg(int snum, int msgnum, int lin)
{
    char buf[MSGMAXLINE];

    clbFmtMsg(Msgs[msgnum].from, Msgs[msgnum].fromDetail,
              Msgs[msgnum].to, Msgs[msgnum].toDetail, buf);
    strcat(buf , ": ");
    strcat(buf , Msgs[msgnum].msgbuf);

    hudSetPrompt(lin, NULL, NoColor, buf, CyanColor);

    return;
}

static void _domydet(void)
{
    hudClearPrompt(MSG_LIN2);

    sendCommand(CPCMD_DETSELF, 0);

    cp_putmsg( "Detonating...", MSG_LIN1 );

    return;

}

static void _doshields( int snum, int up )
{

    if (!sendCommand(CPCMD_SETSHIELDS, (uint16_t)up))
        return;

    if ( up )
    {
        SFCLR(snum, SHIP_F_REPAIR);
        cp_putmsg( "Shields raised.", MSG_LIN1 );
    }
    else
        cp_putmsg( "Shields lowered.", MSG_LIN1 );

    hudClearPrompt(MSG_LIN2);

    return;

}

static void _doorbit( int snum )
{
    int pnum;

    if ( ( Ships[snum].warp == ORBIT_CW ) || ( Ships[snum].warp == ORBIT_CCW ) )
        _infoplanet( "But we are already orbiting ",
                     Ships[snum].lockDetail, snum );
    else if ( ! clbFindOrbit( snum, &pnum ) )
    {
        sprintf( cbuf, "We are not close enough to orbit, %s.",
                 Ships[snum].alias );
        cp_putmsg( cbuf, MSG_LIN1 );
        hudClearPrompt(MSG_LIN2);
    }
    else if ( Ships[snum].warp > MAX_ORBIT_WARP )
    {
        sprintf( cbuf, "We are going too fast to orbit, %s.",
                 Ships[snum].alias );
        cp_putmsg( cbuf, MSG_LIN1 );
        sprintf( cbuf, "Maximum orbital insertion velocity is warp %.1f.",
                 oneplace(MAX_ORBIT_WARP) );
        cp_putmsg( cbuf, MSG_LIN2 );
    }
    else
    {
        sendCommand(CPCMD_ORBIT, 0);
        _infoplanet( "Coming into orbit around ", pnum, snum );
    }

    return;

}

static void _doalloc(char *buf, char ch)
{
    int snum = Context.snum;
    int i, alloc;
    int dwalloc = 0;

    switch (ch)
    {
    case TERM_EXTRA:
        dwalloc = Ships[snum].engalloc;
        break;

    case TERM_NORMAL:
        i = 0;
        utSafeCToI( &alloc, buf, i );			/* ignore status */
        if ( alloc != 0 )
	{
            if ( alloc < 30 )
                alloc = 30;
            else if ( alloc > 70 )
                alloc = 70;
            dwalloc = alloc;
	}
        else
	{
            hudClearPrompt(MSG_LIN1);
            return;
	}

        break;

    default:
        return;
    }

    sendCommand(CPCMD_ALLOC, (uint16_t)dwalloc);

    hudClearPrompt(MSG_LIN1);

    return;

}

static void _dodet( void )
{
    int snum = Context.snum;

    hudClearPrompt(MSG_LIN1);

    if ( Ships[snum].wfuse > 0 )
        cp_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
    else if ( clbUseFuel( snum, DETONATE_FUEL, TRUE, FALSE ) )
    {				/* we don't really use fuel here on the
				   client*/
        cp_putmsg( "detonating...", MSG_LIN1 );
        sendCommand(CPCMD_DETENEMY, 0);
    }
    else
        cp_putmsg( "Not enough fuel to fire detonators.", MSG_LIN1 );

    return;

}

static void _dodistress(char *buf, char ch)
{
    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);

    if (ch == TERM_EXTRA)
        sendCommand(CPCMD_DISTRESS, (uint16_t)UserConf.DistressToFriendly);

    hudClearPrompt(MSG_LIN1);

    return;

}

static int _chkrefit(void)
{
    int snum = Context.snum;
    static char *ntp="We must be orbiting a team owned planet to refit.";
    static char *nek="You must have at least one kill to refit.";
    static char *cararm="You cannot refit while carrying armies";

    hudClearPrompt(MSG_LIN2);

    /* Check for allowability. */
    if ( oneplace( Ships[snum].kills ) < MIN_REFIT_KILLS )
    {
        cp_putmsg( nek, MSG_LIN1 );
        return FALSE;
    }

    if (Ships[snum].lock == LOCK_PLANET
        && Ships[snum].lockDetail < MAXPLANETS)
    {
        int pnum = Ships[snum].lockDetail;

        if (Planets[pnum].team != Ships[snum].team || Ships[snum].warp >= 0.0)
        {
            cp_putmsg( ntp, MSG_LIN1 );
            return FALSE;
        }
    }

    if (Ships[snum].armies != 0)
    {
        cp_putmsg( cararm, MSG_LIN1 );
        return FALSE;
    }

    return TRUE;
}

static int _chkcoup(void)
{
    int snum = Context.snum;
    int i, pnum;
    char *nhp="We must be orbiting our home planet to attempt a coup.";

    hudClearPrompt(MSG_LIN2);

    /* some checks we will do locally, the rest will have to be handled by
       the server */
    /* Check for allowability. */
    if ( oneplace( Ships[snum].kills ) < MIN_COUP_KILLS )
    {
        cp_putmsg(
            "Fleet orders require three kills before a coup can be attempted.",
            MSG_LIN1 );
        return FALSE;
    }
    for ( i = 0; i < MAXPLANETS; i++ )
        if ( Planets[i].team == Ships[snum].team && Planets[i].armies > 0 )
        {
            cp_putmsg( "We don't need to coup, we still have armies left!",
                       MSG_LIN1 );
            return FALSE;
        }
    if ( Ships[snum].warp >= 0.0 )
    {
        cp_putmsg( nhp, MSG_LIN1 );
        return FALSE;
    }
    // the assumption is that if warp < 0, we are in orbit and
    // therefore we are locked onto the planet we are orbiting
    pnum = (int)Ships[snum].lockDetail;
    if ( pnum != Teams[Ships[snum].team].homeplanet )
    {
        cp_putmsg( nhp, MSG_LIN1 );
        return FALSE;
    }
    if ( Planets[pnum].armies > MAX_COUP_ENEMY_ARMIES )
    {
        cp_putmsg( "The enemy is still too strong to attempt a coup.",
                   MSG_LIN1 );
        return FALSE;
    }
    i = Planets[pnum].uninhabtime;
    if ( i > 0 )
    {
        sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
                 i );
        cp_putmsg( cbuf, MSG_LIN1 );
        return FALSE;
    }

    return TRUE;
}

static int _chktow(void)
{
    int snum = Context.snum;

    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);

    if ( Ships[snum].towedby != 0 )
    {
        strcpy(cbuf , "But we are being towed by ") ;
        utAppendShip(cbuf , Ships[snum].towedby) ;
        utAppendChar(cbuf , '!');
        cp_putmsg( cbuf, MSG_LIN2 );
        return FALSE;
    }
    if ( Ships[snum].towing != 0 )
    {
        strcpy(cbuf , "But we're already towing ") ;
        utAppendShip(cbuf , Ships[snum].towing) ;
        utAppendChar(cbuf , '.');
        cp_putmsg( cbuf, MSG_LIN2 );
        return FALSE;
    }

    return TRUE;
}

static void _dotow(char *buf, int ch)
{
    int i, other;
    if (ch == TERM_ABORT)
        return;
    i = 0;
    utSafeCToI( &other, cbuf, i );		/* ignore status */

    sendCommand(CPCMD_TOW, (uint16_t)other);

    return;
}

/* modifies state */
static void _domsgto(char *buf, int ch, int terse)
{
    int i, j;
    static char tbuf[MESSAGE_SIZE];
    char *nf="Not found.";
    char *huh="I don't understand.";
    int editing;
    static msgTo_t to = MSG_TO_NOONE;
    static uint16_t toDetail = 0;

    /* First, find out who we're sending to. */
    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);

    if ( ch == TERM_ABORT )
    {
        hudClearPrompt(MSG_LIN1);
        state = S_NONE;
        prompting = FALSE;

        return;
    }

    strcpy(tbuf, buf) ;

    /* TAB or ENTER means use the target from the last message. */
    editing = ( (ch == TERM_EXTRA || ch == TERM_NORMAL) && buf[0] == 0 );
    if ( editing )
    {
        /* Make up a default string using the last target. */
        if ( to == MSG_TO_SHIP && toDetail < MAXSHIPS )
            sprintf( tbuf, "%d", to );
        else if ( to == MSG_TO_TEAM && toDetail < NUMPLAYERTEAMS )
            strcpy(tbuf , Teams[toDetail].name) ;
        else switch ( to )
             {
             case MSG_TO_ALL:
                 strcpy(tbuf , "All") ;
                 break;
             case MSG_TO_GOD:
                 strcpy(tbuf , "GOD") ;
                 break;
             case MSG_TO_IMPLEMENTORS:
                 strcpy(tbuf , "Implementors") ;
                 break;
             case MSG_TO_FRIENDLY:
                 strcpy(tbuf , "Friend") ;
                 break;
             default:
                 tbuf[0] = 0;
                 break;
             }
    }

    /* Got a target, parse it. */
    utDeleteBlanks( tbuf );
    utToUpperCase( tbuf );
    if (utIsDigits(tbuf))
    {
        /* All digits means a ship number. */
        i = 0;
        utSafeCToI( &j, tbuf, i );		/* ignore status */
        if ( j < 0 || j >= MAXSHIPS )
	{
            cp_putmsg( "No such ship.", MSG_LIN2 );
            hudClearPrompt(MSG_LIN1);
            state = S_NONE;
            prompting = FALSE;
            return;
	}
        if ( Ships[j].status != SS_LIVE )
	{
            cp_putmsg( nf, MSG_LIN2 );
            hudClearPrompt(MSG_LIN1);
            state = S_NONE;
            prompting = FALSE;
            return;
	}
        to = MSG_TO_SHIP;
        toDetail = j;
    }
    else
    {
        switch ( tbuf[0] )
         {
         case 'A':
         case 'a':
             to = MSG_TO_ALL;
             toDetail = 0;
             break;
         case 'G':
         case 'g':
             to = MSG_TO_GOD;
             toDetail = 0;
             break;
         case 'I':
         case 'i':
             to = MSG_TO_IMPLEMENTORS;
             toDetail = 0;
             break;
         default:
             /* check for 'Friend' */
             if (tbuf[0] == 'F' && tbuf[1] == 'R')
             {			/* to friendlies */
                 to = MSG_TO_FRIENDLY;
                 toDetail = 0;
             }
             else
             {
                 /* Check for a team character. */
                 for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                     if ( tbuf[0] == Teams[i].teamchar
                          || tbuf[0] == (char)tolower(Teams[i].teamchar) )
                         break;

                 if ( i >= NUMPLAYERTEAMS )
                 {
                     cp_putmsg( huh, MSG_LIN2 );
                     hudClearPrompt(MSG_LIN1);
                     state = S_NONE;
                     prompting = FALSE;
                     return;
                 }
                 to = MSG_TO_TEAM;
                 toDetail = i;
             };
             break;
         }
    }

    /* Now, construct a header for the selected target. */
    strcpy(tbuf , "Message to ") ;
    if ( to == MSG_TO_SHIP && toDetail < MAXSHIPS )
    {
        if ( Ships[to].status != SS_LIVE )
	{
            cp_putmsg( nf, MSG_LIN2 );
            hudClearPrompt(MSG_LIN1);
            state = S_NONE;
            prompting = FALSE;
            return;
	}
        utAppendShip(tbuf, (int)toDetail) ;
        utAppendChar(tbuf, ':');
    }
    else if ( to == MSG_TO_TEAM && toDetail < NUMPLAYERTEAMS )
    {
        strcat(tbuf , Teams[toDetail].name);
        strcat(tbuf , "s:");
    }
    else
    {
        switch ( to )
         {
         case MSG_TO_ALL:
             strcat(tbuf , "everyone:");
             break;
         case MSG_TO_GOD:
             strcat(tbuf , "GOD:");
             break;
         case MSG_TO_IMPLEMENTORS:
             strcat(tbuf , "The Implementors:");
             break;
         case MSG_TO_FRIENDLY:
             strcat(tbuf , "Friend:");
             break;
         default:
             cp_putmsg( huh, MSG_LIN2 );
             return;
             break;
         }
    }

    if (!terse)
        strcat(tbuf, " ([ESC] to abort)");

    cp_putmsg( tbuf, MSG_LIN1 );
    hudClearPrompt(MSG_LIN2);

    msgto = to;                   /* set global */
    msgtoDetail = toDetail;

    state = S_MSG;
    prm.preinit = FALSE;
    prm.buf = cbuf;
    prm.buflen = MESSAGE_SIZE;
    strcpy(pbuf, "> ");
    prm.pbuf = pbuf;
    prm.terms = TERMS;
    prm.index = MSG_LIN2;
    prm.buf[0] = 0;
    hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
    prompting = TRUE;

    return;
}

/* modifies state */
static void _domsg(char *msg, int ch, int irv)
{
    static char mbuf[MSGMAXLINE];
    char *cptr;
    int len = strlen(msg);

    /* if maxlen reached */
    if (irv == PRM_MAXLEN)
    {                           /* then we need to send what we have
                                   and continue */
        mbuf[0] = 0;
        cptr = &msg[len - 1];

        while ((cptr > msg) && *cptr != ' ')
            cptr--;

        if (cptr > msg)
        {
            *cptr = 0;
            sprintf(mbuf, "%s -", msg);

            cptr++;
            strcpy(prm.pbuf, "- ");
            sprintf(prm.buf, "%s%c", cptr, ch);
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            sendMessage(msgto, msgtoDetail, mbuf);
        }
        else
        {

            strcpy(mbuf, msg);

            strcpy(prm.pbuf, "- ");
            prm.buf[0] = ch;
            prm.buf[1] = 0;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);
            sendMessage(msgto, msgtoDetail, mbuf);
        }

        return;
    }
    else
    {                           /* ready or abort */
        if (ch != TERM_ABORT)
            sendMessage(msgto, msgtoDetail, msg); /* just send it */

        state = S_NONE;
        prompting = FALSE;
        hudClearPrompt(MSG_LIN1);
        hudClearPrompt(MSG_LIN2);
    }

    return;
}

static int _xlateFKey(int ch)
{
    int fkey = -1;

    switch (ch & CQ_FKEY_MASK)
    {
    case CQ_KEY_F1:
        fkey = 1;
        break;
    case CQ_KEY_F2:
        fkey = 2;
        break;
    case CQ_KEY_F3:
        fkey = 3;
        break;
    case CQ_KEY_F4:
        fkey = 4;
        break;
    case CQ_KEY_F5:
        fkey = 5;
        break;
    case CQ_KEY_F6:
        fkey = 6;
        break;
    case CQ_KEY_F7:
        fkey = 7;
        break;
    case CQ_KEY_F8:
        fkey = 8;
        break;
    case CQ_KEY_F9:
        fkey = 9;
        break;
    case CQ_KEY_F10:
        fkey = 10;
        break;
    case CQ_KEY_F11:
        fkey = 11;
        break;
    case CQ_KEY_F12:
        fkey = 12;
        break;
    default:
        fkey = -1;
    }

    if (fkey == -1)
        return fkey;

    if (CQ_MODIFIER(ch) & CQ_KEY_MOD_SHIFT)
        fkey += 12;
    else if (CQ_MODIFIER(ch) & CQ_KEY_MOD_CTRL)
        fkey += 24;
    else if (CQ_MODIFIER(ch) & CQ_KEY_MOD_ALT)
        fkey += 36;

    fkey--;

    return fkey;
}


/*  _docourse - set course */
/*  SYNOPSIS */
/*    int snum */
/*    _docourse( snum ) */
static void _docourse( char *buf, char ch)
{
    int i, j, what, sorpnum, xsorpnum, token, count;
    real dir, appx, appy;
    int snum = Context.snum;
    courseLock_t newlock = LOCK_NONE;
    uint16_t newlockDetail = 0;

    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);

    utDeleteBlanks( buf );
    if ( ch == TERM_ABORT || buf[0] == 0 )
    {
        hudClearPrompt(MSG_LIN1);
        return;
    }

    what = NEAR_ERROR;
    if (utIsDigits(buf))
    {
        /* Raw angle. */
        hudClearPrompt( MSG_LIN1 );
        i = 0;
        if ( utSafeCToI( &j, buf, i ) )
	{
            what = NEAR_DIRECTION;
            dir = (real)utMod360( (real)( j ) );
	}
    }
    else if ( buf[0] == 's' && utIsDigits(&buf[1]) )
    {
        /* Ship. */

        i = 1;
        if ( utSafeCToI( &sorpnum, buf, i ) )
            what = NEAR_SHIP;
    }
    else if ( utArrowsToDir( buf, &dir ) )
        what = NEAR_DIRECTION;
    else if ( utIsSpecial( buf, &i, &token, &count ) )
    {
        if ( clbFindSpecial( snum, token, count, &sorpnum, &xsorpnum ) )
            what = i;
    }
    else if ( clbPlanetMatch( buf, &sorpnum, FALSE ) )
        what = NEAR_PLANET;

    switch ( what )
    {
    case NEAR_SHIP:
        if ( sorpnum < 0 || sorpnum >= MAXSHIPS )
	{
            cp_putmsg( "No such ship.", MSG_LIN2 );
            return;
	}
        if ( sorpnum == snum )
	{
            hudClearPrompt(MSG_LIN1);
            return;
	}
        if ( Ships[sorpnum].status != SS_LIVE )
	{
            cp_putmsg( "Not found.", MSG_LIN2 );
            return;
	}

        if ( SCLOAKED(sorpnum) )
	{
            if ( Ships[sorpnum].warp <= 0.0 )
	    {
                cp_putmsg( "Sensors are unable to lock on.", MSG_LIN2 );
                return;
	    }
	}

        appx = Ships[sorpnum].x;
        appy = Ships[sorpnum].y;

        dir = utAngle( Ships[snum].x, Ships[snum].y, appx, appy );

        /* Give info if he used TAB. */
        if ( ch == TERM_EXTRA )
            _infoship( sorpnum, snum );
        else
            hudClearPrompt(MSG_LIN1);
        break;
    case NEAR_PLANET:
        dir = utAngle( Ships[snum].x, Ships[snum].y, Planets[sorpnum].x, Planets[sorpnum].y );
        if ( ch == TERM_EXTRA )
	{
            newlock = LOCK_PLANET;
            newlockDetail = sorpnum;
            _infoplanet( "Now locked on to ", sorpnum, snum );
	}
        else
            _infoplanet( "Setting course for ", sorpnum, snum );
        break;
    case NEAR_DIRECTION:
        hudClearPrompt(MSG_LIN1);
        break;
    case NEAR_NONE:
        cp_putmsg( "Not found.", MSG_LIN2 );
        return;
        break;
    default:
        /* This includes NEAR_ERROR. */
        cp_putmsg( "I don't understand.", MSG_LIN2 );
        return;
        break;
    }

    sendSetCourse(cInfo.sock, newlock, newlockDetail, dir);

    return;

}

/* will decloak if cloaked */
static int _chkcloak(void)
{
    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);

    if ( SCLOAKED(Context.snum) )
    {
        sendCommand(CPCMD_CLOAK, 0);
        cp_putmsg( "Cloaking device disengaged.", MSG_LIN1 );
        return FALSE;
    }

    return TRUE;
}

static void _docloak( char *buf, char ch)
{
    hudClearPrompt(MSG_LIN1);

    if (ch == TERM_EXTRA)
        sendCommand(CPCMD_CLOAK, 0);

    hudClearPrompt(MSG_LIN1);

    return;

}

static int _review(void)
{
    int snum = Context.snum;

    if ( clbCanRead( snum, msg ))
    {
        rmesg( snum, msg, MSG_LIN1 );
    }
    else
    {
        msg = utModPlusOne( msg - 1, MAXMESSAGES );
        if (msg == lastone)
        {
            state = S_NONE;
            return FALSE;
        }
    }

    return TRUE;

}


static void _doreview(void)
{
    int snum = Context.snum;
    int i;

    lstmsg = Ships[snum].lastmsg;	/* don't want lstmsg changing while
                                           reading old ones. */

    lastone = utModPlusOne( ConqInfo->lastmsg+1, MAXMESSAGES );
    if ( snum >= 0 && snum < MAXSHIPS )
    {
        if ( Ships[snum].lastmsg == LMSG_NEEDINIT )
        {
            cp_putmsg( "There are no old messages.", MSG_LIN1 );
            return;               /* none to read */
        }
        i = Ships[snum].alastmsg;
        if ( i != LMSG_READALL )
            lastone = i;
    }

    msg = lstmsg;

    state = S_REVIEW;

    hudClearPrompt(MSG_LIN1);
    if (!_review())
    {
        cp_putmsg( "There are no old messages.", MSG_LIN1 );
        return;               /* none to read */
    }


    cp_putmsg("--- [SPACE] for more, arrows to scroll, any key to quit ---",
              MSG_LIN2);

    return;

}

static void _dobomb(void)
{
    int snum = Context.snum;
    int pnum;

    SFCLR(snum, SHIP_F_REPAIR);;

    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);

    /* Check for allowability. */
    if ( Ships[snum].warp >= 0.0 )
    {
        cp_putmsg( "We must be orbiting a planet to bombard it.", MSG_LIN1 );
        return;
    }
    pnum = Ships[snum].lockDetail;
    if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON ||
         Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
    {
        cp_putmsg( "There is no one there to bombard.", MSG_LIN1 );
        return;
    }
    if ( Planets[pnum].team == Ships[snum].team )
    {
        cp_putmsg( "We can't bomb our own armies!", MSG_LIN1 );
        return;
    }

    if ( Planets[pnum].team != TEAM_SELFRULED && Planets[pnum].team != TEAM_GOD )
        if ( ! Ships[snum].war[Planets[pnum].team] )
        {
            cp_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
            return;
        }

    /* set up the state, and proceed. */

    sprintf( pbuf, "Press [TAB] to bombard %s, %d armies:",
             Planets[pnum].name, Planets[pnum].armies );

    state = S_BOMB;
    prm.preinit = FALSE;
    prm.buf = cbuf;
    prm.buf[0] = 0;
    prm.buflen = MSGMAXLINE;
    prm.pbuf = pbuf;
    prm.terms = TERMS;
    prm.index = MSG_LIN1;
    hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
    prompting = TRUE;

    lastServerError = 0;          /* so the server can tell us to stop */

    return;
}

/* sets state */
static void _initbeam()
{
    int snum = Context.snum;
    int pnum, capacity, i;
    real rkills;
    char *lastfew="Fleet orders prohibit removing the last three armies.";

    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);

    /* all of these checks are performed server-side as well, but we also
       check here for the obvious problems so we can save time without
       bothering the server for something it will refuse anyway. */

    /* at least the basic checks could be split into a seperate func that
       could be used by both client and server */

    /* Check for allowability. */
    if ( Ships[snum].warp >= 0.0 )
    {
        cp_putmsg( "We must be orbiting a planet to use the transporter.",
                   MSG_LIN1 );
        return;
    }
    pnum = Ships[snum].lockDetail;
    if ( Ships[snum].armies > 0 )
    {
        if ( Planets[pnum].type == PLANET_SUN )
	{
            cp_putmsg( "Idiot!  Our armies will fry down there!", MSG_LIN1 );
            return;
	}
        else if ( Planets[pnum].type == PLANET_MOON )
	{
            cp_putmsg( "Fool!  Our armies will suffocate down there!",
                       MSG_LIN1 );
            return;
	}
        else if ( Planets[pnum].team == TEAM_GOD )
	{
            cp_putmsg(
                "GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
                MSG_LIN1 );
            return;
	}
    }

    i = Planets[pnum].uninhabtime;
    if ( i > 0 )
    {
        sprintf( cbuf, "This planet is uninhabitable for %d more minute",
                 i );
        if ( i != 1 )
            utAppendChar(cbuf , 's');
        utAppendChar(cbuf , '.');
        cp_putmsg( cbuf, MSG_LIN1 );
        return;
    }

    /* can take empty planets */
    if ( Planets[pnum].team != Ships[snum].team &&
         Planets[pnum].team != TEAM_SELFRULED &&
         Planets[pnum].team != TEAM_NOTEAM )
        if ( ! Ships[snum].war[Planets[pnum].team] && Planets[pnum].armies != 0)
        {
            cp_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
            return;
        }

    if ( Ships[snum].armies == 0 &&
         Planets[pnum].team == Ships[snum].team && Planets[pnum].armies <= MIN_BEAM_ARMIES )
    {
        cp_putmsg( lastfew, MSG_LIN1 );
        return;
    }

    rkills = Ships[snum].kills;

    if ( rkills < (real)1.0 )
    {
        cp_putmsg(
            "Fleet orders prohibit beaming armies until you have a kill.",
            MSG_LIN1 );
        return;
    }

    /* Figure out what can be beamed. */
    downmax = Ships[snum].armies;
    if ( clbSPWar(snum,pnum) ||
         Planets[pnum].team == TEAM_SELFRULED ||
         Planets[pnum].team == TEAM_NOTEAM ||
         Planets[pnum].team == TEAM_GOD ||
         Planets[pnum].armies == 0 )
    {
        upmax = 0;
    }
    else
    {
        capacity = min( (int)rkills * 2,
                        ShipTypes[Ships[snum].shiptype].armylim );
        upmax = min( Planets[pnum].armies - MIN_BEAM_ARMIES,
                     capacity - Ships[snum].armies );
    }

    /* If there are armies to beam but we're selfwar... */
    if ( upmax > 0 && selfwar(snum) && Ships[snum].team == Planets[pnum].team )
    {
        if ( downmax <= 0 )
	{
            strcpy(cbuf , "The arm") ;
            if ( upmax == 1 )
                strcat(cbuf , "y is");
            else
                strcat(cbuf , "ies are");
            strcat(cbuf , " reluctant to beam aboard a pirate vessel.");
            cp_putmsg( cbuf, MSG_LIN1 );
            return;
	}
        upmax = 0;
    }

    /* Figure out which direction to beam. */
    if ( upmax <= 0 && downmax <= 0 )
    {
        cp_putmsg( "There is no one to beam.", MSG_LIN1 );
        return;
    }

    if ( upmax <= 0 )
        dirup = FALSE;
    else if ( downmax <= 0 )
        dirup = TRUE;
    else
    {                           /* need to ask beam dir... */
        state = S_BEAMDIR;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = 10;
        prm.pbuf = "Beam [up or down] ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;

        return;
    }

    /* else, get the number to beam */

    if ( dirup )
        beamax = upmax;
    else
        beamax = downmax;

    /* Figure out how many armies should be beamed. */
    sprintf( pbuf, "Beam %s [1-%d] ", (dirup) ? "up" : "down", beamax );

    state = S_BEAMNUM;
    prm.preinit = FALSE;
    prm.buf = cbuf;
    prm.buflen = 10;
    prm.pbuf = pbuf;
    prm.terms = TERMS;
    prm.index = MSG_LIN1;
    prm.buf[0] = 0;
    hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
    prompting = TRUE;

    return;
}

static void _dobeam(char *buf, int ch)
{
    int num, i;

    if ( ch == TERM_ABORT )
    {
        state = S_NONE;
        prompting = FALSE;
        cp_putmsg( abt, MSG_LIN1 );
        return;
    }
    else if ( ch == TERM_EXTRA && buf[0] == 0 )
        num = beamax;
    else
    {
        utDeleteBlanks( buf );
        if (!utIsDigits(buf))
	{
            state = S_NONE;
            prompting = FALSE;
            cp_putmsg( abt, MSG_LIN1 );
            return;
	}
        i = 0;
        utSafeCToI( &num, buf, i );			/* ignore status */
        if ( num < 1 || num > beamax )
	{
            cp_putmsg( abt, MSG_LIN1 );
            state = S_NONE;
            prompting = FALSE;
            return;
	}
    }

    /* now we start the phun. */

    lastServerError = 0;

    /* detail is (armies & 0x00ff), 0x8000 set if beaming down */

    /* start the effects */
    if (dirup)
        cqsEffectPlay(cqsTeamEffects[Ships[Context.snum].team].beamu,
                      &beamHandle,
                      0, 0, 0);
    else
        cqsEffectPlay(cqsTeamEffects[Ships[Context.snum].team].beamd,
                      &beamHandle,
                      0, 0, 0);

    sendCommand(CPCMD_BEAM,
                (dirup) ? (uint16_t)(num & 0x00ff):
                (uint16_t)((num & 0x00ff) | 0x8000));

    state = S_BEAMING;
    prompting = FALSE;

    return;

}


/*  command - execute a user's command  ( COMMAND ) */
/*  SYNOPSIS */
/*    char ch */
/*    command( ch ) */
static void command( int ch )
{
    int i;
    real x;
    int snum = Context.snum;

    if (_KPAngle(ch, &x))         /* hit a keypad/arrow key */
    {				/* alter course Mr. Sulu. */
        hudClearPrompt(MSG_LIN1);
        hudClearPrompt(MSG_LIN2);

        sendSetCourse(cInfo.sock, LOCK_NONE, 0, x);
        return;
    }

    switch ( ch )
    {
    case '0':           /* - '9', '=' :set warp factor */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '=':
        if ( ch == '=' )
            x = 10.0;
        else
	{
            i = ch - '0';
            x = (real) (i);
	}
        _dowarp( Context.snum, x );
        break;
    case 'a':				/* autopilot */
        if ( Users[Ships[Context.snum].unum].ooptions[ OOPT_AUTOPILOT] )
	{
            state = S_DOAUTOPILOT;
            prm.preinit = FALSE;
            prm.buf = cbuf;
            prm.buflen = MSGMAXLINE;
            prm.pbuf = "Press [TAB] to engage autopilot: ";
            prm.terms = TERMS;
            prm.index = MSG_LIN1;
            prm.buf[0] = 0;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
            prompting = TRUE;
	}
        else
	{
            mglBeep(MGL_BEEP_ERR);
            cp_putmsg( "Type h for help.", MSG_LIN2 );
	}
        break;
    case 'A':				/* change allocation */
        state = S_ALLOC;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        prm.pbuf = "New weapons allocation: (30-70) ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;
        break;
    case 'b':				/* beam armies */
        _initbeam();
        break;
    case 'B':				/* bombard a planet */
        _dobomb();  /* will set state */
        break;
    case 'C':				/* cloak control */
        if (_chkcloak())
        {
            state = S_CLOAK;
            prm.preinit = FALSE;
            prm.buf = cbuf;
            prm.buflen = MSGMAXLINE;
            prm.pbuf = "Press [TAB] to engage cloaking device: ";
            prm.terms = TERMS;
            prm.index = MSG_LIN1;
            prm.buf[0] = 0;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
            prompting = TRUE;
        }
        break;
    case 'd':				/* detonate enemy torps */
    case '*':
        _dodet();
        break;
    case 'D':				/* detonate own torps */
        _domydet();
        break;
    case 'E':				/* emergency distress call */
        state = S_DISTRESS;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        prm.pbuf = "Press [TAB] to send an emergency distress call: ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);

        prompting = TRUE;
        break;
    case 'f':				/* phasers */
    {
        state = S_TARGET;
        desttarg = T_PHASER;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        prm.pbuf = "Fire phasers: ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;
    }
    break;
    case 'F':				/* phasers, same direction */
        _dophase(lastphase);
        break;
    case 'h':
        setONode(nCPHelpInit(FALSE));
        break;
    case 'H':
        setONode(nHistlInit(DSP_NODE_CP, FALSE));
        break;
    case 'i':				/* information */
        state = S_DOINFO;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        prm.pbuf = "Information on: ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);

        prompting = TRUE;
        break;
    case 'k':				/* set course */
        state = S_COURSE;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        prm.pbuf = "Come to course: ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;
        break;
    case 'K':				/* coup */
        if (_chkcoup())
        {
            state = S_COUP;
            prm.preinit = FALSE;
            prm.buf = cbuf;
            prm.buflen = MSGMAXLINE;
            prm.pbuf = "Press [TAB] to try it: ";
            prm.terms = TERMS;
            prm.index = MSG_LIN1;
            prm.buf[0] = 0;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
            prompting = TRUE;
        }
        break;
    case 'L':                   /* review old messages */
        _doreview();              /* will set state */
        break;
    case 'm':				/* send a message */
        state = S_MSGTO;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        strcpy(pbuf, "Message to: ");
        prm.pbuf = pbuf;
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;
        break;
    case 'M':				/* strategic/tactical map */
        if (SMAP(Context.snum))
            SFCLR(Context.snum, SHIP_F_MAP);
        else
            SFSET(Context.snum, SHIP_F_MAP);
        break;
    case 'N':				/* change pseudonym */
        strcpy(pbuf , "Old pseudonym: ") ;
        strcat(pbuf , Ships[Context.snum].alias);
        cp_putmsg(pbuf, MSG_LIN1);
        state = S_PSEUDO;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MAXUSERPNAME;
        prm.pbuf = "Enter a new pseudonym: ";
        prm.terms = TERMS;
        prm.index = MSG_LIN2;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;

        break;

    case 'O':
        setONode(nOptionsInit(NOPT_USER, FALSE, DSP_NODE_CP));
        break;

    case 'o':				/* orbit nearby planet */
        _doorbit( Context.snum );
        break;
    case 'P':				/* photon torpedo burst */
    {
        state = S_TARGET;
        desttarg = T_BURST;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        prm.pbuf = "Torpedo burst: ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;
    }
    break;
    case 'p':				/* photon torpedoes */
    {
        state = S_TARGET;
        desttarg = T_TORP;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        prm.pbuf = "Launch torpedo: ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;
    }
    break;
    case 'Q':				/* self destruct */
        if ( SCLOAKED(Context.snum) )
        {
            cp_putmsg( "The cloaking device is using all available power.",
                       MSG_LIN1 );
        }
        else
        {
            state = S_DESTRUCT;
            prm.preinit = FALSE;
            prm.buf = cbuf;
            prm.buflen = MSGMAXLINE;
            prm.pbuf = "Press [TAB] to initiate self-destruct sequence: ";
            prm.terms = TERMS;
            prm.index = MSG_LIN1;
            prm.buf[0] = 0;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
            prompting = TRUE;
        }
        break;
    case 'r':				/* refit */
        if (sStat.flags & SPSSTAT_FLAGS_REFIT)
        {
            if (_chkrefit())
            {
                state = S_REFIT;
                prm.preinit = FALSE;
                prm.buf = cbuf;
                prm.buflen = MSGMAXLINE;
                refitst = Ships[Context.snum].shiptype;
                sprintf(pbuf, "Refit ship type: %s", ShipTypes[refitst].name);
                prm.pbuf = pbuf;
                prm.terms = TERMS;
                prm.index = MSG_LIN1;
                prm.buf[0] = 0;
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
                cp_putmsg("Press [TAB] to change, [ENTER] to accept: ", MSG_LIN2);
                prompting = TRUE;
            }
        }
        else
            mglBeep(MGL_BEEP_ERR);
        break;
    case 'R':				/* repair mode */
        hudClearPrompt(MSG_LIN1);
        sendCommand(CPCMD_REPAIR, 0);
        break;
    case 't':
        if (_chktow())
        {
            state = S_TOW;
            prm.preinit = FALSE;
            prm.buf = cbuf;
            prm.buflen = MSGMAXLINE;
            prm.pbuf = "Tow which ship? ";
            prm.terms = TERMS;
            prm.index = MSG_LIN1;
            prm.buf[0] = 0;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
            prompting = TRUE;
        }
        break;
    case 'S':				/* more user stats */
        setONode(nUserlInit(DSP_NODE_CP, FALSE, Context.snum, FALSE, TRUE));
        break;
    case 'T':				/* team list */
        setONode(nTeamlInit(DSP_NODE_CP, FALSE, Ships[Context.snum].team));
        break;
    case 'u':				/* un-tractor */
        sendCommand(CPCMD_UNTOW, 0);
        break;
    case 'U':				/* user stats */
        setONode(nUserlInit(DSP_NODE_CP, FALSE, Context.snum, FALSE, FALSE));
        break;
    case 'W':				/* war and peace */
        for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
            twar[i] = Ships[Context.snum].war[i];

        state = S_WAR;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = 5;
        prm.pbuf = clbWarPrompt(Context.snum, twar);
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prm.buf[0] = 0;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
        prompting = TRUE;

        break;
    case '-':				/* shields down */
        _doshields( Context.snum, FALSE );
        break;
    case '+':				/* shields up */
        _doshields( Context.snum, TRUE );
        break;
    case '/':				/* player list */
        setONode(nShiplInit(DSP_NODE_CP, FALSE));  /* shipl node */
        break;

    case '[':                   /* zoom out */
    {
        if (SMAP(Context.snum))
        {                     /* LR */
            if (ncpLRMagFactor - 1 >= -5)
            {
                ncpLRMagFactor--;
                cqsEffectPlay(cqsTeamEffects[Ships[snum].team].mag, NULL,
                              0, 0, 0);
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpLRMagFactor, TRUE);
        }
        else
        {
            if (ncpSRMagFactor - 1 >= -5)
            {
                ncpSRMagFactor--;
                cqsEffectPlay(cqsTeamEffects[Ships[snum].team].mag, NULL,
                              0, 0, 0);
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpSRMagFactor, FALSE);
        }
    }
    break;

    case ']':                   /* zoom in */
    {
        if (SMAP(Context.snum))
        {                     /* LR */
            if (ncpLRMagFactor + 1 <= 5)
            {
                ncpLRMagFactor++;
                cqsEffectPlay(cqsTeamEffects[Ships[snum].team].mag, NULL,
                              0, 0, 0);
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpLRMagFactor, TRUE);
        }
        else
        {
            if (ncpSRMagFactor + 1 <= 5)
            {
                ncpSRMagFactor++;
                cqsEffectPlay(cqsTeamEffects[Ships[snum].team].mag, NULL,
                              0, 0, 0);
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpSRMagFactor, FALSE);
        }
    }
    break;

    case '?':				/* planet list */
        if (Context.snum >= 0 && Context.snum < MAXSHIPS)
            setONode(nPlanetlInit(DSP_NODE_CP, FALSE, Context.snum, Ships[Context.snum].team));
        else          /* then use user team if user doen't have a ship yet */
            setONode(nPlanetlInit(DSP_NODE_CP, FALSE, Context.snum, Users[Context.unum].team));
        break;
    case TERM_REDRAW:			/* clear all the prompts */
        hudClearPrompt(MSG_LIN1);
        hudClearPrompt(MSG_LIN2);
        hudClearPrompt(MSG_MSG);

        /* reset the scaling factors */
        ncpLRMagFactor = ncpSRMagFactor = 0;
        setViewerScaling(ncpLRMagFactor, TRUE);
        setViewerScaling(ncpSRMagFactor, FALSE);

        break;

    case TERM_NORMAL:		/* Have [ENTER] act like 'I[ENTER]'  */
    case '\n':
        cbuf[0] = 0;
        _doinfo(cbuf, TERM_NORMAL);
        break;

        /* ack red alert by turning klaxon off  Cataboligne -
           sound code 11.14.6 */
    case TERM_ABORT:
        if (alertHandle != CQS_INVHANDLE)
        {
            cqsEffectStop(alertHandle, FALSE);
            alertHandle = CQS_INVHANDLE;
        }
        break;

    case ' ':
        if (SMAP(snum))
            UserConf.DoLocalLRScan = !UserConf.DoLocalLRScan;
        break;

    case TERM_EXTRA:		/* Have [TAB] act like 'i\t' */
        cbuf[0] = 0;
        _doinfo(cbuf, TERM_EXTRA);
        break;

    case TERM_RELOAD:		/* have server resend current universe */
        sendCommand(CPCMD_RELOAD, 0);
        utLog("client: sent CPCMD_RELOAD");
        break;

    case -1:			/* really nothing, move along */
        break;

        /* nothing. */
    default:
        mglBeep(MGL_BEEP_ERR);
        cp_putmsg( "Type h for help.", MSG_LIN2 );
    }

    return;

}

/*
  Cataboligne - 11.15.6 - theme players for:
  - doomsday music
  - theme sounds (random selection? - need a text file with 1
  song per line, with possible complete path)
  - ship travel sounds (approach, battle, theme)
*/

/*
  play various theme music

  on approach - play music theme for team ships,
  default when no others supplied
  (actually happens in a certain distance range)

*/

/*
  theme music data from .soundsrc - doesnt seem to be in cqi file...hrm
  min distance
  max distance
  wait time in millis
  percentage
*/
#define BATTLE_MAXDIS 2200.0
#define BATTLE_MINDIS 1000.0
#define BATTLE_PROB 0.70        /* chance to play battle music */
#define APPROACH_MINDIS 1000.0
#define APPROACH_MAXDIS 1700.0
#define APPROACH_PROB 0.40      /* chance to play approach music */
#define THEME_PROB 0.30         /* chance to play random theme music */

static void themes()
{
    real dis;
    int mus = -1;                 /* the music we might play */
    static real odist = max(BATTLE_MAXDIS, APPROACH_MAXDIS);
    int snum = Context.snum;
    int warlike;
    real prob = rnd();
    int i;

    if (!cqsSoundAvailable || cqsMusicPlaying())
        return;

    /* go through each ship. */

    warlike = FALSE;
    for ( i = 0; i < MAXSHIPS; i++ )
    {
        int atwar = satwar(snum, i);

        if (i == snum)
            continue;               /* don't care about our ship */

        if ( Ships[i].status != SS_LIVE )
            continue;               /* don't care about the non-living */

        /* check range */
        dis = distf( Ships[snum].x, Ships[snum].y,
                     Ships[i].x, Ships[i].y );

        if (atwar && (dis > BATTLE_MAXDIS || dis < BATTLE_MINDIS))
        {
            continue;               /* not in the right 'battle range band' */
        }

        if (!atwar && (dis > APPROACH_MAXDIS || dis < APPROACH_MINDIS))
        {
            continue;
        } /* not on the right approach range */

        /* if we've already found a warlike ship, and this ship
           is not at war with you, we should ignore it. */
        if (warlike && !atwar)
        {
            continue;
        }

        /* see if we already found one closer */
        if (dis >= odist)
        {
            continue;
        }

        odist = dis;

        /* by now, we have found a closer living ship in range, choose
           the right music.  We give priority to warlike ships, so if
           we've already found one, it will not be overridden by the fact
           that a friendly ship is closer.  */

        if (atwar)
        {
            warlike = TRUE;
            mus = cqsTeamMusic[Ships[i].team].battle;
        }
        else if (!warlike /*&& Ships[Context.snum].team == Ships[i].team*/)
            mus = cqsTeamMusic[Ships[i].team].approach;
    }

    /* now, either we found some theme music to play or we didn't.  If we
       did, play it, else choose a random team's theme music */

    if (mus >= 0)
    {
        /* return if the probabilities are not with us */
        if (warlike && (prob > BATTLE_PROB))
            return;
        else if (prob > APPROACH_PROB)
            return;

        /* play it */
        cqsMusicPlay(mus, FALSE);
        return;
    }

    /* see if the stars are with us */
    if (prob > THEME_PROB)
        return;                     /* nope */

    /* if we are here, choose a random team song to play.  We randomly
       choose a team, and to provide some variation, we randomly choose
       the team's intro or theme music */

    /* first choose whether we will play theme or intro music */
    if (rnd() < 0.5)
        mus = cqsTeamMusic[rndint(0, NUMPLAYERTEAMS - 1)].theme;
    else
        mus = cqsTeamMusic[rndint(0, NUMPLAYERTEAMS - 1)].intro;

    /* now play it */
    cqsMusicPlay(mus, FALSE);

    /* done */
    return;
}

/* play doomsday theme music for nearby players if flags allow -
   dis is distance to doomsday */

/* play intro first when within this range */
#define DOOM_INTRODIS 2200.0
/* play kill music if in this range and self destructing */
#define DOOM_KILLDIS 1400.0
/* play doomsday main theme when in this range */
#define DOOM_MUSDIS 1800.0

static void doomsday_theme (void)
{
    real dis = dist( Ships[Context.snum].x, Ships[Context.snum].y,
                     Doomsday->x, Doomsday->y );
    static int first_doom = 0;

    if (Doomsday->status != DS_LIVE)
        return;

    /* doomsday music theme for ships nearby */

    /* first sight of doomsday - wait for other themes */
    if (first_doom < 2 && dis < DOOM_INTRODIS && !cqsMusicPlaying())
    {
        cqsMusicPlay(cqsDoomsdayMusic.doomin, FALSE);
        first_doom = 2;                                                                     /* intro music only plays once */
    }
    else if (Ships[Context.snum].sdfuse > 0 &&
             dis < DOOM_KILLDIS && first_doom != 3)
    {                           /* self destructing & doom < 1000.0 */
        /*
          this is a 37 sec sample -
          would be real nice if it could be timed so the last 10
          secs play with the ship explosion...
          NOTE: for this to happen we would have to start 12 secs
          prior to the player init of self destruct

          fix - we really want to fade any other music instantly - no delays
        */
        cqsMusicPlay(cqsDoomsdayMusic.doomkill, FALSE);
        first_doom = 3;   /* dont restart playing this */
    }
    else if (dis < DOOM_MUSDIS && !cqsMusicPlaying())
    {
        cqsMusicPlay(cqsDoomsdayMusic.doom, FALSE);
        first_doom = 4;   /* enable kill music if it played once already */
    }
}


void nCPInit(int istopnode)
{
    static int introsPlayed[NUMPLAYERTEAMS];
    char buf[CQI_NAMELEN];

    prompting = FALSE;
    state = S_NONE;
    clientFlags = 0;

    /* init timers */
    rftime = frameTime;
    lastblast = Ships[Context.snum].lastblast;
    lastphase = Ships[Context.snum].lastphase;
    pingPending = FALSE;
    pingStart = 0;

    /* init the hud when starting a fresh ship. */
    if (istopnode)
    {
        hudInitData();
        hudSetInfoFiringAngle(0);
        hudSetInfoTarget(-1, false);
        hudSetInfoTargetAngle(0);
        hudSetInfoTargetDist(0);
    }

    /* clear the prompt lines */
    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);
    hudClearPrompt(MSG_MSG);

    /* flush the input buffer */
    ibufFlush();

    /* first time through */
    if (!nCPNode.animQue)
    {
        int i;

        nCPNode.animQue = &animQue;
        animQueInit(nCPNode.animQue);

        /* setup the team torp animators */
        for (i=0; i<NUMPLAYERTEAMS; i++)
        {
            char nm[CQI_NAMELEN];

            snprintf(nm, CQI_NAMELEN - 1, "ship%c-torp",
                     Teams[i].name[0]);

            if (!animInitState(nm, &ncpTorpAnims[i], NULL))
                utLog("%s: failed to init animstate for animation '%s'",
                      __FUNCTION__, nm);
            else
                animQueAdd(nCPNode.animQue, &ncpTorpAnims[i]);
        }

        /* init the intro music array.  We want to ensure that we play
           a team-specific intro only once */
        for (i=0; i<NUMPLAYERTEAMS; i++)
            introsPlayed[i] = FALSE;

        bombingfx = cqsFindEffect("bombing");
    }

    /* only if we are running this as a topnode frpm nPlay
       do we want to do this */
    if (istopnode && !introsPlayed[Ships[Context.snum].team])
    {
        introsPlayed[Ships[Context.snum].team] = TRUE;
        snprintf(buf, CQI_NAMELEN - 1, "ship%c-intro",
                 Teams[Ships[Context.snum].team].name[0]);
        cqsMusicPlay(cqsFindMusic(buf), FALSE);
    }
    else if (istopnode)
    {                           /* play the theme music if
                                   we've already done the intro music
                                   for this team. */
        snprintf(buf, CQI_NAMELEN - 1, "ship%c-theme",
                 Teams[Ships[Context.snum].team].name[0]);
        cqsMusicPlay(cqsFindMusic(buf), FALSE);
    }
    /* else, don't start playing anything */

    setNode(&nCPNode);

    /* init this so the warp effects don't kick in incorrectly */
    if (istopnode)
        Ships[Context.snum].dwarp = -1;

    return;
}


static int nCPDisplay(dspConfig_t *dsp)
{
    /* Older servers do not send the ships SHIP_F_BOMBING flag, so if
       our state is S_BOMBING, but our ship isn't marked as such (by the
       server), we fake it by setting dobombing, so you can at least see
       your own ship bombing a planet.  If the server is new enough to
       send this flag, gldisplay() will display it instead of this
       boolean. */
    int dobombing = ((state == S_BOMBING) && !SBOMBING(Context.snum));

    /* Viewer */
    renderViewer(UserConf.doVBG, dobombing);

    /* Main/Hud */
    renderHud(dostats);

    /* draw the overlay bg if active */
    mglOverlayQuad();

    return NODE_OK;
}

static int nCPIdle(void)
{
    int pkttype;
    int now;
    char buf[PKT_MAXSIZE];
    uint32_t difftime = utDeltaGrand( Context.msgrand, &now );
    static uint32_t iterstart = 0;
    static uint32_t pingtime = 0;
    static uint32_t themetime = 0;
    static uint32_t dietime = 0;
    uint32_t iternow = clbGetMillis();
    static const uint32_t iterwait = 50;   /* ms */
    static const uint32_t pingwait = 2000; /* ms (2 seconds) */
    static const uint32_t themewait = 5000; /* ms (5 seconds) */
    static const uint32_t dyingwait = 5000; /* watching yourself die */
    real tdelta = (real)iternow - (real)iterstart;


    if (state == S_DEAD)
    {                           /* die for awhile */
        /* also turns off warp/engine sounds */
        /* nDead will stop all other sound effects for us on exit */
        setWarp(0);
        state = S_GHOST;
        /* start a timer so you can see yourself die :) */
        dietime = iternow;
    }

    while ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                       buf, PKT_MAXSIZE, 0, NULL)) > 0)
    {
        switch (pkttype)
        {
            /* FIXME: pings/keepalives should be handled at the
             *  renderNode() level, not here!  Next protocol rev.
             */
        case SP_ACK:
            sAckMsg.code = 0;     /* reset */
            processPacket(buf);
            /* see if it's a ping resp */
            if (sAckMsg.code == PERR_PINGRESP)
            {
                pingPending = FALSE;
                pktStats.pingAvg = (pktStats.pingAvg + (iternow - pingStart)) / 2;
                pingStart = 0;
                continue;
            }

            break;
        default:
            processPacket(buf);
            break;
        }
    }

    if (pkttype < 0)          /* some error */
    {
        utLog("nCPIdle: pktWaitForPacket returned %d", pkttype);
        Ships[Context.snum].status = SS_OFF;
        return NODE_EXIT;
    }

    /* send a ping if it's time */
    if (!pingPending && ((iternow - pingtime) > pingwait))
    {                           /* send a ping request */
        /* only send this if we aren't doing things that this packet would end
         *  up canceling...
         */

        /* FIXME: pings/keepalives should be handled at the renderNode()
         *  level, not here!  The server should handle these
         *  transparently without this crap.  Next protocol rev.
         */
        if (state != S_REFITING && state != S_BOMBING &&
            state != S_BEAMING  && state != S_DESTRUCTING &&
            state != S_WARRING && state != S_AUTOPILOT)
        {

            pingtime = iternow;
            pingStart = iternow;
            pingPending = TRUE;
            sendCommand(CPCMD_PING, 0);
        }
    }

    /* drive the local universe */
    if (tdelta > iterwait)
    {
        clbPlanetDrive(tdelta / 1000.0);
        clbTorpDrive(tdelta / 1000.0);
        iterstart = iternow;
        if (Context.recmode == RECMODE_ON)
            recGenTorpLoc();
    }

    /* if we are a ghost then we already know we are dead. */
    if ( (clientFlags & SPCLNTSTAT_FLAG_KILLED) &&
         state != S_GHOST && state != S_GHOSTING )
    {                           /* we died.  set the state and deal with
                                   it on the next frame */
        state = S_DEAD;
        prompting = FALSE;        /* doesn't really matter */
        return NODE_OK;
    }

    if (state == S_BOMBING && lastServerError)
    {                           /* the server stopped bombing for us */
        cqsEffectStop(bombingHandle, FALSE);
        sendCommand(CPCMD_BOMB, 0); /* to be sure */
        state = S_NONE;
        prompting = FALSE;
        hudClearPrompt(MSG_LIN2);
        return NODE_OK;           /* next iter will process the char */
    }

    if (state == S_BEAMING && lastServerError)
    {                           /* the server stopped beaming for us */
        cqsEffectStop(beamHandle, FALSE);
        sendCommand(CPCMD_BEAM, 0); /* to be sure */
        state = S_NONE;
        prompting = FALSE;
        hudClearPrompt(MSG_LIN2);
        return NODE_OK;           /* next iter will process the char */
    }

    if (state == S_REFITING)
    {                           /* done refiting? */
        if (utDeltaGrand( entertime, &now ) >= REFIT_GRAND)
        {
            hudClearPrompt(MSG_LIN1);
            hudClearPrompt(MSG_LIN2);
            state = S_NONE;
        }
        else
            return NODE_OK;
    }

    if (state == S_WARRING)
    {
        if (utDeltaGrand( entertime, &now ) >= REARM_GRAND)
        {
            hudClearPrompt(MSG_LIN1);
            hudClearPrompt(MSG_LIN2);
            state = S_NONE;
        }
        else
            return NODE_OK;
    }

    /* the nDead node will finish up with any further input if we are dead */
    if (state != S_GHOST && state != S_GHOSTING)
        nCPInput(0);                   /* handle any queued chars */

    /* check for messages */
    if (Context.msgok)
    {
        if (difftime >= NEWMSG_GRAND)
            if ( utGetMsg(Context.snum, &Ships[Context.snum].lastmsg))
            {
                rmesg(Context.snum, Ships[Context.snum].lastmsg, MSG_MSG);
                // only beep if the message wasn't from us...
                if (!(Msgs[Ships[Context.snum].lastmsg].from == MSG_FROM_SHIP
                      && (int)Msgs[Ships[Context.snum].lastmsg].fromDetail == Context.snum) )
                {
                    if (UserConf.MessageBell)
                        mglBeep(MGL_BEEP_MSG);
                }

                Context.msgrand = now;
            }
    }

    if (Context.recmode == RECMODE_ON)
    {
        if ((iternow - rftime) > (int)((1.0 / (real)Context.updsec) * 1000.0))
        {                           /* record a frame */
            recUpdateFrame();
            rftime = iternow;
        }
    }

    /*
      Cataboligne - code for theme checks - 11.20.6
      calls a theme check for any live ship in game

      hope this is ok here Jon
      also perhaps put doomsday_theme check here

      Jon: "Yes!" :)
    */
    if (CQS_ISENABLED(CQS_MUSIC))
    {
        if (((iternow - themetime) > themewait))
        {
            themetime = iternow;
            doomsday_theme();
            themes();
        }
    }

    if ((state == S_GHOST) && ((iternow - dietime) > dyingwait))
    {
        /* we've died long enough, time to go home :( */
        /* stop any effects we care about */
        cqsEffectStop(bombingHandle, FALSE);
        cqsEffectStop(beamHandle, FALSE);
        cqsEffectStop(alertHandle, FALSE);

        beamHandle = CQS_INVHANDLE;
        bombingHandle = CQS_INVHANDLE;
        alertHandle = CQS_INVHANDLE;

        /* in nDead */
        state = S_GHOSTING;

        nDeadInit();
    }
    return NODE_OK;
}

static int nCPInput(int ch)
{
    int cf = ch;                      /* backup of ch for domacros() */
    int irv, tmsg, i;
    real tdir;
    real x;
    int snum = Context.snum;

    if ((CQ_CHAR(ch) == 'B' || CQ_CHAR(ch) == 'b') &&
        CQ_MODIFIER(ch) & CQ_KEY_MOD_ALT)
    {
        UserConf.doVBG = !UserConf.doVBG;
        return NODE_OK;
    }

    /* display render/comm stats */
    if ((CQ_CHAR(ch) == 'S' || CQ_CHAR(ch) == 's') &&
        CQ_MODIFIER(ch) & CQ_KEY_MOD_ALT)
    {
        dostats = !dostats;
        return NODE_OK;
    }

    if ((CQ_CHAR(ch) == 'G' || CQ_CHAR(ch) == 'g') &&
        CQ_MODIFIER(ch) & CQ_KEY_MOD_ALT)
    {
        UserConf.DoTacBkg = !UserConf.DoTacBkg;
        return NODE_OK;
    }

    ch = CQ_CHAR(ch) | CQ_FKEY(ch);

    /* Control-/ (INSTA-QUIT (tm)) */
    if (ch == 0x1c)
    {
        /* first stop all music and effects */
        cqsEffectStop(CQS_INVHANDLE, TRUE);
        cqsMusicStop(TRUE);

        return NODE_EXIT;
    }

    if (state == S_REFITING && ch) /* if refitting, just que all chars */
    {
        ibufPutc(ch);
        return NODE_OK;
    }

    if (state == S_WARRING && ch)
    {
        ibufPutc(ch);
        return NODE_OK;
    }

    if (state == S_BOMBING && ch)
    {                           /* aborting */
        ibufPutc(ch);             /* just que it */
        cqsEffectStop(bombingHandle, FALSE);
        sendCommand(CPCMD_BOMB, 0);
        state = S_NONE;
        prompting = FALSE;
        cp_putmsg( abt, MSG_LIN1 );
        hudClearPrompt(MSG_LIN2);
        return NODE_OK;           /* next iter will process the char */
    }

    if (state == S_BEAMING && ch)
    {                           /* aborting */
        ibufPutc(ch);             /* just que it */
        cqsEffectStop(beamHandle, FALSE);
        sendCommand(CPCMD_BEAM, 0);
        state = S_NONE;
        prompting = FALSE;
        cp_putmsg( abt, MSG_LIN1 );
        hudClearPrompt(MSG_LIN2);
        return NODE_OK;           /* next iter will process the char */
    }

    if (ch == 0)
    {                           /* check for queued chars */
        if (ibufCount())
            ch = ibufGetc();
        else
            return NODE_OK;
    }
    else
    {
        if (ibufCount())
        {
            ibufPutc(ch);                 /* que char */
            ch = ibufGetc();
        }
    }

    if (prompting)
    {
        irv = prmProcInput(&prm, ch);

        switch (state)
        {
        case S_COURSE:
            if (irv > 0)
            {
                _docourse(prm.buf, ch);
                prompting = FALSE;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;

        case S_DOINFO:
            if (irv > 0)
            {
                _doinfo(prm.buf, ch);
                prompting = FALSE;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;

        case S_TARGET:
            if (irv > 0)
            {
                if (_gettarget(prm.buf,
                               (desttarg == T_PHASER) ? lastphase : lastblast,
                               &tdir, ch))
                {
                    lastblast = tdir; /* we set both of them */
                    lastphase = tdir;
                    switch (desttarg)
                    {
                    case T_PHASER:
                        _dophase(tdir);
                        break;
                    case T_TORP:
                        _dotorp(tdir, 1);
                        break;
                    case T_BURST:
                        _dotorp(tdir, 3);
                        break;
                    }
                }
                else
                    cp_putmsg( "Invalid targeting information.", MSG_LIN1 );

                prompting = FALSE;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;

        case  S_CLOAK:
            if (irv > 0)
            {
                _docloak(prm.buf, ch);
                prompting = FALSE;
                state = S_NONE;
            }

            break;

        case  S_ALLOC:
            if (irv > 0)
            {
                _doalloc(prm.buf, ch);
                prompting = FALSE;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;

        case  S_DISTRESS:
            if (irv > 0)
            {
                _dodistress(prm.buf, ch);
                prompting = FALSE;
                state = S_NONE;
            }

            break;

        case  S_COUP:
            if (irv > 0)
            {
                if (ch == TERM_EXTRA)
                {
                    cp_putmsg( "Attempting coup...", MSG_LIN1 );
                    sendCommand(CPCMD_COUP, 0);
                }
                else
                    cp_putmsg( abt, MSG_LIN1 );

                prompting = FALSE;
                state = S_NONE;
            }

            break;

        case  S_TOW:
            if (irv > 0)
            {
                _dotow(prm.buf, ch);
                prompting = FALSE;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;

        case  S_DOAUTOPILOT:
            if (irv > 0)
            {
                if (ch == TERM_EXTRA)
                {
                    sendCommand(CPCMD_AUTOPILOT, 1);
                    state = S_AUTOPILOT;
                }
                else
                {
                    hudClearPrompt(MSG_LIN1);
                    prompting = FALSE;
                    state = S_NONE;
                }
            }

            break;

        case S_AUTOPILOT:
            if (ch == TERM_ABORT)
            {
                sendCommand(CPCMD_AUTOPILOT, 0);
                hudClearPrompt(MSG_LIN1);
                prompting = FALSE;
                state = S_NONE;
            }
            else
                cp_putmsg("Press [ESC] to abort autopilot.", MSG_LIN1);

            break;

        case  S_MSGTO:
            if (irv > 0)
                _domsgto(prm.buf, ch,  UserConf.Terse); /* will set state appropriately */
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;

        case  S_MSG:
            if (irv != 0)
                _domsg(prm.buf, ch, irv); /* will set state appropriately */
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;

        case S_DESTRUCT:
            if (ch == TERM_EXTRA)
            {
                sendCommand(CPCMD_DESTRUCT, 1); /* blow yourself up */
                state = S_DESTRUCTING;
            }
            else
            {                   /* chicken */
                state = S_NONE;
                prompting = FALSE;
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
            }

            break;

        case S_DESTRUCTING:
            if (ch == TERM_ABORT)
            {
                sendCommand(CPCMD_DESTRUCT, 0); /* just kidding */
                state = S_NONE;
                prompting = FALSE;
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
                cp_putmsg( "Self destruct has been cancelled.", MSG_LIN1 );
            }
            else
            {                   /* chicken */
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
                prm.buf[0] = 0;
                cp_putmsg( "Press [ESC] to abort self destruct.", MSG_LIN1 );
                mglBeep(MGL_BEEP_ERR);
            }

            break;

        case S_BOMB:
            if (ch == TERM_EXTRA)
            {
                sendCommand(CPCMD_BOMB, 1);   /* start the bombing */
                state = S_BOMBING;
                prompting = FALSE;

                cqsEffectPlay(bombingfx, &bombingHandle, 0.0, 0.0, 0.0);
                hudClearPrompt(MSG_LIN1);
            }
            else
            {                   /* weak human */
                state = S_NONE;
                prompting = FALSE;
                cp_putmsg( abt, MSG_LIN1 );
                hudClearPrompt(MSG_LIN2);
            }

            break;

        case S_BEAMDIR:
            i = 0;
            switch (ch)
            {
            case 'u':
            case 'U':
                dirup = TRUE;
                i = TRUE;
                break;
            case 'd':
            case 'D':
            case TERM_EXTRA:
                dirup = FALSE;
                i = TRUE;
                break;
            default:
                cp_putmsg( abt, MSG_LIN1 );
                state = S_NONE;
                prompting = FALSE;
                return NODE_OK;
            }

            if (i)
            {                   /* time to ask how many */
                if ( dirup )
                    beamax = upmax;
                else
                    beamax = downmax;

                sprintf( pbuf, "Beam %s [1-%d] ", (dirup) ? "up" : "down",
                         beamax );
                state = S_BEAMNUM;
                prm.preinit = FALSE;
                prm.buf = cbuf;
                prm.buflen = 10;
                prm.pbuf = pbuf;
                prm.terms = TERMS;
                prm.index = MSG_LIN1;
                prm.buf[0] = 0;
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
                prompting = TRUE;
            }

            break;


        case S_BEAMNUM:
            if (irv > 0)
                _dobeam(prm.buf, ch); /* will set state appropriately */
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;


        case S_WAR:
            if (irv > 0)
            {
                if (ch == TERM_ABORT || ch == TERM_NORMAL)
                {
                    state = S_NONE;
                    prompting = FALSE;
                    hudClearPrompt(MSG_LIN1);
                    hudClearPrompt(MSG_LIN2);
                    return NODE_OK;
                }

                if (ch == TERM_EXTRA) /* accepted */
                {
                    int dowait = FALSE;
                    uint16_t cwar;

                    cwar = 0;
                    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                    {
                        if ( twar[i] && ! Ships[snum].war[i] )
                            dowait = TRUE;

                        if (twar[i])
                            cwar |= (1 << i);

                        /* we'll let it happen locally as well... */
                        Users[Ships[Context.snum].unum].war[i] = twar[i];
                        Ships[Context.snum].war[i] = twar[i];
                    }

                    hudClearPrompt(MSG_LIN1);
                    hudClearPrompt(MSG_LIN2);

                    if (dowait)
                    {
                        state = S_WARRING;
                        prompting = FALSE;
                        cp_putmsg(
                            "Reprogramming the battle computer, please stand by...",
                            MSG_LIN2 );

                        utGrand( &entertime ); /* gotta wait */
                    }
                    else
                    {
                        state = S_NONE;
                        prompting = FALSE;
                    }

                    sendCommand(CPCMD_SETWAR, (uint16_t)cwar);
                    return NODE_OK;
                }

            }
            else
            {
                prm.buf[0] = 0;
                for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                    if ( ch == (char)tolower( Teams[i].teamchar ) )
                    {
                        if ( ! twar[i] || ! Ships[Context.snum].rwar[i] )
                            twar[i] = ! twar[i];
                        prm.pbuf = clbWarPrompt(Context.snum, twar);
                        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
                    }
            }

            break;

        case S_PSEUDO:
            if (irv > 0)
            {
                if (ch != TERM_ABORT && prm.buf[0] != 0)
                    sendSetName(prm.buf);
                prompting = FALSE;
                state = S_NONE;
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

            break;

        case  S_REFIT:
            if (irv > 0)
            {
                switch (ch)
                {
                case TERM_ABORT: /* cancelled */
                    state = S_NONE;
                    prompting = FALSE;
                    hudClearPrompt(MSG_LIN1);
                    hudClearPrompt(MSG_LIN2);

                    break;

                case TERM_NORMAL:
                    hudClearPrompt(MSG_LIN1);
                    hudClearPrompt(MSG_LIN2);
                    prm.pbuf = "Refitting ship...";
                    hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
                    sendCommand(CPCMD_REFIT, (uint16_t)refitst);
                    prompting = FALSE;
                    utGrand( &entertime );
                    state = S_REFITING;

                    break;

                case TERM_EXTRA:
                    refitst = utModPlusOne( refitst + 1, MAXNUMSHIPTYPES );
                    sprintf(pbuf, "Refit ship type: %s",
                            ShipTypes[refitst].name);
                    prm.buf[0] = 0;
                    hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);

                    break;

                }

                break;

            }
        }
    }
    else
    {                           /* !prompting */

        if (state == S_REVIEW)
        {                       /* reviewing messages */
            switch (ch)
            {
	    case ' ':
	    case '<':
	    case CQ_KEY_UP:
	    case CQ_KEY_LEFT:
                tmsg = utModPlusOne( msg - 1, MAXMESSAGES );
                while(!clbCanRead( snum, tmsg ) && tmsg != lastone)
		{
                    tmsg = utModPlusOne( tmsg - 1, MAXMESSAGES );
		}
                if (tmsg == lastone)
		{
                    mglBeep(MGL_BEEP_ERR);
		}
                else
                    msg = tmsg;
                break;
	    case '>':
	    case CQ_KEY_DOWN:
	    case CQ_KEY_RIGHT:
                tmsg =  utModPlusOne( msg + 1, MAXMESSAGES );
                while(!clbCanRead( snum, tmsg ) && tmsg != lstmsg + 1 )
		{
                    tmsg = utModPlusOne( tmsg + 1, MAXMESSAGES );
		}
                if (tmsg == (lstmsg + 1))
		{
                    mglBeep(MGL_BEEP_ERR);
		}
                else
                    msg = tmsg;

                break;
	    default:
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
                state = S_NONE;
                return NODE_OK;
                break;
	    }

            if (!_review())
            {
                state = S_NONE;
                return NODE_OK;
            }
        }
        else
        {
            hudClearPrompt(MSG_LIN1);
            hudClearPrompt(MSG_LIN2);

            if (CQ_FKEY(cf) && !_KPAngle(cf, &x))
            {                           /* handle macros */
                if (ibufExpandMacro(_xlateFKey(cf)))
                {
                    while (ibufCount())
                        nCPInput(0); /* recursion warning */
                    return NODE_OK;
                }
                else
                    return NODE_OK;
            }
            else
                command(ch);
        }
    }

    return NODE_OK;
}

static int nCPMInput(mouseData_t *mdata)
{
#if 0
    utLog("%s: mod = %08x, button = %d state = %d\n",
          __FUNCTION__,
          mdata->mod,
          mdata->button,
          mdata->state);
#endif

    /* check to see if event is in viewer area, and a button press */
    if (((mdata->x >= dConf.vX) && (mdata->x <= (dConf.vX + dConf.vW)) &&
         (mdata->y >= dConf.vY) && (mdata->y <= (dConf.vY + dConf.vH))) &&
        (mdata->state == CQ_MOUSE_BDOWN))

    {
        /* compute an angle relative to center of viewer and do
           the macro thang */
        real dir = utAngle((real)(dConf.vX + (dConf.vW / 2.0)),
                           (real)(dConf.vY + (dConf.vH / 2.0)),
                           (real)mdata->x,
                           (real)(dConf.vY + dConf.vH) - ((real)mdata->y - dConf.vY)
            );

        /* Valid macros only */
        if (ibufExpandMouseMacro(mdata->button, mdata->mod, dir))
        {
            /* clean up the state - stop bombing/beaming if neccessary... */
            if (state == S_BOMBING)
            {
                cqsEffectStop(bombingHandle, FALSE);
                sendCommand(CPCMD_BOMB, 0); /* to be sure */
                state = S_NONE;
                prompting = FALSE;
                hudClearPrompt(MSG_LIN2);
            }

            if (state == S_BEAMING)
            {
                cqsEffectStop(beamHandle, FALSE);
                sendCommand(CPCMD_BEAM, 0); /* to be sure */
                state = S_NONE;
                prompting = FALSE;
                hudClearPrompt(MSG_LIN2);
            }
        }
    }

    return NODE_OK;
}
