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
#include "glmisc.h"
#include "node.h"
#include "client.h"
#include "record.h"
#include "conqutil.h"
#include "cprintf.h"

#include "nMenu.h"
#include "nCP.h"
#include "nPlay.h"
#include "cqkeys.h"

#define S_DONE       0          /* ready to play */
#define S_NSERR      1          /* _newship error */
#define S_SELSYS     2          /* select system */
#define S_MENU       3          /* go back to menu */

static int state;
static int fatal = false;
static int shipinited = false;   /* whether we've done _newship() yet */
static int owned[NUMPLAYERTEAMS];
static unsigned int starttime;
static const unsigned int clientStatDelay = 10000; // 10 seconds
static const unsigned int clientSelectDelay = 60000; // 60 seconds

static nodeStatus_t nPlayDisplay(dspConfig_t *);
static nodeStatus_t nPlayIdle(void);
static nodeStatus_t nPlayInput(int ch);

static scrNode_t nPlayNode = {
    nPlayDisplay,               /* display */
    nPlayIdle,                  /* idle */
    nPlayInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animVec */
};

/* select a system to enter */
static void selectentry( uint8_t esystem )
{
    int i;
    char cbuf[BUFFER_SIZE_256];

    /* First figure out which systems we can enter from. */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        if (esystem & (1 << i))
        {
            owned[i] = true;
        }
        else
            owned[i] = false;

    /* Prompt for a decision. */
    strcpy(cbuf , "Enter which system") ;
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        if ( owned[i] )
        {
            char tbuf[BUFFER_SIZE_128];
            snprintf(tbuf, BUFFER_SIZE_128, ", #%d#%c#%d#%s",
                     CyanColor, cbTeams[i].name[0],
                     NoColor, &cbTeams[i].name[1]);
            strcat(cbuf, tbuf);
        }

    /* Change first comma to a colon. */
    char *sptr = strchr(cbuf, ',');
    if (sptr)
        *sptr = ':';

    cprintf(12, 0, ALIGN_CENTER, "%s", cbuf);
    cprintf(MSG_LIN2, 0, ALIGN_CENTER, "--- choose a team letter ---");

    return;
}

void nPlayInit(void)
{
    state = S_SELSYS;               /* default */
    shipinited = false;

    // we will check for these in nPlayIdle() so see what our fate is
    clientStatReceived = false;
    clientLastServerAckCode = 0;

    // let the server know our intentions.  We will either get a sAck
    // code (error) or a clientStat (good).
    if (!sendCommand(CPCMD_ENTER, 0))
        fatal = true;

    starttime = cInfo.nodeMillis;
    setNode(&nPlayNode);

    return;
}


static nodeStatus_t nPlayDisplay(dspConfig_t *dsp)
{
    char cbuf[BUFFER_SIZE_256];

    if (fatal)
        return NODE_EXIT;

    if (state == S_SELSYS)
    {
        if (shipinited)
        {
            selectentry(sClientStat.esystem);
        }
        else
        {
            sprintf(cbuf, "Waiting for a ClientStat...");
            cprintf(5,0,ALIGN_CENTER,"#%d#%s",InfoColor, cbuf);
        }
    }
    else if (state == S_NSERR)
    {
        switch (clientLastServerAckCode)
        {
        case PERR_FLYING:
            sprintf(cbuf, "You're already playing on another ship.");
            cprintf(5,0,ALIGN_CENTER,"#%d#%s",InfoColor, cbuf);
            cbShips[Context.snum].status = SS_RESERVED;
            break;

        default:
            cprintf(5,0,ALIGN_CENTER,
                    "#%d#nPlay: unexpected server ack, code %d",
                    InfoColor, clientLastServerAckCode);
            utLog("nPlay: unexpected server ack, code %d",
                  clientLastServerAckCode);
            break;
        }
        /* Press any key... */
        cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);
    }

    return NODE_OK;
}

static nodeStatus_t nPlayIdle(void)
{
    if (state == S_SELSYS)
    {
        // we are looking for either a clientstat packet (good) or an
        // ack packet (clientLastServerAckCode - bad)

        if (clientLastServerAckCode)
        {
            // something failed...
            utLog("%s: CPCMD_ENTER failed (clientLastServerAckCode = %d)",
                  __FUNCTION__, clientLastServerAckCode);
            state = S_NSERR;
            return NODE_OK;
        }

        if (clientStatReceived)
        {
            // looking good...
            if (!sClientStat.esystem)
            {                       /* we are ready  */
                state = S_DONE;
                return NODE_OK;
            }

            // else, we will need to select a system in display.
            shipinited = true;
        }

        // time out of these don't arrive in a reasonable amount of time
        unsigned int waitDelay = ((shipinited)
                                  ? clientSelectDelay : clientStatDelay);
        if ((cInfo.nodeMillis - starttime) > waitDelay)
        {
            utLog("%s: Timed out waiting for clientStat packet",
                  __FUNCTION__);
            fatal = true;
        }
    }
    else if (state == S_DONE)
    {
        cbShips[Context.snum].sdfuse = 0;       /* zero self destruct fuse */
        utGrand( &Context.msgrand );            /* initialize message timer */
        Context.redraw = true;                /* want redraw first time */
        Context.msgok = true;                 /* ok to get messages */

        /* need to tell the server to resend all the crap it already
           sent in menu - our ship may have chenged */
        sendCommand(CPCMD_RELOAD, 0);

        nCPInit(true);            /* play */
    }
    else if (state == S_MENU)
        nMenuInit();

    return NODE_OK;
}

static nodeStatus_t nPlayInput(int ch)
{
    int i;
    unsigned char c = CQ_CHAR(ch);

    switch (state)
    {
    case S_SELSYS:              /* we are selecting our system */
    {
        if (shipinited)
        {
            switch  ( ch )
            {
                case TERM_NORMAL:
                case TERM_ABORT:	/* doesn't like the choices ;-) */
                    sendCommand(CPCMD_ENTER, 0);
                    state = S_MENU;
                    return NODE_OK;
                    break;
                case TERM_EXTRA:
                    /* Enter the home system. */
                    sendCommand(CPCMD_ENTER, (uint16_t)(1 << cbShips[Context.snum].team));
                    state = S_DONE;
                    return NODE_OK;
                    break;
                default:
                    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
                        if ( cbTeams[i].teamchar == (char)toupper(c) && owned[i] )
                        {
                            /* Found a good one. */
                            sendCommand(CPCMD_ENTER, (uint16_t)(1 << i));
                            state = S_DONE;
                            return NODE_OK;
                        }

                    /* Didn't get a good one; complain and try again. */
                    mglBeep(MGL_BEEP_ERR);
                    break;
            }
        }
    }

    break;

    case S_NSERR:               /* any key to return */
        nMenuInit();

        return NODE_OK;
        break;
    }

    return NODE_OK;
}
