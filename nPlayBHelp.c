/* 
 * nCP help node
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
#include "conqcom.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"

#include "nCP.h"
#include "nDead.h"
#include "nPlayB.h"
#include "nPlayBHelp.h"

static int nPlayBHelpDisplay(dspConfig_t *);
static int nPlayBHelpInput(int ch);

static scrNode_t nPlayBHelpNode = {
  nPlayBHelpDisplay,            /* display */
  NULL,                         /* idle */
  nPlayBHelpInput               /* input */
};


void nPlayBHelpInit(void)
{

  if (dConf.viewerwmapped)
    {
      /* unmap the viewer */
      glutSetWindow(dConf.viewerw);
      glutHideWindow();
      dConf.viewerwmapped = FALSE;

      glutSetWindow(dConf.mainw);
    }

  setNode(&nPlayBHelpNode);

  return;
}


static int nPlayBHelpDisplay(dspConfig_t *dsp)
{
  int lin, col, tlin;
  static int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#%%-9s#%d#%%s",
	      InfoColor,
	      LabelColor);
    }

  cprintf(1,0,ALIGN_CENTER,"#%d#%s", LabelColor, "WATCH WINDOW COMMANDS");
  
  lin = 4;
  
  /* Display the left side. */
  tlin = lin;
  col = 4;
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "w", "watch a ship");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  	"<>", "decrement/increment ship number");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "/", "player list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "f", "forward 30 seconds");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "F", "forward 2 minutes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "b", "backward 30 seconds");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "B", "backward 2 minutes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "r", "reset to beginning");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "q", "quit");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "[SPACE]", "pause/resume playback");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "-", "slow down playback by doubling the frame delay");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "+", "speed up playback by halfing the frame delay");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "M", "short/long range sensor toggle");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "n", "reset to normal playback speed");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "`", "toggle between two ships");
#if 0
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "!", "display toggle line");
#endif

  cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);
  
  return NODE_OK;
}  

static int nPlayBHelpInput(int ch)
{
  /* go back */

  nPlayBInit();

  return NODE_OK;
}

