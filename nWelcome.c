/* 
 * Welcome to this node
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
#include "client.h"
#include "nWelcome.h"
#include "nMenu.h"
#include "gldisplay.h"
#include "node.h"

#define S_DONE         0        /* nothing to display */
#define S_GREETINGS    1        /* GREETINGS - new user */
#define S_ERROR        2        /* some problem */

static int state;

static int fatal = FALSE;
static int newuser = FALSE;
static spAck_t *sack = NULL;
static time_t snooze = (time_t)0;          /* sleep time */

static string sorry1="I'm sorry, but the game is closed for repairs right now.";
static string sorry2="I'm sorry, but there is no room for a new player right now.";
static string sorryn="Please try again some other time.  Thank you.";
static string selected_str="You have been selected to command a";
static string starship_str=" starship.";
static string prepare_str="Prepare to be beamed aboard...";



static int nWelcomeDisplay(dspConfig_t *);

static scrNode_t nWelcomeNode = {
  nWelcomeDisplay,              /* display */
  NULL,                         /* idle */
  NULL                          /* input */
};

/*  gretds - block letter "greetings..." */
/*  SYNOPSIS */
/*    gretds */
static void gretds()
{
  int col,lin;
  string g1=" GGG   RRRR   EEEEE  EEEEE  TTTTT   III   N   N   GGG    SSSS";
  string g2="G   G  R   R  E      E        T      I    NN  N  G   G  S";
  string g3="G      RRRR   EEE    EEE      T      I    N N N  G       SSS";
  string g4="G  GG  R  R   E      E        T      I    N  NN  G  GG      S  ..  ..  ..";
  string g5=" GGG   R   R  EEEEE  EEEEE    T     III   N   N   GGG   SSSS   ..  ..  ..";
  
  col = (int)(Context.maxcol - strlen(g5)) / (int)2;
  lin = 1;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g1);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g2);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g3);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g4);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g5);
  
  return;
  
}

void nWelcomeInit(void)
{
  spClientStat_t *scstat = NULL;
  int pkttype;
  Unsgn8 buf[PKT_MAXSIZE];

  /* now look for SP_CLIENTSTAT or SP_ACK */
  if ((pkttype = 
       readPacket(PKT_FROMSERVER, cInfo.sock, buf, PKT_MAXSIZE, 10)) <= 0)
    {
      clog("nWelcomeInit: read failed\n");
      fatal = TRUE;
      return;
    }

  switch (pkttype)
    {
    case SP_CLIENTSTAT:
      scstat = (spClientStat_t *)buf;

      Context.unum = (int)ntohs(scstat->unum);
      Context.snum = scstat->snum;
      Ships[Context.snum].team = scstat->team;
      break;
    case SP_ACK:
      sack = (spAck_t *)buf;
      state = S_ERROR;
      snooze = (time(0) + 2);
      return;

      break;
    default:
      clog("nWelcomeInit: got unexpected packet type %d\n", pkttype);
      fatal = TRUE;
      state = S_ERROR;
      return;

      break;
    }

  if (pkttype == SP_CLIENTSTAT && (scstat->flags & SPCLNTSTAT_FLAG_NEW))
    {
      newuser = TRUE;
      state = S_GREETINGS;
      snooze = (time(0) + 3);
    }
  else
    {
      newuser = FALSE;
      state = S_DONE;           /* need to wait for user packet */
    }

   setNode(&nWelcomeNode);

  return;
}


static int nWelcomeDisplay(dspConfig_t *dsp)
{
  Unsgn8 buf[PKT_MAXSIZE];
  int team, col = 0;
  time_t t = time(0);

  if (fatal)
    return NODE_EXIT;           /* see ya! */

  if (snooze)
    {
      if (sack)                 /* an error */
        {
          if (t > snooze)     /* time to go */
            return NODE_EXIT;
        }
      else
        {                       /* new user */
          if (t > snooze)
            {
              state = S_DONE;
              snooze = 0;
              return NODE_OK;
            }
        }
    }

  switch (state)
    {
    case S_GREETINGS:
      /* Must be a new player. */
      if ( ConqInfo->closed )
        {
          /* Can't enroll if the game is closed. */
          cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
          cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
        }
      else
        {
          team = Ships[Context.snum].team;
          gretds();                 /* 'GREETINGS' */
          
          if ( vowel( Teams[team].name[0] ) )
            cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s%c #%d#%s #%d#%s",
                    InfoColor,selected_str,'n',CQC_A_BOLD,Teams[team].name,
                    InfoColor,starship_str);
          else
            cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s #%d#%s #%d#%s",
                    InfoColor,selected_str,CQC_A_BOLD,Teams[team].name,
                    InfoColor,starship_str);

          cprintf(MSG_LIN2/2+1,0,ALIGN_CENTER,"#%d#%s",
                  InfoColor, prepare_str );
        }

      return NODE_OK;
      break;

    case S_ERROR:
      switch (sack->code)
        {
        case PERR_CLOSED:
          cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
          cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
          break;

        case PERR_REGISTER:
          cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry2 );
          cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
          break;

        case PERR_NOSHIP:
          cprintf(MSG_LIN2/2, 0, ALIGN_CENTER,
                  "I'm sorry, but there are no ships available right now.");
          cprintf((MSG_LIN2/2)+1, 0, ALIGN_CENTER, 
                  sorryn);
          break;

        default:
          clog("nWelcomeDisplay: unexpected ACK code %d\n", sack->code);
          break;
        }

      return NODE_OK;
      break;

    case S_DONE:
      if (waitForPacket(PKT_FROMSERVER, cInfo.sock, SP_USER, buf, PKT_MAXSIZE,
                        15, NULL) <= 0)
        {
          clog("nWelcomeDisplay: waitforpacket SP_USER returned error");
          return NODE_EXIT;
        }
      else
        procUser(buf);

      nMenuInit();
      return NODE_OK;
      break;

      clog("nWelcomeDisplay: unknown state %d", state);
      return NODE_EXIT;
      break;
    }

  return NODE_OK;
}  
  

