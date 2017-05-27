/* 
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
#include "conqutil.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nHistl.h"

static int nHistlDisplay(dspConfig_t *);
static int nHistlIdle(void);
static int nHistlInput(int ch);

static scrNode_t nHistlNode = {
  nHistlDisplay,               /* display */
  nHistlIdle,                  /* idle */
  nHistlInput,                  /* input */
  NULL,                         /* minput */
  NULL                          /* animQue */
};

static int retnode;             /* the node to return to */

scrNode_t *nHistlInit(int nodeid, int setnode)
{
  retnode = nodeid;

  if (setnode)
    setNode(&nHistlNode);

  return(&nHistlNode);
}


static int nHistlDisplay(dspConfig_t *dsp)
{
  int i, j, unum, lin, col, fline, lline, thistptr = 0;
  char *hd0="C O N Q U E S T   U S E R   H I S T O R Y";
  char connecttm[BUFFER_SIZE];
  char histentrytm[DATESIZE + 1];

				/* Do some screen setup. */
  fline = 1;
  lline = MSG_LIN1 - 1;
  cprintf(fline,0,ALIGN_CENTER,"#%d#%s",LabelColor, hd0);
  fline = fline + 2;
  
  thistptr = ConqInfo->histptr;
  lin = fline;
  col = 1;
  
  i = thistptr + 1;
  for ( j = 0; j < MAXHISTLOG; j++ )
    {
      /* FIXME:  after new proto, extract username and flag
         resigned users (has username and -1 hist unum) */
      i = utModPlusOne( i - 1, MAXHISTLOG );
      unum = History[i].histunum;
      
      if ( unum < 0 || unum >= MAXUSERS )
        continue; 
      if ( ! Users[unum].live )
        continue; 
      
      /* entry time */
      utFormatTime( histentrytm, History[i].histlog);
      
      
      /* now elapsed time */
      utFormatSeconds((int) History[i].elapsed, connecttm);
      /* strip off seconds, or for long times, anything after 7 bytes */
      connecttm[7] = '\0';
      
      cprintf( lin, col, ALIGN_NONE, 
               "#%d#%-10.10s #%d#%16s#%d#-#%d#%7s", 
               YellowLevelColor,
               Users[unum].username, 
               GreenLevelColor,
               histentrytm,
               NoColor,
               RedLevelColor,
               connecttm);
      
      lin++;
      if ( lin > lline )
        {
          col = 40;
          lin = fline;
        }
    }
  
  cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);
  

  return NODE_OK;
}  

static int nHistlIdle(void)
{
  int pkttype;
  char buf[PKT_MAXSIZE];

  while ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                     buf, PKT_MAXSIZE, 0, NULL)) > 0)
    processPacket(buf);

  if (pkttype < 0)          /* some error */
    {
      utLog("nHistlIdle: waiForPacket returned %d", pkttype);
      Ships[Context.snum].status = SS_OFF;
      return NODE_EXIT;
    }

  if (clientFlags & SPCLNTSTAT_FLAG_KILLED && retnode == DSP_NODE_CP)
    {
      /* time to die properly. */
      setONode(NULL);
      nDeadInit();
      return NODE_OK;
    }
      

  return NODE_OK;
}
  
static int nHistlInput(int ch)
{
  /* go back */
  switch (retnode)
    {
    case DSP_NODE_CP:
      setONode(NULL);
      nCPInit(FALSE);
      break;
    case DSP_NODE_MENU:
      setONode(NULL);
      nMenuInit();
      break;

    default:
      utLog("nHistlInput: invalid return node: %d, going to DSP_NODE_MENU",
           retnode);
      setONode(NULL);
      nMenuInit();
      break;
    }

  /* NOTREACHED */
  return NODE_OK;
}

