/*
 * playback menu node
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"
#include "conqcom.h"

#include "color.h"
#include "conf.h"
#include "record.h"
#include "playback.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "prm.h"
#include "glmisc.h"
#include "ui.h"
#include "conqutil.h"

#include "nPlayB.h"
#include "nPlayBMenu.h"
#include "nShipl.h"
#include "nPlanetl.h"
#include "nUserl.h"
#include "nHistl.h"
#include "nPlay.h"
#include "nTeaml.h"
#include "cqkeys.h"

#define S_NONE          0
#define S_WATCH         1       /* prompt for a ship */
static int state;

prm_t prm;
static int prompting;

static char cbuf[BUFFER_SIZE_256];

static int nPlayBMenuDisplay(dspConfig_t *);
static int nPlayBMenuInput(int ch);

static char *nss = NULL;        /* no such ship */

static scrNode_t nPlayBMenuNode = {
    nPlayBMenuDisplay,            /* display */
    NULL,                         /* idle */
    nPlayBMenuInput,              /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */

};

void nPlayBMenuInit(void)
{
    state = S_NONE;
    prompting = FALSE;

    /* init recFrameDelay based on samplerate if neccessary */
    if (recFrameDelay == -1.0)
        recFrameDelay = 1.0 / (real)recFileHeader.samplerate;

    setNode(&nPlayBMenuNode);

    return;
}


static int nPlayBMenuDisplay(dspConfig_t *dsp)
{
    dspReplayMenu();

    if (prompting)
        cprintf(MSG_LIN1, 1, ALIGN_NONE, "#%d#%s #%d#%s",
                CyanColor, prm.pbuf, NoColor, prm.buf);

    if (nss)
        cprintf(MSG_LIN2, 1, ALIGN_NONE, nss);

    return NODE_OK;
}


static int nPlayBMenuInput(int ch)
{
    int irv;

    ch = CQ_CHAR(ch);

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

            if ( (tmpsnum < 1 || tmpsnum > MAXSHIPS) &&
                 tmpsnum != DISPLAY_DOOMSDAY )
            {
                state = S_NONE;
                prompting = FALSE;

                nss = "No such ship.";
                return NODE_OK;
            }

            Context.snum = tmpsnum;
            nPlayBInit();         /* start playing */
        }
        return NODE_OK;
    }

    nss = NULL;
    switch (ch)
    {
    case '/':
        nShiplInit(DSP_NODE_PLAYBMENU, TRUE);
        break;

    case 'q':
        return NODE_EXIT;         /* time to leave */
        break;

    case 'r':
        pbFileSeek(recStartTime);
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
        prm.pbuf = "Watch which ship (<cr> for doomsday)?";
        prm.terms = TERMS;
        prm.index = 20;
        prompting = TRUE;

        break;

    default:
        return NODE_OK;
    }


    return NODE_OK;
}
