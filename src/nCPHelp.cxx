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
#include "nDead.h"
#include "nCPHelp.h"

static nodeStatus_t nCPHelpDisplay(dspConfig_t *);
static nodeStatus_t nCPHelpIdle(void);
static nodeStatus_t nCPHelpInput(int ch);

static scrNode_t nCPHelpNode = {
    nCPHelpDisplay,               /* display */
    nCPHelpIdle,                  /* idle */
    nCPHelpInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animVec */
};


scrNode_t *nCPHelpInit(int setnode)
{

    if (setnode)
        setNode(&nCPHelpNode);

    return(&nCPHelpNode);
}


static nodeStatus_t nCPHelpDisplay(dspConfig_t *dsp)
{
    int lin, col, tlin;
    static bool FirstTime = true;
    static char sfmt[MAX_MSGLINE_LENGTH * 2];

    if (FirstTime)
    {
        FirstTime = false;
        sprintf(sfmt,
                "#%d#%%-9s#%d#%%s",
                InfoColor,
                NoColor);
    }

    cprintf(1,0,ALIGN_CENTER, "#%d#%s", GreenColor, "CONQUEST COMMANDS");

    lin = 3;

    /* Display the left side. */
    tlin = lin;
    col = 4;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "0-9,=", "set warp factor (= is maximum)");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "A", "change w/e allocations");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "b", "beam armies");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "B", "bombard a planet");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "C", "cloaking device");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "d,*", "detonate enemy torpedoes");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "D", "detonate your own torpedoes");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "E", "send emergency distress call");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "f", "fire phasers");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "F", "fire phasers, same direction");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "h", "this");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "H", "user history");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "i", "information");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "k", "set course");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "K", "try a coup");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "L", "review old messages");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "m", "send a message");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "M", "short/long range sensor toggle");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "N", "change your name");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "[SPACE]", "toggle map/lrscan");

    /* Now do the right side. */
    tlin = lin;
    col = 44;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "O", "options menu");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "o", "come into orbit");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "p", "launch photon torpedo");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "P", "launch photon torpedo burst");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "Q", "initiate self-destruct");
    tlin++;
    if (sStat.serverFlags & SERVER_F_REFIT)
    {
        cprintf(tlin,col,ALIGN_NONE,sfmt, "r", "refit ship to new type");
        tlin++;
    }
    cprintf(tlin,col,ALIGN_NONE,sfmt, "R", "enter repair mode");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "S", "more user statistics");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "t", "tractor/untractor a ship");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "T", "team list");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "u", "untractor from other ships");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "U", "user statistics");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "W", "set war or peace");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "-", "lower shields");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "+", "raise shields");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "/", "player list");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "?", "planet list");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "^L", "refresh the screen");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "[ENTER]", "get last info");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "[TAB]", "get next last info");

    cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);

    return NODE_OK;
}

static nodeStatus_t nCPHelpIdle(void)
{
    if (clientStatLastFlags & SPCLNTSTAT_FLAG_KILLED)
    {
        /* time to die properly. */
        setONode(NULL);
        nDeadInit();
        return NODE_OK;
    }

    return NODE_OK;
}

static nodeStatus_t nCPHelpInput(int ch)
{
    /* go back */

    setONode(NULL);
    nCPInit(false);

    return NODE_OK;
}
