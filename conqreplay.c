#include "c_defs.h"

/************************************************************************
 *
 * conqreplay - replay a cqr recording
 *
 * $Id$
 *
 * Copyright 1999-2002 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"
#include "color.h"

#include "record.h"

/* we will have our own copy of the common block here, so setup some stuff */
static char *rcBasePtr = NULL;
static int rcoff = 0;
				/* our own copy */
#define map1d(thevarp, thetype, size) {  \
              thevarp = (thetype *) (rcBasePtr + rcoff); \
              rcoff += (sizeof(thetype) * (size)); \
	}

static rData_t rdata;		/* the input/output record */
static fileHeader_t fhdr;

static real framedelay = -1.0;	/* delaytime between frames */

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

  if ((rcBasePtr = malloc(SZ_CMB)) == NULL)
    {
      perror("map_lcommon(): memory allocation failed\n");
      return(FALSE);
    }


  map1d(CBlockRevision, int, 1);	/* this *must* be the first var */
  map1d(ConqInfo, ConqInfo_t, 1)
  map1d(Users, User_t, MAXUSERS);
  map1d(Robot, Robot_t, 1);
  map1d(Planets, Planet_t, NUMPLANETS + 1);
  map1d(Teams, Team_t, NUMALLTEAMS);
  map1d(Doomsday, Doomsday_t, 1);
  map1d(History, History_t, MAXHISTLOG);
  map1d(Driver, Driver_t, 1);
  map1d(Ships, Ship_t, MAXSHIPS + 1);
  map1d(ShipTypes, ShipType_t, MAXNUMSHIPTYPES);
  map1d(Msgs, Msg_t, MAXMESSAGES);
  map1d(EndOfCBlock, int, 1);

  return(TRUE);
}



/* open, create/load our cmb, and get ready for action if elapsed == NULL
   otherwise, we read the entire file to determine the elapsed time of
   the game and return it */
