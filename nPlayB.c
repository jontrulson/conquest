/* 
 * playback cockpit node
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
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "record.h"
#include "playback.h"
#include "ibuf.h"
#include "prm.h"
#include "cqkeys.h"

#include "nDead.h"
#include "nCPHelp.h"
#include "nPlayB.h"
#include "nPlayBMenu.h"
#include "nPlayBHelp.h"
#include "nShipl.h"
#include "nPlanetl.h"
#include "nUserl.h"
#include "nHistl.h"
#include "nTeaml.h"

#include <assert.h>

#include "glmisc.h"
#include "glfont.h"
#include "render.h"

#include "nPlayB.h"

#define cp_putmsg(str, lin)  setPrompt(lin, NULL, NoColor, str, NoColor)

#define S_NONE         0
#define S_WATCH        1
static int state;

/* the current prompt */
static prm_t prm;
static int prompting = FALSE;

/* misc buffers */
static char cbuf[MID_BUFFER_SIZE];

static int live_ships = TRUE;
static int old_snum = 1;
static char *nss = NULL;        /* no such ship */

extern dspData_t dData;

static int nPlayBDisplay(dspConfig_t *);
static int nPlayBIdle(void);
static int nPlayBInput(int ch);

static scrNode_t nPlayBNode = {
  nPlayBDisplay,               /* display */
  nPlayBIdle,                  /* idle */
  nPlayBInput,                  /* input */
  NULL                          /* next */
};


static void set_header(int snum)
{

  char *heading_fmt = "%s %c%d (%s)%s";
  char *doom_fmt = "%s (%s)";
  char *closed_str1 = "GAME CLOSED -";
  char *robo_str1 = "ROBOT (external)";
  char *robo_str2 = "ROBOT";
  char *ship_str1 = "SHIP";
  static char hbuf[MSGMAXLINE];
  char ssbuf[MSGMAXLINE];
  
  hbuf[0] = EOS;
  ssbuf[0] = EOS;
  
  appstr( ", ", ssbuf );
  appsstatus( Ships[snum].status, ssbuf);
  
  if ( ConqInfo->closed) 
    {
      sprintf(hbuf, heading_fmt, closed_str1, 
	      Teams[Ships[snum].team].teamchar, 
	      snum,
	      Ships[snum].alias, ssbuf); 
    }
  else if ( SROBOT(snum) )
    {
      if (ConqInfo->externrobots == TRUE) 
	{
	  sprintf(hbuf, heading_fmt, robo_str1, 
		  Teams[Ships[snum].team].teamchar, snum,
		  Ships[snum].alias, ssbuf); 
	}
      else 
	{
	  sprintf(hbuf, heading_fmt, robo_str2, 
		  Teams[Ships[snum].team].teamchar, snum,
		  Ships[snum].alias, ssbuf);
	}
    }
  else 
    {
      if (snum == DISPLAY_DOOMSDAY)
        sprintf(hbuf, doom_fmt, Doomsday->name, 
                (Doomsday->status == DS_LIVE) ? DS_LIVE_STR: DS_OFF_STR);
      else
        sprintf(hbuf, heading_fmt, ship_str1, Teams[Ships[snum].team].teamchar,
                snum,
                Ships[snum].alias, ssbuf);
    }
  
  setRecId(hbuf);

}

void set_rectime(void)
{
  char buf[128];
  static char hbuf[128];
  time_t elapsed = (currTime - startTime);
  char *c;

  /* elapsed time */
  fmtseconds((int)elapsed, buf);
  c = &buf[2];			/* skip day count */
  
  /* current frame delay */
  sprintf(hbuf, "%s  %2.3fs", c, framedelay);

  setRecTime(hbuf);

  return;
}



void nPlayBInit(void)
{
  prompting = FALSE;
  state = S_NONE;

  setNode(&nPlayBNode);

  return;
}


static int nPlayBDisplay(dspConfig_t *dsp)
{
  char buf[MSGMAXLINE];
      
  /* Viewer */
  renderViewer();

  /* Main/Hud */
  set_header(Context.snum);
  set_rectime();
  renderHud();

  if (recMsg.msgbuf[0])
    {
      clbFmtMsg(recMsg.msgto, recMsg.msgfrom, buf);
      appstr( ": ", buf );
      appstr( recMsg.msgbuf, buf );
      
      setPrompt(MSG_MSG, NULL, NoColor, buf, CyanColor);
    }

  mglOverlayQuad();             /* render the overlay bg */
  
  if (prompting)
    setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

  return NODE_OK;
}  
  
