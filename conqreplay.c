#include "c_defs.h"

/************************************************************************
 *
 * conqreplay - replay a cqr recording
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "display.h"
#include "record.h"
#include "conf.h"
#include "datatypes.h"
#include "protocol.h"
#include "packet.h"
#include "client.h"
#include "clientlb.h"

static fileHeader_t fhdr;

static real framedelay = -1.0;	/* delaytime between frames */

static Unsgn32 frameCount = 0;

static char *rfname;		/* game file we are playing */

static time_t startTime = 0;
static time_t currTime = 0;

static time_t totElapsed = 0;	/* total game time */

static void replay(void);

void printUsage(void)
{
  fprintf(stderr, "usage: conqreplay [ -d <dly> ] -f <cqr file>\n");
  fprintf(stderr, "       -f <cqr>\t\tspecify the file to replay\n");
  fprintf(stderr, "       -d <dly>\t\tspecify the frame delay\n");

  return;
}

/* create and map our own version of the common block */
static int map_lcommon()
{
  /* a parallel universe, it is */
  fake_common();
  initeverything();
  initmsgs();
  *CBlockRevision = COMMONSTAMP;
  ConqInfo->closed = FALSE;
  Driver->drivstat = DRS_OFF;
  Driver->drivpid = 0;
  Driver->drivowner[0] = EOS;
  
  return(TRUE);
}



/* open, create/load our cmb, and get ready for action if elapsed == NULL
   otherwise, we read the entire file to determine the elapsed time of
   the game and return it */
static int initReplay(char *fname, time_t *elapsed)
{
  int pkttype;
  time_t starttm = 0;
  time_t curTS = 0;
  Unsgn8 buf[PKT_MAXSIZE];

  if (!recordOpenInput(fname))
    {
      printf("initReplay: recordOpenInput(%s) failed\n", fname);
      return(FALSE);
    }

  /* don't bother mapping for just a count */
  if (!elapsed)
    map_lcommon();

  /* now lets read in the file header and check a few things. */

  if (!recordReadHeader(&fhdr))
    return(FALSE);
      
  if (fhdr.vers != RECVERSION)
    {				/* wrong vers */
      clog("initReplay: version mismatch.  got %d, need %d\n",
           fhdr.vers,
           RECVERSION);

      printf("initReplay: version mismatch.  got %d, need %d\n",
	     fhdr.vers,
	     RECVERSION);
      return(FALSE);
    }

  /* if we are looking for the elapsed time, scan the whole file
     looking for timestamps. */
  if (elapsed)			/* we want elapsed time */
    {
      int done = FALSE;

      starttm = fhdr.rectime;

      curTS = 0;
      /* read through the entire file, looking for timestamps. */
      
#if defined(DEBUG_REC)
      clog("conqreplay: initReplay: reading elapsed time");
#endif

      while (!done)
	{
          if ((pkttype = recordReadPkt(buf, PKT_MAXSIZE)) == SP_FRAME)
            {
              spFrame_t *frame = (spFrame_t *)buf;
              
              /* fix up the endianizational interface for the time */
              curTS = (time_t)ntohl(frame->time);
            }

	  if (pkttype == SP_NULL)
	    done = TRUE;	/* we're done */
	}

      if (curTS != 0)
	*elapsed = (curTS - starttm);
      else
	*elapsed = 0;

      /* now close the file so that the next call of initReplay can
	 get a fresh start. */
      recordCloseInput();
    }

  /* now we are ready to start running packets */
  
  return(TRUE);
}

/* overlay the elapsed time, and current framedelay */
void displayReplayData(void)
{
  char buf[128];
  time_t elapsed = (currTime - startTime);
  char *c;

  /* elapsed time */
  fmtseconds((int)elapsed, buf);
  c = &buf[2];			/* skip day count */
  cdputs( c, DISPLAY_LINS + 1, 2 );

  /* current frame delay */
#if 1
  sprintf(buf, "%2.3fs", framedelay);
#else  /* an attempt at fps. */
  if (framedelay != 0)
    sprintf(buf, "%3.2f fps", (1.0 / framedelay));
  else
    sprintf(buf, "MAX fps");
#endif

  cdputs( buf, DISPLAY_LINS + 1, 15);

  /* paused status */
  if (Context.recmode == RECMODE_PAUSED)
    cdputs( "PAUSED: Press [SPACE] to resume", DISPLAY_LINS + 2, 0);

  cdrefresh();

  return;
}


