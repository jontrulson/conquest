#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
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
/* Have Phun!                                                         */
/**********************************************************************/

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"

static char *conquestId = "$Id$";

/*##  conquest - main program */
main(int argc, char *argv[], char *env[]) 
{
  /*    int cdcols, cdlins;*/
  int l; 
  string cpr = COPYRIGHT;
  string arr = "All rights reserved!";
  
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
/*      exit(1);*/
    }
  
  if (GetConf() == ERR)		/* do this BEFORE setgid() call! */
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
  
  rndini( 0, 0 );			/* initialize random numbers */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting cdinit().", __FILE__, __LINE__);
#endif
  
  
  cdinit();				/* set up display environment */
  
  cmaxlin = cdlins( 0 );
  
  cmaxcol = cdcols( 0 );
  
  
  
  csnum = 0;				/* force menu to get a new ship */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() welcoming player.", __FILE__, __LINE__);
#endif
  
  if ( welcome( &cunum ) )
    menu();
  
  drpexit();			/* make the driver go away */
  cdend();				/* clean up display environment */
  conqend();			/* machine dependent clean-up */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() *EXITING*", __FILE__, __LINE__);
#endif
  
  
  exit(0);
  
}


/*##  capentry - captured system entry for a new ship */
/*  SYNOPSIS */
/*    int flag, capentry */
/*    int capentry, snum, system */
/*    system = capentry( snum, system ) */
int capentry( int snum, int *system )
{
  int i, j; 
  int ch; 
  int owned[NUMTEAMS]; 
  
  /* First figure out which systems we can enter from. */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    {
      owned[i] = FALSE;
      /* We must own all three planets in a system. */
      for ( j = 0; j < 3; j = j + 1 )
	{
	  if ( pteam[teamplanets[i][j]] != steam[snum] )
	    goto cnext2_1; /* next 2; */
	}
      owned[i] = TRUE;
    cnext2_1:
      ;
    }
  owned[steam[snum]] = TRUE;		/* always can enter in our system */
  
  /* Now count how many systems we can enter from. */
  j = 0;
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    if ( owned[i] )
      j = j + 1;
  
  /* If we can only enter from one, we're done. */
  if ( j <= 1 )
    {
      *system = steam[snum];
      return ( TRUE );
    }
  
  /* Prompt for a decision. */
  c_strcpy( "Enter which system", cbuf );
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    if ( owned[i] )
      {
	appstr( ", ", cbuf );
	appstr( tname[i], cbuf );
      }
  /* Change first comma to a colon. */
  i = c_index( cbuf, ',' );
  if ( i != ERR )
    cbuf[i] = ':';
  
  cdclrl( MSG_LIN1, 2 );
  cdputc( cbuf, MSG_LIN1 );
  cdmove( 1, 1 );
  cdrefresh( TRUE );
  
  while ( stillalive( csnum ) )
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
	  *system = steam[snum];
	  return ( TRUE );
	  break;
	default:
	  for ( i = 0; i < NUMTEAMS; i = i + 1 )
	    if ( chrteams[i] == cupper( ch ) && owned[i] )
	      {
		/* Found a good one. */
		*system = i;
		return ( TRUE );
	      }
	  /* Didn't get a good one; complain and try again. */
	  cdbeep();
	  cdrefresh( TRUE );
	  break;
	}
    }
  
  return ( FALSE );			/* can get here because of stillalive() */
  
}


