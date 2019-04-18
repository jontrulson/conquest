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
#include "fmt/format.h"

#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "cb.h"
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
#include "ui.h"

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
#include "ping.h"

#include "cqsound.h"

#include "info.h"

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
static unsigned int entertime;

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
static int twar[NUM_PLAYERTEAMS];

/* the current prompt */
static prm_t prm;
static int prompting = false;

/* save lastblast and always use a local copy for both torp and phasering */
static real lastblast;
static real lastphase;

static const char *abt = "...aborted...";

static bool _dostats = false;     /* whether to display rendering stats */

/* Ping status */
static uint32_t pingStart = 0;
static int pingPending = false;

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

// do a reload after entering the game.  We do this since often
// initialization is going on, the driver may be starting, etc.  This
// forces a CPCMD_RELOAD command to be sent, which will cause the
// server to dump it's packet cache and send everything anew.
//
// It will limit the amount of "jitter" upon entry due to differences
// in what the client thinks are the planet and ship positions
// vs. what the server thinks.
static bool doReload = true;

static nodeStatus_t nCPDisplay(dspConfig_t *);
static nodeStatus_t nCPIdle(void);
static nodeStatus_t nCPInput(int ch);
static nodeStatus_t nCPMInput(mouseData_t *mdata);

static scrNode_t nCPNode = {
    nCPDisplay,                   /* display */
    nCPIdle,                      /* idle */
    nCPInput,                     /* input */
    nCPMInput,                    /* minput */
    NULL                          /* animVec */
};

static animVec_t animVec;

/* team torp anim states (exported) */
animStateRec_t ncpTorpAnims[NUM_PLAYERTEAMS];

/* convert a KP key into an angle */
static int _KPAngle(int ch, real *angle)
{
    int rv;

    switch (ch)
    {
    case CQ_KEY_HOME:                /* KP upper left */
        *angle = 135.0;
        rv = true;
        break;
    case CQ_KEY_PAGE_UP:                /* KP upper right */
        *angle = 45.0;
        rv = true;
        break;
    case CQ_KEY_END:                /* KP lower left */
        *angle = 225.0;
        rv = true;
        break;
    case CQ_KEY_PAGE_DOWN:                /* KP lower right */
        *angle = 315.0;
        rv = true;
        break;
    case CQ_KEY_UP:                /* up arrow */
        *angle = 90.0;
        rv = true;
        break;
    case CQ_KEY_DOWN:              /* down arrow */
        *angle = 270.0;
        rv = true;
        break;
    case CQ_KEY_LEFT:              /* left arrow */
        *angle = 180.0;
        rv = true;
        break;
    case CQ_KEY_RIGHT:             /* right arrow */
        *angle = 0.0;
        rv = true;
        break;
    default:
        rv = false;
        break;
    }

    return(rv);
}

static void _dowarp( int snum, real warp )
{
    hudClearPrompt(MSG_LIN2);
    std::string cbuf;

    /* Handle ship limitations. */

    warp = std::min( warp, (real)cbShipTypes[cbShips[snum].shiptype].warpMax );
    if (!sendCommand(CPCMD_SETWARP, (uint16_t)warp))
        return;

    cbuf = fmt::format("Warp {}.", int(warp));
    uiPutMsg(cbuf, MSG_LIN1 );

    return;
}

/* get a target */
static int _gettarget(std::string& buf, real cdefault, real *dir, char ch)
{
    int j;

    if ( ch == TERM_ABORT )
        return ( false );

    utDeleteBlanks( buf );

    if ( buf.empty() )
    {
        /* Default. */
        *dir = cdefault;
        return ( true );
    }

    if (utIsDigits(buf))
    {
        if ( ! utSafeCToI( &j, buf) )
            return ( false );
        *dir = utMod360( (real) j );
        return ( true );
    }
    if ( utArrowsToDir( buf.c_str(), dir ) )
        return ( true );

    return ( false );

}

