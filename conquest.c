#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q U E S T */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres mak2003 */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"
#include "record.h"
#include "ibuf.h"
#include "display.h"
#include "conqnet.h"
#include "packet.h"

#define CLIENT_NOEXTERN
#include "client.h"
#undef CLIENT_NOEXTERN
#include "clientlb.h"
#include "clntauth.h"

#include "meta.h"

#define CLIENTNAME "Conquest"	/* our client name */

static char cbuf[MID_BUFFER_SIZE]; /* general purpose buffer */

static int lastServerError = 0;	/* set by an ACK from server */
static Unsgn8 clientFlags = 0;  /* set according to CLIENTSTAT packets */

int hello(void);		/* meet and greet the server */
void catchSignals(void);
void handleSignal(int sig);

void astservice(int sig);
void stopTimer(void);
void startTimer(void);
void conqend(void);
void processPacket(Unsgn8 *buf);
int selectServer(metaSRec_t *metaServerList, int nums);

void printUsage()
{
  printf("Usage: conquest [-m ][-s server[:port]] [-r recfile] [ -t ]\n");
  printf("                [ -M <metaserver> ]\n\n");
  printf("    -m               query the metaserver\n");
  printf("    -s server[:port] connect to <server> at <port>\n");
  printf("                      default: localhost:1701\n");
  printf("    -r recfile       Record game to <recfile>\n");
  printf("                      recfile will be in compressed format\n");
  printf("                      if conquest was compiled with libz\n");
  printf("                      support\n");
  printf("    -t              telnet mode (no user conf load/save)\n");
  printf("    -M metaserver   specify alternate <metaserver> to contact.\n");
  printf("                     default: %s\n", META_DFLT_SERVER);
  return;
}

int getLocalhost(char *buf, int len)
{
  struct hostent *hp;

  gethostname ( buf, len );
  if ((hp = gethostbyname(buf)) == NULL) 
    {
      fprintf(stderr, "conquest: gethostbyname(): cannot get localhost info.\n");
      return FALSE;
    }

  return TRUE;
}

int connectServer(char *remotehost, Unsgn16 remoteport)
{
  int s;
  struct sockaddr_in sa;
  struct hostent *hp;
  int lin;

  if (!remotehost)
    return FALSE;

  /* display the logo */
  lin = conqlogo();

  lin += 5;

  if ((hp = gethostbyname(remotehost)) == NULL) 
    {
      clog("conquest: %s: no such host\n", remotehost);

      cprintf(lin, 0, ALIGN_CENTER, "conquest: %s: no such host", 
              remotehost);
      putpmt(MTXT_DONE, MSG_LIN2 );
      iogchar();

      return FALSE;
    }

  /* put host's address and address type into socket structure */
  memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);
  sa.sin_family = hp->h_addrtype;

  sa.sin_port = htons(remoteport);

  if ((s = socket(AF_INET, SOCK_STREAM, 0 )) < 0) 
    {
      clog("socket: %s", strerror(errno));
      cprintf(lin, 0, ALIGN_CENTER, "socket: %s", 
              remotehost);
      putpmt(MTXT_DONE, MSG_LIN2 );
      iogchar();
      return FALSE;
    }

  clog("Connecting to host: %s, port %d\n",
       remotehost, remoteport);

  cprintf(lin++, 0, ALIGN_CENTER, "Connecting to host: %s, port %d\n",
          remotehost, remoteport);
  cdrefresh();

  /* connect to the remote server */
  if (connect(s, (const struct sockaddr *)&sa, sizeof(sa)) < 0) 
    {
      clog("connect: %s", strerror(errno));
      clog("Cannot connect to host %s:%d\n", 
           remotehost, remoteport);

      cprintf(lin++, 0, ALIGN_CENTER, "connect: %s", strerror(errno));
      cprintf(lin++, 0, ALIGN_CENTER, "Cannot connect to host %s:%d\n",
              remotehost, remoteport);
      cprintf(lin++, 0, ALIGN_CENTER, 
              "Is there a conquestd server running there?\n");
      putpmt(MTXT_DONE, MSG_LIN2 );
      iogchar();

      return FALSE;
    }

  cprintf(lin++, 0, ALIGN_CENTER, "Connected!");
  cdrefresh();

  c_sleep(2.0);

  cInfo.serverDead = FALSE;
  cInfo.sock = s;

  return TRUE;
}

/*  conquest - main program */
int main(int argc, char *argv[]) 
{
  int i;
  char *ch;
  int wantMetaList = FALSE;     /* wants to see a list from metaserver */
  int serveropt = FALSE;        /* specified a server with '-s' */
  int nums = 0;                     /* num servers from metaGetServerList() */
  char *metaServer = META_DFLT_SERVER; 
  metaSRec_t *metaServerList;   /* list of servers */

  Context.entship = FALSE;
  Context.recmode = RECMODE_OFF;
  Context.updsec = 5;		/* dflt - 5/sec */
  Context.msgrand = getnow(NULL, 0);

  cInfo.sock = -1;
  cInfo.state = CLT_STATE_PREINIT;
  cInfo.serverDead = TRUE;
  cInfo.isLoggedIn = FALSE;
  cInfo.remoteport = CN_DFLT_PORT;

  setSystemLog(FALSE);	/* use $HOME for logfile */
  if (!getLocalhost(cInfo.localhost, MAXHOSTNAME))
    return(1);

  cInfo.remotehost = strdup("localhost"); /* default to your own server */

  /* check options */
  while ((i = getopt(argc, argv, "mM:s:r:t")) != EOF)    /* get command args */
    switch (i)
      {
      case 'm':
        wantMetaList = TRUE;
        break;
      case 'M':
        metaServer = optarg;
        break;
      case 's':                 /* [host[:port]] */
	cInfo.remotehost = (Unsgn8 *)strdup(optarg);
        if (!cInfo.remotehost)
          {
            printf("strdup failed\n");
            exit(1);
          }
        if ((ch = strchr(cInfo.remotehost, ':')) != NULL)
          {
            *ch = 0;
            ch++;
            if ((cInfo.remoteport = atoi(ch)) == 0)
              cInfo.remoteport = CN_DFLT_PORT;

            /* if no host was specified (only the :port), then set to
               localhost */
            if (strlen(cInfo.remotehost) == 0)
              cInfo.remotehost = "localhost";
          }
        else
          cInfo.remoteport = CN_DFLT_PORT;
        
        serveropt = TRUE;

	break;
      case 'r': 
	if (recordOpenOutput(optarg, FALSE))
	  {			/* we are almost ready... */
	    Context.recmode = RECMODE_STARTING;
	    printf("Recording game to %s...\n", optarg);
	  }
	else
	  {
	    Context.recmode = RECMODE_OFF;
            printf("Cannot record game to %s... terminating\n", optarg);
	    exit(1);
	  }
        break;

      case 't':
        confSetTelnetClientMode(TRUE);
        break;

      default:
	printUsage();
	exit(1);
      }

  
#ifdef DEBUG_CONFIG
  clog("%s@%d: main() Reading Configuration files.", __FILE__, __LINE__);
#endif
  
  if (GetConf(0) == ERR)	
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetConf() returned ERR.", __FILE__, __LINE__);
#endif
	exit(1);
      }

  Context.updsec = UserConf.UpdatesPerSecond;
  
  if (serveropt && wantMetaList)
    {
      printf("-m ignored, since -s was specified\n");
      wantMetaList = FALSE;
    }
  else if (wantMetaList)
    {                           /* get the metalist and display */
      printf("Querying metaserver at %s\n",
             metaServer);
      nums = metaGetServerList(metaServer, &metaServerList);

      if (nums < 0)
        {
          printf("metaGetServerList() failed\n");
          return 1;
        }

      if (nums == 0)
        {
          printf("metaGetServerList() reported 0 servers online\n");
          return 1;
        }

      printf("Found %d server(s)\n",
             nums);
    }


  /* a parallel universe, it is */

  fake_common();
  initeverything();		/* initialize the universe.  Wow. Such power */
  initmsgs();
  *CBlockRevision = COMMONSTAMP;
  ConqInfo->closed = FALSE;
  Driver->drivstat = DRS_OFF;
  Driver->drivpid = 0;
  Driver->drivowner[0] = EOS;
  

#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting conqinit().", __FILE__, __LINE__);
#endif
  
  conqinit();			/* machine dependent initialization */
  
  
  rndini( 0, 0 );		/* initialize random numbers */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting cdinit().", __FILE__, __LINE__);
#endif

  
  
  cdinit();			/* set up display environment */
  
  Context.maxlin = cdlins();
  Context.maxcol = cdcols();
  Context.snum = 0;		/* force menu to get a new ship */
  Context.histslot = ERR;
  Context.lasttang = Context.lasttdist = 0;
  Context.lasttarg[0] = EOS;

  if (wantMetaList)
    {                           /* list the servers */
      i = selectServer(metaServerList, nums);

      if (i < 0)
        {                       /* didn't pick one */
          cdend();
          return 1;
        }

      if (cInfo.remotehost)
        free(cInfo.remotehost);

      if ((cInfo.remotehost = strdup(metaServerList[i].altaddr)) == NULL)
        {
          clog("strdup(metaServerList[i]) failed");
          cdend();
          return 1;
        }

      cInfo.remoteport = metaServerList[i].port;

    }


  /* connect to the host */
  if (!connectServer(cInfo.remotehost, cInfo.remoteport))
    {
      cdend();
      return(1);
    }
  
  /* now we need to negotiate. */
  if (!hello())
    {
      cdend();
      clog("conquest: hello() failed\n");
      printf("conquest: hello() failed, check log\n");
      exit(1);
    }

#ifdef DEBUG_FLOW
  clog("%s@%d: main() welcoming player.", __FILE__, __LINE__);
#endif
  
  if ( welcome( &Context.unum ) )
    {
      menu();
    }
  
  cdend();			/* clean up display environment */
  conqend();			/* machine dependent clean-up */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() *EXITING*", __FILE__, __LINE__);
#endif
  
  exit(0);
  
}


/*  capentry - captured system entry for a new ship */
/*  SYNOPSIS */
/*    int flag, capentry */
/*    int capentry, snum, system */
/*    system = capentry( snum, system ) */
int selectentry( Unsgn8 esystem )
{
  int i; 
  int ch; 
  int owned[NUMPLAYERTEAMS]; 
  
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
  
  cdclrl( MSG_LIN1, 2 );
  cdputc( cbuf, MSG_LIN1 );
  cdmove( 1, 1 );
  cdrefresh();
  
  while ( stillalive( Context.snum ) )
    {
      if ( ! iogtimed( &ch, 1.0 ) )
	continue;
      switch  ( ch )
	{
	case TERM_NORMAL:
	case TERM_ABORT:	/* doesn't like the choices ;-) */
	  sendCommand(CPCMD_ENTER, 0);
	  return ( FALSE );
	  break;
	case TERM_EXTRA:
	  /* Enter the home system. */
	  sendCommand(CPCMD_ENTER, (Unsgn16)(1 << Ships[Context.snum].team));
	  return ( TRUE );
	  break;
	default:
	  for ( i = 0; i < NUMPLAYERTEAMS; i++ )
	    if ( Teams[i].teamchar == (char)toupper( ch ) && owned[i] )
	      {
		/* Found a good one. */
		sendCommand(CPCMD_ENTER, (Unsgn16)(1 << i));
		return ( TRUE );
	      }
	  /* Didn't get a good one; complain and try again. */
	  cdbeep();
	  cdrefresh();
	  break;
	}
    }
  
  return ( FALSE );	    /* can get here because of stillalive() */
  
}


