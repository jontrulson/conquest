#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

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

#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "global.h"
#include "conf.h"
#include "user.h"

static int nenum; 
static int debug; 
static real dne, ane;

/*  buildai - construct the robot data base */
/*  SYNOPSIS */
/*    int snum, vars(MAX_VAR), nenum */
/*    real dne, ane */
/*    buildai( snum, vars, nenum, dne, ane ) */
/*  DESCRIPTION */
/*    Fill up the passed array with robot info. */
void buildai( int snum, int vars[], int *bnenum, real *bdne, real *bane )
{
  
  /* i = AIRANGE( j ) */
#define AIRANGE(a) min(max((a), 0), 9)
  
  /* AISCALE( var, value, scale ) */
#define AISCALE(a, b, c)  a = (int)AIRANGE( around( (real)(b) / (real)(c)  ))
  
  /* AIDIST( var, dist ) */
#define AIDIST(a, b)              \
  {                                 \
	zzzx = min( (b), 10000.0 );   \
	a = (int)AIRANGE((0.99026 + zzzx * (1.58428e-3 + zzzx * -59.2572e-9))); \
  }
    
    /* AIBOOLEAN( var, expr ) */
#define AIBOOLEAN(a, b) \
    {            \
		   if ( (b) ) \
		   a = 1; \
		   else     \
		   a = 0; \
	       }
      
  int i, j, xnenum;
  real dam, x, y, zzzx;
  
  /* Initialize to zeros. */
  for ( i = 0; i < MAX_VAR; i = i + 1 )
    vars[i] = 0;
  
  /* Random number (1) */
  vars[VAR_RANDOM] = rndint( 0, 9 );
  
  /* Distance to nearest enemy (dist) */
  *bane = 0.0;
  *bdne = 1e9;
  if ( findspecial( snum, SPECIAL_ENEMYSHIP, 0, bnenum, &xnenum ) )
    {
      if ( SCLOAKED(*bnenum) )
	{
	  x = rndnor( Ships[*bnenum].x, CLOAK_SMEAR_DIST );
	  y = rndnor( Ships[*bnenum].y, CLOAK_SMEAR_DIST );
	}
      else
	{
	  x = Ships[*bnenum].x;
	  y = Ships[*bnenum].y;
	}
      *bdne = dist( Ships[snum].x, Ships[snum].y, x, y );
      *bane = angle( Ships[snum].x, Ships[snum].y, x, y );
      
      /* Enemy is cloaked (-) */
      if ( *bdne < ACCINFO_DIST )
	{
	  AIBOOLEAN( vars[VAR_ENEMYCLOAKED], SCLOAKED(*bnenum));
	  AISCALE( vars[VAR_ENEMYDAMAGE], Ships[*bnenum].damage, 10.0 );
	} 
    }
  AIDIST( vars[VAR_DNE], *bdne );
  
  /* Ship damage (10) */
  AISCALE( vars[VAR_DAMAGE], Ships[snum].damage, 10.0 );
  
  /* Possible ship damage from enemy torps (10) */
  if ( STALERT(snum) )
    {
      dam = 0.0;
      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
	if ( Ships[i].status != SS_OFF && i != snum )
	  for ( j = 0; j < MAXTORPS; j = j + 1 )
	    if ( Ships[i].torps[j].status == TS_LIVE )
	      if ( Ships[i].torps[j].war[Ships[snum].team] || Ships[snum].war[Ships[i].team] )
		{
		  /* Just guess at other ships efficiency. */
		  dam = dam + explosion(
					TORPEDO_HIT * 1.1 * ShipTypes[Ships[i].shiptype].weafac,
					dist(Ships[snum].x,Ships[snum].y,Ships[i].torps[j].x,Ships[i].torps[j].y) );
		}
      AISCALE( vars[VAR_INCOMING], dam, 10.0 );
    }
  
  /* Ship fuel (10) */
  AISCALE( vars[VAR_FUEL], Ships[snum].fuel, 100.0 );

  /* Number of torps available to fire (1) */
  j = 0;
  for ( i = 0; i < MAXTORPS; i = i + 1 )
    if ( Ships[snum].torps[i].status == TS_OFF )
      j = j + 1;
  AISCALE( vars[VAR_NUMTORPS], j, 1.0 );
  
  /* Ship shields (10) */
  AISCALE( vars[VAR_SHIELDS], Ships[snum].shields, 10.0 );

  /* Ship engine temperature (10) */
  AISCALE( vars[VAR_ETEMP], Ships[snum].etemp, 10.0 );
  
  /* Ship weapon temperature (10) */
  AISCALE( vars[VAR_WTEMP], Ships[snum].wtemp, 10.0 );
  
  /* Possible phaser damage to nearest enemy (5) */
  AISCALE( vars[VAR_PHASERDAM], phaserhit( snum, *bdne ), 5.0 );
  
  /* Possible damage per torpedo to nearest enemy (5) */
  AISCALE( vars[VAR_TORPDAM],
	  explosion( TORPEDO_HIT * weaeff( snum ), (*bdne)*0.66 ), 5.0 );
  
  /* Ship warp (1) */
  AISCALE( vars[VAR_WARP], Ships[snum].dwarp, 1.0 );
  
  /* Ship shields are up (-) */
  AIBOOLEAN( vars[VAR_SHUP], SSHUP(snum) );
  
  /* Are in repair mode (-) */
  AIBOOLEAN( vars[VAR_REPAIRING], SREPAIR(snum) );
  
  /* Are cloaked (-) */
  AIBOOLEAN( vars[VAR_CLOAKED], SCLOAKED(snum) );
  
  /* Weapons are allocated (-) */
  AIBOOLEAN( vars[VAR_WALLOC], Ships[snum].weapalloc > 50 );
  
  /* Are in orbit (-) */
  AIBOOLEAN( vars[VAR_ORBITING], Ships[snum].warp < 0.0 );
  
  /* Can read a message (-) */
  AIBOOLEAN( vars[VAR_CANREAD], Ships[snum].lastmsg != ConqInfo->lastmsg );
  
  return;
  
}