static void _dophase( real dir )
{
    /* we used to do some pre-checks here, but depending on lag,
       these checks can be racy.  Now we will send the command regardless
       of what things look like locally, and let the server deal with it. */

    /*  Cataboligne - sound code 10.16.6 */
    if ( cbShips[Context.snum].pfuse == 0 &&
         clbUseFuel( Context.snum, PHASER_FUEL, true, false ) )
        cqsEffectPlay(cqsTeamEffects[cbShips[Context.snum].team].phaser, NULL,
                      0, 0, 0);

    uiPutMsg( "Firing phasers...", MSG_LIN2 );

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
            cqsEffectPlay(cqsTeamEffects[cbShips[Context.snum].team].torp3, NULL,
                          0, 0, 0);
        else
            cqsEffectPlay(cqsTeamEffects[cbShips[Context.snum].team].torp, NULL,
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
static void _doinfo( const std::string& inbuf, char ch, bool doOutput )
{
    info(inbuf, ch, doOutput);

    /* Cataboligne - Spocks viewer sound */
    if (doOutput && rnd() < 0.3)
        cqsEffectPlay(cqsTeamEffects[cbShips[Context.snum].team].info, NULL,
                      0, 0, 0);
}


/* 'read' a msg */
static void rmesg(int snum, int msgnum, int lin)
{
    std::string buf;

    clbFmtMsg(cbMsgs[msgnum].from, cbMsgs[msgnum].fromDetail,
              cbMsgs[msgnum].to, cbMsgs[msgnum].toDetail, buf);
    buf += ": ";
    buf += cbMsgs[msgnum].msgbuf;

    hudSetPrompt(lin, "", NoColor, buf, CyanColor);

    return;
}

static void _domydet(void)
{
    hudClearPrompt(MSG_LIN2);

    sendCommand(CPCMD_DETSELF, 0);

    uiPutMsg( "Detonating...", MSG_LIN1 );

    return;

}

static void _doshields( int snum, int up )
{

    if (!sendCommand(CPCMD_SETSHIELDS, (uint16_t)up))
        return;

    if ( up )
    {
        SFCLR(snum, SHIP_F_REPAIR);
        uiPutMsg( "Shields raised.", MSG_LIN1 );
    }
    else
        uiPutMsg( "Shields lowered.", MSG_LIN1 );

    hudClearPrompt(MSG_LIN2);

    return;

}

static void _doorbit( int snum )
{
    int pnum;
    std::string cbuf;

    if ( ( cbShips[snum].warp == ORBIT_CW ) || ( cbShips[snum].warp == ORBIT_CCW ) )
        infoPlanet( "But we are already orbiting ",
                    cbShips[snum].lockDetail, snum, true );
    else if ( ! clbFindOrbit( snum, &pnum ) )
    {
        cbuf += "We are not close enough to orbit, ";
        cbuf += cbShips[snum].alias;
        cbuf += '.';
        uiPutMsg( cbuf, MSG_LIN1 );
        hudClearPrompt(MSG_LIN2);
    }
    else if ( cbShips[snum].warp > MAX_ORBIT_WARP )
    {
        cbuf += "We are going too fast to orbit, ";
        cbuf += cbShips[snum].alias;
        cbuf += '.';
        uiPutMsg( cbuf, MSG_LIN1 );

        cbuf = fmt::format("Maximum orbital insertion velocity is warp {:.1f}.",
                           MAX_ORBIT_WARP);
        uiPutMsg( cbuf, MSG_LIN2 );
    }
    else
    {
        sendCommand(CPCMD_ORBIT, 0);
        infoPlanet( "Coming into orbit around ", pnum, snum, true );
    }

    return;

}

static void _doalloc(const char *buf, char ch)
{
    int snum = Context.snum;
    int alloc;
    int dwalloc = 0;

    switch (ch)
    {
    case TERM_EXTRA:
        dwalloc = cbShips[snum].engalloc;
        break;

    case TERM_NORMAL:
        utSafeCToI( &alloc, buf, 0 );			/* ignore status */
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

    if ( cbShips[snum].wfuse > 0 )
        uiPutMsg( "Weapons are currently overloaded.", MSG_LIN1 );
    else if ( clbUseFuel( snum, DETONATE_FUEL, true, false ) )
    {				/* we don't really use fuel here on the
				   client*/
        uiPutMsg( "detonating...", MSG_LIN1 );
        sendCommand(CPCMD_DETENEMY, 0);
    }
    else
        uiPutMsg( "Not enough fuel to fire detonators.", MSG_LIN1 );

    return;

}

static void _dodistress(const char *buf, char ch)
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
    static const char *ntp="We must be orbiting a team owned planet to refit.";
    static const char *nek="You must have at least one kill to refit.";
    static const char *cararm="You cannot refit while carrying armies";

    hudClearPrompt(MSG_LIN2);

    /* Check for allowability. */
    if ( oneplace( cbShips[snum].kills ) < MIN_REFIT_KILLS )
    {
        uiPutMsg( nek, MSG_LIN1 );
        return false;
    }

    if (cbShips[snum].lock == LOCK_PLANET
        && cbShips[snum].lockDetail < cbLimits.maxPlanets())
    {
        int pnum = cbShips[snum].lockDetail;

        if (cbPlanets[pnum].team != cbShips[snum].team || cbShips[snum].warp >= 0.0)
        {
            uiPutMsg( ntp, MSG_LIN1 );
            return false;
        }
    }

    if (cbShips[snum].armies != 0)
    {
        uiPutMsg( cararm, MSG_LIN1 );
        return false;
    }

    return true;
}

static int _chkcoup(void)
{
    int snum = Context.snum;
    int i, pnum;
    const char *nhp="We must be orbiting our home planet to attempt a coup.";

    hudClearPrompt(MSG_LIN2);

    /* some checks we will do locally, the rest will have to be handled by
       the server */
    /* Check for allowability. */
    if ( oneplace( cbShips[snum].kills ) < MIN_COUP_KILLS )
    {
        uiPutMsg(
            "Fleet orders require three kills before a coup can be attempted.",
            MSG_LIN1 );
        return false;
    }
    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        if ( cbPlanets[i].team == cbShips[snum].team && cbPlanets[i].armies > 0 )
        {
            uiPutMsg( "We don't need to coup, we still have armies left!",
                       MSG_LIN1 );
            return false;
        }
    if ( cbShips[snum].warp >= 0.0 )
    {
        uiPutMsg( nhp, MSG_LIN1 );
        return false;
    }
    // the assumption is that if warp < 0, we are in orbit and
    // therefore we are locked onto the planet we are orbiting
    pnum = (int)cbShips[snum].lockDetail;
    if ( pnum != cbTeams[cbShips[snum].team].homeplanet )
    {
        uiPutMsg( nhp, MSG_LIN1 );
        return false;
    }
    if ( cbPlanets[pnum].armies > MAX_COUP_ENEMY_ARMIES )
    {
        uiPutMsg( "The enemy is still too strong to attempt a coup.",
                   MSG_LIN1 );
        return false;
    }
    i = cbPlanets[pnum].uninhabtime;
    if ( i > 0 )
    {
        std::string cbuf =
            fmt::format("This planet is uninhabitable for {} more minutes.",
                        i);
        uiPutMsg( cbuf, MSG_LIN1 );
        return false;
    }

    return true;
}

static void _dotow(const char *buf, int ch)
{
    int other;
    if (ch == TERM_ABORT)
        return;

    if (utSafeCToI( &other, buf, 0 ))
        sendCommand(CPCMD_TOW, (uint16_t)other);

    return;
}

/* modifies state */
static void _domsgto(const std::string& buf, int ch, int terse)
{
    int i, j;
    std::string tbuf;
    static const char *nf="Not found.";
    static const char *huh="I don't understand.";
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
        prompting = false;

        return;
    }

    tbuf = buf;

    /* TAB or ENTER means use the target from the last message. */
    editing = ( (ch == TERM_EXTRA || ch == TERM_NORMAL) && buf.empty()
        && to != MSG_TO_NOONE);
    if ( editing )
    {
        /* Make up a default string using the last target. */
        if ( to == MSG_TO_SHIP && toDetail < cbLimits.maxShips() )
            tbuf = fmt::format("{}", (int)toDetail );
        else if ( to == MSG_TO_TEAM && toDetail < NUM_PLAYERTEAMS )
            tbuf = cbTeams[toDetail].name;
        else switch ( to )
             {
             case MSG_TO_ALL:
                 tbuf = "All";
                 break;
             case MSG_TO_GOD:
                 tbuf = "GOD";
                 break;
             case MSG_TO_IMPLEMENTORS:
                 tbuf = "Implementors";
                 break;
             case MSG_TO_FRIENDLY:
                 tbuf = "Friend";
                 break;
             default:
                 tbuf.clear();
                 break;
             }
    }

    /* Got a target, parse it. */
    utDeleteBlanks( tbuf );

    // If it's empty, assume error - stop prompting
    if (tbuf.empty())
    {
        hudClearPrompt(MSG_LIN1);
        hudClearPrompt(MSG_LIN2);
        state = S_NONE;
        prompting = false;
        return;
    }

    // else, figure out who it's too
    if (utIsDigits(tbuf))
    {
        /* All digits means a ship number. */
        utSafeCToI( &j, tbuf, 0 );		/* ignore status */
        if ( j < 0 || j >= cbLimits.maxShips() )
	{
            uiPutMsg( "No such ship.", MSG_LIN2 );
            hudClearPrompt(MSG_LIN1);
            state = S_NONE;
            prompting = false;
            return;
	}
        if ( cbShips[j].status != SS_LIVE )
	{
            uiPutMsg( nf, MSG_LIN2 );
            hudClearPrompt(MSG_LIN1);
            state = S_NONE;
            prompting = false;
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
             if ((char)toupper(tbuf[0]) == 'F'
                 && (char)toupper(tbuf[1]) == 'R')
             {			/* to friendlies */
                 to = MSG_TO_FRIENDLY;
                 toDetail = 0;
             }
             else
             {
                 /* Check for a team character. */
                 for ( i = 0; i < NUM_PLAYERTEAMS; i = i + 1 )
                     if ( tbuf[0] == cbTeams[i].teamchar
                          || tbuf[0] == (char)tolower(cbTeams[i].teamchar) )
                         break;

                 if ( i >= NUM_PLAYERTEAMS )
                 {
                     uiPutMsg( huh, MSG_LIN2 );
                     hudClearPrompt(MSG_LIN1);
                     state = S_NONE;
                     prompting = false;
                     return;
                 }
                 to = MSG_TO_TEAM;
                 toDetail = i;
             };
             break;
         }
    }

    /* Now, construct a header for the selected target. */
    tbuf = "Message to ";
    if ( to == MSG_TO_SHIP && toDetail < cbLimits.maxShips() )
    {
        if ( cbShips[to].status != SS_LIVE )
	{
            uiPutMsg( nf, MSG_LIN2 );
            hudClearPrompt(MSG_LIN1);
            state = S_NONE;
            prompting = false;
            return;
	}
        utAppendShip(tbuf, (int)toDetail);
        tbuf += ':';
    }
    else if ( to == MSG_TO_TEAM && toDetail < NUM_PLAYERTEAMS )
    {
        tbuf += cbTeams[toDetail].name;
        tbuf += "s:";
    }
    else
    {
        switch ( to )
         {
         case MSG_TO_ALL:
             tbuf += "everyone:";
             break;
         case MSG_TO_GOD:
             tbuf += "GOD:";
             break;
         case MSG_TO_IMPLEMENTORS:
             tbuf += "The Implementors:";
             break;
         case MSG_TO_FRIENDLY:
             tbuf += "Friend:";
             break;
         default:
             uiPutMsg( huh, MSG_LIN2 );
             return;
             break;
         }
    }

    if (!terse)
        tbuf += " ([ESC] to abort)";

    uiPutMsg( tbuf, MSG_LIN1 );
    hudClearPrompt(MSG_LIN2);

    msgto = to;                   /* set global */
    msgtoDetail = toDetail;

    state = S_MSG;
    prm.preinit = false;
    prm.buf.clear();
    prm.buflen = MAX_MSGSIZE;
    prm.pbuf = "> ";
    prm.index = MSG_LIN2;
    hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                 NoColor);
    prompting = true;

    return;
}