/*##  command - execute a user's command */
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
      
      if ( swarp[csnum] < 0.0 ) 
	swarp[csnum] = 0.0; 
      sdhead[csnum] = (real)(x); 
      slock[csnum] = 0; 
      
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
      dowarp( csnum, x );
      break;
    case 'a':				/* autopilot */
      if ( uooption[suser[csnum]][ OOPT_AUTOPILOT] )
	{
	  doautopilot( csnum );
	}
      else
	{
	  goto label1;
	}
      break;
    case 'A':				/* change allocation */
      doalloc( csnum );
      stoptimer();
      if ( stillalive( csnum ) )
	display( csnum );
      settimer();
      break;
    case 'b':				/* beam armies */
      dobeam( csnum );
      break;
    case 'B':				/* bombard a planet */
      dobomb( csnum );
      break;
    case 'C':				/* cloak control */
      docloak( csnum );
      break;
    case 'd':				/* detonate enemy torps */
    case '*':
      dodet( csnum );
      break;
    case 'D':				/* detonate own torps */
      domydet( csnum );
      break;
    case 'E':				/* emergency distress call */
      dodistress( csnum );
      break;
    case 'f':				/* phasers */
      dophase( csnum );
      break;
    case 'F':				/* phasers, same direction */
      dolastphase( csnum );
      break;
    case 'h':
      credraw = TRUE;
      stoptimer();
      dohelp( csubdcl );
      if ( stillalive( csnum ) )
	display( csnum );
      settimer();
      break;
    case 'H':
      credraw = TRUE;
      stoptimer();
      histlist( FALSE );
      if ( stillalive( csnum ) )
	display( csnum );
      settimer();
      break;
    case 'i':				/* information */
      doinfo( csnum );
      break;
    case 'I':				/* set user options */
      dooption( csnum, TRUE );
      break;
    case 'k':				/* set course */
      docourse( csnum );
      break;
    case 'K':				/* coup */
      docoup( csnum );
      break;
    case 'L':				/* review old messages */
      doreview( csnum );
      break;
    case 'm':				/* send a message */
      sendmsg( csnum, soption[csnum][OPT_TERSE] );
      break;
    case 'M':				/* strategic/tactical map */
      smap[csnum] = ! smap[csnum];	
      stoptimer();
      display( csnum );
      settimer();
      break;
    case 'N':				/* change pseudonym */
      pseudo( cunum, csnum );
      break;
    case 'o':				/* orbit nearby planet */
      doorbit( csnum );
      break;
    case 'P':				/* photon torpedo burst */
      doburst( csnum );
      break;
    case 'p':				/* photon torpedoes */
      dotorp( csnum );
      break;
    case 'Q':				/* self destruct */
      doselfdest( csnum );
      break;
    case 'R':				/* repair mode */
      if ( ! scloaked[csnum] )
	{
	  cdclrl( MSG_LIN1, 2 );
	  srmode[csnum] = TRUE;
	  sdwarp[csnum] = 0.0;
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
      dotow( csnum );
      break;
    case 'S':				/* more user stats */
      credraw = TRUE;
      stoptimer();
      userstats( FALSE );
      if ( stillalive( csnum ) )
	display( csnum );
      settimer();
      break;
    case 'T':				/* team list */
      credraw = TRUE;
      stoptimer();
      doteamlist( steam[csnum] );
      if ( stillalive( csnum ) )
	display( csnum );
      settimer();
      break;
    case 'u':				/* un-tractor */
      dountow( csnum );
      break;
    case 'U':				/* user stats */
      credraw = TRUE;
      stoptimer();
      userlist( FALSE );
      if ( stillalive( csnum ) )
	display( csnum );
      settimer();
      break;
    case 'W':				/* war and peace */
      dowar( csnum );
      break;
    case '-':				/* shields down */
      doshields( csnum, FALSE );
      stoptimer();
      display( csnum );
      settimer();
      break;
    case '+':				/* shields up */
      doshields( csnum, TRUE );
      stoptimer();
      display( csnum );
      settimer();
      break;
    case '/':				/* player list */
      credraw = TRUE;
      stoptimer();
      playlist( FALSE, FALSE );
      if ( stillalive( csnum ) )
	display( csnum );
      settimer();
      break;
    case '?':				/* planet list */
      credraw = TRUE;
      stoptimer();
      doplanlist( csnum );
      if ( stillalive( csnum ) )
	display( csnum );
      settimer();
      break;
#ifdef NOTUSED
    case '$':				/* spawn to DCL */
      if ( csubdcl )
	{
	  dosubdcl();
	}
      else
	{
	  goto label1;
	}
      break;
#endif
    case '\014':			/* clear and redisplay */
      stoptimer();
      cdredo();
      credraw = TRUE;
      display( csnum );
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


/*##  conqds - display background for Conquest */
/*  SYNOPSIS */
/*    int multiple, switchteams */
/*    conqds( multiple, switchteams ) */
void conqds( int multiple, int switchteams )
{
  int i, length, col, lin, lenc1;
  string c1=" CCC    OOO   N   N   QQQ   U   U  EEEEE   SSSS  TTTTT";
  string c2="C   C  O   O  NN  N  Q   Q  U   U  E      S        T";
  string c3="C      O   O  N N N  Q   Q  U   U  EEE     SSS     T";
  string c4="C   C  O   O  N  NN  Q  Q   U   U  E          S    T";
  string c5=" CCC    OOO   N   N   QQ Q   UUU   EEEEE  SSSS     T";
  
  extern char *ConquestVersion;
  extern char *ConquestDate;
  
  /* First clear the display. */
  cdclear();
  
  /* Display the logo. */
  lenc1 = strlen( c1 );
  col = (cmaxcol-lenc1) / 2;
  lin = 2;
  cdputs( c1, lin, col );
  lin = lin + 1;
  cdputs( c2, lin, col );
  lin = lin + 1;
  cdputs( c3, lin, col );
  lin = lin + 1;
  cdputs( c4, lin, col );
  lin = lin + 1;
  cdputs( c5, lin, col );
  
  /* Draw a box around the logo. */
  lin = lin + 1;
  cdbox( 1, col-2, lin, col+lenc1+1 );
  
  lin = lin + 1;
  if ( *closed )
    cdputc( "The game is closed.", lin );
  else
    {
      sprintf( cbuf, "%s (%s)",
	     ConquestVersion, ConquestDate);
      cdputc( cbuf, lin );
    }
  
  lin = lin + 1;
  cdputc( "Options:", lin );
  
  col = 13;
  lin = lin + 2;
  i = lin;
  cdputs( "(e) - enter the game", lin, col );
  if ( cnewsfile )
    {
      lin = lin + 1;
      cdputs( "(n) - read the news", lin, col );
    }
  lin = lin + 1;
  cdputs( "(h) - read the help lesson", lin, col );
  lin = lin + 1;
  cdputs( "(S) - more user statistics", lin, col );
  lin = lin + 1;
  cdputs( "(T) - team statistics", lin, col );
  lin = lin + 1;
  cdputs( "(U) - user statistics", lin, col );
  lin = lin + 1;
  cdputs( "(L) - review messages", lin, col );
  lin = lin + 1;
  cdputs( "(W) - set war or peace", lin, col );
  lin = lin + 1;
  cdputs( "(I) - change user options", lin, col );
  
  col = 48;
  lin = i;
  cdputs( "(N) - change your name", lin, col );
  if ( ! multiple )
    {
      lin = lin + 1;
      cdputs( "(r) - resign your commission", lin, col );
    }
  if ( multiple || switchteams )
    {
      lin = lin + 1;
      cdputs( "(s) - switch teams", lin, col );
    }
  lin = lin + 1;
  cdputs( "(H) - user history", lin, col );
  lin = lin + 1;
  cdputs( "(/) - player list", lin, col );
  lin = lin + 1;
  cdputs( "(?) - planet list", lin, col );
  lin = lin + 1;
  cdputs( "(q) - exit the program", lin, col );
  
  return;
  
}


/*##  dead - announce to a user that s/he is dead (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    int leave */
/*    dead( snum, leave ) */
void dead( int snum, int leave )
{
  int i, j, k, lin, col, kb, now, entertime; 
  int ch; 
  string ywkb="You were killed by ";
  char buf[128];
  
  /* (Quickly) clear the screen. */
  cdclear();
  cdredo();
  cdrefresh( FALSE );

  /* If something is wrong, don't do anything. */
  if ( snum < 1 || snum > MAXSHIPS )
    return;
  
  /* If our ships pid is wrong, we are indeed lost. */
  if ( spid[snum] != cpid )
    return;
  
  kb = skilledby[snum];
  
  /* Delay while our torps are exploding. */
  grand( &entertime );
  i = 0;
  while ( dgrand( entertime, &now ) < TORPEDOWAIT_GRAND )
    {
      i = 0;
      for ( j = 0; j < MAXTORPS; j = j + 1 )
	if ( tstatus[snum][j] == TS_DETONATE )
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
      c_strcpy( "You scuttled yourself.", cbuf );

      break;
    case KB_NEGENB:
      c_strcpy( "You were destroyed by the negative energy barrier.",
	       cbuf );

      break;
    case KB_CONQUER:
      c_strcpy(
	       "Y O U   C O N Q U E R E D   T H E   U N I V E R S E ! ! !",
	       cbuf );
      break;
    case KB_NEWGAME:
      c_strcpy( "N E W   G A M E !", cbuf );
      break;
    case KB_EVICT:
      c_strcpy( "Closed for repairs.", cbuf );
      break;
    case KB_SHIT:
      c_strcpy( "You are no longer allowed to play.", cbuf );
      break;
    case KB_GOD:
      c_strcpy( "You were killed by an act of GOD.", cbuf );

      break;
    case KB_DOOMSDAY:
      c_strcpy( "You were eaten by the doomsday machine.", cbuf );

      break;
    case KB_GOTDOOMSDAY:
      c_strcpy( "You destroyed the doomsday machine!", cbuf );
      break;
    case KB_DEATHSTAR:
      c_strcpy( "You were vaporized by the Death Star.", cbuf );

      break;
    case KB_LIGHTNING:
      c_strcpy( "You were destroyed by a lightning bolt.", cbuf );

      break;
    default:
      
      if ( kb > 0 && kb <= MAXSHIPS )
	{
	  sprintf( cbuf, "You were kill number %.1f for %s (",
		 skills[kb], spname[kb] );
	  appship( kb, cbuf );
	  appchr( ')', cbuf );
	  if ( sstatus[kb] != SS_LIVE )
	    appstr( ", who also died.", cbuf );
	  else
	    appchr( '.', cbuf );

	}
      else if ( -kb > 0 && -kb <= NUMPLANETS )
	{
	  sprintf(cbuf, "%s%s", ywkb, pname[-kb]);

	  if ( ptype[-kb] == PLANET_SUN )
	    {
	      appstr( "'s solar radiation.", cbuf );
	    }
	  else
	    {
	      appstr( "'s planetary defenses.", cbuf );
	    }
	}
      else
	{
	  /* We were unable to determine the cause of death. */
	  buf[0] = '\0';
	  appship( snum, buf );
	  sprintf(cbuf, "dead: %s was killed by %d.", buf, kb);
	  cerror( cbuf );
	  clog(cbuf);
	  
	  strcpy(cbuf, ywkb);
	  strcat(cbuf, "nothing in particular.  (How strange...)");
	}
    }
  
  cdputc( cbuf, 8 );
  
  if ( kb == KB_NEWGAME )
    {
      sprintf( cbuf, "Universe conquered by %s for the %s team.",
	     conqueror, conqteam );
      cdputc( cbuf, 10 );
    }
  else if ( kb == KB_SELF )
    {
      i = sarmies[snum];
      if ( i > 0 )
	{
	  c_strcpy( "The ", cbuf );
	  if ( i == 1 )
	    appstr( "army", cbuf );
	  else
	    {
	      if ( i < 100 )
		appnum( i, cbuf );
	      else
		appint( i, cbuf );
	      appstr( " armies", cbuf );
	    }
	  appstr( " you were carrying ", cbuf );
	  if ( i == 1 )
	    appstr( "was", cbuf );
	  else
	    appstr( "were", cbuf );
	  appstr( " not amused.", cbuf );
	  cdputc( cbuf, 10 );
	}
    }
  else if ( kb >= 0 )
    {
      if ( sstatus[kb] == SS_LIVE )
	{
	  sprintf( cbuf, "He had %d%% shields and %d%% damage.",
		 round(sshields[kb]), round(sdamage[kb]) );
	  cdputc( cbuf, 10 );
	}
    }
  sprintf( cbuf, "You got %.1f this time.", oneplace(skills[snum]) );
  cdputc( cbuf, 12 );
  cdmove( 1, 1 );
  cdrefresh( FALSE );

  if ( ! ( leave && kb == KB_SELF ) && kb != KB_SHIT && kb != KB_EVICT )
    c_sleep( 4.0 );
  
  for ( i = 1; i <= 10 && sstatus[snum] == SS_DYING; i = i + 1 )
    c_sleep( 1.0 );
  sstatus[snum] = SS_RESERVED;
  ssdfuse[snum] = -TIMEOUT_PLAYER;
  skilledby[snum] = 0;
  
  switch ( kb )
    {
    case KB_CONQUER:
      do
	{
	  cdclear();
	  cdredo();
	  ch = cdgetx( "Any last words? ",
		      14, 1, TERMS, lastwords, MAXLASTWORDS );
	  cdclear();
	  cdredo();
	  if ( lastwords[0] != EOS )
	    {
	      cdputc( "You last words are entered as:", 13 );
	      sprintf( cbuf, "%c%s%c", '"', lastwords, '"' );
	      cdputc( cbuf, 14 );
	    }
	  else
	    cdputc(
		   "You have chosen to NOT leave any last words:", 14 );
	  ch = getcx( "Press TAB to confirm:", 16, 0,
		     TERMS, cbuf, 10 );
	}
      while ( ch != TERM_EXTRA ); /* until -> while */
      break;
    case KB_SELF:
    case KB_EVICT:
    case KB_SHIT:
      /* Do nothing special. */
      break;
    default:
      ioeat();
      putpmt( "--- press space when done ---", MSG_LIN2 );
      cdrefresh( TRUE );
      while ( ! iogtimed( &ch, 1 ) && stillalive( csnum ) )
	;
      break;
    }
  cdmove( 1, 1 );
  
  /* Turn off sticky war so we can change war settings from menu(). */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    srwar[snum][i] = FALSE;
  
  return;
  
}


/*##  dispoption - display options */
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


/*##  doalloc - change weapon/engine allocations */
/*  SYNOPSIS */
/*    int snum */
/*    doalloc( snum ) */
void doalloc( int snum )
{
  char ch;
  int i, alloc;
  int l; 
  
  string pmt="New weapons allocation: (30-70) ";
  
  cdclrl( MSG_LIN1, 2 );
  ch = cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  if ( ch == TERM_EXTRA )
    sweapons[snum] = sengines[snum];
  else if ( ch == TERM_NORMAL )
    {
      i = 0;
      l = safectoi( &alloc, cbuf, i );			/* ignore status */
      if ( alloc != 0 )
	{
	  if ( alloc < 30 )
	    alloc = 30;
	  else if ( alloc > 70 )
	    alloc = 70;
	  sweapons[snum] = alloc;
	}
    }
  
  sengines[snum] = 100 - sweapons[snum];
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*##  doautopilot - handle the autopilot */
/*  SYNOPSIS */
/*    int snum */
/*    doautopilot( snum ) */
void doautopilot( int snum )
{
  int now, laststat; 
  int ch; 
  string conf="Press TAB to engage autopilot:";
  
  cdclrl( MSG_LIN1, 2 );
  if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) != TERM_EXTRA )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  c_putmsg( "Autopilot activated.", MSG_LIN1 );
  srobot[snum] = TRUE;
  gsecs( &laststat );			/* initialize stat timer */
  while ( stillalive( csnum ) )
    {
      /* Make sure we still control our ship. */
      if ( spid[snum] != cpid )
	break;
      
      /* See if it's time to update the statistics. */
      if ( dsecs( laststat, &now ) >= 15 )
	{
	  conqstats( csnum );
	  laststat = now;
	}
      
      /* Get a character. */
      if ( ! iogtimed( &ch, 1 ) )
	continue;		/* next -> echo */
      cmsgok = FALSE;
      grand( &cmsgrand );
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
	  cdrefresh( TRUE );
	}
      cmsgok = TRUE;
      if (ch == TERM_ABORT)
	break;
    }
  srobot[snum] = FALSE;
  saction[snum] = 0;
  
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*##  dobeam - beam armies up or down (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobeam( snum ) */
void dobeam( int snum )
{
  int pnum, total, num, upmax, downmax, capacity, beamax, i;
  int ototal, entertime, now;
  int l, oldsshup, dirup, zeroed, conqed;
  int ch; 
  char buf[MSGMAXLINE];
  real rkills;
  int done = FALSE;
  string lastfew="Fleet orders prohibit removing the last three armies.";
  string abt="...aborted...";
  
  srmode[snum] = FALSE;
  
  cdclrl( MSG_LIN1, 2 );
  
  /* Check for allowability. */
  if ( swarp[snum] >= 0.0 )
    {
      c_putmsg( "We must be orbiting a planet to use the transporter.",
	       MSG_LIN1 );
      return;
    }
  pnum = -slock[snum];
  if ( sarmies[snum] > 0 )
    {
      if ( ptype[pnum] == PLANET_SUN )
	{
	  c_putmsg( "Idiot!  Our armies will fry down there!", MSG_LIN1 );
	  return;
	}
      else if ( ptype[pnum] == PLANET_MOON )
	{
	  c_putmsg( "Phoon!  Our armies will suffocate down there!",
		   MSG_LIN1 );
	  return;
	}
      else if ( pteam[pnum] == TEAM_GOD )
	{
	  c_putmsg(
		   "GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
		   MSG_LIN1 );
	  return;
	}
    }
  
  i = puninhabtime[pnum];
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
  
  if ( pteam[pnum] != steam[snum] &&
      pteam[pnum] != TEAM_SELFRULED &&
      pteam[pnum] != TEAM_NOTEAM )
    if ( ! swar[snum][pteam[pnum]] && parmies[pnum] != 0) /* can take empty planets */
      {
	c_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
	return;
      }
  
  if ( sarmies[snum] == 0 &&
      pteam[pnum] == steam[snum] && parmies[pnum] <= MIN_BEAM_ARMIES )
    {
      c_putmsg( lastfew, MSG_LIN1 );
      return;
    }
  
  rkills = skills[snum];
#ifdef DEBUG_MISC
  clog("dobeam(): rkills=%f skills[%d]=%f",
       rkills, snum, skills[snum]);
#endif
  if ( rkills < (real)1.0 )
    {
      c_putmsg(
	       "Fleet orders prohibit beaming armies until you have a kill.",
	       MSG_LIN1 );
      return;
    }
  
  /* Figure out what can be beamed. */
  downmax = sarmies[snum];
  if ( spwar(snum,pnum) ||
      pteam[pnum] == TEAM_SELFRULED ||
      pteam[pnum] == TEAM_NOTEAM ||
      pteam[pnum] == TEAM_GOD ||
      parmies[pnum] == 0 )
    {
      upmax = 0;
    }
  else
    {
      capacity = min( ifix( rkills ) * 2, armylim[steam[snum]] );
      upmax = min( parmies[pnum] - MIN_BEAM_ARMIES, capacity-sarmies[snum] );
    }
  
  /* If there are armies to beam but we're selfwar... */
  if ( upmax > 0 && selfwar(snum) && steam[snum] == pteam[pnum] )
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
      cdrefresh( TRUE );
      done = FALSE;
      while ( stillalive( csnum ) && done == FALSE)
	{
	  if ( ! iogtimed( &ch, 1 ) )
	    {
	      continue;	/* next */
	    }
	  switch ( clower( ch ) )
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
  ch = cdgetx( cbuf, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
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
      if ( alldig( buf ) != YES )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  return;
	}
      i = 0;
      l = safectoi( &num, buf, i );			/* ignore status */
      if ( num < 1 || num > beamax )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  return;
	}
    }
  
  /* Now we are ready! */
  if ( pteam[pnum] > NUMTEAMS )
    /* If the planet is not race owned, make it war with us. */
    ssrpwar[snum][pnum] = TRUE;
  else if ( pteam[pnum] != steam[snum] )
    {
      /* For a team planet make the war sticky and send an intruder alert. */
      srwar[snum][pteam[pnum]] = TRUE;
      
      /* Chance to create a robot here. */
      intrude( snum, pnum );
    }
  
  /* Lower shields. */
  oldsshup = sshup[snum];
  sshup[snum] = FALSE;
  
  /* Beam. */
  total = 0;
  ototal = -1;				/* force an update the first time */
  zeroed = FALSE;
  conqed = FALSE;
  
  grand( &entertime );
  while(TRUE)			/* repeat infloop */
    {
      if ( ! stillalive( csnum ) )
	return;
      if ( iochav( 0 ) )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  break;
	}
      
      /* See if it's time to beam again. */
      while ( dgrand( entertime, &now ) >= BEAM_GRAND )
	{
	  /*	      entertime = mod( entertime + BEAM_GRAND, 24*60*60*1000 );*/
	  grand(&entertime);
	  PVLOCK(lockword);
	  if ( dirup )
	    {
	      /* Beam up. */
	      if ( parmies[pnum] <= MIN_BEAM_ARMIES )
		{
		  PVUNLOCK(lockword);
		  c_putmsg( lastfew, MSG_LIN1 );
		  break;
		}
	      sarmies[snum] = sarmies[snum] + 1;
	      parmies[pnum] = parmies[pnum] - 1;
	    }
	  else
	    {
	      /* Beam down. */
	      sarmies[snum] = sarmies[snum] - 1;
	      if ( pteam[pnum] == TEAM_NOTEAM || parmies[pnum] == 0 )
		{
		  takeplanet( pnum, snum );
		  conqed = TRUE;
		}
	      else if ( pteam[pnum] != steam[snum] )
		{
		  parmies[pnum] = parmies[pnum] - 1;
		  if ( parmies[pnum] == 0 )
		    {
		      zeroplanet( pnum, snum );
		      zeroed = TRUE;
		    }
		}
	      else
		parmies[pnum] = parmies[pnum] + 1;
	    }
	  PVUNLOCK(lockword);
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
	  appstr( pname[pnum], cbuf );
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
	    cdrefresh( TRUE );		/* display the first time */
	  ototal = total;
	}
      
      if ( dirup && parmies[pnum] <= MIN_BEAM_ARMIES )
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
  sshup[snum] = oldsshup;
  
  /* Try to display the last bombing message. */
  cdrefresh( TRUE );
  
  if ( conqed )
    {
      sprintf( cbuf, "You have conquered %s.", pname[pnum] );
      c_putmsg( cbuf, MSG_LIN1 );
    }
  else if ( zeroed )
    c_putmsg( "Sensors show hostile forces eliminated from the planet.",
	     MSG_LIN1 );
  
  return;
  
}


