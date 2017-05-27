#include "c_defs.h"

/************************************************************************
 *
 * conqreplay - replay a cqr recording for curses interface
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "ui.h"
#include "conqlb.h"
#include "conqutil.h"
#include "cd2lb.h"
#include "iolb.h"
#include "cumisc.h"
#include "record.h"
#include "playback.h"
#include "conf.h"

#include "protocol.h"
#include "packet.h"
#include "client.h"
#include "clientlb.h"
#include "display.h"

static void replay(void);
static void watch(void);
static int prompt_ship(char buf[], int *snum, int *normal);
static void toggle_line(int snum, int old_snum);
static void dowatchhelp(void);
static char *build_toggle_str(char *snum_str, int snum);


/* overlay the elapsed time, and current recFrameDelay */
void displayReplayData(void)
{
  char buf[128];
  time_t elapsed = (recCurrentTime - recStartTime);
  char *c;
  real percent;

  /* elapsed time */
  utFormatSeconds((int)elapsed, buf);
  c = &buf[2];			/* skip day count */
  if (elapsed <= 0)
    elapsed = 1;
  
  percent = ((real)elapsed / (real)recTotalElapsed ) * 100.0;

  /* current speed */
  if (pbSpeed == PB_SPEED_INFINITE)
    /* current frame delay */
    sprintf(buf, "%s (%d%%) INF", c, (int)percent);
  else
    sprintf(buf, "%s (%d%%) %2dx ", c, (int)percent, pbSpeed);

  cdputs( buf, DISPLAY_LINS + 1, 2);

  /* paused status */
  if (Context.recmode == RECMODE_PAUSED)
    cdputs( "PAUSED: Press [SPACE] to resume", DISPLAY_LINS + 2, 0);

  cdrefresh();

  return;
}


/* display a message - mostly ripped from mcuReadMsg() */
void displayMsg(Msg_t *themsg)
{
  char buf[MSGMAXLINE];
  unsigned int attrib = 0;

  if (Context.display == FALSE || Context.recmode != RECMODE_PLAYING)
    return;			/* don't display anything */

  buf[0] = '\0';

  if (Context.hascolor)
    {                           /* set up the attrib so msg's are cyan */
      attrib = CyanColor;
    }

  if (themsg)
    {
      clbFmtMsg(themsg->msgto, themsg->msgfrom, buf);
      
      appstr( ": ", buf );
      appstr( themsg->msgbuf, buf );
      
      uiPutColor(attrib);
      mcuPutMsg( buf, MSG_MSG );
      uiPutColor(0);
    }
  else
    {				/* just clear the message line */
      cdclrl( MSG_MSG, 1 );
    }

  return;

}

/* MAIN */
void conquestReplay(void)
{
  /* if recFrameDelay wasn't overridden, setup based on samplerate */
  if (recFrameDelay == -1.0)
    recFrameDelay = 1.0 / (real)recFileHeader.samplerate;

  replay();
  
  return;
  
}

/*  replay - show the main screen and call watch() */

static void replay(void)
{
  static int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];
  static char cfmt[MSGMAXLINE * 2];
  int ch;
  int done = FALSE;

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#%%s#%d#: %%s",
	      InfoColor,
	      GreenColor);

      sprintf(cfmt,
              "#%d#(#%d#%%c#%d#) - %%s",
              LabelColor,
              InfoColor,
              LabelColor);
    }

  cdclear();
  
  do 
    {
      dspReplayMenu();      
      cdclrl( MSG_LIN1, 2 );

      if ( ! iogtimed( &ch, 1.0 ) )
	continue; 

      /* got a char */

      switch ( ch )
        {
        case 'w':  /* start the voyeurism */
	  watch();
	  cdclear();
          break;

	case 'q':
	  done = TRUE;
	  break;

	case 'r':
	  /* first close the input file, free the common block,
	     then re-init */
	  pbFileSeek(recStartTime);
	  break;

        case '/':
	  mcuPlayList(TRUE, TRUE, 0);
	  cdclear();
	  break;
	}
    } while (!done);


  return;
  
}


