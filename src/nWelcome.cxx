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
#include "client.h"
#include "nWelcome.h"
#include "nMenu.h"
#include "gldisplay.h"
#include "node.h"
#include "conqutil.h"

#define S_DONE         0        /* nothing to display */
#define S_GREETINGS    1        /* GREETINGS - new user */
#define S_ERROR        2        /* some problem */

#define vowel(x) ( (x=='A' || x=='E' || x=='I' || x=='O' || x=='U' ) ? true: false)

static int state;

static bool fatal = false;
static bool serror = false;
static time_t snooze = (time_t)0;          /* sleep time */

static const char *sorry1="I'm sorry, but the game is closed for repairs right now.";
static const char *sorry2="I'm sorry, but there is no room for a new player right now.";
static const char *sorryn="Please try again some other time.  Thank you.";
static const char *selected_str="You have been selected to command a";
static const char *starship_str="starship.";
static const char *prepare_str="Prepare to be beamed aboard...";



static nodeStatus_t nWelcomeDisplay(dspConfig_t *);

static scrNode_t nWelcomeNode = {
    nWelcomeDisplay,              /* display */
    NULL,                         /* idle */
    NULL,                         /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

/*  gretds - block letter "greetings..." */
/*  SYNOPSIS */
/*    gretds */
static void gretds()
{
    int col,lin;
    static const char *g1=" GGG   RRRR   EEEEE  EEEEE  TTTTT   III   N   N   GGG    SSSS";
    static const char *g2="G   G  R   R  E      E        T      I    NN  N  G   G  S";
    static const char *g3="G      RRRR   EEE    EEE      T      I    N N N  G       SSS";
    static const char *g4="G  GG  R  R   E      E        T      I    N  NN  G  GG      S  ..  ..  ..";
    static const char *g5=" GGG   R   R  EEEEE  EEEEE    T     III   N   N   GGG   SSSS   ..  ..  ..";

    col = (int)(Context.maxcol - strlen(g5)) / (int)2;
    lin = 1;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g1);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g2);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g3);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g4);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g5);

    return;

}

void nWelcomeInit(void)
{
    int pkttype;
    char buf[PKT_MAXSIZE];
    bool done = false;

    state = S_ERROR;
    setNode(&nWelcomeNode);

    clientLastServerAckCode = 0;
    clientStatReceived = false;

    /* now look for SP_CLIENTSTAT or SP_ACK */
    while (!done)
    {
        if ((pkttype =
             pktRead(buf, PKT_MAXSIZE, 30)) <= 0)
        {
            utLog("nWelcomeInit: read SP_CLIENTSTAT or SP_ACK failed: %d",
                  pkttype);
            fatal = true;
            return;
        }

        switch (pkttype)
        {
        case SP_CLIENTSTAT:
            // Context.snum and unum is set here
            if (PKT_PROCSP(buf))
            {
                done = true;
            }
            else
            {
                utLog("nWelcomeInit: invalid CLIENTSTAT");
                fatal = true;
                return;
            }
            break;
        case SP_ACK:
            PKT_PROCSP(buf);
            state = S_ERROR;
            serror = true;
            done = true;

            break;
        default:
            utLog("nWelcomeInit: got unexpected packet type %d. Ignoring.",
                  pkttype);
            done = false;

            break;
        }
    }

    if (pkttype == SP_CLIENTSTAT && (sClientStat.flags & SPCLNTSTAT_FLAG_NEW))
    {
        // new user
        state = S_GREETINGS;
        snooze = (time(0) + 3);
    }
    else
    {
        // not a new user
        if (!serror)
            state = S_DONE;           /* need to wait for user packet */
        else
            snooze = (time(0) + 4);
    }

    return;
}


static nodeStatus_t nWelcomeDisplay(dspConfig_t *dsp)
{
    char buf[PKT_MAXSIZE];
    int team, col = 0;
    time_t t = time(0);

    if (fatal)
        return NODE_EXIT;           /* see ya! */

    if (snooze)
    {
        if (serror)                 /* an error */
        {
            if (t > snooze)     /* time to go */
            {
                snooze = 0;
                return NODE_EXIT;
            }
        }
        else
        {                       /* new user */
            if (t > snooze)
            {
                state = S_DONE;
                snooze = 0;
                return NODE_OK_NO_PKTPROC; // we do our own packet handling
            }
        }
    }


    switch (state)
    {
    case S_GREETINGS:
        /* Must be a new player. */
        if ( cbConqInfo->closed )
        {
            /* Can't enroll if the game is closed. */
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
        }
        else
        {
            team = cbShips[Context.snum].team;
            gretds();                 /* 'GREETINGS' */

            cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s%s #%d#%s #%d#%s",
                    InfoColor,selected_str,
                    vowel(cbTeams[team].name[0]) ? "n" : "",
                    CQC_A_BOLD, cbTeams[team].name,
                    InfoColor, starship_str);

            cprintf(MSG_LIN2/2+1,0,ALIGN_CENTER,"#%d#%s",
                    InfoColor, prepare_str );
        }

        return NODE_OK_NO_PKTPROC;
        break;

    case S_ERROR:
        switch (clientLastServerAckCode)
        {
        case PERR_CLOSED:
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
            break;

        case PERR_REGISTER:
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry2 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
            break;

        case PERR_NOSHIP:
            cprintf(MSG_LIN2/2, 0, ALIGN_CENTER,
                    "I'm sorry, but there are no ships available right now.");
            cprintf((MSG_LIN2/2)+1, 0, ALIGN_CENTER, "%s", sorryn);
            break;

        default:
            utLog("nWelcomeDisplay: unexpected ACK code %d\n", clientLastServerAckCode);
            break;
        }

        return NODE_OK;
        break;

    case S_DONE:
    {
        static const unsigned int waitTime = 30000; // 30 secs
        unsigned int startTime = cInfo.nodeMillis;
        bool gotUser = false;
        while ((cInfo.nodeMillis - startTime) < waitTime)
        {
            int rv;
            if ((rv = pktRead(buf, PKT_MAXSIZE, 1)) < 0)
            {
                utLog("%s: pktRead returned error: %d", __FUNCTION__, rv);
                return NODE_EXIT;
            }

            if (rv == 0)
                continue;

            PKT_PROCSP(buf);
            if (rv == SP_USER)
            {
                gotUser = true;
                break;
            }
        }

        if (!gotUser)
        {
            utLog("%s: Timed out waiting for initial SP_USER packet",
                  __FUNCTION__);
            return NODE_EXIT;
        }

        // verify that Context.snum and unum is != -1
        if (Context.snum < 0 || Context.unum < 0)
        {
            utLog("%s: FATAL: Context.snum or unum is invalid "
                  "(snum: %d, unum: %d))",
                  __FUNCTION__, Context.snum, Context.unum);
            return NODE_EXIT;
        }
        nMenuInit();
        return NODE_OK;
    }
    break;

    default:
        utLog("nWelcomeDisplay: unknown state %d", state);
        return NODE_EXIT;
        break;
    }

    return NODE_OK;
}