/*  command - execute a user's command */
/*  SYNOPSIS */
/*    char ch */
/*    command( ch ) */
void command( int ch )
{
  int i;
  real x;
  if (KPAngle(ch, &x) == TRUE)	/* hit a keypad key */
    {				/* change course */
      cdclrl( MSG_LIN1, 1 );
      cdclrl( MSG_LIN2, 1 );
      
      sendSetCourse(cInfo.sock, 0, x);
      return;
    }

  switch ( ch )
    {
    case '0':           /* - '9', '=' :set warp factor */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '=':
      if ( ch == '=' )
	x = 10.0;
      else
	{
	  i = ch - '0';
	  x = (real) (i); 
	}
      dowarp( Context.snum, x );
      break;
    case 'a':				/* autopilot */
      if ( Users[Ships[Context.snum].unum].ooptions[ OOPT_AUTOPILOT] )
	{
	  doautopilot( Context.snum );
	}
      else
	{
	  goto label1;
	}
      break;
    case 'A':				/* change allocation */
      doalloc( Context.snum );
      break;
    case 'b':				/* beam armies */
      dobeam( Context.snum );
      break;
    case 'B':				/* bombard a planet */
      dobomb( Context.snum );
      break;
    case 'C':				/* cloak control */
      docloak( Context.snum );
      break;
    case 'd':				/* detonate enemy torps */
    case '*':
      dodet( Context.snum );
      break;
    case 'D':				/* detonate own torps */
      domydet( Context.snum );
      break;
    case 'E':				/* emergency distress call */
      dodistress( Context.snum );
      break;
    case 'f':				/* phasers */
      dophase( Context.snum );
      break;
    case 'F':				/* phasers, same direction */
      dolastphase( Context.snum );
      break;
    case 'h':
      Context.redraw = TRUE;
      Context.display = FALSE;
      dohelp();
      Context.display = TRUE;
      if ( stillalive( Context.snum ) )
        {
          stopTimer();
          display( Context.snum, FALSE );
          startTimer();
        }
      break;
    case 'H':
      Context.redraw = TRUE;
      Context.display = FALSE;
      histlist( FALSE );
      Context.display = TRUE;
      if ( stillalive( Context.snum ) )
        {
          stopTimer();
          display( Context.snum, FALSE );
          startTimer();
        }
      break;
    case 'i':				/* information */
      doinfo( Context.snum );
      break;
    case 'k':				/* set course */
      docourse( Context.snum );
      break;
    case 'K':				/* coup */
      docoup( Context.snum );
      break;
    case 'L':				/* review old messages */
      doreview( Context.snum );
      break;
    case 'm':				/* send a message */
      clntSendMsg( Context.snum, UserConf.Terse,
		   TRUE);
      break;
    case 'M':				/* strategic/tactical map */
      if (SMAP(Context.snum))
	SFCLR(Context.snum, SHIP_F_MAP);
      else
	SFSET(Context.snum, SHIP_F_MAP);
      stopTimer();
      display( Context.snum, FALSE );
      startTimer();
      break;
    case 'N':				/* change pseudonym */
      clntPseudo( Context.unum, Context.snum );
      break;

    case 'O':
      Context.redraw = TRUE;
      Context.display = FALSE;
      UserOptsMenu(Context.unum);
      Context.display = TRUE;
      if ( stillalive( Context.snum ) )
        {
          stopTimer();
          display( Context.snum, FALSE );
          startTimer();
        }
      break;
    case 'o':				/* orbit nearby planet */
      doorbit( Context.snum );
      break;
    case 'P':				/* photon torpedo burst */
      doburst( Context.snum );
      break;
    case 'p':				/* photon torpedoes */
      dotorp( Context.snum );
      break;
    case 'Q':				/* self destruct */
      doselfdest( Context.snum );
      break;
    case 'r':				/* refit */
      if (sStat.flags & SPSSTAT_FLAGS_REFIT)
	dorefit( Context.snum, TRUE );
      else
	cdbeep();
      break;
    case 'R':				/* repair mode */
      if ( ! SCLOAKED(Context.snum) )
	{
	  cdclrl( MSG_LIN1, 2 );
	  sendCommand(CPCMD_REPAIR, 0);
	}
      else
	{
	  cdclrl( MSG_LIN2, 1 );
	  c_putmsg(
		   "You cannot repair while the cloaking device is engaged.",
		   MSG_LIN1 );
	}
      break;
    case 't':				/* tow */
      dotow( Context.snum );
      break;
    case 'S':				/* more user stats */
      Context.redraw = TRUE;
      Context.display = FALSE;
      userstats( FALSE, Context.snum ); 
      Context.display = TRUE;
      if ( stillalive( Context.snum ) )
        {
          stopTimer();
          display( Context.snum, FALSE );
          startTimer();
        }
      break;
    case 'T':				/* team list */
      Context.redraw = TRUE;
      Context.display = FALSE;
      doteamlist( Ships[Context.snum].team );
      Context.display = TRUE;
      if ( stillalive( Context.snum ) )
        {
          stopTimer();
          display( Context.snum, FALSE );
          startTimer();
        }
      break;
    case 'u':				/* un-tractor */
      sendCommand(CPCMD_UNTOW, 0);
      break;
    case 'U':				/* user stats */
      Context.redraw = TRUE;
      Context.display = FALSE;
      userlist( FALSE, Context.snum );
      Context.display = TRUE;
      if ( stillalive( Context.snum ) )
        {
          stopTimer();
          display( Context.snum, FALSE );
          startTimer();
        }
      break;
    case 'W':				/* war and peace */
      clntDoWar( Context.snum );
      break;
    case '-':				/* shields down */
      doshields( Context.snum, FALSE );
      stopTimer();
      display( Context.snum, FALSE );
      startTimer();
      break;
    case '+':				/* shields up */
      doshields( Context.snum, TRUE );
      stopTimer();
      display( Context.snum, FALSE );
      startTimer();
      break;
    case '/':				/* player list */
      Context.redraw = TRUE;
      Context.display = FALSE;
      playlist( FALSE, FALSE, Context.snum );
      Context.display = TRUE;
      if ( stillalive( Context.snum ) )
        {
          stopTimer();
          display( Context.snum, FALSE );
          startTimer();
        }
      break;
    case '?':				/* planet list */
      Context.redraw = TRUE;
      Context.display = FALSE;
      doplanlist( Context.snum );
      Context.display = TRUE;
      if ( stillalive( Context.snum ) )
        {
          stopTimer();
          display( Context.snum, FALSE );
          startTimer();
        }
	display( Context.snum, FALSE );
      break;
    case TERM_REDRAW:			/* clear and redisplay */
      stopTimer();
      cdredo();
      Context.redraw = TRUE;
      display( Context.snum, FALSE );
      startTimer();
      break;
      
    case TERM_NORMAL:		/* Have [RETURN] act like 'I[RETURN]'  */
    case KEY_ENTER:
    case '\n':
      iBufPut("i\r");		/* (get last info) */
      break;

    case TERM_EXTRA:		/* Have [TAB] act like 'i\t' */
      iBufPut("i\t");		/* (get next last info) */
      break;
      
    case -1:			/* really nothing, move along */
#ifdef DEBUG_IO
      clog("command(): got -1 - ESC?");
#endif
      break;

      /* nothing. */
    default:
    label1:
      cdbeep();
#ifdef DEBUG_IO
      clog("command(): got 0%o, KEY_A1 =0%o", ch, KEY_A1);
#endif
      c_putmsg( "Type h for help.", MSG_LIN2 );
    }
  
  return;
  
}


/*  conqds - display background for Conquest */
/*  SYNOPSIS */
/*    int multiple, switchteams */
/*    conqds( multiple, switchteams ) */
void conqds( int multiple, int switchteams )
{
  int i, col, lin;
  extern char *ConquestVersion;
  extern char *ConquestDate;
  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#(#%d#%%c#%d#) - %%s",
	      LabelColor,
	      InfoColor,
	      LabelColor);
	}
  
  /* First clear the display. */
  cdclear();
  
  /* Display the logo. */
  lin = conqlogo();

  if ( ConqInfo->closed )
    cprintf(lin,0,ALIGN_CENTER,"#%d#%s",RedLevelColor,"The game is closed.");
  else
    cprintf( lin,0,ALIGN_CENTER,"#%d#%s (%s)",YellowLevelColor,
	   ConquestVersion, ConquestDate);
  
  lin++;
  cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Options:");
  
  col = 13;
  lin+=2;
  i = lin;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'e', "enter the game");
  if ( Context.hasnewsfile )
    {
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, 'n', "read the news");
    }
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'h', "read the help lesson");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'S', "more user statistics");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'T', "team statistics");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'U', "user statistics");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'L', "review messages");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'W', "set war or peace");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'I', "change user options");
  
  col = 48;
  lin = i;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'N', "change your name");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'O', "options menu");

  if ( ! multiple )
    {
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, 'r', "resign your commission");
    }
  if ( multiple || switchteams )
    {
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, 's', "switch teams");
    }
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'H', "user history");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, '/', "player list");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, '?', "planet list");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'q', "exit the program");
  
  return;
  
}


/*  dead - announce to a user that s/he is dead (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    int leave */
/*    dead( snum, leave ) */
void dead( int snum, int leave )
{
  int i, kb;
  int ch; 
  string ywkb="You were killed by ";
  char buf[128], junk[128];
  
  /* (Quickly) clear the screen. */
  cdclear();
  cdredo();
  cdrefresh();

  /* If something is wrong, don't do anything. */
  if ( snum < 1 || snum > MAXSHIPS )
    {
      clog("dead: snum < 1 || snum > MAXSHIPS (%d)", snum);
      return;
    }
  
  kb = Ships[snum].killedby;

  /* Figure out why we died. */
  switch ( kb )
    {
    case KB_SELF:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
		"You scuttled yourself.");

      break;
    case KB_NEGENB:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
		"You were destroyed by the negative energy barrier.");

      break;
    case KB_CONQUER:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
	       "Y O U   C O N Q U E R E D   T H E   U N I V E R S E ! ! !");
      break;
    case KB_NEWGAME:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"N E W   G A M E !");
      break;
    case KB_EVICT:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"Closed for repairs.");
      break;
    case KB_SHIT:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You are no longer allowed to play.");
      break;
    case KB_GOD:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You were killed by an act of GOD.");

      break;
    case KB_DOOMSDAY:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You were eaten by the doomsday machine.");

      break;
    case KB_GOTDOOMSDAY:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You destroyed the doomsday machine!");
      break;
    case KB_DEATHSTAR:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You were vaporized by the Death Star.");

      break;
    case KB_LIGHTNING:
      cprintf(8,0,ALIGN_CENTER,"#%d#%s", RedLevelColor, 
      	"You were destroyed by a lightning bolt.");

      break;
    default:
      
	  cbuf[0] = EOS;
	  buf[0] = EOS;
	  junk[0] = EOS;
      if ( kb > 0 && kb <= MAXSHIPS )
	{
	  appship( kb, cbuf );
	  if ( Ships[kb].status != SS_LIVE )
	    appstr( ", who also died.", buf );
	  else
	    appchr( '.', buf );
	  cprintf( 8,0,ALIGN_CENTER, 
		   "#%d#You were kill number #%d#%.1f #%d#for #%d#%s #%d#(#%d#%s#%d#)%s",
		   InfoColor, A_BOLD, Ships[kb].kills, 
		   InfoColor, A_BOLD, Ships[kb].alias, 
		   InfoColor, A_BOLD, cbuf, 
		   InfoColor, buf );
	}
      else if ( -kb > 0 && -kb <= NUMPLANETS )
	{
	  if ( Planets[-kb].type == PLANET_SUN )
	      strcpy(cbuf, "solar radiation.");
	  else
	      strcpy(cbuf, "planetary defenses.");
	  cprintf(8,0,ALIGN_CENTER,"#%d#%s#%d#%s%s#%d#%s", 
		InfoColor, ywkb, A_BOLD, Planets[-kb].name, "'s ",
		InfoColor, cbuf);

	}
      else
	{
	  /* We were unable to determine the cause of death. */
	  buf[0] = EOS;
	  appship( snum, buf );
	  sprintf(cbuf, "dead: %s was killed by %d.", buf, kb);
	  cerror( cbuf );
	  clog(cbuf);
	  
	  cprintf(8,0,ALIGN_CENTER,"#%d#%s%s", 
	  	RedLevelColor, ywkb, "nothing in particular.  (How strange...)");
	}
    }

  
  
  if ( kb == KB_NEWGAME )
    {
      cprintf( 10,0,ALIGN_CENTER,
		"#%d#Universe conquered by #%d#%s #%d#for the #%d#%s #%d#team.",
		 InfoColor, A_BOLD, ConqInfo->conqueror, 
		 InfoColor, A_BOLD, ConqInfo->conqteam, LabelColor );
    }
  else if ( kb == KB_SELF )
    {
      i = Ships[snum].armies;
      if ( i > 0 )
	{
	  junk[0] = EOS; 
	  if ( i == 1 )
	    strcpy( cbuf, "army" );
	  else
	    {
	      if ( i < 100 )
			appnum( i, junk );
	      else
			appint( i, junk );
	      strcpy( cbuf, "armies" );
	    }
	  if ( i == 1 )
	    strcpy( buf, "was" );
	  else
	    strcpy( buf, "were");
	  if ( i == 1 )
		cprintf(10,0,ALIGN_CENTER,
		"#%d#The #%d#%s #%d#you were carrying %s not amused.",
			LabelColor, A_BOLD, cbuf, LabelColor, buf);
	  else
		cprintf(10,0,ALIGN_CENTER,
		"#%d#The #%d#%s %s #%d#you were carrying %s not amused.",
			LabelColor, A_BOLD, junk, cbuf, LabelColor, buf);
	}
    }
  else if ( kb >= 0 )
    {
      if ( Ships[kb].status == SS_LIVE )
	{
	  cprintf( 10,0,ALIGN_CENTER,
		"#%d#He had #%d#%d%% #%d#shields and #%d#%d%% #%d#damage.",
		InfoColor, A_BOLD, round(Ships[kb].shields), 
		InfoColor, A_BOLD, round(Ships[kb].damage),InfoColor );
	}
    }
  cprintf(12,0,ALIGN_CENTER,
	"#%d#You got #%d#%.1f #%d#this time.", 
	InfoColor, A_BOLD, oneplace(Ships[snum].kills), InfoColor );
  cdmove( 1, 1 );
  cdrefresh();

  /* check the clientstat's flags to see if last words are needed */

  if (clientFlags & SPCLNTSTAT_FLAG_CONQUER)
    {
      c_sleep(4.0);
      do
	{
	  cdclear();
	  cdredo();
	  buf[0] = EOS;

	  ch = cdgetx( "Any last words? ",
		       14, 1, TERMS, buf, MAXLASTWORDS, TRUE );

	  cdclear();
	  cdredo();

	  if ( buf[0] != EOS )
	    {
	      cprintf( 13,0,ALIGN_CENTER, "#%d#%s", 
			InfoColor, "You last words are entered as:");
	      cprintf( 14,0,ALIGN_CENTER, "#%d#%c%s%c", 
			YellowLevelColor, '"', buf, '"' );
	    }
	  else
	    cprintf( 14,0,ALIGN_CENTER,"#%d#%s", InfoColor,
		   "You have chosen to NOT leave any last words:" );
	  ch = getcx( "Press TAB to confirm:", 16, 0,
		     TERMS, cbuf, 10 );
	}
      while ( ch != TERM_EXTRA ); /* until . while */

      /* now we just send a message */
      sendMessage(MSG_GOD, buf);
    }      

  /* set the ship reserved (locally).  The server has already done this
     but we have not processed any packets since we died.  This keeps
     menu() from booting us out of the game. */
  Ships[Context.snum].status = SS_RESERVED;

  ioeat();
  putpmt( MTXT_DONE, MSG_LIN2 );
  cdrefresh();
  while ( ! iogtimed( &ch, 1.0 ) )
    ;

  cdmove( 1, 1 );
  
  return;
  
}

