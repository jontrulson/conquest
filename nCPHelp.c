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
#include "nCPHelp.h"

/* from conquestgl */
extern Unsgn8 clientFlags;
extern int lastServerError;

extern void processPacket(char *);

static int nCPHelpDisplay(dspConfig_t *);
static int nCPHelpIdle(void);
static int nCPHelpInput(int ch);

static scrNode_t nCPHelpNode = {
  nCPHelpDisplay,               /* display */
  nCPHelpIdle,                  /* idle */
  nCPHelpInput                  /* input */
};


void nCPHelpInit(void)
{

  if (dConf.viewerwmapped)
    {
      /* unmap the viewer */
      glutSetWindow(dConf.viewerw);
      glutHideWindow();
      dConf.viewerwmapped = FALSE;

      glutSetWindow(dConf.mainw);
    }

  setNode(&nCPHelpNode);

  return;
}


static int nCPHelpDisplay(dspConfig_t *dsp)
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

  cprintf(1,0,ALIGN_CENTER, "#%d#%s", LabelColor, "CONQUEST COMMANDS");
  
  lin = 3;
  
  /* Display the left side. */
  tlin = lin;
  col = 4;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "0-9,=", "set warp factor (= is 10)");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "A", "change w/e allocations");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "b", "beam armies");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "B", "bombard a planet");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "C", "cloaking device");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "d,*", "detonate enemy torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "D", "detonate your own torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "E", "send emergency distress call");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "f", "fire phasers");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "F", "fire phasers, same direction");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "h", "this");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "H", "user history");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "i", "information");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "k", "set course");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "K", "try a coup");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "L", "review old messages");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "m", "send a message");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "M", "short/long range sensor toggle");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "N", "change your name");
  
  /* Now do the right side. */
  tlin = lin;
  col = 44;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "O", "options menu");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "o", "come into orbit");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "p", "launch photon torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "P", "launch photon torpedo burst");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "Q", "initiate self-destruct");
  tlin++;
  if (sStat.flags & SPSSTAT_FLAGS_REFIT)
    {
      cprintf(tlin,col,ALIGN_NONE,sfmt, "r", "refit ship to new type");
      tlin++;
    }
  cprintf(tlin,col,ALIGN_NONE,sfmt, "R", "enter repair mode");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "S", "more user statistics");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "t", "engage tractor beams");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "T", "team list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "u", "un-engage tractor beams");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "U", "user statistics");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "W", "set war or peace");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "-", "lower shields");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "+", "raise shields");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "/", "player list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "?", "planet list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "^L", "refresh the screen");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "[RETURN]", "get last info");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "[TAB]", "get next last info");
  
  cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);

  return NODE_OK;
}  

static int nCPHelpIdle(void)
{
  int pkttype;
  Unsgn8 buf[PKT_MAXSIZE];

  while ((pkttype = waitForPacket(PKT_FROMSERVER, cInfo.sock, PKT_ANYPKT,
                                  buf, PKT_MAXSIZE, 0, NULL)) > 0)
    processPacket(buf);

  if (pkttype < 0)          /* some error */
    {
      clog("nCPHelpIdle: waiForPacket returned %d", pkttype);
      Ships[Context.snum].status = SS_OFF;
      return NODE_EXIT;
    }

  if (clientFlags & SPCLNTSTAT_FLAG_KILLED)
    {
      /* time to die properly. */
      nDeadInit();
      return NODE_OK;
    }


  return NODE_OK;
}
  
static int nCPHelpInput(int ch)
{
  /* go back */

  nCPInit();

  return NODE_OK;
}