/* modifies state */
static void _domsg(const std::string& _msg, int ch, int irv)
{
    std::string msg = _msg;

    /* if maxlen reached */
    if (irv == PRM_MAXLEN)
    {
        /* then we need to send what we have
           and continue */
        size_t lastWordPos = msg.rfind(" ");

        // found last word
        if (lastWordPos != std::string::npos)
        {
            std::string lastWord = msg.substr(lastWordPos);

            prm.pbuf = "- ";
            // preload the buffer with the last word of message and
            // last char received
            prm.buf = fmt::format("{}{}", lastWord, (char)ch);

            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                         CyanColor);

            // delete the last word from msg
            msg.erase(lastWordPos);

            std::string sendBuf = fmt::format("{} -", msg);

            sendMessage(msgto, msgtoDetail, sendBuf.c_str());
        }
        else
        {
            // no words in msg, send whole thing
            prm.pbuf = "- ";
            prm.buf = ch;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                         CyanColor);
            sendMessage(msgto, msgtoDetail, msg.c_str());
        }

        return;
    }
    else
    {                           /* ready or abort */
        if (ch != TERM_ABORT)
            sendMessage(msgto, msgtoDetail, msg.c_str()); /* just send it */

        state = S_NONE;
        prompting = false;
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
static void _docourse( std::string& buf, char ch)
{
    int i, j, sorpnum, xsorpnum, count;
    InfoNear what;
    InfoSpecial token;
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

    // we need to treat a search for a ship a bit specially...
    if ( buf.size() > 1 && buf[0] == 's' && ::isdigit(buf[1]) )
    {
        std::string substr = buf.substr(1);
        // make sure they are all digits...
        if (utIsDigits(substr))
        {
            /* Ship. */
            if ( utSafeCToI( &sorpnum, substr) )
                what = NEAR_SHIP;
        }
    }
    else
    {
        InfoNear near;
        if (utIsDigits(buf))
        {
            /* Raw angle. */
            hudClearPrompt( MSG_LIN1 );
            if ( utSafeCToI( &j, buf) )
            {
                what = NEAR_DIRECTION;
                dir = (real)utMod360( (real)( j ) );
            }
        }
        else if ( utArrowsToDir( buf.c_str(), &dir ) )
            what = NEAR_DIRECTION;
        else if ( utIsSpecial( buf.c_str(), &near, &token, &count ) )
        {
            if ( clbFindSpecial( snum, token, count, &sorpnum, &xsorpnum ) )
                what = near;
        }
        else if ( clbPlanetMatch( buf.c_str(), &sorpnum, false ) )
            what = NEAR_PLANET;
    }

    switch ( what )
    {
    case NEAR_SHIP:
        if ( sorpnum < 0 || sorpnum >= cbLimits.maxShips() )
	{
            uiPutMsg( "No such ship.", MSG_LIN2 );
            return;
	}
        if ( sorpnum == snum )
	{
            hudClearPrompt(MSG_LIN1);
            return;
	}
        if ( cbShips[sorpnum].status != SS_LIVE )
	{
            uiPutMsg( "Not found.", MSG_LIN2 );
            return;
	}

        if ( SCLOAKED(sorpnum) )
	{
            if ( cbShips[sorpnum].warp <= 0.0 )
	    {
                uiPutMsg( "Sensors are unable to lock on.", MSG_LIN2 );
                return;
	    }
	}

        appx = cbShips[sorpnum].x;
        appy = cbShips[sorpnum].y;

        dir = utAngle( cbShips[snum].x, cbShips[snum].y, appx, appy );

        /* Give info if he used TAB. */
        if ( ch == TERM_EXTRA )
            infoShip( sorpnum, snum, true );
        else
            hudClearPrompt(MSG_LIN1);
        break;
    case NEAR_PLANET:
        dir = utAngle( cbShips[snum].x, cbShips[snum].y, cbPlanets[sorpnum].x, cbPlanets[sorpnum].y );
        if ( ch == TERM_EXTRA )
	{
            newlock = LOCK_PLANET;
            newlockDetail = sorpnum;
            infoPlanet( "Now locked on to ", sorpnum, snum, true );
	}
        else
            infoPlanet( "Setting course for ", sorpnum, snum, true );
        break;
    case NEAR_DIRECTION:
        hudClearPrompt(MSG_LIN1);
        break;
    case NEAR_NONE:
        uiPutMsg( "Not found.", MSG_LIN2 );
        return;
        break;
    default:
        /* This includes NEAR_ERROR. */
        uiPutMsg( "I don't understand.", MSG_LIN2 );
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
        uiPutMsg( "Cloaking device disengaged.", MSG_LIN1 );
        return false;
    }

    return true;
}

static void _docloak(char ch)
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
        msg--;
        if (msg < 0)
            msg = cbLimits.maxMsgs() - 1;
        if (msg == lastone)
        {
            state = S_NONE;
            return false;
        }
    }

    return true;

}


static void _doreview(void)
{
    int snum = Context.snum;
    int i;

    lstmsg = cbShips[snum].lastmsg;	/* don't want lstmsg changing while
                                           reading old ones. */

    lastone = mod( cbConqInfo->lastmsg+1, cbLimits.maxMsgs() );
    if ( snum >= 0 && snum < cbLimits.maxShips() )
    {
        if ( cbShips[snum].lastmsg == LMSG_NEEDINIT )
        {
            uiPutMsg( "There are no old messages.", MSG_LIN1 );
            return;               /* none to read */
        }
        i = cbShips[snum].alastmsg;
        if ( i != LMSG_READALL )
            lastone = i;
    }

    msg = lstmsg;

    state = S_REVIEW;

    hudClearPrompt(MSG_LIN1);
    if (!_review())
    {
        uiPutMsg( "There are no old messages.", MSG_LIN1 );
        return;               /* none to read */
    }


    uiPutMsg("--- [SPACE] for more, arrows to scroll, any key to quit ---",
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
    if ( cbShips[snum].warp >= 0.0 )
    {
        uiPutMsg( "We must be orbiting a planet to bombard it.", MSG_LIN1 );
        return;
    }
    pnum = cbShips[snum].lockDetail;
    if ( cbPlanets[pnum].type == PLANET_SUN || cbPlanets[pnum].type == PLANET_MOON ||
         cbPlanets[pnum].team == TEAM_NOTEAM || cbPlanets[pnum].armies == 0 )
    {
        uiPutMsg( "There is no one there to bombard.", MSG_LIN1 );
        return;
    }
    if ( cbPlanets[pnum].team == cbShips[snum].team )
    {
        uiPutMsg( "We can't bomb our own armies!", MSG_LIN1 );
        return;
    }

    if ( cbPlanets[pnum].team != TEAM_SELFRULED && cbPlanets[pnum].team != TEAM_GOD )
        if ( ! cbShips[snum].war[cbPlanets[pnum].team] )
        {
            uiPutMsg( "But we are not at war with this planet!", MSG_LIN1 );
            return;
        }

    /* set up the state, and proceed. */

    state = S_BOMB;
    prm.preinit = false;
    prm.buf.clear();
    prm.buflen = MAX_MSGLINE_LENGTH;
    prm.pbuf = fmt::format("Press [TAB] to bombard {}, {} armies:",
        cbPlanets[pnum].name, cbPlanets[pnum].armies);
    prm.index = MSG_LIN1;
    hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                 NoColor);
    prompting = true;

    clientLastServerAckCode = 0; /* so the server can tell us to stop */

    return;
}