/*##  dobomb - bombard a planet (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dobomb( snum ) */
void dobomb( int snum )
{
  int pnum, i, now, entertime, total, ototal, oparmies;
  real x, killprob;
  int oldsshup; 
  char  buf[MSGMAXLINE];
  
  string lastfew="The last few armies are eluding us.";
  string abt="...aborted...";
  
  srmode[snum] = FALSE;
  
  cdclrl( MSG_LIN2, 1 );
  cdclrl(MSG_LIN1, 1);
  
  /* Check for allowability. */
  if ( swarp[snum] >= 0.0 )
    {
      c_putmsg( "We must be orbiting a planet to bombard it.", MSG_LIN1 );
      return;
    }
  pnum = -slock[snum];
  if ( ptype[pnum] == PLANET_SUN || ptype[pnum] == PLANET_MOON ||
      pteam[pnum] == TEAM_NOTEAM || parmies[pnum] == 0 )
    {
      c_putmsg( "There is no one there to bombard.", MSG_LIN1 );
      return;
    }
  if ( pteam[pnum] == steam[snum] )
    {
      c_putmsg( "We can't bomb our own armies!", MSG_LIN1 );
      return;
    }
  if ( pteam[pnum] != TEAM_SELFRULED && pteam[pnum] != TEAM_GOD )
    if ( ! swar[snum][pteam[pnum]] )
      {
	c_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
	return;
      }
  
  /* Confirm. */
  sprintf( cbuf, "Press TAB to bombard %s, %d armies:",
	 pname[pnum], parmies[pnum] );
  cdclrl( MSG_LIN1, 1 );
  cdclrl( MSG_LIN2, 1 );
  if ( cdgetx( cbuf, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE ) != TERM_EXTRA )
    {
      cdclrl( MSG_LIN1, 1 );
      cdclrl( MSG_LIN2, 1 );
      return;
    }
  
  /* Handle war logic. */
  ssrpwar[snum][pnum] = TRUE;
  if ( pteam[pnum] >= 0 && pteam[pnum] < NUMTEAMS )
    {
      /* For a team planet make the war sticky and send an intruder alert. */
      srwar[snum][pteam[pnum]] = TRUE;
      intrude( snum, pnum );
    }
  /* Planets owned by GOD have a special defense system. */
  if ( pteam[pnum] == TEAM_GOD )
    {
      sprintf( cbuf, "That was a bad idea, %s...", spname[snum] );
      c_putmsg( cbuf, MSG_LIN1 );
      damage( snum,  rnduni( 50.0, 100.0 ), KB_LIGHTNING );
      return;
    }
  
  /* Lower shields. */
  oldsshup = sshup[snum];
  sshup[snum] = FALSE;
  
  /* Bombard. */
  total = 0;
  ototal = -1					/* force an update the first time */;
  oparmies = -1;
  grand( &entertime )			/* get start time */;
  while(TRUE)       /*repeat infloop */
    {
      if ( ! stillalive( csnum ) )
	return;
      if ( iochav( 0 ) )
	{
	  c_putmsg( abt, MSG_LIN1 );
	  break;
	}
      
      cdrefresh(FALSE);
      
      /* See if it's time to bomb yet. */
      while ( abs (dgrand( entertime, &now )) >= BOMBARD_GRAND )
	{
	  if ( swfuse[snum] > 0 )
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
			      (real)((real)parmies[pnum]/100.0))) + 0.5 );
	  /*	    cerror(MSG_GOD, "DEBUG: killprob = %d\n", (int) (killprob *10));*/
	  if ( rnd( 0 ) < killprob )
	    {
	      /*	    cerror(MSG_GOD, "DEBUG: we're in: killprob = %d\n", (int)(killprob * 10));*/
	      PVLOCK(lockword);
	      if ( parmies[pnum] <= MIN_BOMB_ARMIES )
		{
		  /* No more armies left to bomb. */
		  PVUNLOCK(lockword);
		  c_putmsg( lastfew, MSG_LIN1 );
		  goto cbrk22; /* break 2;*/
		}
	      parmies[pnum] = parmies[pnum] - 1;
	      
	      skills[snum] = skills[snum] + BOMBARD_KILLS;
	      ustats[suser[snum]][USTAT_ARMBOMB] =
		ustats[suser[snum]][USTAT_ARMBOMB] + 1;
	      tstats[steam[snum]][TSTAT_ARMBOMB] =
		tstats[steam[snum]][TSTAT_ARMBOMB] + 1;
	      PVUNLOCK(lockword);
	      total = total + 1;
	    }
	  /*	    astservice(0);
		    cdrefresh(FALSE);
		    c_sleep(ITER_SECONDS);
		    */
	}
      
      if ( parmies[pnum] <= MIN_BOMB_ARMIES )
	{
	  /* No more armies left to bomb. */
	  c_putmsg( lastfew, MSG_LIN1 );
	  break;
	}
      
      if ( parmies[pnum] != oparmies || ototal != total )
	{
	  /* Either our bomb run total or the population changed. */
	  oparmies = parmies[pnum];
	  if ( total == 1 )
	    c_strcpy( "y", buf );
	  else
	    c_strcpy( "ies", buf );
	  sprintf( cbuf, "Bombing %s, %d arm%s killed, %d left.",
		 pname[pnum], total, buf, oparmies );
	  c_putmsg( cbuf, MSG_LIN1 );
	  cdrefresh(FALSE);
	  if ( ototal == -1 )
	    {
	      cdrefresh( TRUE );		/* display the first time */
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
  sshup[snum] = oldsshup;
  
  /* Try to display the last bombing message. */
  cdrefresh( TRUE );
  
  return;
  
}


/*##  doburst - launch a burst of three torpedoes */
/*  SYNOPSIS */
/*    int snum */
/*    doburst( snum ) */
void doburst( int snum )
{
  real dir;
  int l; 
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( swfuse[snum] > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < TORPEDO_FUEL )
    {
      c_putmsg( "Not enough fuel to launch a torpedo.", MSG_LIN1 );
      return;
    }
  
  if ( gettarget( "Torpedo burst: ", MSG_LIN1, 1, &dir, slastblast[snum] ) )
    {
      if ( ! launch( snum, dir, 3 ) )
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


/*##  docloak - cloaking device control */
/*  SYNOPSIS */
/*    int snum */
/*    docloak( snum ) */
void docloak( int snum )
{
  string pmt="Press TAB to engage cloaking device: ";
  string nofuel="Not enough fuel to engage cloaking device.";
  
  cdclrl( MSG_LIN1, 1 );
  cdclrl( MSG_LIN2, 1 );
  
  if ( scloaked[snum] )
    {
      scloaked[snum] = FALSE;
      c_putmsg( "Cloaking device disengaged.", MSG_LIN1 );
      return;
    }
  if ( sefuse[snum] > 0 )
    {
      c_putmsg( "Engines are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < CLOAK_ON_FUEL )
    {
      c_putmsg( nofuel, MSG_LIN1 );
      return;
    }
  
  cdclrl( MSG_LIN1, 1 );
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) == TERM_EXTRA )
    {
      if ( cloak( snum ) )
	c_putmsg( "Cloaking device engaged.", MSG_LIN2 );
      else
	c_putmsg( nofuel, MSG_LIN2 );
    }
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*##  docoup - attempt to rise from the ashes (DOES LOCKING) */
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
  if ( oneplace( skills[snum] ) < MIN_COUP_KILLS )
    {
      c_putmsg(
	       "Fleet orders require three kills before a coup can be attempted.",
	       MSG_LIN1 );
      return;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( pteam[i] == steam[snum] && parmies[i] > 0 )
      {
	c_putmsg( "We don't need to coup, we still have armies left!",
		 MSG_LIN1 );
	return;
      }
  if ( swarp[snum] >= 0.0 )
    {
      c_putmsg( nhp, MSG_LIN1 );
      return;
    }
  pnum = -slock[snum];
  if ( pnum != homeplanet[steam[snum]] )
    {
      c_putmsg( nhp, MSG_LIN1 );
      return;
    }
  if ( parmies[pnum] > MAX_COUP_ENEMY_ARMIES )
    {
      c_putmsg( "The enemy is still too strong to attempt a coup.",
	       MSG_LIN1 );
      return;
    }
  i = puninhabtime[pnum];
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
	     i );
      c_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  
  /* Now our team can tell coup time for free. */
  tcoupinfo[steam[snum]] = TRUE;
  
  i = couptime[steam[snum]];
  if ( i > 0 )
    {
      sprintf( cbuf, "Our forces need %d more minutes to organize.", i );
      c_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  
  /* Confirm. */
  cdclrl( MSG_LIN1, 1 );
  if ( cdgetx( conf, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) != TERM_EXTRA )
    {
      c_putmsg( "...aborted...", MSG_LIN1 );
      return;
    }
  
  /* Now wait it out... */
  c_putmsg( "Attempting coup...", MSG_LIN1 );
  cdrefresh( TRUE );
  grand( &entertime );
  while ( dgrand( entertime, &now ) < COUP_GRAND )
    {
      /* See if we're still alive. */
      if ( ! stillalive( csnum ) )
	return;
      
      /* Sleep (and enable asts so the display will work). */
      aston();
      c_sleep( ITER_SECONDS );
      astoff();
    }
  
  cdclrl( MSG_LIN1, 1 );
  PVLOCK(lockword);
  if ( pteam[pnum] == steam[snum] )
    {
      PVUNLOCK(lockword);
      c_putmsg( "Sensors show hostile forces eliminated from the planet.",
	       MSG_LIN2 );
      return;
    }
  
  failprob = parmies[pnum] / MAX_COUP_ENEMY_ARMIES * 0.5 + 0.5;
  if ( rnd( 0 ) < failprob )
    {
      /* Failed; setup new reorganization time. */
      couptime[steam[snum]] = rndint( 5, 10 );
      PVUNLOCK(lockword);
      c_putmsg( "Coup unsuccessful.", MSG_LIN2 );
      return;
    }
  
  takeplanet( pnum, snum );
  parmies[pnum] = rndint( 10, 20 );		/* create token coup force */
  ustats[suser[snum]][USTAT_COUPS] = ustats[suser[snum]][USTAT_COUPS] + 1;
  tstats[steam[snum]][TSTAT_COUPS] = tstats[steam[snum]][TSTAT_COUPS] + 1;
  PVUNLOCK(lockword);
  c_putmsg( "Coup successful!", MSG_LIN2 );
  
  return;
  
}


/*##  docourse - set course */
/*  SYNOPSIS */
/*    int snum */
/*    docourse( snum ) */
void docourse( int snum )
{
  int i, j, what, sorpnum, xsorpnum, newlock, token, count;
  real dir, appx, appy; 
  int ch; 
  
  cdclrl( MSG_LIN1, 2 );

  ch = cdgetx( "Come to course: ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  delblanks( cbuf );
  if ( ch == TERM_ABORT || cbuf[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  newlock = 0;				/* default to no lock */
  fold( cbuf );
  
  what = NEAR_ERROR;
  if ( alldig( cbuf ) == YES )
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
  else if ( cbuf[0] == 's' && alldig( &cbuf[1] ) == YES )
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
      if ( sstatus[sorpnum] != SS_LIVE )
	{
	  c_putmsg( "Not found.", MSG_LIN2 );
	  return;
	}
      if ( scloaked[sorpnum] )
	{
	  if ( swarp[sorpnum] <= 0.0 )
	    {
	      c_putmsg( "Sensors are unable to lock on.", MSG_LIN2 );
	      return;
	    }
	  appx = rndnor(sx[sorpnum], CLOAK_SMEAR_DIST);
	  appy = rndnor(sy[sorpnum], CLOAK_SMEAR_DIST);
	}
      else
	{
	  appx = sx[sorpnum];
	  appy = sy[sorpnum];
	}
      dir = (real)angle( sx[snum], sy[snum], appx, appy );
      
      /* Give info if he used TAB. */
      if ( ch == TERM_EXTRA )
	infoship( sorpnum, snum );
      else
	cdclrl( MSG_LIN1, 1 );
      break;
    case NEAR_PLANET:
      dir = angle( sx[snum], sy[snum], px[sorpnum], py[sorpnum] );
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
  
  if ( swarp[snum] < 0.0 )		/* if orbitting */
    swarp[snum] = 0.0;		/* break orbit */
  sdhead[snum] = dir;			/* set direction first to avoid */
  slock[snum] = newlock;		/*  a race in display() */
  
  return;
  
}


/*##  dodet - detonate enemy torps */
/*  SYNOPSIS */
/*    int snum */
/*    dodet( snum ) */
void dodet( int snum )
{
  cdclrl( MSG_LIN2, 1 );
  
  if ( swfuse[snum] > 0 )
    c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
  else if ( enemydet( snum ) )
    c_putmsg( "detonating...", MSG_LIN1 );
  else
    c_putmsg( "Not enough fuel to fire detonators.", MSG_LIN1 );
  
  return;
  
}


/*##  dodistress - send an emergency distress call */
/*  SYNOPSIS */
/*    int snum */
/*    dodistress( snum ) */
void dodistress( int snum )
{
  int i;
  string pmt="Press TAB to send an emergency distress call: ";
  
  cdclrl( MSG_LIN1, 2 );
  
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) == TERM_EXTRA )
    {
      sprintf( cbuf,
	     "sh=%d, dam=%d, fuel=%d, temp=",
	     round(sshields[snum]),
	     round(sdamage[snum]),
	     round(sfuel[snum]) );
      i = round(swtemp[snum]);
      if ( i < 100 )
	appint( i, cbuf );
      else
	appstr( "**", cbuf );
      appchr( '/', cbuf );
      i = round(setemp[snum]);
      if ( i < 100 )
	appint( i, cbuf );
      else
	appstr( "**", cbuf );
      i = sarmies[snum];
      if ( i > 0 )
	{
	  appstr( ", armies=", cbuf );
	  appint( i, cbuf );
	}
      if ( swfuse[snum] > 0 )
	appstr( ", -weapons", cbuf );
      if ( sefuse[snum] > 0 )
	appstr( ", -engines", cbuf );
      
      stormsg( snum, -steam[snum], cbuf );
    }
  
  cdclrl( MSG_LIN1, 1 );
  
  return;
  
}


/*##  dohelp - display a list of commands */
/*  SYNOPSIS */
/*    int subdcl */
/*    dohelp( subdcl ) */
void dohelp( int subdcl )
{
  int lin, col, tlin;
  int ch;
  
  cdclear();
  cdputc( "CONQUEST COMMANDS", 1 );
  
  lin = 4;
  
  /* Display the left side. */
  tlin = lin;
  col = 4;
  cdputs( "0-9,=  set warp factor (= is 10)", tlin, col );
  tlin = tlin + 1;
  cdputs( "A      change w/e allocations", tlin, col );
  tlin = tlin + 1;
  cdputs( "b      beam armies", tlin, col );
  tlin = tlin + 1;
  cdputs( "B      bombard a planet", tlin, col );
  tlin = tlin + 1;
  cdputs( "C      cloaking device", tlin, col );
  tlin = tlin + 1;
  cdputs( "d,*    detonate enemy torpedoes", tlin, col );
  tlin = tlin + 1;
  cdputs( "D      detonate your own torpedoes", tlin, col );
  tlin = tlin + 1;
  cdputs( "E      send emergency distress call", tlin, col );
  tlin = tlin + 1;
  cdputs( "f      fire phasers", tlin, col );
  tlin = tlin + 1;
  cdputs( "F      fire phasers, same direction", tlin, col );
  tlin = tlin + 1;
  cdputs( "h      this", tlin, col );
  tlin = tlin + 1;
  cdputs( "H      user history", tlin, col );
  tlin = tlin + 1;
  cdputs( "i      information", tlin, col );
  tlin = tlin + 1;
  cdputs( "I      set user options", tlin, col );
  tlin = tlin + 1;
  cdputs( "k      set course", tlin, col );
  tlin = tlin + 1;
  cdputs( "K      try a coup", tlin, col );
  tlin = tlin + 1;
  cdputs( "L      review old messages", tlin, col );
  tlin = tlin + 1;
  cdputs( "m      send a message", tlin, col );
  tlin = tlin + 1;
  cdputs( "M      short/long range sensor toggle", tlin, col );
  
  /* Now do the right side. */
  tlin = lin;
  col = 44;
  cdputs( "N      change your name", tlin, col );
  tlin = tlin + 1;
  cdputs( "o      come into orbit", tlin, col );
  tlin = tlin + 1;
  cdputs( "p      launch photon torpedoes", tlin, col );
  tlin = tlin + 1;
  cdputs( "P      launch photon torpedo burst", tlin, col );
  tlin = tlin + 1;
  cdputs( "Q      initiate self-destruct", tlin, col );
  tlin = tlin + 1;
  cdputs( "R      enter repair mode", tlin, col );
  tlin = tlin + 1;
  cdputs( "S      more user statistics", tlin, col );
  tlin = tlin + 1;
  cdputs( "t      engage tractor beams", tlin, col );
  tlin = tlin + 1;
  cdputs( "T      team list", tlin, col );
  tlin = tlin + 1;
  cdputs( "u      un-engage tractor beams", tlin, col );
  tlin = tlin + 1;
  cdputs( "U      user statistics", tlin, col );
  tlin = tlin + 1;
  cdputs( "W      set war or peace", tlin, col );
  tlin = tlin + 1;
  cdputs( "-      lower shields", tlin, col );
  tlin = tlin + 1;
  cdputs( "+      raise shields", tlin, col );
  tlin = tlin + 1;
  cdputs( "/      player list", tlin, col );
  tlin = tlin + 1;
  cdputs( "?      planet list", tlin, col );
  if ( subdcl )
    {
      tlin = tlin + 1;
      cdputs( "$      spawn to DCL", tlin, col );
    }
  tlin = tlin + 1;
  cdputs( "^L     refresh the screen", tlin, col );
  tlin = tlin + 1;
  cdputs( "[RETURN] get last info", tlin, col);
  tlin = tlin + 1;
  cdputs( "[TAB] get next last info", tlin, col);
  
  putpmt( "--- press space when done ---", MSG_LIN2 );
  cdrefresh( TRUE );
  while ( ! iogtimed( &ch, 1 ) && stillalive( csnum ) )
    ;
  
  return;
  
}


/*##  doinfo - do an info command */
/*  SYNOPSIS */
/*    int snum */
/*    doinfo( snum ) */
void doinfo( int snum )
{
  char ch; 
  int i, j, what, sorpnum, xsorpnum, count, token, now[8]; 
  int l, extra; 
  
  cdclrl( MSG_LIN1, 2 );
  
  
  ch = cdgetx( "Information on: ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
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
      c_strcpy( clastinfostr, cbuf );
      if ( cbuf[0] == EOS )
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
    }
  else
    c_strcpy( cbuf, clastinfostr );
  
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
  else if ( cbuf[0] == 's' && alldig( &cbuf[1] ) == YES )
    {
      i = 1;
      l = safectoi( &j, cbuf, i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( alldig( cbuf ) == YES )
    {
      i = 0;
      l = safectoi( &j, cbuf, i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( planmatch( cbuf, &j, FALSE ) )
    infoplanet( "", j, snum );
  else if ( stmatch( cbuf, "time", FALSE ) )
    {
      getnow( now );
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


/*##  dolastphase - do a fire phasers same direction command */
/*  SYNOPSIS */
/*    int snum */
/*    dolastphase( snum ) */
void dolastphase( int snum )
{
  cdclrl( MSG_LIN1, 1 );
  
  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN2 );
      return;
    }
  if ( swfuse[snum] > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < PHASER_FUEL )
    {
      c_putmsg( "Not enough fuel to fire phasers.", MSG_LIN2 );
      return;
    }
  
  if ( phaser( snum, slastphase[snum] ) )
    cdclrl( MSG_LIN2, 1 );
  else
    c_putmsg( ">PHASERS DRAINED<", MSG_LIN2 );
  
  return;
  
}


/*##  domydet - detonate your own torps */
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


/*##  dooption - set user options */
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
      sop[i] = soption[snum][i];			/* used in case we abort */
      top[i] = soption[snum][i];			/* used for dispoption() */
    }
  
  while ( stillalive( csnum ) && leave == FALSE)
    {
      /* Display the current options. */
      dispoption( top );
      cdrefresh( TRUE );
      
      /* Get a character. */
      if ( ! iogtimed( &ch, 1 ) )
	continue; /* next; */
      switch ( ch )
	{
	case TERM_EXTRA:
	  /* Done fooling around, update the user options. */
	  for ( i = 0; i < MAXOPTIONS; i = i + 1)
	    uoption[suser[snum]][i] = top[i];
	  leave = TRUE;
	  break;
	case TERM_ABORT:
	  /* Decided to abort; restore things. */
	  for ( i = 0; i < MAXOPTIONS; i = i + 1)
	    soption[snum][i] = sop[i];
	  if ( dodisplay )
	    {
	      /* Force an update. */
	      stoptimer();
	      display( snum );		/* update the display */
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
	      soption[snum][tok] = top[tok];
	      
	      if ( dodisplay )
		{
		  /* Force an update. */
		  stoptimer();
		  display( snum );
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


/*##  doorbit - orbit the ship and print a message */
/*  SYNOPSIS */
/*    int snum */
/*    doorbit( snum ) */
void doorbit( int snum )
{
  int pnum;
  
  if ( ( swarp[snum] == ORBIT_CW ) || ( swarp[snum] == ORBIT_CCW ) )
    infoplanet( "But we are already orbiting ", -slock[snum], snum );
  else if ( ! findorbit( snum, &pnum ) )
    {
      sprintf( cbuf, "We are not close enough to orbit, %s.",
	     spname[snum] );
      c_putmsg( cbuf, MSG_LIN1 );
      cdclrl( MSG_LIN2, 1 );
    }
  else if ( swarp[snum] > MAX_ORBIT_WARP )
    {
      sprintf( cbuf, "We are going to fast to orbit, %s.",
	     spname[snum] );
      c_putmsg( cbuf, MSG_LIN1 );
      sprintf( cbuf, "Maximum orbital insertion velocity is warp %.1f.",
	     oneplace(MAX_ORBIT_WARP) );
      c_putmsg( cbuf, MSG_LIN2 );
    }
  else
    {
      orbit( snum, pnum );
      infoplanet( "Coming into orbit around ", -slock[snum], snum );
    }
  
  return;
  
}


/*##  dophase - do a fire phasers command */
/*  SYNOPSIS */
/*    int snum */
/*    dophase( snum ) */
void dophase( int snum )
{
  real dir;
  
  cdclrl( MSG_LIN2, 1 );
  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( swfuse[snum] > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < PHASER_FUEL )
    {
      c_putmsg( "Not enough fuel to fire phasers.", MSG_LIN1 );
      return;
    }
  
  if ( gettarget( "Fire phasers: ", MSG_LIN1, 1, &dir, slastblast[snum] ) )
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


/*##  doplanlist - display the planet list for a ship */
/*  SYNOPSIS */
/*    int snum */
/*    doplanlist( snum ) */
void doplanlist( int snum )
{
  int ch;
  
  cdclear();
  while ( stillalive( csnum ) )
    {
      planlist( steam[snum] );
      putpmt( "--- press space when done ---", MSG_LIN2 );
      cdrefresh( TRUE );
      if ( iogtimed( &ch, 1 ) )
	break;
    }
  return;
  
}


/*##  doreview - review messages for a ship */
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
      
      cmsgok = FALSE;		/* don't want to get msgs when reading
				   old ones.  */
    }

  lstmsg = slastmsg[snum];	/* don't want lstmsg changing while reading old ones. */

  if ( ! review( snum, lstmsg ) )
    {
      c_putmsg( "There are no old messages.", MSG_LIN1 );
      putpmt( "--- press space for more ---", MSG_LIN2 );
      cdrefresh( TRUE );
      while ( ! iogtimed( &ch, 1 ) && stillalive( csnum ) )
	;
      cdclrl( MSG_LIN1, 2 );
    }

  if (RMsg_Line == MSG_LIN1)
    {
      cmsgok = TRUE;		
    }

  return;
  
}


/*##  doselfdest - execute a self-destruct command */
/*  SYNOPSIS */
/*    doselfdest */
void doselfdest(int snum)
{
  
  int i, j, entertime, now; 
  int ch; 
  string pmt="Press TAB to initiate self-destruct sequence: ";
  
  cdclrl( MSG_LIN1, 2 );

  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
               MSG_LIN1 );
      return;
    }
  
  if ( cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE ) != TERM_EXTRA )
    {
      /* Chickened out. */
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  /* See if we want to exit to dcl. */
  /*    cleave = ( 'q' == cbuf[0] || 'Q' == cbuf[0] );*/
  cdclrl( MSG_LIN1, 1 );
  
  /* Set up the destruct fuse. */
  ssdfuse[csnum] = SELFDESTRUCT_FUSE;
  
  gsecs( &entertime );
  
  /* Force a screen update. */
  stoptimer();
  display( csnum );
  settimer();
  cmsgok = TRUE;			/* messages are ok in the beginning */
  while ( ssdfuse[csnum] > 0 )
    {
      ssdfuse[csnum] = SELFDESTRUCT_FUSE - dsecs ( entertime, &now );
      /* Display new messages until T-minus 3 seconds. */
      if ( ssdfuse[csnum] < 3 )
	cmsgok = FALSE;
      
      if ( ! stillalive( csnum ) )
	{
	  /* Died in the process. */
	  ssdfuse[csnum] = 0;
	  return;
	}
      
      if ( iochav( 0 ) )
	{
	  /* Got a new character. */
	  grand( &cmsgrand );
	  cdclrl( MSG_LIN1, 2 );
	  if ( iogchar( ch ) == TERM_ABORT )
	    {
	      ssdfuse[csnum] = 0;
	      c_putmsg( "Self destruct has been canceled.", MSG_LIN1 );
	      return;
	    }
	  else
	    {
	      c_putmsg( "Press ESCAPE to abort self destruct.", MSG_LIN1 );
	      cdbeep();
	      cdrefresh( TRUE );
	    }
	}
      aston();			/* enable asts so the display will work */
      c_sleep( ITER_SECONDS );
      astoff();
    }
  cmsgok = FALSE;			/* turn off messages */
  
  if ( *dstatus == DS_LIVE )
    if ( dist(sx[csnum], sy[csnum], *dx, *dy) <= DOOMSDAY_KILL_DIST )
      {
	*dstatus = DS_OFF;
	stormsg( MSG_DOOM, MSG_ALL, "AIEEEEEEEE!" );
	killship( csnum, KB_GOTDOOMSDAY );
      }
    else
      killship( csnum, KB_SELF );
  else
    killship( csnum, KB_SELF );
  
  return;
  
}


/*##  doshields - raise or lower shields */
/*  SYNOPSIS */
/*    int snum */
/*    int up */
/*    doshields( snum, up ) */
void doshields( int snum, int up )
{
  
  sshup[snum] = up;
  if ( up )
    {
      srmode[snum] = FALSE;
      c_putmsg( "Shields raised.", MSG_LIN1 );
    }
  else
    c_putmsg( "Shields lowered.", MSG_LIN1 );
  cdclrl( MSG_LIN2, 1 );
  
  return;
  
}


/*##  doteamlist - display the team list for a ship */
/*  SYNOPSIS */
/*    int team */
/*    doteamlist( team ) */
void doteamlist( int team )
{
  int ch;
  
  cdclear();
  while ( stillalive( csnum ) )
    {
      teamlist( team );
      putpmt( "--- press space when done ---", MSG_LIN2 );
      cdrefresh( TRUE );
      if ( iogtimed( &ch, 1 ) )
	break;
    }
  return;
  
}


/*##  dotorp - launch single torpedoes */
/*  SYNOPSIS */
/*    int snum */
/*    dotorp( snum ) */
void dotorp( int snum )
{
  real dir;
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( scloaked[snum] )
    {
      c_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return;
    }
  if ( swfuse[snum] > 0 )
    {
      c_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( sfuel[snum] < TORPEDO_FUEL )
    {
      c_putmsg( "Not enough fuel to launch a torpedo.", MSG_LIN1 );
      return;
    }
  if ( gettarget( "Launch torpedo: ", MSG_LIN1, 1, &dir, slastblast[snum] ) )
    {
      if ( ! launch( snum, dir, 1 ) )
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


/*##  dotow - attempt to tow another ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dotow( snum ) */
void dotow( int snum )
{
  char ch;
  int i, other;
  int l, warsome;
  
  cdclrl( MSG_LIN1, 2 );
  if ( stowedby[snum] != 0 )
    {
      c_strcpy( "But we are being towed by ", cbuf );
      appship( stowing[snum], cbuf );
      appchr( '!', cbuf );
      return;
    }
  if ( stowing[snum] != 0 )
    {
      c_strcpy( "But we're already towing ", cbuf );
      appship( stowing[snum], cbuf );
      appchr( '.', cbuf );
      return;
    }
  ch = cdgetx( "Tow which ship? ", MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  cdclrl( MSG_LIN1, 1 );
  if ( ch == TERM_ABORT )
    return;
  
  i = 0;
  l = safectoi( &other, cbuf, i );		/* ignore status */
  cbuf[0] = EOS;
  
  PVLOCK(lockword);
  if ( other < 1 || other > MAXSHIPS )
    c_strcpy( "No such ship.", cbuf );
  else if ( sstatus[other] != SS_LIVE )
    c_strcpy( "Not found.", cbuf );
  else if ( other == snum )
    c_strcpy( "We can't tow ourselves!", cbuf );
  else if ( dist( sx[snum], sy[snum], sx[other], sy[other] ) > TRACTOR_DIST )
    c_strcpy( "That ship is out of tractor range.", cbuf );
  else if ( swarp[other] < 0.0 )
    c_strcpy( "You can't tow a ship out of orbit.", cbuf );
  else if ( sqrt( powf(( (real) (sdx[snum] - sdx[other]) ), (real) 2) +
		 powf( (real) ( sdx[snum] - sdx[other] ), 
		      (real) 2 ) / ( MM_PER_SEC_PER_WARP *
				     ITER_SECONDS )) > MAX_TRACTOR_WARP ) 
    sprintf( cbuf, "That ships relative velocity is higher than %f.",
	   MAX_TRACTOR_WARP );
  else if ( stowing[other] != 0 || stowedby[other] != 0 )
    c_strcpy(
	     "There seems to be some interference with the tractor beams...",
	     cbuf );
  else
    {
      stowedby[other] = snum;
      stowing[snum] = other;
      c_strcpy( "Tractor beams engaged.", cbuf );
    }
  PVUNLOCK(lockword);
  c_putmsg( cbuf, MSG_LIN2 );
  
  return;
  
}


/*##  dountow - release a tow (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    dountow( snum ) */
void dountow( int snum )
{
  int entertime, now;
  int warsome; 
  
  cdclrl( MSG_LIN1, 2 );
  if ( stowedby[snum] != 0 )
    {
      /* If we're at war with him or he's at war with us, make it */
      /*  hard to break free. */
      warsome = ( satwar( snum, stowedby[snum]) );
      if ( warsome )
	{
	  grand( &entertime );
	  while ( dgrand( entertime, &now ) < BREAKAWAY_GRAND )
	    {
	      if ( ! stillalive( csnum ) )
		return;
	      aston();
	      c_sleep( ITER_SECONDS );
	      astoff();
	    }
	}
      if ( warsome && ( rnd( 0 ) > BREAKAWAY_PROB ) )
	c_putmsg( "Attempt to break free failed.", MSG_LIN1 );
      else
	{
	  c_strcpy( "Breaking free from ship ", cbuf );
	  appship( stowedby[snum], cbuf );
	  PVLOCK(lockword);
	  if ( stowedby[snum] != 0 )
	    {
	      /* Coast to a stop. */
	      shead[snum] = shead[stowedby[snum]];
	      swarp[snum] = swarp[stowedby[snum]];
	      
	      /* Release the tow. */
	      if ( stowing[stowedby[snum]] != 0 )
		stowing[stowedby[snum]] = 0;
	      stowedby[snum] = 0;
	    }
	  PVUNLOCK(lockword);
	  appchr( '.', cbuf );
	  c_putmsg( cbuf, MSG_LIN1 );
	}
    }
  else if ( stowing[snum] != 0 )
    {
      c_strcpy( "Tow released from ship ", cbuf );
      appship( stowing[snum], cbuf );
      PVLOCK(lockword);
      if ( stowing[snum] != 0 )
	{
	  /* Set other ship coasting. */
	  shead[stowing[snum]] = shead[snum];
	  swarp[stowing[snum]] = swarp[snum];
	  
	  /* Release the tow. */
	  if ( stowedby[stowing[snum]] != 0 )
	    stowedby[stowing[snum]] = 0;
	  stowing[snum] = 0;
	}
      PVUNLOCK(lockword);
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
    }
  else
    c_putmsg( "No tractor beam activity detected.", MSG_LIN1 );
  
  return;
  
}


/*##  dowar - declare war or peace */
/*  SYNOPSIS */
/*    int snum */
/*    dowar( snum ) */
void dowar( int snum )
{
  int i, entertime, now; 
  int tuwar[NUMTEAMS], dowait;
  int ch;
  const int POffset = 47, WOffset = 61;
  
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    tuwar[i] = swar[snum][i];
  
  cdclrl( MSG_LIN1, 2 );
  
  cdputs(
	 "Press TAB when done, ESCAPE to abort:  Peace: # # # #  War: # # # #", 
	 MSG_LIN1, 1 );
  
  while ( stillalive( csnum ) )
    {
      for ( i = 0; i < NUMTEAMS; i = i + 1 )
	if ( tuwar[i] )
	  {
	    cdput( ' ', MSG_LIN1, POffset + (i*2) );
	    if ( srwar[snum][i] )
	      ch = chrteams[i];
	    else
	      ch = clower(chrteams[i]);
	    cdput( ch, MSG_LIN1, WOffset + (i*2) );
	  }
	else
	  {
	    cdput( clower(chrteams[i]), MSG_LIN1, POffset + (i*2) );
	    cdput( ' ', MSG_LIN1, WOffset+(i*2) );
	  }
      cdrefresh( TRUE );
      if ( iogtimed( &ch, 1 ) == FALSE )
	{
	  continue; /* next; */
	}
      
      ch = clower( ch );
      if ( ch == TERM_ABORT )
	break;
      if ( ch == TERM_EXTRA )
	{
	  /* Now update the war settings. */
	  dowait = FALSE;
	  for ( i = 0; i < NUMTEAMS; i = i + 1 )
	    {
	      if ( tuwar[i] && ! swar[snum][i] )
		dowait = TRUE;
	      uwar[suser[snum]][i] = tuwar[i];
	      swar[snum][i] = tuwar[i];
	    }
	  
	  /* Only check for computer delay when flying. */
	  if ( sstatus[snum] != SS_RESERVED && dowait )
	    {
	      /* We've set war with at least one team, stall a little. */
	      c_putmsg(
		       "Reprogramming the battle computer, please stand by...",
		       MSG_LIN2 );
	      cdrefresh( TRUE );
	      grand( &entertime );
	      while ( dgrand( entertime, &now ) < REARM_GRAND )
		{
		  /* See if we're still alive. */
		  if ( ! stillalive( csnum ) )
		    return;
		  
		  /* Sleep (and enable asts so the display will work). */
		  aston();
		  c_sleep( ITER_SECONDS );
		  astoff();
		}
	    }
	  break;
	}
      
      for ( i = 0; i < NUMTEAMS; i = i + 1 )
	if ( ch == clower( chrteams[i] ) )
	  {
	    if ( ! tuwar[i] || ! srwar[snum][i] )
	      {
		tuwar[i] = ! tuwar[i];
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


/*##  dowarp - set warp factor */
/*  SYNOPSIS */
/*    int snum */
/*    real warp */
/*    dowarp( snum, warp ) */
void dowarp( int snum, real warp )
{
  real mw;
  
  cdclrl( MSG_LIN2, 1 );
  
  if ( sdwarp[snum] == 0.0 && warp != 0.0 )
    {
      /* See if engines are working. */
      if ( sefuse[snum] > 0 )
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
      srmode[snum] = FALSE;
    }
  
  /* If orbitting, break orbit. */
  if ( swarp[snum] < 0.0 )
    {
      swarp[snum] = 0.0;
      slock[snum] = 0;
      sdhead[snum] = shead[snum];
    }
  
  /* Handle ship limitations. */
  sdwarp[snum] = min( warp, warplim[steam[snum]] );
  
  sprintf( cbuf, "Warp %d.", (int) sdwarp[snum] );
  c_putmsg( cbuf, MSG_LIN1 );
  
  /* Warn about damage limitations. */
  mw = maxwarp( snum );
  if ( around( sdwarp[snum] ) > mw )
    {
      sprintf( cbuf, "(Due to damage, warp is currently limited to %.1f.)",
	     mw );
      c_putmsg( cbuf, MSG_LIN2 );
    }
  
  return;
  
}


/*##  getoption - decode char into option */
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


/*##  gretds - block letter "greetings..." */
/*  SYNOPSIS */
/*    gretds */
void gretds()
{
  
  int col;
  string g1=" GGG   RRRR   EEEEE  EEEEE  TTTTT   III   N   N   GGG    SSSS";
  string g2="G   G  R   R  E      E        T      I    NN  N  G   G  S";
  string g3="G      RRRR   EEE    EEE      T      I    N N N  G       SSS";
  string g4="G  GG  R  R   E      E        T      I    N  NN  G  GG      S  ..  ..  ..";
  string g5=" GGG   R   R  EEEEE  EEEEE    T     III   N   N   GGG   SSSS   ..  ..  ..";
  
  col = (cmaxcol-strlen(g5)) / 2;
  cdputs( g1, 1, col );
  cdputs( g2, 2, col );
  cdputs( g3, 3, col );
  cdputs( g4, 4, col );
  cdputs( g5, 5, col );
  
  return;
  
}


/*##  menu - main user menu (DOES LOCKING) */
/*  SYNOPSIS */
/*    menu */
void menu(void)
{
  
  int i, lin, col, sleepy, countdown;
  int ch;
  int lose, oclosed, switchteams, multiple, redraw;
  int playrv;
  
  EnableSignalHandler();	/* enable trapping of interesting signals */
  
  /* Initialize statistics. */
  initstats( &sctime[csnum], &setime[csnum] );
  
  /* Log this entry into the Game. */
  loghist( cunum );
  
  /* Set up a few ship characteristics here rather than in initship(). */
  suser[csnum] = cunum;
  steam[csnum] = uteam[cunum];
  spid[csnum] = cpid;
  for ( i = 0; i < MAXOPTIONS; i = i + 1 )
    soption[csnum][i] = uoption[cunum][i];
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    {
      srwar[csnum][i] = FALSE;
      swar[csnum][i] = uwar[cunum][i];
    }
  stcpn( upname[cunum], spname[csnum], MAXUSERPNAME );
  
  /* Set up some things for the menu display. */
  switchteams = uooption[cunum][OOPT_SWITCHTEAMS];
  multiple = uooption[cunum][OOPT_MULTIPLE];
  oclosed = *closed;
  cleave = FALSE;
  redraw = TRUE;
  sleepy = 0;
  countdown = 0;
  playrv = FALSE;
  
  do                 
    {
      /* Make sure things are proper. */
      if (playrv == ERR) 
	{
	  if ( csnum < 1 || csnum > MAXSHIPS )
	    lose = TRUE;
	  else if ( spid[csnum] != cpid )
	    lose = TRUE;
	  else if ( sstatus[csnum] != SS_RESERVED )
	    {
	      clog( "menu(): Ship %d no longer reserved.", csnum );
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
	  cdputs(
		 "Suddenly  a  sinister,  wraithlike  figure appears before you",
		 lin, col );
	  lin = lin + 1;
	  cdputs(
		 "seeming to float in the air.  In a low,  sorrowful  voice  he",
		 lin, col );
	  lin = lin + 1;
	  cdputs(
		 "says, \"Alas, the very nature of the universe has changed, and",
		 lin, col );
	  lin = lin + 1;
	  cdputs(
		 "your ship cannot be found.  All must now pass away.\"  Raising",
		 lin, col );
	  lin = lin + 1;
	  cdputs(
		 "his  oaken  staff  in  farewell,  he fades into the spreading",
		 lin, col );
	  lin = lin + 1;
	  cdputs(
		 "darkness.  In his place appears a  tastefully  lettered  sign",
		 lin, col );
	  lin = lin + 1;
	  cdputs( "reading:", lin, col );
	  lin = lin + 2;
	  cdputc( "INITIALIZATION FAILURE", lin );
	  lin = lin + 2;
	  cdputs(
		 "The darkness becomes all encompassing, and your vision fails.",
		 lin, col );
	  ioeat();
	  cdmove( 1, 1 );
	  cdrefresh( FALSE );
	  return;
	}
      
      /* Some simple housekeeping. */
      if ( multiple != uooption[cunum][OOPT_MULTIPLE] )
	{
	  multiple = ! multiple;
	  redraw = TRUE;
	}
      
      /*JET - 	if ( switchteams != ( uooption[cunum][OOPT_SWITCHTEAMS] ||
	ustats[cunum][USTAT_ENTRIES] <= 0 ) )
	{
	switchteams = ! switchteams;
	redraw = TRUE;
	}
	*/
      
      if ( switchteams != uooption[cunum][OOPT_SWITCHTEAMS])
	{
	  switchteams = uooption[cunum][OOPT_SWITCHTEAMS];
	  redraw = TRUE;
	}
      if ( oclosed != *closed )
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
      cdputs( cbuf, MSG_LIN1, 1 );
      userline( cunum, csnum, cbuf, FALSE, TRUE );
      cdputs( cbuf, MSG_LIN2, 1 );
      
      cdmove( 1, 1 );
      cdrefresh( TRUE );
      
      /* Try to kill the driver if we started one the last time */
      /*  we played and we've been in the menu long enough. */
      if ( countdown > 0 )
	{
	  countdown = countdown - 1;
	  if ( countdown <= 0 )
	    drkill();
	}
      
      /* Reset up the destruct fuse. */
      ssdfuse[csnum] = -TIMEOUT_PLAYER;
      
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
	  if ( childpid != 0 )
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
	  dooption( csnum, FALSE );
	  break;
	case 'L':
	  doreview( csnum );
	  break;
	case 'n':
	  if ( ! cnewsfile )
	    cdbeep();
	  else
	    {
	      news();
	      redraw = TRUE;
	    }
	  break;
	case 'N':
	  pseudo( cunum, csnum );
	  break;
	case 'r':
	  if ( multiple )
	    cdbeep();
	  else
	    {
	      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
		if ( sstatus[i] == SS_LIVE ||
		    sstatus[i] == SS_ENTERING )
		  if ( suser[i] == cunum )
		    break;
	      if ( i <= MAXSHIPS )
		cdbeep();
	      else
		{
		  cdclrl( MSG_LIN1, 2 );
		  cdrefresh( FALSE );
		  if ( confirm( 0 ) )
		    {
		      resign( cunum );
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
	      steam[csnum] = modp1( steam[csnum]+1, NUMTEAMS );
	      uteam[cunum] = steam[csnum];
	      swar[csnum][steam[csnum]] = FALSE;
	      uwar[cunum][uteam[cunum]] = FALSE;
	    }
	  break;
	case 'S':
	  userstats( FALSE );
	  redraw = TRUE;
	  break;
	case 'T':
	  doteamlist( steam[csnum] );
	  redraw = TRUE;
	  break;
	case 'U':
	  userlist( FALSE );
	  redraw = TRUE;
	  break;
	case 'W':
	  dowar( csnum );
	  redraw = TRUE;
	  break;
	case 'q':
	case 'Q':
	  cleave = TRUE;	
	  break;
	case '/':
	  playlist( FALSE, FALSE );
	  redraw = TRUE;
	  break;
	case '?':
	  doplanlist( csnum );
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
  while ( stillalive( csnum ) &&  !cleave );
  
  /* Make our ship available for others to use. */
  if ( sstatus[csnum] == SS_RESERVED )
    {
      conqstats( csnum );
      PVLOCK(lockword);
      ssdfuse[csnum] = 0;
      sstatus[csnum] = SS_OFF;
      PVUNLOCK(lockword);
    }
  
  return;
  
}


/*##  newship - create a new ship for a user (DOES LOCKING) */
/*  SYNOPSIS */
/*    int status, newship, unum, snum */
/*    int flag, newship */
/*    flag = newship( unum, snum ) */
int newship( int unum, int *snum )
{
  int i, j, system; 
  int fresh;
  int vec[MAXSHIPS];
  char cbuf[MSGMAXLINE];
  int numvec = 0;

  PVLOCK(lockword);
  
  sstatus[*snum] = SS_ENTERING;		/* show intent to fly */

  fresh = TRUE;				/* assume we want a fresh ship*/
  
  /* Count number of his ships flying. */
  j = 0;
  numvec = 0;
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] == SS_LIVE || sstatus[i] == SS_ENTERING )
      if ( suser[i] == unum && *snum != i )
	{
	  j++;
	  vec[numvec++] = i;
	}

  PVUNLOCK(lockword);

  if ( ! uooption[unum][OOPT_MULTIPLE] )
    {
      /* Isn't a multiple; see if we need to reincarnate. */
      if ( j > 0 )
	{
	  /* Need to reincarnate. */
	  cdclear();
	  cdredo();

	  i = MSG_LIN2/2;
	  j = 9;

	  if (kill(spid[vec[0]], 0) == -1)
	    {
	      cdputs( "You're already playing on another ship." , i, j );
	      if ( cdgetx( "Press TAB to reincarnate to this ship: ",
			   i + 1, j, TERMS, cbuf, MSGMAXLINE ) != TERM_EXTRA )
		{
		  sstatus[*snum] = SS_RESERVED;
		  
		  return ( FALSE );
		}
	    }
	  else
	    {
	      sprintf(cbuf, "You're already playing on another ship (pid=%d).",
		      spid[vec[0]]);
	      cdputs( cbuf , i, j );
	      
	      sstatus[*snum] = SS_RESERVED;
	      putpmt( "--- press any key ---", MSG_LIN2 );

	      cdrefresh(TRUE);
	      iogchar(0);
	      return ( FALSE );
	    }


	  /* Look for a live ship for us to take. */
	  PVLOCK(lockword);
	  for ( i = 1; i <= MAXSHIPS; i = i + 1)
	    if ( suser[i] == unum && sstatus[i] == SS_LIVE )
	      {
		fresh = FALSE;
		sstatus[*snum] = SS_OFF;
		*snum = i;
		spid[*snum] = cpid;
		sstatus[*snum] = SS_ENTERING;
		break;
	      }
	  PVUNLOCK(lockword);
	}
    }
  else
    {
      /* Is a multiple. */
      if ( j >= umultiple[unum] )
	{
	  /* Flying too many ships */
	  sstatus[*snum] = SS_RESERVED;
	  cdclear();
	  cdredo();
	  i = MSG_LIN2/2;
	  cdputc(
		 "I'm sorry, but your playing on too many ships right now.", i );
	  i = i + 1;
	  c_strcpy( "You are only allowed to fly ", cbuf );
	  j = umultiple[unum];
	  appint( j, cbuf );
	  appstr( " ship", cbuf );
	  if ( j != 1 )
	    appchr( 's', cbuf );
	  appstr( " at one time.", cbuf );
	  cdputc( cbuf, i );
	  cdrefresh( FALSE );
	  c_sleep( 2.0 );
	  return ( FALSE );
	}
    }
  
  /* Figure out which system to enter. */
  if ( fresh )
    {
      system = steam[*snum];
      if ( ! capentry( *snum, &system ) )
	{
	  sstatus[*snum] = SS_RESERVED;
	  return ( ERR );
	}
    }
  
  PVLOCK(lockword);
  
  /* If necessary, initalize the ship */
  if ( fresh )
    {
      initship( *snum, unum );
      
      /* Randomly position the ship near the home sun (or planet). */
      if ( pprimary[homeplanet[system]] == homesun[system] )
	i = homesun[system];
      else
	i = homeplanet[system];
      putship( *snum, px[i], py[i] );
      sdhead[*snum] = rnduni( 0.0, 359.9 );
      shead[*snum] = sdhead[*snum];
      sdwarp[*snum] = (real) rndint( 2, 5 ) ;/* #~~~ this is a kludge*/
      slock[*snum] = -homeplanet[system];
    }
  else
    {				/* if we're reincarnating, skip any
				   messages that might have been sent
				   while we were gone, if enabled */
      if (conf_ClearOldMsgs == TRUE)
	{
	  PVLOCK(lockmesg);
	  slastmsg[*snum] = *lastmsg;
	  salastmsg[*snum] = slastmsg[*snum];
	  PVUNLOCK(lockmesg);
	}
    }
      
  srobot[*snum] = FALSE;
  saction[*snum] = 0;
  
  
  /* Straighten out the ships deltas. */
  fixdeltas( *snum );
  
  /* Finally, turn the ship on. */
  sstatus[*snum] = SS_LIVE;
  
  PVUNLOCK(lockword);
  
  return ( TRUE );
  
}


/*##  play - play the game */
/*  SYNOPSIS */
/*    play */
int play()
{
  int laststat, now;
  int ch, i, rv;
  char msgbuf[128];
  
  /* Can't carry on without a vessel. */
  if ( (rv = newship( cunum, &csnum )) != TRUE)
    return(rv);
  
  drstart();				/* start a driver, if necessary */
  ssdfuse[csnum] = 0;				/* zero self destruct fuse */
  grand( &cmsgrand );			/* initialize message timer */
  cleave = FALSE;				/* assume we won't want to bail */
  credraw = TRUE;				/* want redraw first time */
  cdisplay = TRUE;				/* ok to display */
  cmsgok = TRUE;				/* ok to get messages */
  cdclear();				/* clear the display */
  cdredo();					/*  (quickly) */
  stoptimer();
  display( csnum );			/* update the screen manually */
  gsecs( &laststat );			/* initialize stat timer */
  astoff();					/* disable before setting timer */
  settimer();				/* setup for next second */
  
  
  /* Tell everybody, we're here */

  sprintf(msgbuf, "%c%d (%s) has entered the game.",
	  chrteams[uteam[suser[csnum]]],
	  csnum,
	  upname[suser[csnum]]);
  
  stormsg(MSG_COMP, MSG_ALL, msgbuf);
  
  /* While we're alive, field commands and process them. */
  while ( stillalive( csnum ) )
    {
      /* Make sure we still control our ship. */
      if ( spid[csnum] != cpid )
	break;
      
      /* Get a char with one second timeout. */
      if ( iogtimed( &ch, 1 ) )
	{
	  if (RMsg_Line == MSG_LIN1)
	    cmsgok = FALSE;	/* off if we  have no msg line */
	  
#ifdef ENABLE_MACROS
	  if ((i = DoMacro(ch)) == TRUE)
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
	  
	  grand( &cmsgrand );
	  cmsgok = TRUE;
	  cdrefresh( TRUE );
	}
      
      /* See if it's time to update the statistics. */
      if ( dsecs( laststat, &now ) >= 15 )
	{
	  conqstats( csnum );
	  laststat = now;
	}
    }
  
  cdisplay = FALSE;
  conqstats( csnum );
  upchuck();
  
  /* Asts are still enabled, simply cancel the next screen update. */
  stoptimer();
  /*    aston();					/* enable asts again */
  
  dead( csnum, cleave );
  
  return;
  
}


/*##  welcome - entry routine */
/*  SYNOPSIS */
/*    int flag, welcome */
/*    int unum */
/*    flag = welcome( unum ) */
int welcome( int *unum )
{
  int i, team; 
  char name[MAXUSERNAME];
  
  string sorry1="I'm sorry, but the game is closed for repairs right now.";
  string sorry2="I'm sorry, but there is no room for a new player right now.";
  string sorry3="I'm sorry, but you are not allowed to play right now.";
  string sorryn="Please try again some other time.  Thank you.";
  
  glname( name );
  if ( ! gunum( unum, name ) )
    {
      /* Must be a new player. */
      cdclear();
      cdredo();
      if ( *closed )
	{
	  /* Can't enroll if the game is closed. */
	  cdputc( sorry1, MSG_LIN2/2 );
	  cdputc( sorryn, MSG_LIN2/2+1 );
	  cdmove( 1, 1 );
	  cdrefresh( FALSE );
	  c_sleep( 2.0 );
	  return ( FALSE );
	}
      team = rndint( 0, NUMTEAMS - 1 );
      cbuf[0] = EOS;
      apptitle( team, cbuf );
      appchr( ' ', cbuf );
      i = strlen( cbuf );
      appstr( name, cbuf );
      cbuf[i] = cupper( cbuf[i] );
      if ( ! c_register( name, cbuf, team, unum ) )
	{
	  cdputc( sorry2, MSG_LIN2/2 );
	  cdputc( sorryn, MSG_LIN2/2+1 );
	  cdmove( 1, 1 );
	  cdrefresh( FALSE );
	  c_sleep( 2.0 );
	  return ( FALSE );
	}
      gretds();
      c_strcpy( "You have been selected to command a", cbuf );
      if ( vowel( tname[team][0] ) )
	appchr( 'n', cbuf );
      appchr( ' ', cbuf );
      appstr( tname[team], cbuf );
      appstr( " starship.", cbuf );
      cdputc( cbuf, MSG_LIN2/2 );
      cdputc( "Prepare to be beamed aboard...", MSG_LIN2/2+1 );
      cdmove( 1, 1 );
      cdrefresh( FALSE );
      c_sleep( 3.0 );
    }
  
  /* Must be special to play when closed. */
  if ( *closed && ! uooption[*unum][OOPT_PLAYWHENCLOSED] )
    {
      cdclear();
      cdredo();
      cdputc( sorry1, MSG_LIN2/2 );
      cdputc( sorryn, MSG_LIN2/2+1 );
      cdmove( 1, 1 );
      cdrefresh( FALSE );
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  /* Can't play if on the shit list. */
  if ( uooption[*unum][OOPT_SHITLIST] )
    {
      cdclear();
      cdredo();
      cdputc( sorry3, MSG_LIN2/2 );
      cdputc( sorryn, MSG_LIN2/2+1 );
      cdmove( 1, 1 );
      cdrefresh( FALSE );
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  /* Can't play without a ship. */
  if ( ! findship( &csnum ) )
    {
      cdclear();
      cdredo();
      cdputc( "I'm sorry, but there are no ships available right now.",
	     MSG_LIN2/2 );
      cdputc( sorryn, MSG_LIN2/2+1 );
      cdmove( 1, 1 );
      cdrefresh( FALSE );
      c_sleep( 2.0 );
      return ( FALSE );
    }
  
  return ( TRUE );
  
}


