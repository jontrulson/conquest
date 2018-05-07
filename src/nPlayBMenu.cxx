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
#include "cb.h"

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
#include "cprintf.h"

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

static prm_t prm;
static int prompting = false;

static nodeStatus_t nPlayBMenuDisplay(dspConfig_t *);
static nodeStatus_t nPlayBMenuInput(int ch);

static const char *nss = NULL;        /* no such ship */

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
    prompting = false;

    /* init recFrameDelay based on samplerate if neccessary */
    if (recFrameDelay == -1.0)
        recFrameDelay = 1.0 / (real)recFileHeader.samplerate;

    setNode(&nPlayBMenuNode);

    return;
}


static nodeStatus_t nPlayBMenuDisplay(dspConfig_t *dsp)
{
    dspReplayMenu();

    if (prompting)
        cprintf(MSG_LIN1, 1, ALIGN_NONE, "#%d#%s #%d#%s",
                CyanColor, prm.pbuf.c_str(), NoColor, prm.buf.c_str());

    if (nss)
        cprintf(MSG_LIN2, 1, ALIGN_NONE, "%s", nss);

    return NODE_OK;
}


static nodeStatus_t nPlayBMenuInput(int ch)
{
    int irv;

    ch = CQ_CHAR(ch);

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
                state = S_NONE;
                prompting = false;

                nss = "No such ship.";
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
                utSafeCToI( &tmpsnum, prm.buf );
            }

            if ( (tmpsnum < 0 || tmpsnum >= cbLimits.maxShips()) &&
                 tmpsnum != DISPLAY_DOOMSDAY )
            {
                state = S_NONE;
                prompting = false;

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
        nShiplInit(DSP_NODE_PLAYBMENU, true);
        break;

    case 'q':
        return NODE_EXIT;         /* time to leave */
        break;

    case 'r':
        pbFileSeek(recStartTime);
        break;

    case 'w':
        state = S_WATCH;
        // Older versions used snum to differentiate between client
        // and server recordings...
        if (recFileHeader.snum == 0
            && recFileHeader.vers == RECVERSION_20031004)
        {
            prm.buf.clear();;
            prm.preinit = false;

        }
        else
        {
            // for older protocols, compensate for 0-based cbShips[]
            int ship = ((recFileHeader.protoVers <= 0x0006) ?
                        recFileHeader.snum - 1:
                        recFileHeader.snum);

            prm.buf = fmt::format("{}", ship);
            prm.preinit = true;
        }

        prm.buflen = MSGMAXLINE;
        prm.pbuf = "Watch which ship? ";
        prm.index = 20;
        prompting = true;

        break;

    default:
        return NODE_OK;
    }


    return NODE_OK;
}
