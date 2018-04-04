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
#include "nUserl.h"
#include "cqkeys.h"

static int snum, godlike;
static int nu;
static int fuser;
static int offset;

static int extrast;             /* normal, or extra stats? */

// for the sorted list
static int *uvec = NULL;


static nodeStatus_t nUserlDisplay(dspConfig_t *);
static nodeStatus_t nUserlIdle(void);
static nodeStatus_t nUserlInput(int ch);

static scrNode_t nUserlNode = {
    nUserlDisplay,               /* display */
    nUserlIdle,                  /* idle */
    nUserlInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

static int retnode;             /* the node to return to */

scrNode_t *nUserlInit(int nodeid, int setnode, int sn, int gl, int extra)
{
    int i, unum;

    retnode = nodeid;
    snum = sn;
    godlike = gl;
    extrast = extra;

    // init the user vector, free it if it already exists
    if (uvec)
        free(uvec);

    // Create it fresh
    if (!(uvec = (int *)malloc(cbLimits.maxUsers() * sizeof(int))))
    {
        utLog("%s: malloc(%lu) failed", __FUNCTION__,
              cbLimits.maxUsers() * sizeof(int));
        fprintf(stderr, "%s: malloc(%lu) failed\n", __FUNCTION__,
                cbLimits.maxUsers() * sizeof(int));
    }

    for (i=0; i<cbLimits.maxUsers(); i++)
        uvec[i] = i;

    /* sort the (living) user list */
    nu = 0;
    for ( unum = 0; unum < cbLimits.maxUsers(); unum++)
        if ( ULIVE(unum) )
        {
            uvec[nu++] = unum;
        }
    clbSortUsers(uvec, nu);

    fuser = 0;

    if (setnode)
        setNode(&nUserlNode);

    return(&nUserlNode);
}


static nodeStatus_t nUserlDisplay(dspConfig_t *dsp)
{
    int j, fline, lline, lin;
    static const char *hd1="U S E R   L I S T";
    static const char *ehd1="M O R E   U S E R   S T A T S";
    static const char *ehd2="name         cpu  conq coup geno  taken bombed/shot  shots  fired   last entry";
    static const char *ehd3="planets  armies    phaser  torps";
    static char cbuf[BUFFER_SIZE_256];
    int color;

    // bail if there was init problems (uvec allocation)
    if (!uvec)
        return NODE_EXIT;

    /* Do some screen setup. */
    lin = 0;
    if (extrast)
    {
        cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", LabelColor, ehd1);
        lin = lin + 2;
        cprintf(lin, 34, ALIGN_NONE, "#%d#%s", LabelColor, ehd3);

        utStrncpy(cbuf, ehd2, sizeof(cbuf)) ;
        lin = lin + 1;
        cprintf(lin, 0, ALIGN_NONE, "#%d#%s", LabelColor, cbuf);
    }
    else
    {
        cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", LabelColor, hd1);
        lin = lin + 3;        /* FIXME - hardcoded??? - dwp */
        clbUserline( -1, -1, cbuf, false, false );
        cprintf(lin, 0, ALIGN_NONE, "#%d#%s", LabelColor, cbuf);
    }

    for ( j = 0; cbuf[j] != 0; j = j + 1 )
        if ( cbuf[j] != ' ' )
            cbuf[j] = '-';

    lin++;
    cprintf(lin, 0, ALIGN_NONE, "#%d#%s", LabelColor, cbuf);

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */

    offset = fuser;
    lin = fline;
    while ( offset < nu && lin <= lline )
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

        cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", color, cbuf);

        offset = offset + 1;
        lin = lin + 1;
    }

    if ( offset >= nu )           /* last page */
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

    // bail if there was init problems (uvec allocation)
    if (!uvec)
        return NODE_EXIT;

    if (ch == TERM_EXTRA)
    {
        fuser = 0;                /* move to first page */
        return NODE_OK;
    }

    if (offset < nu)
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