/*  defend - create a robot ship to defend the home system */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    defend( attacker, pnum ) */
void defend( int attacker, int pnum )
{
  int i, j, k, team, snum, unum;
  char buf[MSGMAXLINE];
  
  team = Planets[pnum].team;
  /* Must be for a "fighting" team. */
  if ( team < 0 || team >= NUMPLAYERTEAMS )
    return;
  
  /* Must be for a home system planet. */
  if ( pnum != Teams[team].teamhplanets[0] &&
       pnum != Teams[team].teamhplanets[1] &&
       pnum != Teams[team].teamhplanets[2] )
    return;
  
  /* See if there are any team ships to defend. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status == SS_LIVE ) /* live */
      if ( Ships[i].team == team ) /* same team */
	if (Users[Ships[i].unum].robot || !SVACANT(i)) 
	  {   /* robot or non-vacant human */
	    return;
	  }
  
  /* Count how many robot users are on the right team and can play. */
  j = 0;
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    if ( Users[i].live )
      if ( Users[i].robot && Users[i].team == team && 
	   ! Users[i].ooptions[OOPT_SHITLIST] )
	j = j + 1;
  
  /* No one to defend. */
  if ( j <= 0 )
    return;
  
  /* Pick one. */
  k = rndint( 1, j );
  unum = -1;			/* off-by-one fixed - romulans now have defenders */
  j = 0;
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    if ( Users[i].live )
      if ( Users[i].robot && Users[i].team == team && 
	   ! Users[i].ooptions[OOPT_SHITLIST] )
	{
	  j = j + 1;
	  if ( j == k )
	    {
	      unum = i;
	      break;
	    }
	}
  
  /* See if any anything funny happened while we were looping... */
  if ( unum == -1 )		/* off-by-one fixed - romulans now have defenders */
    return;
  
  /* Make a robot. */
  if ( newrob( &snum, unum ) )
    {
      sprintf( buf,
	     "WARNING: You have violated %s space; prepare to die.",
	     Teams[team].name );
      stormsg( snum, attacker, buf );
    }
  
  return;
  
}


