#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q U E S T */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
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
#include "conqcom2.h"
#include "global.h"
#include "color.h"

static char *conquestId = "$Id$";
static char cbuf[MID_BUFFER_SIZE]; /* general purpose buffer */

/*  conquest - main program */
main(int argc, char *argv[]) 
{
  
  if ((ConquestUID = GetConquestUID()) == ERR)
    {
      fprintf(stderr, "conquest: GetConquestUID() failed\n");
      exit(1);
    }
  
  if ((ConquestGID = GetConquestGID()) == ERR)
    {
      fprintf(stderr, "conquest: GetConquestGID() failed\n");
      exit(1);
    }
  
#ifdef DEBUG_CONFIG
  clog("%s@%d: main() Reading Configuration files.", __FILE__, __LINE__);
#endif
  
  if (GetSysConf(FALSE) == ERR)
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
    }
				/* do this BEFORE setgid() call! */
  if (GetConf(FALSE, 0) == ERR)	/* always a UT_LOCAL user at this point */
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetConf() returned ERR.", __FILE__, __LINE__);
#endif
      exit(1);
    }
  
  
  if (setgid(ConquestGID) == -1)
    {
      clog("conquest: setgid(%d): %s",
           ConquestGID,
           sys_errlist[errno]);
      fprintf(stderr, "conquest: setgid(): failed\n");
      exit(1);
    }
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() *STARTING*", __FILE__, __LINE__);
#endif
  
#ifdef USE_SEMS
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() getting semephores - GetSem()", __FILE__, __LINE__);
#endif
  
  if (GetSem() == ERR)
    {
      fprintf(stderr, "GetSem() failed to get semaphores. exiting.\n");
      exit(1);
    }
#endif
  
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() mapping common block.", __FILE__, __LINE__);
#endif
  
  map_common();
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting conqinit().", __FILE__, __LINE__);
#endif
  
  conqinit();			/* machine dependent initialization */
  
  rndini( 0, 0 );		/* initialize random numbers */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting cdinit().", __FILE__, __LINE__);
#endif
  
  
  cdinit();			/* set up display environment */
  
  CqContext.maxlin = cdlins();
  
  CqContext.maxcol = cdcols();
  
  
  
  CqContext.snum = 0;		/* force menu to get a new ship */
  CqContext.histslot = ERR;

#ifdef DEBUG_FLOW
  clog("%s@%d: main() welcoming player.", __FILE__, __LINE__);
#endif
  
  if ( welcome( &CqContext.unum ) )
    menu();
  
  drpexit();			/* make the driver go away */
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
int capentry( int snum, int *system )
{
  int i, j; 
  int ch; 
  int owned[NUMPLAYERTEAMS]; 
  
  /* First figure out which systems we can enter from. */
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      owned[i] = FALSE;
      /* We must own all three planets in a system. */
      for ( j = 0; j < 3; j = j + 1 )
	{
	  if ( Planets[Teams[i].teamhplanets[j]].team != Ships[snum].team )
	    goto cnext2_1; /* next 2; */
	}
      owned[i] = TRUE;
    cnext2_1:
      ;
    }
  owned[Ships[snum].team] = TRUE;		/* always can enter in our system */
  
  /* Now count how many systems we can enter from. */
  j = 0;
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    if ( owned[i] )
      j = j + 1;
  
  /* If we can only enter from one, we're done. */
  if ( j <= 1 )
    {
      *system = Ships[snum].team;
      return ( TRUE );
    }
  
  /* Prompt for a decision. */
  c_strcpy( "Enter which system", cbuf );
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
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
  
  while ( stillalive( CqContext.snum ) )
    {
      if ( ! iogtimed( &ch, 1 ) )
	continue; /* next */
      switch  ( ch )
	{
	case TERM_NORMAL:
	case TERM_ABORT:
	  return ( FALSE );
	  break;
	case TERM_EXTRA:
	  /* Enter the home system. */
	  *system = Ships[snum].team;
	  return ( TRUE );
	  break;
	default:
	  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	    if ( Teams[i].teamchar == (char)toupper( ch ) && owned[i] )
	      {
		/* Found a good one. */
		*system = i;
		return ( TRUE );
	      }
	  /* Didn't get a good one; complain and try again. */
	  cdbeep();
	  cdrefresh();
	  break;
	}
    }
  
  return ( FALSE );			/* can get here because of stillalive() */
  
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
      
      if ( Ships[CqContext.snum].warp < 0.0 ) 
	Ships[CqContext.snum].warp = 0.0; 
      Ships[CqContext.snum].dhead = (real)(x); 
      Ships[CqContext.snum].lock = 0; 
      
      return;
    }
  
  switch ( ch )
    {
    case '0':           /* - '9', '=':	/* set warp factor */
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
      dowarp( CqContext.snum, x );
      break;
    case 'a':				/* autopilot */
      if ( Users[Ships[CqContext.snum].unum].ooptions[ OOPT_AUTOPILOT] )
	{
	  doautopilot( CqContext.snum );
	}
      else
	{
	  goto label1;
	}
      break;
    case 'A':				/* change allocation */
      doalloc( CqContext.snum );
      stoptimer();
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case 'b':				/* beam armies */
      dobeam( CqContext.snum );
      break;
    case 'B':				/* bombard a planet */
      dobomb( CqContext.snum );
      break;
    case 'C':				/* cloak control */
      docloak( CqContext.snum );
      break;
    case 'd':				/* detonate enemy torps */
    case '*':
      dodet( CqContext.snum );
      break;
    case 'D':				/* detonate own torps */
      domydet( CqContext.snum );
      break;
    case 'E':				/* emergency distress call */
      dodistress( CqContext.snum );
      break;
    case 'f':				/* phasers */
      dophase( CqContext.snum );
      break;
    case 'F':				/* phasers, same direction */
      dolastphase( CqContext.snum );
      break;
    case 'h':
      CqContext.redraw = TRUE;
      stoptimer();
      dohelp();
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case 'H':
      CqContext.redraw = TRUE;
      stoptimer();
      histlist( FALSE );
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case 'i':				/* information */
      doinfo( CqContext.snum );
      break;
    case 'I':				/* set user options */
      dooption( CqContext.snum, TRUE );
      break;
    case 'k':				/* set course */
      docourse( CqContext.snum );
      break;
    case 'K':				/* coup */
      docoup( CqContext.snum );
      break;
    case 'L':				/* review old messages */
      doreview( CqContext.snum );
      break;
    case 'm':				/* send a message */
      sendmsg( CqContext.snum, Ships[CqContext.snum].options[OPT_TERSE] );
      break;
    case 'M':				/* strategic/tactical map */
      Ships[CqContext.snum].map = ! Ships[CqContext.snum].map;	
      stoptimer();
      display( CqContext.snum, FALSE );
      settimer();
      break;
    case 'N':				/* change pseudonym */
      pseudo( CqContext.unum, CqContext.snum );
      break;

    case 'O':
      stoptimer();
      CqContext.redraw = TRUE;
      UserOptsMenu(CqContext.unum);
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case 'o':				/* orbit nearby planet */
      doorbit( CqContext.snum );
      break;
    case 'P':				/* photon torpedo burst */
      doburst( CqContext.snum );
      break;
    case 'p':				/* photon torpedoes */
      dotorp( CqContext.snum );
      break;
    case 'Q':				/* self destruct */
      doselfdest( CqContext.snum );
      break;
    case 'R':				/* repair mode */
      if ( ! Ships[CqContext.snum].cloaked )
	{
	  cdclrl( MSG_LIN1, 2 );
	  Ships[CqContext.snum].rmode = TRUE;
	  Ships[CqContext.snum].dwarp = 0.0;
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
      dotow( CqContext.snum );
      break;
    case 'S':				/* more user stats */
      CqContext.redraw = TRUE;
      stoptimer();
      userstats( FALSE, CqContext.snum ); 
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case 'T':				/* team list */
      CqContext.redraw = TRUE;
      stoptimer();
      doteamlist( Ships[CqContext.snum].team );
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case 'u':				/* un-tractor */
      dountow( CqContext.snum );
      break;
    case 'U':				/* user stats */
      CqContext.redraw = TRUE;
      stoptimer();
      userlist( FALSE, CqContext.snum );
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case 'W':				/* war and peace */
      dowar( CqContext.snum );
      break;
    case '-':				/* shields down */
      doshields( CqContext.snum, FALSE );
      stoptimer();
      display( CqContext.snum, FALSE );
      settimer();
      break;
    case '+':				/* shields up */
      doshields( CqContext.snum, TRUE );
      stoptimer();
      display( CqContext.snum, FALSE );
      settimer();
      break;
    case '/':				/* player list */
      CqContext.redraw = TRUE;
      stoptimer();
      playlist( FALSE, FALSE, CqContext.snum );
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case '?':				/* planet list */
      CqContext.redraw = TRUE;
      stoptimer();
      doplanlist( CqContext.snum );
      if ( stillalive( CqContext.snum ) )
	display( CqContext.snum, FALSE );
      settimer();
      break;
    case '\014':			/* clear and redisplay */
      stoptimer();
      cdredo();
      CqContext.redraw = TRUE;
      display( CqContext.snum, FALSE );
      settimer();
      break;
      
    case TERM_NORMAL:		/* Have [RETURN] act like 'I[RETURN]'  */
    case KEY_ENTER:
    case '\n':
      iBufPut("i\r");		/* (get last info) */
      break;

    case TERM_EXTRA:		/* Have [TAB] act like 'i\t' */
      iBufPut("i\t");		/* (get next last info) */
      break;
      
    case -1:			/* really nothing */
#ifdef DEBUG_IO
      clog("command(): got -1 - ESC?");
#endif
      break;

      /* nothing. */
    default:
    label1:
      /*	    ioeat();*/
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
  int i, col, lin, lenc1;
  string c1=" CCC    OOO   N   N   QQQ   U   U  EEEEE   SSSS  TTTTT";
  string c2="C   C  O   O  NN  N  Q   Q  U   U  E      S        T";
  string c3="C      O   O  N N N  Q   Q  U   U  EEE     SSS     T";
  string c4="C   C  O   O  N  NN  Q  Q   U   U  E          S    T";
  string c5=" CCC    OOO   N   N   QQ Q   UUU   EEEEE  SSSS     T";
  
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
  lenc1 = strlen( c1 );
  col = (CqContext.maxcol-lenc1) / 2;
  lin = 2;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c1);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c2);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c3);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c4);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c5);

  /* Draw a box around the logo. */
  lin++;
  attrset(A_BOLD);
  cdbox( 1, col-2, lin, col+lenc1+1 );
  attrset(0);
  
  lin++;
  if ( ConqInfo->closed )
    cprintf(lin,0,ALIGN_CENTER,"#%d#%s",RedLevelColor,"The game is closed.");
  else
    cprintf( lin,col,ALIGN_CENTER,"#%d#%s (%s)",YellowLevelColor,
	   ConquestVersion, ConquestDate);
  
  lin++;
  cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Options:");
  
  col = 13;
  lin+=2;
  i = lin;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'e', "enter the game");
  if ( CqContext.hasnewsfile )
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
  int i, j, kb, now, entertime; 
  int ch; 
  string ywkb="You were killed by ";
  char buf[128], junk[128];
  
  /* (Quickly) clear the screen. */
  cdclear();
  cdredo();
  cdrefresh();

  /* If something is wrong, don't do anything. */
  if ( snum < 1 || snum > MAXSHIPS )
    return;
  
  /* If our ships pid is wrong, we are indeed lost. */
  if ( Ships[snum].pid != CqContext.pid )
    return;
  
  kb = Ships[snum].killedby;
  
  /* Delay while our torps are exploding. */
  grand( &entertime );
  i = 0;
  while ( dgrand( entertime, &now ) < TORPEDOWAIT_GRAND )
    {
      i = 0;
      for ( j = 0; j < MAXTORPS; j = j + 1 )
	if ( Ships[snum].torps[j].status == TS_DETONATE )
	  i = i + 1;
      if ( i <= 0 )
	break;
      c_sleep( ITER_SECONDS );
    }
  
  /* There aren't supposed to be any torps left. */
  if ( i > 0 )
    {
      c_strcpy( "dead: ", cbuf );
      appship( snum, cbuf );
      appstr( "'s detonating torp count is %d.", cbuf );
      cerror( cbuf, i );
      clog(cbuf, i);
    }
  
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

  if ( ! ( leave && kb == KB_SELF ) && kb != KB_SHIT && kb != KB_EVICT )
    c_sleep( 4.0 );
  
  for ( i = 1; i <= 10 && Ships[snum].status == SS_DYING; i++ )
    c_sleep( 1.0 );
  Ships[snum].status = SS_RESERVED;
  Ships[snum].sdfuse = -TIMEOUT_PLAYER;
  Ships[snum].killedby = 0;
  
  switch ( kb )
    {
    case KB_CONQUER:
      do
	{
	  cdclear();
	  cdredo();
	  ConqInfo->lastwords[0] = EOS;
	  ch = cdgetx( "Any last words? ",
		       14, 1, TERMS, ConqInfo->lastwords, MAXLASTWORDS, TRUE );
	  cdclear();
	  cdredo();
	  if ( ConqInfo->lastwords[0] != EOS )
	    {
	      cprintf( 13,0,ALIGN_CENTER, "#%d#%s", 
			InfoColor, "You last words are entered as:");
	      cprintf( 14,0,ALIGN_CENTER, "#%d#%c%s%c", 
			YellowLevelColor, '"', ConqInfo->lastwords, '"' );
	    }
	  else
	    cprintf( 14,0,ALIGN_CENTER,"#%d#%s", InfoColor,
		   "You have chosen to NOT leave any last words:" );
	  ch = getcx( "Press TAB to confirm:", 16, 0,
		     TERMS, cbuf, 10 );
	}
      while ( ch != TERM_EXTRA ); /* until . while */
      break;
    case KB_SELF:
    case KB_EVICT:
    case KB_SHIT:
      /* Do nothing special. */
      break;
    default:
      ioeat();
      putpmt( MTXT_DONE, MSG_LIN2 );
      cdrefresh();
      while ( ! iogtimed( &ch, 1 ) && stillalive( CqContext.snum ) )
	;
      break;
    }
  cdmove( 1, 1 );
  
  /* Turn off sticky war so we can change war settings from menu(). */
  for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    Ships[snum].rwar[i] = FALSE;
  
  return;
  
}


