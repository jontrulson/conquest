#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                                 C O N Q L B */
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
#include "conqlb.h"
#define NOCDHEXTERN
#include "conqdata.h"

#include "conf.h"
#include "global.h"
#include "user.h"
#include "color.h"

				/* shared with display.c */
real LastPhasDist = PHASER_DIST;

/*  clbChalkup - perform kills accoutinng */
/*  SYNOPSIS */
/*    int snum */
/*    clbChalkup( snum ) */
/*  Note: This routines ASSUMES you have the common locked before you */
/*        use it. */
void clbChalkup( int snum )
{
  int i, unum, team;
  real x, w, l, m;
  
  unum = Ships[snum].unum;
  team = Ships[snum].team;
  
  /* Update wins. */
  Users[unum].stats[USTAT_WINS] += ifix(Ships[snum].kills);
  Teams[team].stats[TSTAT_WINS] = Teams[team].stats[TSTAT_WINS] + ifix(Ships[snum].kills);
  
  /* Update max kills. */
  i = ifix( Ships[snum].kills );
  if ( i > Users[unum].stats[USTAT_MAXKILLS] )
    Users[unum].stats[USTAT_MAXKILLS] = i;
  
  /* Update rating. */
  l = Users[unum].stats[USTAT_LOSSES];
  if ( l == 0 )
    l = 1;
  w = Users[unum].stats[USTAT_WINS];
  m = Users[unum].stats[USTAT_MAXKILLS];
  Users[unum].rating = ( w / l ) + ( m / 4.0 );
  x = w - l;
  if ( x >= 0.0 )
    Users[unum].rating += pow((real) x, (real) ( 1.0 / 3.0 ));
  else
    Users[unum].rating -= pow((real) -x, (real) ( 1.0 / 3.0 ));
  
  return;
  
}

/*  damage - damage a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    real dam */
/*    clbDamage( snum, dam, kb ) */
void clbDamage( int snum, real dam, int kb )
{
  real mw;
  
  Ships[snum].damage = Ships[snum].damage + dam;
  if ( Ships[snum].damage >= 100.0 )
    clbKillShip( snum, kb );
  else
    {
      mw = maxwarp( snum );
      Ships[snum].dwarp = min( Ships[snum].dwarp, mw );
    }
  
  return;
  
}

/*  detonate - blow up a torpedo (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, tnum */
/*    clbDetonate( snum, tnum ) */
void clbDetonate( int snum, int tnum )
{
  
  PVLOCK(&ConqInfo->lockword);
  if ( Ships[snum].torps[tnum].status == TS_LIVE )
    Ships[snum].torps[tnum].status = TS_DETONATE;
  PVUNLOCK(&ConqInfo->lockword);
  
  return;
  
}
/*  enemydet - detonate enemy torpedos */
/*  SYNOPSIS */
/*    int didit, enemydet */
/*    int snum */
/*    didit = clbEnemyDet( snum ) */
int clbEnemyDet( int snum )
{
  int i, j;
  
  /* Stop repairing. */
  SFCLR(snum, SHIP_F_REPAIR);
  
  if ( ! clbUseFuel( snum, DETONATE_FUEL, TRUE, TRUE ) )
    return ( FALSE );
  
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status != SS_OFF && i != snum )
      for ( j = 0; j < MAXTORPS; j = j + 1 )
	if ( Ships[i].torps[j].status == TS_LIVE )
	  if ( Ships[i].torps[j].war[Ships[snum].team] || Ships[snum].war[Ships[i].team] )
	    if ( dist( Ships[snum].x, Ships[snum].y, Ships[i].torps[j].x, Ships[i].torps[j].y ) <=
		DETONATE_DIST )
	      clbDetonate( i, j );
  
  return ( TRUE );
  
}


/*  hit - hit a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    real ht */
/*    clbHit( snum, ht, kb ) */
void clbHit( int snum, real ht, int kb )
{
  if ( ht > 0.0 )
    {
      if ( SSHUP(snum) && ! SREPAIR(snum) )
	{
	  if ( ht > Ships[snum].shields )
	    {
	      clbDamage( snum, ht-Ships[snum].shields, kb ); 
	      Ships[snum].shields = 0.0;
	    }
	  else
	    {
	      Ships[snum].shields = Ships[snum].shields - ht;
	    }
	}
      else
	{
	  clbDamage( snum, ht, kb );
	}
    }

  return;
}


/*  ikill - ikill a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    clbIKill( snum, kb ) */
/*  Note: This routines ASSUMES you have the common locked before you it. */
void clbIKill( int snum, int kb )
{
  int i, unum, team, kunum, kteam;
  real tkills;
  
  /* Only procede if the ship is alive */
  if ( Ships[snum].status != SS_LIVE )
    return;
  
  /* The ship is alive; kill it. */
  Ships[snum].killedby = kb;
  Ships[snum].status = SS_DYING;
  
  unum = Ships[snum].unum;
  team = Ships[snum].team;

  /* Detonate all torpedos. */

  for ( i = 0; i < MAXTORPS; i = i + 1 )
    if ( Ships[snum].torps[i].status == TS_LIVE )
      Ships[snum].torps[i].status = TS_DETONATE;
  
  /* Release any tows. */
  if ( Ships[snum].towing != 0 )
    Ships[Ships[snum].towing].towedby = 0;
  if ( Ships[snum].towedby != 0 )
    Ships[Ships[snum].towedby].towing = 0;
  
  /* Zero team scan fuses. */
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    Ships[snum].scanned[i] = 0;
  
  if ( kb == KB_CONQUER )
    Ships[snum].kills = Ships[snum].kills + CONQUER_KILLS;
  else if ( kb == KB_GOTDOOMSDAY )
    Ships[snum].kills = Ships[snum].kills + DOOMSDAY_KILLS;
  else if ( kb >= 0 )				/* if a ship did the killing */
    {
      kunum = Ships[kb].unum;
      kteam = Ships[kb].team;
      tkills = 1.0 + ((Ships[snum].kills + Ships[snum].strkills) * KILLS_KILLS);
      if ( Ships[snum].armies > 0 )
	{
	  /* Keep track of carried armies killed - they are special. */
	  tkills = tkills + Ships[snum].armies * ARMY_KILLS;
	  Users[kunum].stats[USTAT_ARMSHIP] += Ships[snum].armies;
	  Teams[kteam].stats[TSTAT_ARMSHIP] += Ships[snum].armies;
	}
      
      /* Kills accounting. */
      if ( Ships[kb].status == SS_LIVE )
	Ships[kb].kills = Ships[kb].kills + tkills;
      else
	{
	  /* Have to do some hacking when our killer is dead. */
	  Users[kunum].stats[USTAT_WINS] -= ifix(Ships[kb].kills);
	  Teams[kteam].stats[TSTAT_WINS] =
	    Teams[kteam].stats[TSTAT_WINS] - ifix(Ships[kb].kills);
	  Ships[kb].kills = Ships[kb].kills + tkills;
	  clbChalkup( kb );
	}
      
      /* Sticky war logic. */
				/* should set sticky war too. -JET */

      if ( ! Ships[snum].war[kteam] )
	{
	  Ships[kb].war[team] = TRUE;
          Ships[kb].rwar[team] = TRUE;
	}
    }
  
  /* Kills accounting. */
  clbChalkup( snum );
  if ( kb != KB_SELF && kb != KB_CONQUER && kb != KB_NEWGAME &&
      kb != KB_EVICT && kb != KB_SHIT && kb != KB_GOD )
    {
      /* Update losses. */
      Users[unum].stats[USTAT_LOSSES] += 1;
      Teams[team].stats[TSTAT_LOSSES] += 1;
    }
  
  if ( ! SROBOT(snum) || Ships[snum].pid != 0 )
    {
      Ships[snum].status = SS_DEAD;
      Ships[snum].sdfuse = -TIMEOUT_PLAYER;		/* setup dead timeout timer */
    }
  else
    {
#if defined(DO_EXPLODING_SHIPS)
      Ships[snum].status = SS_DEAD;
      Ships[snum].sdfuse = -5;          /* setup dead timeout timer,
				      for robots, that's 5 seconds, this
				      gives enough time for detonating torps,
				      just like regular players */
#else
				/* old behavior */
      Ships[snum].status = SS_OFF;   /* turn robots off */
#endif

      /* We'd like to remove this next line so that you could */
      /* use conqoper to see what killed him, but then robots */
      /* show up on the debugging playlist... */
      /*      Ships[snum].killedby = 0;*/
    }
  
  return;
  
}


