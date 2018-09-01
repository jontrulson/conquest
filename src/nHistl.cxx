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
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"
#include "conqutil.h"
#include "cprintf.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nHistl.h"

static nodeStatus_t nHistlDisplay(dspConfig_t *);
static nodeStatus_t nHistlIdle(void);
static nodeStatus_t nHistlInput(int ch);

static scrNode_t nHistlNode = {
    nHistlDisplay,               /* display */
    nHistlIdle,                  /* idle */
    nHistlInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animVec */
};

static int retnode;             /* the node to return to */

scrNode_t *nHistlInit(int nodeid, int setnode)
{
    retnode = nodeid;

    if (setnode)
        setNode(&nHistlNode);

    return(&nHistlNode);
}


static nodeStatus_t nHistlDisplay(dspConfig_t *dsp)
{
    int i, j, lin, col, fline, lline, thistptr = 0;
    static const char *hd0="C O N Q U E S T   U S E R   H I S T O R Y";
    char connecttm[BUFFER_SIZE_256];
    char histentrytm[MAXDATESIZE + 1];
    char puname[MAXUSERNAME];

    /* Do some screen setup. */
    fline = 1;
    lline = MSG_LIN1 - 1;
    cprintf(fline,0,ALIGN_CENTER,"#%d#%s",LabelColor, hd0);
    fline = fline + 2;

    thistptr = cbConqInfo->histptr;
    lin = fline;
    col = 1;

    i = thistptr + 1;
    for ( j = 0; j < cbLimits.maxHist(); j++ )
    {
        bool isResigned = false;

        // adjust based on histptr being the next available slot.
        i--;
        if (i < 0)
            i = cbLimits.maxHist() - 1;

        // No username, no entry
        if (!cbHistory[i].username[0])
            continue;

        if (cbHistory[i].unum < 0) // resigned
            isResigned = true;

        utStrncpy(puname, cbHistory[i].username, MAXUSERNAME);

        /* entry time */
        utFormatTime( histentrytm, cbHistory[i].enterTime);

        /* now elapsed time */
        utFormatSeconds((int) cbHistory[i].elapsed, connecttm);
        /* strip off seconds, or for long times, anything after 7 bytes */
        connecttm[7] = '\0';

        cprintf( lin, col, ALIGN_NONE,
                 "#%d#%-10.10s #%d#%16s#%d#-#%d#%7s",
                 (isResigned) ? CyanColor : YellowLevelColor,
                 puname,
                 GreenLevelColor,
                 histentrytm,
                 NoColor,
                 RedLevelColor,
                 connecttm);

        lin++;

        if (col == 40 && lin > lline)
            break;              // we're done, can't display any more

        if ( lin > lline )
        {
            col = 40;
            lin = fline;
        }
    }

    cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);


    return NODE_OK;
}

static nodeStatus_t nHistlIdle(void)
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

static nodeStatus_t nHistlInput(int ch)
{
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
        utLog("nHistlInput: invalid return node: %d, going to DSP_NODE_MENU",
              retnode);
        setONode(NULL);
        nMenuInit();
        break;
    }

    /* NOTREACHED */
    return NODE_OK;
}
