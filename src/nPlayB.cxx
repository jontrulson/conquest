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
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "record.h"
#include "playback.h"
#include "ibuf.h"
#include "prm.h"
#include "cqkeys.h"
#include "ui.h"

#include "nDead.h"
#include "nCPHelp.h"
#include "nPlayB.h"
#include "nPlayBMenu.h"
#include "nPlayBHelp.h"
#include "nShipl.h"
#include "nPlanetl.h"
#include "nUserl.h"
#include "nHistl.h"
#include "nTeaml.h"

#include <assert.h>

#include "glmisc.h"
#include "glfont.h"
#include "render.h"
#include "anim.h"

#include "GL.h"

#include "nPlayB.h"

#include "hud.h"

#include "cqsound.h"

#define S_NONE         0
#define S_WATCH        1
static int state;

/* the current prompt */
static prm_t prm;
static int prompting = false;

static int live_ships = true;
static int old_snum = 1;
static const char *nss = NULL;        /* no such ship */

extern int ncpSRMagFactor;
extern int ncpLRMagFactor;

static nodeStatus_t nPlayBDisplay(dspConfig_t *);
static nodeStatus_t nPlayBIdle(void);
static nodeStatus_t nPlayBInput(int ch);

static scrNode_t nPlayBNode = {
    nPlayBDisplay,               /* display */
    nPlayBIdle,                  /* idle */
    nPlayBInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animVec */
};

static animVec_t animVec;

/* team torp anim states borrowed fron nCP */
extern animStateRec_t ncpTorpAnims[NUM_PLAYERTEAMS];

static void set_header(int snum)
{

    static const char *heading_fmt = "%s %c%d (%s)%s";
    static const char *robo_str1 = "ROBOT (external)";
    static const char *robo_str2 = "ROBOT";
    static const char *ship_str1 = "SHIP";
    static char hbuf[MAX_MSGLINE_LENGTH];
    char ssbuf[MAX_MSGLINE_LENGTH];

    hbuf[0] = 0;
    ssbuf[0] = 0;

    strcat(ssbuf, ", ");
    utAppendShipStatus(ssbuf, cbShips[snum].status) ;

    if ( SROBOT(snum) )
    {
        if (cbConqInfo->externrobots == true)
	{
            sprintf(hbuf, heading_fmt, robo_str1,
                    cbTeams[cbShips[snum].team].teamchar, snum,
                    cbShips[snum].alias, ssbuf);
	}
        else
	{
            sprintf(hbuf, heading_fmt, robo_str2,
                    cbTeams[cbShips[snum].team].teamchar, snum,
                    cbShips[snum].alias, ssbuf);
	}
    }
    else
    {
        sprintf(hbuf, heading_fmt, ship_str1,
                cbTeams[cbShips[snum].team].teamchar,
                snum,
                cbShips[snum].alias, ssbuf);
    }

    hudSetRecId(hbuf);

}

void set_rectime(void)
{
    char buf[128];
    static char hbuf[128];
    time_t elapsed = (recCurrentTime - recStartTime);
    char *c;
    real percent;

    /* elapsed time */
    utFormatSeconds((int)elapsed, buf);
    c = &buf[2];			/* skip day count */

    if (elapsed <= 0)
        elapsed = 1;

    percent = ((real)elapsed / (real)recTotalElapsed ) * 100.0;

    if (pbSpeed == PB_SPEED_INFINITE)
        /* current frame delay */
        sprintf(hbuf, "%s (%d%%) INF", c, (int)percent);
    else
        sprintf(hbuf, "%s (%d%%) %2dx", c, (int)percent, pbSpeed);

    hudSetRecTime(hbuf);

    return;
}



void nPlayBInit(void)
{
    prompting = false;
    state = S_NONE;

    if (!nPlayBNode.animVec)
    {
        int i;

        // store it for the node renderer
        nPlayBNode.animVec = &animVec;
        animVec.clear();

        /* setup the team torp animators */
        for (i=0; i<NUM_PLAYERTEAMS; i++)
        {
            char nm[CQI_NAMELEN];

            snprintf(nm, CQI_NAMELEN, "ship%c-torp",
                     cbTeams[i].teamchar);

            if (!animInitState(nm, &ncpTorpAnims[i], NULL))
                utLog("%s: failed to init animstate for animation '%s'",
                      __FUNCTION__,
                      nm);
            else
                animVec.push_back(&ncpTorpAnims[i]);
        }
    }

    Context.redraw = true;
    setNode(&nPlayBNode);

    return;
}