/*  dispoption - display options */
/*  SYNOPSIS */
/*    int op(MAXOPTIONS) */
/*    dispoption( op ) */
void dispoption( int op[] )
{
  string l1="Toggle options, TAB when done: phaser (g)raphics=~ (p)lanet names=~";
  string l2="(a)larm bells=~ (i)ntruder alerts=~ (n)umeric map=~ (t)erse=~ (e)xplosions=~";
  
  cdclrl( MSG_LIN1, 2 );
  cdputs( l1, MSG_LIN1, 1 );
  cdputs( l2, MSG_LIN2, 1 );
  
  if ( op[OPT_PHASERGRAPHICS] )
    cdput( 'T', MSG_LIN1, 50 );
  else
    cdput( 'F', MSG_LIN1, 50 );
  
  if ( op[OPT_PLANETNAMES] )
    cdput( 'T', MSG_LIN1, 67 );
  else
    cdput( 'F', MSG_LIN1, 67 );
  
  if ( op[OPT_ALARMBELL] )
    cdput( 'T', MSG_LIN2, 15 );
  else
    cdput( 'F', MSG_LIN2, 15 );
  
  if ( op[OPT_INTRUDERALERT] )
    cdput( 'T', MSG_LIN2, 35 );
  else
    cdput( 'F', MSG_LIN2, 35 );
  
  if ( op[OPT_NUMERICMAP] )
    cdput( 'T', MSG_LIN2, 51 );
  else
    cdput( 'F', MSG_LIN2, 51 );
  
  if ( op[OPT_TERSE] )
    cdput( 'T', MSG_LIN2, 61 );
  else
    cdput( 'F', MSG_LIN2, 61 );
  
  if ( op[OPT_EXPLOSIONS] )
    cdput( 'T', MSG_LIN2, 76 );
  else
    cdput( 'F', MSG_LIN2, 76 );
  
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
  
  string pmt="New weapons allocation: (30-70) ";
  
  cdclrl( MSG_LIN1, 2 );
  cbuf[0] = EOS;
  ch = (char)cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE, TRUE );
  if ( ch == TERM_EXTRA )
    Ships[snum].weapalloc = Ships[snum].engalloc;
  else if ( ch == TERM_NORMAL )
    {
      i = 0;
      safectoi( &alloc, cbuf, i );			/* ignore status */
      if ( alloc != 0 )
	{
	  if ( alloc < 30 )
	    alloc = 30;
	  else if ( alloc > 70 )
	    alloc = 70;
	  Ships[snum].weapalloc = alloc;
	}
    }
  
  Ships[snum].engalloc = 100 - Ships[snum].weapalloc;
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*  doautopilot - handle the autopilot */
/*  SYNOPSIS */
/*    int snum */
/*    doautopilot( snum ) */
void doautopilot( int snum )
{
  int now, laststat; 
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
  
  c_putmsg( "Autopilot activated.", MSG_LIN1 );
  Ships[snum].robot = TRUE;
  gsecs( &laststat );			/* initialize stat timer */
  while ( stillalive( CqContext.snum ) )
    {
      /* Make sure we still control our ship. */
      if ( Ships[snum].pid != CqContext.pid )
	break;
      
      /* See if it's time to update the statistics. */
      if ( dsecs( laststat, &now ) >= 15 )
	{
	  conqstats( CqContext.snum );
	  laststat = now;
	}
      
      /* Get a character. */
      if ( ! iogtimed( &ch, 1 ) )
	continue;		/* next . echo */
      CqContext.msgok = FALSE;
      grand( &CqContext.msgrand );
      switch ( ch )
	{
	case TERM_ABORT:
	  break;
	case '\014':	/* ^L */
	  cdredo();
	  break;
	default:
	  c_putmsg( "Press ESCAPE to abort autopilot.", MSG_LIN1 );
	  cdbeep();
	  cdrefresh();
	}
      CqContext.msgok = TRUE;
      if (ch == TERM_ABORT)
	break;
    }
  Ships[snum].robot = FALSE;
  Ships[snum].action = 0;
  
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*  dobeam - beam armies up or down (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobeam( snum ) */
void dobeam( int snum )
{
  int pnum, total, num, upmax, downmax, capacity, beamax, i;
  int ototal, entertime, now;
  int oldsshup, dirup, zeroed, conqed;
  int ch; 
  char buf[MSGMAXLINE];
  real rkills;
  int done = FALSE;
  string lastfew="Fleet orders prohibit removing the last three armies.";
  string abt="...aborted...";
  
  Ships[snum].rmode = FALSE;
  
  cdclrl( MSG_LIN1, 2 );
  
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
	  c_putmsg( "Phoon!  Our armies will suffocate down there!",
		   MSG_LIN1 );
	  return;
	}
      else if ( Planets[pnum].team == TEAM_GOD )
	{
	  c_putmsg(
		   "GOD.you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
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
  
  if ( Planets[pnum].team != Ships[snum].team &&
      Planets[pnum].team != TEAM_SELFRULED &&
      Planets[pnum].team != TEAM_NOTEAM )
    if ( ! Ships[snum].war[Planets[pnum].team] && Planets[pnum].armies != 0) /* can take empty planets */
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
#ifdef DEBUG_MISC
  clog("dobeam(): rkills=%f Ships[%d].kills=%f",
       rkills, snum, Ships[snum].kills);
#endif
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
      capacity = min( ifix( rkills ) * 2, Teams[Ships[snum].team].armylim );
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
      while ( stillalive( CqContext.snum ) && done == FALSE)
	{
	  if ( ! iogtimed( &ch, 1 ) )
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
  /* Now we are ready! */
  if ( Planets[pnum].team >= NUMPLAYERTEAMS )
    {
      /* If the planet is not race owned, make it war with us. */
      Ships[snum].srpwar[pnum] = TRUE;
    }
  else if ( Planets[pnum].team != Ships[snum].team )
    {
      /* For a team planet make the war sticky and send an intruder alert. */
      Ships[snum].rwar[Planets[pnum].team] = TRUE;
      
      /* Chance to create a robot here. */
      intrude( snum, pnum );
    }
  
  /* Lower shields. */
  oldsshup = Ships[snum].shup;
  Ships[snum].shup = FALSE;
  
  /* Beam. */
  total = 0;
  ototal = -1;				/* force an update the first time */
  zeroed = FALSE;
  conqed = FALSE;
  
  grand( &entertime );
  while(TRUE)			/* repeat infloop */
    {
      if ( ! stillalive( CqContext.snum ) )
	return;
      if ( iochav() )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  break;
	}
      
      /* See if it's time to beam again. */
      while ( dgrand( entertime, &now ) >= BEAM_GRAND )
	{
	  /*	      entertime = mod( entertime + BEAM_GRAND, 24*60*60*1000 );*/
	  grand(&entertime);
	  PVLOCK(&ConqInfo->lockword);
	  if ( dirup )
	    {
	      /* Beam up. */
	      if ( Planets[pnum].armies <= MIN_BEAM_ARMIES )
		{
		  PVUNLOCK(&ConqInfo->lockword);
		  c_putmsg( lastfew, MSG_LIN1 );
		  break;
		}
	      Ships[snum].armies = Ships[snum].armies + 1;
	      Planets[pnum].armies = Planets[pnum].armies - 1;
	    }
	  else
	    {
	      /* Beam down. */
	      Ships[snum].armies = Ships[snum].armies - 1;
	      if ( Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
		{
		  takeplanet( pnum, snum );
		  conqed = TRUE;
		}
	      else if ( Planets[pnum].team != Ships[snum].team )
		{
		  Planets[pnum].armies = Planets[pnum].armies - 1;
		  if ( Planets[pnum].armies == 0 )
		    {
		      zeroplanet( pnum, snum );
		      zeroed = TRUE;
		    }
		}
	      else
		Planets[pnum].armies = Planets[pnum].armies + 1;
	    }
	  PVUNLOCK(&ConqInfo->lockword);
	  total = total + 1;
	  
	  if ( total >= num )
	    {
	      /* Done. */
	      cdclrl( MSG_LIN1, 1 );
	      goto cbrk21; /* break 2;*/
	    }
	}
      
      if ( ototal != total )
	{
	  c_strcpy( "Beaming ", cbuf );
	  if ( dirup )
	    appstr( "up from ", cbuf );
	  else
	    appstr( "down to ", cbuf );
	  appstr( Planets[pnum].name, cbuf );
	  appstr( ", ", cbuf );
	  if ( total == 0 )
	    appstr( "no", cbuf );
	  else
	    appint( total, cbuf );
	  appstr( " arm", cbuf );
	  if ( total == 1 )
	    {
	      appchr( 'y', cbuf );
	    }
	  else
	    {
	      appstr( "ies", cbuf );
	    }
	  appstr( " transported, ", cbuf );
	  appint( num - total, cbuf );
	  appstr( " to go.", cbuf );
	  c_putmsg( cbuf, MSG_LIN1 );
	  if ( ototal == -1 )
	    cdrefresh();		/* display the first time */
	  ototal = total;
	}
      
      if ( dirup && Planets[pnum].armies <= MIN_BEAM_ARMIES )
	{
	  c_putmsg( lastfew, MSG_LIN1 );
	  break;
	}
      
      aston();
      c_sleep( ITER_SECONDS );
      astoff();
    }
 cbrk21:
  
  /* Restore shields. */
  Ships[snum].shup = oldsshup;
  
  /* Try to display the last bombing message. */
  cdrefresh();
  
  if ( conqed )
    {
      sprintf( cbuf, "You have conquered %s.", Planets[pnum].name );
      c_putmsg( cbuf, MSG_LIN1 );
    }
  else if ( zeroed )
    c_putmsg( "Sensors show hostile forces eliminated from the planet.",
	     MSG_LIN1 );
  
  return;
  
}


/*  dobomb - bombard a planet (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobomb( snum ) */
void dobomb( int snum )
{
  int pnum, now, entertime, total, ototal, oparmies;
  real x, killprob;
  int oldsshup; 
  char  buf[MSGMAXLINE];
  
  string lastfew="The last few armies are eluding us.";
  string abt="...aborted...";
  
  Ships[snum].rmode = FALSE;
  
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
  
  /* Handle war logic. */
  Ships[snum].srpwar[pnum] = TRUE;
  if ( Planets[pnum].team >= 0 && Planets[pnum].team < NUMPLAYERTEAMS )
    {
      /* For a team planet make the war sticky and send an intruder alert. */
      Ships[snum].rwar[Planets[pnum].team] = TRUE;
      intrude( snum, pnum );
    }
  /* Planets owned by GOD have a special defense system. */
  if ( Planets[pnum].team == TEAM_GOD )
    {
      sprintf( cbuf, "That was a bad idea, %s...", Ships[snum].alias );
      c_putmsg( cbuf, MSG_LIN1 );
      damage( snum,  rnduni( 50.0, 100.0 ), KB_LIGHTNING );
      return;
    }
  
  /* Lower shields. */
  oldsshup = Ships[snum].shup;
  Ships[snum].shup = FALSE;
  
  /* Bombard. */
  total = 0;
  ototal = -1					/* force an update the first time */;
  oparmies = -1;
  grand( &entertime )			/* get start time */;
  while(TRUE)       /*repeat infloop */
    {
      if ( ! stillalive( CqContext.snum ) )
	return;
      if ( iochav() )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  break;
	}
      
      cdrefresh();
      
      /* See if it's time to bomb yet. */
      while ((int) fabs ((real)dgrand( (int)entertime, (int *)&now )) >= BOMBARD_GRAND )
	{
	  if ( Ships[snum].wfuse > 0 )
	    {
	      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
	      goto cbrk22; /* break 2;*/
	    }
	  x = BOMBARD_FUEL * (real)(BOMBARD_GRAND / 1000.0);
	  if ( ! usefuel( snum, x, TRUE ) )
	    {
	      c_putmsg( "Not enough fuel to bombard.", MSG_LIN1 );
	      goto cbrk22; /* break 2;*/
	    }
	  /*  entertime = mod( entertime + BOMBARD_GRAND, 24*60*60*1000 );*/
	  grand(&entertime);
	  killprob = (real)((BOMBARD_PROB *
			     ((real) weaeff( snum ) *
			      (real)((real)Planets[pnum].armies/100.0))) + 0.5 );
	  /*	    cerror(MSG_GOD, "DEBUG: killprob = %d\n", (int) (killprob *10));*/
	  if ( rnd() < killprob )
	    {
	      /*	    cerror(MSG_GOD, "DEBUG: we're in: killprob = %d\n", (int)(killprob * 10));*/
	      PVLOCK(&ConqInfo->lockword);
	      if ( Planets[pnum].armies <= MIN_BOMB_ARMIES )
		{
		  /* No more armies left to bomb. */
		  PVUNLOCK(&ConqInfo->lockword);
		  c_putmsg( lastfew, MSG_LIN1 );
		  goto cbrk22; /* break 2;*/
		}
	      Planets[pnum].armies = Planets[pnum].armies - 1;
	      
	      Ships[snum].kills = Ships[snum].kills + BOMBARD_KILLS;
	      Users[Ships[snum].unum].stats[USTAT_ARMBOMB] += 1;
	      Teams[Ships[snum].team].stats[TSTAT_ARMBOMB] += 1;
	      PVUNLOCK(&ConqInfo->lockword);
	      total = total + 1;
	    }
	}
      
      if ( Planets[pnum].armies <= MIN_BOMB_ARMIES )
	{
	  /* No more armies left to bomb. */
	  c_putmsg( lastfew, MSG_LIN1 );
	  break;
	}
      
      if ( Planets[pnum].armies != oparmies || ototal != total )
	{
	  /* Either our bomb run total or the population changed. */
	  oparmies = Planets[pnum].armies;
	  if ( total == 1 )
	    c_strcpy( "y", buf );
	  else
	    c_strcpy( "ies", buf );
	  sprintf( cbuf, "Bombing %s, %d arm%s killed, %d left.",
		 Planets[pnum].name, total, buf, oparmies );
	  c_putmsg( cbuf, MSG_LIN1 );
	  cdrefresh();
	  if ( ototal == -1 )
	    {
	      cdrefresh();		/* display the first time */
	    }
	  
	  ototal = total;
	}
      
      aston();
      c_sleep( ITER_SECONDS );
      astoff();
    }
 cbrk22:
  ;
  
  /* Restore shields. */
  Ships[snum].shup = oldsshup;
  
  /* Try to display the last bombing message. */
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
  
  if ( Ships[snum].cloaked )
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
      if ( ! launch( snum, dir, 3, LAUNCH_NORMAL ) )
	c_putmsg( ">TUBES EMPTY<", MSG_LIN2 );
      else
	cdclrl( MSG_LIN1, 1 );
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
  
  if ( Ships[snum].cloaked )
    {
      Ships[snum].cloaked = FALSE;
      c_putmsg( "Cloaking device disengaged.", MSG_LIN1 );
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
      if ( cloak( snum ) )
	c_putmsg( "Cloaking device engaged.", MSG_LIN2 );
      else
	c_putmsg( nofuel, MSG_LIN2 );
    }
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*  docoup - attempt to rise from the ashes (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    docoup( snum ) */
void docoup( int snum )
{
  int i, pnum, now, entertime;
  real failprob;
  string nhp="We must be orbiting our home planet to attempt a coup.";
  string conf="Press TAB to try it: ";
  
  cdclrl( MSG_LIN2, 1 );
  
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
  
  /* Now our team can tell coup time for free. */
  Teams[Ships[snum].team].coupinfo = TRUE;
  
  i = Teams[Ships[snum].team].couptime;
  if ( i > 0 )
    {
      sprintf( cbuf, "Our forces need %d more minutes to organize.", i );
      c_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  
  /* Confirm. */
  cdclrl( MSG_LIN1, 1 );
  cbuf[0] = EOS;
  if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
	       TRUE) != TERM_EXTRA )
    {
      c_putmsg( "...aborted...", MSG_LIN1 );
      return;
    }
  
  /* Now wait it out... */
  c_putmsg( "Attempting coup...", MSG_LIN1 );
  cdrefresh();
  grand( &entertime );
  while ( dgrand( entertime, &now ) < COUP_GRAND )
    {
      /* See if we're still alive. */
      if ( ! stillalive( CqContext.snum ) )
	return;
      
      /* Sleep (and enable asts so the display will work). */
      aston();
      c_sleep( ITER_SECONDS );
      astoff();
    }
  
  cdclrl( MSG_LIN1, 1 );
  PVLOCK(&ConqInfo->lockword);
  if ( Planets[pnum].team == Ships[snum].team )
    {
      PVUNLOCK(&ConqInfo->lockword);
      c_putmsg( "Sensors show hostile forces eliminated from the planet.",
	       MSG_LIN2 );
      return;
    }
  
  failprob = Planets[pnum].armies / MAX_COUP_ENEMY_ARMIES * 0.5 + 0.5;
  if ( rnd() < failprob )
    {
      /* Failed; setup new reorganization time. */
      Teams[Ships[snum].team].couptime = rndint( 5, 10 );
      PVUNLOCK(&ConqInfo->lockword);
      c_putmsg( "Coup unsuccessful.", MSG_LIN2 );
      return;
    }
  
  takeplanet( pnum, snum );
  Planets[pnum].armies = rndint( 10, 20 );		/* create token coup force */
  Users[Ships[snum].unum].stats[USTAT_COUPS] += 1;
  Teams[Ships[snum].team].stats[TSTAT_COUPS] += 1;
  PVUNLOCK(&ConqInfo->lockword);
  c_putmsg( "Coup successful!", MSG_LIN2 );
  
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
      if ( Ships[sorpnum].cloaked )
	{
	  if ( Ships[sorpnum].warp <= 0.0 )
	    {
	      c_putmsg( "Sensors are unable to lock on.", MSG_LIN2 );
	      return;
	    }
	  appx = rndnor(Ships[sorpnum].x, CLOAK_SMEAR_DIST);
	  appy = rndnor(Ships[sorpnum].y, CLOAK_SMEAR_DIST);
	}
      else
	{
	  appx = Ships[sorpnum].x;
	  appy = Ships[sorpnum].y;
	}
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
  
  if ( Ships[snum].warp < 0.0 )		/* if orbitting */
    Ships[snum].warp = 0.0;		/* break orbit */
  Ships[snum].dhead = dir;			/* set direction first to avoid */
  Ships[snum].lock = newlock;		/*  a race in display() */
  
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
  else if ( enemydet( snum ) )
    c_putmsg( "detonating...", MSG_LIN1 );
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
  int i;
  string pmt="Press TAB to send an emergency distress call: ";
  
  cdclrl( MSG_LIN1, 2 );

  cbuf[0] = EOS;
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE,
	       TRUE) == TERM_EXTRA )
    {
      sprintf( cbuf,
	     "sh=%d, dam=%d, fuel=%d, temp=",
	     round(Ships[snum].shields),
	     round(Ships[snum].damage),
	     round(Ships[snum].fuel) );
      i = round(Ships[snum].wtemp);
      if ( i < 100 )
	appint( i, cbuf );
      else
	appstr( "**", cbuf );
      appchr( '/', cbuf );
      i = round(Ships[snum].etemp);
      if ( i < 100 )
	appint( i, cbuf );
      else
	appstr( "**", cbuf );
      i = Ships[snum].armies;
      if ( i > 0 )
	{
	  appstr( ", armies=", cbuf );
	  appint( i, cbuf );
	}
      if ( Ships[snum].wfuse > 0 )
	appstr( ", -weapons", cbuf );
      if ( Ships[snum].efuse > 0 )
	appstr( ", -engines", cbuf );
      
      stormsg( snum, -Ships[snum].team, cbuf );
    }
  
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
  cprintf(tlin,col,ALIGN_NONE,sfmt, "I", "set user options");
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
  while ( ! iogtimed( &ch, 1 ) && stillalive( CqContext.snum ) )
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
      c_strcpy( CqContext.lastinfostr, cbuf );
      if ( cbuf[0] == EOS )
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
    }
  else
    c_strcpy( cbuf, CqContext.lastinfostr );
  
  if ( special( cbuf, &what, &token, &count ) )
    {
      if ( ! findspecial( snum, token, count, &sorpnum, &xsorpnum ) )
	what = NEAR_NONE;
      else if ( extra )
	if ( xsorpnum == 0 )
	  what = NEAR_NONE;
	else
	  sorpnum = xsorpnum;
      
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
  
  if ( Ships[snum].cloaked )
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
    cdclrl( MSG_LIN2, 1 );
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
  int j;
  
  cdclrl( MSG_LIN2, 1 );
  
  c_putmsg( "Detonating...", MSG_LIN1 );
  
  for ( j = 0; j < MAXTORPS; j = j + 1 )
    detonate( snum, j );
  
  return;
  
}


/*  dooption - set user options */
/*  SYNOPSIS */
/*    int snum */
/*    int dodisplay */
/*    dooption( snum, dodisplay ) */
void dooption( int snum, int dodisplay )
{
  int i, tok;
  int ch;
  static int leave = FALSE;
  int top[MAXOPTIONS], sop[MAXOPTIONS]; 
  
  leave = FALSE;
  /* Make some copies of the current ship options. */
  for ( i = 0; i < MAXOPTIONS; i = i + 1)
    {
      sop[i] = Ships[snum].options[i];			/* used in case we abort */
      top[i] = Ships[snum].options[i];			/* used for dispoption() */
    }
  
  while ( stillalive( CqContext.snum ) && leave == FALSE)
    {
      /* Display the current options. */
      dispoption( top );
      cdrefresh();
      
      /* Get a character. */
      if ( ! iogtimed( &ch, 1 ) )
	continue; /* next; */
      switch ( ch )
	{
	case TERM_EXTRA:
	  /* Done fooling around, update the user options. */
	  for ( i = 0; i < MAXOPTIONS; i = i + 1)
	    Users[Ships[snum].unum].options[i] = top[i];
	  leave = TRUE;
	  break;
	case TERM_ABORT:
	  /* Decided to abort; restore things. */
	  for ( i = 0; i < MAXOPTIONS; i = i + 1)
	    Ships[snum].options[i] = sop[i];
	  if ( dodisplay )
	    {
	      /* Force an update. */
	      stoptimer();
	      display( snum, FALSE );		/* update the display */
	      settimer();
	    }
	  
	  leave = TRUE;
	  break;
	default:
	  if ( getoption( ch, &tok ) )
	    {
	      /* Update temporary. */
	      top[tok] = ! top[tok];
	      
	      /* Copy temporary into ship for display() to use. */
	      Ships[snum].options[tok] = top[tok];
	      
	      if ( dodisplay )
		{
		  /* Force an update. */
		  stoptimer();
		  display( snum, FALSE );
		  settimer();
		}
	    }
	  else
	    cdbeep();
	}
    }
  
  cdclrl( MSG_LIN1, 2 );
  
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
      orbit( snum, pnum );
      infoplanet( "Coming into orbit around ", -Ships[snum].lock, snum );
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
  if ( Ships[snum].cloaked )
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
      if ( phaser( snum, dir ) )
	c_putmsg( "Firing phasers...", MSG_LIN2 );
      else
	c_putmsg( ">PHASERS DRAINED<", MSG_LIN2 );
    }
  else
    {
      c_putmsg( "Invalid targeting information.", MSG_LIN1 );
    }

/*  cdclrl( MSG_LIN1, 1 );*/
  
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
    planlist( Users[CqContext.unum].team, snum );
  
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
      
      CqContext.msgok = FALSE;		/* don't want to get msgs when reading
				   old ones.  */
    }

  lstmsg = Ships[snum].lastmsg;	/* don't want lstmsg changing while reading old ones. */

  if ( ! review( snum, lstmsg ) )
    {
      c_putmsg( "There are no old messages.", MSG_LIN1 );
      putpmt( MTXT_MORE, MSG_LIN2 );
      cdrefresh();
      while ( ! iogtimed( &ch, 1 ) && stillalive( CqContext.snum ) )
	;
      cdclrl( MSG_LIN1, 2 );
    }

  if (RMsg_Line == MSG_LIN1)
    {
      CqContext.msgok = TRUE;		
    }

  return;
  
}