/* display a message - mostly ripped from readmsg() */
void displayMsg(Msg_t *themsg)
{
  char buf[MSGMAXLINE];
  unsigned int attrib = 0;

  if (Context.display == FALSE || Context.recmode != RECMODE_PLAYING)
    return;			/* don't display anything */

  buf[0] = '\0';

  if (Context.hascolor)
    {                           /* set up the attrib so msg's are cyan */
      attrib = COLOR_PAIR(COL_CYANBLACK);
    }

  if (themsg)
    {
      fmtmsg(themsg->msgto, themsg->msgfrom, buf);
      
      appstr( ": ", buf );
      appstr( themsg->msgbuf, buf );
      
      attrset(attrib);
      c_putmsg( buf, RMsg_Line );
      attrset(0);
      /* clear second line if sending to MSG_LIN1 */
      if (RMsg_Line == MSG_LIN1)
	{
	  cdclrl( MSG_LIN2, 1 );
	}
    }
  else
    {				/* just clear the message line */
      cdclrl( RMsg_Line, 1 );
#ifdef CONQGL
      showMessage(0, NULL);
#endif
    }

  return;

}

/* read in a header/data packet pair, and add them to our cmb.  return
   the packet type processed or RDATA_NONE if there is no more data or
   other error. */

int processPacket(void)
{
  Unsgn8 buf[PKT_MAXSIZE];
  spFrame_t *frame;
  int pkttype;
  Msg_t Msg;
  spMessage_t *smsg;

#if defined(DEBUG_REC)
  clog("conqreply: processPacket ENTER");
#endif

  if ((pkttype = recordReadPkt(buf, PKT_MAXSIZE)) != SP_NULL)
    {
      switch(pkttype)
        {
        case SP_SHIP:
          procShip(buf);
          break;
        case SP_SHIPSML:
          procShipSml(buf);
          break;
        case SP_SHIPLOC:
          procShipLoc(buf);
          break;
        case SP_USER:
          procUser(buf);
          break;
        case SP_PLANET:
          procPlanet(buf);
          break;
        case SP_PLANETSML:
          procPlanetSml(buf);
          break;
        case SP_PLANETLOC:
          procPlanetLoc(buf);
          break;
        case SP_TORP:
          procTorp(buf);
          break;
        case SP_TORPLOC:
          procTorpLoc(buf);
          break;
        case SP_TEAM:
          procTeam(buf);
          break;
        case SP_MESSAGE:
          smsg = (spMessage_t *)buf;
          memset((void *)&Msg, 0, sizeof(Msg_t));
          strncpy(Msg.msgbuf, smsg->msg, MESSAGE_SIZE);
          Msg.msgfrom = (int)((Sgn16)ntohs(smsg->from));
          Msg.msgto = (int)((Sgn16)ntohs(smsg->to));
          Msg.flags = smsg->flags;

          if (Msg.flags & MSG_FLAGS_FEEDBACK)
            clntDisplayFeedback(smsg->msg);
          else
            displayMsg(&Msg);

          break;

        case SP_FRAME:
          frame = (spFrame_t *)buf;
          /* endian correction*/
          frame->time = (Unsgn32)ntohl(frame->time);
          frame->frame = (Unsgn32)ntohl(frame->frame);

          if (startTime == (time_t)0)
            startTime = (time_t)frame->time;
          currTime = (time_t)frame->time;

          frameCount = (Unsgn32)frame->frame;

          break;
          
        default:
#ifdef DEBUG_REC
          fprintf(stderr, "processPacket: Invalid rtype %d\n", pkttype);
#endif
          break;          
        }
    }

  return pkttype;
}