static nodeStatus_t nPlayBDisplay(dspConfig_t *dsp)
{
    // We don't want to do anything here until the ship's team and
    // type are valid, which may require reading some packets first
    // (in Idle())

    // In particular - a ships team is now initialized to Team::NoTeam
    // on initialization of the CB, so this would core until the
    // ship's team is read in via an spShip_t packet.

    if (Context.snum < 0 || cbShips[Context.snum].team >= NUM_PLAYERTEAMS
        || cbShips[Context.snum].shiptype >= MAXNUMSHIPTYPES )
        return NODE_OK;

    /* Viewer */
    renderViewer(UserConf.doVBG);

    /* Main/Hud */
    set_header(Context.snum);

    set_rectime();

    renderHud(false);

    if (recMsg.msgbuf[0])
    {
        std::string buf;

        clbFmtMsg(recMsg.from, recMsg.fromDetail,
                  recMsg.to, recMsg.toDetail, buf);
        buf += ": ";
        buf += recMsg.msgbuf;

        hudSetPrompt(MSG_MSG, "", NoColor, buf, CyanColor);
    }

    mglOverlayQuad();             /* render the overlay bg */

    if (prompting)
        hudSetPrompt(prm.index, prm.pbuf, NoColor,
                     prm.buf, CyanColor);

    return NODE_OK;
}

static nodeStatus_t nPlayBIdle(void)
{
    int ptype;

    /* GL.c:renderFrame() will wait the appropriate time */

    /* read to the next Frame */
    if (Context.recmode != RECMODE_PAUSED)
        if (Context.recmode == RECMODE_PLAYING)
            if ((ptype = pbProcessIter()) == SP_NULL)
            {
                nPlayBMenuInit();
            }

    return NODE_OK;
}