/* sets state */
static void _initbeam()
{
    int snum = Context.snum;
    int pnum, capacity, i;
    real rkills;
    const char *lastfew="Fleet orders prohibit removing the last three armies.";
    std::string cbuf;

    hudClearPrompt(MSG_LIN1);
    hudClearPrompt(MSG_LIN2);

    /* all of these checks are performed server-side as well, but we also
       check here for the obvious problems so we can save time without
       bothering the server for something it will refuse anyway. */

    /* at least the basic checks could be split into a seperate func that
       could be used by both client and server */

    /* Check for allowability. */
    if ( cbShips[snum].warp >= 0.0 )
    {
        uiPutMsg( "We must be orbiting a planet to use the transporter.",
                   MSG_LIN1 );
        return;
    }
    pnum = cbShips[snum].lockDetail;
    if ( cbShips[snum].armies > 0 )
    {
        if ( cbPlanets[pnum].type == PLANET_SUN )
	{
            uiPutMsg( "Idiot!  Our armies will fry down there!", MSG_LIN1 );
            return;
	}
        else if ( cbPlanets[pnum].type == PLANET_MOON )
	{
            uiPutMsg( "Fool!  Our armies will suffocate down there!",
                       MSG_LIN1 );
            return;
	}
        else if ( cbPlanets[pnum].team == TEAM_GOD )
	{
            uiPutMsg(
                "GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
                MSG_LIN1 );
            return;
	}
    }

    i = cbPlanets[pnum].uninhabtime;
    if ( i > 0 )
    {
        cbuf = fmt::format("This planet is uninhabitable for {} more minute",
                           i );
        if ( i != 1 )
            cbuf += 's';
        cbuf += '.';
        uiPutMsg( cbuf, MSG_LIN1 );
        return;
    }

    /* can take empty planets */
    if ( cbPlanets[pnum].team != cbShips[snum].team &&
         cbPlanets[pnum].team != TEAM_SELFRULED &&
         cbPlanets[pnum].team != TEAM_NOTEAM )
        if ( ! cbShips[snum].war[cbPlanets[pnum].team] && cbPlanets[pnum].armies != 0)
        {
            uiPutMsg( "But we are not at war with this planet!", MSG_LIN1 );
            return;
        }

    if ( cbShips[snum].armies == 0 &&
         cbPlanets[pnum].team == cbShips[snum].team && cbPlanets[pnum].armies <= MIN_BEAM_ARMIES )
    {
        uiPutMsg( lastfew, MSG_LIN1 );
        return;
    }

    rkills = cbShips[snum].kills;

    if ( rkills < (real)1.0 )
    {
        uiPutMsg(
            "Fleet orders prohibit beaming armies until you have a kill.",
            MSG_LIN1 );
        return;
    }

    /* Figure out what can be beamed. */
    downmax = cbShips[snum].armies;
    if ( clbSPWar(snum,pnum) ||
         cbPlanets[pnum].team == TEAM_SELFRULED ||
         cbPlanets[pnum].team == TEAM_NOTEAM ||
         cbPlanets[pnum].team == TEAM_GOD ||
         cbPlanets[pnum].armies == 0 )
    {
        upmax = 0;
    }
    else
    {
        capacity = (int)std::min( rkills * 2,
                                  (real)cbShipTypes[cbShips[snum].shiptype].armyMax );
        upmax = std::min( cbPlanets[pnum].armies - MIN_BEAM_ARMIES,
                          capacity - cbShips[snum].armies );
    }

    /* If there are armies to beam but we're selfwar... */
    if ( upmax > 0 && selfwar(snum) && cbShips[snum].team == cbPlanets[pnum].team )
    {
        if ( downmax <= 0 )
	{
            cbuf = "The arm";
            if ( upmax == 1 )
                cbuf += "y is";
            else
                cbuf += "ies are";
            cbuf += " reluctant to beam aboard a pirate vessel.";
            uiPutMsg( cbuf, MSG_LIN1 );
            return;
	}
        upmax = 0;
    }

    /* Figure out which direction to beam. */
    if ( upmax <= 0 && downmax <= 0 )
    {
        uiPutMsg( "There is no one to beam.", MSG_LIN1 );
        return;
    }

    if ( upmax <= 0 )
        dirup = false;
    else if ( downmax <= 0 )
        dirup = true;
    else
    {                           /* need to ask beam dir... */
        state = S_BEAMDIR;
        prm.preinit = false;
        prm.pbuf = "Beam [up or down] ";
        prm.index = MSG_LIN1;
        prm.buf.clear();
        prm.buflen = 10;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);
        prompting = true;

        return;
    }

    /* else, get the number to beam */

    if ( dirup )
        beamax = upmax;
    else
        beamax = downmax;

    /* Figure out how many armies should be beamed. */
    state = S_BEAMNUM;
    prm.preinit = false;
    prm.pbuf = fmt::format("Beam {} [1-{}] ",
                           ((dirup) ? "up" : "down"),
                           beamax);
    prm.index = MSG_LIN1;
    prm.buf.clear();
    prm.buflen = 10;
    hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                 NoColor);
    prompting = true;

    return;
}

