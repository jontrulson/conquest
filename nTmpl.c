/* 
 * node
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"
#include "datatypes.h"
#include "color.h"
#include "conf.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "nConsvr.h"

static int nConsvrDisplay(dspConfig_t *);
static int nConsvrIdle(void);
static int nConsvrInput(int ch);

static scrNode_t nConsvrNode = {
  nConsvrDisplay,               /* display */
  nConsvrIdle,                  /* idle */
  nConsvrInput                  /* input */
};


void nConsvrInit(void)
{

  setNode(&nConsvrNode);

  return;
}


static int nConsvrDisplay(dspConfig_t *dsp)
{
  return NODE_OK;
}  
  
static int nConsvrIdle(void)
{
  return NODE_OK;
}

static int nConsvrInput(int ch)
{
  return NODE_OK;
}