/*  doalloc - change weapon/engine allocations */
/*  SYNOPSIS */
/*    int snum */
/*    doalloc( snum ) */
void doalloc( int snum )
{
  char ch;
  int i, alloc;
  int dwalloc = 0;
  string pmt="New weapons allocation: (30-70) ";
  
  cdclrl( MSG_LIN1, 2 );
  cbuf[0] = EOS;
  ch = (char)cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE, TRUE );

  switch (ch)
    {
    case TERM_EXTRA:
      dwalloc = Ships[snum].engalloc;
      break;

    case TERM_NORMAL:
      i = 0;
      safectoi( &alloc, cbuf, i );			/* ignore status */
      if ( alloc != 0 )
	{
	  if ( alloc < 30 )
	    alloc = 30;
	  else if ( alloc > 70 )
	    alloc = 70;
	  dwalloc = alloc;
	}
      else
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
     
      break;

    default:
      return;
    }

  sendCommand(CPCMD_ALLOC, (Unsgn16)dwalloc);

  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*  doautopilot - handle the autopilot */
/*  SYNOPSIS */
/*    int snum */
/*    doautopilot( snum ) */
void doautopilot( int snum )
{
  int ch; 
  string conf="Press TAB to engage autopilot:";
  
  cdclrl( MSG_LIN1, 2 );
  cbuf[0] = EOS;
  if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE, 
	       TRUE ) != TERM_EXTRA )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }

  lastServerError = 0;  
  sendCommand(CPCMD_AUTOPILOT, 1); /* turn over command to a machine */

  while ( stillalive( Context.snum ) )
    {
      if (lastServerError)
        break;

      /* Get a character. */
      if ( ! iogtimed( &ch, 1.0 ) )
	continue;		/* next . echo */
      Context.msgok = FALSE;
      grand( &Context.msgrand );
      switch ( ch )
	{
	case TERM_ABORT:
	  break;
	case TERM_REDRAW:	/* ^L */
	  cdredo();
	  break;
	default:
	  c_putmsg( "Press ESCAPE to abort autopilot.", MSG_LIN1 );
	  cdbeep();
	  cdrefresh();
	}
      Context.msgok = TRUE;
      if (ch == TERM_ABORT)
	break;
    }

  sendCommand(CPCMD_AUTOPILOT, 0);
  
  cdclrl( MSG_LIN1, 2 );
  
  return;
}


/*  dobeam - beam armies up or down (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobeam( snum ) */
void dobeam( int snum )
{
  int pnum, num, upmax, downmax, capacity, beamax, i;
  int dirup = FALSE;
  int ch; 
  char buf[MSGMAXLINE];
  real rkills;
  int done = FALSE;
  string lastfew="Fleet orders prohibit removing the last three armies.";
  string abt="...aborted...";
  
  cdclrl( MSG_LIN1, 2 );

  /* all of these checks are performed server-side as well, but we also
     check here for the obvious problems so we can save time without
     bothering the server for something it will refuse anyway. */
  
  /* at least the basic checks could be split into a seperate func that
     could be used by both client and server */

  /* Check for allowability. */
  if ( Ships[snum].warp >= 0.0 )
    {
      c_putmsg( "We must be orbiting a planet to use the transporter.",
	       MSG_LIN1 );
      return;
    }
  pnum = -Ships[snum].lock;
  if ( Ships[snum].armies > 0 )
    {
      if ( Planets[pnum].type == PLANET_SUN )
	{
	  c_putmsg( "Idiot!  Our armies will fry down there!", MSG_LIN1 );
	  return;
	}
      else if ( Planets[pnum].type == PLANET_MOON )
	{
	  c_putmsg( "Fool!  Our armies will suffocate down there!",
		   MSG_LIN1 );
	  return;
	}
      else if ( Planets[pnum].team == TEAM_GOD )
	{
	  c_putmsg(
		   "GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
		   MSG_LIN1 );
	  return;
	}
    }
  
  i = Planets[pnum].uninhabtime;
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minute",
	     i );
      if ( i != 1 )
	appchr( 's', cbuf );
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  
  /* can take empty planets */
  if ( Planets[pnum].team != Ships[snum].team &&
      Planets[pnum].team != TEAM_SELFRULED &&
      Planets[pnum].team != TEAM_NOTEAM )
    if ( ! Ships[snum].war[Planets[pnum].team] && Planets[pnum].armies != 0) 
      {
	c_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
	return;
      }
  
  if ( Ships[snum].armies == 0 &&
      Planets[pnum].team == Ships[snum].team && Planets[pnum].armies <= MIN_BEAM_ARMIES )
    {
      c_putmsg( lastfew, MSG_LIN1 );
      return;
    }
  
  rkills = Ships[snum].kills;

  if ( rkills < (real)1.0 )
    {
      c_putmsg(
	       "Fleet orders prohibit beaming armies until you have a kill.",
	       MSG_LIN1 );
      return;
    }
  
  /* Figure out what can be beamed. */
  downmax = Ships[snum].armies;
  if ( spwar(snum,pnum) ||
      Planets[pnum].team == TEAM_SELFRULED ||
      Planets[pnum].team == TEAM_NOTEAM ||
      Planets[pnum].team == TEAM_GOD ||
      Planets[pnum].armies == 0 )
    {
      upmax = 0;
    }
  else
    {
      capacity = min( ifix( rkills ) * 2, ShipTypes[Ships[snum].shiptype].armylim );
      upmax = min( Planets[pnum].armies - MIN_BEAM_ARMIES, 
		   capacity - Ships[snum].armies );
    }
  
  /* If there are armies to beam but we're selfwar... */
  if ( upmax > 0 && selfwar(snum) && Ships[snum].team == Planets[pnum].team )
    {
      if ( downmax <= 0 )
	{
	  c_strcpy( "The arm", cbuf );
	  if ( upmax == 1 )
	    appstr( "y is", cbuf );
	  else
	    appstr( "ies are", cbuf );
	  appstr( " reluctant to beam aboard a pirate vessel.", cbuf );
	  c_putmsg( cbuf, MSG_LIN1 );
	  return;
	}
      upmax = 0;
    }
  
  /* Figure out which direction to beam. */
  if ( upmax <= 0 && downmax <= 0 )
    {
      c_putmsg( "There is no one to beam.", MSG_LIN1 );
      return;
    }
  if ( upmax <= 0 )
    dirup = FALSE;
  else if ( downmax <= 0 )
    dirup = TRUE;
  else
    {
      c_putmsg( "Beam [up or down] ", MSG_LIN1 );
      cdrefresh();
      done = FALSE;
      while ( stillalive( Context.snum ) && done == FALSE)
	{
	  if ( ! iogtimed( &ch, 1.0 ) )
	    {
	      continue;	/* next */
	    }
	  switch ( (char)tolower( ch ) )
	    {
	    case 'u':
	    case 'U':
	      dirup = TRUE;
	      done = TRUE;
	      break;
	    case 'd':
	    case 'D':
	    case TERM_EXTRA:
	      dirup = FALSE;
	      done = TRUE;
	      break;
	    default:
	      c_putmsg( abt, MSG_LIN1 );
	      return;
	    }
	}
    }
  
  if ( dirup )
    beamax = upmax;
  else
    beamax = downmax;
  
  /* Figure out how many armies should be beamed. */
  if ( dirup )
    c_strcpy( "up", buf );
  else
    c_strcpy( "down", buf );
  sprintf( cbuf, "Beam %s [1-%d] ", buf, beamax );
  cdclrl( MSG_LIN1, 1 );
  buf[0] = EOS;
  ch = cdgetx( cbuf, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, TRUE );
  if ( ch == TERM_ABORT )
    {
      c_putmsg( abt, MSG_LIN1 );
      return;
    }
  else if ( ch == TERM_EXTRA && buf[0] == EOS )
    num = beamax;
  else
    {
      delblanks( buf );
      if ( alldig( buf ) != TRUE )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  return;
	}
      i = 0;
      safectoi( &num, buf, i );			/* ignore status */
      if ( num < 1 || num > beamax )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  return;
	}
    }

  /* now we start the phun. */

  lastServerError = 0;

  /* detail is (armies & 0x00ff), 0x8000 set if beaming down */

  sendCommand(CPCMD_BEAM, 
	      (dirup) ? (Unsgn16)(num & 0x00ff): 
	      (Unsgn16)((num & 0x00ff) | 0x8000));

  while(TRUE)			/* repeat infloop */
    {
      if ( ! stillalive( Context.snum ) )
	return;

      if ( iochav() )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  sendCommand(CPCMD_BEAM, 0); /* stop! */
	  break;
	}

      if (lastServerError)
	{
	  cdclrl( MSG_LIN1, 1 );
	  sendCommand(CPCMD_BEAM, 0); /* make sure */
	  break;
	}

      c_sleep( ITER_SECONDS );
    }
  
  /* Try to display the last beaming message. */
  cdrefresh();
  
  return;
  
}


/*  dobomb - bombard a planet (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobomb( snum ) */
void dobomb( int snum )
{
  int pnum;
  char  buf[MSGMAXLINE];
  string abt="...aborted...";
  
  SFCLR(snum, SHIP_F_REPAIR);;
  
  cdclrl( MSG_LIN2, 1 );
  cdclrl(MSG_LIN1, 1);
  
  /* Check for allowability. */
  if ( Ships[snum].warp >= 0.0 )
    {
      c_putmsg( "We must be orbiting a planet to bombard it.", MSG_LIN1 );
      return;
    }
  pnum = -Ships[snum].lock;
  if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON ||
      Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
    {
      c_putmsg( "There is no one there to bombard.", MSG_LIN1 );
      return;
    }
  if ( Planets[pnum].team == Ships[snum].team )
    {
      c_putmsg( "We can't bomb our own armies!", MSG_LIN1 );
      return;
    }
  if ( Planets[pnum].team != TEAM_SELFRULED && Planets[pnum].team != TEAM_GOD )
    if ( ! Ships[snum].war[Planets[pnum].team] )
      {
	c_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
	return;
      }
  
  /* Confirm. */
  sprintf( cbuf, "Press TAB to bombard %s, %d armies:",
	 Planets[pnum].name, Planets[pnum].armies );
  cdclrl( MSG_LIN1, 1 );
  cdclrl( MSG_LIN2, 1 );
  buf[0] = EOS;
  if ( cdgetx( cbuf, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE,
	       TRUE) != TERM_EXTRA )
    {
      cdclrl( MSG_LIN1, 1 );
      cdclrl( MSG_LIN2, 1 );
      return;
    }
  
  lastServerError = 0;		/* quit if something happens server-side */

  sendCommand(CPCMD_BOMB, 1);	/* start bombing */

  while(TRUE)
    {
      if ( ! stillalive( Context.snum ) )
	break;

      if ( iochav() )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  sendCommand(CPCMD_BOMB, 0);
	  break;
	}
      
      if (lastServerError)
	{
	  sendCommand(CPCMD_BOMB, 0); /* make sure */
	  break;
	}

      c_sleep( ITER_SECONDS );
    }
  
  cdrefresh();
  
  return;
  
}