/* ETAstr - return a string indicating ETA to a target */
char *clbETAStr(real warp, real distance)
{
  real secs;
  real mins;
  static char retstr[64];
  
  if (warp <= 0.0)
    {
      sprintf(retstr, "never");
      return(retstr);
    }
  
  mins = 0.0;
  secs = (real) (distance / (warp * MM_PER_SEC_PER_WARP));
  
  if (secs > 60.0)
    {
      mins = secs / 60.0;
      secs = 0.0;
    }
  
  if (mins != 0.0)
    sprintf(retstr, "%.1f minutes", mins);
  else
    sprintf(retstr, "%.1f seconds", secs);
  
  return(retstr);
}


/*  kill - kill a ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, kb */
/*    kill( snum, kb ) */
void clbKillShip( int snum, int kb )
{
  int sendmesg = FALSE;
  char msgbuf[BUFFER_SIZE];

#if defined(DO_EXPLODING_SHIPS)
  /* launch all torps - sorta, we'll use 'explode' mode... */
  clbLaunch(snum, 0.0, EXPLODESHIP_TORP_COUNT, LAUNCH_EXPLODE);
#endif
  
				/* internal routine. */
  PVLOCK(&ConqInfo->lockword);
  clbIKill( snum, kb );
  PVUNLOCK(&ConqInfo->lockword);

				/* send a msg to all... */
  sendmesg = FALSE;

  /* Figure out why we died. */
  switch ( kb )
    {
    case KB_SELF:
      sprintf(msgbuf, "%c%d (%s) has self-destructed.",
	      Teams[Ships[snum].team].teamchar,
	      snum,
	      Ships[snum].alias);
      sendmesg = TRUE;
      
      break;
    case KB_NEGENB:
      sprintf(msgbuf, "%c%d (%s) was destroyed by the negative energy barrier.",
	      Teams[Ships[snum].team].teamchar,
	      snum,
	      Ships[snum].alias);
      sendmesg = TRUE;
      
      break;
      
    case KB_GOD:
      sprintf(msgbuf, "%c%d (%s) was killed by an act of GOD.",
	      Teams[Ships[snum].team].teamchar,
	      snum,
	      Ships[snum].alias);
      sendmesg = TRUE;
      
      break;
    case KB_DOOMSDAY:
      sprintf(msgbuf, "%c%d (%s) was eaten by the doomsday machine.",
	      Teams[Ships[snum].team].teamchar,
	      snum,
	      Ships[snum].alias);
      sendmesg = TRUE;
      
      break;
    case KB_DEATHSTAR:
      sprintf(msgbuf, "%c%d (%s) was vaporized by the Death Star.",
	      Teams[Ships[snum].team].teamchar,
	      snum,
	      Ships[snum].alias);
      sendmesg = TRUE;

      break;
    case KB_LIGHTNING:
      sprintf(msgbuf, "%c%d (%s) was destroyed by a lightning bolt.",
	      Teams[Ships[snum].team].teamchar,
	      snum,
	      Ships[snum].alias);
      sendmesg = TRUE;

      break;
    default:
      
      if ( kb > 0 && kb <= MAXSHIPS )
	{
	  sprintf(msgbuf, "%c%d (%s) was kill %.1f for %c%d (%s).",
		  Teams[Ships[snum].team].teamchar,
		  snum,
		  Ships[snum].alias,
		  Ships[kb].kills,
		  Teams[Ships[kb].team].teamchar,
		  kb,
		  Ships[kb].alias);
	  sendmesg = TRUE;

	}
      else if ( -kb > 0 && -kb <= NUMPLANETS )
	{
	  sprintf(msgbuf, "%c%d (%s) was destroyed by %s",
		  Teams[Ships[snum].team].teamchar,
		  snum,
		  Ships[snum].alias,
		  Planets[-kb].name);

	  sendmesg = TRUE;
	  
	  if ( Planets[-kb].type == PLANET_SUN )
	    {
	      appstr( "'s solar radiation.", msgbuf );
	    }
	  else
	    {
	      appstr( "'s planetary defenses.", msgbuf );
	    }
	}
    }

  if (sendmesg == TRUE)
    clbStoreMsg(MSG_COMP, MSG_ALL, msgbuf);

  return;
  
}

/* see if we could launch some if we wanted too... */
/* does NO LOCKING, so only use from the client */
int clbCheckLaunch(int snum, int number)
{
  register int i, j;

  if (number == 0)
    return TRUE;

  j = 0;
  for ( i = 0; i < MAXTORPS && number != 0; i++ )
    if ( Ships[snum].torps[i].status == TS_OFF )
      {
	/* Found one. */
	j++;
	number--;
        /* see if this can save some wasted bandwidth by marking the
           torp as reserved.  Since this is client side only, the next
           ship update will set the true count anyway if it actually
           fired.  These are cleaned out with domydet().  The end
           result is not making the server refuse the request anyway,
           wasting valuable time. */
        Ships[snum].torps[i].status = TS_RESERVED;
      }

  if (j == 0)
    return FALSE;
  else
    return TRUE;
}


/*  launch - create new torpedo(s) for a ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    flag = clbLaunch( snum, dir, number_of_torps, launch_type ) */
int clbLaunch( int snum, real dir, int number, int ltype )
{
  register int i, j;
  real speed, adir; 
  int tnum, numslots, numfired;
  static int tslot[MAXTORPS];
  
  /* Stop repairing. */
  SFCLR(snum, SHIP_F_REPAIR);
  
  /* Remember this important direction. */
  Ships[snum].lastblast = dir;
  
  /* Set up last fired phaser direction. */
  Ships[snum].lastphase = dir;

  numslots = 0;
  numfired = 0;
  tnum = number;
  
  /* Find free torp(s). */
  PVLOCK(&ConqInfo->lockword);
  for ( i = 0; i < MAXTORPS && tnum != 0; i++ )
    if ( Ships[snum].torps[i].status == TS_OFF )
      {
	/* Found one. */
	Ships[snum].torps[i].status = TS_LAUNCHING;
	tslot[numslots++] = i;
	tnum--;
      }
  PVUNLOCK(&ConqInfo->lockword);
  
  if (numslots == 0)
    {				/* couldn't find even one */
      return(FALSE);
    }
  
  for (i=0; i<numslots; i++)
    {
      /* Use fuel. */
      if ( clbUseFuel( snum, TORPEDO_FUEL, TRUE, TRUE ) == FALSE)
	{
	  Ships[snum].torps[tslot[i]].status = TS_OFF;
	  continue;
	}
      else
	{			/* fired successfully */
	  numfired++;
	}
      
      /* Initialize it. */
      if (ltype == LAUNCH_EXPLODE)
	{			/* special needs for just exploding torps
				   that shouldn't go anywhere... */
	  Ships[snum].torps[tslot[i]].fuse = 1; /* shouldn't last long */
				/* should be close to the ship */
	  Ships[snum].torps[tslot[i]].x = 
	    rndnor( Ships[snum].x, EXPLODESHIP_TORP_SPREAD );
	  Ships[snum].torps[tslot[i]].y = 
	    rndnor( Ships[snum].y, EXPLODESHIP_TORP_SPREAD );
				/* no movement */
	  speed = 0.0;
				/* no direction or deltas */
	  adir = 0.0;
	  Ships[snum].torps[tslot[i]].dx = 0.0;
	  Ships[snum].torps[tslot[i]].dy = 0.0;

				/* strength of explosion depends on
				   the average of engine and weap
				   efficiency.  This prevents one side
				   from having an explosive adv over
				   another, while allowing greater
				   kills to matter.  */
	  Ships[snum].torps[tslot[i]].mult = (( (real)engeff(snum) + 
						 (real)weaeff(snum) ) / 2.0);
	}
      else
	{
	  Ships[snum].torps[tslot[i]].fuse = TORPEDO_FUSE;
	  Ships[snum].torps[tslot[i]].x = rndnor( Ships[snum].x, 100.0 );
	  Ships[snum].torps[tslot[i]].y = rndnor( Ships[snum].y, 100.0 );
	  speed = ShipTypes[Ships[snum].shiptype].torpwarp * MM_PER_SEC_PER_WARP * 
	    ITER_SECONDS;
	  adir = rndnor( dir, 2.0 );
	  Ships[snum].torps[tslot[i]].dx = (real) (speed * cosd(adir));
	  Ships[snum].torps[tslot[i]].dy = (real)(speed * sind(adir));
	  Ships[snum].torps[tslot[i]].mult = (real)weaeff( snum );
	}

      for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
	{
	  if (ltype == LAUNCH_EXPLODE)
	    {			/* if our ship is exploding we're at war
				   with everything. */
	      Ships[snum].torps[tslot[i]].war[j] = TRUE; 
	    }
	  else
	    Ships[snum].torps[tslot[i]].war[j] = Ships[snum].war[j]; /* just enemies */
	}
	  
      Ships[snum].torps[tslot[i]].status = TS_LIVE;
    } 
  
  if (numfired == 0)
    {				/* couldn't fire any. bummer dude. */
      return(FALSE);
    }
  else
    {				/* torps away! */
      /* Update stats. */
      PVLOCK(&ConqInfo->lockword);
      Users[Ships[snum].unum].stats[USTAT_TORPS] += numfired;
      Teams[Ships[snum].team].stats[TSTAT_TORPS] += numfired;
      PVUNLOCK(&ConqInfo->lockword);
      
      if (numfired == number)
	{			/* fired all requested */
	  return ( TRUE );
	}
      else
	{
	  /* fired some, but not all */
	  return(FALSE);
	}
    }
  
}


