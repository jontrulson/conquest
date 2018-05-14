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
#include "format.h"

#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"
#include "record.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"
#include "cprintf.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nPlayB.h"
#include "nPlayBMenu.h"
#include "nShipl.h"

static int shipOffset; /* offset into cbLimits.maxShips() for this page */
static int shipIdx;

static nodeStatus_t nShiplDisplay(dspConfig_t *);
static nodeStatus_t nShiplIdle(void);
static nodeStatus_t nShiplInput(int ch);

static scrNode_t nShiplNode = {
    nShiplDisplay,               /* display */
    nShiplIdle,                  /* idle */
    nShiplInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

static int retnode;             /* the node to return to */

scrNode_t *nShiplInit(int nodeid, int setnode)
{
    retnode = nodeid;

    shipOffset = 0;
    shipIdx = 0;

    if (setnode)
        setNode(&nShiplNode);

    return(&nShiplNode);
}


static nodeStatus_t nShiplDisplay(dspConfig_t *dsp)
{
    int snum = Context.snum;
    std::string cbuf;
    int lin, col;

    clbShipline(SHIPLINE_HDR, false, cbuf);

    lin = 2;
    col = (int)(Context.maxcol - cbuf.size()) / 2;
    cprintf(lin, col, ALIGN_NONE, "#%d#%s", LabelColor, cbuf.c_str());

    for ( int i=0; i < cbuf.size(); i++ )
        if ( cbuf[i] != ' ' )
            cbuf[i] = '-';
    lin++;
    cprintf(lin, col, ALIGN_NONE, "#%d#%s", LabelColor, cbuf.c_str());

    lin++; // first line to use
    shipIdx = 0;
    if ( shipOffset < cbLimits.maxShips() )
    {
        while ((shipOffset + shipIdx) < cbLimits.maxShips())
        {
            int i = shipOffset + shipIdx;
            shipIdx++;

            if ( cbShips[i].status != SS_LIVE )
                continue;

            clbShipline(i, false, cbuf);
            int color;
            if (snum >= 0 && snum < cbLimits.maxShips() )
            {		/* a normal ship view */
                if ( i == snum )    /* it's ours */
                    color = NoColor | CQC_A_BOLD;
                else if (satwar(i, snum)) /* we're at war with it */
                    color = RedLevelColor;
                else if (cbShips[i].team == cbShips[snum].team
                         && !selfwar(snum))
                    color = GreenLevelColor; /* it's a team ship */
                else
                    color = YellowLevelColor;
            }
            else
            { /* not conqoper, and not a valid ship (main menu) */
                if (cbUsers[Context.unum].war[cbShips[i].team])
                    color = RedLevelColor; // war
                else if (cbUsers[Context.unum].team == cbShips[i].team)
                    color = GreenLevelColor; // it's a team ship
                else
                    color = YellowLevelColor; // meh
            }

            cprintf(lin, col, ALIGN_NONE, "#%d#%s", color, cbuf.c_str());

            lin++;

            if (lin == MSG_LIN1)
                break; // out of while

        } // while

        if ((shipOffset + shipIdx) >= cbLimits.maxShips())
            cprintf(MSG_LIN2, 0,  ALIGN_CENTER, "#%d#%s", NoColor, MTXT_DONE);
        else
            cprintf(MSG_LIN2, 0,  ALIGN_CENTER, "#%d#%s", NoColor, MTXT_MORE);
    } // if

    return NODE_OK;
}

static nodeStatus_t nShiplIdle(void)
{
    if (Context.recmode == RECMODE_PLAYING || Context.recmode == RECMODE_PAUSED)
        return NODE_OK;             /* no packet reading here */

    if (clientStatLastFlags & SPCLNTSTAT_FLAG_KILLED && retnode == DSP_NODE_CP)
    {
        /* time to die properly. */
        setONode(NULL);
        nDeadInit();
        return NODE_OK;
    }


    return NODE_OK;
}

static nodeStatus_t nShiplInput(int ch)
{
    ch = CQ_CHAR(ch);

    if (ch == ' ')
    {
        shipOffset += shipIdx;

        if (shipOffset < cbLimits.maxShips())
            return NODE_OK;
    }
    else if (ch == TERM_EXTRA) // go back to first page
    {
        shipOffset = 0;
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

    case DSP_NODE_PLAYBMENU:
        setONode(NULL);
        nPlayBMenuInit();
        break;

    case DSP_NODE_PLAYB:
        setONode(NULL);
        nPlayBInit();
        break;

    default:
        utLog("nShiplInput: invalid return node: %d, going to DSP_NODE_MENU",
              retnode);
        setONode(NULL);
        nMenuInit();
        break;
    }

    /* NOTREACHED */
    return NODE_OK;
}