/* read and process packets until a FRAME packet or EOD is found */
int processIter(void)
{
  int rtype;

  while(((rtype = processPacket()) != SP_NULL) && rtype != SP_FRAME)
    ;

  return(rtype);
}

/* seek around in a game.  backwards seeks will be slooow... */
void fileSeek(time_t newtime)
{
  if (newtime == currTime)
    return;			/* simple case */

  if (newtime < currTime)
    {				/* backward */
      /* we have to reset everything and start from scratch. */

      recordCloseInput();

      if (!initReplay(rfname, NULL))
	return;			/* bummer */
      
      currTime = startTime;
    }

  /* start searching */

  /* just read packets until 1. currTime exceeds newtime, or 2. no
     data is left */
  Context.display = FALSE; /* don't display things while looking */
  
  while (currTime < newtime)
    if ((processPacket() == SP_NULL))
      break;		/* no more data */
  
  Context.display = TRUE;

  return;
}
	  

/* MAIN */
int main(int argc, char *argv[])
{
  int i;
  extern char *optarg;
  extern int optind;
  
  if (argc <= 1)
    {
      printUsage();
      exit(1);
    }

  rfname = NULL;

  while ((i = getopt(argc, argv, "f:d:")) != EOF)    /* get command args */
    switch (i)
      {
      case 'f': 
	rfname = optarg;
        break;

      case 'd':
	framedelay = ctor(optarg);
	break;

      case '?':
      default: 
	printUsage();
	exit(1);
	break;
      }

  if (rfname == NULL)
    {
      printUsage();
      exit(1);
    }

  setSystemLog(FALSE);	/* use $HOME for logfile */

  GetSysConf(TRUE);             /* need this? */
  GetConf(0);

  /* turn off annoying beeps */
  UserConf.DoAlarms = FALSE;

  rndini( 0, 0 );		/* initialize random numbers */

  /* first, let's get the elapsed time */
  printf("Scanning file %s...\n", rfname);
  if (!initReplay(rfname, &totElapsed))
    exit(1);

  /* now init for real */
  if (!initReplay(rfname, NULL))
    exit(1);

  cdinit();			/* initialize display environment */
  
  Context.unum = MSG_GOD;	/* stow user number */
  Context.snum = ERR;		/* don't display in cdgetp - JET */
  Context.entship = FALSE;	/* never entered a ship */
  Context.histslot = ERR;	/* useless as an op */
  Context.lasttdist = Context.lasttang = 0;
  Context.lasttarg[0] = EOS;

  Context.display = TRUE;

  Context.maxlin = cdlins();	/* number of lines */
  Context.maxcol = cdcols();	/* number of columns */

  Context.recmode = RECMODE_PLAYING;

  /* if framedelay wasn't overridden, setup based on samplerate */
  if (framedelay == -1.0)
    framedelay = 1.0 / (real)fhdr.samplerate;

  replay();
  
  cdend();
  
  exit(0);
  
}

/*  replay - show the main screen and call watch() */