static void _dobeam(std::string& buf, int ch)
{
    int num;

    if ( ch == TERM_ABORT )
    {
        state = S_NONE;
        prompting = false;
        uiPutMsg( abt, MSG_LIN1 );
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
            prompting = false;
            uiPutMsg( abt, MSG_LIN1 );
            return;
	}
        utSafeCToI( &num, buf);
        if ( num < 1 || num > beamax )
	{
            uiPutMsg( abt, MSG_LIN1 );
            state = S_NONE;
            prompting = false;
            return;
	}
    }

    /* now we start the phun. */

    clientLastServerAckCode = 0;

    /* detail is (armies & 0x00ff), 0x8000 set if beaming down */

    /* start the effects */
    if (dirup)
        cqsEffectPlay(cqsTeamEffects[cbShips[Context.snum].team].beamu,
                      &beamHandle,
                      0, 0, 0);
    else
        cqsEffectPlay(cqsTeamEffects[cbShips[Context.snum].team].beamd,
                      &beamHandle,
                      0, 0, 0);

    sendCommand(CPCMD_BEAM,
                (dirup) ? (uint16_t)(num & 0x00ff):
                (uint16_t)((num & 0x00ff) | 0x8000));

    state = S_BEAMING;
    prompting = false;

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
        if ( UAUTOPILOT(cbShips[Context.snum].unum) )
	{
            state = S_DOAUTOPILOT;
            prm.preinit = false;
            prm.buf.clear();
            prm.buflen = MAX_MSGLINE_LENGTH;
            prm.pbuf = "Press [TAB] to engage autopilot: ";
            prm.index = MSG_LIN1;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                         NoColor);
            prompting = true;
	}
        else
	{
            mglBeep(MGL_BEEP_ERR);
            uiPutMsg( "Type h for help.", MSG_LIN2 );
	}
        break;
    case 'A':				/* change allocation */
        state = S_ALLOC;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "New weapons allocation: (30-70) ";
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor,
                     prm.buf, NoColor);
        prompting = true;
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
            prm.preinit = false;
            prm.buf.clear();
            prm.buflen = MAX_MSGLINE_LENGTH;
            prm.pbuf = "Press [TAB] to engage cloaking device: ";
            prm.index = MSG_LIN1;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                         NoColor);
            prompting = true;
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
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "Press [TAB] to send an emergency distress call: ";
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);

        prompting = true;
        break;
    case 'f':				/* phasers */
    {
        state = S_TARGET;
        desttarg = T_PHASER;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "Fire phasers: ";
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);
        prompting = true;
    }
    break;
    case 'F':				/* phasers, same direction */
        _dophase(lastphase);
        break;
    case 'h':
        setONode(nCPHelpInit(false));
        break;
    case 'H':
        setONode(nHistlInit(DSP_NODE_CP, false));
        break;
    case 'i':				/* information */
        state = S_DOINFO;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "Information on: ";
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);

        prompting = true;
        break;
    case 'k':				/* set course */
        state = S_COURSE;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "Come to course: ";
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);
        prompting = true;
        break;
    case 'K':				/* coup */
        if (_chkcoup())
        {
            state = S_COUP;
            prm.preinit = false;
            prm.buf.clear();
            prm.buflen = MAX_MSGLINE_LENGTH;
            prm.pbuf = "Press [TAB] to try it: ";
            prm.index = MSG_LIN1;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                         NoColor);
            prompting = true;
        }
        break;
    case 'L':                   /* review old messages */
        _doreview();              /* will set state */
        break;
    case 'm':				/* send a message */
        state = S_MSGTO;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "Message to: ";
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);
        prompting = true;
        break;
    case 'M':				/* strategic/tactical map */
        if (SMAP(Context.snum))
            SFCLR(Context.snum, SHIP_F_MAP);
        else
            SFSET(Context.snum, SHIP_F_MAP);
        break;
    case 'N':				/* change pseudonym */
    {
        std::string oldPseudo = fmt::format("Old pseudonym: {}",
                                            cbShips[Context.snum].alias);
        uiPutMsg(oldPseudo, MSG_LIN1);

        state = S_PSEUDO;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_USERNAME;
        prm.pbuf = "Enter a new pseudonym: ";
        prm.index = MSG_LIN2;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);
        prompting = true;
    }
    break;

    case 'O':
        setONode(nOptionsInit(NOPT_USER, false, DSP_NODE_CP));
        break;

    case 'o':				/* orbit nearby planet */
        _doorbit( Context.snum );
        break;
    case 'P':				/* photon torpedo burst */
    {
        state = S_TARGET;
        desttarg = T_BURST;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "Torpedo burst: ";
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);
        prompting = true;
    }
    break;
    case 'p':				/* photon torpedoes */
    {
        state = S_TARGET;
        desttarg = T_TORP;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "Launch torpedo: ";
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);
        prompting = true;
    }
    break;
    case 'Q':				/* self destruct */
        if ( SCLOAKED(Context.snum) )
        {
            uiPutMsg( "The cloaking device is using all available power.",
                       MSG_LIN1 );
        }
        else
        {
            state = S_DESTRUCT;
            prm.preinit = false;
            prm.buf.clear();
            prm.buflen = MAX_MSGLINE_LENGTH;
            prm.pbuf = "Press [TAB] to initiate self-destruct sequence: ";
            prm.index = MSG_LIN1;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                         NoColor);
            prompting = true;
        }
        break;
    case 'r':				/* refit */
        if (sStat.serverFlags & SERVER_F_REFIT)
        {
            if (_chkrefit())
            {
                state = S_REFIT;
                prm.preinit = false;
                prm.buf.clear();
                prm.buflen = MAX_MSGLINE_LENGTH;
                refitst = cbShips[Context.snum].shiptype;
                prm.pbuf = fmt::format("Refit ship type: {}",
                                       cbShipTypes[refitst].name);
                prm.index = MSG_LIN1;
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, NoColor);
                uiPutMsg("Press [TAB] to change, [ENTER] to accept: ",
                          MSG_LIN2);
                prompting = true;
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
        hudClearPrompt(MSG_LIN1);
        hudClearPrompt(MSG_LIN2);
        if (!STOWING(Context.snum))
        {
            state = S_TOW;
            prm.preinit = false;
            prm.buf.clear();
            prm.buflen = MAX_MSGLINE_LENGTH;
            prm.pbuf = "Tow which ship? ";
            prm.index = MSG_LIN1;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                         NoColor);
            prompting = true;
        }
        else if (STOWING(Context.snum))
        {
            // send another tow command, which will cancel our current tow
            sendCommand(CPCMD_TOW, cbShips[Context.snum].towing);
        }
        break;
    case 'S':				/* more user stats */
        setONode(nUserlInit(DSP_NODE_CP, false, Context.snum, false, true));
        break;
    case 'T':				/* team list */
        setONode(nTeamlInit(DSP_NODE_CP, false, cbShips[Context.snum].team));
        break;
    case 'u':				/* un-tractor */
        sendCommand(CPCMD_UNTOW, 0);
        break;
    case 'U':				/* user stats */
        setONode(nUserlInit(DSP_NODE_CP, false, Context.snum, false, false));
        break;
    case 'W':				/* war and peace */
        for ( i = 0; i < NUM_PLAYERTEAMS; i = i + 1 )
            twar[i] = cbShips[Context.snum].war[i];

        state = S_WAR;
        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = 5;
        prm.pbuf = clbWarPrompt(Context.snum, twar);
        prm.index = MSG_LIN1;
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                     NoColor);
        prompting = true;

        break;
    case '-':				/* shields down */
        _doshields( Context.snum, false );
        break;
    case '+':				/* shields up */
        _doshields( Context.snum, true );
        break;
    case '/':				/* player list */
        setONode(nShiplInit(DSP_NODE_CP, false));  /* shipl node */
        break;

    case '[':                   /* zoom out */
    {
        bool doEffect = false;
        if (SMAP(Context.snum))
        {                     /* LR */
            if (ncpLRMagFactor - 1 >= -5)
            {
                ncpLRMagFactor--;
                doEffect = true;
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpLRMagFactor, true);
        }
        else
        {
            if (ncpSRMagFactor - 1 >= -5)
            {
                ncpSRMagFactor--;
                doEffect = true;
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpSRMagFactor, false);
        }

        if (doEffect)
        {
            // small probability to play this effect
            if (rnd() < 0.1)
                cqsEffectPlay(cqsTeamEffects[cbShips[snum].team].mag, NULL,
                              0, 0, 0);
        }
    }
    break;

    case ']':                   /* zoom in */
    {
        bool doEffect = false;
        if (SMAP(Context.snum))
        {                     /* LR */
            if (ncpLRMagFactor + 1 <= 5)
            {
                ncpLRMagFactor++;
                doEffect = true;
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpLRMagFactor, true);
        }
        else
        {
            if (ncpSRMagFactor + 1 <= 5)
            {
                ncpSRMagFactor++;
                doEffect = true;
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpSRMagFactor, false);
        }

        if (doEffect)
        {
            // small probability to play this effect
            if (rnd() < 0.1)
                cqsEffectPlay(cqsTeamEffects[cbShips[snum].team].mag, NULL,
                              0, 0, 0);
        }
    }
    break;

    case '?':				/* planet list */
        if (Context.snum >= 0 && Context.snum < cbLimits.maxShips())
            setONode(nPlanetlInit(DSP_NODE_CP, false, Context.snum, cbShips[Context.snum].team));
        else          /* then use user team if user doen't have a ship yet */
            setONode(nPlanetlInit(DSP_NODE_CP, false, Context.snum, cbUsers[Context.unum].team));
        break;
    case TERM_REDRAW:			/* clear all the prompts */
        hudClearPrompt(MSG_LIN1);
        hudClearPrompt(MSG_LIN2);
        hudClearPrompt(MSG_MSG);

        /* reset the scaling factors */
        ncpLRMagFactor = ncpSRMagFactor = 0;
        setViewerScaling(ncpLRMagFactor, true);
        setViewerScaling(ncpSRMagFactor, false);

        break;

    case TERM_NORMAL:		/* Have [ENTER] act like 'I[ENTER]'  */
    case '\n':
        _doinfo("", TERM_NORMAL, true);
        break;

        /* ack red alert by turning klaxon off  Cataboligne -
           sound code 11.14.6 */
    case TERM_ABORT:
        if (alertHandle != CQS_INVHANDLE)
        {
            cqsEffectStop(alertHandle, false);
            alertHandle = CQS_INVHANDLE;
        }
        break;

    case ' ':
        if (SMAP(snum))
            UserConf.DoLocalLRScan = !UserConf.DoLocalLRScan;
        break;

    case TERM_EXTRA:		/* Have [TAB] act like 'i\t' */
        _doinfo("", TERM_EXTRA, true);
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
        uiPutMsg( "Type h for help.", MSG_LIN2 );
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
    static real odist = std::max(BATTLE_MAXDIS, APPROACH_MAXDIS);
    int snum = Context.snum;
    int warlike;
    real prob = rnd();
    int i;

    if (!cqsSoundAvailable || cqsMusicPlaying())
        return;

    /* go through each ship. */

    warlike = false;
    for ( i = 0; i < cbLimits.maxShips(); i++ )
    {
        int atwar = satwar(snum, i);

        if (i == snum)
            continue;               /* don't care about our ship */

        if ( cbShips[i].status != SS_LIVE )
            continue;               /* don't care about the non-living */

        /* check range */
        dis = distf( cbShips[snum].x, cbShips[snum].y,
                     cbShips[i].x, cbShips[i].y );

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
            warlike = true;
            mus = cqsTeamMusic[cbShips[i].team].battle;
        }
        else if (!warlike /*&& cbShips[Context.snum].team == cbShips[i].team*/)
            mus = cqsTeamMusic[cbShips[i].team].approach;
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
        cqsMusicPlay(mus, false);
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
        mus = cqsTeamMusic[rndint(0, NUM_PLAYERTEAMS - 1)].theme;
    else
        mus = cqsTeamMusic[rndint(0, NUM_PLAYERTEAMS - 1)].intro;

    /* now play it */
    cqsMusicPlay(mus, false);

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
    real dis = dist( cbShips[Context.snum].x, cbShips[Context.snum].y,
                     cbDoomsday->x, cbDoomsday->y );
    static int first_doom = 0;

    if (DOOM_LIVE())
        return;

    /* doomsday music theme for ships nearby */

    /* first sight of doomsday - wait for other themes */
    if (first_doom < 2 && dis < DOOM_INTRODIS && !cqsMusicPlaying())
    {
        cqsMusicPlay(cqsDoomsdayMusic.doomin, false);
        first_doom = 2;                                                                     /* intro music only plays once */
    }
    else if (cbShips[Context.snum].sdfuse > 0 &&
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
        cqsMusicPlay(cqsDoomsdayMusic.doomkill, false);
        first_doom = 3;   /* dont restart playing this */
    }
    else if (dis < DOOM_MUSDIS && !cqsMusicPlaying())
    {
        cqsMusicPlay(cqsDoomsdayMusic.doom, false);
        first_doom = 4;   /* enable kill music if it played once already */
    }
}


