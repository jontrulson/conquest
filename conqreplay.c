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

/* FIXME */
#define stoptimer
#define setopertimer

/* we will have our own copy of the common block here, so setup some stuff */
static char *rcBasePtr = NULL;
static int rcoff = 0;
				/* our own copy */
#define map1d(thevarp, thetype, size) {  \
              thevarp = (thetype *) (rcBasePtr + rcoff); \
              rcoff += (sizeof(thetype) * (size)); \
	}

static rData_t rdata;		/* the input/output record */
static int rdata_rfd;		/* the currently open file for reading */

static fileHeader_t fhdr;
static real framedelay = -1.0;	/* delaytime between frames */

static char *rfname;		/* game file we are playing */

static void replay(void);


void printUsage(void)
{
  fprintf(stderr, "usage: conqreplay -f <cqr file>\n");
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

/* open, create/load our cmb, and get ready for action */
static int initReplay(char *fname)
{
  int rv;
  rDataHeader_t rdatahdr;

  if ((rdata_rfd = open(fname, O_RDONLY)) == -1)
    {
      perror("initReplay: open failed");
      return(FALSE);
    }

  if (map_lcommon() == FALSE)
    {
      printf("initReplay: could not map a CMB\n");
      return(FALSE);
    }

  /* now lets read in the file header and check a few things. */

  if ((rv = read(rdata_rfd, (char *)&fhdr, SZ_FILEHEADER)) != SZ_FILEHEADER)
    {
      printf("initReplay: could not read a proper header\n");
      return(FALSE);
    }

  if (fhdr.vers != RECVERSION)
    {				/* wrong vers */
      printf("initReplay: version mismatch.  got %d, need %d\n",
	     fhdr.vers,
	     RECVERSION);
      close(rdata_rfd);
      return(FALSE);
    }

  /* the next 2 packets should be the cmb data hdr and the cmb data
     block */

  if ((rv = read(rdata_rfd, (char *)&rdatahdr, SZ_RDATAHEADER)) != 
      SZ_RDATAHEADER)
    {
      printf("initReplay: could not read the intial data header\n");
      return(FALSE);
    }

  if (rdatahdr.type != RDATA_CMB)
    {
      printf("initReplay: first data header type is not RDATA_CMB\n");
      return(FALSE);
    }

  /* now read it in */
  if ((rv = read(rdata_rfd, (char *)&rdata, SZ_CMB + SZ_DRHSIZE)) != 
      (SZ_CMB + SZ_DRHSIZE))
    {
      printf("initReplay: could not read the intial CMB\n");
      return(FALSE);
    }

  /* ok, now copy it into our local version */
  memcpy((char *)CBlockRevision, (char *)rdata.data.rcmb, SZ_CMB);

  /* now we are ready to start running packets */
  
  return(TRUE);
}

/* display a message - mostly ripped from readmsg() */
void displayMsg(Msg_t *themsg)
{
  int i;
  char buf[MSGMAXLINE];
  unsigned int attrib = 0;


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
  rDataHeader_t rdatahdr;
  int rdsize, rv;

  /* first read in the data header */
  if ((rv = read(rdata_rfd, (char *)&rdatahdr, SZ_RDATAHEADER)) != 
      SZ_RDATAHEADER)
    {
#ifdef DEBUG_REC
      fprintf(stderr, "processPacket: could not read data header, returned %d\n",
	     rv);
#endif

      return(RDATA_NONE);
    }

  if (!(rdsize = recordPkt2Sz(rdatahdr.type)))
    return(RDATA_NONE);

  rdsize += SZ_DRHSIZE;
  /* so now read in the data pkt */
  if ((rv = read(rdata_rfd, (char *)&rdata, rdsize)) != 
      rdsize)
    {
#ifdef DEBUG_REC
      fprintf(stderr, "processPacket: could not read data packet, returned %d\n",
	     rv);
#endif

      return(RDATA_NONE);
    }

#ifdef DEBUG_REC
  fprintf(stderr, "processPacket: rdatahdr.type = %d, rdata.index = %d\n",
	  rdatahdr.type, rdata.index);
#endif

  /* figure out what it is and apply it */
  switch(rdatahdr.type)
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
      break;

    case RDATA_MESSAGE:
      displayMsg(&rdata.data.rmsg);
      break;

    }

  return(rdatahdr.type);
}

/* read and process packets until a TS packaet or EOD is found */
int processIter(void)
{
  int rtype;

  while(((rtype = processPacket()) != RDATA_NONE) && rtype != RDATA_TIME)
    ;

  return(rtype);
}

/*  conqreplay */
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

  GetSysConf(TRUE);
  rndini( 0, 0 );		/* initialize random numbers */
  cdinit();			/* initialize display environment */
  
  CqContext.unum = MSG_GOD;	/* stow user number */
  CqContext.snum = ERR;		/* don't display in cdgetp - JET */
  CqContext.entship = FALSE;	/* never entered a ship */
  CqContext.histslot = ERR;	/* useless as an op */
  CqContext.display = TRUE;

  CqContext.maxlin = cdlins();	/* number of lines */
  CqContext.maxcol = cdcols();	/* number of columns */

  CqContext.recmode = RECMODE_PLAYING;

  if (!initReplay(rfname))
    {
      cdend();
      exit(1);
    }

  /* if framedelay wasn't overriden, setup based on samplerate */
  if (framedelay == -1.0)
    framedelay = (fhdr.samplerate == 1) ? 1.0 : 0.5;

  replay();
  
  cdend();
  
  exit(0);
  
}

/*  replay - show the main screen */

static void replay(void)
{
  int i, lin, col;
  char cbuf[MSGMAXLINE];
  extern char *ConquestVersion;
  extern char *ConquestDate;
  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#%%s#%d#: %%s",
	      InfoColor,
	      GreenColor);
    }

  cdclear();
  
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

  cprintf(lin,col,ALIGN_NONE,sfmt, "File", rfname);
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded By", fhdr.user);
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded On", 
	  ctime(&fhdr.rectime));
  lin++;
  sprintf(cbuf, "%d (delay: %0.3fs)", fhdr.samplerate, framedelay);
  cprintf(lin,col,ALIGN_NONE,sfmt, "Frames per second", cbuf);
  lin++;
  

  watch();

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


	  while (((ptype = processIter()) != RDATA_NONE))	/* repeat */
	    {
#ifdef DEBUG_REC
	      fprintf(stderr, "watch: got iter: packet type: %d\n", 
		      ptype);
#endif

	      CqContext.display = TRUE;
	      c_sleep(framedelay);
		/* set up toggle line display */
		/* cdclrl( MSG_LIN1, 1 ); */
	      if (toggle_flg)
		toggle_line(snum,old_snum);

		
	      setdheader( TRUE ); /* always true for watching ships and
				     doomsday.  We may want to turn it off
				     if we ever add an option for watching
				     planets though, so we'll keep this
				     in for now */
	      
	      display( CqContext.snum, headerflag );

	      if (!iochav())
		{
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
	    }
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
  cprintf(tlin,col,ALIGN_NONE,sfmt, "`", "toggle between two ships");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "!", "display toggle line");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "/", "player list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "q", "quit");

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