/*  orbit - place a ship into orbit around a planet */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    clbOrbit( snum, pnum ) */
void clbOrbit( int snum, int pnum )
{
  real beer; 
  
  Ships[snum].lock = -pnum;
  Ships[snum].dwarp = 0.0;
  
  /* Find bearing to planet. */
  beer = angle( Ships[snum].x, Ships[snum].y, Planets[pnum].x, Planets[pnum].y );
  if ( Ships[snum].head < ( beer - 180.0 ) )
    beer = beer - 360.0;
  
  /* Check beer head to determine orbit direction. */
  if ( beer <= Ships[snum].head )
    {
      Ships[snum].warp = ORBIT_CW;
      Ships[snum].head = mod360( beer + 90.0 );
    }
  else
    {
      Ships[snum].warp = ORBIT_CCW;
      Ships[snum].head = mod360( beer - 90.0 );
    }
  
  return;
  
}


/*  phaser - fire phasers (bug fry!!) (DOES LOCKING) */
/*  SYNOPSIS */
/*    int didit, phaser */
/*    int snum */
/*    real dir */
/*    didit = clbPhaser( snum, dir ) */
int clbPhaser( int snum, real dir )
{
  int k;
  real dis, ang;
  
  /* Set up last weapon direction. */
  Ships[snum].lastblast = dir;
  
  /* Stop repairing. */
  SFCLR(snum, SHIP_F_REPAIR);
  
  /* See if ok to fire. */
  if ( Ships[snum].pfuse > 0 )
    return ( FALSE );
  
  /* Try to use fuel for this shot. */
  if ( ! clbUseFuel( snum, PHASER_FUEL, TRUE, TRUE ) )
    return ( FALSE );
  
  /* Update stats. */
  PVLOCK(&ConqInfo->lockword);
  Users[Ships[snum].unum].stats[USTAT_PHASERS] += 1;
  Teams[Ships[snum].team].stats[TSTAT_PHASERS] += 1;
  PVUNLOCK(&ConqInfo->lockword);
  
  /* Set up last fired direction. */
  Ships[snum].lastphase = dir;
  
  /* Start phaser fuse. */
  Ships[snum].pfuse = PHASER_TENTHS;
  
  /* See what we can hit. */
  for ( k = 1; k <= MAXSHIPS; k = k + 1 )
    if ( Ships[k].status == SS_LIVE && k != snum )
      if ( satwar(snum, k ) )
	{
	  dis = dist( Ships[snum].x, Ships[snum].y, Ships[k].x, Ships[k].y );
	  if ( dis <= PHASER_DIST )
	    {
	      ang = angle( Ships[snum].x, Ships[snum].y, Ships[k].x, Ships[k].y );
	      if ( ang > 315.0 )
		ang = ang - 360.0;
	      if ( fabs( dir - ang ) <= PHASER_SPREAD )
		{
		  clbHit( k, clbPhaserHit( snum, dis ), snum );
		  LastPhasDist = dis;
		}
	      else
		LastPhasDist = PHASER_DIST;
	    }
	  else
	    LastPhasDist = PHASER_DIST;
	}
  
  return ( TRUE );
  
}


/*  phaserhit - determine phaser damage */
/*  SYNOPSIS */
/*    int snum */
/*    real hit, phaserhit, dis */
/*    hit = clbPhaserHit( snum, dis ) */
real clbPhaserHit( int snum, real dis )
{
  return (( - dis / PHASER_DIST + 1.0 ) * PHASER_HIT * weaeff( snum ));
  
}



/*  register - register a new user (DOES LOCKING) */
/*  SYNOPSIS */
/*    char lname(), rname() */
/*    int team, unum */
/*    int flag, register */
/*    flag = register( lname, rname, team, unum ) */
int clbRegister( char *lname, char *rname, int team, int *unum ) 
{
  int i, j;
  
  PVLOCK(&ConqInfo->lockword);
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    if ( ! Users[i].live )
      {
	Users[i].live = TRUE;
	PVUNLOCK(&ConqInfo->lockword);
	Users[i].rating = 0.0;
	Users[i].team = team;
	Users[i].robot = FALSE;
	Users[i].multiple = 1;		/* but the option bit is off */

	Users[i].type = 0;	/* we will use this someday */
	
	for ( j = 0; j < MAXUSTATS; j = j + 1 )
	  Users[i].stats[j] = 0;
	
	for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
	  Users[i].war[j] = TRUE;
	Users[i].war[Users[i].team] = FALSE;
	
	for ( j = 0; j < MAXOOPTIONS; j = j + 1 )
	  Users[i].ooptions[j] = FALSE;
	
	if (SysConf.AllowSwitchteams == TRUE)
	  {
				/* allow users to switchteams when dead */
	    Users[i].ooptions[OOPT_SWITCHTEAMS] = TRUE; 
	  }
	else
	  Users[i].ooptions[OOPT_SWITCHTEAMS] = FALSE;

	Users[i].lastentry = 0;	/* never */
	stcpn( lname, Users[i].username, MAXUSERNAME );
	stcpn( rname, Users[i].alias, MAXUSERPNAME );
	*unum = i;
	return ( TRUE );
      }
  
  PVUNLOCK(&ConqInfo->lockword);
  
  return ( FALSE );
  
}


/*  resign - remove a user from the user list (DOES LOCKING) */
/*  SYNOPSIS */
/*    int unum */
/*    clbResign( unum ) */
void clbResign( int unum, int isoper )
{
  int i;
  char usrname[MAXUSERNAME], usralias[MAXUSERPNAME];

				/* make copies */
  strncpy(usrname, Users[unum].username, MAXUSERNAME - 1);
  strncpy(usralias, Users[unum].alias, MAXUSERPNAME - 1);

  PVLOCK(&ConqInfo->lockword);
  if ( unum >= 0 && unum < MAXUSERS )
    {
      Users[unum].live = FALSE;

      for ( i = 0; i < MAXHISTLOG; i = i + 1 )
	if ( unum == History[i].histunum )
	  {
	    History[i].histunum = -1;
	    History[i].histlog = 0;
	  }
    }
  PVUNLOCK(&ConqInfo->lockword);

  if (isoper != TRUE)
    clog("INFO: %s (%s) has resigned",
	 usrname, usralias);
	      
  return;
}