/*  doburst - launch a burst of three torpedoes */
/*  SYNOPSIS */
/*    int snum */
/*    doburst( snum ) */
void doburst( int snum )
{
  real dir;
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( SCLOAKED(snum) )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( Ships[snum].wfuse > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( Ships[snum].fuel < TORPEDO_FUEL )
    {
      c_putmsg( "Not enough fuel to launch a torpedo.", MSG_LIN1 );
      return;
    }
  
  if ( gettarget( "Torpedo burst: ", MSG_LIN1, 1, &dir, Ships[snum].lastblast ) )
    {
      if ( ! checklaunch( snum, 3 ) )
	c_putmsg( ">TUBES EMPTY<", MSG_LIN2 );
      else
	{			/* a local approx */
	  sendFireTorps(3, dir);
	  cdclrl( MSG_LIN1, 1 );
	}
    }
  else
    {
      c_putmsg( "Invalid targeting information.", MSG_LIN1 );
    }

  
  return;
  
}


/*  docloak - cloaking device control */
/*  SYNOPSIS */
/*    int snum */
/*    docloak( snum ) */
void docloak( int snum )
{
  string pmt="Press TAB to engage cloaking device: ";
  string nofuel="Not enough fuel to engage cloaking device.";
  
  cdclrl( MSG_LIN1, 1 );
  cdclrl( MSG_LIN2, 1 );
  
  if ( SCLOAKED(snum) )
    {
      sendCommand(CPCMD_CLOAK, 0);
      c_putmsg( "Cloaking device disengaged.", MSG_LIN1 );
      SFCLR(snum, SHIP_F_CLOAKED); /* do it locally */
      return;
    }
  if ( Ships[snum].efuse > 0 )
    {
      c_putmsg( "Engines are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( Ships[snum].fuel < CLOAK_ON_FUEL )
    {
      c_putmsg( nofuel, MSG_LIN1 );
      return;
    }
  
  cdclrl( MSG_LIN1, 1 );
  cbuf[0] = EOS;
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
	       TRUE) == TERM_EXTRA )
    {
      if ( ! usefuel( snum, CLOAK_ON_FUEL, FALSE, TRUE ) )
	{			/* an approximation of course... */
	  c_putmsg( nofuel, MSG_LIN2 );
	  return;
	}

      sendCommand(CPCMD_CLOAK, 0);
      SFSET(snum, SHIP_F_CLOAKED); /* do it locally */
      c_putmsg( "Cloaking device engaged.", MSG_LIN2 );
    }
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}

/*  dorefit - attempt to change the nature of things */
/*  SYNOPSIS */
/*    int snum */
/*    dorefit( snum ) */
void dorefit( int snum, int dodisplay )
{
  int ch, pnum, now, entertime, leave;
  char buf1[128];
  int oldstype = 0, stype = 0;
  string ntp="We must be orbiting a team owned planet to refit.";
  string nek="You must have at least one kill to refit.";
  string conf="Press TAB to change, ENTER to accept: ";
  string cararm="You cannot refit while carrying armies";
  
  cdclrl( MSG_LIN2, 1 );
  
  /* Check for allowability. */
  if ( oneplace( Ships[snum].kills ) < MIN_REFIT_KILLS )
    {
      c_putmsg( nek, MSG_LIN1 );
      return;
    }

  pnum = -Ships[snum].lock;

  if (Planets[pnum].team != Ships[snum].team || Ships[snum].warp >= 0.0)
    {
      c_putmsg( ntp, MSG_LIN1 );
      return;
    }

  if (Ships[snum].armies != 0)
    {
      c_putmsg( cararm, MSG_LIN1 );
      return;
    }

				/* we have at least 1 kill, and are */
				/* orbiting a team owned planet */

  stype = oldstype = Ships[snum].shiptype;
  leave = FALSE;

  while ( stillalive( snum ) && !leave)
  {
    /* Display the current options. */
    
    cdclrl( MSG_LIN1, 1 );
    cdclrl( MSG_LIN2, 1 );    
    
    buf1[0] = '\0';
    appstr("Refit ship type: ", buf1);
    appstr(ShipTypes[stype].name, buf1);
    
    c_putmsg(buf1, MSG_LIN1);
    c_putmsg(conf, MSG_LIN2);
      
    cdrefresh();
      
    /* Get a character. */
    if ( ! iogtimed( &ch, 1.0 ) )
      continue; /* next; */
    switch ( ch )
      {
      case TERM_EXTRA:
	stype = modp1( stype + 1, MAXNUMSHIPTYPES );
	leave = FALSE;

	if ( dodisplay )
	  {
	    /* Force an update. */
	    stopTimer();
	    display( snum, FALSE );
	    startTimer();
	  }

	break;

      case TERM_NORMAL:
	leave = TRUE;
	break;

      case TERM_ABORT:
	/* Decided to abort; restore things. */

	stype = oldstype;
	leave = TRUE;

	if ( dodisplay )
	  {
	    /* Force an update. */
	    stopTimer();
	    display( snum, FALSE );		/* update the display */
	    startTimer();
	  }
	
	break;
      default:
	cdbeep();
	leave = FALSE;
	break;
      }
  }
  
  if (stype == oldstype)
    {
      cdclrl( MSG_LIN1, 1 );
      cdclrl( MSG_LIN2, 1 );
      
      return; 
    }

  /* Now wait it out... */
  cdclrl( MSG_LIN1, 1 );
  cdclrl( MSG_LIN2, 1 );
  c_putmsg( "Refitting ship...", MSG_LIN1 );
  sendCommand(CPCMD_REFIT, (Unsgn16)stype);
  cdrefresh();
  grand( &entertime );
  while ( dgrand( entertime, &now ) < REFIT_GRAND )
    {
      /* See if we're still alive. */
      if ( ! stillalive( snum ) )
	return;
      
      /* Sleep */
      c_sleep( ITER_SECONDS );
    }
  
  cdclrl( MSG_LIN1, 1 );
  cdclrl( MSG_LIN2, 1 );

  return;
  
}

/*  docoup - attempt to rise from the ashes (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    docoup( snum ) */
void docoup( int snum )
{
  int i, pnum;
  string nhp="We must be orbiting our home planet to attempt a coup.";
  string conf="Press TAB to try it: ";
  
  cdclrl( MSG_LIN2, 1 );
  
  /* some checks we will do locally, the rest will have to be handled by
     the server */
  /* Check for allowability. */
  if ( oneplace( Ships[snum].kills ) < MIN_COUP_KILLS )
    {
      c_putmsg(
	       "Fleet orders require three kills before a coup can be attempted.",
	       MSG_LIN1 );
      return;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( Planets[i].team == Ships[snum].team && Planets[i].armies > 0 )
      {
	c_putmsg( "We don't need to coup, we still have armies left!",
		 MSG_LIN1 );
	return;
      }
  if ( Ships[snum].warp >= 0.0 )
    {
      c_putmsg( nhp, MSG_LIN1 );
      return;
    }
  pnum = -Ships[snum].lock;
  if ( pnum != Teams[Ships[snum].team].homeplanet )
    {
      c_putmsg( nhp, MSG_LIN1 );
      return;
    }
  if ( Planets[pnum].armies > MAX_COUP_ENEMY_ARMIES )
    {
      c_putmsg( "The enemy is still too strong to attempt a coup.",
	       MSG_LIN1 );
      return;
    }
  i = Planets[pnum].uninhabtime;
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
	     i );
      c_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  

  /* at this point, we will just send the request, and let the server
     deal with it */

  /* Confirm. */
  cdclrl( MSG_LIN1, 1 );
  cbuf[0] = EOS;
  if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
	       TRUE) != TERM_EXTRA )
    {
      c_putmsg( "...aborted...", MSG_LIN1 );
      return;
    }

  c_putmsg( "Attempting coup...", MSG_LIN1 );
  sendCommand(CPCMD_COUP, 0);
  
  return;
  
}


/*  docourse - set course */
/*  SYNOPSIS */
/*    int snum */
/*    docourse( snum ) */
void docourse( int snum )
{
  int i, j, what, sorpnum, xsorpnum, newlock, token, count;
  real dir, appx, appy; 
  int ch; 
  
  cdclrl( MSG_LIN1, 2 );

  cbuf[0] = EOS;
  ch = cdgetx( "Come to course: ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE, TRUE );
  delblanks( cbuf );
  if ( ch == TERM_ABORT || cbuf[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  newlock = 0;				/* default to no lock */
  fold( cbuf );
  
  what = NEAR_ERROR;
  if ( alldig( cbuf ) == TRUE )
    {
      /* Raw angle. */
      cdclrl( MSG_LIN1, 1 );
      i = 0;
      if ( safectoi( &j, cbuf, i ) )
	{
	  what = NEAR_DIRECTION;
	  dir = (real)mod360( (real)( j ) );
	}
    }
  else if ( cbuf[0] == 's' && alldig( &cbuf[1] ) == TRUE )
    {
      /* Ship. */

      i = 1;
      if ( safectoi( &sorpnum, cbuf, i ) )
	what = NEAR_SHIP;
    }
  else if ( arrows( cbuf, &dir ) )
    what = NEAR_DIRECTION;
  else if ( special( cbuf, &i, &token, &count ) )
    {
      if ( findspecial( snum, token, count, &sorpnum, &xsorpnum ) )
	what = i;
    }
  else if ( planmatch( cbuf, &sorpnum, FALSE ) )
    what = NEAR_PLANET;
  
  switch ( what )
    {
    case NEAR_SHIP:
      if ( sorpnum < 1 || sorpnum > MAXSHIPS )
	{
	  c_putmsg( "No such ship.", MSG_LIN2 );
	  return;
	}
      if ( sorpnum == snum )
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
      if ( Ships[sorpnum].status != SS_LIVE )
	{
	  c_putmsg( "Not found.", MSG_LIN2 );
	  return;
	}

      if ( SCLOAKED(sorpnum) )
	{
	  if ( Ships[sorpnum].warp <= 0.0 )
	    {
	      c_putmsg( "Sensors are unable to lock on.", MSG_LIN2 );
	      return;
	    }
	}

      appx = Ships[sorpnum].x;
      appy = Ships[sorpnum].y;

      dir = (real)angle( Ships[snum].x, Ships[snum].y, appx, appy );
      
      /* Give info if he used TAB. */
      if ( ch == TERM_EXTRA )
	infoship( sorpnum, snum );
      else
	cdclrl( MSG_LIN1, 1 );
      break;
    case NEAR_PLANET:
      dir = angle( Ships[snum].x, Ships[snum].y, Planets[sorpnum].x, Planets[sorpnum].y );
      if ( ch == TERM_EXTRA )
	{
	  newlock = -sorpnum;
	  infoplanet( "Now locked on to ", sorpnum, snum );
	}
      else
	infoplanet( "Setting course for ", sorpnum, snum );
      break;
    case NEAR_DIRECTION:
      cdclrl( MSG_LIN1, 1 );
      break;
    case NEAR_NONE:
      c_putmsg( "Not found.", MSG_LIN2 );
      return;
      break;
    default:
      /* This includes NEAR_ERROR. */
      c_putmsg( "I don't understand.", MSG_LIN2 );
      return;
      break;
    }
  
  sendSetCourse(cInfo.sock, newlock, dir);

  return;
  
}


/*  dodet - detonate enemy torps */
/*  SYNOPSIS */
/*    int snum */
/*    dodet( snum ) */
void dodet( int snum )
{
  cdclrl( MSG_LIN2, 1 );
  
  if ( Ships[snum].wfuse > 0 )
    c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
  else if ( usefuel( snum, DETONATE_FUEL, TRUE, FALSE ) )
    {				/* we don't really use fuel here on the
				   client*/
      c_putmsg( "detonating...", MSG_LIN1 );
      sendCommand(CPCMD_DETENEMY, 0);
    }
  else
    c_putmsg( "Not enough fuel to fire detonators.", MSG_LIN1 );
  
  return;
  
}


/*  dodistress - send an emergency distress call */
/*  SYNOPSIS */
/*    int snum */
/*    dodistress( snum ) */
void dodistress( int snum )
{
  string pmt="Press TAB to send an emergency distress call: ";
  
  cdclrl( MSG_LIN1, 2 );

  cbuf[0] = EOS;
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
	       TRUE) == TERM_EXTRA )
    sendCommand(CPCMD_DISTRESS, (Unsgn16)UserConf.DistressToFriendly);
  
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*  dohelp - display a list of commands */
/*  SYNOPSIS */
/*    int subdcl */
/*    dohelp( subdcl ) */
void dohelp( void )
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
  
  cdclear();
  cprintf(1,0,ALIGN_CENTER, "#%d#%s", LabelColor, "CONQUEST COMMANDS");
  
  lin = 3;
  
  /* Display the left side. */
  tlin = lin;
  col = 4;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "0-9,=", "set warp factor (= is 10)");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "A", "change w/e allocations");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "b", "beam armies");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "B", "bombard a planet");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "C", "cloaking device");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "d,*", "detonate enemy torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "D", "detonate your own torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "E", "send emergency distress call");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "f", "fire phasers");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "F", "fire phasers, same direction");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "h", "this");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "H", "user history");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "i", "information");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "k", "set course");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "K", "try a coup");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "L", "review old messages");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "m", "send a message");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "M", "short/long range sensor toggle");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "N", "change your name");
  
  /* Now do the right side. */
  tlin = lin;
  col = 44;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "O", "options menu");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "o", "come into orbit");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "p", "launch photon torpedoes");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "P", "launch photon torpedo burst");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "Q", "initiate self-destruct");
  tlin++;
  if (sStat.flags & SPSSTAT_FLAGS_REFIT)
    {
      cprintf(tlin,col,ALIGN_NONE,sfmt, "r", "refit ship to new type");
      tlin++;
    }
  cprintf(tlin,col,ALIGN_NONE,sfmt, "R", "enter repair mode");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "S", "more user statistics");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "t", "engage tractor beams");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "T", "team list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "u", "un-engage tractor beams");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "U", "user statistics");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "W", "set war or peace");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "-", "lower shields");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "+", "raise shields");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "/", "player list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "?", "planet list");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "^L", "refresh the screen");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  "[RETURN]", "get last info");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "[TAB]", "get next last info");
  
  putpmt( MTXT_DONE, MSG_LIN2 );
  cdrefresh();
  while ( ! iogtimed( &ch, 1.0 ) && stillalive( Context.snum ) )
    ;
  
  return;
  
}


