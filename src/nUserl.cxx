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
#include <vector>

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
#include "cprintf.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nUserl.h"
#include "cqkeys.h"

static int snum, godlike;
static int fuser;
static int offset;

static int extrast;             /* normal, or extra stats? */

// for the sorted list
static std::vector<int> uvec;


static nodeStatus_t nUserlDisplay(dspConfig_t *);
static nodeStatus_t nUserlIdle(void);
static nodeStatus_t nUserlInput(int ch);

static scrNode_t nUserlNode = {
    nUserlDisplay,               /* display */
    nUserlIdle,                  /* idle */
    nUserlInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animVec */
};

static int retnode;             /* the node to return to */

scrNode_t *nUserlInit(int nodeid, int setnode, int sn, int gl, int extra)
{
    int unum;

    retnode = nodeid;
    snum = sn;
    godlike = gl;
    extrast = extra;

    // init the user vector, free it if it already exists
    uvec.clear();

    /* sort the (living) user list */
    for ( unum = 0; unum < cbLimits.maxUsers(); unum++)
        if ( ULIVE(unum) )
        {
            uvec.push_back(unum);
        }
    clbSortUsers(uvec);

    fuser = 0;

    if (setnode)
        setNode(&nUserlNode);

    return(&nUserlNode);
}


static nodeStatus_t nUserlDisplay(dspConfig_t *dsp)
{
    int j, fline, lline, lin;
    static const std::string hd1="U S E R   L I S T";
    static const std::string ehd1="M O R E   U S E R   S T A T S";
    std::string cbuf;
    int color;

    /* Do some screen setup. */
    lin = 0;
    if (extrast)
    {
        cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", LabelColor, ehd1.c_str());
        lin = lin + 2;
        clbStatline(STATLINE_HDR1, cbuf);
        cprintf(lin, 32, ALIGN_NONE, "#%d#%s", LabelColor, cbuf.c_str());

        clbStatline(STATLINE_HDR2, cbuf);
        lin = lin + 1;
        cprintf(lin, 0, ALIGN_NONE, "#%d#%s", LabelColor, cbuf.c_str());
    }
    else
    {
        cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", LabelColor, hd1.c_str());
        lin = lin + 3;
        clbUserline( -1, -1, cbuf, false, false );
        cprintf(lin, 0, ALIGN_NONE, "#%d#%s", LabelColor, cbuf.c_str());
    }

    for ( j=0; j<cbuf.size(); j++ )
        if ( cbuf[j] != ' ' )
            cbuf[j] = '-';

    lin++;
    cprintf(lin, 0, ALIGN_NONE, "#%d#%s", LabelColor, cbuf.c_str());

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */

    offset = fuser;
    lin = fline;
    while ( offset < uvec.size() && lin <= lline )
    {
        if (extrast)
            clbStatline( uvec[offset], cbuf );
        else
            clbUserline( uvec[offset], -1, cbuf, godlike, false );

        /* determine color */
        if ( snum >= 0 && snum < cbLimits.maxShips() ) /* we're a valid ship */
        {
            if ( strcmp(cbUsers[uvec[offset]].username,
                        cbUsers[cbShips[snum].unum].username) == 0 &&
                 cbUsers[uvec[offset]].type == cbUsers[cbShips[snum].unum].type)
                color = NoColor | CQC_A_BOLD; /* it's ours */
            else if (cbShips[snum].war[cbUsers[uvec[offset]].team])
                color = RedLevelColor; /* we're at war with it */
            else if (cbShips[snum].team == cbUsers[uvec[offset]].team && !selfwar(snum))
                color = GreenLevelColor; /* it's a team ship */
            else
                color = YellowLevelColor;
        }
        else if (godlike)/* we are running conqoper */
            color = YellowLevelColor; /* bland view */
        else			/* we don't have a ship yet */
        {
            if ( strcmp(cbUsers[uvec[offset]].username,
                        cbUsers[Context.unum].username) == 0 &&
                 cbUsers[uvec[offset]].type == cbUsers[Context.unum].type)
                color = NoColor | CQC_A_BOLD;    /* it's ours */
            else if (cbUsers[Context.unum].war[cbUsers[uvec[offset]].team])
                color = RedLevelColor; /* we're war with them (might be selfwar) */
            else if (cbUsers[Context.unum].team == cbUsers[uvec[offset]].team)
                color = GreenLevelColor; /* team ship */
            else
                color = YellowLevelColor;
        }

        cprintf(lin, 0, ALIGN_NONE, "#%d#%s", color, cbuf.c_str());

        offset++;
        lin++;
    }

    if ( offset >= uvec.size() )           /* last page */
        cprintf(MSG_LIN2, 0, ALIGN_CENTER, "#%d#%s", NoColor, MTXT_DONE);
    else
        cprintf(MSG_LIN2, 0, ALIGN_CENTER, "#%d#%s", NoColor, MTXT_MORE);

    return NODE_OK;
}

static nodeStatus_t nUserlIdle(void)
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

static nodeStatus_t nUserlInput(int ch)
{
    ch = CQ_CHAR(ch);

    if (ch == TERM_EXTRA)
    {
        fuser = 0;                /* move to first page */
        return NODE_OK;
    }

    if (offset < uvec.size())
    {
        if (ch == ' ')
        {
            fuser = offset;
            return NODE_OK;
        }
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
        utLog("nUserlInput: invalid return node: %d, going to DSP_NODE_MENU",
              retnode);
        setONode(NULL);
        nMenuInit();
        break;
    }

    /* NOTREACHED */
    return NODE_OK;
}