/*  takeplanet - take a planet (DOES SPECIAL LOCKING) */
/*  SYNOPSIS */
/*    int pnum, snum */
/*    clbTakePlanet( pnum, snum ) */
/*  Note: This routines ASSUMES you have the common locked before you it. */
void clbTakePlanet( int pnum, int snum )
{
  int i;
  char buf[MSGMAXLINE];
  int oteam, didgeno;
  
  oteam = Planets[pnum].team;
  Planets[pnum].team = Ships[snum].team;
  Planets[pnum].armies = 1;
  Ships[snum].kills = Ships[snum].kills + PLANET_KILLS;
  Users[Ships[snum].unum].stats[USTAT_CONQPLANETS] += 1;
  Teams[Ships[snum].team].stats[TSTAT_CONQPLANETS] += 1;


				/* Check here for genocides */

  if ( oteam != TEAM_SELFRULED && oteam != TEAM_NOTEAM )
    {
      /* Check whether that was the last planet owned by the vanquished. */

      didgeno = 1;

      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
        if ( Planets[i].team == oteam )
	  {
	    didgeno = 0;
	    break;
	  }
      /* Yes. */
      if ( didgeno && (snum > 0 && snum <= MAXSHIPS) )
        {
          Users[Ships[snum].unum].stats[USTAT_GENOCIDE] += 1;
          Teams[Ships[snum].team].stats[TSTAT_GENOCIDE] += 1;
	  clog("INFO: %s (%s) genocided the %s team!",
	       Users[Ships[snum].unum].username,
	       Ships[snum].alias,
	       Teams[Ships[snum].team].name);
        }

    }


  sprintf( buf, "All hail the liberating %s armies.  Thanks, ",
	 Teams[Ships[snum].team].name );
  appship( snum, buf );
  appchr( '!', buf );
  
  /* Check whether the universe has been conquered. */
  for ( i = 1; i <= NUMCONPLANETS; i = i + 1 )
    if ( Planets[i].type == PLANET_CLASSM || Planets[i].type == PLANET_DEAD )
      if ( Planets[i].team != Ships[snum].team || ! Planets[i].real )
	{
	  /* No. */
	  clbStoreMsg( -pnum, -Ships[snum].team, buf );
	  return;
	}
  /* Yes! */
  getdandt( ConqInfo->conqtime, 0 );
  stcpn( Ships[snum].alias, ConqInfo->conqueror, MAXUSERPNAME );
  ConqInfo->lastwords[0] = EOS;
  Users[Ships[snum].unum].stats[USTAT_CONQUERS] += 1;
  Teams[Ships[snum].team].stats[TSTAT_CONQUERS] += 1;
  stcpn( Teams[Ships[snum].team].name, ConqInfo->conqteam, MAXTEAMNAME );

  clog("INFO: %s (%s) has Conquered the Universe!",
       Users[Ships[snum].unum].username, 
       Ships[snum].alias);
  
  clbIKill( snum, KB_CONQUER );
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status == SS_LIVE )
      clbIKill( i, KB_NEWGAME );
  
  PVUNLOCK(&ConqInfo->lockword);
  clbInitGame();
  PVLOCK(&ConqInfo->lockword);
  
  return;
  
}


/*  userline - format user statistics */
/*  SYNOPSIS */
/*    int unum, snum */
/*    char buf() */
/*    int showgods, showteam */
/*    clbUserline( unum, snum, buf, showgods, showteam ) */
/* Special hack: If snum is valid, the team and pseudonym are taken from */
/* the ship instead of the user. */
void clbUserline( int unum, int snum, char *buf, int showgods, int showteam )
{
  int team;
  char ch, ch2, junk[MSGMAXLINE], timstr[20], name[MAXUSERPNAME];
  
  char *hd1="name          pseudonym           team skill  wins  loss mxkls  ships     time";
  char tname[MAXUSERNAME + 2];	/* posss '@' and NULL */
  
  
  if ( unum < 0 || unum >= MAXUSERS )
    {
      c_strcpy( hd1, buf );
      return;
    }
  if ( ! Users[unum].live )
    {
      buf[0] = EOS;
      return;
    }
  
  ch2 = ' ';

  if ( ch2 != '+' && isagod(unum))
    ch2 = '+';		
  
  /* If we were given a valid ship number, use it's information. */
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      c_strcpy( Ships[snum].alias, name );
      team = Ships[snum].team;
    }
  else
    {

      c_strcpy( Users[unum].alias, name );
      team = Users[unum].team;
    } 

  /* Figure out which team he's on. */
  if ( Users[unum].ooptions[OOPT_MULTIPLE] && ! showteam )
    ch = 'M';
  else
    ch = Teams[team].teamchar;

  strncpy(tname, Users[unum].username, MAXUSERNAME - 1);
  tname[MAXUSERNAME - 1] = EOS;

  sprintf( junk, "%-12.12s %c%-21.21s %c %6.1f",
	   tname,
	   ch2,
	   name,
	   ch,
	   Users[unum].rating );
  
  fmtminutes( ( Users[unum].stats[USTAT_SECONDS] + 30 ) / 60, timstr );
  
  sprintf( buf, "%s %5d %5d %5d %5d %9s",
	 junk,
	 Users[unum].stats[USTAT_WINS],
	 Users[unum].stats[USTAT_LOSSES],
	 Users[unum].stats[USTAT_MAXKILLS],
	 Users[unum].stats[USTAT_ENTRIES],
	 timstr );
  
  return;
  
}



/*  statline - format a user stat line */
/*  SYNOPSIS */
/*    int unum */
/*    char buf() */
/*    clbStatline( unum, buf ) */
void clbStatline( int unum, char *buf )
{
  int i, j;
  char ch, junk[MSGMAXLINE], percent[MSGMAXLINE], morejunk[MSGMAXLINE];
  char datestr[DATESIZE];
  char tname[MAXUSERNAME + 2];	/* posss '@' and NULL */

  if ( unum < 0 || unum >= MAXUSERS )
    {
      buf[0] = EOS;
      return;
    }
  if ( ! Users[unum].live )
    {
      buf[0] = EOS;
      return;
    }
  
  if ( Users[unum].stats[USTAT_SECONDS] == 0 )
    c_strcpy( "- ", percent );
  else
    {
      i = 1000 * Users[unum].stats[USTAT_CPUSECONDS] / Users[unum].stats[USTAT_SECONDS];
      sprintf( percent, "%3d%%", (i + 5) / 10 );
    }
 
  strcpy(tname, Users[unum].username);

  sprintf( junk, "%-12s %4s %4d %4d %4d",
	 tname,
	 percent,
	 Users[unum].stats[USTAT_CONQUERS],
	 Users[unum].stats[USTAT_COUPS],
	 Users[unum].stats[USTAT_GENOCIDE] );
  
  sprintf( buf, "%s %6d %6d %4d %6d %5d",
	 junk,
	 Users[unum].stats[USTAT_CONQPLANETS],
	 Users[unum].stats[USTAT_ARMBOMB],
	 Users[unum].stats[USTAT_ARMSHIP],
	 Users[unum].stats[USTAT_PHASERS],
	 Users[unum].stats[USTAT_TORPS] );
  
  /* Convert zero counts to dashes. */
  ch = EOS;
  for ( i = 9; buf[i] != EOS; i = i + 1 )
    {
      if ( buf[i] == '0' )
	if ( ch == ' ' )
	  if ( buf[i+1] == ' ' || buf[i+1] == EOS )
	    buf[i] = '-';
      ch = buf[i];
    }

  if (Users[unum].lastentry == 0) /* never */
    {
      sprintf(junk, " %13.13s", "never");
      appstr( junk, buf );
    }      
  else
    {				/* format it properly */
      getdandt(datestr, Users[unum].lastentry);
    
      sprintf( junk, " %16.16s", datestr );
      j = 0;
      for (i=0; i<6; i++)
	{
	  morejunk[j++] = junk[i];
	}
      /* remove the seconds - ugh*/
      for (i=9; i < 17; i++)
	{
	  morejunk[j++] = junk[i];
	}
      morejunk[j] = EOS;
      
      appstr( morejunk, buf );
    }
      
  return;
}