/*  doinfo - do an info command */
/*  SYNOPSIS */
/*    int snum */
/*    doinfo( snum ) */
void doinfo( int snum )
{
  char ch; 
  int i, j, what, sorpnum, xsorpnum, count, token, now[NOWSIZE]; 
  int extra; 
  
  cdclrl( MSG_LIN1, 2 );
  
  cbuf[0] = EOS;
  ch = (char)cdgetx( "Information on: ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
		     TRUE);
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  extra = ( ch == TERM_EXTRA );
  
  /* Default to what we did last time. */
  delblanks( cbuf );
  fold( cbuf );
  if ( cbuf[0] == EOS )
    {
      c_strcpy( Context.lastinfostr, cbuf );
      if ( cbuf[0] == EOS )
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
    }
  else
    c_strcpy( cbuf, Context.lastinfostr );
  
  if ( special( cbuf, &what, &token, &count ) )
    {
      if ( ! findspecial( snum, token, count, &sorpnum, &xsorpnum ) )
	what = NEAR_NONE;
      else if ( extra )
	{
	  if ( xsorpnum == 0 )
	    what = NEAR_NONE;
	  else
	    sorpnum = xsorpnum;
	}
      
      if ( what == NEAR_SHIP )
	infoship( sorpnum, snum );
      else if ( what == NEAR_PLANET )
	infoplanet( "", sorpnum, snum );
      else
	c_putmsg( "Not found.", MSG_LIN2 );
    }
  else if ( cbuf[0] == 's' && alldig( &cbuf[1] ) == TRUE )
    {
      i = 1;
      safectoi( &j, cbuf, i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( alldig( cbuf ) == TRUE )
    {
      i = 0;
      safectoi( &j, cbuf, i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( planmatch( cbuf, &j, FALSE ) )
    infoplanet( "", j, snum );
  else if ( stmatch( cbuf, "time", FALSE ) )
    {
      getnow( now, 0 );
      c_strcpy( "It's ", cbuf );
      appnumtim( now, cbuf );
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
    }
  else
    {
      c_putmsg( "I don't understand.", MSG_LIN2 );
      cdmove( MSG_LIN1, 1 );
    }
  
  return;
  
}


/*  dolastphase - do a fire phasers same direction command */
/*  SYNOPSIS */
/*    int snum */
/*    dolastphase( snum ) */
void dolastphase( int snum )
{
  cdclrl( MSG_LIN1, 1 );
  
  if ( SCLOAKED(snum) )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN2 );
      return;
    }
  if ( Ships[snum].wfuse > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( Ships[snum].fuel < PHASER_FUEL )
    {
      c_putmsg( "Not enough fuel to fire phasers.", MSG_LIN2 );
      return;
    }
  
  if ( phaser( snum, Ships[snum].lastphase ) )
    {				/* a local approximation of course */
      sendCommand(CPCMD_FIREPHASER, (Unsgn16)(Ships[snum].lastphase * 100.0));
      cdclrl( MSG_LIN2, 1 );
    }
  else
    c_putmsg( ">PHASERS DRAINED<", MSG_LIN2 );
  
  return;
  
}


/*  domydet - detonate your own torps */
/*  SYNOPSIS */
/*    int snum */
/*    domydet( snum ) */
void domydet( int snum )
{
  cdclrl( MSG_LIN2, 1 );
  
  sendCommand(CPCMD_DETSELF, 0);

  c_putmsg( "Detonating...", MSG_LIN1 );
  
  return;
  
}

/*  doorbit - orbit the ship and print a message */
/*  SYNOPSIS */
/*    int snum */
/*    doorbit( snum ) */
void doorbit( int snum )
{
  int pnum;
  
  if ( ( Ships[snum].warp == ORBIT_CW ) || ( Ships[snum].warp == ORBIT_CCW ) )
    infoplanet( "But we are already orbiting ", -Ships[snum].lock, snum );
  else if ( ! findorbit( snum, &pnum ) )
    {
      sprintf( cbuf, "We are not close enough to orbit, %s.",
	     Ships[snum].alias );
      c_putmsg( cbuf, MSG_LIN1 );
      cdclrl( MSG_LIN2, 1 );
    }
  else if ( Ships[snum].warp > MAX_ORBIT_WARP )
    {
      sprintf( cbuf, "We are going to fast to orbit, %s.",
	     Ships[snum].alias );
      c_putmsg( cbuf, MSG_LIN1 );
      sprintf( cbuf, "Maximum orbital insertion velocity is warp %.1f.",
	     oneplace(MAX_ORBIT_WARP) );
      c_putmsg( cbuf, MSG_LIN2 );
    }
  else
    {
      sendCommand(CPCMD_ORBIT, 0);
      infoplanet( "Coming into orbit around ", pnum, snum );
    }
  
  return;
  
}


/*  dophase - do a fire phasers command */
/*  SYNOPSIS */
/*    int snum */
/*    dophase( snum ) */
void dophase( int snum )
{
  real dir;
  
  cdclrl( MSG_LIN2, 1 );
  if ( SCLOAKED(snum) )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( Ships[snum].wfuse > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( Ships[snum].fuel < PHASER_FUEL )
    {
      c_putmsg( "Not enough fuel to fire phasers.", MSG_LIN1 );
      return;
    }
  
  if ( gettarget( "Fire phasers: ", MSG_LIN1, 1, &dir, Ships[snum].lastblast ) )
    {
      if ( Ships[snum].pfuse <= 0 && usefuel( snum, PHASER_FUEL, 
					      TRUE, FALSE ) )
	{			/* a local approximation of course */
	  c_putmsg( "Firing phasers...", MSG_LIN2 );
	  sendCommand(CPCMD_FIREPHASER, (Unsgn16)(dir * 100.0));
	}
      else
	c_putmsg( ">PHASERS DRAINED<", MSG_LIN2 );
    }
  else
    {
      c_putmsg( "Invalid targeting information.", MSG_LIN1 );
    }

  return;
  
}


/*  doplanlist - display the planet list for a ship */
/*  SYNOPSIS */
/*    int snum */
/*    doplanlist( snum ) */
void doplanlist( int snum )
{

  if (snum > 0 && snum <= MAXSHIPS)
    planlist( Ships[snum].team, snum );
  else		/* then use user team if user doen't have a ship yet */
    planlist( Users[Context.unum].team, snum );
  
  return;
  
}


/*  doreview - review messages for a ship */
/*  SYNOPSIS */
/*    int snum */
/*    doreview( snum ) */
void doreview( int snum )
{
  int ch;
  int lstmsg;			/* saved last msg in case new ones come in */
  
  if (RMsg_Line == MSG_LIN1)
    {				/* if we don't have an extra msg line,
				   then make sure new msgs don't come
				   in while reviewing */
      
      Context.msgok = FALSE;		/* don't want to get msgs when reading
				   old ones.  */
    }

  lstmsg = Ships[snum].lastmsg;	/* don't want lstmsg changing while reading old ones. */

  if ( ! review( snum, lstmsg ) )
    {
      c_putmsg( "There are no old messages.", MSG_LIN1 );
      putpmt( MTXT_MORE, MSG_LIN2 );
      cdrefresh();
      while ( ! iogtimed( &ch, 1.0 ) && stillalive( Context.snum ) )
	;
      cdclrl( MSG_LIN1, 2 );
    }

  if (RMsg_Line == MSG_LIN1)
    {
      Context.msgok = TRUE;		
    }

  return;
  
}


/*  doselfdest - execute a self-destruct command */
/*  SYNOPSIS */
/*    doselfdest */
void doselfdest(int snum)
{
  string pmt="Press TAB to initiate self-destruct sequence: ";
  
  cdclrl( MSG_LIN1, 2 );

  if ( SCLOAKED(snum) )
    {
      c_putmsg( "The cloaking device is using all available power.",
               MSG_LIN1 );
      return;
    }

  cbuf[0] = EOS;
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
	       TRUE) != TERM_EXTRA )
    {
      /* Chickened out. */
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  cdclrl( MSG_LIN1, 1 );
  
  lastServerError = 0;
  sendCommand(CPCMD_DESTRUCT, 1); /* blow yourself up */

  while ( TRUE )
    {
      if ( ! stillalive( Context.snum ) )
	return;			/* Died in the process. */
      
      if ( iochav() )
	{
	  /* Got a new character. */
	  cdclrl( MSG_LIN1, 2 );
	  if ( iogchar() == TERM_ABORT )
	    {
	      sendCommand(CPCMD_DESTRUCT, 0);
	      c_putmsg( "Self destruct has been canceled.", MSG_LIN1 );
	      return;
	    }
	  else
	    {
	      c_putmsg( "Press ESCAPE to abort self destruct.", MSG_LIN1 );
	      cdbeep();
	      cdrefresh();
	    }
	}

      if (lastServerError)
        {
          sendCommand(CPCMD_DESTRUCT, 0); /* make sure */
          break;
        }


      c_sleep( ITER_SECONDS );
    } /* end while */

  return;
  
}


/*  doshields - raise or lower shields */
/*  SYNOPSIS */
/*    int snum */
/*    int up */
/*    doshields( snum, up ) */
void doshields( int snum, int up )
{

  if (!sendCommand(CPCMD_SETSHIELDS, (Unsgn16)up))
    return;

  if ( up )
    {
      SFCLR(snum, SHIP_F_REPAIR);
      c_putmsg( "Shields raised.", MSG_LIN1 );
    }
  else
    c_putmsg( "Shields lowered.", MSG_LIN1 );

  cdclrl( MSG_LIN2, 1 );
  
  return;
  
}


/*  doteamlist - display the team list for a ship */
/*  SYNOPSIS */
/*    int team */
/*    doteamlist( team ) */
void doteamlist( int team )
{
  int ch;
  
  cdclear();
  while ( stillalive( Context.snum ) )
    {
      teamlist( team );
      putpmt( MTXT_DONE, MSG_LIN2 );
      cdrefresh();
      if ( iogtimed( &ch, 1.0 ) )
	break;
    }
  return;
  
}


/*  dotorp - launch single torpedoes */
/*  SYNOPSIS */
/*    int snum */
/*    dotorp( snum ) */
void dotorp( int snum )
{
  real dir;
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( SCLOAKED(snum) )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( Ships[snum].wfuse > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( Ships[snum].fuel < TORPEDO_FUEL )
    {
      c_putmsg( "Not enough fuel to launch a torpedo.", MSG_LIN1 );
      return;
    }
  if ( gettarget( "Launch torpedo: ", MSG_LIN1, 1, &dir, Ships[snum].lastblast ) )
    {
      if ( ! checklaunch( snum, 1 ) )
	c_putmsg( ">TUBES EMPTY<", MSG_LIN2 );
      else
	{			/* a local approx */
	  sendFireTorps(1, dir);
	  cdclrl( MSG_LIN1, 1 );
	}
    }
  else
    {
      c_putmsg( "Invalid targeting information.", MSG_LIN1 );
    }
  
  return;
  
}

/*  dotow - attempt to tow another ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dotow( snum ) */
void dotow( int snum )
{
  char ch;
  int i, other;
  
  cdclrl( MSG_LIN1, 2 );
  if ( Ships[snum].towedby != 0 )
    {
      c_strcpy( "But we are being towed by ", cbuf );
      appship( Ships[snum].towing, cbuf );
      appchr( '!', cbuf );
      c_putmsg( cbuf, MSG_LIN2 );
      return;
    }
  if ( Ships[snum].towing != 0 )
    {
      c_strcpy( "But we're already towing ", cbuf );
      appship( Ships[snum].towing, cbuf );
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN2 );
      return;
    }
  cbuf[0] = EOS;
  ch = (char)cdgetx( "Tow which ship? ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
		     TRUE);
  cdclrl( MSG_LIN1, 1 );
  if ( ch == TERM_ABORT )
    return;
  
  i = 0;
  safectoi( &other, cbuf, i );		/* ignore status */

  sendCommand(CPCMD_TOW, (Unsgn16)other);

  return;
  
}


/*  dowarp - set warp factor */
/*  SYNOPSIS */
/*    int snum */
/*    real warp */
/*    dowarp( snum, warp ) */
void dowarp( int snum, real warp )
{
  cdclrl( MSG_LIN2, 1 );
  
  /* Handle ship limitations. */
  
  warp = min( warp, ShipTypes[Ships[snum].shiptype].warplim );
  if (!sendCommand(CPCMD_SETWARP, (Unsgn16)warp))
    return;
  
  sprintf( cbuf, "Warp %d.", (int) warp );
  c_putmsg( cbuf, MSG_LIN1 );
  
  return;
  
}

/*  gretds - block letter "greetings..." */
/*  SYNOPSIS */
/*    gretds */
void gretds()
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


/*  menu - main user menu (DOES LOCKING) */
/*  SYNOPSIS */
/*    menu */
void menu(void)
{
  
  int i, lin, col, sleepy, countdown;
  int ch;
  int lose, oclosed, switchteams, multiple, redraw;
  int playrv;
  int rv;
  int pkttype;
  struct timeval timeout;
  fd_set readfds;
  Unsgn8 buf[PKT_MAXSIZE];
  spAck_t *sack;
  char *if1="Suddenly  a  sinister,  wraithlike  figure appears before you";
  char *if2="seeming to float in the air.  In a low,  sorrowful  voice  he";
  char *if3="says, \"Alas, the very nature of the universe has changed, and";
  char *if4="your ship cannot be found.  All must now pass away.\"  Raising";
  char *if5="his  oaken  staff  in  farewell,  he fades into the spreading";
  char *if6="darkness.  In his place appears a  tastefully  lettered  sign";
  char *if7="reading:";
  char *if8="INITIALIZATION FAILURE";
  char *if9="The darkness becomes all encompassing, and your vision fails.";
  
  catchSignals();	/* enable trapping of interesting signals */
  

  /* we will init some things.  Then we will look for either an
     SP_SHIP or NAK packet (if something bad happened) */

  /* Initialize statistics. */
  initstats( &Ships[Context.snum].ctime, &Ships[Context.snum].etime );
  
  /* Log this entry into the Game. */
  Context.histslot = loghist( Context.unum );
  
  /* Set up some things for the menu display. */
  switchteams = Users[Context.unum].ooptions[OOPT_SWITCHTEAMS];
  multiple = Users[Context.unum].ooptions[OOPT_MULTIPLE];
  oclosed = ConqInfo->closed;
  Context.leave = FALSE;
  redraw = TRUE;
  sleepy = 0;
  countdown = 0;
  playrv = FALSE;
  

  /* now look for our ship packet before we get started.  It should be a
     full SP_SHIP packet for this first time */
  if (waitForPacket(PKT_FROMSERVER, cInfo.sock, SP_SHIP, buf, PKT_MAXSIZE,
		    15, NULL) <= 0)
    {
      clog("conquest:menu: didn't get initial SP_SHIP");
      return;
    }
  else
    procShip(buf);

  do                 
    {
      /* Make sure things are proper. */

      lose = FALSE;

      while ((pkttype = waitForPacket(PKT_FROMSERVER, cInfo.sock, PKT_ANYPKT,
				      buf, PKT_MAXSIZE, 0, NULL)) > 0)
	{			/* proc packets while we get them */
	  switch (pkttype)
	    {
	    case SP_ACK:
	      sack = (spAck_t *)buf;
	      if (sack->code == PERR_LOSE)
		lose = TRUE;
	      else
		clog("conquest:menu: got unexp ack code %d", sack->code);

	      break;

	    default:
	      processPacket(buf);
	      break;
	    }
	}
	      
      if (pkttype < 0)		/* some error */
	{
	  clog("conquest:menu:waiForPacket returned %d", pkttype);
	  Ships[Context.snum].status = SS_OFF;
	  return;
	}


      if ( lose )				/* again, Jorge? */
	{
	  /* We reincarnated or else something bad happened. */
	  lin = 7;
	  col = 11;
	  cdclear();;
	  cdredo();;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if1);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if2);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if3);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if4);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if5);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if6);
	  lin++;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if7);
	  lin+=2;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor | A_BLINK, if8);
	  lin+=2;
	  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if9);
	  ioeat();
	  cdmove( 1, 1 );
	  cdrefresh();
	  return;
	}
      
      /* Some simple housekeeping. */
      if ( multiple != Users[Context.unum].ooptions[OOPT_MULTIPLE] )
	{
	  multiple = ! multiple;
	  redraw = TRUE;
	}
      
      if ( switchteams != Users[Context.unum].ooptions[OOPT_SWITCHTEAMS])
	{
	  switchteams = Users[Context.unum].ooptions[OOPT_SWITCHTEAMS];
	  redraw = TRUE;
	}
      if ( oclosed != ConqInfo->closed )
	{
	  oclosed = ! oclosed;
	  redraw = TRUE;
	}
      if ( redraw )
	{
	  conqds( multiple, switchteams );
	  redraw = FALSE;
	}
      else
	cdclrl( MSG_LIN1, 2 );
      
      userline( -1, -1, cbuf, FALSE, TRUE );
      attrset(LabelColor);
      cdputs( cbuf, MSG_LIN1, 1 );
      userline( Context.unum, 0, cbuf, FALSE, TRUE );
      attrset(A_BOLD);
      cdputs( cbuf, MSG_LIN2, 1 );
      attrset(0);
      
      cdmove( 1, 1 );
      cdrefresh();
      
      /*  we've been in the menu long enough? */
      if ( countdown > 0 )
	countdown--;
      
      /* wait up to a second for something to happen */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      
      FD_ZERO(&readfds);
      FD_SET(cInfo.sock, &readfds);
      FD_SET(PollInputfd, &readfds);

      if ((rv=select((max(cInfo.sock, PollInputfd) + 1), &readfds, NULL, 
                     NULL, &timeout)) > 0)
        {                           /* we have activity */
          if (FD_ISSET(cInfo.sock, &readfds))
            {                   /* we have a packet */
              continue;            /* go process them at the top */
            }

          if (FD_ISSET(PollInputfd, &readfds))
            {          /* got a char */
              ch = iogchar();

              /* Got a character, zero timeout. */
              sleepy = 0;
              switch ( ch )
                {
                case 'e':
                  if (!sendCommand(CPCMD_ENTER, 0))
                    playrv = FALSE;
                  else
                    {
                      playrv = play();
                      countdown = 15;
                      redraw = TRUE;
                    }
                  if (playrv == FALSE)
                    Context.leave = TRUE; /* something didn't work right */
                  
                  Context.display = FALSE;
                  break;
                case 'h':
                  helplesson();
                  redraw = TRUE;
                  break;
                case 'H':
                  histlist( FALSE );
                  redraw = TRUE;
                  break;
                case 'L':
                  doreview( Context.snum );
                  break;
                case 'n':
                  if ( ! Context.hasnewsfile )
                    cdbeep();
                  else
                    {
                      news();
                      redraw = TRUE;
                    }
                  break;
                case 'N':
                  /*	  pseudo( Context.unum, Context.snum );*/
                  clntPseudo( Context.unum, Context.snum );
                  break;
                case 'O':
                  UserOptsMenu(Context.unum);
                  redraw = TRUE;
                  break;
                case 'r':
                  if ( multiple )
                    cdbeep();
                  else
                    {
                      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
                        if ( Ships[i].status == SS_LIVE ||
                             Ships[i].status == SS_ENTERING )
                          if ( Ships[i].unum == Context.unum )
                            break;
                      if ( i <= MAXSHIPS )
                        cdbeep();
                      else
                        {
                          cdclrl( MSG_LIN1, 2 );
                          cdrefresh();
                          if ( confirm() )
                            {
                              /* should exit here */
                              sendCommand(CPCMD_RESIGN, 0);
                              cdend();
                              exit(0);
                              break;
                            }
                        }
                    }
                  break;
                case 's':
                  if ( ! multiple && ! switchteams )
                    cdbeep();
                  else
                    {
                      /* we'll update local data here anyway, even though it will be
                         overwritten on the next ship update.  Improves perceived
                         response time. */
                      Ships[Context.snum].team = 
                        modp1( Ships[Context.snum].team+1, NUMPLAYERTEAMS );
                      Ships[Context.snum].shiptype = 
                        Teams[Ships[Context.snum].team].shiptype;
                      Users[Context.unum].team = Ships[Context.snum].team;
                      Ships[Context.snum].war[Ships[Context.snum].team] = FALSE;
                      Users[Context.unum].war[Users[Context.unum].team] = FALSE;
                      
                      sendCommand(CPCMD_SWITCHTEAM, (Unsgn16)Ships[Context.snum].team);
                    }
                  break;
                case 'S':
                  userstats( FALSE, 0 ); /* we're never really neutral ;-) - dwp */
                  redraw = TRUE;
                  break;
                case 'T':
                  doteamlist( Ships[Context.snum].team );
                  redraw = TRUE;
                  break;
                case 'U':
                  userlist( FALSE, 0 );
                  redraw = TRUE;
                  break;
                case 'W':
                  /*	  dowar( Context.snum );*/
                  clntDoWar( Context.snum );
                  redraw = TRUE;
                  break;
                case 'q':
                case 'Q':
                  Context.leave = TRUE;	
                  break;
                case '/':
                  playlist( FALSE, FALSE, 0 );
                  redraw = TRUE;
                  break;
                case '?':
                  doplanlist( 0 );
                  redraw = TRUE;
                  break;
                case TERM_REDRAW:	/* ^L */
                  cdredo();
                  redraw = TRUE;
                  break;
                case ' ':
                case TERM_NORMAL:           
                  /* Do nothing. */
                  break;
                default:
                  cdbeep();
                  break;
                }
            }
        }

      if (rv < 0 && errno != EINTR)
        {
          clog("conquest:menu: select returned %d, %s\n",
               rv, strerror(errno));
          Context.leave = TRUE;
        }

      /* We get here if a char hasn't been typed. */
      sleepy++;
      if ( sleepy > 300 )
        break;
    }
  while ( stillalive( Context.snum ) &&  !Context.leave );

  return;
  
}