/*  watch - peer over someone's shoulder */
/*  SYNOPSIS */
/*    watch */
static void watch(void)
{
  int ptype;
  int snum, tmp_snum, old_snum;
  int ch, normal;
  int msgrand;
  char buf[MSGMAXLINE];
  int live_ships = TRUE;
  int toggle_flg = FALSE;   /* jon no like the toggle line ... :-) */
  int upddsp = FALSE;

  normal = TRUE;

  if (!prompt_ship(buf, &snum, &normal))
    return;
  else
    {
      old_snum = tmp_snum = snum;
      Context.redraw = TRUE;
      cdclear();
      cdredo();
      utGrand( &msgrand );
      
      Context.snum = snum;		/* so display knows what to display */
      /*	  setopertimer();*/
      
      
      while (TRUE)
	{
	  if (Context.recmode == RECMODE_PLAYING)
	    if ((ptype = pbProcessIter()) == SP_NULL)
	      return;

	  Context.display = TRUE;
	  
	  if (toggle_flg)
	    toggle_line(snum,old_snum);
	  
	  if (Context.recmode == RECMODE_PLAYING || upddsp)
	    {
	      display(Context.snum);
	      displayReplayData();
              if (recMsg.msgbuf[0])
                displayMsg(&recMsg);
	      upddsp = FALSE;	/* use this for one-shots */
	    }
	  
	  if (!iogtimed(&ch, recFrameDelay))
	  continue;
	  
	  /* got a char */
	  cdclrl( MSG_LIN1, 2 );
	  switch ( ch )
	    {
	    case 'q': 
	      return;
	      break;
	    case 'h':
	      dowatchhelp();
	      Context.redraw = TRUE;
	      upddsp = TRUE;
	      break;
	      
	    case 'f':	/* move forward 30 seconds */
	      displayMsg(NULL);
	      pbFileSeek(recCurrentTime + 30);
	      upddsp = TRUE;
	      break;
	      
	    case 'F':	/* move forward 2 minutes */
	      displayMsg(NULL);
	      pbFileSeek(recCurrentTime + (2 * 60));
	      upddsp = TRUE;
	      break;
	      
	    case 'M':	/* toggle lr/sr */
              if (SMAP(Context.snum))
                SFCLR(Context.snum, SHIP_F_MAP);
              else
                SFSET(Context.snum, SHIP_F_MAP);
	      upddsp = TRUE;
	      break;
	      
	    case 'b':	/* move backward 30 seconds */
	      displayMsg(NULL);
	      cdputs( "Rewinding...", MSG_LIN1, 0);
	      cdrefresh();
	      pbFileSeek(recCurrentTime - 30);
	      cdclrl( MSG_LIN1, 1 );
	      upddsp = TRUE;
	      break;
	      
	    case 'B':	/* move backward 2 minutes */
	      displayMsg(NULL);
	      cdputs( "Rewinding...", MSG_LIN1, 0);
	      cdrefresh();
	      pbFileSeek(recCurrentTime - (2 * 60));
	      cdclrl( MSG_LIN1, 1 );
	      upddsp = TRUE;
	      break;

	    case 'r':	/* reset to beginning */
	      cdputs( "Rewinding...", MSG_LIN1, 0);
	      cdrefresh();
	      pbFileSeek(recStartTime);
	      cdclrl( MSG_LIN1, 1 );
	      upddsp = TRUE;
	      break;

	    case ' ':	/* pause/resume playback */
	      if (Context.recmode == RECMODE_PLAYING)
		{		/* pause */
		  Context.recmode = RECMODE_PAUSED;
		}
	      else 
		{		/* resume */
		  Context.recmode = RECMODE_PLAYING;
		}

	      upddsp = TRUE;
	      break;
		    
	    case 'n':		/* set recFrameDelay to normal playback
				   speed.*/
              pbSetPlaybackSpeed(1, recFileHeader.samplerate);
	      upddsp = TRUE;
	      break;

	      /* these seem backward, but it's easier to understand
		 the '+' is faster, and '-' is slower ;-) */
	    case '-':
              pbSetPlaybackSpeed(pbSpeed - 1, recFileHeader.samplerate);
	      upddsp = TRUE;
	      break;

	    case '+': 
	    case '=':
              pbSetPlaybackSpeed(pbSpeed + 1, recFileHeader.samplerate);
	      upddsp = TRUE;
	      break;

	    case TERM_REDRAW:	/* ^L */
	      cdclear();
	      Context.redraw = TRUE;
	      upddsp = TRUE;
	      break;


	    case 'w': /* look at any ship (live or not) if specifically
			 asked for */
	      tmp_snum = snum;
	      if (prompt_ship(buf, &snum, &normal)) 
		{
		  if (tmp_snum != snum) 
		    {
		      old_snum = tmp_snum;
		      tmp_snum = snum;
		      Context.redraw = TRUE;
		    }
		  Context.snum = snum;
		}
	      upddsp = TRUE;
	      break;

	    case '`':                 /* toggle between two ships */
	      if (normal || (!normal && old_snum > 0))
		{
		  if (old_snum != snum) 
		    {
		      tmp_snum = snum;
		      snum = old_snum;
		      old_snum = tmp_snum;
			  
		      Context.snum = snum;
		      Context.redraw = TRUE;
		      upddsp = TRUE;
		    }
		}
	      else
		cdbeep();
	      break;

	    case '/':                /* ship list - dwp */
	      mcuPlayList( TRUE, FALSE, 0 );
	      Context.redraw = TRUE;
	      upddsp = TRUE;
	      break;
	    case '!':
	      if (toggle_flg)
		toggle_flg = FALSE;
	      else
		toggle_flg = TRUE;
	      break;
	    case '>':  /* forward rotate ship numbers (including doomsday) - dwp */
	    case KEY_RIGHT:
	    case KEY_UP:
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
			  cdbeep();
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
		      if (normal)
			i = DISPLAY_DOOMSDAY;
		      else
			i = 1;
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

	      upddsp = TRUE;

	      break;
	    case '<':  /* reverse rotate ship numbers (including doomsday)  - dwp */
	    case KEY_LEFT:
	    case KEY_DOWN:
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
			  cdbeep();
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
		      if (normal)
			i = DISPLAY_DOOMSDAY;
		      else
			i = MAXSHIPS;
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
	      upddsp = TRUE;


	      break;
	    case TERM_ABORT:
	      return;
	      break;
	    default:
	      cdbeep();
	      mcuPutMsg( "Type h for help.", MSG_LIN2 );
	      break;
	    }
	} /* end while */
    } /* end else */

  /* NOTREACHED */
  
}