/*  clbZeroPlanet - zero a planet (DOES SPECIAL LOCKING) */
/*  SYNOPSIS */
/*    int pnum, snum */
/*    clbZeroPlanet( pnum, snum ) */
/*  NOTE */
/*    This routines ASSUMES you have the common area locked before you it. */
void clbZeroPlanet( int pnum, int snum )
{
  int oteam, i; 
  
  oteam = Planets[pnum].team;
  Planets[pnum].team = TEAM_NOTEAM;
  Planets[pnum].armies = 0;
  
  /* Make the planet not scanned. */
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    Planets[pnum].scanned[i] = FALSE;
  
  if ( oteam != TEAM_SELFRULED && oteam != TEAM_NOTEAM )
    {
      /* Check whether that was the last planet owned by the vanquished. */
      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
	if ( Planets[i].team == oteam )
	  return;
      /* Yes. */
      Teams[oteam].couptime = rndint( MIN_COUP_MINUTES, MAX_COUP_MINUTES );
      Teams[oteam].coupinfo = FALSE;		/* lost coup info */
      if ( snum > 0 && snum <= MAXSHIPS )
	{
	  Users[Ships[snum].unum].stats[USTAT_GENOCIDE] += 1;
	  Teams[Ships[snum].team].stats[TSTAT_GENOCIDE] += 1;
          clog("INFO: %s (%s) genocided the %s team!",
               Users[Ships[snum].unum].username,
               Ships[snum].alias,
               Teams[Ships[snum].team].name);
	}
    }
  
  return;
  
}

char *clbWarPrompt(int snum, int twar[])
{
  static char wbuf[BUFFER_SIZE];
  static char *fmt = "Press TAB when done, ESCAPE to abort:  Peace: %c %c %c %c  War: %c %c %c %c";
  int i;
  char ch, peace[NUMPLAYERTEAMS], war[NUMPLAYERTEAMS];

  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    if ( twar[i] )
      {
        if ( Ships[snum].rwar[i] )
          ch = Teams[i].teamchar;
        else
          ch = (char)tolower(Teams[i].teamchar);
        
        peace[i] = ' ';
        war[i] = ch;
      }
    else
      {
        peace[i] = (char)tolower(Teams[i].teamchar);
        war[i] = ' ';
      }

  sprintf(wbuf, fmt, 
          peace[0],
          peace[1],
          peace[2],
          peace[3],
          war[0],
          war[1],
          war[2],
          war[3]);

  return wbuf;
}



/*  canread - determine if a message is readable */
/*  SYNOPSIS */
/*    int ok, canread */
/*    int snum, msgnum */
/*    ok = clbCanRead( snum, msgnum ) */
int clbCanRead( int snum, int msgnum )
{
  int from, to;
  unsigned char flags;
  
  from = Msgs[msgnum].msgfrom;
  to = Msgs[msgnum].msgto;
  flags = Msgs[msgnum].flags;
  
  if (from == 0 && to == 0)
    {				/* uninitialized msgs */
      return(FALSE);		/* no point in reading it */
    }

  /* If we're GOD, we can read it. unless it's a COMP MSG*/
  if ( snum == MSG_GOD && from != MSG_COMP)
    {
      return ( TRUE );
    }
  
  /* It's to us. */
  if ( to == snum )
    return(TRUE);

  /* It's from us */
  if (from == snum)
    return(TRUE);
  
  /* if it's to god, and we are an oper... */
  if (to == MSG_GOD && Users[Ships[snum].unum].ooptions[OOPT_OPER])
    return TRUE;

  /* It's to everybody. */
  if ( to == MSG_ALL )
    return(TRUE);
  
  /* Only check these if we're a ship. */
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      /* We can only read team messages if we're not self-war. */
      if ( ( -to == Ships[snum].team ) && ! selfwar(snum) )
	return ( TRUE );
      
      /* see if it's a message to friendly ships from another ship */

      if (to == MSG_FRIENDLY && (from > 0 && from <= MAXSHIPS))
	{
	  if (Ships[snum].war[Ships[from].team] == FALSE && 
	      Ships[from].war[Ships[snum].team] == FALSE)
	    return TRUE;
	}

      /* See if we are allowed to read GOD messages. */
      if ( to == MSG_GOD || from == MSG_GOD || to == MSG_IMPLEMENTORS )
	return ( Users[Ships[snum].unum].ooptions[OOPT_OPER] );
    }
  
  /* If we got here, we can't read it. */
  return ( FALSE );
  
}


/*  clearships - reset ships and torpedoes */
/*  SYNOPSIS */
/*    clearships */
void clbClearShips(void)
{
  
  int i;
  
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    clbZeroShip( i );
  
  return;
  
}


/*  cvtcoords - convert from internal coords to screen coords */
/*  SYNOPSIS */
/*    int inbounds, cvtcoords */
/*    real cenx, ceny, x, y, scale */
/*    int lin, col */
/*    inbounds = clbCVTCoords( ceny, ceny, x, y, scale, lin, col ) */
int clbCVTCoords( real cenx, real ceny, real x, real y, real scale, 
	      int *lin, int *col )
{
  *col = round( (Context.maxcol-STAT_COLS)/2 + (x-cenx) / scale * WIDTH_FAC ) +
    STAT_COLS;
  
  *lin = round( (DISPLAY_LINS/2+1) - (y-ceny) / scale );
  if ( *lin < 0 || *lin > DISPLAY_LINS || *col <= STAT_COLS || *col > Context.maxcol )
    return ( FALSE );
  
  return ( TRUE );
  
}


/*  doomfind - find a planet or ship for the doomsday machine to head for */
/*  SYNOPSIS */
/*    doomfind */
void clbDoomFind(void)
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
void clbDoomsday(void)
{
  
  Doomsday->heading = rnduni( 0.0, 360.0 );
  Doomsday->x = DOOMSDAY_START_DIST * cosd(Doomsday->heading);
  Doomsday->y = DOOMSDAY_START_DIST * sind(Doomsday->heading);
  clbDoomFind();
  Doomsday->status = DS_LIVE;
  
  return;
  
}


/*  findorbit - find a planet for a ship to orbit */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    int flag, findorbit */
/*    flag = clbFindOrbit( snum, pnum ) */
int clbFindOrbit( int snum, int *pnum )
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
/*    truth = clbFindShip( snum ) */
int clbFindShip( int *snum )
{
  int i;
  PVLOCK(&ConqInfo->lockword);
  *snum = -1;
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    {
      /* first, look for reserved ships that have no valid pid */
      if ( Ships[i].status == SS_RESERVED )
        if (!checkPID(Ships[i].pid))
          {
            Ships[i].status = SS_OFF; /* no-one there, turn it off */
            clog("INFO: clbFindShip(): turned off reserved ship %d\n",
                 i);
          }

      /* if it's off, grab it */
      if ( Ships[i].status == SS_OFF )
        {
          *snum = i;
          clbZeroShip( *snum );
          Ships[*snum].status = SS_RESERVED;
          Ships[*snum].lastmsg = LMSG_NEEDINIT;
          Ships[*snum].sdfuse = -TIMEOUT_PLAYER;
          Ships[*snum].ctime = 0;
          Ships[*snum].etime = 0;
          Ships[*snum].cacc = 0;
          Ships[*snum].eacc = 0;
          break;
        }
    }
  PVUNLOCK(&ConqInfo->lockword);
  
  return ( *snum != -1 );
  
}