static void replay(void)
{
  int i, lin, col;
  char cbuf[MSGMAXLINE];
  extern char *ConquestVersion;
  extern char *ConquestDate;
  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];
  static char cfmt[MSGMAXLINE * 2];
  int ch;
  int done = FALSE;
  char *c;

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
      lin = 1;
      if ( fhdr.vers != RECVERSION ) 
	{
	  attrset(RedLevelColor);
	  sprintf( cbuf, "CONQUEST CQR VERSION MISMATCH %d != %d",
		   fhdr.vers, RECVERSION );
	  cdputc( cbuf, lin );
	  attrset(0);
          c_sleep(3.0);
          return;
	}
      
      if ( fhdr.cmnrev != COMMONSTAMP ) 
	{
	  attrset(RedLevelColor);
	  sprintf( cbuf, "CONQUEST COMMON BLOCK MISMATCH %d != %d",
		   fhdr.cmnrev, COMMONSTAMP );
	  cdputc( cbuf, lin );
	  attrset(0);
          c_sleep(3.0);
          return;
	}
      
      attrset(NoColor|A_BOLD);
      cdputc( "CONQUEST REPLAY PROGRAM", lin );
      attrset(YellowLevelColor);
      sprintf( cbuf, "%s (%s)",
               ConquestVersion, ConquestDate);
      cdputc( cbuf, lin+1 );

      lin+=3;
      
      cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Recording info:");
      lin+=2;
      i = lin;
      
      col = 5;
      
      cprintf(lin,col,ALIGN_NONE,sfmt, "File               ", rfname);
      lin++;

      cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded By        ", fhdr.user);
      lin++;

      if (fhdr.snum == 0)
        cprintf(lin,col,ALIGN_NONE,sfmt, "Recording Type     ", "Server");
      else
        {
          sprintf(cbuf, "Client (Ship %d)", fhdr.snum);
          cprintf(lin,col,ALIGN_NONE,sfmt, "Recording Type     ", cbuf);
        }
      lin++;

      cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded On        ", 
	      ctime((time_t *)&fhdr.rectime));
      lin++;
      sprintf(cbuf, "%d (delay: %0.3fs)", fhdr.samplerate, framedelay);
      cprintf(lin,col,ALIGN_NONE,sfmt, "Updates per second ", cbuf);
      lin++;
      fmtseconds(totElapsed, cbuf);

      if (cbuf[0] == '0')	/* see if we need the day count */
	c = &cbuf[2];	
      else
	c = cbuf;

      cprintf(lin,col,ALIGN_NONE,sfmt, "Total Game Time    ", c);
      lin++;
      fmtseconds((currTime - startTime), cbuf);

      if (cbuf[0] == '0')
	c = &cbuf[2];	
      else
	c = cbuf;
      cprintf(lin,col,ALIGN_NONE,sfmt, "Current Time       ", c);
      lin++;
      lin++;
      
      cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Commands:");
      lin+=3;
      
      cprintf(lin,col,ALIGN_NONE,cfmt, 'w', "watch a ship");
      lin++;
      cprintf(lin,col,ALIGN_NONE,cfmt, '/', "list ships");
      lin++;
      cprintf(lin,col,ALIGN_NONE,cfmt, 'r', "reset to beginning");
      lin++;
      cprintf(lin,col,ALIGN_NONE,cfmt, 'q', "quit");
      lin++;
      
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
	  fileSeek(startTime);
	  break;

        case '/':
	  playlist(TRUE, TRUE, 0);
	  cdclear();
	  break;
	}
    } while (!done);


  return;
  
}


/*  watch - peer over someone's shoulder */
/*  SYNOPSIS */
/*    watch */
void watch(void)
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
      grand( &msgrand );
      
      Context.snum = snum;		/* so display knows what to display */
      /*	  setopertimer();*/
      
      
      while (TRUE)
	{
	  if (Context.recmode == RECMODE_PLAYING)
	    if ((ptype = processIter()) == SP_NULL)
	      return;

	  Context.display = TRUE;
	  
	  if (toggle_flg)
	    toggle_line(snum,old_snum);
	  
	  
	  setdheader( TRUE ); /* always true for watching ships and
				 doomsday.  We may want to turn it off
				 if we ever add an option for watching
				 planets though, so we'll keep this
				 in for now */
	  
	  if (Context.recmode == RECMODE_PLAYING || upddsp)
	    {
	      display(Context.snum, headerflag);
	      displayReplayData();
	      upddsp = FALSE;	/* use this for one-shots */
	    }
	  
	  if (!iogtimed(&ch, framedelay))
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
	      fileSeek(currTime + 30);
	      upddsp = TRUE;
	      break;
	      
	    case 'F':	/* move forward 2 minutes */
	      displayMsg(NULL);
	      fileSeek(currTime + (2 * 60));
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
	      fileSeek(currTime - 30);
	      cdclrl( MSG_LIN1, 1 );
	      upddsp = TRUE;
	      break;
	      
	    case 'B':	/* move backward 2 minutes */
	      displayMsg(NULL);
	      cdputs( "Rewinding...", MSG_LIN1, 0);
	      cdrefresh();
	      fileSeek(currTime - (2 * 60));
	      cdclrl( MSG_LIN1, 1 );
	      upddsp = TRUE;
	      break;

	    case 'r':	/* reset to beginning */
	      cdputs( "Rewinding...", MSG_LIN1, 0);
	      cdrefresh();
	      fileSeek(startTime);
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
		    
	    case 'n':		/* set framedelay to normal playback
				   speed.*/
	      framedelay = 1.0 / (real)fhdr.samplerate;
	      upddsp = TRUE;
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
	      upddsp = TRUE;
	      break;

	    case '+': 
	    case '=':
	      if (framedelay != 0)
		{		/* can't divide 0 in our universe */
		  framedelay /= 2;
		  if (framedelay < 0.0)
		    framedelay = 0.0;
		}
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
	      playlist( TRUE, FALSE, 0 );
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
			  if (stillalive(i))
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
		    if ((snum > 0 && stillalive(snum)) || 
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
			  if (stillalive(i))
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
		    if ((snum > 0 && stillalive(snum)) || 
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
	      c_putmsg( "Type h for help.", MSG_LIN2 );
	      break;
	    }
	} /* end while */
    } /* end else */

  /* NOTREACHED */
  
}

