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
#include "conqlb.h"
#include "record.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nPlayB.h"
#include "nPlayBMenu.h"
#include "nShipl.h"

/* from conquestgl */
extern Unsgn8 clientFlags; 
extern int lastServerError;

extern void processPacket(char *);
static char *hd2="ship  name          pseudonym              kills     type";
static int fship;

static int nShiplDisplay(dspConfig_t *);
static int nShiplIdle(void);
static int nShiplInput(int ch);

static scrNode_t nShiplNode = {
  nShiplDisplay,               /* display */
  nShiplIdle,                  /* idle */
  nShiplInput                  /* input */
};

static int retnode;             /* the node to return to */

void nShiplInit(int nodeid)
{
  retnode = nodeid;

  if (dConf.viewerwmapped)
    {
      /* unmap the viewer */
      glutSetWindow(dConf.viewerw);
      glutHideWindow();
      dConf.viewerwmapped = FALSE;

      glutSetWindow(dConf.mainw);
    }

  setNode(&nShiplNode);

  return;
}


static int nShiplDisplay(dspConfig_t *dsp)
{
  int snum = Context.snum;
  static const int doall = FALSE; /* for now... */
  static char cbuf[BUFFER_SIZE];
  int i, unum, status, kb, lin, col;
  int fline, lline;
  char sbuf[20];
  char kbuf[20];
  char pidbuf[20];
  char ubuf[MAXUSERNAME + 2];
  int color;

  c_strcpy( hd2, cbuf );

  col = (int)(Context.maxcol - strlen( cbuf )) / (int)2;
  lin = 2;
  cprintf(lin, col, ALIGN_NONE, "#%d#%s", LabelColor, cbuf);
  
  for ( i = 0; cbuf[i] != EOS; i = i + 1 )
    if ( cbuf[i] != ' ' )
      cbuf[i] = '-';
  lin = lin + 1;
  cprintf(lin, col, ALIGN_NONE, "#%d#%s", LabelColor, cbuf);
  
  fline = lin + 1;				/* first line to use */
  lline = MSG_LIN1;				/* last line to use */
  
  i = fship;

  lin = fline;
  while ( i <= MAXSHIPS && lin <= lline )
    {
      status = Ships[i].status;
      
      kb = Ships[i].killedby;
      if ( status == SS_LIVE ||
           ( doall && ( status != SS_OFF || kb != 0 ) ) )
        {
          sbuf[0] = EOS;
          appship( i, sbuf );
          appstr(" ", sbuf);
          appchr(ShipTypes[Ships[i].shiptype].name[0], sbuf);
          
          unum = Ships[i].unum;
          if ( unum >= 0 && unum < MAXUSERS )
            {
              if (SROBOT(i)) /* robot */
                strcpy(pidbuf, " ROBOT");
              else if (SVACANT(i)) 
                strcpy(pidbuf, "VACANT");
              else
                strcpy(pidbuf, "  LIVE");
              
              strcpy(ubuf, Users[unum].username);
              
              sprintf(kbuf, "%6.1f", (Ships[i].kills + Ships[i].strkills));
              sprintf( cbuf, "%-5s %-13.13s %-21.21s %-8s %6s",
                       sbuf, ubuf, Ships[i].alias, 
                       kbuf, pidbuf );
            }
          else
            sprintf( cbuf, "%-5s %13s %21s %8s %6s", sbuf,
                     " ", " ", " ", " " );
          if ( doall && kb != 0 )
            {
              appstr( "  ", cbuf);
              appkb( kb, cbuf );
            }
          
          if (snum > 0 && snum <= MAXSHIPS )
            {		/* a normal ship view */
              if ( i == snum )    /* it's ours */
                color = NoColor | CQC_A_BOLD;
              else if (satwar(i, snum)) /* we're at war with it */
                color = RedLevelColor;
              else if (Ships[i].team == Ships[snum].team && !selfwar(snum))
                color = GreenLevelColor; /* it's a team ship */
              else
                color = YellowLevelColor;
            }
          else
            { /* not conqoper, and not a valid ship (main menu) */
              if (Users[Context.unum].war[Ships[i].team])  /* we're at war with ships's
                                                              team */
                color = RedLevelColor;
              else if (Users[Context.unum].team == Ships[i].team)
                color = GreenLevelColor; /* it's a team ship */
              else
                color = YellowLevelColor;
            }
          cprintf(lin, col, ALIGN_NONE, "#%d#%s", color, cbuf);
          
          if ( doall && status != SS_LIVE )
            {
              cbuf[0] = EOS;
              appsstatus( status, cbuf );
              
              cprintf(lin, col - 2 - strlen( cbuf ), 
                      ALIGN_NONE, "#%d#%s", YellowLevelColor, cbuf);
            }
        }
      i = i + 1;
      lin = lin + 1;
    }

  cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);
  
  return NODE_OK;
}  

static int nShiplIdle(void)
{
  int pkttype;
  Unsgn8 buf[PKT_MAXSIZE];

  if (Context.recmode == RECMODE_PLAYING)
    return NODE_OK;             /* no packet reading here */

  while ((pkttype = waitForPacket(PKT_FROMSERVER, cInfo.sock, PKT_ANYPKT,
                                  buf, PKT_MAXSIZE, 0, NULL)) > 0)
    processPacket(buf);

  if (pkttype < 0)          /* some error */
    {
      clog("nShiplIdle: waiForPacket returned %d", pkttype);
      Ships[Context.snum].status = SS_OFF;
      return NODE_EXIT;
    }

  if (clientFlags & SPCLNTSTAT_FLAG_KILLED && retnode == DSP_NODE_CP)
    {
      /* time to die properly. */
      nDeadInit();
      return NODE_OK;
    }
      

  return NODE_OK;
}
  
static int nShiplInput(int ch)
{
  /* go back */
  switch (retnode)
    {
    case DSP_NODE_CP:
      nCPInit();
      break;
    case DSP_NODE_MENU:
      nMenuInit();
      break;

    case DSP_NODE_PLAYBMENU:
      nPlayBMenuInit();
      break;

    case DSP_NODE_PLAYB:
      nPlayBInit();
      break;

    default:
      clog("nShiplInput: invalid return node: %d, going to DSP_NODE_MENU",
           retnode);
      nMenuInit();
      break;
    }

  /* NOTREACHED */
  return NODE_OK;
}