/*  findspecial - search for nearest some-thing */
/*  SYNOPSIS */
/*    int flag, findspecial */
/*    int snum, token, count, sorpnum, xsorpnum */
/*    flag = clbFindSpecial( snum, token, count, sorpnum, xsorpnum ) */
int clbFindSpecial( int snum, int token, int count, int *sorpnum, int *xsorpnum )
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
		if (SCLOAKED(i) && Ships[i].warp == 0.0 && 
		    satwar(snum, i) &&
		    SROBOT(snum))
		  continue; /* nobody here but us chickens... */
#endif /* WARP0CLOAK */
		td = dist(Ships[snum].x, Ships[snum].y, Ships[i].x, Ships[i].y);
		if ( td < nd )
		  {
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
		{
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
			clbSPWar( snum, i ) &&
			Planets[i].type != PLANET_MOON ) );
	      break;
	    case SPECIAL_FUELPLANET:
	      valid = ( ( Planets[i].scanned[Ships[snum].team] || peaceful ) &&
		       ! clbSPWar( snum, i ) &&
		       Planets[i].armies > 0 &&
		       Planets[i].type == PLANET_CLASSM );
	      break;
	    case SPECIAL_PLANET:
	      valid = TRUE;
	      break;
	    case SPECIAL_REPAIRPLANET:
	      valid = ( ( Planets[i].scanned[Ships[snum].team] || peaceful ) &&
		       ! clbSPWar( snum, i ) &&
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
	    {
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
	    }
	  if ( valid )
	    {
	      td = dist(Ships[snum].x, Ships[snum].y, Planets[i].x, Planets[i].y);
	      if ( td < nd )
		{
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
/*    clbFixDeltas( snum ) */
void clbFixDeltas( int snum )
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
/*    truth = clbGetUserNum( unum, lname ) */
int clbGetUserNum( int *unum, char *lname, int ltype )
{
  int i;
  char *lptr = lname;

  *unum = -1;
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    if ( Users[i].live )
      if ( strcmp( lptr, Users[i].username ) == 0 )
	{
	  *unum = i;
	  return ( TRUE );
	}
  
  return ( FALSE );
  
}

/*  initeverything - initialize (with extra cheese and tomato) (DOES LOCKING) */
/*  SYNOPSIS */
/*    initeverything */
void clbInitEverything(void)
{
  
  int i, j;
  
  /* Twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  PVUNLOCK(&ConqInfo->lockmesg);

  /* Zero EVERYTHING. */
  clbZeroEverything();

  PVLOCK(&ConqInfo->lockword);
  
  /* Turn off the universe. */
  ConqInfo->closed = TRUE;
  
  /* reset the lockwords  */
  ConqInfo->lockword = 0;
  ConqInfo->lockmesg = 0;

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
  
  clbInitRobots();
  clbInitUniverse();
  
  return;
  
}


/*  initgame - initialize the game-permanent variables */
/*  SYNOPSIS */
/*    initgame */
void clbInitGame(void)
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
  clbInitPlanets();
  
  return;
  
}


/*  initmsgs - initialize the message data structures */
/*  SYNOPSIS */
/*    initmsgs */
void clbInitMsgs(void)
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
void clbInitPlanets(void)
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
void clbInitRobots(void)
{
  int i, j, unum;
  
  /* SETROBOT( name, pname, team ) */
#define SETROBOT(x, y, z) \
  { \
      if ( clbGetUserNum( &unum, x, UT_LOCAL ) ) \
          stcpn( y, Users[unum].alias, MAXUSERPNAME ); \
      else if ( clbRegister( x, y, z, &unum ) ) \
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
/*    clbInitShip( snum, unum ) */
/* Note: this routine assumes that the ship is reserved. */
void clbInitShip( int snum, int unum )
{
  int i, j;
  
  /* sstatus(snum)				# never modified here */
  Ships[snum].killedby = 0;
  /* suser(snum)				# setup in menu() or newrob() */
  /* steam(snum)				# setup in menu() or newrob() */
  /* spid(snum)				# setup in menu() or newrob() */
  /* shiptype - setup in newship and newrob */
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
  SFSET(snum, SHIP_F_SHUP);
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
  SFCLR(snum, SHIP_F_REPAIR);
  SFCLR(snum, SHIP_F_CLOAKED);
  /* soption(snum,i)				# setup in menu() */
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      /* srwar(snum,i)				# setup in menu() or newrob() */
      /* swar(snum,i)				# setup in menu() or newrob() */
      Ships[snum].scanned[i] = 0;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    Ships[snum].srpwar[i] = FALSE;
  /* ssdfuse(snum)				# setup in clbFindShip() */
  PVLOCK(&ConqInfo->lockmesg);
  if ( Ships[snum].lastmsg == LMSG_NEEDINIT )
    {
      Ships[snum].lastmsg = ConqInfo->lastmsg;
      Ships[snum].alastmsg = Ships[snum].lastmsg;
    }
  PVUNLOCK(&ConqInfo->lockmesg);
  SFCLR(snum, SHIP_F_MAP);
  Ships[snum].towing = 0;
  Ships[snum].towedby = 0;
  Ships[snum].lastblast = 0.0;
  Ships[snum].lastphase = 0.0;
  Ships[snum].pfuse = 0;
  SFCLR(snum, SHIP_F_TALERT);
  SFCLR(snum, SHIP_F_ROBOT);
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

  Ships[snum].flags = SHIP_F_NONE;
  return;
  
}


/*  inituniverse - initialize (without cheese and tomato) (DOES LOCKING) */
/*  SYNOPSIS */
/*    inituniverse */
void clbInitUniverse(void)
{
  
  int i;
  
  /* Twiddle the lockword. */
  PVUNLOCK(&ConqInfo->lockword);
  PVLOCK(&ConqInfo->lockword);
  
  /* Turn off the universe. */
  ConqInfo->closed = TRUE;
  
  stcpn( "Scout", ShipTypes[ST_SCOUT].name, MAXSTNAME );
  ShipTypes[ST_SCOUT].armylim = 7;
  ShipTypes[ST_SCOUT].warplim = 10.0;
  ShipTypes[ST_SCOUT].engfac = 1.2;
  ShipTypes[ST_SCOUT].accelfac = 1.6;
  ShipTypes[ST_SCOUT].weafac = 0.83;
  ShipTypes[ST_SCOUT].torpwarp = 14.0;

  stcpn( "Destroyer", ShipTypes[ST_DESTROYER].name, MAXSTNAME );
  ShipTypes[ST_DESTROYER].armylim = 9;
  ShipTypes[ST_DESTROYER].warplim = 9.0;
  ShipTypes[ST_DESTROYER].engfac = 1.0;
  ShipTypes[ST_DESTROYER].accelfac = 1.0;
  ShipTypes[ST_DESTROYER].weafac = 1.0;
  ShipTypes[ST_DESTROYER].torpwarp = 12.0;

  stcpn( "Cruiser", ShipTypes[ST_CRUISER].name, MAXSTNAME );
  ShipTypes[ST_CRUISER].armylim = 11;
  ShipTypes[ST_CRUISER].warplim = 8.0;
  ShipTypes[ST_CRUISER].engfac = 0.8;
  ShipTypes[ST_CRUISER].accelfac = 0.8;
  ShipTypes[ST_CRUISER].weafac = 1.17;
  ShipTypes[ST_CRUISER].torpwarp = 10.0;

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
  
  Teams[TEAM_FEDERATION].shiptype = ST_DESTROYER;
  Teams[TEAM_KLINGON].shiptype = ST_DESTROYER;
  Teams[TEAM_ROMULAN].shiptype = ST_CRUISER;
  Teams[TEAM_ORION].shiptype = ST_SCOUT;
  
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
  
  clbInitGame();
  clbClearShips();
  clbInitMsgs();
  
  return;
  
}


/*  intrude - possibly send an intruder alert */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    clbIntrude( snum, pnum ) */
void clbIntrude( int snum, int pnum )
{
  char buf[MSGMAXLINE];
  string atta=" attacking";
  string armeq=", armies=";
  
  if ( Planets[pnum].real &&
      Planets[pnum].team != TEAM_SELFRULED &&
      Planets[pnum].team != TEAM_NOTEAM )
    {
      if ( snum == MSG_DOOM )
	{
	  c_strcpy( Doomsday->name, buf );
	  upper( Doomsday->name );
	  appstr( atta, buf );
	  appstr( armeq, buf );
	  appint( Planets[pnum].armies, buf );
	  clbStoreMsgf( -pnum, -Planets[pnum].team, buf, MSG_FLAGS_INTRUDER );
	}
      else if ( Ships[snum].war[Planets[pnum].team] )
	{
	  c_strcpy( "INTRUDER ALERT - ", buf );
	  appship( snum, buf );
	  appstr( atta, buf );
	  appstr( armeq, buf );
	  appint( Planets[pnum].armies, buf );
	  clbStoreMsgf( -pnum, -Planets[pnum].team, buf, MSG_FLAGS_INTRUDER );
	  defend( snum, pnum );
	}
    }
  
  return;
  
}


/*  loghist - log this entry into the Game (DOES LOCKING) */
/*  SYNOPSIS */
/*    int unum */
/*    clbLogHist( unum ) */
int clbLogHist( int unum )
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
/*    warp = clbNewWarp( snum, dwarp ) */
real clbNewWarp( int snum, real dwarp )
{
  real x, acc;
  
  x = dwarp - Ships[snum].warp;
  acc = (real) ShipTypes[Ships[snum].shiptype].accelfac * 
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
/*    phase = clbPhoon( pnum ) */
int clbPhoon( int pnum )
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
/*    status = clbPlanetMatch( str, pnum, godlike ) */
int clbPlanetMatch( char *str, int *pnum, int godlike )
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


/*  putship - place a ship in the universe */
/*  SYNOPSIS */
/*    int snum */
/*    real basex, basey */
/*    clPutShip( snum, basex, basey ) */
void clPutShip( int snum, real basex, real basey )
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
  clog( "clPutShip(): Failed retry maximum on ship %d", snum );
  
  return;
  
}


/* fmtmsg - format a message string -JET */
int clbFmtMsg(int to, int from, char *buf)
{

  buf[0] = '\0';

				/* Format who the message is from. */
  if ( from > 0 && from <= MAXSHIPS )
    {
      appship( from, buf );
    }
  else if ( -from > 0 && -from <= NUMPLANETS )
    c_strcpy( Planets[-from].name, buf );
  else switch ( from )
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
  if ( to > 0 && to <= MAXSHIPS )
    appship( to, buf );
  else if ( -to >= 0 && -to < NUMPLAYERTEAMS )
    {
      appchr( Teams[-to].teamchar, buf );
    }
  else 
    {
      switch ( to )
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
	case MSG_FRIENDLY:
	  appstr( "FRIEND", buf );
	  break;
	default:
	  appstr( "???", buf );
	  break;
	}
    }

  return(TRUE);
}


/* cmpplanet - compare planet names based on index passed via qsort() */
static int cmpplanet(void *cmp1, void *cmp2)
{
  register int *icmp1, *icmp2;
  
  icmp1 = (int *) cmp1;
  icmp2 = (int *) cmp2;
  
  return(strcmp(Planets[*icmp1].name, Planets[*icmp2].name));
}

/*  sortplanets - sort planets by name */
/*  SYNOPSIS */
/*    int sv(NUMPLANETS) */
/*    clbSortPlanets( sv ) */
/* This routine ASSUMES that the sort vector is initialized, */
/* for the reason that it is fastest to sort a list that is */
/* already sorted. */
void clbSortPlanets( int sv[] )
{
  qsort(&sv[1], NUMPLANETS, sizeof(int), 
	(int (*)(const void *, const void *))cmpplanet);
  
  return;
  
}

/* cmpuser - compare users based on skill */
static int cmpuser(void *cmp1, void *cmp2)
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
void clbSortUsers( int uv[], int numentries )
{
  qsort(uv, numentries, sizeof(int), 
	(int (*)(const void *, const void *))cmpuser);
  
  return;
  
}

/*  spwar - test whether a ship is at war with a planet */
/*  SYNOPSIS */
/*    int atwar, spwar */
/*    int snum, pnum */
/*    atwar = clbSPWar( snum, pnum ) */
int clbSPWar( int snum, int pnum )
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
  
}


/*  stillalive - determine if a ship is still alive */
/*  SYNOPSIS */
/*    int flag, stillalive */
/*    int snum */
/*    flag = clbStillAlive( snum ) */
int clbStillAlive( int snum )
{


  if (snum < 0 || snum > MAXSHIPS)
    return(TRUE);

  /* Look for religious trouble or the "closed" sign in the window. */
  if ( Users[Ships[snum].unum].ooptions[OOPT_SHITLIST] )
    {
      if ( Ships[snum].status == SS_LIVE )
	clbKillShip( snum, KB_SHIT );

      return ( FALSE );
    }
  if ( ConqInfo->closed && ! Users[Ships[snum].unum].ooptions[OOPT_PLAYWHENCLOSED] )
    {
      if ( Ships[snum].status == SS_LIVE )
	clbKillShip( snum, KB_EVICT );

      return ( FALSE );
    }

  if ( Ships[snum].status == SS_RESERVED || Ships[snum].status == SS_ENTERING )
    return ( TRUE );

  return ( Ships[snum].status == SS_LIVE );
  
}

/* wrapper for stormsg.  Most routines just use this version. */
void clbStoreMsg( int from, int to, char *msg )
{
  clbStoreMsgf(from, to, msg, MSG_FLAGS_NONE);
  return;
}

/*  stormsgf - store a message in the msg buffer with flags (DOES LOCKING) */
void clbStoreMsgf( int from, int to, char *msg, unsigned char flags )
{
  int nlastmsg, i;
  char buf[128];

				/* don't do this if invalid common block */
  if (*CBlockRevision != COMMONSTAMP)
    return;

  /* don't bother with tersables/feedbacks to robots */
  if (to > 0 && to <= MAXSHIPS && SROBOT(to))
    if (flags & (MSG_FLAGS_TERSABLE | MSG_FLAGS_FEEDBACK))
      return;

  PVLOCK(&ConqInfo->lockmesg);
  nlastmsg = modp1( ConqInfo->lastmsg + 1, MAXMESSAGES );
  stcpn( msg, Msgs[nlastmsg].msgbuf, MESSAGE_SIZE );
  Msgs[nlastmsg].msgfrom = from;
  Msgs[nlastmsg].msgto = to;
  Msgs[nlastmsg].flags = flags;
  ConqInfo->lastmsg = nlastmsg;
  
  /* Remove allowable last message restrictions. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( nlastmsg == Ships[i].alastmsg )
      Ships[i].alastmsg = LMSG_READALL;
  PVUNLOCK(&ConqInfo->lockmesg);

  if (SysConf.LogMessages == TRUE)
    {
      clbFmtMsg(to, from, buf);
      clog("MSG: %s: %s",
	   buf, msg);
    }

  
  return;
  
}


/*  usefuel - consume a quantity of matter-antimatter devices */
/*  SYNOPSIS */
/*    int snum */
/*    real fuel */
/*    int ok, usefuel, weapon */
/*    ok = clbUseFuel( snum, fuel, weapon ) */
int clbUseFuel( int snum, real fuel, int weapon, int forreal )
{
  /* 'forreal' allows the client to test whether a command would succeed
     before bothering to send it to the server (where it will just be
     ignored anyway if you don't have the fuel).  */

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
	  if (forreal)
	    Ships[snum].dwarp = 0.0;
	  return ( FALSE );
	}
    }

  if (forreal)
    Ships[snum].fuel = Ships[snum].fuel - fuel;

  if ( Ships[snum].fuel < 0.0 )
    {
      /* When you're out of gas, you're out of fun... */
      if (forreal) 
	{
	  Ships[snum].fuel = 0.0;
	  SFCLR(snum, SHIP_F_CLOAKED);
	  Ships[snum].dwarp = 0.0;
	}
      return ( FALSE );
    }
  else if ( Ships[snum].fuel > 999.0 )
    {
      if (forreal)
	Ships[snum].fuel = 999.0;
    }
  
  /* Temperature. */
  if ( weapon )
    {
      if (forreal)
	{
	  /* the server will send this message if needed */
	  Ships[snum].wtemp += ((fuel * TEMPFUEL_FAC) / weaeff ( snum ));
	  if ( Ships[snum].wtemp < 0.0 )
	    Ships[snum].wtemp = 0.0;
	  else if ( Ships[snum].wtemp >= 100.0 )
	    if ( rnd() < WEAPON_DOWN_PROB )
	      {
		Ships[snum].wfuse = rndint( MIN_DOWN_FUSE, MAX_DOWN_FUSE );
		clbStoreMsgf( MSG_COMP, snum, "Weapons overload.",
			  MSG_FLAGS_TERSABLE);
	      }
	}
    }
  else
    {
      if (forreal)
	{
	  /* the server will send this message if needed */
	  Ships[snum].etemp = Ships[snum].etemp + fuel * TEMPFUEL_FAC / engeff( snum );
	  if ( Ships[snum].etemp < 0.0 )
	    Ships[snum].etemp = 0.0;
	  else if ( Ships[snum].etemp >= 100.0 )
	    if ( rnd() < ENGINE_DOWN_PROB )
	      {
		Ships[snum].efuse = rndint( MIN_DOWN_FUSE, MAX_DOWN_FUSE );
		clbStoreMsgf( MSG_COMP, snum, "Engines super-heated.",
			  MSG_FLAGS_TERSABLE);
	      }
	}
    }
  
  return ( TRUE );
  
}


/*  zeroeverything - initialize the common block to zeros */
/*  SYNOPSIS */
/*    zeroeverything */
void clbZeroEverything(void)
{
  
  zero_common();
  
  return;
  
}


/*  zeroship - turn OFF a ship and its torpedos */
/*  SYNOPSIS */
/*    int snum */
/*    clbZeroShip( snum ) */
void clbZeroShip( int snum )
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
  SFSET(snum, SHIP_F_SHUP);
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
  SFCLR(snum, SHIP_F_REPAIR);
  SFCLR(snum, SHIP_F_CLOAKED);
  Ships[snum].shiptype = ST_SCOUT;
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
  SFCLR(snum, SHIP_F_MAP);
  Ships[snum].towing = 0;
  Ships[snum].towedby = 0;
  Ships[snum].lastblast = 0.0;
  Ships[snum].lastphase = 0.0;
  Ships[snum].pfuse = 0;
  SFCLR(snum, SHIP_F_TALERT);
  SFCLR(snum, SHIP_F_ROBOT);
  Ships[snum].action = 0;
  for ( i = 0; i < MAXUSERPNAME; i = i + 1 )
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

/* get the current username.  We really only need to look it up once
   per session... returns a pointer to a static string. */
char *clbGetUserLogname(void)
{
#define MAXPWNAME 128
  struct passwd *pwd = NULL;
  static char pwname[MAXPWNAME] = "";

  if (pwname[0] == 0)
    {
      if ((pwd = getpwuid(geteuid())) == NULL)
	{
	  clog("ERROR: clbGetUserLogname(): getpwuid(geteuid()) failed: %s",
	       strerror(errno));
	  
	  pwname[0] = 0;
	}
      else
	{
	  memset(pwname, 0, MAXPWNAME);
	  strncpy(pwname, pwd->pw_name, MAXPWNAME - 1);
	}
    }

  return pwname;
}

/*  planetdrive - move the planets based on interval */
/*  SYNOPSIS */
/*    planetdrive */
void clbPlanetDrive(real itersec)
{
  int i;
  real speed;

  for ( i = NUMPLANETS; i > 0; i = i - 1 )
    {
      /* Advance porbang(). */
      if ( Planets[i].primary != 0 )
	{

	  Planets[i].orbang = mod360( Planets[i].orbang + 
                                      Planets[i].orbvel *
                                      itersec / 60.0 );
        
	  Planets[i].x = Planets[Planets[i].primary].x + 
	    Planets[i].orbrad * cosd(Planets[i].orbang);
	  Planets[i].y = Planets[Planets[i].primary].y + 
	    Planets[i].orbrad * sind(Planets[i].orbang);
	  
	}
      else if ( Planets[i].orbvel != 0.0 )
	{
	  /* Special hack for planets to move in a straight line. */
	  speed = Planets[i].orbvel * MM_PER_SEC_PER_WARP * itersec;
	  Planets[i].x = Planets[i].x + speed * cosd(Planets[i].orbang);
	  Planets[i].y = Planets[i].y + speed * sind(Planets[i].orbang);
	}
    }
  
  return;
  
}

/*  torpdrive - move the torps based on interval */
void clbTorpDrive(real itersec)
{
  int s, i, j;
  static int ship[MAXSHIPS + 1];
  static int FirstTime = TRUE;
    
  if (FirstTime)
    {
      FirstTime = FALSE;
      /* Randomize ship ordering. */

      for ( s = 1; s <= MAXSHIPS; s = s + 1 )
        ship[s] = s;

      for ( s = 1; s <= MAXSHIPS; s = s + 1 )
        {
          i = rndint( 1, MAXSHIPS );
          j = ship[i];
          ship[i] = ship[s];
          ship[s] = j;
          
        }
    }

  for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
      i = ship[s];
      if ( Ships[i].status != SS_OFF )
	{
	  for ( j = 0; j < MAXTORPS; j = j + 1 )
	    {
	      if ( Ships[i].torps[j].status == TS_LIVE )
		{
		  /* Movement. */
		  Ships[i].torps[j].x = Ships[i].torps[j].x + 
                    (Ships[i].torps[j].dx * (itersec / ITER_SECONDS));
		  Ships[i].torps[j].y = Ships[i].torps[j].y + 
                    (Ships[i].torps[j].dy * (itersec / ITER_SECONDS));

		}
            }
        }
    }

  return;
}