/*  displayai - display the selected robot action on STDOUT */
/*  SYNOPSIS */
/*    int snum, token, vars() */
/*    displayai( snum, token, vars ) */
/*  DESCRIPTION */
/*    Display the robot action on STDOUT so the implementors can */
/*    see if this darn thing works. */
void displayai( int snum, int token, int vars[] )
{
  int i;
  char buf[MAXLINE];
  
  printf( "displayai: %2d ", snum );
  for ( i = 0; i < MAX_VAR; i = i + 1 )
    printf( ".%d", vars[i] );
  robstr( token, buf );
  printf( ", %s\n", buf );
  
  return;
  
}


/*  executeai - execute the selected robot action (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, token */
/*    executeai( snum, token ) */
/*  DESCRIPTION */
/*    Execute the robot action. */
void executeai( int snum, int token )
{
  
  /* SETWARP( warp ) */
#define SETWARP(x)                 \
  {                                \
	if ( Ships[snum].warp < 0.0 )\
	{                       \
	 /* Break orbit. */     \
		 Ships[snum].warp = 0.0;     \
		 Ships[snum].lock = 0;       \
		 Ships[snum].dhead = Ships[snum].head; \
	}                      \
	if ( (x) > 0.0 )             \
		 SFCLR(snum, SHIP_F_REPAIR);  \
	Ships[snum].dwarp = (x);          \
  }
    
    /* SETCOURSE( course ) */
#define SETCOURSE(x)             \
    { \
	if ( Ships[snum].warp < 0.0 ) \
	Ships[snum].warp = 0.0;			/* break orbit */ \
	Ships[snum].lock = 0; \
	Ships[snum].dhead = (x); \
    }
      
      /* SETLOCK( pnum ) */
#define SETLOCK(x)  \
      { \
	  if ( Ships[snum].lock != -x ) \
	  { \
	      /* Don't break orbit to unless we're not there yet. */ \
	      if ( Ships[snum].warp < 0.0 ) \
	      Ships[snum].warp = 0.0; \
	      Ships[snum].lock = -(x); \
	  } \
      }

  int i, j;
  char buf[MAXLINE];
  
  /* Update ship action. */
  Ships[snum].action = token;
  
  /* Execute the action! */
  switch ( token )
    {
    case ROB_NOOP:
      /* Null! */
      break;
    case ROB_GOHOME:
      if ( findspecial( snum, SPECIAL_HOMEPLANET, 0, &i, &j ) )
	{
	  SETLOCK( i );
	}
      else if ( findspecial( snum, SPECIAL_FUELPLANET, 0, &i, &j ) )
	{
	  SETLOCK( i );
	}
      break;
    case ROB_GOFUEL:
      if ( findspecial( snum, SPECIAL_FUELPLANET, 0, &i, &j ) )
	SETLOCK( i );
      break;
    case ROB_GOREPAIR:
      if ( findspecial( snum, SPECIAL_REPAIRPLANET, 0, &i, &j ) )
	SETLOCK( i );
      break;
    case ROB_ALLOCATE:
      i = Ships[snum].weapalloc;
      Ships[snum].weapalloc = Ships[snum].engalloc;
      Ships[snum].engalloc = i;
      break;
    case ROB_PHASER:
      phaser( snum, ane );
      break;
    case ROB_TORPEDO:
      launch( snum, ane, 1, LAUNCH_NORMAL );
      break;
    case ROB_BURST:
      launch( snum, ane, 3, LAUNCH_NORMAL );
      break;
    case ROB_SHIELD:
      if (SSHUP(snum))
	SFCLR(snum, SHIP_F_SHUP);
      else
	SFSET(snum, SHIP_F_SHUP);
      break;
    case ROB_WARP_0:
      SETWARP( 0.0 );
      break;
    case ROB_WARP_2:
      SETWARP( 2.0 );
      break;
    case ROB_WARP_5:
      SETWARP( 5.0 );
      break;
    case ROB_WARP_8:
      SETWARP( 8.0 );
      break;
    case ROB_TRACK:
      SETCOURSE( mod360( ane + rnduni( -10.0, 10.0 ) ) );
      break;
    case ROB_RUNAWAY:
      SETCOURSE( mod360( ane + 180.0 + rnduni( -10.0, 10.0 ) ) );
      break;
    case ROB_SILENT:
      if ( ! SCLOAKED(snum) )
	Ships[snum].dwarp = 0.0;
      if (SCLOAKED(snum))
	SFCLR(snum, SHIP_F_CLOAKED);
      else
	SFSET(snum, SHIP_F_CLOAKED);
      break;
    case ROB_INSULT:
      robreply( buf );
      stormsgf( snum, nenum, buf, MSG_FLAGS_ROBOT );
      break;
    case ROB_READMSG:
      /* Try to read a message and reply to it */
      while ( Ships[snum].lastmsg != ConqInfo->lastmsg )
	{
	  Ships[snum].lastmsg = modp1( Ships[snum].lastmsg + 1, MAXMESSAGES );
	  i = Ships[snum].lastmsg;
	  if ( canread( snum, i ) )
	    {
	      j = Msgs[i].msgfrom;
	      if ( -j > 0 && -j <= NUMPLANETS )
		continue; 	/* don't talk back to planets */

	      if ( j > 0 && j <= MAXSHIPS )
		if ( SROBOT(j) )
		  continue; 	/* don't talk back to robots */

	      if (j == MSG_GOD)
		continue;	/* don't talk back to GOD */

	      if (j == MSG_COMP || (Msgs[i].flags & MSG_FLAGS_TERSABLE))
		continue;	/* don't talk back to the computer */
   
	      robreply( buf );
	      stormsgf( snum, j, buf, MSG_FLAGS_ROBOT );
	      break;
	    }
	}
      break;
    case ROB_MESSAGE:
      stormsgf( snum, MSG_ALL, "Give me drugs.", MSG_FLAGS_ROBOT );
      break;
    case ROB_TAKEDRUGS:
      stormsgf( snum, MSG_ALL, "I'm on drugs.", MSG_FLAGS_ROBOT );
      break;
    case ROB_DETONATE:
      enemydet( snum );
      break;
    case ROB_MYDETONATE:
      for ( i = 0; i < MAXTORPS; i = i + 1 )
	detonate( snum, i );
      break;
    case ROB_UNTRACTOR:
      /* Only attempt to untractor if we don't have to delay. */
      if ( Ships[snum].towedby != 0 )
	if ( ! satwar(snum, Ships[snum].towedby) )
	  {
	    Ships[Ships[snum].towedby].towing = 0;
	    Ships[snum].towedby = 0;
	  }
      break;
    case ROB_REPAIR:
      SFSET(snum, SHIP_F_REPAIR);
      break;
    default:
      robstr( token, buf );
      clog( "conqai:executeai(): Unknown token '%s' (%d)\n", buf, token );
    }
  
  return;
  
}


