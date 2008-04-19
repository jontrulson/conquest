/* 
 * play node.  Sets up for the cockpit (CP).
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
#include "glmisc.h"
#include "node.h"
#include "client.h"
#include "record.h"

#include "nMenu.h"
#include "nCP.h"
#include "nPlay.h"
#include "cqkeys.h"

#define S_DONE       0          /* ready to play */
#define S_NSERR      1          /* _newship error */
#define S_SELSYS     2          /* select system */
#define S_MENU       3          /* go back to menu */

static int state;
static int fatal = FALSE;
static spAck_t *sack = NULL;
static spClientStat_t scstat = {};
static int shipinited = FALSE;   /* whether we've done _newship() yet */
static int owned[NUMPLAYERTEAMS]; 

static int nPlayDisplay(dspConfig_t *);
static int nPlayIdle(void);
static int nPlayInput(int ch);

static scrNode_t nPlayNode = {
  nPlayDisplay,               /* display */
  nPlayIdle,                  /* idle */
  nPlayInput,                  /* input */
  NULL,                         /* minput */
  NULL                          /* animQue */
};

/* select a system to enter */
static void selectentry( Unsgn8 esystem )
{
  int i; 
  char cbuf[BUFFER_SIZE];
  
  /* First figure out which systems we can enter from. */
  for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    if (esystem & (1 << i))
      {
	owned[i] = TRUE;
      }
    else
      owned[i] = FALSE;
  
  /* Prompt for a decision. */
  c_strcpy( "Enter which system", cbuf );
  for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    if ( owned[i] )
      {
	appstr( ", ", cbuf );
	appstr( Teams[i].name, cbuf );
      }
  /* Change first comma to a colon. */
  i = c_index( cbuf, ',' );
  if ( i != ERR )
    cbuf[i] = ':';
  
  cprintf(12, 0, ALIGN_CENTER, cbuf);

  return;
}



/*  _newship - here we will await a ClientStat from the server (indicating
    our possibly new ship), or a NAK indicating a problem.
*/
static int _newship( int unum, int *snum )
{
  int pkttype;
  char buf[PKT_MAXSIZE];
  int sockl[2] = {cInfo.sock, cInfo.usock};

  /* here we will wait for ack's or a clientstat pkt. Acks indicate an
     error.  If the clientstat pkt's esystem is !0, we need to prompt
     for the system to enter and send it in a CP_COMMAND:CPCMD_ENTER
     pkt. */


  while (TRUE)
    {
      if ((pkttype = waitForPacket(PKT_FROMSERVER, sockl, PKT_ANYPKT,
				   buf, PKT_MAXSIZE, 60, NULL)) < 0)
	{
	  clog("nPlay: _newship: waitforpacket returned %d", pkttype);
          fatal = TRUE;
	  return FALSE;
	}
      
      switch (pkttype)
	{
	case 0:			/* timeout */
	  return FALSE;
	  break;
	  
	case SP_ACK:		/* bummer */
	  sack = (spAck_t *)buf;
          state = S_NSERR;
	  
	  return FALSE;		/* always a failure */
	  break;
	  
	case SP_CLIENTSTAT:
          {
            spClientStat_t *scstatp;

            if ((scstatp = chkClientStat(buf)))
              {
                scstat = *scstatp; /* make a copy */
                /* first things first */
                Context.unum = scstat.unum;
                Context.snum = scstat.snum;
                Ships[Context.snum].team = scstat.team;
                
                return TRUE;
              }
            else
              {
                clog("nPlay: _newship: invalid CLIENTSTAT");
                return FALSE;
              }
          }
	  break;
	  
	  /* we might get other packets too */
	default:
	  processPacket(buf);
	  break;
	}
    }

  /* if we are here, something unexpected happened */
  return FALSE;			/* NOTREACHED */

  
}

void nPlayInit(void)
{
  state = S_SELSYS;               /* default */
  shipinited = FALSE;
  /* let the server know our intentions */
  if (!sendCommand(CPCMD_ENTER, 0))
    fatal = TRUE;

  setNode(&nPlayNode);

  return;
}


