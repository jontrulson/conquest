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

#include <string>
#include "fmt/format.h"

#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "gldisplay.h"
#include "client.h"
#include "node.h"
#include "ship.h"
#include "prm.h"
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"
#include "nDead.h"
#include "nMenu.h"
#include "nPlay.h"
#include "cqkeys.h"
#include "cqsound.h"
#include "glmisc.h"
#include "cprintf.h"

static int snum;
static killedBy_t kb;                  /* killed by... */
static uint16_t detail;
static const char *ywkb="You were killed by ";
static Ship_t eShip = {}; /* copy of killers ship, if killed by ship */

#define S_PRESSANY  0           /* press any key... */
#define S_LASTWORDS 1           /* you won, get conquer msg */
#define S_LASTWCONF 2           /* confirm last words */
static int state;

static prm_t prm;

static nodeStatus_t nDeadDisplay(dspConfig_t *);
static nodeStatus_t nDeadIdle(void);
static nodeStatus_t nDeadInput(int ch);

static scrNode_t nDeadNode = {
    nDeadDisplay,               /* display */
    nDeadIdle,                  /* idle */
    nDeadInput,                 /* input */
    NULL,                       /* minput */
    NULL                        /* animVec */
};


void nDeadInit(void)
{
    state = S_PRESSANY;
    snum = Context.snum;

    /* If something is wrong, don't do anything. */
    if ( snum < 0 || snum >= cbLimits.maxShips() )
    {
        utLog("nDead: nDeadInit: snum < 0 || snum >= cbLimits.maxShips() (%d)", snum);
        nMenuInit();
    }

    kb = cbShips[snum].killedBy;
    detail = cbShips[snum].killedByDetail;

    if (kb == KB_SHIP && detail < cbLimits.maxShips())
        eShip = cbShips[detail];        /* get copy of killers ship */
    else
        memset((void *)&eShip, 0, sizeof(Ship_t));

    if (clientStatLastFlags & SPCLNTSTAT_FLAG_CONQUER)
    {
        state = S_LASTWORDS;

        prm.preinit = false;
        prm.buf.clear();
        prm.buflen = MAXLASTWORDS;
    }

    /* turn off any running effects */
    cqsEffectStop(CQS_INVHANDLE, true);

    setONode(&nDeadNode);

    return;
}