/*  newship - here we will await a ClientStat from the server (indicating
    our possibly new ship), or a NAK indicating a problem.
*/
/*  SYNOPSIS */
/*    int status, newship, unum, snum */
/*    int flag, newship */
/*    flag = newship( unum, snum ) */
int newship( int unum, int *snum )
{
  int i, j;
  char cbuf[MSGMAXLINE];
  spAck_t *sack;
  spClientStat_t *scstat;
  int pkttype;
  Unsgn8 buf[PKT_MAXSIZE];

  /* here we will wait for ack's or a clientstat pkt. Acks indicate an
     error.  If the clientstat pkt's esystem is !0, we need to prompt
     for the system to enter and send it in a CP_COMMAND:CPCMD_ENTER
     pkt. */


  while (TRUE)
    {
      if ((pkttype = waitForPacket(PKT_FROMSERVER, cInfo.sock, PKT_ANYPKT,
				   buf, PKT_MAXSIZE, 30, NULL)) < 0)
	{
	  clog("conquest:newship: waitforpacket returned %d", pkttype);
	  return FALSE;
	}
      
      switch (pkttype)
	{
	case 0:			/* timeout */
	  return FALSE;
	  break;
	  
	case SP_ACK:		/* bummer */
	  sack = (spAck_t *)buf;
	  switch (sack->code)
	    {
	    case PERR_FLYING:
	      cdclear();
	      cdredo();
	      sprintf(cbuf, "You're already playing on another ship.");
	      cprintf(5,0,ALIGN_CENTER,"#%d#%s",InfoColor, cbuf);
	      Ships[*snum].status = SS_RESERVED;
	      putpmt( "--- press any key ---", MSG_LIN2 );
	      cdrefresh();	      

	      iogchar();
	      
	      break;
	      
	    case PERR_TOOMANYSHIPS:
	      cdclear();
	      cdredo();
	      i = MSG_LIN2/2;
	      cdputc(
		     "I'm sorry, but your playing on too many ships right now.", i );
	      i = i + 1;
	      c_strcpy( "You are only allowed to fly ", cbuf );
	      j = Users[unum].multiple;
	      appint( j, cbuf );
	      appstr( " ship", cbuf );
	      if ( j != 1 )
		appchr( 's', cbuf );
	      appstr( " at one time.", cbuf );
	      cdputc( cbuf, i );
	      cdrefresh();
	      c_sleep( 2.0 );
	      Ships[*snum].status = SS_RESERVED;
	      
	      break;
	      
	    default:
	      clog("conquest:newship: unexpected ack code %d",
		   sack->code);
	      break;
	    }
	  
	  return FALSE;		/* always a failure */
	  break;
	  
	case SP_CLIENTSTAT:
	  scstat = (spClientStat_t *)buf;
	  
	  /* first things first */
	  Context.unum = (int)ntohs(scstat->unum);
	  Context.snum = scstat->snum;
	  Ships[Context.snum].team = scstat->team;
	  
	  if (scstat->esystem == 0)	/* we are done */
	    return TRUE;
	  
	  /* otherwise, need to prompt for system to enter */
	  if (selectentry(scstat->esystem))
	    return TRUE;		/* done */
	  else
	    return FALSE;
	  
	  break;
	  
	  /* we might get these packets too */
	case SP_PLANET:
	case SP_PLANETSML:
	case SP_PLANETLOC:
	case SP_SHIP:
	case SP_SHIPSML:
	case SP_SHIPLOC:
	case SP_MESSAGE:
	case SP_USER:
	case SP_TEAM:
	case SP_HISTORY:
	  processPacket(buf);

	  break;

	default:
	  clog("conquest:newship: unexpected packet type %d", pkttype);
	  break;
	}
    }

  /* if we are here, something unexpected happened */
  return FALSE;			/* NOTREACHED */

  
}