static nodeStatus_t nPlayBInput(int ch)
{
    int irv;
    int snum = Context.snum;

    // CTRL-B, background star texture
    if (CQ_CHAR(ch) == 0x02)
    {
        UserConf.doVBG = !UserConf.doVBG;
        return NODE_OK;
    }

    /* CTRL-G, toggle tac grid on/off */
    if (CQ_CHAR(ch) == 0x07)
    {
        UserConf.DoTacBkg = !UserConf.DoTacBkg;
        return NODE_OK;
    }

    ch = CQ_CHAR(ch) | CQ_FKEY(ch);

    if (prompting)
    {
        int tmpsnum;
        irv = prmProcInput(prm, ch);

        if (irv > 0)
        {
            if (ch == TERM_ABORT)
            {
                state = S_NONE;
                prompting = false;

                return NODE_OK;
            }

            utDeleteBlanks( prm.buf );
            if ( prm.buf.empty() )
            {
                    nss = "No such ship.";
                    prompting = false;
                    return NODE_OK;
            }
            else
            {
                if (!utIsDigits(prm.buf))
                {
                    state = S_NONE;
                    prompting = false;

                    nss = "No such ship.";
                    return NODE_OK;
                }
                /* ignore return status */
                utSafeCToI( &tmpsnum, prm.buf);
            }

            if ( tmpsnum < 0 || tmpsnum >= cbLimits.maxShips() )
            {
                state = S_NONE;
                prompting = false;

                nss = "No such ship.";
                return NODE_OK;
            }

            prompting = false;
            state = S_NONE;

            // Save old snum for '`' command...
            old_snum = Context.snum;

            Context.snum = tmpsnum;
            hudClearPrompt(MSG_LIN1);
            nPlayBInit();         /* start playing */
        }

        if (nss)
            hudSetPrompt(MSG_LIN2, "", NoColor, nss, NoColor);

        return NODE_OK;
    }


    nss = NULL;
    hudClearPrompt(MSG_LIN1);

    switch (ch)
    {
    case 'q':
        nPlayBMenuInit();
        return NODE_OK;
        break;
    case 'h':
        setONode(nPlayBHelpInit(false));
        break;
    case 'f':	/* move forward 30 seconds */
        uiPutMsg("", MSG_LIN1);
        pbFileSeek(recCurrentTime + 30);
        Context.redraw = true;
        break;

    case 'F':	/* move forward 2 minutes */
        uiPutMsg("", MSG_LIN1);
        pbFileSeek(recCurrentTime + (2 * 60));
        Context.redraw = true;
        break;

    case 'M':	/* toggle lr/sr */
        if (SMAP(Context.snum))
            SFCLR(Context.snum, SHIP_F_MAP);
        else
            SFSET(Context.snum, SHIP_F_MAP);
        break;

    case 'b':	/* move backward 30 seconds */
        uiPutMsg("Rewinding...", MSG_LIN1);
        pbFileSeek(recCurrentTime - 30);
        uiPutMsg("", MSG_LIN1);
        Context.redraw = true;
        break;

    case 'B':	/* move backward 2 minutes */
        uiPutMsg("Rewinding...", MSG_LIN1);
        pbFileSeek(recCurrentTime - (2 * 60));
        uiPutMsg("", MSG_LIN1);
        Context.redraw = true;
        break;

    case 'r':	/* reset to beginning */
        uiPutMsg("Rewinding...", MSG_LIN1);
        pbFileSeek(recStartTime);
        uiPutMsg("", MSG_LIN1);
        Context.redraw = true;
        break;

    case ' ':	/* pause/resume playback */
        if (Context.recmode == RECMODE_PLAYING)
        {		/* pause */
            Context.recmode = RECMODE_PAUSED;
            uiPutMsg("PAUSED: Press [SPACE] to resume", MSG_LIN1);
        }
        else
        {		/* resume */
            Context.recmode = RECMODE_PLAYING;
            uiPutMsg("", MSG_LIN1);
        }

        break;

    case 'n':		/* set recFrameDelay to normal playback
                           speed.*/
        pbSetPlaybackSpeed(1, recFileHeader.samplerate);
        break;

        /* these seem backward, but it's easier to understand
           the '+' is faster, and '-' is slower ;-) */
    case '-':

        pbSetPlaybackSpeed(pbSpeed - 1, recFileHeader.samplerate);
        break;

    case '+':
    case '=':
        pbSetPlaybackSpeed(pbSpeed + 1, recFileHeader.samplerate);
        break;

    case 'w':
        state = S_WATCH;
        if (recFileHeader.snum == 0
            && recFileHeader.vers == RECVERSION_20031004)
        {
            prm.buf.clear();
            prm.preinit = false;

        }
        else
        {
            // for older protocols, compensate for 0-based cbShips[]
            int ship = ((recFileHeader.protoVers <= 0x0006) ?
                        recFileHeader.snum - 1: recFileHeader.snum);

            prm.buf = fmt::format("{}", ship);
            prm.preinit = true;
        }

        prm.buflen = MAX_MSGLINE_LENGTH;
        prm.pbuf = "Watch which ship? ";
        prm.index = 20;
        prompting = true;

        break;

    case '`':                 /* toggle between two ships */
        if (old_snum != snum)
        {
            int tmp_snum = snum;

            snum = old_snum;
            old_snum = tmp_snum;

            Context.snum = snum;
            Context.redraw = true;
        }
        else
            mglBeep(MGL_BEEP_ERR);

        break;

    case '/':
        setONode(nShiplInit(DSP_NODE_PLAYB, false));
        return NODE_OK;
        break;

    case '>':  /* forward rotate ship numbers */
    case CQ_KEY_RIGHT:
    case CQ_KEY_UP:
        while (true)
        {
            int i;

            if (live_ships)
            {	/* we need to make sure that there is
                   actually something alive or an
                   infinite loop will result... */
                int foundone = false;

                for (i=0; i < cbLimits.maxShips(); i++)
                {
                    if (clbStillAlive(i))
                    {
                        foundone = true;
                    }
                }
                if (foundone == false)
                {
                    mglBeep(MGL_BEEP_ERR);
                    break; /* didn't find one, beep, leave everything
                              alone*/
                }
            }

            if (snum == (cbLimits.maxShips() - 1))
            {	  /* doomsday - wrap around to first ship */
                i = 0;
            }
            else
                i = snum + 1;

            snum = i;

            Context.redraw = true;

            if (live_ships)
            {
                if (snum >= 0 && clbStillAlive(snum))
                {
                    Context.snum = snum;
                    break;
                }
                else
                    continue;
            }
            else
            {
                Context.snum = snum;
                break;
            }
        }

        break;
    case '<':  /* reverse rotate ship numbers */
    case CQ_KEY_LEFT:
    case CQ_KEY_DOWN:
        while (true)
        {
            int i;

            if (live_ships)
            {	/* we need to make sure that there is
                   actually something alive or an
                   infinite loop will result... */
                int foundone = false;

                for (i=0; i < cbLimits.maxShips(); i++)
                {
                    if (clbStillAlive(i))
                    {
                        foundone = true;
                    }
                }
                if (foundone == false)
                {
                    mglBeep(MGL_BEEP_ERR);
                    break; /* didn't find one, beep, leave everything
                              alone*/
                }
            }

            if (snum == 0)
            {
                i = cbLimits.maxShips() - 1;
            }
            else
                i = snum - 1;

            snum = i;

            Context.redraw = true;

            if (live_ships)
                if (snum >= 0 && clbStillAlive(snum))
                {
                    Context.snum = snum;
                    break;
                }
                else
                    continue;
            else
            {
                Context.snum = snum;
                break;
            }
        }

        break;

    case '[':                   /* zoom out */
    {
        if (SMAP(snum))
        {                     /* LR */
            if (ncpLRMagFactor - 1 >= -5)
            {
                ncpLRMagFactor--;
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
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpSRMagFactor, false);
        }
    }
    break;

    case ']':                   /* zoom in */
    {
        if (SMAP(snum))
        {                     /* LR */
            if (ncpLRMagFactor + 1 <= 5)
            {
                ncpLRMagFactor++;
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
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpSRMagFactor, false);
        }
    }
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

    case TERM_ABORT:
        nPlayBMenuInit();
        return NODE_OK;
        break;
    default:
        mglBeep(MGL_BEEP_ERR);
        uiPutMsg( "Type h for help.", MSG_LIN2 );
        break;
    }

    return NODE_OK;
}
