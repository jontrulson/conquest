/* 
 * nCP help node
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "conqcom.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"
#include "ui.h"

#include "nCP.h"
#include "nDead.h"
#include "nPlayB.h"
#include "nPlayBHelp.h"

static int nPlayBHelpDisplay(dspConfig_t *);
static int nPlayBHelpInput(int ch);

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


static int nPlayBHelpDisplay(dspConfig_t *dsp)
{

  dspReplayHelp();

  cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);
  
  return NODE_OK;
}  

static int nPlayBHelpInput(int ch)
{
  /* go back */

  setONode(NULL);
  nPlayBInit();

  return NODE_OK;
}