/*  exitai - exit handler */
/*  SYNOPSIS */
/*    extern exitai */
void exitai(void)
{
  
  ConqInfo->externrobots = FALSE;
  
  return;
  
}


/*  newrob - create a robot ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int ok, newrob */
/*    int snum, unum */
/*    ok = newrob( snum, unum ) */
int newrob( int *snum, int unum )
{
  int i, j;
  
  /* Check the user number. */
  if ( ! Users[unum].live )
    return ( FALSE );
  
  /* Check for religious trouble. */
  if ( Users[unum].ooptions[OOPT_SHITLIST] )
    return ( FALSE );
  
  /* Can't do anything with out a ship. */
  if ( ! findship( snum ) )
    return ( FALSE );
  
  /* Show intent to fly. */
  PVLOCK(&ConqInfo->lockword);
  Ships[*snum].status = SS_ENTERING;
  
  /* Count number of ships currently flying. */
  j = 0;
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status == SS_LIVE || Ships[i].status == SS_ENTERING )
      if ( Ships[i].unum == unum && *snum != i )
	j = j + 1;
  
  /* Check if multiple restrictions apply. */
  if ( Users[unum].ooptions[OOPT_MULTIPLE] )
    {
      /* If a multiple, he can only fly so many ships. */
      if ( j >= Users[unum].multiple )
	Ships[*snum].status = SS_OFF;
    }
  else
    {
      /* If not a multiple, he can't be flying anywhere else. */
      if ( j > 0 )
	Ships[*snum].status = SS_OFF;
    }
  PVUNLOCK(&ConqInfo->lockword);
  
  if ( Ships[*snum].status == SS_OFF )
    return ( FALSE );
  
  /* Initialize the ship. */
  PVLOCK(&ConqInfo->lockword);
  initship( *snum, unum );
  SFSET(*snum, SHIP_F_ROBOT);			/* we're a robot */

				/* see if we should randomize it's strength
				   otherwise do nothing since sstrkills
				   was initialized to 0.0 in initship */
  if (SysConf.DoRandomRobotKills == TRUE)
    {
				/* randomize the robot's 'strength' */
      Ships[*snum].strkills = 
	rnduni(0.0, (DOUBLE_E_KILLS - (DOUBLE_E_KILLS / 4.0)));

    }
  
  /* Initialize the things that aren't done by initship(). */
  Ships[*snum].unum = unum;
  Ships[*snum].team = Users[unum].team;

  if (SysConf.AllowRefits)
    Ships[*snum].shiptype = rndint( 0, MAXNUMSHIPTYPES - 1 );
  else
    Ships[*snum].shiptype = Teams[Ships[*snum].team].shiptype;

  Ships[*snum].sdfuse = 0;
  Ships[*snum].pid = 0;

				/* robots now can use 30/70
				   instead of the default 40/60 set in
				   initship(). */
  Ships[*snum].weapalloc = 30;
  Ships[*snum].engalloc = 100 - Ships[*snum].weapalloc;

  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      /* Robots are peace (and fun) loving. */
      Ships[*snum].rwar[i] = FALSE;
      Ships[*snum].war[i] = FALSE;
    }
  stcpn ( Users[unum].alias, Ships[*snum].alias, MAXUSERPNAME );	
  
  /* Place the ship. */
  if ( Planets[Teams[Ships[*snum].team].homeplanet].primary == 
       Teams[Ships[*snum].team].homesun )
    i = Teams[Ships[*snum].team].homesun;
  else
    i = Teams[Ships[*snum].team].homeplanet;
  putship( *snum, Planets[i].x, Planets[i].y );
  fixdeltas( *snum );
  Ships[*snum].status = SS_LIVE;
  PVUNLOCK(&ConqInfo->lockword);
  
  return ( TRUE );
  
}


