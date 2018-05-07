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
#include "ui.h"
#include "cprintf.h"

#include "nCP.h"
#include "nDead.h"
#include "nPlayB.h"
#include "nPlayBHelp.h"

static nodeStatus_t nPlayBHelpDisplay(dspConfig_t *);
static nodeStatus_t nPlayBHelpInput(int ch);

static scrNode_t nPlayBHelpNode = {
    nPlayBHelpDisplay,            /* display */
    NULL,                         /* idle */
    nPlayBHelpInput,               /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */

};


scrNode_t *nPlayBHelpInit(int setnode)
{
    if (setnode)
        setNode(&nPlayBHelpNode);

    return(&nPlayBHelpNode);
}


static nodeStatus_t nPlayBHelpDisplay(dspConfig_t *dsp)
{

    dspReplayHelp();

    cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);

    return NODE_OK;
}

static nodeStatus_t nPlayBHelpInput(int ch)
{
    /* go back */

    setONode(NULL);
    nPlayBInit();

    return NODE_OK;
}
