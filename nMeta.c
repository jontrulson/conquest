/* 
 * meta select server node
 *
 * can NODE_EXIT or transfer to ConSvr node
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
#include "clientlb.h"
#include "nMeta.h"
#include "nConsvr.h"
#include "gldisplay.h"
#include "cqkeys.h"

static char *header = "Server List";
static char *header2fmt = "(Page %d of %d)";
static char headerbuf[BUFFER_SIZE];
static char header2buf[BUFFER_SIZE];
static char *eprompt = "Arrow keys to select, [TAB] to accept, any other key to quit.";

static const int servers_per_page = 10;
static int flin, llin, clin, pages, curpage;
static char servervec[META_MAXSERVERS][MAXHOSTNAME + 10]; /* hostname + port */

static int nMetaDisplay(dspConfig_t *);
static int nMetaInput(int ch);

static metaSRec_t *metaServerList;
static int nums;

static scrNode_t nMetaNode = {
  nMetaDisplay,                 /* display */
  NULL,                         /* idle */
  nMetaInput                    /* input */
};


static void dispServerInfo(dspConfig_t *dsp, metaSRec_t *metaServerList, 
                           int num)
{
  static char buf1[BUFFER_SIZE];
  static char buf2[BUFFER_SIZE];
  static char buf3[BUFFER_SIZE];
  static char buf4[BUFFER_SIZE];
  static char buf5[BUFFER_SIZE];
  static char pbuf1[BUFFER_SIZE];
  static char pbuf2[BUFFER_SIZE];
  static char pbuf3[BUFFER_SIZE];
  static char pbuf4[BUFFER_SIZE];
  static char pbuf5[BUFFER_SIZE];
  GLfloat x, y, w, h;
  static int inited = FALSE;
  static const int hcol = 2, icol = 11;
  int tlin = 3;

  tlin = 3;
  x = dsp->ppCol;
  y = (dsp->ppRow * tlin);
  w = (dsp->wW - (dsp->ppCol * 3.0));
  h = (dsp->ppRow * 5.4);

  if (!inited)
    {
      inited = TRUE;
      sprintf(pbuf1, "#%d#Server: ", MagentaColor);
      sprintf(buf1, "#%d#%%s", NoColor);

      sprintf(pbuf2, "#%d#Version: ", MagentaColor);
      sprintf(buf2, "#%d#%%s", NoColor);

      sprintf(pbuf3, "#%d#Status: ", MagentaColor);
      sprintf(buf3, 
              "#%d#Ships #%d#%%d/%%d #%d#"
              "(#%d#%%d #%d#active, #%d#%%d #%d#vacant, "
              "#%d#%%d #%d#robot)",
              NoColor, CyanColor, NoColor,
              CyanColor, NoColor, CyanColor, NoColor, 
              CyanColor, NoColor);

      sprintf(pbuf4, "#%d#Flags: ", MagentaColor);
      sprintf(buf4, "#%d#%%s", NoColor);

      sprintf(pbuf5, "#%d#MOTD: ", MagentaColor);
      sprintf(buf5, "#%d#%%s", NoColor);
    }

  cprintf(tlin, hcol, ALIGN_NONE, pbuf1);
  cprintf(tlin++, icol, ALIGN_NONE, buf1, metaServerList[num].servername);

  cprintf(tlin, hcol, ALIGN_NONE, pbuf2);
  cprintf(tlin++, icol, ALIGN_NONE, buf2, metaServerList[num].serverver);

  cprintf(tlin, hcol, ALIGN_NONE, pbuf3);
  cprintf(tlin++, icol, ALIGN_NONE, buf3,
          (metaServerList[num].numactive + metaServerList[num].numvacant +
           metaServerList[num].numrobot),
	  metaServerList[num].numtotal, 
          metaServerList[num].numactive, 
	  metaServerList[num].numvacant, metaServerList[num].numrobot);

  cprintf(tlin, hcol, ALIGN_NONE, pbuf4);
  cprintf(tlin++, icol, ALIGN_NONE, buf4, 
          clntServerFlagsStr(metaServerList[num].flags));

  cprintf(tlin, hcol, ALIGN_NONE, pbuf5);
  cprintf(tlin++, icol, ALIGN_NONE, buf5, metaServerList[num].motd);

  drawLineBox(x, y, w, h, CyanColor, 2.0);

  return;
}


