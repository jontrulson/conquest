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
#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "cb.h"
#include "conqlb.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"
#include "conqutil.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nPlanetl.h"
#include "cqkeys.h"

static int team, snum;

static int PlanetOffset;    /* offset into cbLimits.maxPlanets() for this page */
static int PlanetIdx;
static const char *hd0="P L A N E T   L I S T   ";
static const char *hd1="' = must take to conquer the Universe)";
static const char *hd2="planet      type team armies          planet      type team armies";
static char hd3[BUFFER_SIZE_256];

static nodeStatus_t nPlanetlDisplay(dspConfig_t *);
static nodeStatus_t nPlanetlIdle(void);
static nodeStatus_t nPlanetlInput(int ch);

static scrNode_t nPlanetlNode = {
    nPlanetlDisplay,               /* display */
    nPlanetlIdle,                  /* idle */
    nPlanetlInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

static int retnode;             /* the node to return to */

scrNode_t *nPlanetlInit(int nodeid, int setnode, int sn, int tn)
{
    retnode = nodeid;

    PlanetOffset = 0;
    PlanetIdx = 0;
    snum = sn;
    team = tn;

    if (setnode)
        setNode(&nPlanetlNode);

    return(&nPlanetlNode);
}


static nodeStatus_t nPlanetlDisplay(dspConfig_t *dsp)
{
    int i, lin, col, olin, pnum;
    int sv[cbLimits.maxPlanets()];
    char ch, junk[10];
    int outattr;
    int col2;
    const int column_h = 7;
    const int column_1 = 3;
    const int column_2 = 41;
    char xbuf[BUFFER_SIZE_256];
    static char pd0[BUFFER_SIZE_1024];
    static bool FirstTime = true;

    if (FirstTime)
    {
        FirstTime = false;
        /* build header fmt string */
        sprintf(pd0,
                "#%d#%s#%d#%s#%d#%s#%d#%s" ,
                LabelColor,
                hd0,
                InfoColor,
                "('",
                SpecialColor,
                "+",
                InfoColor,
                hd1);

        strcpy( hd3, hd2 );

        for ( i = 0; hd3[i] != 0; i++ )
            if ( hd3[i] != ' ' )
                hd3[i] = '-';
    }

    /* sort the planets */
    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        sv[i] = i;
    clbSortPlanets( sv );

    lin = 1;

    cprintf(lin, column_h, ALIGN_NONE, "%s", pd0);

    /* display column headings */
    lin += 2;
    cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", LabelColor, hd2);
    lin++;
    cprintf(lin, 0,  ALIGN_CENTER, "#%d#%s", LabelColor, hd3);
    lin++;
    olin = lin;
    col = column_1;
    col2 = false;

    PlanetIdx = 0;
    if (PlanetOffset < cbLimits.maxPlanets())
    {
        while ((PlanetOffset + PlanetIdx) < cbLimits.maxPlanets())
        {
            i = PlanetOffset + PlanetIdx;
            PlanetIdx++;
            pnum = sv[i];

            /* colorize - dwp */
            if ( snum >= 0 && snum < cbLimits.maxShips())
            {	/* if user has a valid ship */
                if ( cbPlanets[pnum].team == cbShips[snum].team && !selfwar(snum) )
                    outattr = GreenLevelColor;
                else if ( (clbSPWar(snum,pnum) && cbPlanets[pnum].scanned[cbShips[snum].team] ) ||
                          cbPlanets[pnum].type == PLANET_SUN )
                    outattr = RedLevelColor;
                else
                    outattr = YellowLevelColor;
            }
            else
            {			/* else, user doesn't have a ship yet */
                if (team == TEAM_NOTEAM)
                {			/* via conqoper */
                    switch(cbPlanets[pnum].type)
                    {
                    case PLANET_SUN:
                        outattr = RedLevelColor;
                        break;
                    case PLANET_CLASSM:
                        outattr = GreenLevelColor;
                        break;
                    case PLANET_DEAD:
                        outattr = YellowLevelColor;
                        break;
                    case PLANET_CLASSA:
                    case PLANET_CLASSO:
                    case PLANET_CLASSZ:
                        outattr = CQC_A_BOLD;
                        break;
                    case PLANET_GHOST:
                        outattr = NoColor;
                        break;
                    default:
                        outattr = SpecialColor;
                        break;
                    }
                }
                else
                {			/* via menu() */
                    if ( cbPlanets[pnum].team == cbUsers[Context.unum].team &&
                         !(cbUsers[Context.unum].war[cbUsers[Context.unum].team]))
                    {
                        outattr = GreenLevelColor;
                    }
                    else if ( cbPlanets[pnum].type == PLANET_SUN ||
                              (cbPlanets[pnum].team < NUMPLAYERTEAMS &&
                               cbUsers[Context.unum].war[cbPlanets[pnum].team] &&
                               cbPlanets[pnum].scanned[cbUsers[Context.unum].team]) )
                    {
                        outattr = RedLevelColor;
                    }
                    else
                    {
                        outattr = YellowLevelColor;
                    }
                }
            }

            /* Don't display unless it's real. */
            if ( ! PVISIBLE(pnum) )
                continue;

            /* I want everything if it's real */

            /* Figure out who owns it and count armies. */
            ch =  cbTeams[cbPlanets[pnum].team].teamchar;
            sprintf( junk, "%d", cbPlanets[pnum].armies );

            /* Then modify based on scan information. */

            if ( team != TEAM_NOTEAM )
                if ( ! cbPlanets[pnum].scanned[team] )
                {
                    ch = '?';
                    strcpy(junk , "?") ;
                }

            /* Suns and moons are displayed as unowned. */
            if ( cbPlanets[pnum].type == PLANET_SUN || cbPlanets[pnum].type == PLANET_MOON )
                ch = ' ';

            /* Don't display armies for suns unless we're special. */
            if ( cbPlanets[pnum].type == PLANET_SUN )
                if ( team != TEAM_NOTEAM )
                    junk[0] = 0;

            /* Moons aren't supposed to have armies. */
            if ( cbPlanets[pnum].type == PLANET_MOON )
            {
                if ( team != TEAM_NOTEAM )
                    junk[0] = 0;
                else if ( cbPlanets[pnum].armies == 0 )
                    junk[0] = 0;
            }

            char coreflag = ' ';
            char homeflag = ' ';

            /* flag planets that are required for a conq */

            // check for homeplanet
            if (PHOMEPLANET(pnum))
                homeflag = cbTeams[cbPlanets[pnum].defendteam].name[0];

            // core planet - required for conquer
            if (PCORE(pnum))
                coreflag = '+';

            cprintf(lin, col,  ALIGN_NONE, "#%d#%c %c", SpecialColor,
                    homeflag, coreflag);
            col += 4;

            sprintf(xbuf,"%-11s ",cbPlanets[pnum].name);  /* cbPlanets[pnum].name */
            cprintf(lin, col,  ALIGN_NONE, "#%d#%s", outattr, xbuf);

            col+=(strlen(xbuf));
            sprintf( xbuf, "%-4c %-3c  ",
                     cbConqInfo->chrplanets[cbPlanets[pnum].type], ch);
            cprintf(lin, col,  ALIGN_NONE, "#%d#%s", outattr, xbuf);

            col+=(strlen(xbuf));
            sprintf(xbuf,"%4s",junk);
            cprintf(lin, col,  ALIGN_NONE, "#%d#%s",
                    (junk[0] == '?') ? YellowLevelColor : outattr,
                    xbuf);

            lin++;;
            if ( lin == MSG_LIN1 )
            {
                if (col2)	/* need a new page... */
                {
                    break; /* out of while */
                }
                else
                {
                    lin = olin;
                    col2 = true;
                }
            }

            if (!col2)
                col = column_1;
            else
                col = column_2;

        } /* while */

        if ((PlanetOffset + PlanetIdx) >= cbLimits.maxPlanets())
            cprintf(MSG_LIN2, 0,  ALIGN_CENTER, "#%d#%s", NoColor, MTXT_DONE);
        else
            cprintf(MSG_LIN2, 0,  ALIGN_CENTER, "#%d#%s", NoColor, MTXT_MORE);

    } /* if PlanetOffset <= cbLimits.maxPlanets() */

    return NODE_OK;
}

static nodeStatus_t nPlanetlIdle(void)
{
    if (clientStatLastFlags & SPCLNTSTAT_FLAG_KILLED && retnode == DSP_NODE_CP)
    {
        /* time to die properly. */
        setONode(NULL);
        nDeadInit();
        return NODE_OK;
    }


    return NODE_OK;
}

static nodeStatus_t nPlanetlInput(int ch)
{
    ch = CQ_CHAR(ch);

    if (ch == ' ')
    {
        PlanetOffset += PlanetIdx;

        if (PlanetOffset < cbLimits.maxPlanets())
            return NODE_OK;
    }

    /* go back */
    switch (retnode)
    {
    case DSP_NODE_CP:
        setONode(NULL);
        nCPInit(false);
        break;
    case DSP_NODE_MENU:
        setONode(NULL);
        nMenuInit();
        break;

    default:
        utLog("nPlanetlInput: invalid return node: %d, going to DSP_NODE_MENU",
              retnode);
        setONode(NULL);
        nMenuInit();
        break;
    }

    /* NOTREACHED */
    return NODE_OK;
}