void nCPInit(int istopnode)
{
    static int introsPlayed[NUM_PLAYERTEAMS];
    char buf[CQI_NAMELEN];

    cInfo.state = CLIENT_STATE_PLAY;
    prompting = false;
    state = S_NONE;
    clientStatLastFlags = 0;

    /* init timers */
    rftime = frameTime;
    lastblast = cbShips[Context.snum].lastblast;
    lastphase = cbShips[Context.snum].lastphase;
    pingPending = false;
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
    if (!nCPNode.animVec)
    {
        int i;

        // store it for the node renderer
        nCPNode.animVec = &animVec;
        // init
        animVec.clear();

        /* setup the team torp animators */
        for (i=0; i<NUM_PLAYERTEAMS; i++)
        {
            char nm[CQI_NAMELEN];

            snprintf(nm, CQI_NAMELEN, "ship%c-torp",
                     cbTeams[i].teamchar);

            if (!animInitState(nm, &ncpTorpAnims[i], NULL))
                utLog("%s: failed to init animstate for animation '%s'",
                      __FUNCTION__, nm);
            else
                animVec.push_back(&ncpTorpAnims[i]);
        }

        /* init the intro music array.  We want to ensure that we play
           a team-specific intro only once */
        for (i=0; i<NUM_PLAYERTEAMS; i++)
            introsPlayed[i] = false;

        bombingfx = cqsFindEffect("bombing");
    }

    /* only if we are running this as a topnode frpm nPlay
       do we want to do this */
    if (istopnode && !introsPlayed[cbShips[Context.snum].team])
    {
        introsPlayed[cbShips[Context.snum].team] = true;
        snprintf(buf, CQI_NAMELEN, "ship%c-intro",
                 cbTeams[cbShips[Context.snum].team].teamchar);
        cqsMusicPlay(cqsFindMusic(buf), false);
    }
    else if (istopnode)
    {                           /* play the theme music if
                                   we've already done the intro music
                                   for this team. */
        snprintf(buf, CQI_NAMELEN, "ship%c-theme",
                 cbTeams[cbShips[Context.snum].team].teamchar);
        cqsMusicPlay(cqsFindMusic(buf), false);
    }
    /* else, don't start playing anything */

    setNode(&nCPNode);

    /* init this so the warp effects don't kick in incorrectly */
    if (istopnode)
        cbShips[Context.snum].dwarp = -1;

    doReload = true;

    return;
}


static nodeStatus_t nCPDisplay(dspConfig_t *dsp)
{
    /* Viewer */
    renderViewer(UserConf.doVBG);

    /* Main/Hud */
    renderHud(_dostats);

    /* draw the overlay bg if active */
    mglOverlayQuad();

    return NODE_OK;
}

