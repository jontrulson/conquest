#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/*                               C O N Q M I S C */
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
#include "conqcom2.h"

#define NOCDHEXTERN
#include "conqdata.h"				/* robot strategy data */

#include "global.h"
#include "color.h"

/*  appkb - append killed by string */
/*  SYNOPSIS */
/*    int kb */
/*    char buf() */
/*    appkb( kb, buf ) */
void appkb( int kb, char *buf )
{
  
  switch ( kb )
    {
    case KB_SELF:
      appstr( "self", buf );
      break;
    case KB_NEGENB:
      appstr( "negenb", buf );
      break;
    case KB_CONQUER:
      appstr( "conquer", buf );
      break;
    case KB_NEWGAME:
      appstr( "newgame", buf );
      break;
    case KB_EVICT:
      appstr( "evict", buf );
      break;
    case KB_SHIT:
      appstr( "shit", buf );
      break;
    case KB_DOOMSDAY:
      appstr( "doomsday", buf );
      break;
    case KB_GOTDOOMSDAY:
      appstr( "gotdoomsday", buf );
      break;
    case KB_GOD:
      appstr( "GOD", buf );
      break;
    default:
      if ( kb > 0 && kb <= MAXSHIPS )
	appship( kb, buf );
      else if ( -kb > 0 && -kb <= NUMPLANETS )
	appstr( Planets[-kb].name, buf );
      else
	appint( kb, buf );
      break;
    }
  
  return;
  
}


/*  appship - append a ship number to a string */
/*  SYNOPSIS */
/*    int snum */
/*    char str() */
/*    appship( snum, str ) */
void appship( int snum, char *str )
{
  int i;
  char ch;
  
  ch = 'S';
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      i = Ships[snum].team;
      if ( i >= 0 && i < NUMPLAYERTEAMS )
	ch = Teams[i].teamchar;
    }
  
  appchr( ch, str );
  appint( snum, str );
  
  return;
}


/*  canread - determine if a message is readable */
/*  SYNOPSIS */
/*    int ok, canread */
/*    int snum, msgnum */
/*    ok = canread( snum, msgnum ) */
int canread( int snum, int msgnum )
{
  int from, to;
  
  
  from = Msgs[msgnum].msgfrom;
  to = Msgs[msgnum].msgto;
  
  if (from == 0 && to == 0)
    {				/* uninitialized msgs */
      return(FALSE);		/* no point in reading it */
    }

  /* If we're GOD, we can read it. unless it's a COMP MSG*/
  if ( snum == MSG_GOD && from != MSG_COMP)
    return ( TRUE );
  
  /* It's to us. */
  if ( to == snum )
    {				/* extra check to see if is from a robot
				   and to is a valid ship */
      if (conf_NoRobotMsgs == TRUE && from > 0 &&
	  Ships[from].robot == TRUE && 
	  (snum > 0 && snum <= MAXSHIPS))
	{                       /* see if it's a robot, if so ignore */
          return(FALSE);
	}
      else
	{
	  return ( TRUE );
	}
    }

  /* It's from us */
  if (from == snum)
    return(TRUE);
  
  /* It's to everybody. */
  if ( to == MSG_ALL )
    {				/* extra check for player enter/leave msg */
      if (from == MSG_COMP && snum != MSG_GOD)
	{				/* a player enter/exit/info message */
	  if (conf_RecPlayerMsgs == FALSE)
	    return(FALSE);
	  else
	    return(TRUE);
	}
				/* else, we can read it */
      return(TRUE);
    }
  
  /* Only check these if we're a ship. */
  if ( snum > 0 && snum <= MAXSHIPS )
    {
				/* if user doesn't want robot msgs
				   don't show any */
      if (conf_NoRobotMsgs == TRUE && from > 0 && 
	  Ships[from].robot == TRUE)
	{			/* see if it's a robot, if so ignore */
	  return(FALSE);
	}
      
      /* We can only read team messages if we're not self-war. */
      if ( ( -to == Ships[snum].team ) && ! selfwar(snum) )
	{
	  /* Planet alert for our team. */
	  if ( -from > 0 && -from <= NUMPLANETS )
	    return ( Ships[snum].options[OPT_INTRUDERALERT] );
	  else
	    return ( TRUE );
	}
      
      /* See if we are allowed to read GOD messages. */
      if ( to == MSG_GOD || from == MSG_GOD || to == MSG_IMPLEMENTORS )
	return ( Users[Ships[snum].unum].ooptions[OOPT_GODMSG] );
    }
  
  /* If we got here, we can't read it. */
  return ( FALSE );
  
}


/*  clearships - reset ships and torpedoes */
/*  SYNOPSIS */
/*    clearships */
void clearships(void)
{
  
  int i;
  
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    zeroship( i );
  
  return;
  
}


/*  cvtcoords - convert from internal coords to screen coords */
/*  SYNOPSIS */
/*    int inbounds, cvtcoords */
/*    real cenx, ceny, x, y, scale */
/*    int lin, col */
/*    inbounds = cvtcoords( ceny, ceny, x, y, scale, lin, col ) */
int cvtcoords( real cenx, real ceny, real x, real y, real scale, 
	      int *lin, int *col )
{
  *col = round( (CqContext.maxcol-STAT_COLS)/2 + (x-cenx) / scale * WIDTH_FAC ) +
    STAT_COLS;
  
  *lin = round( (DISPLAY_LINS/2+1) - (y-ceny) / scale );
  if ( *lin < 0 || *lin > DISPLAY_LINS || *col <= STAT_COLS || *col > CqContext.maxcol )
    return ( FALSE );
  
  return ( TRUE );
  
}


/*  doomfind - find a planet or ship for the doomsday machine to head for */
/*  SYNOPSIS */
/*    doomfind */
void doomfind(void)
{
  
  int i;
  real taste, tastiness;
  
  tastiness = 0.0;
  Doomsday->lock = -PNUM_MURISAK;
  
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( Planets[i].real )
      if ( Planets[i].armies > 0 && Planets[i].team != TEAM_NOTEAM )
	{
	  taste = Planets[i].armies * BOMBARD_KILLS / dist(Doomsday->x, Doomsday->y, Planets[i].x, Planets[i].y);
	  if ( taste > tastiness )
	    {
	      tastiness = taste;
	      Doomsday->lock = -i;
	    }
	}
  
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status == SS_LIVE )
      {
	taste = ( 1.0 +
		 Ships[i].kills * KILLS_KILLS +
		 Ships[i].armies * ARMY_KILLS ) / dist(Doomsday->x, Doomsday->y, Ships[i].x, Ships[i].y);
	if ( taste > tastiness )
	  {
	    tastiness = taste;
	    Doomsday->lock = i;
	  }
      }
  
  if ( Doomsday->lock < 0 )
    Doomsday->heading = angle( Doomsday->x, Doomsday->y, Planets[-Doomsday->lock].x, Planets[-Doomsday->lock].y );
  else if ( Doomsday->lock > 0 )
    Doomsday->heading = angle( Doomsday->x, Doomsday->y, Ships[Doomsday->lock].x, Ships[Doomsday->lock].y );
  
  return;
  
}


/*  doomsday - start the doomsday device */
/*  SYNOPSIS */
/*    doomsday */
void doomsday(void)
{
  
  Doomsday->heading = rnduni( 0.0, 360.0 );
  Doomsday->x = DOOMSDAY_START_DIST * cosd(Doomsday->heading);
  Doomsday->y = DOOMSDAY_START_DIST * sind(Doomsday->heading);
  doomfind();
  Doomsday->status = DS_LIVE;
  
  return;
  
}


/*  findorbit - find a planet for a ship to orbit */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    int flag, findorbit */
/*    flag = findorbit( snum, pnum ) */
int findorbit( int snum, int *pnum )
{
  int i;
  
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( Planets[i].real &&
	( dist( Ships[snum].x, Ships[snum].y, Planets[i].x, Planets[i].y ) <= ORBIT_DIST ) )
      {
	*pnum = i;
	return ( TRUE );
      }
  /* Didn't find one. */
  /*    *pnum = 0;*/
  return ( FALSE );
  
}


/*  findship - find a free ship and reserve it (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    int truth, findship */
/*    truth = findship( snum ) */
int findship( int *snum )
{
  int i;
  PVLOCK(&ConqInfo->lockword);
  *snum = -1;
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status == SS_OFF )
      {
	*snum = i;
	zeroship( *snum );
	Ships[*snum].status = SS_RESERVED;
	Ships[*snum].lastmsg = LMSG_NEEDINIT;
	Ships[*snum].sdfuse = -TIMEOUT_PLAYER;
	Ships[*snum].ctime = 0;
	Ships[*snum].etime = 0;
	Ships[*snum].cacc = 0;
	Ships[*snum].eacc = 0;
	break;
      }
  PVUNLOCK(&ConqInfo->lockword);
  
  return ( *snum != -1 );
  
}