static int nPlayBIdle(void)
{
  int ptype;

  /* GL.c:renderFrame() will wait the appropriate time */

  /* read to the next Frame */
  if (Context.recmode != RECMODE_PAUSED)
    if (Context.recmode == RECMODE_PLAYING)
      if ((ptype = pbProcessIter()) == SP_NULL)
        nPlayBMenuInit();

  return NODE_OK;
}

static int nPlayBInput(int ch)
{
  int irv;
  int snum = Context.snum;
  int tmp_snum = 1;

  if (prompting)
    {
      int tmpsnum;
      irv = prmProcInput(&prm, ch);

      if (irv > 0)
        {
          if (ch == TERM_ABORT)
            {
              state = S_NONE;
              prompting = False;

              return NODE_OK;
            }

          delblanks( prm.buf );
          if ( strlen( prm.buf ) == 0 )
            {              /* watch doomsday machine */
              tmpsnum = DISPLAY_DOOMSDAY;
            }
          else
            {
              if ( alldig( prm.buf ) != TRUE )
                {
                  state = S_NONE;
                  prompting = False;

                  nss = "No such ship.";
                  return NODE_OK; 
                }
              safectoi( &tmpsnum, prm.buf, 0 );     /* ignore return status */
            }

          if ( (tmpsnum < 1 || tmpsnum > MAXSHIPS) && 
               tmpsnum != DISPLAY_DOOMSDAY )
            {
              state = S_NONE;
              prompting = False;

              nss = "No such ship.";
              return NODE_OK;
            }

          prompting = FALSE;
          state = S_NONE;

          Context.snum = tmpsnum;
          clrPrompt(MSG_LIN1);
          nPlayBInit();         /* start playing */
        }

      if (nss)
        setPrompt(MSG_LIN2, NULL, NoColor, nss, NoColor);

      return NODE_OK;
    }


  nss = NULL;
  clrPrompt(MSG_LIN1);

  switch (ch)
    {
    case 'q': 
      nPlayBMenuInit();
      return NODE_OK;
      break;
    case 'h':
      setONode(nPlayBHelpInit(FALSE));
      break;
    case 'f':	/* move forward 30 seconds */
      cp_putmsg(NULL, MSG_LIN1);
      pbFileSeek(currTime + 30);
      break;
      
    case 'F':	/* move forward 2 minutes */
      cp_putmsg(NULL, MSG_LIN1);
      pbFileSeek(currTime + (2 * 60));
      break;
      
    case 'M':	/* toggle lr/sr */
      if (SMAP(Context.snum))
        SFCLR(Context.snum, SHIP_F_MAP);
      else
        SFSET(Context.snum, SHIP_F_MAP);
      break;
      
    case 'b':	/* move backward 30 seconds */
      cp_putmsg("Rewinding...", MSG_LIN1);
      pbFileSeek(currTime - 30);
      cp_putmsg(NULL, MSG_LIN1);
      break;
      
    case 'B':	/* move backward 2 minutes */
      cp_putmsg("Rewinding...", MSG_LIN1);
      pbFileSeek(currTime - (2 * 60));
      cp_putmsg(NULL, MSG_LIN1);
      break;
      
    case 'r':	/* reset to beginning */
      cp_putmsg("Rewinding...", MSG_LIN1);
      pbFileSeek(startTime);
      cp_putmsg(NULL, MSG_LIN1);
      break;
      
    case ' ':	/* pause/resume playback */
      if (Context.recmode == RECMODE_PLAYING)
        {		/* pause */
          Context.recmode = RECMODE_PAUSED;
          cp_putmsg("PAUSED: Press [SPACE] to resume", MSG_LIN1);
        }
      else 
        {		/* resume */
          Context.recmode = RECMODE_PLAYING;
          cp_putmsg(NULL, MSG_LIN1);
        }
      
      break;
      
    case 'n':		/* set framedelay to normal playback
                           speed.*/
      framedelay = 1.0 / (real)fhdr.samplerate;
      break;
      
      /* these seem backward, but it's easier to understand
         the '+' is faster, and '-' is slower ;-) */
    case '-':
      /* if it's at 0, we should still
         be able to double it. sorta. */
      if (framedelay == 0.0) 
        framedelay = 0.001;
      framedelay *= 2;
      if (framedelay > 10.0) /* really, 10s is a *long* time
                                between frames... */
        framedelay = 10.0;
      break;
      
    case '+': 
    case '=':
      if (framedelay != 0)
        {		/* can't divide 0 in our universe */
          framedelay /= 2;
          if (framedelay < 0.0)
            framedelay = 0.0;
        }
      break;
      
    case 'w':
      state = S_WATCH;
      if (fhdr.snum == 0)
        {
          cbuf[0] = EOS;
          prm.preinit = False;

        }
      else
        {
          sprintf(cbuf, "%d", fhdr.snum);
          prm.preinit = True;
        }

      prm.buf = cbuf;
      prm.buflen = MSGMAXLINE;
      prm.pbuf = "Watch which ship (<cr> for doomsday)? ";
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prompting = TRUE;

      break;

    case '`':                 /* toggle between two ships */
      if (old_snum != snum) 
        {
          
          tmp_snum = snum;
          snum = old_snum;
          old_snum = tmp_snum;
          
          Context.snum = snum;
        }
      else
        mglBeep();

      break;

    case '/':
      setONode(nShiplInit(DSP_NODE_PLAYB, FALSE));
      return NODE_OK;
      break;

    case '>':  /* forward rotate ship numbers (including doomsday) - dwp */
    case CQ_KEY_RIGHT:
    case CQ_KEY_UP:
      while (TRUE)
        {
          int i;
          
          if (live_ships)
            {	/* we need to make sure that there is
                   actually something alive or an
                   infinite loop will result... */
              int foundone = FALSE;
              
              for (i=1; i <= MAXSHIPS; i++)
                {
                  if (clbStillAlive(i))
                    {
                      foundone = TRUE;
                    }
                }
              if (foundone == FALSE)
                {	/* check the doomsday machine */
                  if (Doomsday->status == DS_LIVE)
                    foundone = TRUE;
                }
              
              if (foundone == FALSE)
                {
                  mglBeep();
                  break; /* didn't find one, beep, leave everything
                            alone*/
                }
            }
          
          if (snum == DISPLAY_DOOMSDAY)
            {	  /* doomsday - wrap around to first ship */
              i = 1;
            }
          else	
            i = snum + 1;
          
          if (i > MAXSHIPS)
            {	/* if we're going past
                   now loop thu specials (only doomsday for
                   now... ) */
              i = DISPLAY_DOOMSDAY;
            }
          
          snum = i;
          
          Context.redraw = TRUE;
          
          if (live_ships)
            if ((snum > 0 && clbStillAlive(snum)) || 
                (snum == DISPLAY_DOOMSDAY && Doomsday->status == DS_LIVE))
              {
                Context.snum = snum;
                break;
              }
            else
              continue;
          else
            {
              Context.snum = snum;
              break;
            }
        }
      
      break;
    case '<':  /* reverse rotate ship numbers (including doomsday)  - dwp */
    case CQ_KEY_LEFT:
    case CQ_KEY_DOWN:
      while (TRUE)
        {
          int i;
          
          if (live_ships)
            {	/* we need to make sure that there is
                   actually something alive or an
                   infinite loop will result... */
              int foundone = FALSE;
              
              for (i=1; i <= MAXSHIPS; i++)
                {
                  if (clbStillAlive(i))
                    {
                      foundone = TRUE;
                    }
                }
              if (foundone == FALSE)
                {	/* check the doomsday machine */
                  if (Doomsday->status == DS_LIVE)
                    foundone = TRUE;
                }
              
              if (foundone == FALSE)
                {
                  mglBeep();
                  break; /* didn't find one, beep, leave everything
                            alone*/
                }
            }
          
          
          if (snum == DISPLAY_DOOMSDAY)
            {	  /* doomsday - wrap around to last ship */
              i = MAXSHIPS;
            }
          else	
            i = snum - 1;
          
          if (i <= 0)
            {	/* if we're going past
                   now loop thu specials (only doomsday for
                   now... )*/
              i = DISPLAY_DOOMSDAY;
            }
          
          snum = i;
          
          Context.redraw = TRUE;
          
          if (live_ships)
            if ((snum > 0 && clbStillAlive(snum)) || 
                (snum == DISPLAY_DOOMSDAY && Doomsday->status == DS_LIVE))
              {
                Context.snum = snum;
                break;
              }
            else
              continue;
          else
            {
              Context.snum = snum;
              break;
            }
        }
      
      break;
    case TERM_ABORT:
      nPlayBMenuInit();
      return NODE_OK;
      break;
    default:
      mglBeep();
      cp_putmsg( "Type h for help.", MSG_LIN2 );
      break;
    }

  return NODE_OK;
}