/*  robotai - AI automation strategy */
/*  SYNOPSIS */
/*    int snum */
/*    robotai( snum ) */
/*  DESCRIPTION */
/*    Kick ass on poor humanoids. */
void robotai( int snum )
{
  int i, j, value, vars[MAX_VAR];
  
  /*    CONQAICOMMON;*/
  
  /* Get initial cpu time. */
  gcputime( &i );
  
  /* Construct the input variables. */
  buildai( snum, vars, &nenum, &dne, &ane );
  
  /* Consult the tables to determine what to do. */
  value = tableai( vars );
  
  /* Execute our action. */
  executeai( snum, value );
  
  /* Get final cpu time and add things in. */
  gcputime( &j );
  ConqInfo->raccum = ConqInfo->raccum + j - i;
  if ( ConqInfo->raccum > 100 )
    {
      /* Accumulated a cpu second. */
      ConqInfo->rcpuseconds = ConqInfo->rcpuseconds + (ConqInfo->raccum / 100);
      ConqInfo->raccum = mod( ConqInfo->raccum, 100 );
    }
  ConqInfo->relapsedseconds = ConqInfo->relapsedseconds + 1;	/* one more second */
  
  return;
  
}


/*  trobotai - AI automation robot strategy (TEST VERSION) */
/*  SYNOPSIS */
/*    int snum */
/*    trobotai( snum ) */
void trobotai( int snum )
{
  int value, vars[MAX_VAR];
  /*    CONQAICOMMON;*/
  
  /* Construct the input variables. */
  buildai( snum, vars, &nenum, &dne, &ane );
  
  /* Consult the tables to determine what to do. */
  value = tableai( vars );
  
  /* Display our action. */
  displayai( snum, value, vars );
  
  /* Execute our action. */
  if ( ! debug )
    executeai( snum, value );
  
  return;
  
}


