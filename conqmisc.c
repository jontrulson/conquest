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
	appstr( pname[-kb], buf );
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
      i = steam[snum];
      if ( i >= 0 && i < NUMTEAMS )
	ch = chrteams[i];
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
  
  
  from = msgfrom[msgnum];
  to = msgto[msgnum];
  
  if (from == 0 && to == 0)
    {				/* uninitialized msgs */
      return(FALSE);		/* no point in reading it */
    }

  /* If we're GOD, we can read it. unless it's a COMP MSG*/
  if ( snum == MSG_GOD && from != MSG_COMP)
    return ( TRUE );
  
  /* It's to us. */
  if ( to == snum )
    {				/* extra check to see if from is a robot
				   and to is a valid ship */
      if (conf_NoRobotMsgs == TRUE && srobot[from] == TRUE && 
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
      if (conf_NoRobotMsgs == TRUE && srobot[from] == TRUE)
	{			/* see if it's a robot, if so ignore */
	  return(FALSE);
	}
      
      /* We can only read team messages if we're not self-war. */
      if ( ( -to == steam[snum] ) && ! selfwar(snum) )
	{
	  /* Planet alert for our team. */
	  if ( -from > 0 && -from <= NUMPLANETS )
	    return ( soption[snum][OPT_INTRUDERALERT] );
	  else
	    return ( TRUE );
	}
      
      /* See if we are allowed to read GOD messages. */
      if ( to == MSG_GOD || from == MSG_GOD || to == MSG_IMPLEMENTORS )
	return ( uooption[suser[snum]][OOPT_GODMSG] );
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
  *col = round( (cmaxcol-STAT_COLS)/2 + (x-cenx) / scale * WIDTH_FAC ) +
    STAT_COLS;
  
  *lin = round( (DISPLAY_LINS/2+1) - (y-ceny) / scale );
  if ( *lin < 0 || *lin > DISPLAY_LINS || *col <= STAT_COLS || *col > cmaxcol )
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
  *dlock = -PNUM_MURISAK;
  
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( preal[i] )
      if ( parmies[i] > 0 && pteam[i] != TEAM_NOTEAM )
	{
	  taste = parmies[i] * BOMBARD_KILLS / dist(*dx, *dy, px[i], py[i]);
	  if ( taste > tastiness )
	    {
	      tastiness = taste;
	      *dlock = -i;
	    }
	}
  
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] == SS_LIVE )
      {
	taste = ( 1.0 +
		 skills[i] * KILLS_KILLS +
		 sarmies[i] * ARMY_KILLS ) / dist(*dx, *dy, sx[i], sy[i]);
	if ( taste > tastiness )
	  {
	    tastiness = taste;
	    *dlock = i;
	  }
      }
  
  if ( *dlock < 0 )
    *dhead = angle( *dx, *dy, px[-*dlock], py[-*dlock] );
  else if ( *dlock > 0 )
    *dhead = angle( *dx, *dy, sx[*dlock], sy[*dlock] );
  
  return;
  
}