void setdheader(int show_header)
{

  headerflag = show_header;
  return;
}

int prompt_ship(char buf[], int *snum, int *normal)
{
  int tch;
  int tmpsnum = 0;
  string pmt="Watch which ship (<cr> for doomsday)? ";
  string nss="No such ship.";

  tmpsnum = *snum;

  cdclrl( MSG_LIN1, 2 );

  if (fhdr.snum == 0)
    buf[0] = EOS;
  else
    sprintf(buf, "%d", fhdr.snum);

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

  delblanks( buf );

  if ( strlen( buf ) == 0 ) 
    {              /* watch doomsday machine */
      tmpsnum = DISPLAY_DOOMSDAY;
      *normal = TRUE;		/* doomsday doesn't have a debugging view */
    }
  else
    {
      if ( alldig( buf ) != TRUE )
	{
	  cdputs( nss, MSG_LIN2, 1 );
	  cdmove( 1, 1 );
	  cdrefresh();
	  c_sleep( 1.0 );
	  return(FALSE); /* dwp */
	}
      safectoi( &tmpsnum, buf, 0 );	/* ignore return status */
    }

  if ( (tmpsnum < 1 || tmpsnum > MAXSHIPS) && tmpsnum != DISPLAY_DOOMSDAY )
    {
      cdputs( nss, MSG_LIN2, 1 );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 1.0 );
      return(FALSE); /* dwp */
    }

  *snum = tmpsnum;
  return(TRUE);

} /* end prompt_ship() */


/*  dowatchhelp - display a list of commands while watching a ship */
/*  SYNOPSIS */
/*    dowatchhelp( void ) */
void dowatchhelp(void)
{
  int lin, col, tlin;
  int ch;
  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#%%-9s#%d#%%s",
	      InfoColor,
	      LabelColor);
	}

  cdclear();
  cprintf(1,0,ALIGN_CENTER,"#%d#%s", LabelColor, "WATCH WINDOW COMMANDS");
  
  lin = 4;
  
  /* Display the left side. */
  tlin = lin;
  col = 4;
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "w", "watch a ship");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  	"<>", "decrement/increment ship number\n");
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
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "!", "display toggle line");

  putpmt( MTXT_DONE, MSG_LIN2 );
  cdrefresh();
  while ( ! iogtimed( &ch, 1.0 ) )
    ;
  cdclrl( MSG_LIN2, 1 );
  
  return;
  
}


void toggle_line(int snum, int old_snum)
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

char *build_toggle_str(char *snum_str, int snum)
{
  
  char buf[MSGMAXLINE];
  static char *doomsday_str = "DM";
  static char *deathstar_str = "DS";
  static char *unknown_str = "n/a";
  
  buf[0] = EOS;
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