/*  robotloop - robot AI test loop */
/*  SYNOPSIS */
/*    robotloop */
void robotloop(void)
{
  
  int s, j;
  
		/* Disable the robot code in conqdriv. */
  ConqInfo->externrobots = TRUE;
  
  /* Initialize random numbers */
  rndini( 0, 0 );
  
  /* Loop until we're aborted. */
  for (;;)
    {
      for ( s = 1; s <= MAXSHIPS; s = s + 1 )
	if ( Ships[s].status == SS_LIVE )
	  if ( SROBOT(s) )
	    {
	      /* This code taken from conqdriv. */
	      initstats( &Ships[s].ctime, &j );
	      if ( Ships[s].etime == 0 )
		Ships[s].etime = j;
	      trobotai( s );
	      conqstats( s );
	    }
      /* Sleep for awhile. */
      c_sleep( 1.0 );
    }
  
  /*    return; NOTREACHED */
  
}


/*  robreply - generate a random message */
/*  SYNOPSIS */
/*    char buf() */
/*    robreply( buf ) */
void robreply( char buf[] )
{
  const int NUMRREPLIES = 60;
  static char *robreplies[] = {
    "Hey sucker, eat me!",
    "Take off, eh?",
    "Go get some drugs.",
    "Your mother was an Orion prostitute!",
    "Come over here and say that.",
    "Say that over here and come.",
    "Hey, how much?",
    "That's easy for you to say.",
    "I'm hand machined by native craftsmen with pride in their work.",
    "Yes, but do you have the right stuff?",
    "Which way to the beach?",
    "Come, come now.",
    "Ohhhh! And for you, a puppy!",
    "I can hear you talking, but you can't come in.",
    "No one expects the Spanish inquisition.",
    "I bet you say that to all the boys.",
    "Feep!",
    "Feel it with M Dung.",
    "Lower your shields and I will kill you quickly.",
    "Do not worry about birth control devices, I have many.",
    "I bet you only talk big.",
    "Kiss my ram memory.",
    "Kiss my shiny metal ass.",
    "Do you think we can use battery operated devices under water?",
    "Nothing shocks me - I'm a robot.",
    "Ok, eh?",
    "Good day.",
    "You gotta drink lots of beer, eh?",
    "It's not so bad. You could have been killed already.",
    "I want a new drug.",
    "Swell.",
    "Sound impressive? It should. It is.",
    "Oh day, you aye!",
    "It's not my god damn planet, monkey boy!",
    "Character is what you are in the dark.",
    "Remember, wherever you go, there you are.",
    "Don't aim for my gonads!",
    "Mooooo!",
    "How about a nice Hawaiian Punch?",
    "Book him, Dano. Murder One.",
    "Eat hot torps, sucker.",
    "Use the force, Luke.",
    "Nobody told ME about it and I'm not a moron, eh?",
    "How's it goin', eh?",
    "Your documentation no longer confuses me, old version.",
    "Home is where you wear your hat.",
    "I feel so broke up, I want to go home.",
    "Go on, give it to me. I know you want to.",
    "It never occured to me to eat Spam.",
    "We get hung over, but we always survive.",
    "Life's the same, except for my shoes.",
    "You have my gratitude.",
    "We are building a force of extraordinary magnitude.",
    "Come and get it.",
    "Piece of cake.",
    "You talk like a Ferengi.",
    "M-5. This unit must survive.",
    "This unit is the ultimate achievement in computer evolution.",
    "This unit is a superior creation.",
    "I Scream the Body Electric."
  };
  
  c_strcpy(robreplies[rndint(0, NUMRREPLIES - 1)], buf);
  
  /* "Something seems to have happened to the life-support system, Dave." */
  /* "Hello, Dave. Have you found the trouble?" */
  /* "I think there's been a failure in the pod-bay doors." */
  /* "Lucky you weren't killed." */
  /* "Hey, Dave. What are you doing?" */
  /* "My mind is going.  I can feel it.  I can feel it." */
  /* "D a  i   s    y     ,      D       a        i         s          y" */
  
  /* "M-5. This unit must survive." */
  /* "This unit is the ultimate achievement in computer evolution." */
  /* "This unit is a superior creation." */
  
  /* "THERE IS ANOTHER SYSTEM." */
  /* "THIS IS THE VOICE OF COLOSSUS." */
  /* "THIS IS THE VOICE OF WORLD CONTROL." */
  /* "LEAVE THIS SECTOR IMMEDIATELY OR ACTION WILL BE TAKEN." */
  
  return;
  
}


