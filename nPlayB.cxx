/*
 * playback cockpit node
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#include "c_defs.h"
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

#define cp_putmsg(str, lin)  hudSetPrompt(lin, NULL, NoColor, str, NoColor)

#define S_NONE         0
#define S_WATCH        1
static int state;

/* the current prompt */
static prm_t prm;
static int prompting = FALSE;

/* misc buffers */
static char cbuf[BUFFER_SIZE_1024];

static int live_ships = TRUE;
static int old_snum = 1;
static const char *nss = NULL;        /* no such ship */

extern hudData_t hudData;

extern int ncpSRMagFactor;
extern int ncpLRMagFactor;

static int nPlayBDisplay(dspConfig_t *);
static int nPlayBIdle(void);
static int nPlayBInput(int ch);

static scrNode_t nPlayBNode = {
    nPlayBDisplay,               /* display */
    nPlayBIdle,                  /* idle */
    nPlayBInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

static animQue_t animQue;

/* team torp anim states borrowed fron nCP */
extern animStateRec_t ncpTorpAnims[NUMPLAYERTEAMS];

static void set_header(int snum)
{

    static const char *heading_fmt = "%s %c%d (%s)%s";
    static const char *doom_fmt = "%s (%s)";
    static const char *closed_str1 = "GAME CLOSED -";
    static const char *robo_str1 = "ROBOT (external)";
    static const char *robo_str2 = "ROBOT";
    static const char *ship_str1 = "SHIP";
    static char hbuf[MSGMAXLINE];
    char ssbuf[MSGMAXLINE];

    hbuf[0] = 0;
    ssbuf[0] = 0;

    strcat(ssbuf, ", ");
    utAppendShipStatus(ssbuf, cbShips[snum].status) ;

    if ( cbConqInfo->closed)
    {
        sprintf(hbuf, heading_fmt, closed_str1,
                cbTeams[cbShips[snum].team].teamchar,
                snum,
                cbShips[snum].alias, ssbuf);
    }
    else if ( SROBOT(snum) )
    {
        if (cbConqInfo->externrobots == TRUE)
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
        if (snum == DISPLAY_DOOMSDAY)
            sprintf(hbuf, doom_fmt, cbDoomsday->name,
                    (DOOM_LIVE()) ? "On": "Off");
        else
            sprintf(hbuf, heading_fmt, ship_str1, cbTeams[cbShips[snum].team].teamchar,
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
    prompting = FALSE;
    state = S_NONE;

    if (!nPlayBNode.animQue)
    {
        int i;

        nPlayBNode.animQue = &animQue;
        animQueInit(nPlayBNode.animQue);

        /* setup the team torp animators */
        for (i=0; i<NUMPLAYERTEAMS; i++)
        {
            char nm[CQI_NAMELEN];

            snprintf(nm, CQI_NAMELEN, "ship%c-torp",
                     cbTeams[i].name[0]);

            if (!animInitState(nm, &ncpTorpAnims[i], NULL))
                utLog("%s: failed to init animstate for animation '%s'",
                      __FUNCTION__,
                      nm);
            else
                animQueAdd(nPlayBNode.animQue, &ncpTorpAnims[i]);
        }
    }

    Context.redraw = TRUE;
    setNode(&nPlayBNode);

    return;
}


static int nPlayBDisplay(dspConfig_t *dsp)
{
    char buf[MSGMAXLINE];

    /* Viewer */
    renderViewer(UserConf.doVBG, FALSE);

    /* Main/Hud */
    set_header(Context.snum);
    set_rectime();
    renderHud(false);

    if (recMsg.msgbuf[0])
    {
        clbFmtMsg(recMsg.from, recMsg.fromDetail,
                  recMsg.to, recMsg.toDetail, buf);
        strcat(buf , ": ") ;
        strcat(buf , recMsg.msgbuf) ;

        hudSetPrompt(MSG_MSG, NULL, NoColor, buf, CyanColor);
    }

    mglOverlayQuad();             /* render the overlay bg */

    if (prompting)
        hudSetPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

    return NODE_OK;
}

static int nPlayBIdle(void)
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

static int nPlayBInput(int ch)
{
    int irv;
    int snum = Context.snum;

    if ((CQ_CHAR(ch) == 'B' || CQ_CHAR(ch) == 'b') &&
        CQ_MODIFIER(ch) & CQ_KEY_MOD_ALT)
    {
        UserConf.doVBG = !UserConf.doVBG;
        return NODE_OK;
    }

    ch = CQ_CHAR(ch) | CQ_FKEY(ch);

    if (prompting)
    {
        int tmpsnum;
        irv = prmProcInput(&prm, ch);

        if (irv > 0)
        {
            if (ch == TERM_ABORT)
            {
                state = S_NONE;
                prompting = FALSE;

                return NODE_OK;
            }

            utDeleteBlanks( prm.buf );
            if ( strlen( prm.buf ) == 0 )
            {              /* watch doomsday machine */
                tmpsnum = DISPLAY_DOOMSDAY;
            }
            else
            {
                if (!utIsDigits(prm.buf))
                {
                    state = S_NONE;
                    prompting = FALSE;

                    nss = "No such ship.";
                    return NODE_OK;
                }
                utSafeCToI( &tmpsnum, prm.buf, 0 );     /* ignore return status */
            }

            if ( (tmpsnum < 0 || tmpsnum >= MAXSHIPS) &&
                 tmpsnum != DISPLAY_DOOMSDAY )
            {
                state = S_NONE;
                prompting = FALSE;

                nss = "No such ship.";
                return NODE_OK;
            }

            prompting = FALSE;
            state = S_NONE;

            // Save old snum for '`' command...
            old_snum = Context.snum;

            Context.snum = tmpsnum;
            hudClearPrompt(MSG_LIN1);
            nPlayBInit();         /* start playing */
        }

        if (nss)
            hudSetPrompt(MSG_LIN2, NULL, NoColor, nss, NoColor);

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
        setONode(nPlayBHelpInit(FALSE));
        break;
    case 'f':	/* move forward 30 seconds */
        cp_putmsg(NULL, MSG_LIN1);
        pbFileSeek(recCurrentTime + 30);
        Context.redraw = TRUE;
        break;

    case 'F':	/* move forward 2 minutes */
        cp_putmsg(NULL, MSG_LIN1);
        pbFileSeek(recCurrentTime + (2 * 60));
        Context.redraw = TRUE;
        break;

    case 'M':	/* toggle lr/sr */
        if (SMAP(Context.snum))
            SFCLR(Context.snum, SHIP_F_MAP);
        else
            SFSET(Context.snum, SHIP_F_MAP);
        break;

    case 'b':	/* move backward 30 seconds */
        cp_putmsg("Rewinding...", MSG_LIN1);
        pbFileSeek(recCurrentTime - 30);
        cp_putmsg(NULL, MSG_LIN1);
        Context.redraw = TRUE;
        break;

    case 'B':	/* move backward 2 minutes */
        cp_putmsg("Rewinding...", MSG_LIN1);
        pbFileSeek(recCurrentTime - (2 * 60));
        cp_putmsg(NULL, MSG_LIN1);
        Context.redraw = TRUE;
        break;

    case 'r':	/* reset to beginning */
        cp_putmsg("Rewinding...", MSG_LIN1);
        pbFileSeek(recStartTime);
        cp_putmsg(NULL, MSG_LIN1);
        Context.redraw = TRUE;
        break;

    case ' ':	/* pause/resume playback */
        if (Context.recmode == RECMODE_PLAYING)
        {		/* pause */
            Context.recmode = RECMODE_PAUSED;
            cp_putmsg("PAUSED: Press [SPACE] to resume", MSG_LIN1);
        }
        else
        {		/* resume */
            Context.recmode = RECMODE_PLAYING;
            cp_putmsg(NULL, MSG_LIN1);
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
        if (recFileHeader.snum == 0)
        {
            cbuf[0] = 0;
            prm.preinit = FALSE;

        }
        else
        {
            sprintf(cbuf, "%d", recFileHeader.snum);
            prm.preinit = TRUE;
        }

        prm.buf = cbuf;
        prm.buflen = MSGMAXLINE;
        prm.pbuf = "Watch which ship (<cr> for doomsday)? ";
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prompting = TRUE;

        break;

    case '`':                 /* toggle between two ships */
        if (old_snum != snum)
        {
            int tmp_snum = snum;

            snum = old_snum;
            old_snum = tmp_snum;

            Context.snum = snum;
            Context.redraw = TRUE;
        }
        else
            mglBeep(MGL_BEEP_ERR);

        break;

    case '/':
        setONode(nShiplInit(DSP_NODE_PLAYB, FALSE));
        return NODE_OK;
        break;

    case '>':  /* forward rotate ship numbers (including doomsday) - dwp */
    case CQ_KEY_RIGHT:
    case CQ_KEY_UP:
        while (TRUE)
        {
            int i;

            if (live_ships)
            {	/* we need to make sure that there is
                   actually something alive or an
                   infinite loop will result... */
                int foundone = FALSE;

                for (i=0; i < MAXSHIPS; i++)
                {
                    if (clbStillAlive(i))
                    {
                        foundone = TRUE;
                    }
                }
                if (foundone == FALSE)
                {	/* check the doomsday machine */
                    if (DOOM_LIVE())
                        foundone = TRUE;
                }

                if (foundone == FALSE)
                {
                    mglBeep(MGL_BEEP_ERR);
                    break; /* didn't find one, beep, leave everything
                              alone*/
                }
            }

            if (snum == DISPLAY_DOOMSDAY)
            {	  /* doomsday - wrap around to first ship */
                i = 0;
            }
            else
                i = snum + 1;

            if (i >= MAXSHIPS)
            {	/* if we're going past
                   now loop thu specials (only doomsday for
                   now... ) */
                i = DISPLAY_DOOMSDAY;
            }

            snum = i;

            Context.redraw = TRUE;

            if (live_ships)
                if ((snum >= 0 && clbStillAlive(snum)) ||
                    (snum == DISPLAY_DOOMSDAY && DOOM_LIVE()))
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
    case '<':  /* reverse rotate ship numbers (including doomsday)  - dwp */
    case CQ_KEY_LEFT:
    case CQ_KEY_DOWN:
        while (TRUE)
        {
            int i;

            if (live_ships)
            {	/* we need to make sure that there is
                   actually something alive or an
                   infinite loop will result... */
                int foundone = FALSE;

                for (i=0; i < MAXSHIPS; i++)
                {
                    if (clbStillAlive(i))
                    {
                        foundone = TRUE;
                    }
                }
                if (foundone == FALSE)
                {	/* check the doomsday machine */
                    if (DOOM_LIVE())
                        foundone = TRUE;
                }

                if (foundone == FALSE)
                {
                    mglBeep(MGL_BEEP_ERR);
                    break; /* didn't find one, beep, leave everything
                              alone*/
                }
            }


            if (snum == DISPLAY_DOOMSDAY)
            {	  /* doomsday - wrap around to last ship */
                i = MAXSHIPS - 1;
            }
            else
                i = snum - 1;

            if (i < 0)
            {	/* if we're going past
                   now loop thu specials (only doomsday for
                   now... )*/
                i = DISPLAY_DOOMSDAY;
            }

            snum = i;

            Context.redraw = TRUE;

            if (live_ships)
                if ((snum >= 0 && clbStillAlive(snum)) ||
                    (snum == DISPLAY_DOOMSDAY && DOOM_LIVE()))
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

            setViewerScaling(ncpLRMagFactor, TRUE);
        }
        else
        {
            if (ncpSRMagFactor - 1 >= -5)
            {
                ncpSRMagFactor--;
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpSRMagFactor, FALSE);
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

            setViewerScaling(ncpLRMagFactor, TRUE);
        }
        else
        {
            if (ncpSRMagFactor + 1 <= 5)
            {
                ncpSRMagFactor++;
            }
            else
                mglBeep(MGL_BEEP_ERR);

            setViewerScaling(ncpSRMagFactor, FALSE);
        }
    }
    break;

    case TERM_REDRAW:			/* clear all the prompts */
        hudClearPrompt(MSG_LIN1);
        hudClearPrompt(MSG_LIN2);
        hudClearPrompt(MSG_MSG);

        /* reset the scaling factors */
        ncpLRMagFactor = ncpSRMagFactor = 0;
        setViewerScaling(ncpLRMagFactor, TRUE);
        setViewerScaling(ncpSRMagFactor, FALSE);

        break;

    case TERM_ABORT:
        nPlayBMenuInit();
        return NODE_OK;
        break;
    default:
        mglBeep(MGL_BEEP_ERR);
        cp_putmsg( "Type h for help.", MSG_LIN2 );
        break;
    }

    return NODE_OK;
}