/*  findspecial - search for nearest some-thing */
/*  SYNOPSIS */
/*    int flag, findspecial */
/*    int snum, token, count, sorpnum, xsorpnum */
/*    flag = findspecial( snum, token, count, sorpnum, xsorpnum ) */
int findspecial( int snum, int token, int count, int *sorpnum, int *xsorpnum )
{
  int i, a, na, ta, u, nu, tu;
  real d, nd, td;
  int valid, peaceful;
  
  *sorpnum = 0;					/* zero nearest */
  *xsorpnum = 0;				/* zero second nearest */
  d = 2e20;					/* distance from nearest */
  nd = 3e20;					/* distance from second nearest */
  a = 20000;					/* armies of nearest */
  na = 30000;					/* armies of second nearest */
  u = 20000;					/* uninhabitable time of nearest */
  nu = 20000;					/* uninhabitable time of next */
  switch ( token )
    {
    case SPECIAL_SHIP:
    case SPECIAL_ENEMYSHIP:
    case SPECIAL_TEAMSHIP:
      /* Nearest ship, nearest enemy ship, and nearest team ship. */
      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
	if ( i != snum && Ships[i].status == SS_LIVE )
	  {
	    switch ( token )
	      {
	      case SPECIAL_ENEMYSHIP:
		valid = satwar(snum, i);
		break;
	      case SPECIAL_SHIP:
		valid = TRUE;
		break;
	      case SPECIAL_TEAMSHIP:
		valid = ( Ships[i].team == Ships[snum].team &&
			 ! satwar(snum, i) );
		break;
	      default:
		return ( FALSE );		/* this can't happen */
	      }
	    if ( valid )
	      {
#ifdef WARP0CLOAK
		if (Ships[i].cloaked && Ships[i].warp == 0.0 && 
		    satwar(snum, i) &&
		    Ships[snum].robot)
		  continue; /* nobody here but us chickens... */
#endif /* WARP0CLOAK */
		td = dist(Ships[snum].x, Ships[snum].y, Ships[i].x, Ships[i].y);
		if ( td < nd )
		  if ( td < d )
		    {
		      *xsorpnum = *sorpnum;
		      nd = d;
		      *sorpnum = i;
		      d = td;
		    }
		  else
		    {
		      *xsorpnum = i;
		      nd = td;
		    }
	      }
	  }
      break;
    case SPECIAL_HOMEPLANET:
      /* Home planet. */
      switch ( Ships[snum].team )
	{
	case TEAM_FEDERATION:
	  *sorpnum = Teams[TEAM_FEDERATION].homeplanet;
	  break;
	case TEAM_ROMULAN:
	  *sorpnum = Teams[TEAM_ROMULAN].homeplanet;
	  break;
	case TEAM_KLINGON:
	  *sorpnum = Teams[TEAM_KLINGON].homeplanet;
	  break;
	case TEAM_ORION:
	  *sorpnum = Teams[TEAM_ORION].homeplanet;
	  break;
	default:
	  return ( FALSE );
	}
      break;
    case SPECIAL_WEAKPLANET:
      /* Weakest non-team planet. */
      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
	{
	  /* Only can look for "real" planets. */
	  if ( ! Planets[i].real )
	    continue; /* jet next;*/
	  /* Ignore suns and moons. */
	  if ( Planets[i].type == PLANET_SUN || Planets[i].type == PLANET_MOON )
	    continue; 

	  valid = ( Planets[i].scanned[Ships[snum].team] &&
		    Planets[i].team != Ships[snum].team );

	  /* Handle army threshold logic. */
	  if ( valid )
	    valid = ( Planets[i].armies >= count );

	  if ( valid )
	    {
	      ta = Planets[i].armies;
	      tu = Planets[i].uninhabtime;
	      td = dist(Ships[snum].x, Ships[snum].y, Planets[i].x, Planets[i].y);
	      
	      /* Uninhabitable time is of next importance, */
	      /*  number of armies is of first importantance, and */
	      /*  distance is of last importance. */
	      if ( tu < nu ||
		  ( tu == nu && ( ta < na || ( ta == na && td < nd ) ) ) )
		if ( tu < u ||
		    ( tu == u && ( ta < a || ( ta == a && td < d ) ) ) )
		  {
		    *xsorpnum = *sorpnum;
		    na = a;
		    nu = u;
		    nd = d;
		    *sorpnum = i;
		    a = ta;
		    u = tu;
		    d = td;
		  }
		else
		  {
		    *xsorpnum = i;
		    na = ta;
		    nu = tu;
		    nd = td;
		  }
	    }
	}
      break;
    case SPECIAL_ARMYPLANET:
    case SPECIAL_ENEMYPLANET:
    case SPECIAL_FUELPLANET:
    case SPECIAL_PLANET:
    case SPECIAL_REPAIRPLANET:
    case SPECIAL_TEAMPLANET:
      
      /* Determine if we at peace with all teams. */
      peaceful = TRUE;
      for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	if ( Ships[snum].war[i] )
	  {
	    peaceful = FALSE;
	    break;
	  }
      
      /* Loop through the planets. */
      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
	{
	  /* Only can look for "real" planets. */
	  if ( ! Planets[i].real )
	    continue; 
	  /* Ignore suns and moons. */
	  if ( Planets[i].type == PLANET_SUN || Planets[i].type == PLANET_MOON )
	    continue; 
	  switch ( token )
	    {
	    case SPECIAL_ARMYPLANET:
	      valid = ( Planets[i].team == Ships[snum].team );
	      break;
	    case SPECIAL_ENEMYPLANET:
	      valid = ( ! Planets[i].scanned[Ships[snum].team] ||
		       ( Planets[i].armies > 0 &&
			spwar( snum, i ) &&
			Planets[i].type != PLANET_MOON ) );
	      break;
	    case SPECIAL_FUELPLANET:
	      valid = ( ( Planets[i].scanned[Ships[snum].team] || peaceful ) &&
		       ! spwar( snum, i ) &&
		       Planets[i].armies > 0 &&
		       Planets[i].type == PLANET_CLASSM );
	      break;
	    case SPECIAL_PLANET:
	      valid = TRUE;
	      break;
	    case SPECIAL_REPAIRPLANET:
	      valid = ( ( Planets[i].scanned[Ships[snum].team] || peaceful ) &&
		       ! spwar( snum, i ) &&
		       Planets[i].armies > 0 &&
		       Planets[i].type != PLANET_MOON );
	      break;
	    case SPECIAL_TEAMPLANET:
	      valid = ( Planets[i].team == Ships[snum].team );
	      break;
	    default:
	      return ( FALSE );		/* this can't happen */
	    }
	  /* Handle army threshold logic. */
	  if ( valid )
	    switch ( token )
	      {
	      case SPECIAL_ARMYPLANET:
		valid = ( ( Planets[i].armies - 3 ) >= count );
		break;
	      case SPECIAL_PLANET:
	      case SPECIAL_ENEMYPLANET:
		valid = ( ! Planets[i].scanned[Ships[snum].team] ||
			 Planets[i].armies >= count );
		break;
	      case SPECIAL_FUELPLANET:
	      case SPECIAL_REPAIRPLANET:
	      case SPECIAL_TEAMPLANET:
		valid = ( Planets[i].armies >= count );
		break;
	      default:
		return ( FALSE );	/* this can't happen */
	      }
	  if ( valid )
	    {
	      td = dist(Ships[snum].x, Ships[snum].y, Planets[i].x, Planets[i].y);
	      if ( td < nd )
		if ( td < d )
		  {
		    *xsorpnum = *sorpnum;
		    nd = d;
		    *sorpnum = i;
		    d = td;
		  }
		else
		  {
		    *xsorpnum = i;
		    nd = td;
		  }
	    }
	}
      break;
    default:
      return ( FALSE );			/* this can't happen */
    }
  
  return ( *sorpnum != 0 );
  
}


/*  fixdeltas - update sdx and sdy */
/*  SYNOPSIS */
/*    int snum */
/*    fixdeltas( snum ) */
void fixdeltas( int snum )
{
  real speed;
  
  speed = Ships[snum].warp * MM_PER_SEC_PER_WARP * ITER_SECONDS;
  Ships[snum].dx = speed * cosd(Ships[snum].head);
  Ships[snum].dy = speed * sind(Ships[snum].head);
  
  return;
  
}


/*  gunum - get the user number of the specified user */
/*  SYNOPSIS */
/*    int truth, gunum */
/*    int unum */
/*    char lname() */
/*    truth = gunum( unum, lname ) */
int gunum( int *unum, char *lname, int ltype )
{
  int i, chktype;
  char *lptr = lname;

  if (*lptr == '@')
    {				/* a remote user only lookup  */
      lptr++;
      chktype = UT_REMOTE;
    }
  else
    chktype = UT_LOCAL;
  
  if (ltype != -1)
    chktype = ltype;		/* provide override via option */

  *unum = -1;
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    if ( Users[i].live && Users[i].type == chktype)
      if ( strcmp( lptr, Users[i].username ) == 0 )
	{
	  *unum = i;
	  return ( TRUE );
	}
  
  return ( FALSE );
  
}


/*  histlist - display the last usage list */
/*  SYNOPSIS */
/*    int godlike */
/*    histlist( godlike ) */
void histlist( int godlike )
{
  int i, j, unum, lin, col, fline, lline, thistptr = 0;
  int ch;
  char *hd0="C O N Q U E S T   U S E R   H I S T O R Y";
  char puname[SIZEUSERNAME + 2]; /* for '\0' and '@' */
  char connecttm[BUFFER_SIZE];
  char histentrytm[DATESIZE + 1];

				/* Do some screen setup. */
  cdclear();
  fline = 1;
  lline = MSG_LIN1 - 1;
  cprintf(fline,0,ALIGN_CENTER,"#%d#%s",LabelColor, hd0);
  fline = fline + 2;
  
  thistptr = -1;		/* force an update the first time */
  while (TRUE) /* repeat */
    {
      if ( ! godlike )
	if ( ! stillalive( CqContext.snum ) )
	  break;
      
      thistptr = ConqInfo->histptr;
      lin = fline;
      col = 1;
      cdclrl( fline, lline - fline + 1 );
      
      i = thistptr + 1;
      for ( j = 0; j < MAXHISTLOG; j++ )
	{
	  i = modp1( i - 1, MAXHISTLOG );
	  unum = History[i].histunum;
	  
	  if ( unum < 0 || unum >= MAXUSERS )
	    continue; 
	  if ( ! Users[unum].live )
	    continue; 
	  if (Users[unum].type == UT_REMOTE)
	    sprintf(puname, "@%s", Users[unum].username);
	  else
	    strcpy(puname, Users[unum].username);
	  
				/* entry time */
	  getdandt( histentrytm, History[i].histlog);
	  
	  
				/* now elapsed time */
	  fmtseconds((int) History[i].elapsed, connecttm);
	  /* strip off seconds, or for long times, anything after 7 bytes */
	  connecttm[7] = '\0';
	  
	  cprintf( lin, col, ALIGN_NONE, 
		   "#%d#%-10.10s #%d#%16s#%d#-#%d#%7s", 
		   YellowLevelColor,
		   puname, 
		   GreenLevelColor,
		   histentrytm,
		   NoColor,
		   RedLevelColor,
		   connecttm);
	  
	  lin++;
	  if ( lin > lline )
	    {
	      col = 40;
	      lin = fline;
	    }
	}
      
      putpmt( MTXT_DONE, MSG_LIN2 );
      cdrefresh();
      if ( iogtimed( &ch, 1 ) )
	break;				/* exit loop if we got one */
    }
  
  return;
  
}


/*  initeverything - initialize (with extra cheese and tomato) (DOES LOCKING) */
/*  SYNOPSIS */
/*    initeverything */
void initeverything(void)
{
  
  int i, j;
  
  /* Twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  PVUNLOCK(&ConqInfo->lockmesg);

  /* Zero EVERYTHING. */
  zeroeverything();

  PVLOCK(&ConqInfo->lockword);
  
  /* Turn off the universe. */
  ConqInfo->closed = TRUE;
  
  /* reset the lockwords if using semaphores */
#if defined(USE_PVLOCK) && defined(USE_SEMS)
  ConqInfo->lockword = 0;
  ConqInfo->lockmesg = 0;
#endif

  /* Zero team stats. */
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    for ( j = 0; j < MAXTSTATS; j = j + 1 )
      Teams[i].stats[j] = 0;
  
  /* De-register all users. */
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    {
      Users[i].live = FALSE;
      Users[i].type = UT_LOCAL;
    }
  
  ConqInfo->celapsedseconds = 0;
  ConqInfo->ccpuseconds = 0;
  ConqInfo->delapsedseconds = 0;
  ConqInfo->dcpuseconds = 0;
  ConqInfo->relapsedseconds = 0;
  ConqInfo->rcpuseconds = 0;
  ConqInfo->raccum = 0;
  
  stcpn( "never", ConqInfo->lastupchuck, DATESIZE );
  getdandt( ConqInfo->inittime, 0 );
  getdandt( ConqInfo->conqtime, 0 );
  stcpn( "GOD", ConqInfo->conqueror, MAXUSERPNAME );
  stcpn( "self ruled", ConqInfo->conqteam, MAXTEAMNAME );
  stcpn( "Let there be light...", ConqInfo->lastwords, MAXLASTWORDS );
  
  /* Un-twiddle the lockwords. */
  PVUNLOCK(&ConqInfo->lockword);
  PVUNLOCK(&ConqInfo->lockmesg);
  
  initrobots();
  inituniverse();
  
  return;
  
}


