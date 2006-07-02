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
#include "nMenu.h"
#include "nDead.h"
#include "nTeaml.h"

static int team;

/* from conquestgl */
extern Unsgn8 clientFlags; 
extern int lastServerError;

static int nTeamlDisplay(dspConfig_t *);
static int nTeamlIdle(void);
static int nTeamlInput(int ch);

static scrNode_t nTeamlNode = {
  nTeamlDisplay,               /* display */
  nTeamlIdle,                  /* idle */
  nTeamlInput,                  /* input */
  NULL,                         /* minput */
  NULL                          /* animQue */
};

static int retnode;             /* the node to return to */

scrNode_t *nTeamlInit(int nodeid, int setnode, int tn)
{
  team = tn;
  retnode = nodeid;

  if (setnode)
    setNode(&nTeamlNode);

  return(&nTeamlNode);
}


static int nTeamlDisplay(dspConfig_t *dsp)
{
  int i, j, lin, col, ctime, etime;
  int godlike;
  char buf[MSGMAXLINE], timbuf[5][DATESIZE];
  real x[5];
  static string sfmt="%15s %11s %11s %11s %11s %11s";
  static char *stats="Statistics since: ";
  static char *last_conquered="Universe last conquered at: ";

  char tmpfmt[MSGMAXLINE * 2];
  static char sfmt2[MSGMAXLINE * 2];
  static char sfmt3[MSGMAXLINE * 2];
  static char dfmt2[MSGMAXLINE * 2];
  static char pfmt2[MSGMAXLINE * 2];
  static int FirstTime = TRUE;	/* Only necc if the colors aren't 
				   going to change at runtime */

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt2,
	      "#%d#%%16s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s",
	      LabelColor,
	      GreenLevelColor,
	      YellowLevelColor,
	      RedLevelColor,
	      SpecialColor,
	      InfoColor);

      sprintf(sfmt3,
	      "#%d#%%15s #%d#%%12s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s",
	      LabelColor,
	      GreenLevelColor,
	      YellowLevelColor,
	      RedLevelColor,
	      SpecialColor,
	      InfoColor);

      sprintf(dfmt2,
	      "#%d#%%15s #%d#%%12d #%d#%%11d #%d#%%11d #%d#%%11d #%d#%%11d",
	      LabelColor,
	      GreenLevelColor,
	      YellowLevelColor,
	      RedLevelColor,
	      SpecialColor,
	      InfoColor);

      sprintf(pfmt2,
		  "#%d#%%15s #%d#%%11.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%%",
	      LabelColor,
	      GreenLevelColor,
	      YellowLevelColor,
	      RedLevelColor,
	      SpecialColor,
	      InfoColor);

  } /* FIRST_TIME */

  godlike = ( team < 0 || team >= NUMPLAYERTEAMS );
  col = 0; /*1*/
  
  lin = 1;
  /* team stats and last date conquered */
  sprintf(tmpfmt,"#%d#%%s#%d#%%s",LabelColor,InfoColor);
  cprintf(lin,0,ALIGN_CENTER, tmpfmt, stats, ConqInfo->inittime);
  lin++;

  /* last conquered */
  cprintf(lin, 0, ALIGN_CENTER, tmpfmt, last_conquered, 
	  ConqInfo->conqtime);
  lin++;

  /* last conqueror and conqteam */
  sprintf(tmpfmt,"#%d#by #%d#%%s #%d#for the #%d#%%s #%d#team",
		LabelColor,(int)CQC_A_BOLD,LabelColor,(int)CQC_A_BOLD,LabelColor);
  cprintf(lin,0,ALIGN_CENTER, tmpfmt, ConqInfo->conqueror, 
	  ConqInfo->conqteam);
  
  col=0;  /* put col back to 0 for rest of display */
  lin = lin + 1;

  if ( ConqInfo->lastwords[0] != EOS )
    {
      sprintf(tmpfmt, "#%d#%%c%%s%%c", YellowLevelColor);
      cprintf(lin, 0, ALIGN_CENTER, tmpfmt, '"', ConqInfo->lastwords, '"' );
    }
  
  lin+=2;
  sprintf( buf, sfmt, " ",
	 Teams[0].name, Teams[1].name, Teams[2].name, Teams[3].name, "Totals" );
  cprintf(lin,col,0, sfmt2, " ",
	 Teams[0].name, Teams[1].name, Teams[2].name, Teams[3].name, "Totals" );
  
  lin++;
  for ( i = 0; buf[i] != EOS; i++ )
    if ( buf[i] != ' ' )
      buf[i] = '-';

  cprintf(lin,col, ALIGN_NONE, "#%d#%s", LabelColor, buf);
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Conquers",
	 Teams[0].stats[TSTAT_CONQUERS], Teams[1].stats[TSTAT_CONQUERS],
	 Teams[2].stats[TSTAT_CONQUERS], Teams[3].stats[TSTAT_CONQUERS],
	 Teams[0].stats[TSTAT_CONQUERS] + Teams[1].stats[TSTAT_CONQUERS] +
	 Teams[2].stats[TSTAT_CONQUERS] + Teams[3].stats[TSTAT_CONQUERS] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Wins",
	 Teams[0].stats[TSTAT_WINS], Teams[1].stats[TSTAT_WINS],
	 Teams[2].stats[TSTAT_WINS], Teams[3].stats[TSTAT_WINS],
	 Teams[0].stats[TSTAT_WINS] + Teams[1].stats[TSTAT_WINS] +
	 Teams[2].stats[TSTAT_WINS] + Teams[3].stats[TSTAT_WINS] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Losses",
	 Teams[0].stats[TSTAT_LOSSES], Teams[1].stats[TSTAT_LOSSES],
	 Teams[2].stats[TSTAT_LOSSES], Teams[3].stats[TSTAT_LOSSES],
	 Teams[0].stats[TSTAT_LOSSES] + Teams[1].stats[TSTAT_LOSSES] +
	 Teams[2].stats[TSTAT_LOSSES] + Teams[3].stats[TSTAT_LOSSES] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Ships",
	 Teams[0].stats[TSTAT_ENTRIES], Teams[1].stats[TSTAT_ENTRIES],
	 Teams[2].stats[TSTAT_ENTRIES], Teams[3].stats[TSTAT_ENTRIES],
	 Teams[0].stats[TSTAT_ENTRIES] + Teams[1].stats[TSTAT_ENTRIES] +
	 Teams[2].stats[TSTAT_ENTRIES] + Teams[3].stats[TSTAT_ENTRIES] );
  
  lin++;
  etime = Teams[0].stats[TSTAT_SECONDS] + Teams[1].stats[TSTAT_SECONDS] +
    Teams[2].stats[TSTAT_SECONDS] + Teams[3].stats[TSTAT_SECONDS];
  fmtseconds( Teams[0].stats[TSTAT_SECONDS], timbuf[0] );
  fmtseconds( Teams[1].stats[TSTAT_SECONDS], timbuf[1] );
  fmtseconds( Teams[2].stats[TSTAT_SECONDS], timbuf[2] );
  fmtseconds( Teams[3].stats[TSTAT_SECONDS], timbuf[3] );
  fmtseconds( etime, timbuf[4] );
  cprintf(lin,col,0, sfmt3, "Time",
	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  
  lin++;
  ctime = Teams[0].stats[TSTAT_CPUSECONDS] + Teams[1].stats[TSTAT_CPUSECONDS] +
    Teams[2].stats[TSTAT_CPUSECONDS] + Teams[3].stats[TSTAT_CPUSECONDS];
  fmtseconds( Teams[0].stats[TSTAT_CPUSECONDS], timbuf[0] );
  fmtseconds( Teams[1].stats[TSTAT_CPUSECONDS], timbuf[1] );
  fmtseconds( Teams[2].stats[TSTAT_CPUSECONDS], timbuf[2] );
  fmtseconds( Teams[3].stats[TSTAT_CPUSECONDS], timbuf[3] );
  fmtseconds( ctime, timbuf[4] );
  cprintf( lin,col,0, sfmt3, "Cpu time",
	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  
  lin++;
  for ( i = 0; i < 4; i++ )
    {
      j = Teams[i].stats[TSTAT_SECONDS];
      if ( j <= 0 )
	x[i] = 0.0;
      else
	x[i] = 100.0 * ((real) Teams[i].stats[TSTAT_CPUSECONDS] / (real) j);
    }
  if ( etime <= 0 )
    x[4] = 0.0;
  else
    x[4] = 100.0 * (real) ctime / (real)etime;
  cprintf( lin,col,0, pfmt2, "Cpu usage", x[0], x[1], x[2], x[3], x[4] );

  lin++;
  cprintf( lin,col,0, dfmt2, "Phaser shots",
	 Teams[0].stats[TSTAT_PHASERS], Teams[1].stats[TSTAT_PHASERS],
	 Teams[2].stats[TSTAT_PHASERS], Teams[3].stats[TSTAT_PHASERS],
	 Teams[0].stats[TSTAT_PHASERS] + Teams[1].stats[TSTAT_PHASERS] +
	 Teams[2].stats[TSTAT_PHASERS] + Teams[3].stats[TSTAT_PHASERS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Torps fired",
	 Teams[0].stats[TSTAT_TORPS], Teams[1].stats[TSTAT_TORPS],
	 Teams[2].stats[TSTAT_TORPS], Teams[3].stats[TSTAT_TORPS],
	 Teams[0].stats[TSTAT_TORPS] + Teams[1].stats[TSTAT_TORPS] +
	 Teams[2].stats[TSTAT_TORPS] + Teams[3].stats[TSTAT_TORPS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Armies bombed",
	 Teams[0].stats[TSTAT_ARMBOMB], Teams[1].stats[TSTAT_ARMBOMB],
	 Teams[2].stats[TSTAT_ARMBOMB], Teams[3].stats[TSTAT_ARMBOMB],
	 Teams[0].stats[TSTAT_ARMBOMB] + Teams[1].stats[TSTAT_ARMBOMB] +
	 Teams[2].stats[TSTAT_ARMBOMB] + Teams[3].stats[TSTAT_ARMBOMB] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Armies captured",
	 Teams[0].stats[TSTAT_ARMSHIP], Teams[1].stats[TSTAT_ARMSHIP],
	 Teams[2].stats[TSTAT_ARMSHIP], Teams[3].stats[TSTAT_ARMSHIP],
	 Teams[0].stats[TSTAT_ARMSHIP] + Teams[1].stats[TSTAT_ARMSHIP] +
	 Teams[2].stats[TSTAT_ARMSHIP] + Teams[3].stats[TSTAT_ARMSHIP] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Planets taken",
	 Teams[0].stats[TSTAT_CONQPLANETS], Teams[1].stats[TSTAT_CONQPLANETS],
	 Teams[2].stats[TSTAT_CONQPLANETS], Teams[3].stats[TSTAT_CONQPLANETS],
	 Teams[0].stats[TSTAT_CONQPLANETS] + Teams[1].stats[TSTAT_CONQPLANETS] +
	 Teams[2].stats[TSTAT_CONQPLANETS] + Teams[3].stats[TSTAT_CONQPLANETS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Coups",
	 Teams[0].stats[TSTAT_COUPS], Teams[1].stats[TSTAT_COUPS],
	 Teams[2].stats[TSTAT_COUPS], Teams[3].stats[TSTAT_COUPS],
	 Teams[0].stats[TSTAT_COUPS] + Teams[1].stats[TSTAT_COUPS] +
	 Teams[2].stats[TSTAT_COUPS] + Teams[3].stats[TSTAT_COUPS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Genocides",
	 Teams[0].stats[TSTAT_GENOCIDE], Teams[1].stats[TSTAT_GENOCIDE],
	 Teams[2].stats[TSTAT_GENOCIDE], Teams[3].stats[TSTAT_GENOCIDE],
	 Teams[0].stats[TSTAT_GENOCIDE] + Teams[1].stats[TSTAT_GENOCIDE] +
	 Teams[2].stats[TSTAT_GENOCIDE] + Teams[3].stats[TSTAT_GENOCIDE] );
  
  for ( i = 0; i < 4; i++ )
    if ( Teams[i].couptime == 0 )
      timbuf[i][0] = EOS;
    else
      sprintf( timbuf[i], "%d", Teams[i].couptime );
  
  if ( ! godlike )
    {
      for ( i = 0; i < 4; i++ )
	if ( team != i )
	  c_strcpy( "-", timbuf[i] );
	else if ( ! Teams[i].coupinfo && timbuf[i][0] != EOS )
	  c_strcpy( "?", timbuf[i] );
    }
  
  timbuf[4][0] = EOS;
  
  lin++;
  cprintf( lin,col,0, sfmt3, "Coup time",
 	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

  cprintf( MSG_LIN2, 0, ALIGN_CENTER, "#%d#%s", NoColor, MTXT_DONE);

  return NODE_OK;
}  

static int nTeamlIdle(void)
{
  int pkttype;
  Unsgn8 buf[PKT_MAXSIZE];
  int sockl[2] = {cInfo.sock, cInfo.usock};

  while ((pkttype = waitForPacket(PKT_FROMSERVER, sockl, PKT_ANYPKT,
                                  buf, PKT_MAXSIZE, 0, NULL)) > 0)
    processPacket(buf);

  if (pkttype < 0)          /* some error */
    {
      clog("nTeamlIdle: waiForPacket returned %d", pkttype);
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
  
static int nTeamlInput(int ch)
{
  /* go back */
  switch (retnode)
    {
    case DSP_NODE_CP:
      setONode(NULL);
      nCPInit();
      break;
    case DSP_NODE_MENU:
      setONode(NULL);
      nMenuInit();
      break;

    default:
      clog("nTeamlInput: invalid return node: %d, going to DSP_NODE_MENU",
           retnode);
      setONode(NULL);
      nMenuInit();
      break;
    }

  /* NOTREACHED */
  return NODE_OK;
}