static int nPlayDisplay(dspConfig_t *dsp)
{
  char cbuf[BUFFER_SIZE];
  int i, j;

  if (fatal)
    return NODE_EXIT;

  if (state == S_SELSYS)
    {                 
      if (!shipinited)
        {                       /* need to call _newship */
          shipinited = TRUE;
          if (!_newship( Context.unum, &Context.snum ))
            {
              state = S_NSERR;
              return NODE_OK;
            }
        }
          
      if (!scstat.esystem)
        {                       /* we are ready  */
          state = S_DONE;
          return NODE_OK;
        }
      else
        {                       /* need to display/get a selection */
          selectentry(scstat.esystem);
        }
    }
  else if (state == S_NSERR)
    {
      if (sack)
        {
          switch (sack->code)
            {
            case PERR_FLYING:
              sprintf(cbuf, "You're already playing on another ship.");
              cprintf(5,0,ALIGN_CENTER,"#%d#%s",InfoColor, cbuf);
              Ships[Context.snum].status = SS_RESERVED;
              break;
              
            case PERR_TOOMANYSHIPS:
              i = MSG_LIN2/2;
              cprintf(i, 0, ALIGN_CENTER, 
                      "I'm sorry, but you're playing on too many ships right now.");
              i = i + 1;
              c_strcpy( "You are only allowed to fly ", cbuf );
              j = Users[Context.unum].multiple;
              appint( j, cbuf );
              appstr( " ship", cbuf );
              if ( j != 1 )
                appchr( 's', cbuf );
              appstr( " at one time.", cbuf );
              cprintf(i, 0, ALIGN_CENTER, cbuf);
              break;
              
            default:
              clog("nPlayDisplay: _newship: unexpected ack code %d",
                   sack->code);
              break;
            }
        }
    }


  return NODE_OK;
}  
  
static int nPlayIdle(void)
{
  if (state == S_DONE)
    {
      Context.entship = TRUE;
      Ships[Context.snum].sdfuse = 0;       /* zero self destruct fuse */
      grand( &Context.msgrand );            /* initialize message timer */
      Context.leave = FALSE;                /* assume we won't want to bail */
      Context.redraw = TRUE;                /* want redraw first time */
      Context.msgok = TRUE;         /* ok to get messages */
      
      Context.display = TRUE;               /* ok to display */
      
      /* start recording if neccessary */
      if (Context.recmode == RECMODE_STARTING)
        {
          if (recordInitOutput(Context.unum, getnow(NULL, 0), Context.snum,
                               FALSE))
            {
              Context.recmode = RECMODE_ON;
            }
          else
            Context.recmode = RECMODE_OFF;
        }
      
      /* need to tell the server to resend all the crap it already
         sent in menu - our ship may have chenged */
      sendCommand(CPCMD_RELOAD, 0);

      nCPInit(TRUE);            /* play */
    }
  else if (state == S_MENU)
    nMenuInit();

  return NODE_OK;
}

static int nPlayInput(int ch)
{
  int i;
  unsigned char c = CQ_CHAR(ch);

  switch (state)
    {
    case S_SELSYS:              /* we are selecting our system */
      {
        switch  ( ch )
          {
          case TERM_NORMAL:
          case TERM_ABORT:	/* doesn't like the choices ;-) */
            sendCommand(CPCMD_ENTER, 0);
            state = S_MENU;
            return NODE_OK;
            break;
          case TERM_EXTRA:
            /* Enter the home system. */
            sendCommand(CPCMD_ENTER, (Unsgn16)(1 << Ships[Context.snum].team));
            state = S_DONE;
            return NODE_OK;
            break;
          default:
            for ( i = 0; i < NUMPLAYERTEAMS; i++ )
              if ( Teams[i].teamchar == (char)toupper(c) && owned[i] )
                {
                  /* Found a good one. */
                  sendCommand(CPCMD_ENTER, (Unsgn16)(1 << i));
                  state = S_DONE;
                  return NODE_OK;
                }
            
            /* Didn't get a good one; complain and try again. */
            mglBeep(MGL_BEEP_ERR);
            break;
          }
      }
      
      break;

    case S_NSERR:               /* any key to return */
      nMenuInit();

      return NODE_OK;
      break;
    }

  return NODE_OK;
}