/*  initgame - initialize the game-permanent variables */
/*  SYNOPSIS */
/*    initgame */
void initgame(void)
{
  
  int i, j;
  
  
  /* Twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  
  PVLOCK(&ConqInfo->lockword);
  
  /* Driver. */
  Driver->drivsecs = 0;
  
  /* Doomsday machine. */
  Doomsday->status = DS_OFF;
  Doomsday->x = 0.0;
  Doomsday->y = 0.0;
  Doomsday->dx = 0.0;
  Doomsday->dy = 0.0;
  Doomsday->heading = 0.0;
  Doomsday->lock = 0;
  stcpn( "Doomsday Machine", Doomsday->name, MAXUSERPNAME );
  
  /* Set up initial armies on planets. */
  Planets[PNUM_SOL].team = TEAM_NOTEAM;
  Planets[PNUM_EARTH].team = TEAM_FEDERATION;
  Planets[PNUM_TELOS].team = TEAM_FEDERATION;
  Planets[PNUM_OMEGA].team = TEAM_FEDERATION;
  Planets[PNUM_SIRIUS].team = TEAM_NOTEAM;
  Planets[PNUM_ROMULUS].team = TEAM_ROMULAN;
  Planets[PNUM_REMUS].team = TEAM_ROMULAN;
  Planets[PNUM_RHO].team = TEAM_ROMULAN;
  Planets[PNUM_KEJELA].team = TEAM_NOTEAM;
  Planets[PNUM_KLINGUS].team = TEAM_KLINGON;
  Planets[PNUM_LEUDUS].team = TEAM_KLINGON;
  Planets[PNUM_TARSUS].team = TEAM_KLINGON;
  Planets[PNUM_BETELGEUSE].team = TEAM_NOTEAM;
  Planets[PNUM_ORION].team = TEAM_ORION;
  Planets[PNUM_OBERON].team = TEAM_ORION;
  Planets[PNUM_UMBRIEL].team = TEAM_ORION;
  Planets[PNUM_MURISAK].team = TEAM_NOTEAM;
  Planets[PNUM_JANUS].team = TEAM_SELFRULED;
  Planets[PNUM_SERITIL].team = TEAM_SELFRULED;
  Planets[PNUM_ELAS].team = TEAM_SELFRULED;
  Planets[PNUM_SHERMAN].team = TEAM_SELFRULED;
  Planets[PNUM_CHERON].team = TEAM_SELFRULED;
  Planets[PNUM_DAKEL].team = TEAM_SELFRULED;
  Planets[PNUM_OLDAR].team = TEAM_SELFRULED;
  Planets[PNUM_SARAC].team = TEAM_SELFRULED;
  Planets[PNUM_EMINIAR].team = TEAM_SELFRULED;
  Planets[PNUM_VENAR].team = TEAM_SELFRULED;
  Planets[PNUM_DYNEB].team = TEAM_SELFRULED;
  Planets[PNUM_XIDEX].team = TEAM_SELFRULED;
  Planets[PNUM_RIGELB].team = TEAM_SELFRULED;
  
  Planets[PNUM_SYRINX].team = TEAM_NOTEAM;
  Planets[PNUM_ALTAIR].team = TEAM_SELFRULED;
  Planets[PNUM_HELL].team = TEAM_SELFRULED;
  Planets[PNUM_JINX].team = TEAM_SELFRULED;
  Planets[PNUM_LUNA].team = TEAM_NOTEAM;
  
  Planets[PNUM_GHOST1].team = TEAM_NOTEAM;
  Planets[PNUM_GHOST2].team = TEAM_NOTEAM;
  Planets[PNUM_GHOST3].team = TEAM_NOTEAM;
  Planets[PNUM_GHOST4].team = TEAM_NOTEAM;

				/* set up teams for extra planets */
  if (NUM_EXTRAPLANETS > 0)
    {				/* there are extra planets
				   they are numbered starting at
				   1 (+ NUM_BASEPLANETS) */
      for (i=1; i<= NUM_EXTRAPLANETS; i++) 
	{
	  Planets[NUM_BASEPLANETS + i].team = TEAM_SELFRULED;
	}
    }

  
  Planets[PNUM_SOL].armies = rndint(80, 180);
  Planets[PNUM_EARTH].armies = 50;
  Planets[PNUM_TELOS].armies = 50;
  Planets[PNUM_OMEGA].armies = 50;
  Planets[PNUM_SIRIUS].armies = rndint(80, 180);
  Planets[PNUM_ROMULUS].armies = 50;
  Planets[PNUM_REMUS].armies = 50;
  Planets[PNUM_RHO].armies = 50;
  Planets[PNUM_KEJELA].armies = rndint(80, 180);
  Planets[PNUM_KLINGUS].armies = 50;
  Planets[PNUM_LEUDUS].armies = 50;
  Planets[PNUM_TARSUS].armies = 50;
  Planets[PNUM_BETELGEUSE].armies = rndint(80, 180);
  Planets[PNUM_ORION].armies = 50;
  Planets[PNUM_OBERON].armies = 50;
  Planets[PNUM_UMBRIEL].armies = 50;
  Planets[PNUM_MURISAK].armies = rndint(80, 180);
  Planets[PNUM_JANUS].armies = 25;
  Planets[PNUM_SERITIL].armies = 25;
  Planets[PNUM_ELAS].armies = 25;
  Planets[PNUM_SHERMAN].armies = 25;
  Planets[PNUM_CHERON].armies = 25;
  Planets[PNUM_DAKEL].armies = 25;
  Planets[PNUM_OLDAR].armies = 25;
  Planets[PNUM_SARAC].armies = 25;
  Planets[PNUM_EMINIAR].armies = 25;
  Planets[PNUM_VENAR].armies = 25;
  Planets[PNUM_DYNEB].armies = 25;
  Planets[PNUM_XIDEX].armies = 25;
  Planets[PNUM_RIGELB].armies = 25;
  Planets[PNUM_SYRINX].armies = rndint(100, 200);
  
  /* The rest don't matter since you don't have to conquer them. */

  Planets[PNUM_ALTAIR].armies = rndint(80, 120);
  Planets[PNUM_HELL].armies = rndint(50, 100);
  Planets[PNUM_JINX].armies = rndint(100, 200);
  Planets[PNUM_LUNA].armies = 0;
  
  Planets[PNUM_GHOST1].armies = 0;
  Planets[PNUM_GHOST2].armies = 0;
  Planets[PNUM_GHOST3].armies = 0;
  Planets[PNUM_GHOST4].armies = 0;
  
  /* Set up the pscanned array so that each team has scanned its own planets. */
  for ( i = 1 ; i <= NUMPLANETS; i = i + 1 )
    {
      Planets[i].uninhabtime = 0;		/* planets start out inhabitable */
      for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
	Planets[i].scanned[j] = FALSE;
    }
  
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      /* Each team has scanned its own planets. */
      for ( j = 0; j < 3; j = j + 1 )
	Planets[Teams[i].teamhplanets[j]].scanned[i] = TRUE;
      
      Teams[i].couptime = 0;		/* time left to coup starts at zero. */
      Teams[i].coupinfo = FALSE;		/* don't know coup time */
    }
  
  /* Un-twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  
  /* Set up the physical universe. */
  initplanets();
  
  return;
  
}


/*  initmsgs - initialize the message data structures */
/*  SYNOPSIS */
/*    initmsgs */
void initmsgs(void)
{
  
  int i;
  
  /* Zero the message buffer. */
  for ( i = 0; i < MAXMESSAGES; i = i + 1 )
    {
      Msgs[i].msgbuf[0] = EOS;
      Msgs[i].msgfrom = 0;
      Msgs[i].msgto = 0;
    }
  ConqInfo->lastmsg = 0;
  ConqInfo->glastmsg = ConqInfo->lastmsg;
  
  return;
  
}