static int prompt_ship(char buf[], int *snum, int *normal)
{
  int tch;
  int tmpsnum = 0;
  char *pmt="Watch which ship (<cr> for doomsday)? ";
  char *nss="No such ship.";

  tmpsnum = *snum;

  cdclrl( MSG_LIN1, 2 );

  if (recFileHeader.snum == 0)
    buf[0] = 0;
  else
    sprintf(buf, "%d", recFileHeader.snum);

  tch = cdgetx( pmt, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, TRUE );
  cdclrl( MSG_LIN1, 1 );

  if ( tch == TERM_ABORT )
    {
      return(FALSE); /* dwp */
    }
  
  /* 
   * DWP - Removed old code to watch doomsday machine.
   * Watch it with conqlb.c display() now.
   */
  
  *normal = ( tch != TERM_EXTRA );		/* line feed means debugging */

  utDeleteBlanks( buf );

  if ( strlen( buf ) == 0 ) 
    {              /* watch doomsday machine */
      tmpsnum = DISPLAY_DOOMSDAY;
      *normal = TRUE;		/* doomsday doesn't have a debugging view */
    }
  else
    {
      if (!utIsDigits(buf))
	{
	  cdputs( nss, MSG_LIN2, 1 );
	  cdmove( 1, 1 );
	  cdrefresh();
	  utSleep( 1.0 );
	  return(FALSE); /* dwp */
	}
      utSafeCToI( &tmpsnum, buf, 0 );	/* ignore return status */
    }

  if ( (tmpsnum < 1 || tmpsnum > MAXSHIPS) && tmpsnum != DISPLAY_DOOMSDAY )
    {
      cdputs( nss, MSG_LIN2, 1 );
      cdmove( 1, 1 );
      cdrefresh();
      utSleep( 1.0 );
      return(FALSE); /* dwp */
    }

  *snum = tmpsnum;
  return(TRUE);

} /* end prompt_ship() */


/*  dowatchhelp - display a list of commands while watching a ship */
/*  SYNOPSIS */
/*    dowatchhelp( void ) */
static void dowatchhelp(void)
{
  int ch;

  cdclear();

  dspReplayHelp();

  mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
  cdrefresh();
  while ( ! iogtimed( &ch, 1.0 ) )
    ;
  cdclrl( MSG_LIN2, 1 );
  
  return;
  
}


static void toggle_line(int snum, int old_snum)
{

  char *frmt_str = "(') = Toggle %s:%s";
  char snum_str[MSGMAXLINE];
  char old_snum_str[MSGMAXLINE];
  char buf[MSGMAXLINE];
  
  build_toggle_str(snum_str,snum);
  build_toggle_str(old_snum_str,old_snum);
  
  sprintf(buf,frmt_str,snum_str, old_snum_str);
  cdputs( buf, MSG_LIN1, (Context.maxcol-(strlen(buf))));  /* at end of line */
  
  return;

}

static char *build_toggle_str(char *snum_str, int snum)
{
  static char *doomsday_str = "DM";
  static char *deathstar_str = "DS";
  static char *unknown_str = "n/a";
  
  if (snum > 0 && snum <= MAXSHIPS)
    {          /* ship */
      sprintf(snum_str,"%c%d", Teams[Ships[snum].team].teamchar, snum);
    }
  else if (snum < 0 && -snum <= NUMPLANETS) 
    {  /* planet */

      sprintf(snum_str, "%c%c%c", 
	      Planets[-snum].name[0], 
	      Planets[-snum].name[1], 
	      Planets[-snum].name[2]);
    }
  else if (snum == DISPLAY_DOOMSDAY)          /* specials */
    strcpy(snum_str,doomsday_str);
  else if (snum == DISPLAY_DEATHSTAR)
    strcpy(snum_str,deathstar_str);
  else                                        /* should not get here */
    strcpy(snum_str,unknown_str);
  
  return(snum_str);

}