/*  doomsday - start the doomsday device */
/*  SYNOPSIS */
/*    doomsday */
void doomsday(void)
{
  
  *dhead = rnduni( 0.0, 360.0 );
  *dx = DOOMSDAY_START_DIST * cosd(*dhead);
  *dy = DOOMSDAY_START_DIST * sind(*dhead);
  doomfind();
  *dstatus = DS_LIVE;
  
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
    if ( preal[i] &&
	( dist( sx[snum], sy[snum], px[i], py[i] ) <= ORBIT_DIST ) )
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
  
  PVLOCK(lockword);
  *snum = -1;
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] == SS_OFF )
      {
	*snum = i;
	zeroship( *snum );
	sstatus[*snum] = SS_RESERVED;
	slastmsg[*snum] = LMSG_NEEDINIT;
	ssdfuse[*snum] = -TIMEOUT_PLAYER;
	sctime[*snum] = 0;
	setime[*snum] = 0;
	scacc[*snum] = 0;
	seacc[*snum] = 0;
	break;
      }
  PVUNLOCK(lockword);
  
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
	if ( i != snum && sstatus[i] == SS_LIVE )
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
		valid = ( steam[i] == steam[snum] &&
			 ! satwar(snum, i) );
		break;
	      default:
		return ( FALSE );		/* this can't happen */
	      }
	    if ( valid )
	      {
#ifdef WARP0CLOAK
		if (scloaked[i] && swarp[i] == 0.0 && 
		    satwar(snum, i) &&
		    srobot[snum])
		  continue; /* nobody here but us chickens... */
#endif /* WARP0CLOAK */
		td = dist(sx[snum], sy[snum], sx[i], sy[i]);
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
      switch ( steam[snum] )
	{
	case TEAM_FEDERATION:
	  *sorpnum = homeplanet[TEAM_FEDERATION];
	  break;
	case TEAM_ROMULAN:
	  *sorpnum = homeplanet[TEAM_ROMULAN];
	  break;
	case TEAM_KLINGON:
	  *sorpnum = homeplanet[TEAM_KLINGON];
	  break;
	case TEAM_ORION:
	  *sorpnum = homeplanet[TEAM_ORION];
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
	  if ( ! preal[i] )
	    continue; /* jet next;*/
	  /* Ignore suns and moons. */
	  if ( ptype[i] == PLANET_SUN || ptype[i] == PLANET_MOON )
	    continue; 

	  valid = ( pscanned[i][steam[snum]] &&
		    pteam[i] != steam[snum] );

	  /* Handle army threshold logic. */
	  if ( valid )
	    valid = ( parmies[i] >= count );

	  if ( valid )
	    {
	      ta = parmies[i];
	      tu = puninhabtime[i];
	      td = dist(sx[snum], sy[snum], px[i], py[i]);
	      
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
      for ( i = 0; i < NUMTEAMS; i = i + 1 )
	if ( swar[snum][i] )
	  {
	    peaceful = FALSE;
	    break;
	  }
      
      /* Loop through the planets. */
      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
	{
	  /* Only can look for "real" planets. */
	  if ( ! preal[i] )
	    continue; 
	  /* Ignore suns and moons. */
	  if ( ptype[i] == PLANET_SUN || ptype[i] == PLANET_MOON )
	    continue; 
	  switch ( token )
	    {
	    case SPECIAL_ARMYPLANET:
	      valid = ( pteam[i] == steam[snum] );
	      break;
	    case SPECIAL_ENEMYPLANET:
	      valid = ( ! pscanned[i][steam[snum]] ||
		       ( parmies[i] > 0 &&
			spwar( snum, i ) &&
			ptype[i] != PLANET_MOON ) );
	      break;
	    case SPECIAL_FUELPLANET:
	      valid = ( ( pscanned[i][steam[snum]] || peaceful ) &&
		       ! spwar( snum, i ) &&
		       parmies[i] > 0 &&
		       ptype[i] == PLANET_CLASSM );
	      break;
	    case SPECIAL_PLANET:
	      valid = TRUE;
	      break;
	    case SPECIAL_REPAIRPLANET:
	      valid = ( ( pscanned[i][steam[snum]] || peaceful ) &&
		       ! spwar( snum, i ) &&
		       parmies[i] > 0 &&
		       ptype[i] != PLANET_MOON );
	      break;
	    case SPECIAL_TEAMPLANET:
	      valid = ( pteam[i] == steam[snum] );
	      break;
	    default:
	      return ( FALSE );		/* this can't happen */
	    }
	  /* Handle army threshold logic. */
	  if ( valid )
	    switch ( token )
	      {
	      case SPECIAL_ARMYPLANET:
		valid = ( ( parmies[i] - 3 ) >= count );
		break;
	      case SPECIAL_PLANET:
	      case SPECIAL_ENEMYPLANET:
		valid = ( ! pscanned[i][steam[snum]] ||
			 parmies[i] >= count );
		break;
	      case SPECIAL_FUELPLANET:
	      case SPECIAL_REPAIRPLANET:
	      case SPECIAL_TEAMPLANET:
		valid = ( parmies[i] >= count );
		break;
	      default:
		return ( FALSE );	/* this can't happen */
	      }
	  if ( valid )
	    {
	      td = dist(sx[snum], sy[snum], px[i], py[i]);
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
  
  speed = swarp[snum] * MM_PER_SEC_PER_WARP * ITER_SECONDS;
  sdx[snum] = speed * cosd(shead[snum]);
  sdy[snum] = speed * sind(shead[snum]);
  
  return;
  
}


/*  gunum - get the user number of the specified user */
/*  SYNOPSIS */
/*    int truth, gunum */
/*    int unum */
/*    char lname() */
/*    truth = gunum( unum, lname ) */
int gunum( int *unum, char *lname )
{
  int i;
  
  *unum = -1;
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    if ( ulive[i] )
      if ( strcmp( lname, cuname[i] ) == 0 )
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
  int i, j, unum, lin, col, fline, lline, thistptr;
  int ch;
  char *hd0="C O N Q U E S T   U S E R   H I S T O R Y";
  
  /* Do some screen setup. */
  cdclear();
  fline = 1;
  lline = MSG_LIN1 - 1;
  cprintf(fline,0,ALIGN_CENTER,"#%d#%s",LabelColor, hd0);
  fline = fline + 2;
  
  thistptr = -1;			/* force an update the first time */
  while (TRUE) /* repeat */
    {
      if ( ! godlike )
	if ( ! stillalive( csnum ) )
	  break;
      if ( thistptr != *histptr )
	{
	  /* Need to update the display */
	  thistptr = *histptr;
	  lin = fline;
	  col = 8;
	  cdclrl( fline, lline - fline + 1 );
	  
	  /*GAG	  i = modp1( thistptr + 1, MAXHISTLOG );	/* gag... */
	  /*	    i = MAXHISTLOG - 1;	                        /* gag... */
	  
	  i = thistptr + 1;
	  for ( j = 0; j < MAXHISTLOG; j++ )
	    {
	      i = modp1( i - 1, MAXHISTLOG );
	      unum = histunum[i];
	      
	      if ( unum < 0 || unum >= MAXUSERS )
		continue; /* jet next;*/
	      if ( ! ulive[unum] )
		continue; /* jet next */
	      cprintf( lin,col,ALIGN_NONE, "#%d#%-12s %17s", YellowLevelColor,
			cuname[unum], histlog[i] );
	      lin++;
	      if ( lin > lline )
		{
		  col = 43;
		  lin = fline;
		}
	    }
	}
      
      putpmt( "--- press space when done ---", MSG_LIN2 );
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
  
  /* Zero EVERYTHING. */
  zeroeverything();
  
  /* Twiddle the lockword. */
  PVUNLOCK(lockword);
  PVLOCK(lockword);
  
  /* Turn off the universe. */
  *closed = TRUE;
  
  /* reset the lockwords if using semaphores */
#if defined(USE_PVLOCK) && defined(USE_SEMS)
  *lockword = 0;
  *lockmesg = 0;
#endif

  /* Zero team stats. */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    for ( j = 0; j < MAXTSTATS; j = j + 1 )
      tstats[i][j] = 0;
  
  /* De-register all users. */
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    ulive[i] = FALSE;
  
  *celapsedseconds = 0;
  *ccpuseconds = 0;
  *delapsedseconds = 0;
  *dcpuseconds = 0;
  *relapsedseconds = 0;
  *rcpuseconds = 0;
  *raccum = 0;
  
  stcpn( "never", lastupchuck, DATESIZE );
  getdandt( inittime );
  getdandt( conqtime );
  stcpn( "GOD", conqueror, MAXUSERPNAME );
  stcpn( "self ruled", conqteam, MAXTEAMNAME );
  stcpn( "Let there be light...", lastwords, MAXLASTWORDS );
  
  /* Un-twiddle the lockwords. */
  PVUNLOCK(lockword);
  PVUNLOCK(lockmesg);
  
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
  PVUNLOCK(lockword);
  
  PVLOCK(lockword);
  
  /* Driver. */
  *drivsecs = 0;
  
  /* Doomsday machine. */
  *dstatus = DS_OFF;
  *dtype = 0;				/* should have constants for this */
  *dx = 0.0;
  *dy = 0.0;
  *ddx = 0.0;
  *ddy = 0.0;
  *dhead = 0.0;
  *dlock = 0;
  stcpn( "Doomsday Machine", dname, MAXUSERPNAME );
  
  /* Set up initial armies on planets. */
  pteam[PNUM_SOL] = TEAM_NOTEAM;
  pteam[PNUM_EARTH] = TEAM_FEDERATION;
  pteam[PNUM_TELOS] = TEAM_FEDERATION;
  pteam[PNUM_OMEGA] = TEAM_FEDERATION;
  pteam[PNUM_SIRIUS] = TEAM_NOTEAM;
  pteam[PNUM_ROMULUS] = TEAM_ROMULAN;
  pteam[PNUM_REMUS] = TEAM_ROMULAN;
  pteam[PNUM_RHO] = TEAM_ROMULAN;
  pteam[PNUM_KEJELA] = TEAM_NOTEAM;
  pteam[PNUM_KLINGUS] = TEAM_KLINGON;
  pteam[PNUM_LEUDUS] = TEAM_KLINGON;
  pteam[PNUM_TARSUS] = TEAM_KLINGON;
  pteam[PNUM_BETELGEUSE] = TEAM_NOTEAM;
  pteam[PNUM_ORION] = TEAM_ORION;
  pteam[PNUM_OBERON] = TEAM_ORION;
  pteam[PNUM_UMBRIEL] = TEAM_ORION;
  pteam[PNUM_MURISAK] = TEAM_NOTEAM;
  pteam[PNUM_JANUS] = TEAM_SELFRULED;
  pteam[PNUM_SERITIL] = TEAM_SELFRULED;
  pteam[PNUM_ELAS] = TEAM_SELFRULED;
  pteam[PNUM_SHERMAN] = TEAM_SELFRULED;
  pteam[PNUM_CHERON] = TEAM_SELFRULED;
  pteam[PNUM_DAKEL] = TEAM_SELFRULED;
  pteam[PNUM_OLDAR] = TEAM_SELFRULED;
  pteam[PNUM_SARAC] = TEAM_SELFRULED;
  pteam[PNUM_EMINIAR] = TEAM_SELFRULED;
  pteam[PNUM_VENAR] = TEAM_SELFRULED;
  pteam[PNUM_DYNEB] = TEAM_SELFRULED;
  pteam[PNUM_XIDEX] = TEAM_SELFRULED;
  pteam[PNUM_RIGELB] = TEAM_SELFRULED;
  
  pteam[PNUM_SYRINX] = TEAM_NOTEAM;
  pteam[PNUM_ALTAIR] = TEAM_SELFRULED;
  pteam[PNUM_HELL] = TEAM_SELFRULED;
  pteam[PNUM_JINX] = TEAM_SELFRULED;
  pteam[PNUM_LUNA] = TEAM_NOTEAM;
  
  pteam[PNUM_GHOST1] = TEAM_NOTEAM;
  pteam[PNUM_GHOST2] = TEAM_NOTEAM;
  pteam[PNUM_GHOST3] = TEAM_NOTEAM;
  pteam[PNUM_GHOST4] = TEAM_NOTEAM;

				/* set up teams for extra planets */
  if (NUM_EXTRAPLANETS > 0)
    {				/* there are extra planets
				   they are numbered starting at
				   1 (+ NUM_BASEPLANETS) */
      for (i=1; i<= NUM_EXTRAPLANETS; i++) 
	{
	  pteam[NUM_BASEPLANETS + i] = TEAM_SELFRULED;
	}
    }

  
  parmies[PNUM_SOL] = rndint(80, 180);
  parmies[PNUM_EARTH] = 50;
  parmies[PNUM_TELOS] = 50;
  parmies[PNUM_OMEGA] = 50;
  parmies[PNUM_SIRIUS] = rndint(80, 180);
  parmies[PNUM_ROMULUS] = 50;
  parmies[PNUM_REMUS] = 50;
  parmies[PNUM_RHO] = 50;
  parmies[PNUM_KEJELA] = rndint(80, 180);
  parmies[PNUM_KLINGUS] = 50;
  parmies[PNUM_LEUDUS] = 50;
  parmies[PNUM_TARSUS] = 50;
  parmies[PNUM_BETELGEUSE] = rndint(80, 180);
  parmies[PNUM_ORION] = 50;
  parmies[PNUM_OBERON] = 50;
  parmies[PNUM_UMBRIEL] = 50;
  parmies[PNUM_MURISAK] = rndint(80, 180);
  parmies[PNUM_JANUS] = 25;
  parmies[PNUM_SERITIL] = 25;
  parmies[PNUM_ELAS] = 25;
  parmies[PNUM_SHERMAN] = 25;
  parmies[PNUM_CHERON] = 25;
  parmies[PNUM_DAKEL] = 25;
  parmies[PNUM_OLDAR] = 25;
  parmies[PNUM_SARAC] = 25;
  parmies[PNUM_EMINIAR] = 25;
  parmies[PNUM_VENAR] = 25;
  parmies[PNUM_DYNEB] = 25;
  parmies[PNUM_XIDEX] = 25;
  parmies[PNUM_RIGELB] = 25;
  
  /* The rest don't matter since you don't have to conquer them. */
  parmies[PNUM_SYRINX] = rndint(100, 200);
  parmies[PNUM_ALTAIR] = rndint(80, 120);
  parmies[PNUM_HELL] = rndint(50, 100);
  parmies[PNUM_JINX] = rndint(100, 200);
  parmies[PNUM_LUNA] = 0;
  
  parmies[PNUM_GHOST1] = 0;
  parmies[PNUM_GHOST2] = 0;
  parmies[PNUM_GHOST3] = 0;
  parmies[PNUM_GHOST4] = 0;
  
  /* Set up the pscanned array so that each team has scanned its own planets. */
  for ( i = 1 ; i <= NUMPLANETS; i = i + 1 )
    {
      puninhabtime[i] = 0;		/* planets start out inhabitable */
      for ( j = 0; j < NUMTEAMS; j = j + 1 )
	pscanned[i][j] = FALSE;
    }
  
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    {
      /* Each team has scanned its own planets. */
      for ( j = 0; j < 3; j = j + 1 )
	pscanned[teamplanets[i][j]][i] = TRUE;
      
      couptime[i] = 0;		/* time left to coup starts at zero. */
      tcoupinfo[i] = FALSE;		/* don't know coup time */
    }
  
  /* Un-twiddle the lockword. */
  PVUNLOCK(lockword);
  
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
      msgbuf[i][0] = EOS;
      msgfrom[i] = 0;
      msgto[i] = 0;
    }
  *lastmsg = 0;
  *glastmsg = *lastmsg;
  
  return;
  
}


/*  initplanets - initialize the planets */
/*  SYNOPSIS */
/*    initplanets */
void initplanets(void)
{
  /* SETPLANET( name, pnum ) */
#define SETPLANET(x, y)  stcpn( x, pname[y], MAXPLANETNAME ) 
  
  int i; 
  real orbang, orbvel;
  
  /* Twiddle the lockword. */
  PVUNLOCK(lockword);
  PVLOCK(lockword);
  
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
  
  ptype[PNUM_SOL] = PLANET_SUN;
  ptype[PNUM_EARTH] = PLANET_CLASSM;
  ptype[PNUM_TELOS] = PLANET_DEAD;
  ptype[PNUM_OMEGA] = PLANET_DEAD;
  ptype[PNUM_SIRIUS] = PLANET_SUN;
  ptype[PNUM_ROMULUS] = PLANET_CLASSM;
  ptype[PNUM_REMUS] = PLANET_DEAD;
  ptype[PNUM_RHO] = PLANET_DEAD;
  ptype[PNUM_KEJELA] = PLANET_SUN;
  ptype[PNUM_KLINGUS] = PLANET_CLASSM;
  ptype[PNUM_LEUDUS] = PLANET_DEAD;
  ptype[PNUM_TARSUS] = PLANET_DEAD;
  ptype[PNUM_BETELGEUSE] = PLANET_SUN;
  ptype[PNUM_ORION] = PLANET_CLASSM;
  ptype[PNUM_OBERON] = PLANET_DEAD;
  ptype[PNUM_UMBRIEL] = PLANET_DEAD;
  ptype[PNUM_MURISAK] = PLANET_SUN;
  ptype[PNUM_JANUS] = PLANET_CLASSM;
  ptype[PNUM_SERITIL] = PLANET_DEAD;
  ptype[PNUM_ELAS] = PLANET_CLASSM;
  ptype[PNUM_SHERMAN] = PLANET_CLASSM;
  ptype[PNUM_CHERON] = PLANET_DEAD;
  ptype[PNUM_DAKEL] = PLANET_CLASSM;
  ptype[PNUM_OLDAR] = PLANET_DEAD;
  ptype[PNUM_SARAC] = PLANET_CLASSM;
  ptype[PNUM_EMINIAR] = PLANET_DEAD;
  ptype[PNUM_VENAR] = PLANET_CLASSM;
  ptype[PNUM_DYNEB] = PLANET_DEAD;
  ptype[PNUM_XIDEX] = PLANET_CLASSM;
  ptype[PNUM_RIGELB] = PLANET_DEAD;
  ptype[PNUM_GHOST1] = PLANET_GHOST;
  ptype[PNUM_GHOST2] = PLANET_GHOST;
  ptype[PNUM_GHOST3] = PLANET_GHOST;
  ptype[PNUM_GHOST4] = PLANET_GHOST;
  ptype[PNUM_GHOST5] = PLANET_GHOST;
  
  ptype[PNUM_SYRINX] = PLANET_SUN;
  ptype[PNUM_LUNA] = PLANET_MOON;
  ptype[PNUM_ALTAIR] = PLANET_CLASSM;
  ptype[PNUM_HELL] = PLANET_DEAD;
  ptype[PNUM_JINX] = PLANET_CLASSM;
  
  stcpn( "class M planet", ptname[PLANET_CLASSM], MAXPTYPENAME );
  stcpn( "dead planet", ptname[PLANET_DEAD], MAXPTYPENAME );
  stcpn( "sun", ptname[PLANET_SUN], MAXPTYPENAME );
  stcpn( "moon", ptname[PLANET_MOON], MAXPTYPENAME );
  stcpn( "ghost planet", ptname[PLANET_GHOST], MAXPTYPENAME );
  stcpn( "class A planet", ptname[PLANET_CLASSA], MAXPTYPENAME );
  stcpn( "class O planet", ptname[PLANET_CLASSO], MAXPTYPENAME );
  stcpn( "class Z planet", ptname[PLANET_CLASSZ], MAXPTYPENAME );
  
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    {
      preal[i] = TRUE;	/* by default, you can see most planets */
      porbvel[i] = 0.0;
      porbrad[i] = 0;
      pprimary[i] = 0;
    }
  
  /* Can't see the ghost planets. */
  preal[PNUM_GHOST1] = FALSE;
  preal[PNUM_GHOST2] = FALSE;
  preal[PNUM_GHOST3] = FALSE;
  preal[PNUM_GHOST4] = FALSE;
  preal[PNUM_GHOST5] = FALSE;
  
  /* Set up the X-Y coordinates of the suns.  Start with Murisak. */
  /* at the center, then place the other eight suns relative to it, */
  /* then the radii of the planets. */
  px[PNUM_MURISAK] = 0.0;
  py[PNUM_MURISAK] = 0.0;
  pprimary[PNUM_MURISAK] = 0;	/* the only "stationary" object */
  porbrad[PNUM_MURISAK] = 0.0;
  porbang[PNUM_MURISAK] = 0.0;
  porbvel[PNUM_MURISAK] = 0.0;
  
  pprimary[PNUM_BETELGEUSE] = PNUM_MURISAK;
  porbrad[PNUM_BETELGEUSE] = 11000.0;
  porbang[PNUM_BETELGEUSE] = 45.0;
  porbvel[PNUM_BETELGEUSE] = 0.0;
  
  pprimary[PNUM_KEJELA] = PNUM_MURISAK;
  porbrad[PNUM_KEJELA] = 11000.0;
  porbang[PNUM_KEJELA] = 135.0;
  porbvel[PNUM_KEJELA] = 0.0;
  
  pprimary[PNUM_SIRIUS] = PNUM_MURISAK;
  porbrad[PNUM_SIRIUS] = 11000.0;
  porbang[PNUM_SIRIUS] = 225.0;
  porbvel[PNUM_SIRIUS] = 0.0;
  
  pprimary[PNUM_SOL] = PNUM_MURISAK;
  porbrad[PNUM_SOL] = 11000.0;
  porbang[PNUM_SOL] = 315.0;
  porbvel[PNUM_SOL] = 0.0;
  
  pprimary[PNUM_GHOST1] = PNUM_MURISAK;
  porbrad[PNUM_GHOST1] = 12000.0;
  porbang[PNUM_GHOST1] = 0.0;
  porbvel[PNUM_GHOST1] = 0.0;
  
  pprimary[PNUM_GHOST2] = PNUM_MURISAK;
  porbrad[PNUM_GHOST2] = 12000.0;
  porbang[PNUM_GHOST2] = 90.0;
  porbvel[PNUM_GHOST2] = 0.0;
  
  pprimary[PNUM_GHOST3] = PNUM_MURISAK;
  porbrad[PNUM_GHOST3] = 12000.0;
  porbang[PNUM_GHOST3] = 180.0;
  porbvel[PNUM_GHOST3] = 0.0;
  
  pprimary[PNUM_GHOST4] = PNUM_MURISAK;
  porbrad[PNUM_GHOST4] = 12000.0;
  porbang[PNUM_GHOST4] = 270.0;
  porbvel[PNUM_GHOST4] = 0.0;
  
  pprimary[PNUM_GHOST5] = PNUM_MURISAK;
  porbrad[PNUM_GHOST5] = 12000.0;
  porbang[PNUM_GHOST5] = 0.0;
  porbvel[PNUM_GHOST5] = 0.0;
  
  /* Murisak's planets. */
  pprimary[PNUM_JANUS] = PNUM_MURISAK;
  porbrad[PNUM_JANUS] = 2600.0;
  pprimary[PNUM_SERITIL] = PNUM_MURISAK;
  porbrad[PNUM_SERITIL] = 2600.0;
  pprimary[PNUM_ELAS] = PNUM_MURISAK;
  porbrad[PNUM_ELAS] = 2600.0;
  pprimary[PNUM_SHERMAN] = PNUM_MURISAK;
  porbrad[PNUM_SHERMAN] = 2600.0;
  pprimary[PNUM_CHERON] = PNUM_MURISAK;
  porbrad[PNUM_CHERON] = 2600.0;
  /* Sol's planets. */
  pprimary[PNUM_EARTH] = PNUM_SOL;
  porbrad[PNUM_EARTH] = 2600.0;
  pprimary[PNUM_TELOS] = PNUM_SOL;
  porbrad[PNUM_TELOS] = 2600.0;
  pprimary[PNUM_OMEGA] = PNUM_SOL;
  porbrad[PNUM_OMEGA] = 2600.0;
  /* Sirius' planets. */
  pprimary[PNUM_ROMULUS] = PNUM_SIRIUS;
  porbrad[PNUM_ROMULUS] = 2600.0;
  pprimary[PNUM_REMUS] = PNUM_SIRIUS;
  porbrad[PNUM_REMUS] = 2600.0;
  pprimary[PNUM_RHO] = PNUM_SIRIUS;
  porbrad[PNUM_RHO] = 2600.0;
  /* Kejela's planets. */
  pprimary[PNUM_KLINGUS] = PNUM_KEJELA;
  porbrad[PNUM_KLINGUS] = 2600.0;
  pprimary[PNUM_LEUDUS] = PNUM_KEJELA;
  porbrad[PNUM_LEUDUS] = 2600.0;
  pprimary[PNUM_TARSUS] = PNUM_KEJELA;
  porbrad[PNUM_TARSUS] = 2600.0;
  /* Betelgeuse's planets. */
  pprimary[PNUM_ORION] = PNUM_BETELGEUSE;
  porbrad[PNUM_ORION] = 2600.0;
  pprimary[PNUM_OBERON] = PNUM_BETELGEUSE;
  porbrad[PNUM_OBERON] = 2600.0;
  pprimary[PNUM_UMBRIEL] = PNUM_BETELGEUSE;
  porbrad[PNUM_UMBRIEL] = 2600.0;
  /* Side systems. */
  pprimary[PNUM_XIDEX] = PNUM_GHOST1;
  porbrad[PNUM_XIDEX] = 1150.0;
  pprimary[PNUM_RIGELB] = PNUM_GHOST1;
  porbrad[PNUM_RIGELB] = 1150.0;
  pprimary[PNUM_VENAR] = PNUM_GHOST2;
  porbrad[PNUM_VENAR] = 1150.0;
  pprimary[PNUM_DYNEB] = PNUM_GHOST2;
  porbrad[PNUM_DYNEB] = 1150.0;
  pprimary[PNUM_SARAC] = PNUM_GHOST3;
  porbrad[PNUM_SARAC] = 1150.0;
  pprimary[PNUM_EMINIAR] = PNUM_GHOST3;
  porbrad[PNUM_EMINIAR] = 1150.0;
  pprimary[PNUM_DAKEL] = PNUM_GHOST4;
  porbrad[PNUM_DAKEL] = 1150.0;
  pprimary[PNUM_OLDAR] = PNUM_GHOST4;
  porbrad[PNUM_OLDAR] = 1150.0;
  
  pprimary[PNUM_SYRINX] = PNUM_MURISAK;
  porbrad[PNUM_SYRINX] = 23000.0;
  
  pprimary[PNUM_LUNA] = PNUM_EARTH;
  porbrad[PNUM_LUNA] = 1250.0;
  
  pprimary[PNUM_ALTAIR] = PNUM_MURISAK;
  porbrad[PNUM_ALTAIR] = 6125.0;

  pprimary[PNUM_HELL] = PNUM_SYRINX;
  porbrad[PNUM_HELL] = 1900.0;
  pprimary[PNUM_JINX] = PNUM_SYRINX;
  porbrad[PNUM_JINX] = 2600.0;
  
  
  /* Set orbital angles and velocities for planets, and place them. */
  /* Murisak's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_JANUS] = orbang;
  porbvel[PNUM_JANUS] = orbvel;
  porbang[PNUM_CHERON] = mod360( orbang + 1.0/5.0*360.0 );
  porbvel[PNUM_CHERON] = orbvel;
  porbang[PNUM_SHERMAN] = mod360( orbang + 2.0/5.0*360.0 );
  porbvel[PNUM_SHERMAN] = orbvel;
  porbang[PNUM_ELAS] = mod360( orbang + 3.0/5.0*360.0 );
  porbvel[PNUM_ELAS] = orbvel;
  porbang[PNUM_SERITIL] = mod360( orbang + 4.0/5.0*360.0 );
  porbvel[PNUM_SERITIL] = orbvel;
  /* Sol's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_EARTH] = orbang;
  porbvel[PNUM_EARTH] = orbvel;
  porbang[PNUM_TELOS] = mod360( orbang + 2.0/3.0*360.0 );
  porbvel[PNUM_TELOS] = orbvel;
  porbang[PNUM_OMEGA] = mod360( orbang + 1.0/3.0*360.0 );
  porbvel[PNUM_OMEGA] = orbvel;
  /* Luna. */
  porbvel[PNUM_LUNA] = 12.0 * orbvel;
  porbang[PNUM_LUNA] = rnduni( 0.0, 360.0 );
  /* Sirius' planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_ROMULUS] = orbang;
  porbvel[PNUM_ROMULUS] = orbvel;
  porbang[PNUM_REMUS] = mod360( orbang + 2.0/3.0*360.0 );
  porbvel[PNUM_REMUS] = orbvel;
  porbang[PNUM_RHO] = mod360( orbang + 1.0/3.0*360.0 );
  porbvel[PNUM_RHO] = orbvel;
  /* Kejela's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_KLINGUS] = orbang;
  porbvel[PNUM_KLINGUS] = orbvel;
  porbang[PNUM_LEUDUS] = mod360( orbang + 2.0/3.0*360.0 );
  porbvel[PNUM_LEUDUS] = orbvel;
  porbang[PNUM_TARSUS] = mod360( orbang + 1.0/3.0*360.0 );
  porbvel[PNUM_TARSUS] = orbvel;
  /* Betelgeuse's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_ORION] = orbang;
  porbvel[PNUM_ORION] = orbvel;
  porbang[PNUM_OBERON] = mod360( orbang + 2.0/3.0*360.0 );
  porbvel[PNUM_OBERON] = orbvel;
  porbang[PNUM_UMBRIEL] = mod360( orbang + 1.0/3.0*360.0 );
  porbvel[PNUM_UMBRIEL] = orbvel;
  /* Side systems. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_XIDEX] = orbang;
  porbvel[PNUM_XIDEX] = orbvel;
  porbang[PNUM_RIGELB] = mod360( orbang + 1.0/2.0*360.0 );
  porbvel[PNUM_RIGELB] = orbvel;
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_VENAR] = orbang;
  porbvel[PNUM_VENAR] = orbvel;
  porbang[PNUM_DYNEB] = mod360( orbang + 1.0/2.0*360.0 );
  porbvel[PNUM_DYNEB] = orbvel;
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_SARAC] = orbang;
  porbvel[PNUM_SARAC] = orbvel;
  porbang[PNUM_EMINIAR] = mod360( orbang + 1.0/2.0*360.0 );
  porbvel[PNUM_EMINIAR] = orbvel;
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );
  porbang[PNUM_DAKEL] = orbang;
  porbvel[PNUM_DAKEL] = orbvel;
  porbang[PNUM_OLDAR] = mod360( orbang + 1.0/2.0*360.0 );
  porbvel[PNUM_OLDAR] = orbvel;
  
  porbvel[PNUM_SYRINX] = -6.314179;
  porbang[PNUM_SYRINX] = rnduni(0.0, 360.0);
  
  /* Syrinx's planets. */
  orbang = rnduni( 0.0, 360.0 );
  orbvel = rndnor( PLANET_ORBIT_FAC, 2.0 ) * ( rndint( 0, 1 ) * 2 - 1 );

  porbang[PNUM_ALTAIR] = rnduni(0.0, 360.0);
  porbvel[PNUM_ALTAIR] = -23.1;

  porbang[PNUM_HELL] = mod360( orbang + 2.0/3.0*360.0 );
  porbvel[PNUM_HELL] = orbvel;

  porbang[PNUM_JINX] = mod360( orbang + 1.0/3.0*360.0 );
  porbvel[PNUM_JINX] = 7.5;

  /* Place the planets in their proper orbits. */
  for ( i = NUMPLANETS; i > 0; i = i - 1 )
    if ( pprimary[i] != 0 )
      {
	px[i] = px[pprimary[i]] + porbrad[i] * cosd(porbang[i]);
	py[i] = py[pprimary[i]] + porbrad[i] * sind(porbang[i]);
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
	  strcpy(pname[t], buf);
	  ptype[t] = PLANET_GHOST;
	  preal[t] = FALSE;
	  px[t] = 0.0;
	  py[t] = 0.0;
	  pprimary[t] = PNUM_MURISAK;
	  porbrad[t] = rnduni(30000.0, 70000.0);
	  porbvel[t] = rnduni(-5.0, +5.0);
	  porbang[t] = rnduni(0.0, 360.0);
	}
    }

  /* Un-twiddle the lockword. */
  PVUNLOCK(lockword);
  
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
      if ( gunum( &unum, x ) ) \
      stcpn( y, upname[unum], MAXUSERPNAME ); \
      else if ( c_register( x, y, z, &unum ) ) \
      { \
	  urobot[unum] = TRUE; \
	  uooption[unum][OOPT_MULTIPLE] = TRUE; \
	  umultiple[unum] = MAXSHIPS; \
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
      rstrat[i][j] = trstrat[i][j];
  
  /* Copy the action vector. */
  for ( i = 0; i < 32; i = i + 1 )
    rvec[i] = trvec[i];
  
  *externrobots = FALSE;	/* XXX temporary */
  
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
  skilledby[snum] = 0;
  /* suser(snum)				# setup in menu() or newrob() */
  /* steam(snum)				# setup in menu() or newrob() */
  /* spid(snum)				# setup in menu() or newrob() */
  sstrkills[snum] = 0.0;	/* init to 0.0, newrob will init this
				   for robots to make them stronger, faster.. ;-) */
  sx[snum] = 0.0;
  sy[snum] = 0.0;
  sdx[snum] = 0.0;
  sdy[snum] = 0.0;
  shead[snum] = 0.0;
  sdhead[snum] = 0.0;
  swarp[snum] = 0.0;
  sdwarp[snum] = 0.0;
  slock[snum] = 0;
  sshup[snum] = TRUE;
  sshields[snum] = 100.0;
  skills[snum] = 0.0;
  sdamage[snum] = 0.0;
  sfuel[snum] = 999.0;
  swtemp[snum] = 0.0;
  setemp[snum] = 0.0;
  swfuse[snum] = 0;
  sefuse[snum] = 0;
  sweapons[snum] = 40;
  sengines[snum] = 100 - sweapons[snum];
  sarmies[snum] = 0;
  srmode[snum] = FALSE;
  scloaked[snum] = FALSE;
  /* soption(snum,i)				# setup in menu() */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    {
      /* srwar(snum,i)				# setup in menu() or newrob() */
      /* swar(snum,i)				# setup in menu() or newrob() */
      sscanned[snum][i] = 0;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    ssrpwar[snum][i] = FALSE;
  /* ssdfuse(snum)				# setup in findship() */
  PVLOCK(lockmesg);
  if ( slastmsg[snum] == LMSG_NEEDINIT )
    {
      slastmsg[snum] = *lastmsg;
      salastmsg[snum] = slastmsg[snum];
    }
  PVUNLOCK(lockmesg);
  smap[snum] = FALSE;
  stowing[snum] = 0;
  stowedby[snum] = 0;
  slastblast[snum] = 0.0;
  slastphase[snum] = 0.0;
  spfuse[snum] = 0;
  stalert[snum] = FALSE;
  srobot[snum] = FALSE;
  saction[snum] = 0;
  /* spname(1,snum)				# setup in menu() or newrob() */
  
  /* Zero torpedos. */
  for ( i = 0; i < MAXTORPS; i = i + 1 )
    {
      tstatus[snum][i] = TS_OFF;
      tfuse[snum][i] = 0;
      tx[snum][i] = 0.0;
      ty[snum][i] = 0.0;
      tdx[snum][i] = 0.0;
      tdy[snum][i] = 0.0;
      tmult[snum][i] = 0.0;
      for ( j = 0; j < NUMTEAMS; j = j + 1 )
	twar[snum][i][j] = FALSE;
    }
  
  /* Update user some stats. */
  getdandt( ulastentry[unum] );		/* time stamp for this entry */
  if ( ulastentry[unum][9] == ' ' )		/* remove seconds */
    strcpy(&ulastentry[unum][6], &ulastentry[unum][9]);
  /*    scopy( ulastentry[unum], 9, ulastentry[unum], 6 );*/
  ustats[unum][USTAT_ENTRIES] = ustats[unum][USTAT_ENTRIES] + 1;
  tstats[steam[snum]][TSTAT_ENTRIES] = tstats[steam[snum]][TSTAT_ENTRIES] + 1;
  
  return;
  
}


/*  inituniverse - initialize (without cheese and tomato) (DOES LOCKING) */
/*  SYNOPSIS */
/*    inituniverse */
void inituniverse(void)
{
  
  int i;
  
  /* Twiddle the lockword. */
  PVUNLOCK(lockword);
  PVLOCK(lockword);
  
  /* Turn off the universe. */
  *closed = TRUE;
  
  teamplanets[TEAM_FEDERATION][0] = PNUM_EARTH;
  teamplanets[TEAM_FEDERATION][1] = PNUM_TELOS;
  teamplanets[TEAM_FEDERATION][2] = PNUM_OMEGA;
  teamplanets[TEAM_ROMULAN][0] = PNUM_ROMULUS;
  teamplanets[TEAM_ROMULAN][1] = PNUM_REMUS;
  teamplanets[TEAM_ROMULAN][2] = PNUM_RHO;
  teamplanets[TEAM_KLINGON][0] = PNUM_KLINGUS;
  teamplanets[TEAM_KLINGON][1] = PNUM_LEUDUS;
  teamplanets[TEAM_KLINGON][2] = PNUM_TARSUS;
  teamplanets[TEAM_ORION][0] = PNUM_ORION;
  teamplanets[TEAM_ORION][1] = PNUM_OBERON;
  teamplanets[TEAM_ORION][2] = PNUM_UMBRIEL;
  
  homeplanet[TEAM_FEDERATION] = PNUM_EARTH;
  homeplanet[TEAM_ROMULAN] = PNUM_ROMULUS;
  homeplanet[TEAM_KLINGON] = PNUM_KLINGUS;
  homeplanet[TEAM_ORION] = PNUM_ORION;
  
  homesun[TEAM_FEDERATION] = PNUM_SOL;
  homesun[TEAM_ROMULAN] = PNUM_SIRIUS;
  homesun[TEAM_KLINGON] = PNUM_KEJELA;
  homesun[TEAM_ORION] = PNUM_BETELGEUSE;
  
  warplim[TEAM_FEDERATION] = 9.0;
  warplim[TEAM_ROMULAN] = 8.0;
  warplim[TEAM_KLINGON] = 9.0;
  warplim[TEAM_ORION] = 10.0;
  
  armylim[TEAM_FEDERATION] = 9;
  armylim[TEAM_ROMULAN] = 11;
  armylim[TEAM_KLINGON] = 9;
  armylim[TEAM_ORION] = 7;
  
  /* DOALT_UNIV has diff engfac[]'s */
  /*#ifdef DOALT_UNIV
    engfac[TEAM_FEDERATION] = 1.5;
    engfac[TEAM_ROMULAN] = 1.5;
    engfac[TEAM_KLINGON] = 1.5;
    engfac[TEAM_ORION] = 1.5;
    #else
    */
  
  engfac[TEAM_FEDERATION] = 1.0;
  engfac[TEAM_ROMULAN] = 0.8;
  engfac[TEAM_KLINGON] = 1.0;
  engfac[TEAM_ORION] = 1.2;
  
  /*#endif*/
  
  accelfac[TEAM_FEDERATION] = 1.0;
  accelfac[TEAM_ROMULAN] = 0.8;
  accelfac[TEAM_KLINGON] = 1.0;
  accelfac[TEAM_ORION] = 1.6;
  
  /* DOALT_UNIV has diff weafac[]'s */
  /*#ifdef DOALT_UNIV
    weafac[TEAM_ROMULAN] = 1.1;
    weafac[TEAM_ORION] = 0.90;
    weafac[TEAM_FEDERATION] = rnduni(0.9, 1.1);
    weafac[TEAM_KLINGON] = rnduni(0.9, 1.1);
    #else
    */
  
  weafac[TEAM_ROMULAN] = 1.17;
  weafac[TEAM_ORION] = 0.83;
  weafac[TEAM_FEDERATION] = 1.0;
  weafac[TEAM_KLINGON] = 1.0;
  
  /*#endif*/
  
  torpwarp[TEAM_FEDERATION] = 12.0;
  torpwarp[TEAM_ROMULAN] = 10.0;
  torpwarp[TEAM_KLINGON] = 12.0;
  torpwarp[TEAM_ORION] = 14.0;
  
  stcpn( "Federation", tname[TEAM_FEDERATION], MAXTEAMNAME );
  stcpn( "Romulan", tname[TEAM_ROMULAN], MAXTEAMNAME );
  stcpn( "Klingon", tname[TEAM_KLINGON], MAXTEAMNAME );
  stcpn( "Orion", tname[TEAM_ORION], MAXTEAMNAME );
  stcpn( "self ruled", tname[TEAM_SELFRULED], MAXTEAMNAME );
  stcpn( "non", tname[TEAM_NOTEAM], MAXTEAMNAME );
  stcpn( "GOD", tname[TEAM_GOD], MAXTEAMNAME );
  stcpn( "Empire", tname[TEAM_EMPIRE], MAXTEAMNAME );
  
  chrplanets[PLANET_CLASSM] = 'M';
  chrplanets[PLANET_DEAD] = 'D';
  chrplanets[PLANET_SUN] = 'S';
  chrplanets[PLANET_MOON] = 'm';
  chrplanets[PLANET_GHOST] = 'G';
  chrplanets[PLANET_CLASSA] = 'A';
  chrplanets[PLANET_CLASSO] = 'O';
  chrplanets[PLANET_CLASSZ] = 'Z';
  
  chrteams[TEAM_FEDERATION] = 'F';
  chrteams[TEAM_ROMULAN] = 'R';
  chrteams[TEAM_KLINGON] = 'K';
  chrteams[TEAM_ORION] = 'O';
  chrteams[TEAM_SELFRULED] = '-';
  chrteams[TEAM_NOTEAM] = ' ';
  chrteams[TEAM_GOD] = 'G';
  chrteams[TEAM_EMPIRE] = 'E';
  
  chrtorps[TEAM_FEDERATION] = '*';
  chrtorps[TEAM_ROMULAN] = '@';
  chrtorps[TEAM_KLINGON] = '+';
  chrtorps[TEAM_ORION] = '.';
  
  /* Initialize driver variables. */
  *drivcnt = 0;
  drivowner[0] = EOS;
  
  /* Initialize user history stuff. */
  *histptr = 0;
  for ( i = 0; i < MAXHISTLOG; i = i + 1 )
    {
      histunum[i] = -1;
      histlog[i][0] = EOS;
    }
  
  /* Un-twiddle the lockword. */
  PVUNLOCK(lockword);
  
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
  
  if ( preal[pnum] &&
      pteam[pnum] != TEAM_SELFRULED &&
      pteam[pnum] != TEAM_NOTEAM )
    if ( snum == MSG_DOOM )
      {
	c_strcpy( dname, buf );
	upper( dname );
	appstr( atta, buf );
	appstr( armeq, buf );
	appint( parmies[pnum], buf );
	stormsg( -pnum, -pteam[pnum], buf );
      }
    else if ( swar[snum][pteam[pnum]] )
      {
	c_strcpy( "INTRUDER ALERT - ", buf );
	appship( snum, buf );
	appstr( atta, buf );
	appstr( armeq, buf );
	appint( parmies[pnum], buf );
	stormsg( -pnum, -pteam[pnum], buf );
	defend( snum, pnum );
      }
  
  return;
  
}


/*  loghist - log this entry into the Game (DOES LOCKING) */
/*  SYNOPSIS */
/*    int unum */
/*    loghist( unum ) */
void loghist( int unum )
{
  PVLOCK(lockword);
  *histptr = modp1( *histptr + 1, MAXHISTLOG );
  
  getdandt( histlog[*histptr] );	/* time stamp for this entry */
  
  /* remove seconds */
  /*    if ( histlog[*histptr][9] == ' ' )	
	scopy( histlog[*histptr], 9, histlog[*histptr], 6 );
	*/
  
  histunum[*histptr] = unum;
  PVUNLOCK(lockword);
  return;
}


/*  newarp - handle ship acceleration. */
/*  SYNOPSIS */
/*    real warp, newarp, dwarp */
/*    int snum */
/*    warp = newarp( snum, dwarp ) */
real newarp( int snum, real dwarp )
{
  real x, acc;
  
  x = dwarp - swarp[snum];
  acc = (real) accelfac[steam[snum]] * (real) engeff( snum ) * ITER_SECONDS;
  if ( acc >= fabs( x ) )
    return ( dwarp );			/* close enough (or equal) */
  else if ( x < 0.0 )
    return ( swarp[snum] - acc );
  
  return ( swarp[snum] + acc );
  
}


/*  phoon - calculate the phase of a moon */
/*  SYNOPSIS */
/*    int phase, phoon, pnum */
/*    phase = phoon( pnum ) */
int phoon( int pnum )
{
  int i, j, ph;
  
  /* Suns simply don't have phases. */
  if ( ptype[pnum] == PLANET_SUN )
    return ( PHOON_NO );
  
  /* You have to orbit some-thing to have a phase. */
  i = pprimary[pnum];
  if ( i == 0 )
    return ( PHOON_NO );
  
  /* Your primary must be a non-sun that is real. */
  if ( ptype[i] == PLANET_SUN || ! preal[i] )
    return ( PHOON_NO );
  
  /* Your primary has to orbit a (real) sun to have a phase. */
  j = pprimary[i];
  if ( j == 0 )
    return ( PHOON_NO );
  if ( ptype[j] != PLANET_SUN || ! preal[j] )
    return ( PHOON_NO );
  
  /* Things are cool, now calculate the phase. */
  ph = (int) ( mod360( porbang[pnum] - porbang[i] - 45.0 ) / 90.0 );
  
  /* The number calculated is in the range 0 to 3, and works fine */
  /* if the moon is orbiting counter clockwise. If it is orbiting */
  /* in the other direction, we must swap the first and last quarters. */
  if ( porbvel[pnum] < 0.0 )
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
	if ( stmatch( str, pname[*pnum], FALSE ) )
	  return ( TRUE );
    }
  else
    {
      for ( *pnum = 1; *pnum <= NUMPLANETS; *pnum = *pnum + 1 )
	if ( preal[*pnum] )
	  if ( stmatch( str, pname[*pnum], FALSE ) )
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
	    if ( tcol > STAT_COLS && tcol <= cmaxcol - 1 )
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
      sx[snum] = rndnor( basex, smear );
      sy[snum] = rndnor( basey, smear );
      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
	if ( sstatus[i] == SS_LIVE )
	  if ( satwar( i, snum ) &&
	      i != snum &&
	      dist( sx[snum], sy[snum], sx[i], sy[i] ) <= ENTRY_ENEMY_DIST )
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
  i = msgfrom[msgnum];

  if ( i > 0 && i <= MAXSHIPS )
    {
      buf[0] = EOS;
      appship( i, buf );
    }
  else if ( -i > 0 && -i <= NUMPLANETS )
    c_strcpy( pname[-i], buf );
  else switch ( i )
    {
    case MSG_NOONE:
      c_strcpy( "nobody", buf );
      break;
    case MSG_GOD:
      c_strcpy( "GOD", buf );
      break;
    case MSG_DOOM:
      concat( "The ", dname, buf );
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
  i = msgto[msgnum];
  if ( i == snum )
    appstr( "you", buf );
  else if ( i > 0 && i <= MAXSHIPS )
    appship( i, buf );
  else if ( -i >= 0 && -i < NUMTEAMS )
    {
      appchr( chrteams[-i], buf );
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
  appstr( msgbuf[msgnum], buf );

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
  ch = cdgetx( mto, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
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
      else if ( -to >= 0 && -to < NUMTEAMS )
	c_strcpy( tname[-to], buf );
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
      if ( sstatus[j] != SS_LIVE )
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
      for ( i = 0; i < NUMTEAMS; i = i + 1 )
	if ( buf[0] == chrteams[i] || buf[0] == (char)tolower(chrteams[i]) )
	  break;
      if ( i >= NUMTEAMS )
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
      if ( sstatus[to] != SS_LIVE )
	{
	  c_putmsg( nf, MSG_LIN2 );
	  return;
	}
      appship( to, buf );
      appchr( ':', buf );
    }
  else if ( -to >= 0 && -to < NUMTEAMS )
    {
      appstr( tname[-to], buf );
      appstr( "s:", buf );
    }
  else switch ( to ) /* jet added breaks */
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
  if ( cdgetp( ">", MSG_LIN2, 1, TERMS, msg, i, &append_flg, do_append_flg ) != TERM_ABORT )
    if ( to != MSG_IMPLEMENTORS )
      stormsg( from, to, msg );
    else
      {
	/* Handle a message to the Implementors. */
	c_strcpy( "Communique from ", buf );
	if ( from > 0 && from <= MAXSHIPS )
	  {
	    appstr( spname[from], buf );
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
  
  return(strcmp(pname[*icmp1], pname[*icmp2]));
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

/*  spwar - test whether a ship is at war with a planet */
/*  SYNOPSIS */
/*    int atwar, spwar */
/*    int snum, pnum */
/*    atwar = spwar( snum, pnum ) */
int spwar( int snum, int pnum )
{
  
  
  if ( ! preal[pnum] )
    return ( FALSE );		/* can't be at war unless it's real */
  else if ( ptype[pnum] == PLANET_SUN )
    return ( TRUE );		/* always at war with suns */
  else if ( ptype[pnum] == PLANET_MOON )
    return ( FALSE );		/* never at war with moons */
  else if ( parmies[pnum] <= 0 )
    return ( FALSE );		/* can't have war without armies */
  else switch ( pteam[pnum] )	/* jet added breaks */
    {
    case TEAM_FEDERATION:
    case TEAM_ROMULAN:
    case TEAM_KLINGON:
    case TEAM_ORION:
      if ( pteam[pnum] == steam[snum] )
	return ( FALSE );
      else
	return ( swar[snum][pteam[pnum]] );
      break;
    default:
      return ( ssrpwar[snum][pnum] );
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
  if ( uooption[suser[snum]][OOPT_SHITLIST] )
    {
      if ( sstatus[snum] == SS_LIVE )
	killship( snum, KB_SHIT );
      return ( FALSE );
    }
  if ( *closed && ! uooption[suser[snum]][OOPT_PLAYWHENCLOSED] )
    {
      if ( sstatus[snum] == SS_LIVE )
	killship( snum, KB_EVICT );
      return ( FALSE );
    }
  
  if ( sstatus[snum] == SS_RESERVED || sstatus[snum] == SS_ENTERING )
    return ( TRUE );
  
  return ( sstatus[snum] == SS_LIVE );
  
}


/*  stormsg - store a message in the message buffer (DOES LOCKING) */
/*  SYNOPSIS */
/*    int from, to */
/*    char msg() */
/*    stormsg( from, to, msg ) */
void stormsg( int from, int to, char *msg )
{
  int nlastmsg, i;
  
  
  PVLOCK(lockmesg);
  nlastmsg = modp1( *lastmsg + 1, MAXMESSAGES );
  stcpn( msg, msgbuf[nlastmsg], MESSAGE_SIZE );
  msgfrom[nlastmsg] = from;
  msgto[nlastmsg] = to;
  *lastmsg = nlastmsg;
  
  /* Remove allowable last message restrictions. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( nlastmsg == salastmsg[i] )
      salastmsg[i] = LMSG_READALL;
  PVUNLOCK(lockmesg);
  
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
      if ( swfuse[snum] > 0 )
	return ( FALSE );
    }
  else
    {
      if ( sefuse[snum] > 0 )
	{
	  sdwarp[snum] = 0.0;
	  return ( FALSE );
	}
    }

  sfuel[snum] = sfuel[snum] - fuel;

  if ( sfuel[snum] < 0.0 )
    {
      /* When you're out of gas, you're out of fun... */
      sfuel[snum] = 0.0;
      scloaked[snum] = FALSE;
      sdwarp[snum] = 0.0;
      return ( FALSE );
    }
  else if ( sfuel[snum] > 999.0 )
    sfuel[snum] = 999.0;
  
  /* Temperature. */
  if ( weapon )
    {
      swtemp[snum] = swtemp[snum] + ((fuel * TEMPFUEL_FAC) / weaeff ( snum ));
      if ( swtemp[snum] < 0.0 )
	swtemp[snum] = 0.0;
      else if ( swtemp[snum] >= 100.0 )
	if ( rnd() < WEAPON_DOWN_PROB )
	  {
	    swfuse[snum] = rndint( MIN_DOWN_FUSE, MAX_DOWN_FUSE );
	    if ( ! soption[snum][OPT_TERSE] )
	      stormsg( MSG_COMP, snum, "Weapons overload." );
	  }
    }
  else
    {
      setemp[snum] = setemp[snum] + fuel * TEMPFUEL_FAC / engeff( snum );
      if ( setemp[snum] < 0.0 )
	setemp[snum] = 0.0;
      else if ( setemp[snum] >= 100.0 )
	if ( rnd() < ENGINE_DOWN_PROB )
	  {
	    sefuse[snum] = rndint( MIN_DOWN_FUSE, MAX_DOWN_FUSE );
	    if ( ! soption[snum][OPT_TERSE] )
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
  
  sstatus[snum] = SS_OFF;
  skilledby[snum] = 0;
  suser[snum] = 0;
  steam[snum] = 0;
  spid[snum] = 0;
  sx[snum] = 0.0;
  sy[snum] = 0.0;
  sdx[snum] = 0.0;
  sdy[snum] = 0.0;
  shead[snum] = 0.0;
  sdhead[snum] = 0.0;
  swarp[snum] = 0.0;
  sdwarp[snum] = 0.0;
  slock[snum] = 0;
  sshup[snum] = FALSE;
  sshields[snum] = 0.0;
  skills[snum] = 0.0;
  sdamage[snum] = 0.0;
  sfuel[snum] = 0.0;
  setemp[snum] = 0.0;
  swtemp[snum] = 0.0;
  swfuse[snum] = 0;
  sefuse[snum] = 0;
  sweapons[snum] = 0;
  sengines[snum] = 0;
  sarmies[snum] = 0;
  srmode[snum] = FALSE;
  scloaked[snum] = FALSE;
  for ( i = 0; i < MAXOPTIONS; i = i + 1 )
    soption[snum][i] = FALSE;
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    {
      srwar[snum][i] = FALSE;
      swar[snum][i] = FALSE;
      sscanned[snum][i] = 0;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    ssrpwar[snum][i] = FALSE;
  ssdfuse[snum] = 0;
  slastmsg[snum] = 0;
  salastmsg[snum] = 0;
  smap[snum] = FALSE;
  stowing[snum] = 0;
  stowedby[snum] = 0;
  slastblast[snum] = 0.0;
  slastphase[snum] = 0.0;
  spfuse[snum] = 0;
  stalert[snum] = FALSE;
  srobot[snum] = FALSE;
  saction[snum] = 0;
  for ( i = 0; i < SIZEUSERPNAME; i = i + 1 )
    spname[snum][i] = EOS;
  sctime[snum] = 0;
  setime[snum] = 0;
  scacc[snum] = 0;
  seacc[snum] = 0;
  
  for ( i = 0; i < MAXTORPS; i = i + 1 )
    {
      tstatus[snum][i] = TS_OFF;
      tfuse[snum][i] = 0;
      tx[snum][i] = 0.0;
      ty[snum][i] = 0.0;
      tdx[snum][i] = 0.0;
      tdy[snum][i] = 0.0;
      tmult[snum][i] = 0.0;
      for ( j = 0; j < NUMTEAMS; j = j + 1 )
	twar[snum][i][j] = FALSE;
    }
  
  return;
  
}

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