/*  initplanets - initialize the planets */
/*  SYNOPSIS */
/*    initplanets */
void initplanets(void)
{
  /* SETPLANET( name, pnum ) */
#define SETPLANET(x, y)  stcpn( x, Planets[y].name, MAXPLANETNAME ) 
  
  int i; 
  real orbang, orbvel;
  
  /* Twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  PVLOCK(&ConqInfo->lockword);
  
  SETPLANET( "Sol", PNUM_SOL );
  SETPLANET( "Earth", PNUM_EARTH );
  SETPLANET( "Telos", PNUM_TELOS );
  SETPLANET( "Omega", PNUM_OMEGA );
  SETPLANET( "Sirius", PNUM_SIRIUS );
  SETPLANET( "Romulus", PNUM_ROMULUS );
  SETPLANET( "Remus", PNUM_REMUS );
  SETPLANET( "Rho", PNUM_RHO );
  SETPLANET( "Kejela", PNUM_KEJELA );
  SETPLANET( "Klingus", PNUM_KLINGUS );
  SETPLANET( "Leudus", PNUM_LEUDUS );
  SETPLANET( "Tarsus", PNUM_TARSUS );
  SETPLANET( "Betelgeuse", PNUM_BETELGEUSE );
  SETPLANET( "Orion", PNUM_ORION );
  SETPLANET( "Oberon", PNUM_OBERON );
  SETPLANET( "Umbriel", PNUM_UMBRIEL );
  SETPLANET( "Murisak", PNUM_MURISAK );
  SETPLANET( "Janus", PNUM_JANUS );
  SETPLANET( "Seritil", PNUM_SERITIL );
  SETPLANET( "Elas", PNUM_ELAS );
  SETPLANET( "Sherman", PNUM_SHERMAN );
  SETPLANET( "Cheron", PNUM_CHERON );
  SETPLANET( "Dakel", PNUM_DAKEL );
  SETPLANET( "Oldar", PNUM_OLDAR );
  SETPLANET( "Sarac", PNUM_SARAC );
  SETPLANET( "Eminiar", PNUM_EMINIAR );
  SETPLANET( "Venar", PNUM_VENAR );
  SETPLANET( "Dyneb", PNUM_DYNEB );
  SETPLANET( "Xidex", PNUM_XIDEX );
  SETPLANET( "RigelB", PNUM_RIGELB );
  
  SETPLANET( "Syrinx", PNUM_SYRINX );
  SETPLANET( "Luna", PNUM_LUNA );
  SETPLANET( "Altair", PNUM_ALTAIR );
  SETPLANET( "Hell", PNUM_HELL );
  SETPLANET( "Jinx", PNUM_JINX );
  
  SETPLANET( "Ghost 1", PNUM_GHOST1 );
  SETPLANET( "Ghost 2", PNUM_GHOST2 );
  SETPLANET( "Ghost 3", PNUM_GHOST3 );
  SETPLANET( "Ghost 4", PNUM_GHOST4 );
  SETPLANET( "Ghost 5", PNUM_GHOST5 );
  
  Planets[PNUM_SOL].type = PLANET_SUN;
  Planets[PNUM_EARTH].type = PLANET_CLASSM;
  Planets[PNUM_TELOS].type = PLANET_DEAD;
  Planets[PNUM_OMEGA].type = PLANET_DEAD;
  Planets[PNUM_SIRIUS].type = PLANET_SUN;
  Planets[PNUM_ROMULUS].type = PLANET_CLASSM;
  Planets[PNUM_REMUS].type = PLANET_DEAD;
  Planets[PNUM_RHO].type = PLANET_DEAD;
  Planets[PNUM_KEJELA].type = PLANET_SUN;
  Planets[PNUM_KLINGUS].type = PLANET_CLASSM;
  Planets[PNUM_LEUDUS].type = PLANET_DEAD;
  Planets[PNUM_TARSUS].type = PLANET_DEAD;
  Planets[PNUM_BETELGEUSE].type = PLANET_SUN;
  Planets[PNUM_ORION].type = PLANET_CLASSM;
  Planets[PNUM_OBERON].type = PLANET_DEAD;
  Planets[PNUM_UMBRIEL].type = PLANET_DEAD;
  Planets[PNUM_MURISAK].type = PLANET_SUN;
  Planets[PNUM_JANUS].type = PLANET_CLASSM;
  Planets[PNUM_SERITIL].type = PLANET_DEAD;
  Planets[PNUM_ELAS].type = PLANET_CLASSM;
  Planets[PNUM_SHERMAN].type = PLANET_CLASSM;
  Planets[PNUM_CHERON].type = PLANET_DEAD;
  Planets[PNUM_DAKEL].type = PLANET_CLASSM;
  Planets[PNUM_OLDAR].type = PLANET_DEAD;
  Planets[PNUM_SARAC].type = PLANET_CLASSM;
  Planets[PNUM_EMINIAR].type = PLANET_DEAD;
  Planets[PNUM_VENAR].type = PLANET_CLASSM;
  Planets[PNUM_DYNEB].type = PLANET_DEAD;
  Planets[PNUM_XIDEX].type = PLANET_CLASSM;
  Planets[PNUM_RIGELB].type = PLANET_DEAD;
  Planets[PNUM_GHOST1].type = PLANET_GHOST;
  Planets[PNUM_GHOST2].type = PLANET_GHOST;
  Planets[PNUM_GHOST3].type = PLANET_GHOST;
  Planets[PNUM_GHOST4].type = PLANET_GHOST;
  Planets[PNUM_GHOST5].type = PLANET_GHOST;
  
  Planets[PNUM_SYRINX].type = PLANET_SUN;
  Planets[PNUM_LUNA].type = PLANET_MOON;
  Planets[PNUM_ALTAIR].type = PLANET_CLASSM;
  Planets[PNUM_HELL].type = PLANET_DEAD;
  Planets[PNUM_JINX].type = PLANET_CLASSM;
  
  stcpn( "class M planet", ConqInfo->ptname[PLANET_CLASSM], MAXPTYPENAME );
  stcpn( "dead planet", ConqInfo->ptname[PLANET_DEAD], MAXPTYPENAME );
  stcpn( "sun", ConqInfo->ptname[PLANET_SUN], MAXPTYPENAME );
  stcpn( "moon", ConqInfo->ptname[PLANET_MOON], MAXPTYPENAME );
  stcpn( "ghost planet", ConqInfo->ptname[PLANET_GHOST], MAXPTYPENAME );
  stcpn( "class A planet", ConqInfo->ptname[PLANET_CLASSA], MAXPTYPENAME );
  stcpn( "class O planet", ConqInfo->ptname[PLANET_CLASSO], MAXPTYPENAME );
  stcpn( "class Z planet", ConqInfo->ptname[PLANET_CLASSZ], MAXPTYPENAME );
  
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    {
      Planets[i].real = TRUE;	/* by default, you can see most planets */
      Planets[i].orbvel = 0.0;
      Planets[i].orbrad = 0;
      Planets[i].primary = 0;
    }
  
  /* Can't see the ghost planets. */
  Planets[PNUM_GHOST1].real = FALSE;
  Planets[PNUM_GHOST2].real = FALSE;
  Planets[PNUM_GHOST3].real = FALSE;
  Planets[PNUM_GHOST4].real = FALSE;
  Planets[PNUM_GHOST5].real = FALSE;
  
  /* Set up the X-Y coordinates of the suns.  Start with Murisak. */
  /* at the center, then place the other eight suns relative to it, */
  /* then the radii of the planets. */
  Planets[PNUM_MURISAK].x = 0.0;
  Planets[PNUM_MURISAK].y = 0.0;
  Planets[PNUM_MURISAK].primary = 0;	/* the only "stationary" object */
  Planets[PNUM_MURISAK].orbrad = 0.0;
  Planets[PNUM_MURISAK].orbang = 0.0;
  Planets[PNUM_MURISAK].orbvel = 0.0;
  
  Planets[PNUM_BETELGEUSE].primary = PNUM_MURISAK;
  Planets[PNUM_BETELGEUSE].orbrad = 11000.0;
  Planets[PNUM_BETELGEUSE].orbang = 45.0;
  Planets[PNUM_BETELGEUSE].orbvel = 0.0;
  
  Planets[PNUM_KEJELA].primary = PNUM_MURISAK;
  Planets[PNUM_KEJELA].orbrad = 11000.0;
  Planets[PNUM_KEJELA].orbang = 135.0;
  Planets[PNUM_KEJELA].orbvel = 0.0;
  
  Planets[PNUM_SIRIUS].primary = PNUM_MURISAK;
  Planets[PNUM_SIRIUS].orbrad = 11000.0;
  Planets[PNUM_SIRIUS].orbang = 225.0;
  Planets[PNUM_SIRIUS].orbvel = 0.0;
  
  Planets[PNUM_SOL].primary = PNUM_MURISAK;
  Planets[PNUM_SOL].orbrad = 11000.0;
  Planets[PNUM_SOL].orbang = 315.0;
  Planets[PNUM_SOL].orbvel = 0.0;
  
  Planets[PNUM_GHOST1].primary = PNUM_MURISAK;
  Planets[PNUM_GHOST1].orbrad = 12000.0;
  Planets[PNUM_GHOST1].orbang = 0.0;
  Planets[PNUM_GHOST1].orbvel = 0.0;
  
  Planets[PNUM_GHOST2].primary = PNUM_MURISAK;
  Planets[PNUM_GHOST2].orbrad = 12000.0;
  Planets[PNUM_GHOST2].orbang = 90.0;
  Planets[PNUM_GHOST2].orbvel = 0.0;
  
  Planets[PNUM_GHOST3].primary = PNUM_MURISAK;
  Planets[PNUM_GHOST3].orbrad = 12000.0;
  Planets[PNUM_GHOST3].orbang = 180.0;
  Planets[PNUM_GHOST3].orbvel = 0.0;
  
  Planets[PNUM_GHOST4].primary = PNUM_MURISAK;
  Planets[PNUM_GHOST4].orbrad = 12000.0;
  Planets[PNUM_GHOST4].orbang = 270.0;
  Planets[PNUM_GHOST4].orbvel = 0.0;
  
  Planets[PNUM_GHOST5].primary = PNUM_MURISAK;
  Planets[PNUM_GHOST5].orbrad = 12000.0;
  Planets[PNUM_GHOST5].orbang = 0.0;
  Planets[PNUM_GHOST5].orbvel = 0.0;
  
  /* Murisak's planets. */
  Planets[PNUM_JANUS].primary = PNUM_MURISAK;
  Planets[PNUM_JANUS].orbrad = 2600.0;
  Planets[PNUM_SERITIL].primary = PNUM_MURISAK;
  Planets[PNUM_SERITIL].orbrad = 2600.0;
  Planets[PNUM_ELAS].primary = PNUM_MURISAK;
  Planets[PNUM_ELAS].orbrad = 2600.0;
  Planets[PNUM_SHERMAN].primary = PNUM_MURISAK;
  Planets[PNUM_SHERMAN].orbrad = 2600.0;
  Planets[PNUM_CHERON].primary = PNUM_MURISAK;
  Planets[PNUM_CHERON].orbrad = 2600.0;
  /* Sol's planets. */
  Planets[PNUM_EARTH].primary = PNUM_SOL;
  Planets[PNUM_EARTH].orbrad = 2600.0;
  Planets[PNUM_TELOS].primary = PNUM_SOL;
  Planets[PNUM_TELOS].orbrad = 2600.0;
  Planets[PNUM_OMEGA].primary = PNUM_SOL;
  Planets[PNUM_OMEGA].orbrad = 2600.0;
  /* Sirius' planets. */
  Planets[PNUM_ROMULUS].primary = PNUM_SIRIUS;
  Planets[PNUM_ROMULUS].orbrad = 2600.0;
  Planets[PNUM_REMUS].primary = PNUM_SIRIUS;
  Planets[PNUM_REMUS].orbrad = 2600.0;
  Planets[PNUM_RHO].primary = PNUM_SIRIUS;
  Planets[PNUM_RHO].orbrad = 2600.0;
  /* Kejela's planets. */
  Planets[PNUM_KLINGUS].primary = PNUM_KEJELA;
  Planets[PNUM_KLINGUS].orbrad = 2600.0;
  Planets[PNUM_LEUDUS].primary = PNUM_KEJELA;
  Planets[PNUM_LEUDUS].orbrad = 2600.0;
  Planets[PNUM_TARSUS].primary = PNUM_KEJELA;
  Planets[PNUM_TARSUS].orbrad = 2600.0;
  /* Betelgeuse's planets. */
  Planets[PNUM_ORION].primary = PNUM_BETELGEUSE;
  Planets[PNUM_ORION].orbrad = 2600.0;
  Planets[PNUM_OBERON].primary = PNUM_BETELGEUSE;
  Planets[PNUM_OBERON].orbrad = 2600.0;
  Planets[PNUM_UMBRIEL].primary = PNUM_BETELGEUSE;
  Planets[PNUM_UMBRIEL].orbrad = 2600.0;
  /* Side systems. */
  Planets[PNUM_XIDEX].primary = PNUM_GHOST1;
  Planets[PNUM_XIDEX].orbrad = 1150.0;
  Planets[PNUM_RIGELB].primary = PNUM_GHOST1;
  Planets[PNUM_RIGELB].orbrad = 1150.0;
  Planets[PNUM_VENAR].primary = PNUM_GHOST2;
  Planets[PNUM_VENAR].orbrad = 1150.0;
  Planets[PNUM_DYNEB].primary = PNUM_GHOST2;
  Planets[PNUM_DYNEB].orbrad = 1150.0;
  Planets[PNUM_SARAC].primary = PNUM_GHOST3;
  Planets[PNUM_SARAC].orbrad = 1150.0;
  Planets[PNUM_EMINIAR].primary = PNUM_GHOST3;
  Planets[PNUM_EMINIAR].orbrad = 1150.0;
  Planets[PNUM_DAKEL].primary = PNUM_GHOST4;
  Planets[PNUM_DAKEL].orbrad = 1150.0;
  Planets[PNUM_OLDAR].primary = PNUM_GHOST4;
  Planets[PNUM_OLDAR].orbrad = 1150.0;
  
  Planets[PNUM_SYRINX].primary = PNUM_MURISAK;
  Planets[PNUM_SYRINX].orbrad = 23000.0;
  
  Planets[PNUM_LUNA].primary = PNUM_EARTH;
  Planets[PNUM_LUNA].orbrad = 1250.0;
  
  Planets[PNUM_ALTAIR].primary = PNUM_MURISAK;
  Planets[PNUM_ALTAIR].orbrad = 6125.0;

  Planets[PNUM_HELL].primary = PNUM_SYRINX;
  Planets[PNUM_HELL].orbrad = 1900.0;
  Planets[PNUM_JINX].primary = PNUM_SYRINX;
  Planets[PNUM_JINX].orbrad = 2600.0;
  
  
  /* Set orbital angles and velocities for planets, and place them. */
  /* Murisak's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_JANUS].orbang = orbang;
  Planets[PNUM_JANUS].orbvel = orbvel;
  Planets[PNUM_CHERON].orbang = mod360( orbang + 1.0/5.0*360.0 );
  Planets[PNUM_CHERON].orbvel = orbvel;
  Planets[PNUM_SHERMAN].orbang = mod360( orbang + 2.0/5.0*360.0 );
  Planets[PNUM_SHERMAN].orbvel = orbvel;
  Planets[PNUM_ELAS].orbang = mod360( orbang + 3.0/5.0*360.0 );
  Planets[PNUM_ELAS].orbvel = orbvel;
  Planets[PNUM_SERITIL].orbang = mod360( orbang + 4.0/5.0*360.0 );
  Planets[PNUM_SERITIL].orbvel = orbvel;
  /* Sol's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_EARTH].orbang = orbang;
  Planets[PNUM_EARTH].orbvel = orbvel;
  Planets[PNUM_TELOS].orbang = mod360( orbang + 2.0/3.0*360.0 );
  Planets[PNUM_TELOS].orbvel = orbvel;
  Planets[PNUM_OMEGA].orbang = mod360( orbang + 1.0/3.0*360.0 );
  Planets[PNUM_OMEGA].orbvel = orbvel;
  /* Luna. */
  Planets[PNUM_LUNA].orbvel = 12.0 * orbvel;
  Planets[PNUM_LUNA].orbang = rnduni( 0.0, 360.0 );
  /* Sirius' planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_ROMULUS].orbang = orbang;
  Planets[PNUM_ROMULUS].orbvel = orbvel;
  Planets[PNUM_REMUS].orbang = mod360( orbang + 2.0/3.0*360.0 );
  Planets[PNUM_REMUS].orbvel = orbvel;
  Planets[PNUM_RHO].orbang = mod360( orbang + 1.0/3.0*360.0 );
  Planets[PNUM_RHO].orbvel = orbvel;
  /* Kejela's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_KLINGUS].orbang = orbang;
  Planets[PNUM_KLINGUS].orbvel = orbvel;
  Planets[PNUM_LEUDUS].orbang = mod360( orbang + 2.0/3.0*360.0 );
  Planets[PNUM_LEUDUS].orbvel = orbvel;
  Planets[PNUM_TARSUS].orbang = mod360( orbang + 1.0/3.0*360.0 );
  Planets[PNUM_TARSUS].orbvel = orbvel;
  /* Betelgeuse's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_ORION].orbang = orbang;
  Planets[PNUM_ORION].orbvel = orbvel;
  Planets[PNUM_OBERON].orbang = mod360( orbang + 2.0/3.0*360.0 );
  Planets[PNUM_OBERON].orbvel = orbvel;
  Planets[PNUM_UMBRIEL].orbang = mod360( orbang + 1.0/3.0*360.0 );
  Planets[PNUM_UMBRIEL].orbvel = orbvel;
  /* Side systems. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_XIDEX].orbang = orbang;
  Planets[PNUM_XIDEX].orbvel = orbvel;
  Planets[PNUM_RIGELB].orbang = mod360( orbang + 1.0/2.0*360.0 );
  Planets[PNUM_RIGELB].orbvel = orbvel;
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_VENAR].orbang = orbang;
  Planets[PNUM_VENAR].orbvel = orbvel;
  Planets[PNUM_DYNEB].orbang = mod360( orbang + 1.0/2.0*360.0 );
  Planets[PNUM_DYNEB].orbvel = orbvel;
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_SARAC].orbang = orbang;
  Planets[PNUM_SARAC].orbvel = orbvel;
  Planets[PNUM_EMINIAR].orbang = mod360( orbang + 1.0/2.0*360.0 );
  Planets[PNUM_EMINIAR].orbvel = orbvel;
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  Planets[PNUM_DAKEL].orbang = orbang;
  Planets[PNUM_DAKEL].orbvel = orbvel;
  Planets[PNUM_OLDAR].orbang = mod360( orbang + 1.0/2.0*360.0 );
  Planets[PNUM_OLDAR].orbvel = orbvel;
  
  Planets[PNUM_SYRINX].orbvel = -6.314179;
  Planets[PNUM_SYRINX].orbang = rnduni(0.0, 360.0);
  
  /* Syrinx's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );

  Planets[PNUM_ALTAIR].orbang = rnduni(0.0, 360.0);
  Planets[PNUM_ALTAIR].orbvel = -23.1;

  Planets[PNUM_HELL].orbang = mod360( orbang + 2.0/3.0*360.0 );
  Planets[PNUM_HELL].orbvel = orbvel;

  Planets[PNUM_JINX].orbang = mod360( orbang + 1.0/3.0*360.0 );
  Planets[PNUM_JINX].orbvel = 7.5;

  /* Place the planets in their proper orbits. */
  for ( i = NUMPLANETS; i > 0; i = i - 1 )
    if ( Planets[i].primary != 0 )
      {
	Planets[i].x = Planets[Planets[i].primary].x + 
	  Planets[i].orbrad * cosd(Planets[i].orbang);
	Planets[i].y = Planets[Planets[i].primary].y + 
	  Planets[i].orbrad * sind(Planets[i].orbang);
      }
  
				/* Now we init the 'extra' planets with
				   defaults */
  if (NUM_EXTRAPLANETS > 0)
    {				/* there are extra planets
				   they are numbered starting at
				   1 (+ NUM_BASEPLANETS) */
      for (i=1; i<= NUM_EXTRAPLANETS; i++) 
	{
	  char buf[64];
	  int t;

	  t = NUM_BASEPLANETS + i;
	  sprintf(buf, "Extra %d", i);
	  strcpy(Planets[t].name, buf);
	  Planets[t].type = PLANET_GHOST;
	  Planets[t].real = FALSE;
	  Planets[t].x = 0.0;
	  Planets[t].y = 0.0;
	  Planets[t].primary = PNUM_MURISAK;
	  Planets[t].orbrad = rnduni(30000.0, 70000.0);
	  Planets[t].orbvel = rnduni(-5.0, +5.0);
	  Planets[t].orbang = rnduni(0.0, 360.0);
	}
    }

  /* Un-twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  
  /* Protect against a system crash here! */
  upchuck();
  
  return;
  
}