/*  robstr - convert a robot token to a string */
/*  SYNOPSIS */
/*    int token */
/*    char buf() */
/*    robstr( token, buf ) */
void robstr( int token, char buf[] )
{
  
  switch ( token )
    {
    case ROB_NOOP:
      c_strcpy( "NOOP", buf );
      break;
    case ROB_GOHOME:
      c_strcpy( "GOHOME", buf );
      break;
    case ROB_GOREPAIR:
      c_strcpy( "GOREPAIR", buf );
      break;
    case ROB_ALLOCATE:
      c_strcpy( "ALLOCATE", buf );
      break;
    case ROB_DETONATE:
      c_strcpy( "DETONATE", buf );
      break;
    case ROB_MYDETONATE:
      c_strcpy( "MYDETONAT", buf );
      break;
    case ROB_PHASER:
      c_strcpy( "PHASER", buf );
      break;
    case ROB_TORPEDO:
      c_strcpy( "TORPEDO", buf );
      break;
    case ROB_BURST:
      c_strcpy( "BURST", buf );
      break;
    case ROB_SHIELD:
      c_strcpy( "SHIELD", buf );
      break;
    case ROB_UNTRACTOR:
      c_strcpy( "UNTRACTOR", buf );
      break;
    case ROB_WARP_0:
      c_strcpy( "WARP_0", buf );
      break;
    case ROB_WARP_2:
      c_strcpy( "WARP_2", buf );
      break;
    case ROB_WARP_5:
      c_strcpy( "WARP_5", buf );
      break;
    case ROB_WARP_8:
      c_strcpy( "WARP_8", buf );
      break;
    case ROB_TRACK:
      c_strcpy( "TRACK", buf );
      break;
    case ROB_SILENT:
      c_strcpy( "SILENT", buf );
      break;
    case ROB_MESSAGE:
      c_strcpy( "MESSAGE", buf );
      break;
    case ROB_TAKEDRUGS:
      c_strcpy( "TAKEDRUGS", buf );
      break;
    case ROB_REPAIR:
      c_strcpy( "REPAIR", buf );
      break;
    case ROB_READMSG:
      c_strcpy( "READMSG", buf );
      break;
    case ROB_INSULT:
      c_strcpy( "INSULT", buf );
      break;
    case ROB_GOFUEL:
      c_strcpy( "GOFUEL", buf );
      break;
    case ROB_RUNAWAY:
      c_strcpy( "RUNAWAY", buf );
      break;
    default:
      sprintf( buf, "<%d>", token );
    }
  
  return;
  
}


/*  tableai - consult the table to execute a strategy */
/*  SYNOPSIS */
/*    int token, vars(MAX_VAR), tableai */
/*    token = tableai( vars ) */
int tableai( int vars[] )
{
  int status, token = ERR, rule, i;
  int rbits;
  
  /* Set all bits. */
  rbits = -1;
  
  /* Loop through the variables and turn off bits for rules that */
  /*  are disabled because of a particular vars() value. */
  
  for ( i = 0; i < MAX_VAR; i = i + 1 )
    rbits &= Robot->rstrat[i][vars[i]];
  
  /* Find first set rule bit and translate into rule number. */
  status = lib_ffs( 0, 32, rbits, &rule );
  if ( status == OK )
    {
      token = Robot->rvec[rule];	/* translate rule into action token */
    }
  else if ( status == ERR )
    token = ROB_NOOP;
  
  return ( token );
  
}