void nMetaInit(metaSRec_t *ServerList, int numserv)
{
  int i;
  metaServerList = ServerList;
  nums = numserv;

  /* this is the number of required pages,        
     though page accesses start at 0 */
  if (nums >= servers_per_page)
    {
      pages = nums / servers_per_page;
      if ((nums % servers_per_page) != 0)
        pages++;                /* for runoff */
    }
  else
    pages = 1;

				/* init the servervec array */
  for (i=0; i < nums; i++)
  {
    sprintf(servervec[i], "%s:%d", 
            metaServerList[i].altaddr,
            metaServerList[i].port);
  }

  curpage = 0;

  flin = 9;			/* first server line */
  llin = 0;			/* last server line on this page */
  clin = 0;			/* current server line */

  setNode(&nMetaNode);

  return;
}

static int nMetaDisplay(dspConfig_t *dsp)
{
  int i, k;
  char *dispmac;
  int lin;
  int col;

  sprintf(header2buf, header2fmt, curpage + 1, pages);
  sprintf(headerbuf, "%s %s", header, header2buf);

  lin = 1;
  
  cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", NoColor, headerbuf);
  
  lin = flin;
  col = 1;
  
  i = 0;			/* start at index 0 */

				/* figure out the last editable line on
				   this page */

  if (curpage == (pages - 1)) /* last page - might be less than full */
    llin = (nums % servers_per_page);	/* ..or more than empty? ;-) */
  else
    llin = servers_per_page;
  
  i = 0;
  while (i < llin)
    {			/* display this page */
      /* get the server number for this line */
      k = (curpage * servers_per_page) + i; 
      
      dispmac = servervec[k];

      /* highlight the currently selected line */
      if (i == clin)
        cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d#",
                RedLevelColor, dispmac, NoColor);
      else
        cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d#",
                InfoColor, dispmac, NoColor);

      
      lin++;
      i++;
    }
  
  cprintf(MSG_LIN1, 1, ALIGN_NONE, eprompt);
  
  if (clin >= llin)
    clin = llin - 1;
  
  dispServerInfo(dsp, metaServerList, clin);

  return NODE_OK;
}


  
static int nMetaInput(int ch)
{
  int i;

  switch(ch)
    {
    case CQ_KEY_UP:		/* up */
    case CQ_KEY_LEFT:
    case 'w':
    case 'k':
      clin--;
      if (clin < 0)
        {
          if (pages != 1)
            {
              curpage--;
              if (curpage < 0)
                {
                  curpage = pages - 1;
                }
            }
          
          /* setup llin  for current page */
          if (curpage == (pages - 1)) 
            llin = (nums % servers_per_page);
          else
            llin = servers_per_page;
          
          clin = llin - 1; 
        }
      break;
      
    case CQ_KEY_DOWN:		/* down */
    case CQ_KEY_RIGHT:
    case 'x':
    case 'j':
      clin++;
      if (clin >= llin)
        {
          if (pages != 1)
            {
              curpage++;
              if (curpage >= pages)
                {
                  curpage = 0;
                }
            }
          
          clin = 0; 
        }
      break;
      
    case CQ_KEY_PAGE_UP:		/* prev page */
      if (pages != 1)
        {
          curpage--;
          if (curpage < 0)
            {
              curpage = pages - 1;
            }
        }
      
      break;
      
    case CQ_KEY_PAGE_DOWN:		/* next page */
      if (pages != 1)
        {
          curpage++;
          if (curpage >= pages)
            {
              curpage = 0;
            }
        }
      
      break;
      
    case TERM_EXTRA:	/* selected one */

      if (cInfo.remotehost)
        free(cInfo.remotehost);
      
      i = (curpage * servers_per_page) + clin;
      if ((cInfo.remotehost = strdup(metaServerList[i].altaddr)) == NULL)
        {
          clog("strdup(metaServerList[i]) failed");
          return NODE_EXIT;
        }
      cInfo.remoteport = metaServerList[i].port;

      /* transfer to the Consvr node */
      nConsvrInit(cInfo.remotehost, cInfo.remoteport);
      break;
      
    default:		/* everything else */
      return NODE_EXIT;
      break;
    }
  
  return NODE_OK;
}