/*  initrobots - initialize robots */
/*  SYNOPSIS */
/*    initrobots */
void initrobots(void)
{
  int i, j, unum;
  
  /* SETROBOT( name, pname, team ) */
#define SETROBOT(x, y, z) \
  { \
      if ( gunum( &unum, x, UT_LOCAL ) ) \
          stcpn( y, Users[unum].alias, MAXUSERPNAME ); \
      else if ( c_register( x, y, z, &unum ) ) \
      { \
	  Users[unum].robot = TRUE; \
	  Users[unum].ooptions[OOPT_MULTIPLE] = TRUE; \
	  Users[unum].multiple = MAXSHIPS; \
          Users[unum].type = UT_LOCAL; /* robots are always local */ \
      } \
  }
    
    /* Create robot guardians. */
  SETROBOT( "Romulan", "Colossus", TEAM_ROMULAN );
  SETROBOT( "Orion", "HAL 9000", TEAM_ORION );
  SETROBOT( "Federation", "M-5", TEAM_FEDERATION );
  SETROBOT( "Klingon", "Guardian", TEAM_KLINGON );
  
  /* Copy the strategy table. */
  for ( i = 0; i < MAX_VAR; i = i + 1 )
    for ( j = 0; j < 10; j = j + 1 )
      Robot->rstrat[i][j] = trstrat[i][j];
  
  /* Copy the action vector. */
  for ( i = 0; i < 32; i = i + 1 )
    Robot->rvec[i] = trvec[i];
  
  ConqInfo->externrobots = FALSE;	/* XXX temporary */
  
  return;
  
}


/*  initship - initialize a ship for use (DOES MESSAGE LOCKING) */
/*  SYNOPSIS */
/*    int snum, unum */
/*    initship( snum, unum ) */
/* Note: this routine assumes that the ship is reserved. */
void initship( int snum, int unum )
{
  int i, j;
  
  /* sstatus(snum)				# never modified here */
  Ships[snum].killedby = 0;
  /* suser(snum)				# setup in menu() or newrob() */
  /* steam(snum)				# setup in menu() or newrob() */
  /* spid(snum)				# setup in menu() or newrob() */
  Ships[snum].strkills = 0.0;	/* init to 0.0, newrob will init this
				   for robots to make them stronger, faster.. ;-) */
  Ships[snum].x = 0.0;
  Ships[snum].y = 0.0;
  Ships[snum].dx = 0.0;
  Ships[snum].dy = 0.0;
  Ships[snum].head = 0.0;
  Ships[snum].dhead = 0.0;
  Ships[snum].warp = 0.0;
  Ships[snum].dwarp = 0.0;
  Ships[snum].lock = 0;
  Ships[snum].shup = TRUE;
  Ships[snum].shields = 100.0;
  Ships[snum].kills = 0.0;
  Ships[snum].damage = 0.0;
  Ships[snum].fuel = 999.0;
  Ships[snum].wtemp = 0.0;
  Ships[snum].etemp = 0.0;
  Ships[snum].wfuse = 0;
  Ships[snum].efuse = 0;
  Ships[snum].weapalloc = 40;
  Ships[snum].engalloc = 100 - Ships[snum].weapalloc;
  Ships[snum].armies = 0;
  Ships[snum].rmode = FALSE;
  Ships[snum].cloaked = FALSE;
  /* soption(snum,i)				# setup in menu() */
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      /* srwar(snum,i)				# setup in menu() or newrob() */
      /* swar(snum,i)				# setup in menu() or newrob() */
      Ships[snum].scanned[i] = 0;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    Ships[snum].srpwar[i] = FALSE;
  /* ssdfuse(snum)				# setup in findship() */
  PVLOCK(&ConqInfo->lockmesg);
  if ( Ships[snum].lastmsg == LMSG_NEEDINIT )
    {
      Ships[snum].lastmsg = ConqInfo->lastmsg;
      Ships[snum].alastmsg = Ships[snum].lastmsg;
    }
  PVUNLOCK(&ConqInfo->lockmesg);
  Ships[snum].map = FALSE;
  Ships[snum].towing = 0;
  Ships[snum].towedby = 0;
  Ships[snum].lastblast = 0.0;
  Ships[snum].lastphase = 0.0;
  Ships[snum].pfuse = 0;
  Ships[snum].talert = FALSE;
  Ships[snum].robot = FALSE;
  Ships[snum].action = 0;
  /* spname(1,snum)				# setup in menu() or newrob() */
  
  /* Zero torpedos. */
  for ( i = 0; i < MAXTORPS; i = i + 1 )
    {
      Ships[snum].torps[i].status = TS_OFF;
      Ships[snum].torps[i].fuse = 0;
      Ships[snum].torps[i].x = 0.0;
      Ships[snum].torps[i].y = 0.0;
      Ships[snum].torps[i].dx = 0.0;
      Ships[snum].torps[i].dy = 0.0;
      Ships[snum].torps[i].mult = 0.0;
      for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
	Ships[snum].torps[i].war[j] = FALSE;
    }
  
  /* Update user some stats. */
  Users[unum].lastentry = getnow(NULL, 0);/* time stamp for this entry */

  Users[unum].stats[USTAT_ENTRIES] += 1;
  Teams[Ships[snum].team].stats[TSTAT_ENTRIES] += 1;
  
  return;
  
}