/* borrowed from glut */
#if defined(SVR4) && !defined(sun)  /* Sun claims SVR4, but
                                       wants 2 args. */
#define GETTIMEOFDAY(_x) gettimeofday(_x)
#else
#define GETTIMEOFDAY(_x) gettimeofday(_x, NULL)
#endif
#define ADD_TIME(dest, src1, src2) { \
  if(((dest).tv_usec = \
    (src1).tv_usec + (src2).tv_usec) >= 1000000) { \
    (dest).tv_usec -= 1000000; \
    (dest).tv_sec = (src1).tv_sec + (src2).tv_sec + 1; \
  } else { \
    (dest).tv_sec = (src1).tv_sec + (src2).tv_sec; \
    if(((dest).tv_sec >= 1) && (((dest).tv_usec <0))) { \
      (dest).tv_sec --;(dest).tv_usec += 1000000; \
    } \
  } \
}
#define TIMEDELTA(dest, src1, src2) { \
  if(((dest).tv_usec = (src1).tv_usec - (src2).tv_usec) < 0) { \
    (dest).tv_usec += 1000000; \
    (dest).tv_sec = (src1).tv_sec - (src2).tv_sec - 1; \
  } else { \
     (dest).tv_sec = (src1).tv_sec - (src2).tv_sec; \
  } \
}

/* return time in milliseconds */
Unsgn32 clbGetMillis(void)
{
  static int firsttime = TRUE;
  static struct timeval start;
  struct timeval elapsed, now;
  Unsgn32 elapse;

  if (firsttime)
    {
      firsttime = FALSE;
      
      GETTIMEOFDAY(&start);
    }

  GETTIMEOFDAY(&now);
  TIMEDELTA(elapsed, now, start);
  /* Return elapsed milliseconds. */
  elapse = (Unsgn32) ((elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000));

  /* don't return 0 */
  if (elapse == 0)
    elapse = 1;

  return elapse;
}

  