static nodeStatus_t nCPIdle(void)
{
    uint64_t now;
    uint64_t difftime = utDeltaGrand( Context.msgrand, &now );
    static uint32_t iterstart = 0;
    static uint32_t themetime = 0;
    static uint32_t dietime = 0;
    static uint32_t tadTime = 0;
    uint32_t iternow = cInfo.nodeMillis;
    static const uint32_t iterwait = 50;   /* ms */
    static const uint32_t themewait = 5000; /* ms (5 seconds) */
    static const uint32_t dyingwait = 5000; /* watching yourself die */
    static const uint32_t tadWait = 1000; /* update the TAD every second */
    real tdelta = (real)iternow - (real)iterstart;

    // we only want to do this once after entry, *after* all
    // initialization will have been completed.
    if (doReload)
    {
        doReload = false;
        sendCommand(CPCMD_RELOAD, 0);
    }


    if (state == S_DEAD)
    {                           /* die for awhile */
        /* also turns off warp/engine sounds */
        /* nDead will stop all other sound effects for us on exit */
        cqsSetWarp(0);
        state = S_GHOST;
        /* start a timer so you can see yourself die :) */
        dietime = iternow;
    }

    /* drive the local universe */
    if (tdelta > iterwait)
    {
        clbPlanetDrive(tdelta / 1000.0);
        clbTorpDrive(tdelta / 1000.0);
        iterstart = iternow;
    }

    /* if we are a ghost then we already know we are dead. */
    if ( (clientStatLastFlags & SPCLNTSTAT_FLAG_KILLED) &&
         state != S_GHOST && state != S_GHOSTING )
    {                           /* we died.  set the state and deal with
                                   it on the next frame */
        state = S_DEAD;
        prompting = false;        /* doesn't really matter */
        return NODE_OK;
    }

    if (state == S_BOMBING && clientLastServerAckCode)
    {                           /* the server stopped bombing for us */
        cqsEffectStop(bombingHandle, false);
        sendCommand(CPCMD_BOMB, 0); /* to be sure */
        state = S_NONE;
        prompting = false;
        hudClearPrompt(MSG_LIN2);
        return NODE_OK;           /* next iter will process the char */
    }

    if (state == S_BEAMING && clientLastServerAckCode)
    {                           /* the server stopped beaming for us */
        cqsEffectStop(beamHandle, false);
        sendCommand(CPCMD_BEAM, 0); /* to be sure */
        state = S_NONE;
        prompting = false;
        hudClearPrompt(MSG_LIN2);
        return NODE_OK;           /* next iter will process the char */
    }

    if (state == S_REFITING)
    {                           /* done refiting? */
        if ((cInfo.nodeMillis - entertime) >= REFIT_GRAND)
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
        if ((cInfo.nodeMillis - entertime) >= REARM_GRAND)
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
            if ( utGetMsg(Context.snum, &cbShips[Context.snum].lastmsg))
            {
                rmesg(Context.snum, cbShips[Context.snum].lastmsg, MSG_MSG);
                // only beep if the message wasn't from us...
                if (!(cbMsgs[cbShips[Context.snum].lastmsg].from == MSG_FROM_SHIP
                      && (int)cbMsgs[cbShips[Context.snum].lastmsg].fromDetail == Context.snum) )
                {
                    if (UserConf.MessageBell)
                        mglBeep(MGL_BEEP_MSG);
                }

                Context.msgrand = now;
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
        cqsEffectStop(bombingHandle, false);
        cqsEffectStop(beamHandle, false);
        cqsEffectStop(alertHandle, false);

        beamHandle = CQS_INVHANDLE;
        bombingHandle = CQS_INVHANDLE;
        alertHandle = CQS_INVHANDLE;

        /* in nDead */
        state = S_GHOSTING;

        nDeadInit();
    }

    // if we have valid data, update the TAD every second
    if (UserConf.hudInfo && !prompting && ((iternow - tadTime) > tadWait))
    {
        tadTime = iternow;
        if (!Context.lastInfoStr.empty())
            _doinfo("", 0, false);
    }

    return NODE_OK;
}

static nodeStatus_t nCPInput(int ch)
{
    int cf = ch;                      /* backup of ch for domacros() */
    int irv, tmsg, i;
    real tdir;
    real x;
    int snum = Context.snum;

    // CTRL-B, background star texture
    if (CQ_CHAR(ch) == 0x02)
    {
        UserConf.doVBG = !UserConf.doVBG;
        return NODE_OK;
    }

    /* CTRL-S, display render/comm stats */
    if (CQ_CHAR(ch) == 0x13)
    {
        _dostats = !_dostats;
        return NODE_OK;
    }

    /* CTRL-G, toggle tac grid on/off */
    if (CQ_CHAR(ch) == 0x07)
    {
        UserConf.DoTacBkg = !UserConf.DoTacBkg;
        return NODE_OK;
    }

    ch = CQ_CHAR(ch) | CQ_FKEY(ch);

    /* Control-/ (INSTA-QUIT (tm)) */
    if (ch == 0x1c)
    {
        /* first stop all music and effects */
        cqsEffectStop(CQS_INVHANDLE, true);
        cqsMusicStop(true);

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
        cqsEffectStop(bombingHandle, false);
        sendCommand(CPCMD_BOMB, 0);
        state = S_NONE;
        prompting = false;
        uiPutMsg( abt, MSG_LIN1 );
        hudClearPrompt(MSG_LIN2);
        return NODE_OK;           /* next iter will process the char */
    }

    if (state == S_BEAMING && ch)
    {                           /* aborting */
        ibufPutc(ch);             /* just que it */
        cqsEffectStop(beamHandle, false);
        sendCommand(CPCMD_BEAM, 0);
        state = S_NONE;
        prompting = false;
        uiPutMsg( abt, MSG_LIN1 );
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
        irv = prmProcInput(prm, ch);

        switch (state)
        {
        case S_COURSE:
            if (irv > 0)
            {
                _docourse(prm.buf, ch);
                prompting = false;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

            break;

        case S_DOINFO:
            if (irv > 0)
            {
                _doinfo(prm.buf.c_str(), ch, true);
                prompting = false;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

            break;

        case S_TARGET:
            if (irv > 0)
            {
                if (_gettarget(prm.buf,
                               (desttarg == T_PHASER) ? lastphase : lastblast,
                               &tdir, ch))
                {
                    // We set them both here so the last thing fired
                    // sets the direction for future firing by default
                    lastphase = tdir;
                    lastblast = tdir;
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
                    uiPutMsg( "Invalid targeting information.", MSG_LIN1 );

                prompting = false;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

            break;

        case  S_CLOAK:
            if (irv > 0)
            {
                _docloak(ch);
                prompting = false;
                state = S_NONE;
            }

            break;

        case  S_ALLOC:
            if (irv > 0)
            {
                _doalloc(prm.buf.c_str(), ch);
                prompting = false;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

            break;

        case  S_DISTRESS:
            if (irv > 0)
            {
                _dodistress(prm.buf.c_str(), ch);
                prompting = false;
                state = S_NONE;
            }

            break;

        case  S_COUP:
            if (irv > 0)
            {
                if (ch == TERM_EXTRA)
                {
                    uiPutMsg( "Attempting coup...", MSG_LIN1 );
                    sendCommand(CPCMD_COUP, 0);
                }
                else
                    uiPutMsg( abt, MSG_LIN1 );

                prompting = false;
                state = S_NONE;
            }

            break;

        case  S_TOW:
            if (irv > 0)
            {
                _dotow(prm.buf.c_str(), ch);
                prompting = false;
                state = S_NONE;
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

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
                    prompting = false;
                    state = S_NONE;
                }
            }

            break;

        case S_AUTOPILOT:
            if (ch == TERM_ABORT)
            {
                sendCommand(CPCMD_AUTOPILOT, 0);
                hudClearPrompt(MSG_LIN1);
                prompting = false;
                state = S_NONE;
            }
            else
                uiPutMsg("Press [ESC] to abort autopilot.", MSG_LIN1);

            break;

        case  S_MSGTO:
            if (irv > 0)
                _domsgto(prm.buf, ch,  UserConf.Terse); /* will set state appropriately */
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

            break;

        case  S_MSG:
            if (irv != 0)
                _domsg(prm.buf, ch, irv); /* will set state appropriately */
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

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
                prompting = false;
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
            }

            break;

        case S_DESTRUCTING:
            if (ch == TERM_ABORT)
            {
                sendCommand(CPCMD_DESTRUCT, 0); /* just kidding */
                state = S_NONE;
                prompting = false;
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
                uiPutMsg( "Self destruct has been cancelled.", MSG_LIN1 );
            }
            else
            {                   /* chicken */
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
                prm.buf.clear();
                uiPutMsg( "Press [ESC] to abort self destruct.", MSG_LIN1 );
                mglBeep(MGL_BEEP_ERR);
            }

            break;

        case S_BOMB:
            if (ch == TERM_EXTRA)
            {
                sendCommand(CPCMD_BOMB, 1);   /* start the bombing */
                state = S_BOMBING;
                prompting = false;

                cqsEffectPlay(bombingfx, &bombingHandle, 0.0, 0.0, 0.0);
                hudClearPrompt(MSG_LIN1);
            }
            else
            {                   /* weak human */
                state = S_NONE;
                prompting = false;
                uiPutMsg( abt, MSG_LIN1 );
                hudClearPrompt(MSG_LIN2);
            }

            break;

        case S_BEAMDIR:
            switch (ch)
            {
            case 'u':
            case 'U':
                dirup = true;
                break;
            case 'd':
            case 'D':
            case TERM_EXTRA:
                dirup = false;
                break;
            default:
                uiPutMsg( abt, MSG_LIN1 );
                state = S_NONE;
                prompting = false;
                return NODE_OK;
            }

            /* time to ask how many */
            if ( dirup )
                beamax = upmax;
            else
                beamax = downmax;

            state = S_BEAMNUM;
            prm.preinit = false;
            prm.buf.clear();
            prm.buflen = 10;
            prm.pbuf = fmt::format("Beam {} [1-{}] ",
                                   (dirup) ? "up" : "down",
                                   beamax );
            prm.index = MSG_LIN1;
            hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf,
                         NoColor);
            prompting = true;

            break;


        case S_BEAMNUM:
            if (irv > 0)
                _dobeam(prm.buf, ch); /* will set state appropriately */
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

            break;


        case S_WAR:
            if (irv > 0)
            {
                if (ch == TERM_ABORT || ch == TERM_NORMAL)
                {
                    state = S_NONE;
                    prompting = false;
                    hudClearPrompt(MSG_LIN1);
                    hudClearPrompt(MSG_LIN2);
                    return NODE_OK;
                }

                if (ch == TERM_EXTRA) /* accepted */
                {
                    int dowait = false;
                    uint16_t cwar;

                    cwar = 0;
                    for ( i = 0; i < NUM_PLAYERTEAMS; i = i + 1 )
                    {
                        if ( twar[i] && ! cbShips[snum].war[i] )
                            dowait = true;

                        if (twar[i])
                            cwar |= (1 << i);

                        /* we'll let it happen locally as well... */
                        cbUsers[cbShips[Context.snum].unum].war[i] = twar[i];
                        cbShips[Context.snum].war[i] = twar[i];
                    }

                    hudClearPrompt(MSG_LIN1);
                    hudClearPrompt(MSG_LIN2);

                    if (dowait)
                    {
                        state = S_WARRING;
                        prompting = false;
                        uiPutMsg(
                            "Reprogramming the battle computer, please stand by...",
                            MSG_LIN2 );

                        entertime = cInfo.nodeMillis; /* gotta wait */
                    }
                    else
                    {
                        state = S_NONE;
                        prompting = false;
                    }

                    sendCommand(CPCMD_SETWAR, (uint16_t)cwar);
                    return NODE_OK;
                }

            }
            else
            {
                prm.buf.clear();
                for ( i = 0; i < NUM_PLAYERTEAMS; i++ )
                {
                    if ( (sStat.serverFlags & SERVER_F_NOTEAMWAR)
                         && i == cbShips[Context.snum].team
                         && ch == (char)tolower(cbTeams[i].teamchar))
                    {
                        // not allowed, make sure war status is
                        // cleared, then beep
                        cbShips[Context.snum].war[cbShips[Context.snum].team] = false;
                        mglBeep(MGL_BEEP_ERR);
                    }
                    else
                    {
                        if ( ch == (char)tolower( cbTeams[i].teamchar ) )
                        {
                            if ( ! twar[i] || ! cbShips[Context.snum].rwar[i] )
                                twar[i] = ! twar[i];
                            prm.pbuf = clbWarPrompt(Context.snum, twar);
                            hudSetPrompt(prm.index, prm.pbuf,
                                         NoColor, prm.buf, NoColor);
                        }
                    }
                }
            }

            break;

        case S_PSEUDO:
            if (irv > 0)
            {
                if (ch != TERM_ABORT && !prm.buf.empty())
                    sendSetName(prm.buf.c_str());
                prompting = false;
                state = S_NONE;
                hudClearPrompt(MSG_LIN1);
                hudClearPrompt(MSG_LIN2);
            }
            else
                hudSetPrompt(prm.index, prm.pbuf, NoColor,
                             prm.buf, CyanColor);

            break;

        case  S_REFIT:
            if (irv > 0)
            {
                switch (ch)
                {
                case TERM_ABORT: /* cancelled */
                    state = S_NONE;
                    prompting = false;
                    hudClearPrompt(MSG_LIN1);
                    hudClearPrompt(MSG_LIN2);

                    break;

                case TERM_NORMAL:
                    hudClearPrompt(MSG_LIN1);
                    hudClearPrompt(MSG_LIN2);
                    prm.pbuf =
                        fmt::format("Refitting ship. This will take {} "
                                    "seconds to complete...",
                                    REFIT_GRAND/1000);
                    hudSetPrompt(prm.index, prm.pbuf, NoColor,
                                 prm.buf, NoColor);
                    sendCommand(CPCMD_REFIT, (uint16_t)refitst);
                    prompting = false;
                    entertime = cInfo.nodeMillis;
                    state = S_REFITING;

                    break;

                case TERM_EXTRA:
                    refitst = mod( refitst + 1, MAXNUMSHIPTYPES );
                    prm.pbuf = fmt::format("Refit ship type: {}",
                                           cbShipTypes[refitst].name);
                    prm.buf.clear();
                    hudSetPrompt(prm.index, prm.pbuf, NoColor,
                                 prm.buf, NoColor);

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
                tmsg = msg - 1;
                if (tmsg < 0)
                    tmsg = cbLimits.maxMsgs() - 1;
                while(!clbCanRead( snum, tmsg ) && tmsg != lastone)
		{
                    tmsg--;
                    if (tmsg < 0)
                        tmsg = cbLimits.maxMsgs() - 1;
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
                tmsg =  mod( msg + 1, cbLimits.maxMsgs() );
                while(!clbCanRead( snum, tmsg ) && tmsg != lstmsg + 1 )
		{
                    tmsg = mod( tmsg + 1, cbLimits.maxMsgs() );
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

static nodeStatus_t nCPMInput(mouseData_t *mdata)
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
                cqsEffectStop(bombingHandle, false);
                sendCommand(CPCMD_BOMB, 0); /* to be sure */
                state = S_NONE;
                prompting = false;
                hudClearPrompt(MSG_LIN2);
            }

            if (state == S_BEAMING)
            {
                cqsEffectStop(beamHandle, false);
                sendCommand(CPCMD_BEAM, 0); /* to be sure */
                state = S_NONE;
                prompting = false;
                hudClearPrompt(MSG_LIN2);
            }
        }
    }

    return NODE_OK;
}