/*  inituniverse - initialize (without cheese and tomato) (DOES LOCKING) */
/*  SYNOPSIS */
/*    inituniverse */
void inituniverse(void)
{
  
  int i;
  
  /* Twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  PVLOCK(&ConqInfo->lockword);
  
  /* Turn off the universe. */
  ConqInfo->closed = TRUE;
  
  Teams[TEAM_FEDERATION].teamhplanets[0] = PNUM_EARTH;
  Teams[TEAM_FEDERATION].teamhplanets[1] = PNUM_TELOS;
  Teams[TEAM_FEDERATION].teamhplanets[2] = PNUM_OMEGA;
  Teams[TEAM_ROMULAN].teamhplanets[0] = PNUM_ROMULUS;
  Teams[TEAM_ROMULAN].teamhplanets[1] = PNUM_REMUS;
  Teams[TEAM_ROMULAN].teamhplanets[2] = PNUM_RHO;
  Teams[TEAM_KLINGON].teamhplanets[0] = PNUM_KLINGUS;
  Teams[TEAM_KLINGON].teamhplanets[1] = PNUM_LEUDUS;
  Teams[TEAM_KLINGON].teamhplanets[2] = PNUM_TARSUS;
  Teams[TEAM_ORION].teamhplanets[0] = PNUM_ORION;
  Teams[TEAM_ORION].teamhplanets[1] = PNUM_OBERON;
  Teams[TEAM_ORION].teamhplanets[2] = PNUM_UMBRIEL;
  
  Teams[TEAM_FEDERATION].homeplanet = PNUM_EARTH;
  Teams[TEAM_ROMULAN].homeplanet = PNUM_ROMULUS;
  Teams[TEAM_KLINGON].homeplanet = PNUM_KLINGUS;
  Teams[TEAM_ORION].homeplanet = PNUM_ORION;
  
  Teams[TEAM_FEDERATION].homesun = PNUM_SOL;
  Teams[TEAM_ROMULAN].homesun = PNUM_SIRIUS;
  Teams[TEAM_KLINGON].homesun = PNUM_KEJELA;
  Teams[TEAM_ORION].homesun = PNUM_BETELGEUSE;
  
  Teams[TEAM_FEDERATION].warplim = 9.0;
  Teams[TEAM_ROMULAN].warplim = 8.0;
  Teams[TEAM_KLINGON].warplim = 9.0;
  Teams[TEAM_ORION].warplim = 10.0;
  
  Teams[TEAM_FEDERATION].armylim = 9;
  Teams[TEAM_ROMULAN].armylim = 11;
  Teams[TEAM_KLINGON].armylim = 9;
  Teams[TEAM_ORION].armylim = 7;
  
  Teams[TEAM_FEDERATION].engfac = 1.0;
  Teams[TEAM_ROMULAN].engfac = 0.8;
  Teams[TEAM_KLINGON].engfac = 1.0;
  Teams[TEAM_ORION].engfac = 1.2;
  
  Teams[TEAM_FEDERATION].accelfac = 1.0;
  Teams[TEAM_ROMULAN].accelfac = 0.8;
  Teams[TEAM_KLINGON].accelfac = 1.0;
  Teams[TEAM_ORION].accelfac = 1.6;
  
  Teams[TEAM_ROMULAN].weafac = 1.17;
  Teams[TEAM_ORION].weafac = 0.83;
  Teams[TEAM_FEDERATION].weafac = 1.0;
  Teams[TEAM_KLINGON].weafac = 1.0;
  
  Teams[TEAM_FEDERATION].torpwarp = 12.0;
  Teams[TEAM_ROMULAN].torpwarp = 10.0;
  Teams[TEAM_KLINGON].torpwarp = 12.0;
  Teams[TEAM_ORION].torpwarp = 14.0;
  
  stcpn( "Federation", Teams[TEAM_FEDERATION].name, MAXTEAMNAME );
  stcpn( "Romulan", Teams[TEAM_ROMULAN].name, MAXTEAMNAME );
  stcpn( "Klingon", Teams[TEAM_KLINGON].name, MAXTEAMNAME );
  stcpn( "Orion", Teams[TEAM_ORION].name, MAXTEAMNAME );
  stcpn( "self ruled", Teams[TEAM_SELFRULED].name, MAXTEAMNAME );
  stcpn( "non", Teams[TEAM_NOTEAM].name, MAXTEAMNAME );
  stcpn( "GOD", Teams[TEAM_GOD].name, MAXTEAMNAME );
  stcpn( "Empire", Teams[TEAM_EMPIRE].name, MAXTEAMNAME );
  
  ConqInfo->chrplanets[PLANET_CLASSM] = 'M';
  ConqInfo->chrplanets[PLANET_DEAD] = 'D';
  ConqInfo->chrplanets[PLANET_SUN] = 'S';
  ConqInfo->chrplanets[PLANET_MOON] = 'm';
  ConqInfo->chrplanets[PLANET_GHOST] = 'G';
  ConqInfo->chrplanets[PLANET_CLASSA] = 'A';
  ConqInfo->chrplanets[PLANET_CLASSO] = 'O';
  ConqInfo->chrplanets[PLANET_CLASSZ] = 'Z';
  
  Teams[TEAM_FEDERATION].teamchar = 'F';
  Teams[TEAM_ROMULAN].teamchar = 'R';
  Teams[TEAM_KLINGON].teamchar = 'K';
  Teams[TEAM_ORION].teamchar = 'O';
  Teams[TEAM_SELFRULED].teamchar = '-';
  Teams[TEAM_NOTEAM].teamchar = ' ';
  Teams[TEAM_GOD].teamchar = 'G';
  Teams[TEAM_EMPIRE].teamchar = 'E';
  
  Teams[TEAM_FEDERATION].torpchar = '*';
  Teams[TEAM_ROMULAN].torpchar = '@';
  Teams[TEAM_KLINGON].torpchar = '+';
  Teams[TEAM_ORION].torpchar = '.';
  
  /* Initialize driver variables. */
  Driver->drivcnt = 0;
  Driver->drivowner[0] = EOS;
  
  /* Initialize user history stuff. */
  ConqInfo->histptr = 0;
  for ( i = 0; i < MAXHISTLOG; i = i + 1 )
    {
      History[i].histunum = -1;
      History[i].histlog = 0;
    }
  
  /* Un-twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  
  initgame();
  clearships();
  initmsgs();
  
  return;
  
}


/*  intrude - possibly send an intruder alert */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    intrude( snum, pnum ) */
void intrude( int snum, int pnum )
{
  char buf[MSGMAXLINE];
  string atta=" attacking";
  string armeq=", armies=";
  
  if ( Planets[pnum].real &&
      Planets[pnum].team != TEAM_SELFRULED &&
      Planets[pnum].team != TEAM_NOTEAM )
    if ( snum == MSG_DOOM )
      {
	c_strcpy( Doomsday->name, buf );
	upper( Doomsday->name );
	appstr( atta, buf );
	appstr( armeq, buf );
	appint( Planets[pnum].armies, buf );
	stormsg( -pnum, -Planets[pnum].team, buf );
      }
    else if ( Ships[snum].war[Planets[pnum].team] )
      {
	c_strcpy( "INTRUDER ALERT - ", buf );
	appship( snum, buf );
	appstr( atta, buf );
	appstr( armeq, buf );
	appint( Planets[pnum].armies, buf );
	stormsg( -pnum, -Planets[pnum].team, buf );
	defend( snum, pnum );
      }
  
  return;
  
}


/*  loghist - log this entry into the Game (DOES LOCKING) */
/*  SYNOPSIS */
/*    int unum */
/*    loghist( unum ) */
int loghist( int unum )
{
  int hnum;

  PVLOCK(&ConqInfo->lockword);
  ConqInfo->histptr = modp1( ConqInfo->histptr + 1, MAXHISTLOG );
				/* time stamp for this entry */
  History[ConqInfo->histptr].histlog = getnow(NULL, 0 );	
  History[ConqInfo->histptr].elapsed = (time_t)0;
  History[ConqInfo->histptr].histunum = unum;
  hnum = ConqInfo->histptr;

  PVUNLOCK(&ConqInfo->lockword);
  return(hnum);
}


/*  newarp - handle ship acceleration. */
/*  SYNOPSIS */
/*    real warp, newarp, dwarp */
/*    int snum */
/*    warp = newarp( snum, dwarp ) */
real newarp( int snum, real dwarp )
{
  real x, acc;
  
  x = dwarp - Ships[snum].warp;
  acc = (real) Teams[Ships[snum].team].accelfac * 
    (real) engeff( snum ) * ITER_SECONDS;
  if ( acc >= fabs( x ) )
    return ( dwarp );			/* close enough (or equal) */
  else if ( x < 0.0 )
    return ( Ships[snum].warp - acc );
  
  return ( Ships[snum].warp + acc );
  
}


/*  phoon - calculate the phase of a moon */
/*  SYNOPSIS */
/*    int phase, phoon, pnum */
/*    phase = phoon( pnum ) */
int phoon( int pnum )
{
  int i, j, ph;
  
  /* Suns simply don't have phases. */
  if ( Planets[pnum].type == PLANET_SUN )
    return ( PHOON_NO );
  
  /* You have to orbit some-thing to have a phase. */
  i = Planets[pnum].primary;
  if ( i == 0 )
    return ( PHOON_NO );
  
  /* Your primary must be a non-sun that is real. */
  if ( Planets[i].type == PLANET_SUN || ! Planets[i].real )
    return ( PHOON_NO );
  
  /* Your primary has to orbit a (real) sun to have a phase. */
  j = Planets[i].primary;
  if ( j == 0 )
    return ( PHOON_NO );
  if ( Planets[j].type != PLANET_SUN || ! Planets[j].real )
    return ( PHOON_NO );
  
  /* Things are cool, now calculate the phase. */
  ph = (int) ( mod360( Planets[pnum].orbang - Planets[i].orbang - 45.0 ) / 90.0 );
  
  /* The number calculated is in the range 0 to 3, and works fine */
  /* if the moon is orbiting counter clockwise. If it is orbiting */
  /* in the other direction, we must swap the first and last quarters. */
  if ( Planets[pnum].orbvel < 0.0 )
    switch ( ph )
      {
      case PHOON_FIRST:
	ph = PHOON_LAST;
	break;
      case PHOON_LAST:
	ph = PHOON_FIRST;
	break;
      }
  return ( ph );
  
}


/*  planmatch - check if a string matches a planet name */
/*  SYNOPSIS */
/*    int planmatch, pnum, godlike */
/*    char str() */
/*    int status, godlike */
/*    status = planmatch( str, pnum, godlike ) */
int planmatch( char *str, int *pnum, int godlike )
{
  if ( godlike )
    {
      for ( *pnum = 1; *pnum <= NUMPLANETS; *pnum = *pnum + 1 )
	if ( stmatch( str, Planets[*pnum].name, FALSE ) )
	  return ( TRUE );
    }
  else
    {
      for ( *pnum = 1; *pnum <= NUMPLANETS; *pnum = *pnum + 1 )
	if ( Planets[*pnum].real )
	  if ( stmatch( str, Planets[*pnum].name, FALSE ) )
	    return ( TRUE );
    }
  
  return ( FALSE );
  
}


/*  puthing - put an object on the display */
/*  SYNOPSIS */
/*    int what, lin, col */
/*    puthing( what, lin, col ) */
void puthing( int what, int lin, int col )
{
  int i, j, tlin, tcol;
  char buf[3][7];
  
  switch ( what )
    {
    case PLANET_SUN:
      c_strcpy( " \\|/ ", buf[0] );
      c_strcpy( "-- --", buf[1] );
      c_strcpy( " /|\\ ", buf[2] );
      break;
    case PLANET_CLASSM:
    case PLANET_CLASSA:
    case PLANET_CLASSO:
    case PLANET_CLASSZ:
    case PLANET_DEAD:
    case PLANET_GHOST:
      c_strcpy( " .-. ", buf[0] );
      c_strcpy( "(   )", buf[1] );
      c_strcpy( " `-' ", buf[2] );
      break;
    case PLANET_MOON:
      c_strcpy( "     ", buf[0] );
      c_strcpy( " ( ) ", buf[1] );
      c_strcpy( "     ", buf[2] );
      break;
    case THING_EXPLOSION:
      c_strcpy( " %%% ", buf[0] );
      c_strcpy( "%%%%%", buf[1] );
      c_strcpy( " %%% ", buf[2] );
      break;
    case THING_DEATHSTAR:
      c_strcpy( "/===\\", buf[0] );
      c_strcpy( "===O=", buf[1] );
      c_strcpy( "\\===/", buf[2] );
      break;
    default:
      c_strcpy( " ??? ", buf[0] );
      c_strcpy( "?????", buf[1] );
      c_strcpy( " ??? ", buf[2] );
      break;
    }
  
  for ( j=0; j<3; j++ )
    {
      tlin = lin + j - 1;
      /*	tlin = lin + j - 2; */
      if ( tlin >= 0 && tlin <= DISPLAY_LINS )
	for ( i = 0; i < 6; i = i + 1 )
	  {
	    tcol = col + i - 1;
	    /*	      tcol = col + i - 3;*/
	    if ( tcol > STAT_COLS && tcol <= CqContext.maxcol - 1 )
	      if (buf[j][i] != '\0')
		cdput( buf[j][i], tlin, tcol );
	  }
    }
  
  return;
  
}


/*  putship - place a ship in the universe */
/*  SYNOPSIS */
/*    int snum */
/*    real basex, basey */
/*    putship( snum, basex, basey ) */
void putship( int snum, real basex, real basey )
{
  int i, j;
  real smear; 
  
  
  smear = ENTRY_SMEAR_DIST;
  for ( j = 1; j <= 64; j = j + 1 )
    {
      /* If we're doing poorly, expand the area of entry. */
      if ( mod( j, 16 ) == 0 )
	smear = smear * 1.2;
      Ships[snum].x = rndnor( basex, smear );
      Ships[snum].y = rndnor( basey, smear );
      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
	if ( Ships[i].status == SS_LIVE )
	  if ( satwar( i, snum ) &&
	      i != snum &&
	      dist( Ships[snum].x, Ships[snum].y, Ships[i].x, Ships[i].y ) <= ENTRY_ENEMY_DIST )
	    goto clbl1; /* jet next;  2; */
      /* If we got here, then the position isn't near an enemy ship. */
      return;
      
    clbl1: /* jet - next 2 target */
      ;
    }
  
  /* If we got here, we couldn't find a "good" position, */
  /*  so just report the error and let it slide. */
  cerror( "putship(): Failed retry maximum on ship %d", snum );
  clog( "putship(): Failed retry maximum on ship %d", snum );
  
  return;
  
}


/*  readmsg - display a message */
/*  SYNOPSIS */
/*    int snum, msgnum */
/*    readmsg( snum, msgnum ) */
int readmsg( int snum, int msgnum, int dsplin )
{
  int i;
  char buf[MSGMAXLINE];
  unsigned int attrib = 0;
  
  
  buf[0] = '\0';
  
  if (HasColors)
    {				/* set up the attrib so msg's are cyan */
      attrib = COLOR_PAIR(COL_CYANBLACK);
    }

  /* Format who the message is from. */
  i = Msgs[msgnum].msgfrom;

  if ( i > 0 && i <= MAXSHIPS )
    {
      buf[0] = EOS;
      appship( i, buf );
    }
  else if ( -i > 0 && -i <= NUMPLANETS )
    c_strcpy( Planets[-i].name, buf );
  else switch ( i )
    {
    case MSG_NOONE:
      c_strcpy( "nobody", buf );
      break;
    case MSG_GOD:
      c_strcpy( "GOD", buf );
      break;
    case MSG_DOOM:
      concat( "The ", Doomsday->name, buf );
      break;
    case MSG_OUTSIDE:
      buf[0] = EOS;
      break;
    case MSG_COMP:
      c_strcpy( "Comp", buf );
      break;
    default:
      c_strcpy( "???", buf );
      break;
    }
  
  appstr( "->", buf );
  
  /* Format who the message is to. */
  i = Msgs[msgnum].msgto;
  if ( i == snum )
    appstr( "you", buf );
  else if ( i > 0 && i <= MAXSHIPS )
    appship( i, buf );
  else if ( -i >= 0 && -i < NUMPLAYERTEAMS )
    {
      appchr( Teams[-i].teamchar, buf );
    }
  else 
    {
      switch ( i )
	{
	case MSG_NOONE:
	  appstr( "nobody", buf );
	  break;
	case MSG_GOD:
	  appstr( "GOD", buf );
	  break;
	case MSG_ALL:
	  appstr( "ALL", buf );
	  break;
	case MSG_IMPLEMENTORS:
	  appstr( "IMPs", buf );
	  break;
	default:
	  appstr( "???", buf );
	  break;
	}
    }
  
  appstr( ": ", buf );
  appstr( Msgs[msgnum].msgbuf, buf );

  attrset(attrib);
  c_putmsg( buf, dsplin );
  attrset(0);
				/* clear second line if sending to MSG_LIN1 */
  if (dsplin == MSG_LIN1)
    {
      cdclrl( MSG_LIN2, 1 );
    }
  
  return(TRUE);
  
}


/*  sendmsg - prompt the user for a message and send it */
/*  SYNOPSIS */
/*    int from */
/*    sendmsg( from, terse ) */
void sendmsg( int from, int terse )
{
  int i, j; 
  char buf[MSGMAXLINE] = "", msg[MESSAGE_SIZE] = "";
  int ch;
  int editing; 
  string mto="Message to: ";
  string nf="Not found.";
  string huh="I don't understand.";

  int append_flg;  /* when user types to the limit. */
  int do_append_flg;
  
  static int to = MSG_NOONE;
  
  /* First, find out who we're sending to. */
  cdclrl( MSG_LIN1, 2 );
  buf[0] = EOS;
  ch = cdgetx( mto, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, TRUE );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  /* TAB or RETURN means use the target from the last message. */
  editing = ( (ch == TERM_EXTRA || ch == TERM_NORMAL) && buf[0] == EOS );
  if ( editing )
    {
      /* Make up a default string using the last target. */
      if ( to > 0 && to <= MAXSHIPS )
	sprintf( buf, "%d", to );
      else if ( -to >= 0 && -to < NUMPLAYERTEAMS )
	c_strcpy( Teams[-to].name, buf );
      else switch ( to )
	{
	case MSG_ALL:
	  c_strcpy( "All", buf );
	  break;
	case MSG_GOD:
	  c_strcpy( "GOD", buf );
	  break;
	case MSG_IMPLEMENTORS:
	  c_strcpy( "Implementors", buf );
	  break;
	default:
	  buf[0] = EOS;
	  break;
	}
      
    }
  
  /* Got a target, parse it. */
  delblanks( buf );
  upper( buf );
  if ( alldig( buf ) == TRUE )
    {
      /* All digits means a ship number. */
      i = 0;
      safectoi( &j, buf, i );		/* ignore status */
      if ( j < 1 || j > MAXSHIPS )
	{
	  c_putmsg( "No such ship.", MSG_LIN2 );
	  return;
	}
      if ( Ships[j].status != SS_LIVE )
	{
	  c_putmsg( nf, MSG_LIN2 );
	  return;
	}
      to = j;
    }
  else switch ( buf[0] )
    {
    case 'A':
    case 'a':
      to = MSG_ALL;
      break;
    case 'G':
    case 'g':
      to = MSG_GOD;
      break;
    case 'I':
    case 'i':
      to = MSG_IMPLEMENTORS;
      break;
    default:
      /* Check for a team character. */
      for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	if ( buf[0] == Teams[i].teamchar || buf[0] == (char)tolower(Teams[i].teamchar) )
	  break;
      if ( i >= NUMPLAYERTEAMS )
	{
	  c_putmsg( huh, MSG_LIN2 );
	  return;
	}
      to = -i;
      break;
    }
  
  /* Now, construct a header for the selected target. */
  c_strcpy( "Message to ", buf );
  if ( to > 0 && to <= MAXSHIPS )
    {
      if ( Ships[to].status != SS_LIVE )
	{
	  c_putmsg( nf, MSG_LIN2 );
	  return;
	}
      appship( to, buf );
      appchr( ':', buf );
    }
  else if ( -to >= 0 && -to < NUMPLAYERTEAMS )
    {
      appstr( Teams[-to].name, buf );
      appstr( "s:", buf );
    }
  else switch ( to ) 
    {
    case MSG_ALL:
      appstr( "everyone:", buf );
      break;
    case MSG_GOD:
      appstr( "GOD:", buf );
      break;
    case MSG_IMPLEMENTORS:
      appstr( "The Implementors:", buf );
      break;
    default:
      c_putmsg( huh, MSG_LIN2 );
      return;
      break;
    }
  
  if ( ! terse )
    appstr( " (ESCAPE to abort)", buf );
  
  c_putmsg( buf, MSG_LIN1 );
  cdclrl( MSG_LIN2, 1 );
  
  if ( ! editing )
    msg[0] = EOS;
  
  if ( to == MSG_IMPLEMENTORS )
    i = MSGMAXLINE;
  else
    i = MESSAGE_SIZE;
  
  append_flg = TRUE;
  while (append_flg == TRUE) {
  append_flg = FALSE;
  do_append_flg = TRUE;
  msg[0] = EOS;
  if ( cdgetp( ">", MSG_LIN2, 1, TERMS, msg, i, 
	       &append_flg, do_append_flg, TRUE ) != TERM_ABORT )
    if ( to != MSG_IMPLEMENTORS )
      stormsg( from, to, msg );
    else
      {
	/* Handle a message to the Implementors. */
	c_strcpy( "Communique from ", buf );
	if ( from > 0 && from <= MAXSHIPS )
	  {
	    appstr( Ships[from].alias, buf );
	    appstr( " on board ", buf );
	    appship( from, buf );
	  }
	else if ( from == MSG_GOD )
	  appstr( "GOD", buf );
	else
	  {
	    appchr( '(', buf );
	    appint( from, buf );
	    appchr( ')', buf );
	  }
	stormsg( from, MSG_IMPLEMENTORS, msg ); /* GOD == IMP */
	/* log it to the logfile too */
	clog("MESSAGE TO IMPLEMENTORS: %s: %s", buf, msg);
      }
    } /* end while loop */ 
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}

/* cmpplanet - compare planet names based on index passed via qsort() */
int cmpplanet(void *cmp1, void *cmp2)
{
  register int *icmp1, *icmp2;
  
  icmp1 = (int *) cmp1;
  icmp2 = (int *) cmp2;
  
  return(strcmp(Planets[*icmp1].name, Planets[*icmp2].name));
}

/*  sortplanets - sort planets by name */
/*  SYNOPSIS */
/*    int sv(NUMPLANETS) */
/*    sortplanets( sv ) */
/* This routine ASSUMES that the sort vector is initialized, */
/* for the reason that it is fastest to sort a list that is */
/* already sorted. */
void sortplanets( int sv[] )
{
  qsort(&sv[1], NUMPLANETS, sizeof(int), 
	(int (*)(const void *, const void *))cmpplanet);
  
  return;
  
}

/* cmpuser - compare users based on skill */
int cmpuser(void *cmp1, void *cmp2)
{
  register int *icmp1, *icmp2;

  icmp1 = (int *) cmp1;
  icmp2 = (int *) cmp2;
  
  if (Users[*icmp1].rating > Users[*icmp2].rating)
    return(-1);
  else if (Users[*icmp1].rating < Users[*icmp2].rating)
    return(1);
  else
    return(0);
}

/*  sortusers - sort users by skill */
/* This routine ASSUMES that the sort vector is initialized, */
/* for the reason that it is fastest to sort a list that is */
/* already sorted. */
void sortusers( int uv[], int numentries )
{
  qsort(uv, numentries, sizeof(int), 
	(int (*)(const void *, const void *))cmpuser);
  
  return;
  
}

/*  spwar - test whether a ship is at war with a planet */
/*  SYNOPSIS */
/*    int atwar, spwar */
/*    int snum, pnum */
/*    atwar = spwar( snum, pnum ) */
int spwar( int snum, int pnum )
{
  
  
  if ( ! Planets[pnum].real )
    return ( FALSE );		/* can't be at war unless it's real */
  else if ( Planets[pnum].type == PLANET_SUN )
    return ( TRUE );		/* always at war with suns */
  else if ( Planets[pnum].type == PLANET_MOON )
    return ( FALSE );		/* never at war with moons */
  else if ( Planets[pnum].armies <= 0 )
    return ( FALSE );		/* can't have war without armies */
  else switch ( Planets[pnum].team )	/* jet added breaks */
    {
    case TEAM_FEDERATION:
    case TEAM_ROMULAN:
    case TEAM_KLINGON:
    case TEAM_ORION:
      if ( Planets[pnum].team == Ships[snum].team )
	return ( FALSE );
      else
	return ( Ships[snum].war[Planets[pnum].team] );
      break;
    default:
      return ( Ships[snum].srpwar[pnum] );
    }
  
  /*    return ( TRUE );			/* can't get here... */
  
}


/*  stillalive - determine if a ship is still alive */
/*  SYNOPSIS */
/*    int flag, stillalive */
/*    int snum */
/*    flag = stillalive( snum ) */
int stillalive( int snum )
{
  if (snum < 0 || snum > MAXSHIPS)
    return(TRUE);

  /* Look for religious trouble or the "closed" sign in the window. */
  if ( Users[Ships[snum].unum].ooptions[OOPT_SHITLIST] )
    {
      if ( Ships[snum].status == SS_LIVE )
	killship( snum, KB_SHIT );
      return ( FALSE );
    }
  if ( ConqInfo->closed && ! Users[Ships[snum].unum].ooptions[OOPT_PLAYWHENCLOSED] )
    {
      if ( Ships[snum].status == SS_LIVE )
	killship( snum, KB_EVICT );
      return ( FALSE );
    }
  
  if ( Ships[snum].status == SS_RESERVED || Ships[snum].status == SS_ENTERING )
    return ( TRUE );
  
  return ( Ships[snum].status == SS_LIVE );
  
}


/*  stormsg - store a message in the message buffer (DOES LOCKING) */
/*  SYNOPSIS */
/*    int from, to */
/*    char msg() */
/*    stormsg( from, to, msg ) */
void stormsg( int from, int to, char *msg )
{
  int nlastmsg, i;
  
				/* don't do this if invalid common block */
  if (*CBlockRevision != COMMONSTAMP)
    return;

  PVLOCK(&ConqInfo->lockmesg);
  nlastmsg = modp1( ConqInfo->lastmsg + 1, MAXMESSAGES );
  stcpn( msg, Msgs[nlastmsg].msgbuf, MESSAGE_SIZE );
  Msgs[nlastmsg].msgfrom = from;
  Msgs[nlastmsg].msgto = to;
  ConqInfo->lastmsg = nlastmsg;
  
  /* Remove allowable last message restrictions. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( nlastmsg == Ships[i].alastmsg )
      Ships[i].alastmsg = LMSG_READALL;
  PVUNLOCK(&ConqInfo->lockmesg);
  
  return;
  
}


/*  usefuel - consume a quantity of matter-antimatter devices */
/*  SYNOPSIS */
/*    int snum */
/*    real fuel */
/*    int ok, usefuel, weapon */
/*    ok = usefuel( snum, fuel, weapon ) */
int usefuel( int snum, real fuel, int weapon )
{
  if ( fuel <= 0.0 )
    return ( FALSE );
  if ( weapon )
    {
      if ( Ships[snum].wfuse > 0 )
	return ( FALSE );
    }
  else
    {
      if ( Ships[snum].efuse > 0 )
	{
	  Ships[snum].dwarp = 0.0;
	  return ( FALSE );
	}
    }

  Ships[snum].fuel = Ships[snum].fuel - fuel;

  if ( Ships[snum].fuel < 0.0 )
    {
      /* When you're out of gas, you're out of fun... */
      Ships[snum].fuel = 0.0;
      Ships[snum].cloaked = FALSE;
      Ships[snum].dwarp = 0.0;
      return ( FALSE );
    }
  else if ( Ships[snum].fuel > 999.0 )
    Ships[snum].fuel = 999.0;
  
  /* Temperature. */
  if ( weapon )
    {
      Ships[snum].wtemp += ((fuel * TEMPFUEL_FAC) / weaeff ( snum ));
      if ( Ships[snum].wtemp < 0.0 )
	Ships[snum].wtemp = 0.0;
      else if ( Ships[snum].wtemp >= 100.0 )
	if ( rnd() < WEAPON_DOWN_PROB )
	  {
	    Ships[snum].wfuse = rndint( MIN_DOWN_FUSE, MAX_DOWN_FUSE );
	    if ( ! Ships[snum].options[OPT_TERSE] )
	      stormsg( MSG_COMP, snum, "Weapons overload." );
	  }
    }
  else
    {
      Ships[snum].etemp = Ships[snum].etemp + fuel * TEMPFUEL_FAC / engeff( snum );
      if ( Ships[snum].etemp < 0.0 )
	Ships[snum].etemp = 0.0;
      else if ( Ships[snum].etemp >= 100.0 )
	if ( rnd() < ENGINE_DOWN_PROB )
	  {
	    Ships[snum].efuse = rndint( MIN_DOWN_FUSE, MAX_DOWN_FUSE );
	    if ( ! Ships[snum].options[OPT_TERSE] )
	      stormsg( MSG_COMP, snum, "Engines super-heated." );
	  }
    }
  
  return ( TRUE );
  
}


/*  zeroeverything - initialize the common block to zeros */
/*  SYNOPSIS */
/*    zeroeverything */
void zeroeverything(void)
{
  
  zero_common();
  
  return;
  
}


/*  zeroship - turn OFF a ship and its torpedos */
/*  SYNOPSIS */
/*    int snum */
/*    zeroship( snum ) */
void zeroship( int snum )
{
  int i, j;
  
  Ships[snum].status = SS_OFF;
  Ships[snum].killedby = 0;
  Ships[snum].unum = 0;
  Ships[snum].team = 0;
  Ships[snum].pid = 0;
  Ships[snum].x = 0.0;
  Ships[snum].y = 0.0;
  Ships[snum].dx = 0.0;
  Ships[snum].dy = 0.0;
  Ships[snum].head = 0.0;
  Ships[snum].dhead = 0.0;
  Ships[snum].warp = 0.0;
  Ships[snum].dwarp = 0.0;
  Ships[snum].lock = 0;
  Ships[snum].shup = FALSE;
  Ships[snum].shields = 0.0;
  Ships[snum].kills = 0.0;
  Ships[snum].damage = 0.0;
  Ships[snum].fuel = 0.0;
  Ships[snum].etemp = 0.0;
  Ships[snum].wtemp = 0.0;
  Ships[snum].wfuse = 0;
  Ships[snum].efuse = 0;
  Ships[snum].weapalloc = 0;
  Ships[snum].engalloc = 0;
  Ships[snum].armies = 0;
  Ships[snum].rmode = FALSE;
  Ships[snum].cloaked = FALSE;
  for ( i = 0; i < MAXOPTIONS; i = i + 1 )
    Ships[snum].options[i] = FALSE;
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      Ships[snum].rwar[i] = FALSE;
      Ships[snum].war[i] = FALSE;
      Ships[snum].scanned[i] = 0;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    Ships[snum].srpwar[i] = FALSE;
  Ships[snum].sdfuse = 0;
  Ships[snum].lastmsg = 0;
  Ships[snum].alastmsg = 0;
  Ships[snum].map = FALSE;
  Ships[snum].towing = 0;
  Ships[snum].towedby = 0;
  Ships[snum].lastblast = 0.0;
  Ships[snum].lastphase = 0.0;
  Ships[snum].pfuse = 0;
  Ships[snum].talert = FALSE;
  Ships[snum].robot = FALSE;
  Ships[snum].action = 0;
  for ( i = 0; i < SIZEUSERPNAME; i = i + 1 )
    Ships[snum].alias[i] = EOS;
  Ships[snum].ctime = 0;
  Ships[snum].etime = 0;
  Ships[snum].cacc = 0;
  Ships[snum].eacc = 0;
  
  for ( i = 0; i < MAXTORPS; i = i + 1 )
    {
      Ships[snum].torps[i].status = TS_OFF;
      Ships[snum].torps[i].fuse = 0;
      Ships[snum].torps[i].x = 0.0;
      Ships[snum].torps[i].y = 0.0;
      Ships[snum].torps[i].dx = 0.0;
      Ships[snum].torps[i].dy = 0.0;
      Ships[snum].torps[i].mult = 0.0;
      for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
	Ships[snum].torps[i].war[j] = FALSE;
    }
  
  return;
  
}

				/* convert a KP key into an angle */
int KPAngle(int ch, real *angle)
{
  int rv;
  
  switch (ch)
    {
    case KEY_HOME:
    case KEY_A1:		/* KP upper left */
      *angle = 135.0;
      rv = TRUE;
      break;
    case KEY_PPAGE:
    case KEY_A3:		/* KP upper right */
      *angle = 45.0;
      rv = TRUE;
      break;
    case KEY_END:
    case KEY_C1:		/* KP lower left */
      *angle = 225.0;
      rv = TRUE;
      break;
    case KEY_NPAGE:
    case KEY_C3:		/* KP lower right */
      *angle = 315.0;
      rv = TRUE;
      break;
    case KEY_UP:		/* up arrow */
      *angle = 90.0;
      rv = TRUE;
      break;
    case KEY_DOWN:		/* down arrow */
      *angle = 270.0;
      rv = TRUE;
      break;
    case KEY_LEFT:		/* left arrow */
      *angle = 180.0;
      rv = TRUE;
      break;
    case KEY_RIGHT:		/* right arrow */
      *angle = 0.0;
      rv = TRUE;
      break;
    default:
      rv = FALSE;
      break;
    }
  
  return(rv);
}

				/* convert a KP key into a 'dir' key */
int KP2DirKey(int *ch)
{
  int rv;
  char cch;
  
  switch (*ch)
    {
    case KEY_HOME:
    case KEY_A1:		/* KP upper left */
      cch = 'q';
      rv = TRUE;
      break;
    case KEY_PPAGE:
    case KEY_A3:		/* KP upper right */
      cch = 'e';
      rv = TRUE;
      break;
    case KEY_END:
    case KEY_C1:		/* KP lower left */
      cch = 'z';
      rv = TRUE;
      break;
    case KEY_NPAGE:
    case KEY_C3:		/* KP lower right */
      cch = 'c';
      rv = TRUE;
      break;
    case KEY_UP:		/* up arrow */
      cch = 'w';
      rv = TRUE;
      break;
    case KEY_DOWN:		/* down arrow */
      cch = 'x';
      rv = TRUE;
      break;
    case KEY_LEFT:		/* left arrow */
      cch = 'a';
      rv = TRUE;
      break;
    case KEY_RIGHT:		/* right arrow */
      cch = 'd';
      rv = TRUE;
      break;
    default:
      cch = (char)0;
      rv = FALSE;
      break;
    }
  
  if ((int)cch != 0)
    *ch = (char)cch;

  return(rv);
}