void processPacket(Unsgn8 *buf)
{
  int pkttype;
  spClientStat_t *scstat;
  spAck_t *sack;
  spAckMsg_t *sackm;

  if (!buf)
    return;

  pkttype = (int)buf[0];

  switch (pkttype)
    {
    case SP_ACK:
      sack = (spAck_t *)buf;
      lastServerError = sack->code;
      break;
    case SP_ACKMSG:
      sackm = (spAckMsg_t *)buf;
      sackm->txt[MESSAGE_SIZE - 1] = 0;
      lastServerError = sackm->code;
      break;
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
    case SP_CLIENTSTAT:
      scstat = (spClientStat_t *)buf;
      Context.snum = scstat->snum;
      Context.unum = (int)ntohs(scstat->unum);
      Ships[Context.snum].team = scstat->team;
      clientFlags = scstat->flags;
      break;

    case SP_MESSAGE:
      procMessage(buf);
      break;
      
    case SP_SERVERSTAT:
      procServerStat(buf);
      break;

    case SP_CONQINFO:
      procConqInfo(buf);
      break;

    case SP_HISTORY:
      procHistory(buf);
      break;

    default:
      clog("conquest:processPacket: got unexpected packet type %d",
	   pkttype);
      break;
    }

  return;
}


/*  play - play the game  ( PLAY ) */
/*  SYNOPSIS */
/*    play */
int play()
{
  int ch, rv;
  struct timeval timeout;
  fd_set readfds;
  
  /* Can't carry on without a vessel. */

  if ( (rv = newship( Context.unum, &Context.snum )) != TRUE)
    return(rv);
  else
    Context.entship = TRUE;
  
  Ships[Context.snum].sdfuse = 0;	/* zero self destruct fuse */
  grand( &Context.msgrand );		/* initialize message timer */
  Context.leave = FALSE;		/* assume we won't want to bail */
  Context.redraw = TRUE;		/* want redraw first time */
  Context.msgok = TRUE;		/* ok to get messages */
  cdclear();			/* clear the display */
  cdredo();			/*  (quickly) */
  stopTimer();			/* stop the display interrupt */
  display( Context.snum, FALSE );	/* update the screen manually */
  Context.display = TRUE;		/* ok to display */
  clientFlags = 0;
  startTimer();			/* setup for next second */
  

  /* start recording if neccessary */
  if (Context.recmode == RECMODE_STARTING)
    {
      if (recordInitOutput(Context.unum, getnow(NULL, 0), Context.snum))
        {
          Context.recmode = RECMODE_ON;
          /* need to tell the server to resend all the crap it already
             sent in menu so it can be recorded... sorry. */
          sendCommand(CPCMD_RELOAD, 0);
        }
      else
        Context.recmode = RECMODE_OFF;
    }

  /* While we're alive, field commands and process them. Inbound
     packets are handled by astservice() */
  /* when we become 'killed', then dead processing can begin. */
  while ( !(clientFlags & SPCLNTSTAT_FLAG_KILLED) )
    {

      /* Get a packet or char with one second timeout. */
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;

      FD_ZERO(&readfds);
      FD_SET(cInfo.sock, &readfds);
      FD_SET(PollInputfd, &readfds);

      if ((rv=select((max(cInfo.sock, PollInputfd) + 1), &readfds, NULL, 
                     NULL, &timeout)) > 0)
        {                           /* we have activity */
          if (FD_ISSET(cInfo.sock,&readfds))
            {                   /* we have a packet */
              /* call astservice.  it will get any packets,
                 and update the display */
              stopTimer();      /* to be sure */
              astservice(0);    /* will restart the timer */
            }

      /* Get a char with one second timeout. */

          if (FD_ISSET(PollInputfd, &readfds))
            {
              ch = iogchar();
              /* only process commands if we are live (the server will ignore
                 them anyway) */
              if (Ships[Context.snum].status == SS_LIVE)
                {
                  if (RMsg_Line == MSG_LIN1)
                    Context.msgok = FALSE;      /* off if we  have no msg line */
                  if (DoMacro(ch) == TRUE)
                    {
                      while (iBufCount())
                        {
                          ch = iBufGetCh();
                          command( ch );
                        }
                    }
                  else
                    {
                      do 
                        {
                          command( ch );
                        } while (iBufCount() && (ch = iBufGetCh()));
                    }
                }
            }
        }


      if (rv < 0 && errno != EINTR)
        {
          clog("conquest:play: select returned %d, %s\n",
               rv, strerror(errno));
          Context.display = FALSE;
          return FALSE;
        }
      else
        {
          Context.msgok = TRUE;
	  cdrefresh();
        }

    } /* while stillalive */
	  
  Context.display = FALSE;
  
  /* Asts are still enabled, simply cancel the next screen update. */
  stopTimer();

  c_sleep( 1.0 );
  clog("INFO: Died, calling dead()");
  dead( Context.snum, Context.leave );
  
  return(TRUE);
  
}


/*  welcome - entry routine */
/*  SYNOPSIS */
/*    int flag, welcome */
/*    int unum */
/*    flag = welcome( unum ) */
int welcome( int *unum )
{
  int i, team, col; 
  char name[MAXUSERNAME];
  string sorry1="I'm sorry, but the game is closed for repairs right now.";
  string sorry2="I'm sorry, but there is no room for a new player right now.";
  string sorryn="Please try again some other time.  Thank you.";
  char * selected_str="You have been selected to command a";
  char * starship_str=" starship.";
  char * prepare_str="Prepare to be beamed aboard...";
  spClientStat_t *scstat = NULL;
  spAck_t *sack = NULL;
  int pkttype;
  Unsgn8 buf[PKT_MAXSIZE];
  
  
  col=0;

  if (!Logon(name))
    {
      clog("conquest: Logon failed.");
      return FALSE;
    }

  /* now look for SP_CLIENTSTAT or SP_ACK */
  if ((pkttype = 
       readPacket(PKT_FROMSERVER, cInfo.sock, buf, PKT_MAXSIZE, 10)) <= 0)
    {
      clog("welcome: read failed\n");
      return FALSE;
    }

  switch (pkttype)
    {
    case SP_CLIENTSTAT:
      scstat = (spClientStat_t *)buf;

      *unum = (int)ntohs(scstat->unum);
      Context.snum = scstat->snum;
      Ships[Context.snum].team = scstat->team;

      break;
    case SP_ACK:
      sack = (spAck_t *)buf;
      break;
    default:
      clog("conquest:welcome: got unexpected packet type %d\n", pkttype);
      return FALSE;
      break;
    }

  if ( pkttype == SP_CLIENTSTAT && (scstat->flags & SPCLNTSTAT_FLAG_NEW) )
    {				
      /* Must be a new player. */
      cdclear();
      cdredo();
      if ( ConqInfo->closed )
	{
	  /* Can't enroll if the game is closed. */
	  cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
	  cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
	  cdmove( 1, 1 );
	  cdrefresh();
	  c_sleep( 2.0 );
	  return ( FALSE );
	}
      team = scstat->team;
      cbuf[0] = EOS;
      apptitle( team, cbuf );
      appchr( ' ', cbuf );
      i = strlen( cbuf );
      appstr( name, cbuf );
      cbuf[i] = (char)toupper( cbuf[i] );

      gretds();			/* 'GREETINGS' */

      if ( vowel( Teams[team].name[0] ) )
      	cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s%c #%d#%s #%d#%s",
		InfoColor,selected_str,'n',A_BOLD,Teams[team].name,
		InfoColor,starship_str);
      else
	cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s #%d#%s #%d#%s",
		InfoColor,selected_str,A_BOLD,Teams[team].name,
		InfoColor,starship_str);
      cprintf(MSG_LIN2/2+1,0,ALIGN_CENTER,"#%d#%s",
	      InfoColor, prepare_str );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 3.0 );
    }


  if (pkttype == SP_ACK)	/* some problem was detected */
    {
      switch (sack->code)
	{
	case PERR_CLOSED:
	  cdclear();
	  cdredo();
	  cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
	  cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
	  cdmove( 1, 1 );
	  cdrefresh();
	  c_sleep( 2.0 );
	  return ( FALSE );

	  break;

	case PERR_REGISTER:
          cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry2 );
          cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
          cdmove( 1, 1 );
          cdrefresh();
          c_sleep( 2.0 );
          return ( FALSE );

	  break;

	case PERR_NOSHIP:
	  cdclear();
	  cdredo();
	  cdputc( "I'm sorry, but there are no ships available right now.",
		  MSG_LIN2/2 );
	  cdputc( sorryn, MSG_LIN2/2+1 );
	  cdmove( 1, 1 );
	  cdrefresh();
	  c_sleep( 2.0 );
	  return ( FALSE );

	  break;

	default:
	  clog("welcome: unexpected ACK code %d\n", sack->code);
	  return FALSE;
	  break;

	}

      return FALSE;		/* NOTREACHED */
    }

  /* if we are here, then we recieved a client stat pkt, and now must wait
     for a user packet for this user. */
      

  if (waitForPacket(PKT_FROMSERVER, cInfo.sock, SP_USER, buf, PKT_MAXSIZE,
		    15, NULL) <= 0)
    {
      clog("conquest:welcome: waitforpacket SP_USER returned error");
      return FALSE;
    }
  else
    procUser(buf);


  /* ready to rock. */
  return ( TRUE );
}