/*  doselfdest - execute a self-destruct command */
/*  SYNOPSIS */
/*    doselfdest */
void doselfdest(int snum)
{
  
  int entertime, now; 
  string pmt="Press TAB to initiate self-destruct sequence: ";
  
  cdclrl( MSG_LIN1, 2 );

  if ( Ships[snum].cloaked )
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
  
  /* See if we want to exit to dcl. */
  /*    leave = ( 'q' == cbuf[0] || 'Q' == cbuf[0] );*/
  cdclrl( MSG_LIN1, 1 );
  
  /* Set up the destruct fuse. */
  Ships[CqContext.snum].sdfuse = SELFDESTRUCT_FUSE;
  
  gsecs( &entertime );
  
  /* Force a screen update. */
  stoptimer();
  display( CqContext.snum, FALSE );
  settimer();
  CqContext.msgok = TRUE;			/* messages are ok in the beginning */
  while ( Ships[CqContext.snum].sdfuse > 0 )
    {
      Ships[CqContext.snum].sdfuse = SELFDESTRUCT_FUSE - dsecs ( entertime, &now );

      /* Display new messages until T-minus 3 seconds. */

      if ( Ships[CqContext.snum].sdfuse < 3 )
	CqContext.msgok = FALSE;

      if ( ! stillalive( CqContext.snum ) )
	{
	  /* Died in the process. */
	  Ships[CqContext.snum].sdfuse = 0;
	  return;
	}
      
      if ( iochav() )
	{
	  /* Got a new character. */
	  grand( &CqContext.msgrand );
	  cdclrl( MSG_LIN1, 2 );
	  if ( iogchar() == TERM_ABORT )
	    {
	      Ships[CqContext.snum].sdfuse = 0;
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
      aston();			/* enable asts so the display will work */
      c_sleep( ITER_SECONDS );
      astoff();
    } /* end while */
  CqContext.msgok = FALSE;			/* turn off messages */
  
  if ( Doomsday->status == DS_LIVE )
    {
      if ( dist(Ships[CqContext.snum].x, Ships[CqContext.snum].y, Doomsday->x, Doomsday->y) <= DOOMSDAY_KILL_DIST )
	{
	  Doomsday->status = DS_OFF;
	  stormsg( MSG_DOOM, MSG_ALL, "AIEEEEEEEE!" );
	  killship( CqContext.snum, KB_GOTDOOMSDAY );
	}
      else
	if (stillalive(CqContext.snum))	/* if we're not dead yet... */
	  killship( CqContext.snum, KB_SELF );
    }
  else
    {
	if (stillalive(CqContext.snum))	/* if we're not dead yet... */
	  killship( CqContext.snum, KB_SELF );
    }
  
  return;
  
}


/*  doshields - raise or lower shields */
/*  SYNOPSIS */
/*    int snum */
/*    int up */
/*    doshields( snum, up ) */
void doshields( int snum, int up )
{
  
  Ships[snum].shup = up;
  if ( up )
    {
      Ships[snum].rmode = FALSE;
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
  while ( stillalive( CqContext.snum ) )
    {
      teamlist( team );
      putpmt( MTXT_DONE, MSG_LIN2 );
      cdrefresh();
      if ( iogtimed( &ch, 1 ) )
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
  
  if ( Ships[snum].cloaked )
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
      if ( ! launch( snum, dir, 1, LAUNCH_NORMAL ) )
	c_putmsg( ">TUBES EMPTY<", MSG_LIN2 );
      else
	cdclrl( MSG_LIN1, 1 );
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
      return;
    }
  if ( Ships[snum].towing != 0 )
    {
      c_strcpy( "But we're already towing ", cbuf );
      appship( Ships[snum].towing, cbuf );
      appchr( '.', cbuf );
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
  cbuf[0] = EOS;
  
  PVLOCK(&ConqInfo->lockword);
  if ( other < 1 || other > MAXSHIPS )
    c_strcpy( "No such ship.", cbuf );
  else if ( Ships[other].status != SS_LIVE )
    c_strcpy( "Not found.", cbuf );
  else if ( other == snum )
    c_strcpy( "We can't tow ourselves!", cbuf );
  else if ( dist( Ships[snum].x, Ships[snum].y, Ships[other].x, Ships[other].y ) > TRACTOR_DIST )
    c_strcpy( "That ship is out of tractor range.", cbuf );
  else if ( Ships[other].warp < 0.0 )
    c_strcpy( "You can't tow a ship out of orbit.", cbuf );
  else if ( sqrt( pow(( (real) (Ships[snum].dx - Ships[other].dx) ), (real) 2.0) +
		  pow( (real) ( Ships[snum].dy - Ships[other].dy ), (real) 2.0 ) ) / 
	    ( MM_PER_SEC_PER_WARP * ITER_SECONDS ) > MAX_TRACTOR_WARP ) 
    sprintf( cbuf, "That ships relative velocity is higher than %2.1f.",
	     MAX_TRACTOR_WARP );
  else if ( Ships[other].towing != 0 || Ships[other].towedby != 0 )
    c_strcpy(
	     "There seems to be some interference with the tractor beams...",
	     cbuf );
  else
    {
      Ships[other].towedby = snum;
      Ships[snum].towing = other;
      c_strcpy( "Tractor beams engaged.", cbuf );
    }
  PVUNLOCK(&ConqInfo->lockword);
  c_putmsg( cbuf, MSG_LIN2 );
  
  return;
  
}


/*  dountow - release a tow (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dountow( snum ) */
void dountow( int snum )
{
  int entertime, now;
  int warsome; 
  
  cdclrl( MSG_LIN1, 2 );
  if ( Ships[snum].towedby != 0 )
    {
      /* If we're at war with him or he's at war with us, make it */
      /*  hard to break free. */
      warsome = ( satwar( snum, Ships[snum].towedby) );
      if ( warsome )
	{
	  grand( &entertime );
	  while ( dgrand( entertime, &now ) < BREAKAWAY_GRAND )
	    {
	      if ( ! stillalive( CqContext.snum ) )
		return;
	      aston();
	      c_sleep( ITER_SECONDS );
	      astoff();
	    }
	}
      if ( warsome && ( rnd() > BREAKAWAY_PROB ) )
	c_putmsg( "Attempt to break free failed.", MSG_LIN1 );
      else
	{
	  c_strcpy( "Breaking free from ship ", cbuf );
	  appship( Ships[snum].towedby, cbuf );
	  PVLOCK(&ConqInfo->lockword);
	  if ( Ships[snum].towedby != 0 )
	    {
	      /* Coast to a stop. */
	      Ships[snum].head = Ships[Ships[snum].towedby].head;
	      Ships[snum].warp = Ships[Ships[snum].towedby].warp;
	      
	      /* Release the tow. */
	      if ( Ships[Ships[snum].towedby].towing != 0 )
		Ships[Ships[snum].towedby].towing = 0;
	      Ships[snum].towedby = 0;
	    }
	  PVUNLOCK(&ConqInfo->lockword);
	  appchr( '.', cbuf );
	  c_putmsg( cbuf, MSG_LIN1 );
	}
    }
  else if ( Ships[snum].towing != 0 )
    {
      c_strcpy( "Tow released from ship ", cbuf );
      appship( Ships[snum].towing, cbuf );
      PVLOCK(&ConqInfo->lockword);
      if ( Ships[snum].towing != 0 )
	{
	  /* Set other ship coasting. */
	  Ships[Ships[snum].towing].head = Ships[snum].head;
				/* only set warp if valid JET - 9/15/97 */
	  if (Ships[snum].warp >= 0.0)
	    Ships[Ships[snum].towing].warp = Ships[snum].warp;
	  
	  /* Release the tow. */
	  if ( Ships[Ships[snum].towing].towedby != 0 )
	    Ships[Ships[snum].towing].towedby = 0;
	  Ships[snum].towing = 0;
	}
      PVUNLOCK(&ConqInfo->lockword);
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
    }
  else
    c_putmsg( "No tractor beam activity detected.", MSG_LIN1 );
  
  return;
  
}


/*  dowar - declare war or peace */
/*  SYNOPSIS */
/*    int snum */
/*    dowar( snum ) */
void dowar( int snum )
{
  int i, entertime, now; 
  int twar[NUMPLAYERTEAMS], dowait;
  int ch;
  const int POffset = 47, WOffset = 61;
  
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    twar[i] = Ships[snum].war[i];
  
  cdclrl( MSG_LIN1, 2 );
  
  cdputs(
	 "Press TAB when done, ESCAPE to abort:  Peace: # # # #  War: # # # #", 
	 MSG_LIN1, 1 );
  
  while ( stillalive( CqContext.snum ) )
    {
      for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	if ( twar[i] )
	  {
	    cdput( ' ', MSG_LIN1, POffset + (i*2) );
	    if ( Ships[snum].rwar[i] )
	      ch = Teams[i].teamchar;
	    else
	      ch = (char)tolower(Teams[i].teamchar);
	    cdput( ch, MSG_LIN1, WOffset + (i*2) );
	  }
	else
	  {
	    cdput( (char)tolower(Teams[i].teamchar), MSG_LIN1, POffset + (i*2) );
	    cdput( ' ', MSG_LIN1, WOffset+(i*2) );
	  }
      cdrefresh();
      if ( iogtimed( &ch, 1 ) == FALSE )
	{
	  continue; /* next; */
	}
      
      ch = (char)tolower( ch );
      if ( ch == TERM_ABORT )
	break;
      if ( ch == TERM_EXTRA )
	{
	  /* Now update the war settings. */
	  dowait = FALSE;
	  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	    {
	      if ( twar[i] && ! Ships[snum].war[i] )
		dowait = TRUE;
	      Users[Ships[snum].unum].war[i] = twar[i];
	      Ships[snum].war[i] = twar[i];
	    }
	  
	  /* Only check for computer delay when flying. */
	  if ( Ships[snum].status != SS_RESERVED && dowait )
	    {
	      /* We've set war with at least one team, stall a little. */
	      c_putmsg(
		       "Reprogramming the battle computer, please stand by...",
		       MSG_LIN2 );
	      cdrefresh();
	      grand( &entertime );
	      while ( dgrand( entertime, &now ) < REARM_GRAND )
		{
		  /* See if we're still alive. */
		  if ( ! stillalive( CqContext.snum ) )
		    return;
		  
		  /* Sleep (and enable asts so the display will work). */
		  aston();
 		  c_sleep( ITER_SECONDS );
		  astoff();
		}
	    }
	  break;
	}
      
      for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	if ( ch == (char)tolower( Teams[i].teamchar ) )
	  {
	    if ( ! twar[i] || ! Ships[snum].rwar[i] )
	      {
		twar[i] = ! twar[i];
		goto ccont1;	/* next 2  */
	      }
	    break;
	  }
      cdbeep();
      
    ccont1:				/* goto  */
      ;
    }
  
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*  dowarp - set warp factor */
/*  SYNOPSIS */
/*    int snum */
/*    real warp */
/*    dowarp( snum, warp ) */
void dowarp( int snum, real warp )
{
  real mw;
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( Ships[snum].dwarp == 0.0 && warp != 0.0 )
    {
      /* See if engines are working. */
      if ( Ships[snum].efuse > 0 )
	{
	  c_putmsg( "Engines are currently overloaded.", MSG_LIN1 );
	  return;
	}
      
      /* No charge if already warp 0. */
      if ( usefuel( snum, ENGINES_ON_FUEL, FALSE ) == FALSE)
	{
	  c_putmsg( "We don't have enough fuel.", MSG_LIN1 );
	  return;
	}
      
      /* Don't stop repairing if changing to warp 0. */
      Ships[snum].rmode = FALSE;
    }
  
  /* If orbitting, break orbit. */
  if ( Ships[snum].warp < 0.0 )
    {
      Ships[snum].warp = 0.0;
      Ships[snum].lock = 0;
      Ships[snum].dhead = Ships[snum].head;
    }
  
  /* Handle ship limitations. */
  Ships[snum].dwarp = min( warp, Teams[Ships[snum].team].warplim );
  
  sprintf( cbuf, "Warp %d.", (int) Ships[snum].dwarp );
  c_putmsg( cbuf, MSG_LIN1 );
  
  /* Warn about damage limitations. */
  mw = maxwarp( snum );
  if ( around( Ships[snum].dwarp ) > mw )
    {
      sprintf( cbuf, "(Due to damage, warp is currently limited to %.1f.)",
	     mw );
      c_putmsg( cbuf, MSG_LIN2 );
    }
  
  return;
  
}


/*  getoption - decode char into option */
/*  SYNOPSIS */
/*    int flag, getoption */
/*    char ch */
/*    int tok */
/*    flag = getoption( ch, tok ) */
int getoption( char ch, int *tok )
{
  switch ( ch )
    {
    case 'g':
      *tok = OPT_PHASERGRAPHICS;
      break;
    case 'p':
      *tok = OPT_PLANETNAMES;
      break;
    case 'a':
      *tok = OPT_ALARMBELL;
      break;
    case 'i':
      *tok = OPT_INTRUDERALERT;
      break;
    case 'n':
      *tok = OPT_NUMERICMAP;
      break;
    case 't':
      *tok = OPT_TERSE;
      break;
    case 'e':
      *tok = OPT_EXPLOSIONS;
      break;
    default:
      *tok = 0;
      return ( FALSE );
    }
  return ( TRUE );
  
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
  
  col = (int)(CqContext.maxcol - strlen(g5)) / (int)2;
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
  char *if1="Suddenly  a  sinister,  wraithlike  figure appears before you";
  char *if2="seeming to float in the air.  In a low,  sorrowful  voice  he";
  char *if3="says, \"Alas, the very nature of the universe has changed, and";
  char *if4="your ship cannot be found.  All must now pass away.\"  Raising";
  char *if5="his  oaken  staff  in  farewell,  he fades into the spreading";
  char *if6="darkness.  In his place appears a  tastefully  lettered  sign";
  char *if7="reading:";
  char *if8="INITIALIZATION FAILURE";
  char *if9="The darkness becomes all encompassing, and your vision fails.";
  
  EnableConquestSignalHandler();	/* enable trapping of interesting signals */
  
  /* Initialize statistics. */
  initstats( &Ships[CqContext.snum].ctime, &Ships[CqContext.snum].etime );
  
  /* Log this entry into the Game. */
  CqContext.histslot = loghist( CqContext.unum );
  
  /* Set up a few ship characteristics here rather than in initship(). */
  Ships[CqContext.snum].unum = CqContext.unum;
  Ships[CqContext.snum].team = Users[CqContext.unum].team;

  Ships[CqContext.snum].pid = CqContext.pid;
  for ( i = 0; i < MAXOPTIONS; i = i + 1 )
    Ships[CqContext.snum].options[i] = Users[CqContext.unum].options[i];
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      Ships[CqContext.snum].rwar[i] = FALSE;
      Ships[CqContext.snum].war[i] = Users[CqContext.unum].war[i];
    }
  stcpn( Users[CqContext.unum].alias, Ships[CqContext.snum].alias, MAXUSERPNAME );
  
  /* Set up some things for the menu display. */
  switchteams = Users[CqContext.unum].ooptions[OOPT_SWITCHTEAMS];
  multiple = Users[CqContext.unum].ooptions[OOPT_MULTIPLE];
  oclosed = ConqInfo->closed;
  CqContext.leave = FALSE;
  redraw = TRUE;
  sleepy = 0;
  countdown = 0;
  playrv = FALSE;
  
  do                 
    {
      /* Make sure things are proper. */
      if (playrv == ERR) 
	{
	  if ( CqContext.snum < 1 || CqContext.snum > MAXSHIPS )
	    lose = TRUE;
	  else if ( Ships[CqContext.snum].pid != CqContext.pid )
	    lose = TRUE;
	  else if ( Ships[CqContext.snum].status != SS_RESERVED )
	    {
	      clog( "menu(): Ship %d no longer reserved.", CqContext.snum );
	      lose = TRUE;
	    }
	  else
	    lose = FALSE;
	}
      else
	lose = FALSE;

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
      if ( multiple != Users[CqContext.unum].ooptions[OOPT_MULTIPLE] )
	{
	  multiple = ! multiple;
	  redraw = TRUE;
	}
      
      if ( switchteams != Users[CqContext.unum].ooptions[OOPT_SWITCHTEAMS])
	{
	  switchteams = Users[CqContext.unum].ooptions[OOPT_SWITCHTEAMS];
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
      userline( CqContext.unum, 0, cbuf, FALSE, TRUE );
      attrset(A_BOLD);
      cdputs( cbuf, MSG_LIN2, 1 );
      attrset(0);
      
      cdmove( 1, 1 );
      cdrefresh();
      
      /* Try to kill the driver if we started one the last time */
      /*  we played and we've been in the menu long enough. */
      if ( countdown > 0 )
	{
	  countdown = countdown - 1;
	  if ( countdown <= 0 )
	    drkill();
	}
      
      /* Reset up the destruct fuse. */
      Ships[CqContext.snum].sdfuse = -TIMEOUT_PLAYER;
      
      /* Get a char with timeout. */
      if ( ! iogtimed( &ch, 1 ) )
	{
	  /* We get here if a char hasn't been typed. */
	  sleepy = sleepy + 1;
	  if ( sleepy > 300 )
	    break;
	  continue; /* next */
	}
      
      /* Got a character, zero timeout. */
      sleepy = 0;
      switch ( ch )
	{
	case 'e':
	  playrv = play();
	  if ( CqContext.childpid != 0 )
	    countdown = 15;
	  else
	    countdown = 0;
	  redraw = TRUE;
	  break;
	case 'h':
	  helplesson();
	  redraw = TRUE;
	  break;
	case 'H':
	  histlist( FALSE );
	  redraw = TRUE;
	  break;
	case 'I':
	  dooption( CqContext.snum, FALSE );
	  break;
	case 'L':
	  doreview( CqContext.snum );
	  break;
	case 'n':
	  if ( ! CqContext.hasnewsfile )
	    cdbeep();
	  else
	    {
	      news();
	      redraw = TRUE;
	    }
	  break;
	case 'N':
	  pseudo( CqContext.unum, CqContext.snum );
	  break;
	case 'O':
          UserOptsMenu(CqContext.unum);
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
		  if ( Ships[i].unum == CqContext.unum )
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
		      resign( CqContext.unum, FALSE );
		      Ships[CqContext.snum].status = SS_OFF;
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
	      Ships[CqContext.snum].team = modp1( Ships[CqContext.snum].team+1, NUMPLAYERTEAMS );
	      Users[CqContext.unum].team = Ships[CqContext.snum].team;
	      Ships[CqContext.snum].war[Ships[CqContext.snum].team] = FALSE;
	      Users[CqContext.unum].war[Users[CqContext.unum].team] = FALSE;
	    }
	  break;
	case 'S':
	  userstats( FALSE, 0 ); /* we're never really neutral ;-) - dwp */
	  redraw = TRUE;
	  break;
	case 'T':
	  doteamlist( Ships[CqContext.snum].team );
	  redraw = TRUE;
	  break;
	case 'U':
	  userlist( FALSE, 0 );
	  redraw = TRUE;
	  break;
	case 'W':
	  dowar( CqContext.snum );
	  redraw = TRUE;
	  break;
	case 'q':
	case 'Q':
	  CqContext.leave = TRUE;	
	  break;
	case '/':
	  playlist( FALSE, FALSE, 0 );
	  redraw = TRUE;
	  break;
	case '?':
	  doplanlist( 0 );
	  redraw = TRUE;
	  break;
	case '\014':	/* ^L */
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
  while ( stillalive( CqContext.snum ) &&  !CqContext.leave );
  
  /* Make our ship available for others to use. */
  if ( Ships[CqContext.snum].status == SS_RESERVED )
    {
      conqstats( CqContext.snum );
      PVLOCK(&ConqInfo->lockword);
      Ships[CqContext.snum].sdfuse = 0;
      Ships[CqContext.snum].status = SS_OFF;
      PVUNLOCK(&ConqInfo->lockword);
    }
  
  return;
  
}


/*  newship - create a new ship for a user (DOES LOCKING) */
/*  SYNOPSIS */
/*    int status, newship, unum, snum */
/*    int flag, newship */
/*    flag = newship( unum, snum ) */
int newship( int unum, int *snum )
{
  int i, j, k, l, system; 
  int fresh;
  int selectnum;
  int vec[MAXSHIPS];
  char cbuf[MSGMAXLINE];
  char cbuf2[MSGMAXLINE];
  char selectship[MSGMAXLINE];
  int availlist[MAXSHIPS], numavail;
  int numvec = 0;
  int ch;

  PVLOCK(&ConqInfo->lockword);
  
  Ships[*snum].status = SS_ENTERING;		/* show intent to fly */

  fresh = TRUE;				/* assume we want a fresh ship*/
  
  /* Count number of his ships flying. */
  j = 0;
  numvec = 0;
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status == SS_LIVE || Ships[i].status == SS_ENTERING )
      if ( Ships[i].unum == unum && *snum != i )
	{
	  j++;
	  vec[numvec++] = i;
	}

  PVUNLOCK(&ConqInfo->lockword);

  if ( ! Users[unum].ooptions[OOPT_MULTIPLE] )
    {
      /* Isn't a multiple; see if we need to reincarnate. */
      if ( j > 0 )
	{
	  /* Need to reincarnate. */
	  cdclear();
	  cdredo();

	  i = MSG_LIN2/2;
	  j = 9;

	  if (CheckPid(Ships[vec[0]].pid) == FALSE)
	    {			/* it's available */
	      attrset(InfoColor);
	      cdputs( "You're already playing on another ship." , i, j );
	      cbuf[0] = EOS;
	      if ( cdgetx( "Press TAB to reincarnate to this ship: ",
			   i + 1, j, TERMS, cbuf, MSGMAXLINE,
			   TRUE) != TERM_EXTRA )
		{
		  Ships[*snum].status = SS_RESERVED;
		  attrset(0);
		  return ( FALSE );
		}
	      attrset(0);
	    }
	  else
	    {
	      sprintf(cbuf, "You're already playing on another ship (pid=%d).",
		      Ships[vec[0]].pid);
	      cprintf(i,j,ALIGN_NONE,"#%d#%s",InfoColor, cbuf);
	      
	      Ships[*snum].status = SS_RESERVED;
	      putpmt( "--- press any key ---", MSG_LIN2 );

	      cdrefresh();
	      iogchar();
	      return ( FALSE );
	    }


	  /* Look for a live ship for us to take. */
	  PVLOCK(&ConqInfo->lockword);
	  for ( i = 1; i <= MAXSHIPS; i = i + 1)
	    if ( Ships[i].unum == unum && Ships[i].status == SS_LIVE )
	      {
		fresh = FALSE;
		Ships[*snum].status = SS_OFF;
		*snum = i;
		Ships[*snum].pid = CqContext.pid;
		Ships[*snum].status = SS_ENTERING;
		break;
	      }
	  PVUNLOCK(&ConqInfo->lockword);
	}
    }
  else
    {				/* a multiple, so see what's available */
      cdclear();
      cdrefresh();

      while (TRUE)
	{

	  cdclra(0, 0, MSG_LIN1 + 2, cdcols() - 1);

	  PVLOCK(&ConqInfo->lockword);
	  
	  /* Count number of his ships flying. */
	  j = 0;
	  numvec = 0;
	  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
	    if ( Ships[i].status == SS_LIVE || Ships[i].status == SS_ENTERING )
	      if ( Ships[i].unum == unum && *snum != i )
		{
		  j++;
		  vec[numvec++] = i;
		}
	  
	  PVUNLOCK(&ConqInfo->lockword);

	  numavail = 0;
	  for (k=0; k < numvec; k++)
	    {
	      if (CheckPid(Ships[vec[k]].pid) == FALSE)
		{
		  /* no pid, so available */
		  availlist[numavail++] = k;
		}
	    }

	  /* Is a multiple, max ships already in and no ships to
	     reincarnate too */
	  if ( j >= Users[unum].multiple && numavail == 0)
	    {
	      Ships[*snum].status = SS_RESERVED;
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

	      return ( FALSE );
	    }
	  
	  
	  if (numavail > 0)
	    {
	      /* we need to display a menu allowing the user to reincarnate
		 to an existing (but vacant) ship, or if he/she has slots
		 left, enter a new ship.
		 */
	      
	      
	      i = 3;
	      
	      cprintf(i++, 0, ALIGN_CENTER, 
		      "#%d#The following ship(s) are available for you to reincarnate to.", 
		      InfoColor);
	      
	      cbuf[0] = '\0';
	      for (k=0; k < numavail; k++)
		{
		  sprintf(cbuf2, "%d ", vec[availlist[k]]);
		  strcat(cbuf, cbuf2);
		}

	      if (j < Users[unum].multiple)
		{
		  cprintf(MSG_LIN1, 0, ALIGN_LEFT, 
			  "#%d#Enter a ship number, or press [TAB] to create a new one.",
			  NoColor);

                  cprintf(MSG_LIN2, 0, ALIGN_LEFT,
                          "#%d#[RETURN] to quit.",
                          NoColor);
		}
	      else
		cprintf(MSG_LIN1, 0, ALIGN_LEFT,
			"#%d#Enter a ship number to reincarnate to.",
			NoColor);

	      /* Now list the ships */

	      i++; i++;
	      cprintf(i++, 0, ALIGN_CENTER,
		      "#%d#Ship Number: #%d#%s",
		      NoColor | A_BOLD,
		      GreenLevelColor,
		      cbuf);

	      cdmove(0, 0);
	      cdrefresh();

	      if (iogtimed(&ch, 1))
		{
		  iBufPutc(ch);	/* stuff the char back in */
		  selectship[0] = EOS;
		  l = cdgetx("Ship Number: ", i, 1, TERMS, selectship, 
			     MSGMAXLINE / 2, TRUE);

		  if (l == TERM_EXTRA || l == TERM_NORMAL)
		    {
		      if (strlen(selectship))
			{
			  selectnum = atoi(selectship);
			  if (selectnum != 0 && selectnum <= MAXSHIPS)
			    { /* See if it's valid */
			      int found = FALSE;
			      
			      for (k=0; k < numavail && found == FALSE ; k++)
				{
				  if (vec[availlist[k]] == selectnum)
				    found = TRUE;
				}
			      if (found  == TRUE)
				{
				  PVLOCK(&ConqInfo->lockword);
				  Ships[*snum].status = SS_OFF;
				  *snum = selectnum;
				  fresh = FALSE;
				  Ships[*snum].pid = CqContext.pid;
				  Ships[*snum].status = SS_ENTERING;
				  PVUNLOCK(&ConqInfo->lockword);
				  break;
				}
			    }
			}
		      else  /* if strlen(selectship) */ 
			{ 
				/* if selectship was empty and term =
				   TERM_NORMAL, quit */
			  if (l == TERM_NORMAL)
			    {
			      Ships[*snum].status = SS_RESERVED;
			      
			      return(FALSE);
			    }
			  
			  
			  if ( j < Users[unum].multiple)
			    {
			      fresh = TRUE;
			      break;
			    }
			  else
			    {
			      cdbeep();
			    }
			}
		    }
		}
	    }
	  else
	    {			/* nothing available */
	      fresh = TRUE;
	      break;
	    }

	} /* back to the top... */
	      
    }
  
  /* Figure out which system to enter. */
  if ( fresh )
    {
				/* (re)init the ship's team! (bug 1/10/98) */
      Ships[*snum].team = Users[CqContext.unum].team;
      system = Ships[*snum].team;
      if ( ! capentry( *snum, &system ) )
	{
	  Ships[*snum].status = SS_RESERVED;
	  return ( ERR );
	}
    }
  else
    {
				/* now we clear ship's elapsed/cpu seconds
                                   so that there won't be a huge addition to
                                   the Teams/Users/Ships timing stats when
                                   a VACANT ships re-enters Conquest (in
				   case the signal handler didn't get a
				   chance to run) */
      Ships[*snum].ctime = 0;
      Ships[*snum].etime = 0;
    }

  
  PVLOCK(&ConqInfo->lockword);
  
  /* If necessary, initalize the ship */
  if ( fresh )
    {
      initship( *snum, unum );
      
      /* Randomly position the ship near the home sun (or planet). */
      if ( Planets[Teams[system].homeplanet].primary == Teams[system].homesun )
	i = Teams[system].homesun;
      else
	i = Teams[system].homeplanet;
      putship( *snum, Planets[i].x, Planets[i].y );
      Ships[*snum].dhead = rnduni( 0.0, 359.9 );
      Ships[*snum].head = Ships[*snum].dhead;
      Ships[*snum].dwarp = (real) rndint( 2, 5 ) ;/* #~~~ this is a kludge*/
      Ships[*snum].lock = -Teams[system].homeplanet;
    }
  else
    {				/* if we're reincarnating, skip any
				   messages that might have been sent
				   while we were gone, if enabled */
      if (conf_ClearOldMsgs == TRUE)
	{
	  PVLOCK(&ConqInfo->lockmesg);
	  Ships[*snum].lastmsg = ConqInfo->lastmsg;
	  Ships[*snum].alastmsg = Ships[*snum].lastmsg;
	  PVUNLOCK(&ConqInfo->lockmesg);
	}
				/* init user's last entry time */
      Users[Ships[*snum].unum].lastentry = getnow(NULL, 0);
    }
      
  Ships[*snum].robot = FALSE;
  Ships[*snum].action = 0;
  
  
  /* Straighten out the ships deltas. */
  fixdeltas( *snum );
  
  /* Finally, turn the ship on. */
  Ships[*snum].status = SS_LIVE;
  
  PVUNLOCK(&ConqInfo->lockword);
  
  return ( TRUE );
  
}


/*  play - play the game */
/*  SYNOPSIS */
/*    play */
int play()
{
  int laststat, now;
  int ch, rv;
  char msgbuf[128];
  
  /* Can't carry on without a vessel. */
  if ( (rv = newship( CqContext.unum, &CqContext.snum )) != TRUE)
    return(rv);
  
  drstart();			/* start a driver, if necessary */
  Ships[CqContext.snum].sdfuse = 0;	/* zero self destruct fuse */
  grand( &CqContext.msgrand );		/* initialize message timer */
  CqContext.leave = FALSE;		/* assume we won't want to bail */
  CqContext.redraw = TRUE;		/* want redraw first time */
  CqContext.display = TRUE;		/* ok to display */
  CqContext.msgok = TRUE;		/* ok to get messages */
  cdclear();			/* clear the display */
  cdredo();			/*  (quickly) */
  stoptimer();			/* stop the display interrupt */
  display( CqContext.snum, FALSE );	/* update the screen manually */
  gsecs( &laststat );		/* initialize stat timer */
  astoff();			/* disable before setting timer */
  settimer();			/* setup for next second */
  
  
  /* Tell everybody, we're here */

  sprintf(msgbuf, "%c%d (%s) has entered the game.",
	  Teams[Ships[CqContext.snum].team].teamchar,
	  CqContext.snum,
	  Ships[CqContext.snum].alias);
  
  stormsg(MSG_COMP, MSG_ALL, msgbuf);
  
  /* While we're alive, field commands and process them. */
  while ( stillalive( CqContext.snum ) )
    {
      /* Make sure we still control our ship. */
      if ( Ships[CqContext.snum].pid != CqContext.pid )
	break;
      
      /* Get a char with one second timeout. */
      if ( iogtimed( &ch, 1 ) )
	{
	  if (RMsg_Line == MSG_LIN1)
	    CqContext.msgok = FALSE;	/* off if we  have no msg line */
	  
#ifdef ENABLE_MACROS
	  if (DoMacro(ch) == TRUE)
	    {
	      while (iBufEmpty() == FALSE)
		{
		  ch = iBufGetCh();
		  command( ch );
		}
	    }
	  else
	    command( ch );
#else
	  command( ch );
#endif
	  
	  grand( &CqContext.msgrand );
	  CqContext.msgok = TRUE;
	  cdrefresh();
	}
      
      /* See if it's time to update the statistics. */
      if ( dsecs( laststat, &now ) >= 15 )
	{
	  conqstats( CqContext.snum );
	  laststat = now;
	}
    }
  
  CqContext.display = FALSE;
  conqstats( CqContext.snum );
  upchuck();
  
  /* Asts are still enabled, simply cancel the next screen update. */
  stoptimer();
  /*    aston();					/* enable asts again */
  
  dead( CqContext.snum, CqContext.leave );
  
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
  char password[MAXUSERNAME];	/* encrypted pw, "" if local */
  int utype;			/* user type -local/remote */
  string sorry1="I'm sorry, but the game is closed for repairs right now.";
  string sorry2="I'm sorry, but there is no room for a new player right now.";
  string sorry3="I'm sorry, but you are not allowed to play right now.";
  string sorryn="Please try again some other time.  Thank you.";
  char * selected_str="You have been selected to command a";
  char * starship_str=" starship.";
  char * prepare_str="Prepare to be beamed aboard...";
  
  col=0;

  utype = Logon(name, password);

  if ( ! gunum( unum, name, utype ) )
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
      team = rndint( 0, NUMPLAYERTEAMS - 1 );
      cbuf[0] = EOS;
      apptitle( team, cbuf );
      appchr( ' ', cbuf );
      i = strlen( cbuf );
      appstr( name, cbuf );
      cbuf[i] = (char)toupper( cbuf[i] );
      if ( ! c_register( name, cbuf, team, unum ) )
	{
	  cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry2 );
	  cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
	  cdmove( 1, 1 );
	  cdrefresh();
	  c_sleep( 2.0 );
	  return ( FALSE );
	}
				/* if a remote, then need to re-read/create
				   config file, and re-init color */
      if (utype == UT_REMOTE)
	{
	  if (GetConf(TRUE, *unum) == ERR)
	    return(FALSE);
	  InitColors();
	}

				/* copy in the password */
      strcpy(Users[*unum].pw, password);
				/* set lastentry time for new players */
      Users[*unum].lastentry = getnow(NULL, 0);
      gretds();

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

				/* if a remote, then need to re-read/create
				   config file, and re-init color */
  if (utype == UT_REMOTE)
    {
      if (GetConf(TRUE, *unum) == ERR)
	return(FALSE);
	InitColors();
    }

  
  /* Must be special to play when closed. */
  if ( ConqInfo->closed && ! Users[*unum].ooptions[OOPT_PLAYWHENCLOSED] )
    {
      cdclear();
      cdredo();
      cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
      cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  /* Can't play if on the shit list. */
  if ( Users[*unum].ooptions[OOPT_SHITLIST] )
    {
      cdclear();
      cdredo();
      cdputc( sorry3, MSG_LIN2/2 );
      cdputc( sorryn, MSG_LIN2/2+1 );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  /* Can't play without a ship. */
  if ( ! findship( &CqContext.snum ) )
    {
      cdclear();
      cdredo();
      cdputc( "I'm sorry, but there are no ships available right now.",
	     MSG_LIN2/2 );
      cdputc( sorryn, MSG_LIN2/2+1 );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  return ( TRUE );
  
}


