/*
 * nCP help node
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

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