static int initReplay(char *fname, time_t *elapsed)
{
  int rv;
  int rtype;
  time_t starttm = 0;
  time_t curTS = 0;

  if (!recordOpenInput(fname))
    {
      printf("initReplay: recordOpenInput(%s) failed\n", fname);
      return(FALSE);
    }

  if (!elapsed)
    if (map_lcommon() == FALSE)
      {
	printf("initReplay: could not map a CMB\n");
	return(FALSE);
      }

  /* now lets read in the file header and check a few things. */

  if (!recordReadHeader(&fhdr))
    return(FALSE);
      
  if (fhdr.vers != RECVERSION)
    {				/* wrong vers */
      printf("initReplay: version mismatch.  got %d, need %d\n",
	     fhdr.vers,
	     RECVERSION);
      return(FALSE);
    }

  /* the next packet should be the cmb data block */
  if ((rtype = recordReadPkt(&rdata, NULL)) != RDATA_CMB)
    {
      printf("initReplay: first packet type (%d) is not RDATA_CMB\n",
	     rtype);
      return(FALSE);
    }

  
  /* ok, now copy it into our local version if we are not looking for
     elapsed time */
  if (!elapsed)
    memcpy((char *)CBlockRevision, (char *)rdata.data.rcmb, SZ_CMB);

  /* if we are looking for the elapsed time, scan the whole file
     looking for timestamps. */
  if (elapsed)			/* we want elapsed time */
    {
      int done = FALSE;

      /*  */
      starttm = fhdr.rectime;

      curTS = 0;
      /* read through the entire file, looking for timestamps. */
      
      while (!done)
	{
	  if ((rtype = recordReadPkt(&rdata, NULL)) == RDATA_TIME)
	    curTS = rdata.data.rtime;

	  if (rtype == RDATA_NONE)
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

  /* elapsed time */
  fmtseconds((int)elapsed, buf);
  cdputs( buf, DISPLAY_LINS + 1, 2 );

  /* current frame delay */
  sprintf(buf, "%0.3fs", framedelay);
  cdputs( buf, DISPLAY_LINS + 1, 15);

  /* paused status */
  if (CqContext.recmode == RECMODE_PAUSED)
    cdputs( "PAUSED: Press [SPACE] to resume", DISPLAY_LINS + 2, 0);

  cdrefresh();

  return;
}


/* display a message - mostly ripped from readmsg() */
void displayMsg(Msg_t *themsg)
{
  int i;
  char buf[MSGMAXLINE];
  unsigned int attrib = 0;

  if (CqContext.display == FALSE || CqContext.recmode != RECMODE_PLAYING)
    return;			/* don't display anything */

  buf[0] = '\0';

  if (HasColors)
    {                           /* set up the attrib so msg's are cyan */
      attrib = COLOR_PAIR(COL_CYANBLACK);
    }

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

  return;

}

/* read in a header/data packet pair, and add them to our cmb.  return
   the packet type processed or RDATA_NONE if there is no more data or
   other error. */

int processPacket(void)
{
  int rdsize, rv, rtype;

  if ((rtype = recordReadPkt(&rdata, NULL)) == RDATA_NONE)
    return(RDATA_NONE);
    
  /* figure out what it is and apply it */
  switch(rtype)
    {
    case RDATA_SHIP:
      Ships[rdata.index] = rdata.data.rship;
      /* stop annoying beeps for all ships. */
      Ships[rdata.index].options[OPT_ALARMBELL] = FALSE;
      break;

    case RDATA_PLANET:
      Planets[rdata.index] = rdata.data.rplanet;
      break;

    case RDATA_TIME: 		/* timestamp */
      if (startTime == (time_t)0)
	startTime = rdata.data.rtime;
      currTime = rdata.data.rtime;
      break;

    case RDATA_MESSAGE:
      displayMsg(&rdata.data.rmsg);
      break;

#ifdef DEBUG_REC
    default:
      fprintf(stderr, "processPacket: Invalid rtype %d\n", rtype);
      break;
#endif
    }

  return(rtype);
}

/* read and process packets until a TS packaet or EOD is found */
int processIter(void)
{
  int rtype;

  while(((rtype = processPacket()) != RDATA_NONE) && rtype != RDATA_TIME)
    ;

  return(rtype);
}

/* seek around in a game.  backwards seeks will be slooow... */
void fileSeek(time_t newtime)
{
  /* determine if the newtime is greater or less than the current time */

  if (newtime == currTime)
    return;			/* simple case */

  if (newtime < currTime)
    {				/* backward */
      /* we have to reset everything and start from scratch. */

      recordCloseInput();
      if (rcBasePtr)
	{
	  free(rcBasePtr);
	  rcBasePtr = NULL;
	  rcoff = 0;
	}
      if (!initReplay(rfname, NULL))
	return;			/* bummer */
      
      currTime = startTime;
    }

  /* start searching */

  /* just read packets until 1. currTime exceeds newtime, or 2. no
     data is left */
  CqContext.display = FALSE; /* don't display things while looking */
  
  while (currTime < newtime)
    if (processPacket() == RDATA_NONE)
      break;		/* no more data */
  
  CqContext.display = TRUE;

  return;
}
	  

/* MAIN */
main(int argc, char *argv[])
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



  GetSysConf(TRUE);
  rndini( 0, 0 );		/* initialize random numbers */

  /* first, let's get the elapsed time */
  printf("Scanning file %s...\n", rfname);
  if (!initReplay(rfname, &totElapsed))
    exit(1);

  /* now init for real */
  if (!initReplay(rfname, NULL))
    exit(1);

  cdinit();			/* initialize display environment */
  
  CqContext.unum = MSG_GOD;	/* stow user number */
  CqContext.snum = ERR;		/* don't display in cdgetp - JET */
  CqContext.entship = FALSE;	/* never entered a ship */
  CqContext.histslot = ERR;	/* useless as an op */
  CqContext.display = TRUE;

  CqContext.maxlin = cdlins();	/* number of lines */
  CqContext.maxcol = cdcols();	/* number of columns */

  CqContext.recmode = RECMODE_PLAYING;

  /* if framedelay wasn't overridden, setup based on samplerate */
  if (framedelay == -1.0)
    framedelay = (fhdr.samplerate == 1) ? 1.0 : 0.5;

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
  string prmt="Command: ";
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
      lin = 1;
      if ( *CBlockRevision == COMMONSTAMP ) 
	{
	  attrset(NoColor|A_BOLD);
	  cdputc( "CONQUEST REPLAY PROGRAM", lin );
	  attrset(YellowLevelColor);
	  sprintf( cbuf, "%s (%s)",
		   ConquestVersion, ConquestDate);
	  cdputc( cbuf, lin+1 );
	}
      else
	{
	  attrset(RedLevelColor);
	  sprintf( cbuf, "CONQUEST COMMON BLOCK MISMATCH %d != %d",
		   *CBlockRevision, COMMONSTAMP );
	  cdputc( cbuf, lin );
	  attrset(0);
	}
      
      lin+=3;
      
      cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Recording info:");
      lin+=2;
      i = lin;
      
      col = 5;
      
      cprintf(lin,col,ALIGN_NONE,sfmt, "File              ", rfname);
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded By       ", fhdr.user);
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded On       ", 
	      ctime(&fhdr.rectime));
      lin++;
      sprintf(cbuf, "%d (delay: %0.3fs)", fhdr.samplerate, framedelay);
      cprintf(lin,col,ALIGN_NONE,sfmt, "Frames per second ", cbuf);
      lin++;
      fmtseconds(totElapsed, cbuf);
      cprintf(lin,col,ALIGN_NONE,sfmt, "Total Game Time   ", cbuf);
      lin++;
      fmtseconds((currTime - startTime), cbuf);
      cprintf(lin,col,ALIGN_NONE,sfmt, "Current Time      ", cbuf);
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

      if ( ! iogtimed( &ch, 1 ) )
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
  int msgrand, readone, now;
  char buf[MSGMAXLINE];
  int live_ships = TRUE;
  int toggle_flg = FALSE;   /* jon no like the toggle line ... :-) */
  normal = TRUE;

  if (!prompt_ship(buf, &snum, &normal))
    return;
  else
    {
      old_snum = tmp_snum = snum;
      CqContext.redraw = TRUE;
      cdclear();
      cdredo();
      grand( &msgrand );
      
      CqContext.snum = snum;		/* so display knows what to display */
      /*	  setopertimer();*/
      
      
      while (TRUE)
	{
	  if (CqContext.recmode == RECMODE_PLAYING)
	    if ((ptype = processIter()) == RDATA_NONE)
	      return;

#ifdef DEBUG_REC
	  fprintf(stderr, "watch: got iter: packet type: %d\n", 
		  ptype);
#endif

	  CqContext.display = TRUE;
	  
	  if (toggle_flg)
	    toggle_line(snum,old_snum);
	  
	  
	  setdheader( TRUE ); /* always true for watching ships and
				 doomsday.  We may want to turn it off
				 if we ever add an option for watching
				 planets though, so we'll keep this
				 in for now */
	  
	  display( CqContext.snum, headerflag );
	  displayReplayData();
	  
	  if (!iochav())
	    {
	      c_sleep(framedelay);
	      continue;
	    }
	  
	  /* got a char */
	  ch = iogchar();
	  
	  cdclrl( MSG_LIN1, 2 );
	  switch ( ch )
	    {
	    case 'q': 
	      return;
	      break;
	    case 'h':
	      dowatchhelp();
	      CqContext.redraw = TRUE;
	      break;
	      
	    case 'f':	/* move forward 30 seconds */
	      fileSeek(currTime + 30);
	      break;
	      
	    case 'F':	/* move forward 2 minutes */
	      fileSeek(currTime + (2 * 60));
	      break;
	      
	    case 'b':	/* move backward 30 seconds */
	      cdputs( "Rewinding...", MSG_LIN1, 0);
	      cdrefresh();
	      fileSeek(currTime - 30);
	      cdclrl( MSG_LIN1, 1 );
	      break;
	      
	    case 'B':	/* move backward 2 minutes */
	      cdputs( "Rewinding...", MSG_LIN1, 0);
	      cdrefresh();
	      fileSeek(currTime - (2 * 60));
	      cdclrl( MSG_LIN1, 1 );
	      break;

	    case 'r':	/* reset to beginning */
	      cdputs( "Rewinding...", MSG_LIN1, 15);
	      cdrefresh();
	      fileSeek(startTime);
	      cdclrl( MSG_LIN1, 1 );
	      break;

	    case ' ':	/* pause/resume playback */
	      if (CqContext.recmode == RECMODE_PLAYING)
		{		/* pause */
		  CqContext.recmode = RECMODE_PAUSED;
		}
	      else 
		{		/* resume */
		  CqContext.recmode = RECMODE_PLAYING;
		}

	      break;
		    
	    case '+':
	      /* if it's at 0, we should still
		 be able to double it. sorta. */
	      if (framedelay == 0.0) 
		framedelay = 0.001;
	      framedelay *= 2;
	      if (framedelay > 10.0) /* really, 10s is a *long* time
					between frames... */
		framedelay = 10.0;
	      break;

	    case '-': 
	      if (framedelay != 0)
		{		/* can't devide 0 */
		  framedelay /= 2;
		  if (framedelay < 0.0)
		    framedelay = 0.0;
		}
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
		      CqContext.redraw = TRUE;
		    }
		  CqContext.snum = snum;
		}
	      break;

	    case '`':                 /* toggle between two ships */
	      if (normal || (!normal && old_snum > 0))
		{
		  if (old_snum != snum) 
		    {
		      tmp_snum = snum;
		      snum = old_snum;
		      old_snum = tmp_snum;
			  
		      CqContext.snum = snum;
		      CqContext.redraw = TRUE;
		      display( CqContext.snum, headerflag );
		    }
		}
	      else
		cdbeep();
	      break;

	    case '/':                /* ship list - dwp */
	      playlist( TRUE, FALSE, 0 );
	      CqContext.redraw = TRUE;
	      break;
	    case '\\':               /* big ship list - dwp */
	      playlist( TRUE, TRUE, 0 );
	      CqContext.redraw = TRUE;
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
			
		  CqContext.redraw = TRUE;
		      
		  if (live_ships)
		    if ((snum > 0 && stillalive(snum)) || 
			(snum == DISPLAY_DOOMSDAY && Doomsday->status == DS_LIVE))
		      {
			CqContext.snum = snum;
			break;
		      }
		    else
		      continue;
		  else
		    {
		      CqContext.snum = snum;
		      break;
		    }
		}

	      display( CqContext.snum, headerflag );
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
			
		  CqContext.redraw = TRUE;
		      
		  if (live_ships)
		    if ((snum > 0 && stillalive(snum)) || 
			(snum == DISPLAY_DOOMSDAY && Doomsday->status == DS_LIVE))
		      {
			CqContext.snum = snum;
			break;
		      }
		    else
		      continue;
		  else
		    {
		      CqContext.snum = snum;
		      break;
		    }
		}
	      display( CqContext.snum, headerflag );

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
  buf[0] = EOS;
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
  cprintf(tlin,col,ALIGN_NONE,sfmt, "-", "decrease frame delay by half (faster)");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "+", "double the frame delay (slower)");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "`", "toggle between two ships");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "!", "display toggle line");

  putpmt( MTXT_DONE, MSG_LIN2 );
  cdrefresh();
  while ( ! iogtimed( &ch, 1 ) )
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
  cdputs( buf, MSG_LIN1, (CqContext.maxcol-(strlen(buf))));  /* at end of line */
  
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