int hello(void)
{
  cpHello_t chello;
  spAckMsg_t *sackmsg;
  Unsgn8 buf[PKT_MAXSIZE];
  int pkttype;
  extern char *ConquestVersion, *ConquestDate;


  /* there should be a server hello waiting for us */
  if ((pkttype = readPacket(PKT_FROMSERVER, cInfo.sock, 
			    buf, PKT_MAXSIZE, 10)) < 0)
  {
    clog("HELLO: read server hello failed\n");
    return FALSE;
  }

  if (pkttype == 0)
  {
    clog("HELLO: read server hello: timeout.\n");
    return FALSE;
  }

  if (pkttype != SP_HELLO)
  {
    clog("HELLO: read server hello: wrong packet type %d\n", pkttype);
    return FALSE;
  }

  sHello = *(spHello_t *)buf;

  /* fix up byte ordering */
  sHello.protover = (Unsgn16)ntohs(sHello.protover);
  sHello.cmnrev = (Unsgn32)ntohl(sHello.cmnrev);

  sHello.servername[CONF_SERVER_NAME_SZ - 1] = 0;
  sHello.serverver[CONF_SERVER_NAME_SZ - 1] = 0;
  sHello.motd[CONF_SERVER_MOTD_SZ - 1] = 0;

  clog("HELLO: CLNT: sname = '%s'\n"
       "             sver = '%s'\n"
       "             protv = 0x%04hx, cmnr = %d, flags: 0x%02x\n"
       "             motd = '%s",
       sHello.servername,
       sHello.serverver,
       sHello.protover,
       sHello.cmnrev,
       sHello.flags,
       sHello.motd);

  /* now send a client hello */
  chello.type = CP_HELLO;
  chello.updates = Context.updsec;
  chello.protover = htons(PROTOCOL_VERSION);
  chello.cmnrev = htonl(COMMONSTAMP);

  strncpy(chello.clientname, CLIENTNAME, CONF_SERVER_NAME_SZ);
  strncpy(chello.clientver, ConquestVersion, CONF_SERVER_NAME_SZ);

  strcat(chello.clientver, " ");
  strncat(chello.clientver, ConquestDate, 
	  (CONF_SERVER_NAME_SZ - strlen(ConquestVersion)) - 2);

  if (!writePacket(PKT_TOSERVER, cInfo.sock, (Unsgn8 *)&chello))
    {
      clog("HELLO: write client hello failed\n");
      return FALSE;
    }

  clog("HELLO: sent client hello to server");

  /* now we need a server stat or a Nak */

  if ((pkttype = readPacket(PKT_FROMSERVER, cInfo.sock, 
			    buf, PKT_MAXSIZE, 5)) < 0)
  {
    clog("HELLO: read server SP_ACKMSG or SP_ACK failed\n");
    return FALSE;
  }

  if (pkttype == SP_ACKMSG || pkttype == SP_ACK)/* we only get this if problem */
    {
      if (pkttype == SP_ACKMSG)
	{
	  sackmsg = (spAckMsg_t *)buf;
	  if (sackmsg->txt)
	    {
	      clog("conquest:hello:NAK:%s '%s'\n", 
		   psev2String(sackmsg->severity), 
		   sackmsg->txt);
	      printf("conquest:hello:NAK:%s '%s'\n", 
		   psev2String(sackmsg->severity), 
		   sackmsg->txt);

	    }
	}
      return FALSE;
    }

  if (pkttype == SP_SERVERSTAT)
    {
      procServerStat(buf);
# if defined(DEBUG_CLIENTPROC)
      clog("HELLO: recv SP_SERVERSTAT: ships = %d, na = %d, nv = %d, nr = %d\n"
           " nu = %d flags = 0x%08x",
	   sStat.numtotal,
	   sStat.numactive,
	   sStat.numvacant,
	   sStat.numrobot,
	   sStat.numusers,
	   sStat.flags);
#endif
    }
  else
    {
      clog("HELLO: pkttype = %d, was waiting for SP_SERVERSTAT", pkttype);
      return FALSE;
    }

  sendAck(cInfo.sock, PKT_TOSERVER, PSEV_INFO, PERR_OK, NULL);

  return TRUE;
}

void catchSignals(void)
{
#ifdef DEBUG_SIG
  clog("catchSignals() ENABLED");
#endif
  
  signal(SIGHUP, (void (*)(int))handleSignal);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTERM, (void (*)(int))handleSignal);  
  signal(SIGINT, SIG_IGN);
  signal(SIGQUIT, (void (*)(int))handleSignal);
  
  return;
}

void handleSignal(int sig)
{
  
#ifdef DEBUG_SIG
  clog("handleSignal() got SIG %d", sig);
#endif
  
  switch(sig)
    {
    case SIGQUIT:
    case SIGINT:
    case SIGTERM:
    case SIGHUP:
      stopTimer();
      cdclear();
      cdrefresh();
      conqend();		/* sends a disconnect packet */
      cdend();
      
      exit(0);
      break;

    default:
      break;
    }

  catchSignals();	/* reset */
  return;
}

/*  astservice - ast service routine for conquest */
/*  SYNOPSIS */
/*    astservice */
/* This routine gets called from a sys$setimr ast. Normally, it */
/* checks for new packet and outputs one screen update and then sets up */
/* another timer request. */
void astservice(int sig)
{
  int now;
  int readone;
  static int RMsggrand = 0;
  int difftime;
  Unsgn8 buf[PKT_MAXSIZE];

  stopTimer();

  /* good time to look for packets... We do this here so the display
     doesn't stop when executing various commands */


  while (readPacket(PKT_FROMSERVER, cInfo.sock,
		    buf, PKT_MAXSIZE, 0) > 0)
    processPacket(buf); /* process them */

  /* Don't do anything if we're not supposed to. */
  if ( ! Context.display )
    {
      startTimer();
      return;
    }

  /* See if we can display a new message. */

				/* for people with 25 lines, we
				   use a different timer so that
				   NEWMSG_GRAND intervals will determine
				   whether it's time to display a new
				   message... Otherwise, Context.msgrand
				   is used - which means that NEWMSG_GRAND
				   interval will have to pass after issuing
				   any command before a new msg will disp
				   12/28/98 */
  readone = FALSE;
  if ( Context.msgok )
    {
      if (RMsg_Line != MSG_LIN1)
	{			/* we have line 25 for msgs */
	  difftime = dgrand( RMsggrand, &now );
	}
      else
	{
	  difftime = dgrand( Context.msgrand, &now );
	}

      if ( difftime >= NEWMSG_GRAND )
	if ( getamsg( Context.snum, &Ships[Context.snum].lastmsg ) )
	  {
	    if (readmsg( Context.snum, Ships[Context.snum].lastmsg, 
			 RMsg_Line ) == TRUE)
	      {
		if (Msgs[Ships[Context.snum].lastmsg].msgfrom != 
		    Context.snum)
		  if (UserConf.MessageBell)
		    cdbeep();
		/* set both timers, regardless of which
		   one we're actally concerned with */
		Context.msgrand = now;
		RMsggrand = now;
		readone = TRUE;
	      }
	  }
    }

  /* Perform one ship display update. */
  display( Context.snum, FALSE );

  recordUpdateFrame();          /* update recording */
  
  /* Un-read the message if there's a chance it got garbaged. */
  if ( readone )
    if (RMsg_Line == MSG_LIN1)	/* we don't have an extra msg line */
      if ( iochav() )
	Ships[Context.snum].lastmsg = modp1( Ships[Context.snum].lastmsg - 1, MAXMESSAGES );
  
  /* Schedule for next time. */
  startTimer();

  return;
  
}

/*  stopTimer - cancel timer */
/*  SYNOPSIS */
/*    stopTimer */
void stopTimer(void)
{
  int old_disp;                 /* whether we should re-enable
                                   Context.display */
#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif
  
  /* we need to turn off display while doing this.  Preserve the previous
     value on return */
  old_disp = Context.display;
  if (Context.display)
    Context.display = FALSE;
  

  signal(SIGALRM, SIG_IGN);
  
#ifdef HAS_SETITIMER
  itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
  itimer.it_value.tv_usec = itimer.it_interval.tv_usec = 0;
  
  setitimer(ITIMER_REAL, &itimer, NULL);
#else
  alarm(0);
#endif

  Context.display = old_disp;
  
  return;
  
}


/*  startTimer - set timer to display() */
/*  SYNOPSIS */
/*    csetimer */
void startTimer(void)
{
  static struct sigaction Sig;

#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif
  
  Sig.sa_handler = (void (*)(int))astservice;
  
  Sig.sa_flags = 0;

  if (sigaction(SIGALRM, &Sig, NULL) == -1)
    {
      clog("startTimer():sigaction(): %s\n", strerror(errno));
      exit(errno);
    }
  
#ifdef HAS_SETITIMER

  if (Context.updsec >= 1 && Context.updsec <= 10)
    {
      if (Context.updsec == 1)
	{
	  itimer.it_value.tv_sec = 1;
	  itimer.it_value.tv_usec = 0;
	}
      else
	{
	  itimer.it_value.tv_sec = 0;
	  itimer.it_value.tv_usec = (1000000 / Context.updsec);
	}
    }
  else
    {
      itimer.it_value.tv_sec = 0;
      itimer.it_value.tv_usec = (1000000 / 2); /* 2/sec */
    }

  itimer.it_interval.tv_sec = itimer.it_value.tv_sec;
  itimer.it_interval.tv_usec = itimer.it_value.tv_usec;

  setitimer(ITIMER_REAL, &itimer, NULL);
#else
  alarm(1);			/* set alarm() */
#endif  
  return;
  
}

/*  conqend - machine dependent clean-up */
/*  SYNOPSIS */
/*    conqend */
void conqend(void)
{
  sendCommand(CPCMD_DISCONNECT, 0); /* tell the server */
  recordCloseOutput();

  return;
  
}

void dispServerInfo(int lin, metaSRec_t *metaServerList, int num)
{
  char buf[MID_BUFFER_SIZE];

  cdline ( lin, 0, lin, Context.maxcol );
  lin++;

  sprintf(buf, "#%d#Server: #%d#  %%s", MagentaColor, NoColor);
  cprintf(lin++, 0, ALIGN_NONE, buf, metaServerList[num].servername);

  sprintf(buf, "#%d#Version: #%d# %%s", MagentaColor, NoColor);
  cprintf(lin++, 0, ALIGN_NONE, buf, metaServerList[num].serverver);

  sprintf(buf, 
	  "#%d#Status: #%d#  Ships #%d#%%d/%%d #%d#"
	  "(#%d#%%d #%d#active, #%d#%%d #%d#vacant, "
	  "#%d#%%d #%d#robot)",
	  MagentaColor, NoColor, CyanColor, NoColor,
	  CyanColor, NoColor, CyanColor, NoColor, 
	  CyanColor, NoColor);

  cprintf(lin++, 0, ALIGN_NONE, buf,
          (metaServerList[num].numactive + metaServerList[num].numvacant +
           metaServerList[num].numrobot),
	  metaServerList[num].numtotal, 
          metaServerList[num].numactive, 
	  metaServerList[num].numvacant, metaServerList[num].numrobot);

  sprintf(buf, "#%d#Flags: #%d#   %%s", MagentaColor, NoColor);
  cprintf(lin++, 0, ALIGN_NONE, buf, 
          clntServerFlagsStr(metaServerList[num].flags));

  sprintf(buf, "#%d#MOTD: #%d#    %%s", MagentaColor, NoColor);
  cprintf(lin++, 0, ALIGN_NONE, buf, metaServerList[num].motd);
  cdline ( lin, 0, lin, Context.maxcol );
  return;
}


int selectServer(metaSRec_t *metaServerList, int nums)
{
  int i, k;
  static char *header = "Server List";
  static char *header2fmt = "(Page %d of %d)";
  static char headerbuf[BUFFER_SIZE];
  static char header2buf[BUFFER_SIZE];
  static char *eprompt = "Arrow keys to select, [TAB] to accept, any other key to quit.";
  int Done = FALSE;
  int ch;
  char *dispmac;
  int lin = 0, col = 0, flin, llin, clin, pages, curpage;
  const int servers_per_page = 10;
  char servervec[META_MAXSERVERS][MAXHOSTNAME + 10]; /* hostname + port */

				/* this is the number of required pages,
				   though page accesses start at 0 */
  if (nums >= servers_per_page)
    {
      pages = nums / servers_per_page;
      if ((nums % servers_per_page) != 0)
	pages++;		/* for runoff */
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

  cdclear();			/* First clear the display. */

  flin = 9;			/* first server line */
  llin = 0;			/* last server line on this page */
  clin = 0;			/* current server line */


  while (Done == FALSE)
    {
      sprintf(header2buf, header2fmt, curpage + 1, pages);
      sprintf(headerbuf, "%s %s", header, header2buf);

      cdclrl( 1, MSG_LIN2);	/* clear screen area */
      lin = 1;
      col = ((int)(cdcols() - strlen(headerbuf)) / 2);

      cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, headerbuf);
      
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

	  cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d#",
		  InfoColor, dispmac, NoColor);

	  lin++;
	  i++;
	}

				/* now the editing phase */
      cdclrl( MSG_LIN1, 2  );
      cdputs(eprompt, MSG_LIN1, 1);

      if (clin >= llin)
	clin = llin - 1;

      dispServerInfo(2, metaServerList, clin);
      cdmove(flin + clin, 1); 
      
      /* Get a char */
      ch = iogchar();
      

      switch(ch)
	{
	case KEY_UP:		/* up */
	case KEY_LEFT:
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

	case KEY_DOWN:		/* down */
	case KEY_RIGHT:
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

	case KEY_PPAGE:		/* prev page */
	  if (pages != 1)
	    {
	      curpage--;
	      if (curpage < 0)
		{
		  curpage = pages - 1;
		}
	    }

	  break;

	case KEY_NPAGE:		/* next page */
	  if (pages != 1)
	    {
	      curpage++;
	      if (curpage >= pages)
		{
		  curpage = 0;
		}
	    }

	  break;

	case TERM_EXTRA:	/* change something */
          return ((curpage * servers_per_page) + clin);
	  break;

	default:		/* everything else */
          return ERR;
	  break;
	}

    }

  return TRUE;
}