static nodeStatus_t nDeadDisplay(dspConfig_t *dsp)
{
    int i;

    /* Figure out why we died. */
    switch ( kb )
    {
        case KB_SELF:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "You scuttled yourself.");

            break;
        case KB_NEGENB:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "You were destroyed by the negative energy barrier.");

            break;
        case KB_CONQUER:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "Y O U   C O N Q U E R E D   T H E   U N I V E R S E ! ! !");
            break;
        case KB_NEWGAME:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "N E W   G A M E !");
            break;
        case KB_EVICT:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "Closed for repairs.");
            break;
        case KB_SHIT:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "You are no longer allowed to play.");
            break;
        case KB_GOD:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "You were killed by an act of GOD.");

            break;
        case KB_DOOMSDAY:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "You were eaten by the doomsday machine.");

            break;
        case KB_GOTDOOMSDAY:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "You destroyed the doomsday machine!");
            break;
        case KB_LIGHTNING:
            cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor,
                    "You were destroyed by a lightning bolt.");

            break;
        case KB_SHIP:
            if ( detail < cbLimits.maxShips() )
            {
                std::string shipBuf = utShipStr((int)detail);
                cprintf( 8,0,ALIGN_CENTER,
                         "#%d#You were kill number #%d#%.1f #%d#for "
                         "#%d#%s #%d#(#%d#%s#%d#)%s",
                         InfoColor, CQC_A_BOLD, eShip.kills,
                         InfoColor, CQC_A_BOLD, eShip.alias,
                         InfoColor, CQC_A_BOLD, shipBuf.c_str(),
                         InfoColor,
                         ((eShip.status != SS_LIVE) ? ", who also died." : "."));
            }

            break;
        case KB_PLANET:
            if ( detail < cbLimits.maxPlanets() )
            {
                std::string buf;

                if ( cbPlanets[detail].type == PLANET_SUN )
                    buf = "solar radiation.";
                else
                    buf = "planetary defenses.";

                cprintf(8,0,ALIGN_CENTER,"#%d#%s#%d#%s%s#%d#%s",
                        InfoColor, ywkb, CQC_A_BOLD, cbPlanets[detail].name,
                        "'s ",
                        InfoColor, buf.c_str());
            }

            break;

        default:
        {
            /* We were unable to determine the cause of death. */
            std::string buf =
                fmt::format("dead (unknown cause): {} was killed by {}({}).",
                            utShipStr(snum),
                            (int)kb, (int)detail);

            utLog("%s: %s", __FUNCTION__, buf.c_str());

            cprintf(8, 0, ALIGN_CENTER, "#%d#%s%s",
                    RedLevelColor, ywkb,
                    "nothing in particular.  (How strange...)");
        }
    }

    if ( kb == KB_NEWGAME )
    {
        cprintf( 10,0,ALIGN_CENTER,
                 "#%d#Universe conquered by #%d#%s #%d#for the #%d#%s #%d#team.",
		 InfoColor, CQC_A_BOLD, cbConqInfo->conqueror,
		 InfoColor, CQC_A_BOLD, cbConqInfo->conqteam, LabelColor );
    }
    else if ( kb == KB_SELF )
    {
        i = cbShips[snum].armies;
        if ( i > 0 )
	{
            cprintf(10, 0, ALIGN_CENTER,
                    "#%d#The #%d#%s #%d#you were carrying %s not amused.",
                    LabelColor, CQC_A_BOLD,
                    ((i == 1)? "army" : "armies"),
                    LabelColor,
                    ((i == 1)? "was" : "were"));
	}
    }
    else if ( kb == KB_SHIP )
    {
        if ( eShip.status == SS_LIVE )
	{
            cprintf( 10,0,ALIGN_CENTER,
                     "#%d#He had #%d#%d%% #%d#shields and #%d#%d%% #%d#damage.",
                     InfoColor, CQC_A_BOLD, iround(eShip.shields),
                     InfoColor, CQC_A_BOLD, iround(eShip.damage),InfoColor );
	}
    }

    cprintf(12,0,ALIGN_CENTER,
            "#%d#You got #%d#%.1f #%d#this time.",
            InfoColor, CQC_A_BOLD, oneplace(cbShips[snum].kills), InfoColor );

    if (state == S_PRESSANY)
        cprintf(MSG_LIN1, 0, ALIGN_CENTER,
                "[ESC] for Main Menu, [TAB] to re-enter the game.");
    else
    {
        if (state == S_LASTWORDS)
        {
            cprintf(14, 0, ALIGN_LEFT, "#%d#Any last words? #%d#%s",
                    CyanColor, NoColor, prm.buf.c_str());
        }
        if (state == S_LASTWCONF)
        {
            if (!prm.buf.empty())
            {
                cprintf( 13,0,ALIGN_CENTER, "#%d#%s",
                         InfoColor, "Your last words are entered as:");
                cprintf( 14,0,ALIGN_CENTER, "#%d#%c%s%c",
                         YellowLevelColor, '"', prm.buf.c_str(), '"' );
            }
            else
                cprintf( 14,0,ALIGN_CENTER,"#%d#%s", InfoColor,
                         "You have chosen to NOT leave any last words:" );

            cprintf( 16,0,ALIGN_CENTER, "Press [TAB] to confirm");
        }
    }

    return NODE_OK;
}

static nodeStatus_t nDeadIdle(void)
{
    return NODE_OK;
}

static nodeStatus_t nDeadInput(int ch)
{
    int irv;

    ch = CQ_CHAR(ch);

    switch (state)
    {
    case S_PRESSANY:
        if (ch == TERM_ABORT || ch == TERM_EXTRA) /* ESC or TAB */
        {
            setONode(NULL);

            switch (ch)
            {
            case TERM_ABORT:    /* ESC */
                nMenuInit();      /* go to menu */
                return NODE_OK;
            case TERM_EXTRA:
                /* since the server is already in the 'menu' waiting for us
                 * just go for it.
                 */
                nPlayInit();
                return NODE_OK;
            }
        }
        else
            mglBeep(MGL_BEEP_ERR);

        break;
    case S_LASTWORDS:
        irv = prmProcInput(prm, ch);
        if (irv > 0)
            state = S_LASTWCONF;

        break;

    case S_LASTWCONF:
        if (ch == TERM_EXTRA)
        {                       /* we are done */
            sendMessage(MSG_TO_GOD, 0, prm.buf.c_str());
            setONode(NULL);
            nMenuInit();
        }
        else
        {                       /* back to the drawing board */
            state = S_LASTWORDS;
            prm.buf.clear();
        }

        break;
    }

    return NODE_OK;
}
