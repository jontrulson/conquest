#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
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
#include "conqcom2.h"
#include "global.h"
#include "color.h"

				/* shared with display.c */
real LastPhasDist = PHASER_DIST;

/*  chalkup - perform kills accoutinng */
/*  SYNOPSIS */
/*    int snum */
/*    chalkup( snum ) */
/*  Note: This routines ASSUMES you have the common locked before you it. */
void chalkup( int snum )
{
  int i, unum, team;
  real x, w, l, m;
  
  unum = suser[snum];
  team = steam[snum];
  
  /* Update wins. */
  ustats[unum][USTAT_WINS] = ustats[unum][USTAT_WINS] + ifix(skills[snum]);
  tstats[team][TSTAT_WINS] = tstats[team][TSTAT_WINS] + ifix(skills[snum]);
  
  /* Update max kills. */
  i = ifix( skills[snum] );
  if ( i > ustats[unum][USTAT_MAXKILLS] )
    ustats[unum][USTAT_MAXKILLS] = i;
  
  /* Update rating. */
  l = ustats[unum][USTAT_LOSSES];
  if ( l == 0 )
    l = 1;
  w = ustats[unum][USTAT_WINS];
  m = ustats[unum][USTAT_MAXKILLS];
  urating[unum] = ( w / l ) + ( m / 4.0 );
  x = w - l;
  if ( x >= 0.0 )
    urating[unum] = urating[unum] + pow((real) x, (real) ( 1.0 / 3.0 ));
  else
    urating[unum] = urating[unum] - pow((real) -x, (real) ( 1.0 / 3.0 ));
  
  return;
  
}


/*  cloak - attempt to engage the cloaking device */
/*  SYNOPSIS */
/*    int didit, cloak */
/*    int snum */
/*    didit = cloak( snum ) */
int cloak( int snum )
{
  srmode[snum] = FALSE;
  if ( ! usefuel( snum, CLOAK_ON_FUEL, FALSE ) )
    return ( FALSE );
  scloaked[snum] = TRUE;
  return ( TRUE );
  
}


/*  damage - damage a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    real dam */
/*    damage( snum, dam, kb ) */
void damage( int snum, real dam, int kb )
{
  real mw;
  
  sdamage[snum] = sdamage[snum] + dam;
  if ( sdamage[snum] >= 100.0 )
    killship( snum, kb );
  else
    {
      mw = maxwarp( snum );
      sdwarp[snum] = min( sdwarp[snum], mw );
    }
  
  return;
  
}

/*  detonate - blow up a torpedo (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, tnum */
/*    detonate( snum, tnum ) */
void detonate( int snum, int tnum )
{
  
  PVLOCK(lockword);
  if ( tstatus[snum][tnum] == TS_LIVE )
    tstatus[snum][tnum] = TS_DETONATE;
  PVUNLOCK(lockword);
  
  return;
  
}
/*  enemydet - detonate enemy torpedos */
/*  SYNOPSIS */
/*    int didit, enemydet */
/*    int snum */
/*    didit = enemydet( snum ) */
int enemydet( int snum )
{
  int i, j;
  
  /* Stop repairing. */
  srmode[snum] = FALSE;
  
  if ( ! usefuel( snum, DETONATE_FUEL, TRUE ) )
    return ( FALSE );
  
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] != SS_OFF && i != snum )
      for ( j = 0; j < MAXTORPS; j = j + 1 )
	if ( tstatus[i][j] == TS_LIVE )
	  if ( twar[i][j][steam[snum]] || swar[snum][steam[i]] )
	    if ( dist( sx[snum], sy[snum], tx[i][j], ty[i][j] ) <=
		DETONATE_DIST )
	      detonate( i, j );
  
  return ( TRUE );
  
}


/*  hit - hit a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    real ht */
/*    hit( snum, ht, kb ) */
void hit( int snum, real ht, int kb )
{
  if ( ht > 0.0 )
    if ( sshup[snum] && ! srmode[snum] )
      if ( ht > sshields[snum] )
	{
 	  damage( snum, ht-sshields[snum], kb ); 
	  sshields[snum] = 0.0;
	}
      else
	{
	  sshields[snum] = sshields[snum] - ht;
	}
    else
      {
	damage( snum, ht, kb );
      }

  return;
}


/*  ikill - ikill a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    ikill( snum, kb ) */
/*  Note: This routines ASSUMES you have the common locked before you it. */
void ikill( int snum, int kb )
{
  int i, unum, team, kunum, kteam;
  real tkills;
  
  /* Only procede if the ship is alive */
  if ( sstatus[snum] != SS_LIVE )
    return;
  
  /* The ship is alive; kill it. */
  skilledby[snum] = kb;
  sstatus[snum] = SS_DYING;
  
  unum = suser[snum];
  team = steam[snum];

  /* Detonate all torpedos. */

  for ( i = 0; i < MAXTORPS; i = i + 1 )
    if ( tstatus[snum][i] == TS_LIVE )
      tstatus[snum][i] = TS_DETONATE;
  
  /* Release any tows. */
  if ( stowing[snum] != 0 )
    stowedby[stowing[snum]] = 0;
  if ( stowedby[snum] != 0 )
    stowing[stowedby[snum]] = 0;
  
  /* Zero team scan fuses. */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    sscanned[snum][i] = 0;
  
  if ( kb == KB_CONQUER )
    skills[snum] = skills[snum] + CONQUER_KILLS;
  else if ( kb == KB_GOTDOOMSDAY )
    skills[snum] = skills[snum] + DOOMSDAY_KILLS;
  else if ( kb >= 0 )				/* if a ship did the killing */
    {
      kunum = suser[kb];
      kteam = steam[kb];
      tkills = 1.0 + ((skills[snum] + sstrkills[snum]) * KILLS_KILLS);
      if ( sarmies[snum] > 0 )
	{
	  /* Keep track of carried armies killed - they are special. */
	  tkills = tkills + sarmies[snum] * ARMY_KILLS;
	  ustats[kunum][USTAT_ARMSHIP] =
	    ustats[kunum][USTAT_ARMSHIP] + sarmies[snum];
	  tstats[kteam][TSTAT_ARMSHIP] =
	    tstats[kteam][TSTAT_ARMSHIP] + sarmies[snum];
	}
      
      /* Kills accounting. */
      if ( sstatus[kb] == SS_LIVE )
	skills[kb] = skills[kb] + tkills;
      else
	{
	  /* Have to do some hacking when our killer is dead. */
	  ustats[kunum][USTAT_WINS] =
	    ustats[kunum][USTAT_WINS] - ifix(skills[kb]);
	  tstats[kteam][TSTAT_WINS] =
	    tstats[kteam][TSTAT_WINS] - ifix(skills[kb]);
	  skills[kb] = skills[kb] + tkills;
	  chalkup( kb );
	}
      
      /* Sticky war logic. */
				/* should set sticky war too. -JET */

      if ( ! swar[snum][kteam] )
	{
	  swar[kb][team] = TRUE;
          srwar[kb][team] = TRUE;
	}
    }
  
  /* Kills accounting. */
  chalkup( snum );
  if ( kb != KB_SELF && kb != KB_CONQUER && kb != KB_NEWGAME &&
      kb != KB_EVICT && kb != KB_SHIT && kb != KB_GOD )
    {
      /* Update losses. */
      ustats[unum][USTAT_LOSSES] = ustats[unum][USTAT_LOSSES] + 1;
      tstats[team][TSTAT_LOSSES] = tstats[team][TSTAT_LOSSES] + 1;
    }
  
  if ( ! srobot[snum] || spid[snum] != 0 )
    {
      sstatus[snum] = SS_DEAD;
      ssdfuse[snum] = -TIMEOUT_PLAYER;		/* setup dead timeout timer */
    }
  else
    {
#if defined(DO_EXPLODING_SHIPS)
      sstatus[snum] = SS_DEAD;
      ssdfuse[snum] = -5;          /* setup dead timeout timer,
				      for robots, that's 5 seconds, this
				      gives enough time for detonating torps,
				      just like regular players */
#else
				/* old behavior */
      sstatus[snum] = SS_OFF;   /* turn robots off */
#endif

      /* We'd like to remove this next line so that you could */
      /* use conqoper to see what killed him, but then robots */
      /* show up on the debugging playlist... */
      /*      skilledby[snum] = 0;*/
    }
  
  return;
  
}


/*  infoplanet - write out information about a planet */
/*  SYNOPSIS */
/*    char str() */
/*    int pnum, snum */
/*    infoplanet( str, pnum, snum ) */
void infoplanet( char *str, int pnum, int snum )
{
  int i, j; 
  int godlike, canscan; 
  char buf[MSGMAXLINE*2], junk[MSGMAXLINE];
  real x, y;
  
  /* Check range of the passed planet number. */
  if ( pnum <= 0 || pnum > NUMPLANETS )
    {
      c_putmsg( "No such planet.", MSG_LIN1 );
      cdclrl( MSG_LIN2, 1 );
      cdmove( MSG_LIN1, 1 );
      cerror("infoplanet: Called with invalid pnum (%d).",
	     pnum );
      return;
    }
  
  /* GOD is too clever. */
  godlike = ( snum < 1 || snum > MAXSHIPS );
  
  /* In some cases, report hostilities. */
  junk[0] = EOS;
  if ( ptype[pnum] == PLANET_CLASSM || ptype[pnum] == PLANET_DEAD )
    if ( ! godlike )
      if ( pscanned[pnum][steam[snum]] && spwar( snum, pnum ) )
	appstr( " (hostile)", junk );
  
  /* Things that orbit things that orbit have phases. */
  switch ( phoon( pnum ) )
    {
    case PHOON_FIRST:
      appstr( " (first quarter)", junk );
      break;
    case PHOON_FULL:
      appstr( " (full)", junk );
      break;
    case PHOON_LAST:
      appstr( " (last quarter)", junk );
      break;
    case PHOON_NEW:
      appstr( " (new)", junk );
      break;
    case PHOON_NO:
      /* Do no-thing. */;
      break;
    default:
      appstr( " (weird)", junk );
      break;
    }
  
  if ( godlike )
    {
      x = 0.0;
      y = 0.0;
    }
  else
    {
      x = sx[snum];
      y = sy[snum];
    }
  
  if (sysconf_DoETAStats)
    {
      static char tmpstr[64];
      
      if (swarp[snum] > 0.0)
	{
	  sprintf(tmpstr, ", ETA %s",
		  ETAstr(swarp[snum], 	
			 round( dist( x, y, px[pnum], py[pnum])) ));
	}
      else
	tmpstr[0] = '\0';
      
      sprintf( buf, "%s%s, a %s%s, range %d, direction %d%s",
	     str,
	     pname[pnum],
	     ptname[ptype[pnum]],
	     junk,
	     round( dist( x, y, px[pnum], py[pnum] ) ),
	     round( angle( x, y, px[pnum], py[pnum] ) ),
	     tmpstr);
    }
  else
    sprintf( buf, "%s%s, a %s%s, range %d, direction %d",
	   str,
	   pname[pnum],
	   ptname[ptype[pnum]],
	   junk,
	   round( dist( x, y, px[pnum], py[pnum] ) ),
	   round( angle( x, y, px[pnum], py[pnum] ) ));
  
  if ( godlike )
    canscan = TRUE;
  else
    canscan = pscanned[pnum][steam[snum]];
  
  junk[0] = EOS;
  if ( ptype[pnum] != PLANET_SUN && ptype[pnum] != PLANET_MOON )
    {
      if ( ! canscan )
	c_strcpy( "with unknown occupational forces", junk );
      else
	{
	  i = parmies[pnum];
	  if ( i == 0 )
	    {
	      j = puninhabtime[pnum];
	      if ( j > 0 )
		sprintf( junk, "uninhabitable for %d more minutes", j );
	      else
		c_strcpy( "with NO armies", junk );
	    }
	  else
	    {
	      sprintf( junk, "with %d %s arm", i, tname[pteam[pnum]] );
	      if ( i == 1 )
		appstr( "y", junk );
	      else
		appstr( "ies", junk );
	    }
	}
      
      /* Now see if we can tell about coup time. */
      if ( godlike )
	canscan = FALSE;			/* GOD can use teaminfo instead */
      else
	canscan = ( pnum == homeplanet[steam[snum]] &&
		   tcoupinfo[steam[snum]] );
      if ( canscan )
	{
	  j = couptime[steam[snum]];
	  if ( j > 0 )
	    {
	      if ( junk[0] != EOS )
		appstr( ", ", junk );
	      appint( j, junk );
	      appstr( " minutes until coup time", junk );
	    }
	}
    }
  
  if ( junk[0] == EOS )
    {
      appchr( '.', buf );
    }
  else
    {
      appchr( ',', buf );
      appchr( '.', junk );
    }
  
  /* Now output the info. Break the stuff in buf across two lines */
  /*  (if necessary) and force the stuff in junk (the number of */
  /*  armies for planets) to be all on the second line. */
  i = strlen( buf );				/* strlen of first part */
  j = 69;					/* desired maximum length */
  if ( i <= j )
    {
      /* The first part is small enough. */
      c_putmsg( buf, MSG_LIN1 );
      if ( junk[0] != EOS )
	c_putmsg( junk, MSG_LIN2 );
      else
	cdclrl( MSG_LIN2, 1 );
    }
  else
    {
      /* Break it into two lines. */
      i = j + 1;
      while ( buf[i] != ' ' && i > 1 )
	i = i - 1;
      appchr( ' ', buf );
      appstr( junk, buf );
      buf[i] = EOS;				/* terminate at blank */
      c_putmsg( buf, MSG_LIN1 );
      c_putmsg( &buf[i+1], MSG_LIN2 );
    }
  
  cdmove( MSG_LIN1, 1 );
  return;
  
}

/* ETAstr - return a string indicating ETA to a target */
char *ETAstr(real warp, real distance)
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

/*  infoship - write out information about a ship */
/*  SYNOPSIS */
/*    int snum, scanner */
/*    infoship( snum, scanner ) */
void infoship( int snum, int scanner )
{
  int i, status;
  char junk[MSGMAXLINE];
  real x, y, dis, kills, appx, appy;
  int godlike, canscan;
  
  godlike = ( scanner < 1 || scanner > MAXSHIPS );
  
  cdclrl( MSG_LIN2, 1 );
  if ( snum < 1 || snum > MAXSHIPS )
    {
      c_putmsg( "No such ship.", MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
      return;
    }
  status = sstatus[snum];
  if ( ! godlike && status != SS_LIVE )
    {
      c_putmsg( "Not found.", MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
      return;
    }
  cbuf[0] = EOS;
  appship( snum, cbuf );
  if ( snum == scanner )
    {
      /* Silly Captain... */
      appstr( ": That's us, silly!", cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
      return;
    }
  /* Scan another ship. */
  if ( godlike )
    {
      x = 0.0;
      y = 0.0;
    }
  else
    {
      x = sx[scanner];
      y = sy[scanner];
    }
  if ( scloaked[snum] )
    {
      appx = rndnor(sx[snum], CLOAK_SMEAR_DIST);
      appy = rndnor(sy[snum], CLOAK_SMEAR_DIST);
    }
  else
    {
      appx = sx[snum];
      appy = sy[snum];
    }
  dis = dist( x, y, appx, appy );
  if ( godlike )
    canscan = TRUE;
  else
    {
      /* Help out the driver with this scan. */
      if ( (dis < ACCINFO_DIST && ! scloaked[snum]) && ! selfwar(scanner) )
	sscanned[snum][ steam[scanner]] = SCANNED_FUSE;
      
      /* Decide if we can do an acurate scan. */
      canscan = ( (dis < ACCINFO_DIST && ! scloaked[snum]) ||
		 ( (sscanned[snum][ steam[scanner]] > 0) && ! selfwar(scanner) ) );
    }
  
  appstr( ": ", cbuf );
  if ( spname[snum][0] != EOS )
    {
      appstr( spname[snum], cbuf );
      appstr( ", ", cbuf );
    }
  kills = (skills[snum] + sstrkills[snum]);
  if ( kills == 0.0 )
    appstr( "no", cbuf );
  else
    {
      sprintf( junk, "%.1f", kills );
      appstr( junk, cbuf );
    }
  appstr( " kill", cbuf );
  if ( kills != 1.0 )
    appchr( 's', cbuf );
  if ( dis < ACCINFO_DIST && scloaked[snum] )
    appstr( " (CLOAKED) ", cbuf );
  else
    appstr( ", ", cbuf );
  if ( godlike )
    {
      appsstatus( status, cbuf );
      appchr( '.', cbuf );
    }
  else if ( swar[snum][steam[scanner]] )
    appstr( "at WAR.", cbuf );
  else
    appstr( "at peace.", cbuf );
  
  c_putmsg( cbuf, MSG_LIN1 );
  
  if ( ! scloaked[snum] || swarp[snum] > 0.0 )
    {
      sprintf( cbuf, "Range %d, direction %d",
	     round( dis ), round( angle( x, y, appx, appy ) ) );
      
      if (sysconf_DoETAStats)
	{
	  if (swarp[scanner] > 0.0)
	    {
	      static char tmpstr[64];
	      
	      sprintf(tmpstr, ", ETA %s",
		      ETAstr(swarp[scanner], dis));
	      appstr(tmpstr, cbuf);
	    }
	}
    }
  else
    cbuf[0] = EOS;
  
  if ( canscan )
    {
      if ( cbuf[0] != EOS )
	appstr( ", ", cbuf );
      appstr( "shields ", cbuf );
      if ( sshup[snum] && ! srmode[snum] )
	appint( round( sshields[snum] ), cbuf );
      else
	appstr( "DOWN", cbuf );
      i = round( sdamage[snum] );
      if ( i > 0 )
	{
	  if ( cbuf[0] != EOS )
	    appstr( ", ", cbuf );
	  sprintf( junk, "damage %d", i );
	  appstr( junk, cbuf );
	}
      i = sarmies[snum];
      if ( i > 0 )
	{
	  sprintf( junk, ", with %d arm", i );
	  appstr( junk, cbuf );
	  if ( i == 1 )
	    {
	      appchr( 'y', cbuf );
	    }
	  else
	    appstr( "ies", cbuf );
	}
    }
  if ( cbuf[0] != EOS )
    {
      cbuf[0] = (char)toupper( cbuf[0] );
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN2 );
    }
  
  cdmove( MSG_LIN1, 1 );
  return;
  
}


/*  kill - kill a ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, kb */
/*    kill( snum, kb ) */
void killship( int snum, int kb )
{
  int sendmsg = FALSE;
  char msgbuf[128];

#if defined(DO_EXPLODING_SHIPS)
  /* launch all torps - sorta, we'll use 'explode' mode... */
  launch(snum, 0.0, 6, LAUNCH_EXPLODE);
#endif
  
				/* internal routine. */
  PVLOCK(lockword);
  ikill( snum, kb );
  PVUNLOCK(lockword);

				/* send a msg to all... */
  sendmsg = FALSE;

  /* Figure out why we died. */
  switch ( kb )
    {
    case KB_SELF:
      sprintf(msgbuf, "%c%d (%s) has self-destructed.",
	      chrteams[steam[snum]],
	      snum,
	      spname[snum]);
      sendmsg = TRUE;
      
      break;
    case KB_NEGENB:
      sprintf(msgbuf, "%c%d (%s) was destroyed by the negative energy barrier.",
	      chrteams[steam[snum]],
	      snum,
	      spname[snum]);
      sendmsg = TRUE;
      
      break;
      
    case KB_GOD:
      sprintf(msgbuf, "%c%d (%s) was killed by an act of GOD.",
	      chrteams[steam[snum]],
	      snum,
	      spname[snum]);
      sendmsg = TRUE;
      
      break;
    case KB_DOOMSDAY:
      sprintf(msgbuf, "%c%d (%s) was eaten by the doomsday machine.",
	      chrteams[steam[snum]],
	      snum,
	      spname[snum]);
      sendmsg = TRUE;
      
      break;
    case KB_DEATHSTAR:
      sprintf(msgbuf, "%c%d (%s) was vaporized by the Death Star.",
	      chrteams[steam[snum]],
	      snum,
	      spname[snum]);
      sendmsg = TRUE;

      break;
    case KB_LIGHTNING:
      sprintf(msgbuf, "%c%d (%s) was destroyed by lightning bolt.",
	      chrteams[steam[snum]],
	      snum,
	      spname[snum]);
      sendmsg = TRUE;

      break;
    default:
      
      if ( kb > 0 && kb <= MAXSHIPS )
	{
	  sprintf(msgbuf, "%c%d (%s) was kill %.1f for %c%d (%s).",
		  chrteams[steam[snum]],
		  snum,
		  spname[snum],
		  skills[kb],
		  chrteams[steam[kb]],
		  kb,
		  spname[kb]);
	  sendmsg = TRUE;

	}
      else if ( -kb > 0 && -kb <= NUMPLANETS )
	{
	  sprintf(msgbuf, "%c%d (%s) was destroyed by %s",
		  chrteams[steam[snum]],
		  snum,
		  spname[snum],
		  pname[-kb]);

	  sendmsg = TRUE;
	  
	  if ( ptype[-kb] == PLANET_SUN )
	    {
	      appstr( "'s solar radiation.", msgbuf );
	    }
	  else
	    {
	      appstr( "'s planetary defenses.", msgbuf );
	    }
	}
    }

  if (sendmsg == TRUE)
    stormsg(MSG_COMP, MSG_ALL, msgbuf);

  return;
  
}


/*  launch - create new torpedo(s) for a ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    flag = launch( snum, dir, number_of_torps, launch_type ) */
int launch( int snum, real dir, int number, int ltype )
{
  register int i, j;
  real speed, adir; 
  int tnum, numslots, numfired;
  static int tslot[MAXTORPS];
  
  /* Stop repairing. */
  srmode[snum] = FALSE;
  
  /* Remember this important direction. */
  slastblast[snum] = dir;
  
  /* Set up last fired phaser direction. */
  slastphase[snum] = dir;

  numslots = 0;
  numfired = 0;
  tnum = number;
  
  /* Find free torp(s). */
  PVLOCK(lockword);
  for ( i = 0; i < MAXTORPS && tnum != 0; i++ )
    if ( tstatus[snum][i] == TS_OFF )
      {
	/* Found one. */
	tstatus[snum][i] = TS_LAUNCHING;
	tslot[numslots++] = i;
	tnum--;
      }
  PVUNLOCK(lockword);
  
  if (numslots == 0)
    {				/* couldn't find even one */
      return(FALSE);
    }
  
  for (i=0; i<numslots; i++)
    {
      /* Use fuel. */
      if ( usefuel( snum, TORPEDO_FUEL, TRUE ) == FALSE)
	{
	  tstatus[snum][tslot[i]] = TS_OFF;
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
	  tfuse[snum][tslot[i]] = 1; /* shouldn't last long */
				/* should be close to the ship */
	  tx[snum][tslot[i]] = rndnor( sx[snum], 15.0 );
	  ty[snum][tslot[i]] = rndnor( sy[snum], 15.0 );
				/* no movement */
	  speed = 0.0;
				/* no direction or deltas */
	  adir = 0.0;
	  tdx[snum][tslot[i]] = 0.0;
	  tdy[snum][tslot[i]] = 0.0;

				/* strength of explosion depends on
				   the average of engine and weap
				   efficiency.  This prevents one side
				   from having an explosive adv over
				   another, while allowing greater
				   kills to matter.  */
	  tmult[snum][tslot[i]] = (( (real)engeff(snum) + 
				     (real)weaeff(snum) ) / 2.0);
	}
      else
	{
	  tfuse[snum][tslot[i]] = TORPEDO_FUSE;
	  tx[snum][tslot[i]] = rndnor( sx[snum], 100.0 );
	  ty[snum][tslot[i]] = rndnor( sy[snum], 100.0 );
	  speed = torpwarp[steam[snum]] * MM_PER_SEC_PER_WARP * ITER_SECONDS;
	  adir = rndnor( dir, 2.0 );
	  tdx[snum][tslot[i]] = (real) (speed * cosd(adir));
	  tdy[snum][tslot[i]] = (real)(speed * sind(adir));
	  tmult[snum][tslot[i]] = (real)weaeff( snum );
	}

      for ( j = 0; j < NUMTEAMS; j = j + 1 )
	{
	  if (ltype == LAUNCH_EXPLODE)
	    {			/* we're at war with everything except
				   our own team */
	      if (j == steam[snum])
		twar[snum][tslot[i]][j] = FALSE;
	      else
		twar[snum][tslot[i]][j] = TRUE; 
	    }
	  else
	    twar[snum][tslot[i]][j] = swar[snum][j]; /* just enemies */
	}
	  
      tstatus[snum][tslot[i]] = TS_LIVE;
    } 
  
  if (numfired == 0)
    {				/* couldn't fire any. bummer dude. */
      return(FALSE);
    }
  else
    {				/* torps away! */
      /* Update stats. */
      PVLOCK(lockword);
      ustats[suser[snum]][USTAT_TORPS] =
	ustats[suser[snum]][USTAT_TORPS] + numfired;
      tstats[steam[snum]][TSTAT_TORPS] =
	tstats[steam[snum]][TSTAT_TORPS] + numfired;
      PVUNLOCK(lockword);
      
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
/*    orbit( snum, pnum ) */
void orbit( int snum, int pnum )
{
  real beer; 
  
  slock[snum] = -pnum;
  sdwarp[snum] = 0.0;
  
  /* Find bearing to planet. */
  beer = angle( sx[snum], sy[snum], px[pnum], py[pnum] );
  if ( shead[snum] < ( beer - 180.0 ) )
    beer = beer - 360.0;
  
  /* Check beer head to determine orbit direction. */
  if ( beer <= shead[snum] )
    {
      swarp[snum] = ORBIT_CW;
      shead[snum] = mod360( beer + 90.0 );
    }
  else
    {
      swarp[snum] = ORBIT_CCW;
      shead[snum] = mod360( beer - 90.0 );
    }
  
  return;
  
}


/*  phaser - fire phasers (bug fry!!) (DOES LOCKING) */
/*  SYNOPSIS */
/*    int didit, phaser */
/*    int snum */
/*    real dir */
/*    didit = phaser( snum, dir ) */
int phaser( int snum, real dir )
{
  int k;
  real dis, ang;
  
  /* Set up last weapon direction. */
  slastblast[snum] = dir;
  
  /* Stop repairing. */
  srmode[snum] = FALSE;
  
  /* See if ok to fire. */
  if ( spfuse[snum] > 0 )
    return ( FALSE );
  
  /* Try to use fuel for this shot. */
  if ( ! usefuel( snum, PHASER_FUEL, TRUE ) )
    return ( FALSE );
  
  /* Update stats. */
  PVLOCK(lockword);
  ustats[suser[snum]][USTAT_PHASERS] = ustats[suser[snum]][USTAT_PHASERS] + 1;
  tstats[steam[snum]][TSTAT_PHASERS] = tstats[steam[snum]][TSTAT_PHASERS] + 1;
  PVUNLOCK(lockword);
  
  /* Set up last fired direction. */
  slastphase[snum] = dir;
  
  /* Start phaser fuse. */
  spfuse[snum] = PHASER_TENTHS;
  
  /* See what we can hit. */
  for ( k = 1; k <= MAXSHIPS; k = k + 1 )
    if ( sstatus[k] == SS_LIVE && k != snum )
      if ( satwar(snum, k ) )
	{
	  dis = dist( sx[snum], sy[snum], sx[k], sy[k] );
	  if ( dis <= PHASER_DIST )
	    {
	      ang = angle( sx[snum], sy[snum], sx[k], sy[k] );
	      if ( ang > 315.0 )
		ang = ang - 360.0;
	      if ( fabs( dir - ang ) <= PHASER_SPREAD )
		{
		  hit( k, phaserhit( snum, dis ), snum );
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
/*    hit = phaserhit( snum, dis ) */
real phaserhit( int snum, real dis )
{
  return (( - dis / PHASER_DIST + 1.0 ) * PHASER_HIT * weaeff( snum ));
  
}


/*  planlist - list planets */
/*  SYNOPSIS */
/*    int team */
/*    planlist( team ) */
void planlist( int team, int snum )
{
  int i, lin, col, olin, pnum;
  static int sv[NUMPLANETS + 1];
  int cmd;
  char ch, junk[10], coreflag;
  char *hd0="P L A N E T   L I S T   ";
  char *hd1="' = must take to conquer the Universe)";
  string hd2="planet      type team armies          planet      type team armies";
  char hd3[BUFFER_SIZE];
  int outattr;
  int col2;
  int column_h = 7;
  int column_1 = 5;
  int column_2 = 43;
  char xbuf[BUFFER_SIZE];
  char pd0[MID_BUFFER_SIZE];
  static int FirstTime = TRUE;
  int PlanetOffset;		/* offset into NUMPLANETS for this page */
  int PlanetIdx = 0;
  int Done;

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;

				/* build header fmt string */
      sprintf(pd0,
	      "#%d#%s#%d#%s#%d#%s#%d#%s" ,
	      LabelColor,
		  hd0,
	      InfoColor,
		  "('",
	      SpecialColor,
		  "+", 
	      InfoColor,
		  hd1);

				/* sort the planets */
      for ( i = 1; i <= NUMPLANETS; i++ )
	sv[i] = i;
      sortplanets( sv );
      
    }

  strcpy( hd3, hd2 );
  for ( i = 0; hd3[i] != EOS; i++ )
    if ( hd3[i] != ' ' )
      hd3[i] = '-';
  
  PlanetIdx = 0;

  PlanetOffset = 1;
  cdclear();
  Done = FALSE;
  do
    {

      cdclra(0, 0, MSG_LIN1 + 2, cdcols() - 1);
      PlanetIdx = 0;
      lin = 1;
      col = column_h;
      
      cprintf(lin, column_h, ALIGN_NONE, pd0);
      
      /* display column headings */
      lin += 2;
      attrset(LabelColor);
      cdputc( hd2, lin );
      lin++;
      cdputc( hd3, lin );
      attrset(0);
      lin++;
      olin = lin;
      col = column_1;
      col2 = FALSE;

      PlanetIdx = 0;
      
      if (PlanetOffset <= NUMPLANETS)
	{
	  while ((PlanetOffset + PlanetIdx) <= NUMPLANETS)
	    {
	      i = PlanetOffset + PlanetIdx;
	      PlanetIdx++;
	      pnum = sv[i];
	      
	      /* colorize - dwp */    
	      if ( snum > 0 && snum <= MAXSHIPS)
		{	/* if user has a valid ship */
		  if ( pteam[pnum] == steam[snum] && !selfwar(snum) )
		    outattr = GreenLevelColor;
		  else if ( (spwar(snum,pnum) && pscanned[pnum][steam[snum]] ) ||
			    ptype[pnum] == PLANET_SUN )
		    outattr = RedLevelColor;
		  else 
		    outattr = YellowLevelColor;
		}
	      else
		{			/* else, user doesn't have a ship yet */
		  if (team == TEAM_NOTEAM)
		    {			/* via conqoper */
		      switch(ptype[pnum])
			{
			case PLANET_SUN:
			  outattr = RedLevelColor;
			  break;
			case PLANET_CLASSM:
			  outattr = GreenLevelColor;
			  break;
			case PLANET_DEAD:
			  outattr = YellowLevelColor;
			  break;
			case PLANET_CLASSA:
			case PLANET_CLASSO:
			case PLANET_CLASSZ:
			  outattr = A_BOLD;
			  break;
			case PLANET_GHOST:
			  outattr = NoColor;
			  break;
			default:
			  outattr = SpecialColor;
			  break;
			}
		    }
		  else
		    {			/* via menu() */
		      if ( pteam[pnum] == uteam[cunum] && 
			   !(uwar[cunum][uteam[cunum]]))
			{
			  outattr = GreenLevelColor;
			}
		      else if ( ptype[pnum] == PLANET_SUN ||
				(pteam[pnum] < NUMTEAMS && 
				 uwar[cunum][pteam[pnum]] &&
				 pscanned[pnum][uteam[cunum]]) )
			{
			  outattr = RedLevelColor;
			}
		      else 
			{
			  outattr = YellowLevelColor;
			}
		    }
		}
	      
	      /* Don't display unless it's real. */
	      if ( ! preal[pnum] )
		continue; 
	      
	      /* I want everything if it's real */
	      
	      /* Figure out who owns it and count armies. */
	      ch =  chrteams[pteam[pnum]];
	      sprintf( junk, "%d", parmies[pnum] );
	      
	      /* Then modify based on scan information. */
	      
	      if ( team != TEAM_NOTEAM )
		if ( ! pscanned[pnum][team] )
		  {
		    ch = '?';
		    c_strcpy( "?", junk );
		  }
	      
	      /* Suns and moons are displayed as unowned. */
	      if ( ptype[pnum] == PLANET_SUN || ptype[pnum] == PLANET_MOON )
		ch = ' ';
	      
	      /* Don't display armies for suns unless we're special. */
	      if ( ptype[pnum] == PLANET_SUN )
		if ( team != TEAM_NOTEAM )
		  junk[0] = EOS;
	      
	      /* Moons aren't supposed to have armies. */
	      if ( ptype[pnum] == PLANET_MOON )
		if ( team != TEAM_NOTEAM )
		  junk[0] = EOS;
		else if ( parmies[pnum] == 0 )
		  junk[0] = EOS;
	      
	      coreflag = ' ';
	      
	      /* flag planets that are required for a conq */
	      if (ptype[pnum] == PLANET_CLASSM || ptype[pnum] == PLANET_DEAD)
		{
		  if (pnum > NUMCONPLANETS)
		    coreflag = ' ';
		  else
		    coreflag = '+';
		}
	      
	      sprintf(xbuf,"%c ",coreflag);  /* coreflag */
	      attrset(SpecialColor);
	      cdputs( xbuf, lin, col );
	      
	      col+=(strlen(xbuf));
	      sprintf(xbuf,"%-11s ",pname[pnum]);  /* pname[pnum] */
	      attrset(outattr);
	      cdputs( xbuf, lin, col );
	      
	      col+=(strlen(xbuf));
	      sprintf( xbuf, "%-4c %-3c  ", chrplanets[ptype[pnum]], ch);
	      cdputs( xbuf, lin, col );
	      
	      col+=(strlen(xbuf));
	      sprintf(xbuf,"%4s",junk);
	      if (junk[0] == '?')
		attrset(YellowLevelColor);
	      else
		attrset(InfoColor);
	      cdputs( xbuf, lin, col );
	      attrset(0); 

	      lin++;;
	      if ( lin == MSG_LIN1 )
		{
		  if (col2)	/* need a new page... */
		    {
		      break; /* out of while */
		    }
		  else
		    {
		      lin = olin;
		      col2 = TRUE;
		    }
		}

	      if (!col2)
		col = column_1;
	      else
		col = column_2;
	      
	    } /* while */


	  putpmt( "--- press [SPACE] to continue, q to quit ---", MSG_LIN2 );
	  cdrefresh();

	  if (iogtimed( &cmd, 1 ))
	    {			/* got a char */
	      if (cmd == 'q' || cmd == 'Q' || cmd == TERM_ABORT)
		{		/* quit */
		  Done = TRUE;
		}
	      else
		{		/* some other key... */
				/* setup for new page */
		  PlanetOffset += PlanetIdx;
		  if (PlanetOffset > NUMPLANETS)
		    {		/* pointless to continue */
		      Done = TRUE;
		    }

		}
	    }

				/* didn't get a char, update */
	  if (snum > 0 && snum <= MAXSHIPS)
	    if (!stillalive(snum))
	      Done = TRUE;

	} /* if PlanetOffset <= NUMPLANETS */
      else
	Done = TRUE;		/* else PlanetOffset > NUMPLANETS */
      
    } while(Done != TRUE); /* do */
  
  return;
  
}


/*  playlist - list ships */
/*  SYNOPSIS */
/*    int godlike, doall */
/*    playlist( godlike, doall ) */
void playlist( int godlike, int doall, int snum )
{
  int i, unum, status, kb, lin, col;
  int fline, lline, fship;
  char sbuf[20];
  char kbuf[20];
  char pidbuf[20];
  int ch;
  char *hd1="ship name          pseudonym              kills      pid";
  
  /* Do some screen setup. */
  cdclear();
  attrset(LabelColor);  /* dwp */
  c_strcpy( hd1, cbuf );

  col = (int)(cdcols() - strlen( cbuf )) / (int)2;
  lin = 2;
  cdputs( cbuf, lin, col );
  
  for ( i = 0; cbuf[i] != EOS; i = i + 1 )
    if ( cbuf[i] != ' ' )
      cbuf[i] = '-';
  lin = lin + 1;
  cdputs( cbuf, lin, col );
  attrset(0);          /* dwp */
  
  fline = lin + 1;				/* first line to use */
  lline = MSG_LIN1;				/* last line to use */
  fship = 1;					/* first user in uvec */
  
  while(TRUE) /* repeat- while */
    {
      if ( ! godlike )
	if ( ! stillalive( csnum ) )
	  break;
      i = fship;
      cdclrl( fline, lline - fline + 1 );
      lin = fline;
      while ( i <= MAXSHIPS && lin <= lline )
	{
	  status = sstatus[i];

	  kb = skilledby[i];
	  if ( status == SS_LIVE ||
	      ( doall && ( status != SS_OFF || kb != 0 ) ) )
	    {
	      sbuf[0] = EOS;
	      appship( i, sbuf );
	      unum = suser[i];
	      if ( unum >= 0 && unum < MAXUSERS )
		{
		  if (spid[i] == 0) /* robot */
		    strcpy(pidbuf, " ROBOT");
		  else if (CheckPid(spid[i]) == FALSE) 
		    strcpy(pidbuf, "VACANT");
		  else
		    sprintf(pidbuf, "%6d", spid[i]);

		  sprintf(kbuf, "%6.1f", (skills[i] + sstrkills[i]));
		  sprintf( cbuf, "%-4s %-13.13s %-21.21s %-8s %6s",
			   sbuf, cuname[unum], spname[i], 
			   kbuf, pidbuf );
		}
	      else
		sprintf( cbuf, "%-4s %13s %21s %8s %6s", sbuf,
		       " ", " ", " ", " " );
	      if ( doall && kb != 0 )
		{
		  appstr( "  ", cbuf);
		  appkb( kb, cbuf );
		}

		if (snum > 0 && snum <= MAXSHIPS )
		  {		/* a normal ship view */
		    if ( i == snum )    /* it's ours */
		      attrset(A_BOLD);
		    else if (satwar(i, snum)) /* we're at war with it */
		      attrset(RedLevelColor);
		    else if (steam[i] == steam[snum] && !selfwar(snum))
		      attrset(GreenLevelColor); /* it's a team ship */
		    else
		      attrset(YellowLevelColor);
		  }
		else if (godlike) /* conqoper */
		  {		
		    attrset(YellowLevelColor);
		  }
		else
		  { /* not conqoper, and not a valid ship (main menu) */
		    if (uwar[cunum][steam[i]])  /* we're at war with ships's
						   team */
		      attrset(RedLevelColor);
		    else if (uteam[cunum] == steam[i])
		      attrset(GreenLevelColor); /* it's a team ship */
		    else
		      attrset(YellowLevelColor);
		  }

	      cdputs( cbuf, lin, col );
	      attrset(0);
	      if ( doall && status != SS_LIVE )
		{
		  cbuf[0] = EOS;
		  appsstatus( status, cbuf );
		  /*		  appstr("(", cbuf);
		  appint(spid[i], cbuf);
		  appstr(")", cbuf);
		  */
		  
		  attrset(YellowLevelColor);  /* dwp */
		  cdputs( cbuf, lin, col - 2 - strlen( cbuf ) );
		  attrset(0);          /* dwp */
		}
	    }
	  i = i + 1;
	  lin = lin + 1;
	}
      if ( i > MAXSHIPS )
	{
	  /* We're displaying the last page. */
	  putpmt( "--- press space when done ---", MSG_LIN2 );
	  cdrefresh();
	  if ( iogtimed( &ch, 1 ) )
	    {
	      if ( ch == TERM_EXTRA )
		fship = 1;			/* move to first page */
	      else
		break;
	    }
	}
      else
	{
	  /* There are ships left to display. */
	  putpmt( "--- press space for more ---", MSG_LIN2 );
	  cdrefresh();
	  if ( iogtimed( &ch, 1 ) )
	    {
	      if ( ch == TERM_EXTRA )
		fship = 0;			/* move to first page */
	      else if ( ch == ' ' )
		fship = i;			/* move to next page */
	      else
		break;
	    }
	}
    }
  
  return;
  
}


/*  pseudo - change an user's pseudonym */
/*  SYNOPSIS */
/*    int unum, snum */
/*    pseudo( unum, snum ) */
void pseudo( int unum, int snum )
{
  char ch, buf[MSGMAXLINE];
  
  buf[0] = EOS;

  cdclrl( MSG_LIN1, 2 );
  c_strcpy( "Old pseudonym: ", buf );
  if ( snum > 0 && snum <= MAXSHIPS )
    appstr( spname[snum], buf );
  else
    appstr( upname[unum], buf );
  cdputc( buf, MSG_LIN1 );
  ch = getcx( "Enter a new pseudonym: ",
	     MSG_LIN2, -4, TERMS, buf, MAXUSERPNAME );
  if ( ch == TERM_ABORT || buf[0] == EOS)
    {
      cdclrl( MSG_LIN1, 2 );
      return;
    }

  stcpn( buf, upname[unum], MAXUSERPNAME );
  if ( snum > 0 && snum <= MAXSHIPS )
    stcpn( buf, spname[snum], MAXUSERPNAME );
  
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*  register - register a new user (DOES LOCKING) */
/*  SYNOPSIS */
/*    char lname(), rname() */
/*    int team, unum */
/*    int flag, register */
/*    flag = register( lname, rname, team, unum ) */
int c_register( char *lname, char *rname, int team, int *unum ) 
{
  int i, j;
  
  PVLOCK(lockword);
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    if ( ! ulive[i] )
      {
	ulive[i] = TRUE;
	PVUNLOCK(lockword);
	urating[i] = 0.0;
	uteam[i] = team;
	urobot[i] = FALSE;
	umultiple[i] = 2;			/* but the option bit is off */
	
	for ( j = 0; j < MAXUSTATS; j = j + 1 )
	  ustats[i][j] = 0;
	
	for ( j = 0; j < NUMTEAMS; j = j + 1 )
	  uwar[i][j] = TRUE;
	uwar[i][uteam[i]] = FALSE;
	
	for ( j = 0; j < MAXOPTIONS; j = j + 1 )
	  uoption[i][j] = TRUE;
/*	uoption[i][OPT_INTRUDERALERT] = FALSE; JET - this turns off ALL msgs
                                                     from planets. not a
						     good default... */
	uoption[i][OPT_NUMERICMAP] = FALSE;
	uoption[i][OPT_TERSE] = FALSE;
	
	for ( j = 0; j < MAXOOPTIONS; j = j + 1 )
	  uooption[i][j] = FALSE;
	
	uooption[i][OOPT_SWITCHTEAMS] = TRUE; /* allow users to switchteams when dead */

	stcpn( "never", ulastentry[i], DATESIZE );
	stcpn( lname, cuname[i], MAXUSERNAME );
	stcpn( rname, upname[i], MAXUSERPNAME );
	*unum = i;
	return ( TRUE );
      }
  
  PVUNLOCK(lockword);
  
  return ( FALSE );
  
}


/*  resign - remove a user from the user list (DOES LOCKING) */
/*  SYNOPSIS */
/*    int unum */
/*    resign( unum ) */
void resign( int unum )
{
  int i;
  
  PVLOCK(lockword);
  if ( unum >= 0 && unum < MAXUSERS )
    {
      ulive[unum] = FALSE;
      for ( i = 0; i < MAXHISTLOG; i = i + 1 )
	if ( unum == histunum[i] )
	  {
	    histunum[i] = -1;
	    histlog[i][0] = EOS;
	  }
    }
  PVUNLOCK(lockword);
  
  return;
  
}


/*  review - review old messages */
/*  SYNOPSIS */
/*    int flag, review */
/*    int snum, slm */
/*    flag = review( snum, slm ) */
int review( int snum, int slm )
{
  int ch, Done, i, msg, tmsg, lastone; 
  int didany;
  
  didany = FALSE;
  Done = FALSE;

  lastone = modp1( *lastmsg+1, MAXMESSAGES );
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      if ( slastmsg[snum] == LMSG_NEEDINIT )
	return ( FALSE );				/* none to read */
      i = salastmsg[snum];
      if ( i != LMSG_READALL )
	lastone = i;
    }
  
  cdclrl( MSG_LIN1, 1 );
  
  /*  for ( msg = slm; msg != lastone; msg = modp1( msg-1, MAXMESSAGES ) )*/

  msg = slm;

  do
    {
      if ( canread( snum, msg ))
	{
	  readmsg( snum, msg, MSG_LIN1 );
	  didany = TRUE;
	  putpmt( "--- [SPACE] for more, arrows to scroll, any key to quit ---", 
		  MSG_LIN2 );
	  cdrefresh();
	  ch = iogchar();
	  switch(ch)
	    {
	    case ' ':
	    case '<':
	    case KEY_UP:
	    case KEY_LEFT:
	      tmsg = modp1( msg - 1, MAXMESSAGES );
	      while(!canread( snum, tmsg ) && tmsg != lastone)
		{
		  tmsg = modp1( tmsg - 1, MAXMESSAGES );
		}
	      if (tmsg == lastone)
		{
		  cdbeep();
		}
	      else
		msg = tmsg;
	      
	      break;
	    case '>':
	    case KEY_DOWN:
	    case KEY_RIGHT:
	      tmsg =  modp1( msg + 1, MAXMESSAGES );
	      while(!canread( snum, tmsg ) && tmsg != slm + 1 )
		{
		  tmsg = modp1( tmsg + 1, MAXMESSAGES );
		}
	      if (tmsg == (slm + 1))
		{
		  cdbeep();
		}
	      else
		msg = tmsg;
	      
	      break;
	    default:
	      Done = TRUE;
	      break;
	    }
	}
      else
	{
	  msg = modp1( msg - 1, MAXMESSAGES );
	  if (msg == lastone)
	    Done = TRUE;
	}

    } while (Done == FALSE);

  cdclrl( MSG_LIN1, 2 );
  
  return ( didany );
  
}


/*  takeplanet - take a planet (DOES SPECIAL LOCKING) */
/*  SYNOPSIS */
/*    int pnum, snum */
/*    takeplanet( pnum, snum ) */
/*  Note: This routines ASSUMES you have the common locked before you it. */
void takeplanet( int pnum, int snum )
{
  int i;
  char buf[MSGMAXLINE];
  
  pteam[pnum] = steam[snum];
  parmies[pnum] = 1;
  skills[snum] = skills[snum] + PLANET_KILLS;
  ustats[suser[snum]][USTAT_CONQPLANETS] =
    ustats[suser[snum]][USTAT_CONQPLANETS] + 1;
  tstats[steam[snum]][TSTAT_CONQPLANETS] =
    tstats[steam[snum]][TSTAT_CONQPLANETS] + 1;
  sprintf( buf, "All hail the liberating %s armies.  Thanks, ",
	 tname[steam[snum]] );
  appship( snum, buf );
  appchr( '!', buf );
  
  /* Check whether the universe has been conquered. */
  for ( i = 0; i < NUMCONPLANETS; i = i + 1 )
    if ( ptype[i] == PLANET_CLASSM || ptype[i] == PLANET_DEAD )
      if ( pteam[i] != steam[snum] || ! preal[i] )
	{
	  /* No. */
	  stormsg( -pnum, -steam[snum], buf );
	  return;
	}
  /* Yes! */
  getdandt( conqtime );
  stcpn( spname[snum], conqueror, MAXUSERPNAME );
  lastwords[0] = EOS;
  ustats[suser[snum]][USTAT_CONQUERS] = ustats[suser[snum]][USTAT_CONQUERS] + 1;
  tstats[steam[snum]][TSTAT_CONQUERS] = tstats[steam[snum]][TSTAT_CONQUERS] + 1;
  stcpn( tname[steam[snum]], conqteam, MAXTEAMNAME );
  ikill( snum, KB_CONQUER );
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] == SS_LIVE )
      ikill( i, KB_NEWGAME );
  
  PVUNLOCK(lockword);
  initgame();
  PVLOCK(lockword);
  
  return;
  
}


/*  teamlist - list team statistics */
/*  SYNOPSIS */
/*    int team */
/*    teamlist( team ) */
void teamlist( int team )
{
  int i, j, lin, col, ctime, etime;
  int godlike;
  char buf[MSGMAXLINE], timbuf[5][DATESIZE];
  real x[5];
  string sfmt="%15s %11s %11s %11s %11s %11s";
  char *stats="Statistics since: ";
  char *last_conquered="Universe last conquered at: ";

  char tmpfmt[MSGMAXLINE * 2];
  static char sfmt2[MSGMAXLINE * 2];
  static char sfmt3[MSGMAXLINE * 2];
  static char dfmt2[MSGMAXLINE * 2];
  static char pfmt2[MSGMAXLINE * 2];
  static int FirstTime = TRUE;	/* Only necc if the colors aren't 
				   going to change at runtime */

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt2,
	      "#%d#%%16s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s",
	      LabelColor,
	      GreenLevelColor,
	      YellowLevelColor,
	      RedLevelColor,
	      SpecialColor,
	      InfoColor);

	  /* FIXME - dwp for Time, Cpu time, Coup time */
      sprintf(sfmt3,
	      "#%d#%%15s #%d#%%12s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s",
	      LabelColor,
	      GreenLevelColor,
	      YellowLevelColor,
	      RedLevelColor,
	      SpecialColor,
	      InfoColor);

      sprintf(dfmt2,
	      "#%d#%%15s #%d#%%12d #%d#%%11d #%d#%%11d #%d#%%11d #%d#%%11d",
	      LabelColor,
	      GreenLevelColor,
	      YellowLevelColor,
	      RedLevelColor,
	      SpecialColor,
	      InfoColor);

      sprintf(pfmt2,
		  "#%d#%%15s #%d#%%11.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%%",
	      LabelColor,
	      GreenLevelColor,
	      YellowLevelColor,
	      RedLevelColor,
	      SpecialColor,
	      InfoColor);

  } /* end FIRST_TIME */

  godlike = ( team < 0 || team >= NUMTEAMS );
  col = 0; /*1*/
  
  lin = 1;
  /* team stats and last date conquered */
  sprintf(tmpfmt,"#%d#%%s#%d#%%s",LabelColor,InfoColor);
  cprintf(lin,0,ALIGN_CENTER,tmpfmt,stats,inittime);
  lin++;

  /* last conquered */
  cprintf(lin,0,ALIGN_CENTER,tmpfmt,last_conquered,conqtime);
  lin++;

  /* last conqueror and conqteam */
  sprintf(tmpfmt,"#%d#by #%d#%%s #%d#for the #%d#%%s #%d#team",
		LabelColor,(int)A_BOLD,LabelColor,(int)A_BOLD,LabelColor);
  cprintf(lin,0,ALIGN_CENTER,tmpfmt,conqueror,conqteam);
  
  col=0;  /* put col back to 0 for rest of display */
  lin = lin + 1;
  cdclrl( lin, 1 );
  if ( lastwords[0] != EOS )
    {
	  sprintf(tmpfmt,"#%d#%%c%%s%%c",YellowLevelColor);
      cprintf(lin,0,ALIGN_CENTER, tmpfmt, '"', lastwords, '"' );
    }
  
  lin+=2;
  sprintf( buf, sfmt, " ",
	 tname[0], tname[1], tname[2], tname[3], "Totals" );
  cprintf(lin,col,0, sfmt2, " ",
	 tname[0], tname[1], tname[2], tname[3], "Totals" );
  
  lin++;
  for ( i = 0; buf[i] != EOS; i++ )
    if ( buf[i] != ' ' )
      buf[i] = '-';
  attrset(LabelColor);
  cdputs( buf, lin, col );
  attrset(0);
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Conquers",
	 tstats[0][TSTAT_CONQUERS], tstats[1][TSTAT_CONQUERS],
	 tstats[2][TSTAT_CONQUERS], tstats[3][TSTAT_CONQUERS],
	 tstats[0][TSTAT_CONQUERS] + tstats[1][TSTAT_CONQUERS] +
	 tstats[2][TSTAT_CONQUERS] + tstats[3][TSTAT_CONQUERS] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Wins",
	 tstats[0][TSTAT_WINS], tstats[1][TSTAT_WINS],
	 tstats[2][TSTAT_WINS], tstats[3][TSTAT_WINS],
	 tstats[0][TSTAT_WINS] + tstats[1][TSTAT_WINS] +
	 tstats[2][TSTAT_WINS] + tstats[3][TSTAT_WINS] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Losses",
	 tstats[0][TSTAT_LOSSES], tstats[1][TSTAT_LOSSES],
	 tstats[2][TSTAT_LOSSES], tstats[3][TSTAT_LOSSES],
	 tstats[0][TSTAT_LOSSES] + tstats[1][TSTAT_LOSSES] +
	 tstats[2][TSTAT_LOSSES] + tstats[3][TSTAT_LOSSES] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Ships",
	 tstats[0][TSTAT_ENTRIES], tstats[1][TSTAT_ENTRIES],
	 tstats[2][TSTAT_ENTRIES], tstats[3][TSTAT_ENTRIES],
	 tstats[0][TSTAT_ENTRIES] + tstats[1][TSTAT_ENTRIES] +
	 tstats[2][TSTAT_ENTRIES] + tstats[3][TSTAT_ENTRIES] );
  
  lin++;
  etime = tstats[0][TSTAT_SECONDS] + tstats[1][TSTAT_SECONDS] +
    tstats[2][TSTAT_SECONDS] + tstats[3][TSTAT_SECONDS];
  fmtseconds( tstats[0][TSTAT_SECONDS], timbuf[0] );
  fmtseconds( tstats[1][TSTAT_SECONDS], timbuf[1] );
  fmtseconds( tstats[2][TSTAT_SECONDS], timbuf[2] );
  fmtseconds( tstats[3][TSTAT_SECONDS], timbuf[3] );
  fmtseconds( etime, timbuf[4] );
  cprintf(lin,col,0, sfmt3, "Time",
	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  
  lin++;
  ctime = tstats[0][TSTAT_CPUSECONDS] + tstats[1][TSTAT_CPUSECONDS] +
    tstats[2][TSTAT_CPUSECONDS] + tstats[3][TSTAT_CPUSECONDS];
  fmtseconds( tstats[0][TSTAT_CPUSECONDS], timbuf[0] );
  fmtseconds( tstats[1][TSTAT_CPUSECONDS], timbuf[1] );
  fmtseconds( tstats[2][TSTAT_CPUSECONDS], timbuf[2] );
  fmtseconds( tstats[3][TSTAT_CPUSECONDS], timbuf[3] );
  fmtseconds( ctime, timbuf[4] );
  cprintf( lin,col,0, sfmt3, "Cpu time",
	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  
  lin++;
  for ( i = 0; i < 4; i++ )
    {
      j = tstats[i][TSTAT_SECONDS];
      if ( j <= 0 )
	x[i] = 0.0;
      else
	x[i] = 100.0 * ((real) tstats[i][TSTAT_CPUSECONDS] / (real) j);
    }
  if ( etime <= 0 )
    x[4] = 0.0;
  else
    x[4] = 100.0 * (real) ctime / (real)etime;
  cprintf( lin,col,0, pfmt2, "Cpu usage", x[0], x[1], x[2], x[3], x[4] );

  lin++;
  cprintf( lin,col,0, dfmt2, "Phaser shots",
	 tstats[0][TSTAT_PHASERS], tstats[1][TSTAT_PHASERS],
	 tstats[2][TSTAT_PHASERS], tstats[3][TSTAT_PHASERS],
	 tstats[0][TSTAT_PHASERS] + tstats[1][TSTAT_PHASERS] +
	 tstats[2][TSTAT_PHASERS] + tstats[3][TSTAT_PHASERS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Torps fired",
	 tstats[0][TSTAT_TORPS], tstats[1][TSTAT_TORPS],
	 tstats[2][TSTAT_TORPS], tstats[3][TSTAT_TORPS],
	 tstats[0][TSTAT_TORPS] + tstats[1][TSTAT_TORPS] +
	 tstats[2][TSTAT_TORPS] + tstats[3][TSTAT_TORPS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Armies bombed",
	 tstats[0][TSTAT_ARMBOMB], tstats[1][TSTAT_ARMBOMB],
	 tstats[2][TSTAT_ARMBOMB], tstats[3][TSTAT_ARMBOMB],
	 tstats[0][TSTAT_ARMBOMB] + tstats[1][TSTAT_ARMBOMB] +
	 tstats[2][TSTAT_ARMBOMB] + tstats[3][TSTAT_ARMBOMB] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Armies captured",
	 tstats[0][TSTAT_ARMSHIP], tstats[1][TSTAT_ARMSHIP],
	 tstats[2][TSTAT_ARMSHIP], tstats[3][TSTAT_ARMSHIP],
	 tstats[0][TSTAT_ARMSHIP] + tstats[1][TSTAT_ARMSHIP] +
	 tstats[2][TSTAT_ARMSHIP] + tstats[3][TSTAT_ARMSHIP] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Planets taken",
	 tstats[0][TSTAT_CONQPLANETS], tstats[1][TSTAT_CONQPLANETS],
	 tstats[2][TSTAT_CONQPLANETS], tstats[3][TSTAT_CONQPLANETS],
	 tstats[0][TSTAT_CONQPLANETS] + tstats[1][TSTAT_CONQPLANETS] +
	 tstats[2][TSTAT_CONQPLANETS] + tstats[3][TSTAT_CONQPLANETS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Coups",
	 tstats[0][TSTAT_COUPS], tstats[1][TSTAT_COUPS],
	 tstats[2][TSTAT_COUPS], tstats[3][TSTAT_COUPS],
	 tstats[0][TSTAT_COUPS] + tstats[1][TSTAT_COUPS] +
	 tstats[2][TSTAT_COUPS] + tstats[3][TSTAT_COUPS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Genocides",
	 tstats[0][TSTAT_GENOCIDE], tstats[1][TSTAT_GENOCIDE],
	 tstats[2][TSTAT_GENOCIDE], tstats[3][TSTAT_GENOCIDE],
	 tstats[0][TSTAT_GENOCIDE] + tstats[1][TSTAT_GENOCIDE] +
	 tstats[2][TSTAT_GENOCIDE] + tstats[3][TSTAT_GENOCIDE] );
  
  for ( i = 0; i < 4; i++ )
    if ( couptime[i] == 0 )
      timbuf[i][0] = EOS;
    else
      sprintf( timbuf[i], "%d", couptime[i] );
  
  if ( ! godlike )
    for ( i = 0; i < 4; i++ )
      if ( team != i )
	c_strcpy( "-", timbuf[i] );
      else if ( ! tcoupinfo[i] && timbuf[i][0] != EOS )
	c_strcpy( "?", timbuf[i] );
  
  timbuf[4][0] = EOS;
  
  lin++;
  cprintf( lin,col,0, sfmt3, "Coup time",
 	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

  attrset(0);
  
  return;
  
}


/*  userline - format user statistics */
/*  SYNOPSIS */
/*    int unum, snum */
/*    char buf() */
/*    int showgods, showteam */
/*    userline( unum, snum, buf, showgods, showteam ) */
/* Special hack: If snum is valid, the team and pseudonym are taken from */
/* the ship instead of the user. */
void userline( int unum, int snum, char *buf, int showgods, int showteam )
{
  int i, team;
  char ch, ch2, junk[MSGMAXLINE], timstr[20], name[MAXUSERPNAME];
  
  char *hd1="name          pseudonym           team skill  wins  loss mxkls  ships     time";
  
  
  if ( unum < 0 || unum >= MAXUSERS )
    {
      c_strcpy( hd1, buf );
      return;
    }
  if ( ! ulive[unum] )
    {
      buf[0] = EOS;
      return;
    }
  
  ch2 = ' ';
  if ( showgods )
    {
      for ( i = 2; i < MAXOOPTIONS; i++)
	if ( uooption[unum][i] )
	  {
	    ch2 = '+';
	    break;
	  }
      if ( ch2 != '+' )
	if ( isagod(cuname[unum]) )
	  ch2 = '+';
    }
  
  /* If we were given a valid ship number, use it's information. */
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      c_strcpy( spname[snum], name );
      team = steam[snum];
    }
  else
    {
      c_strcpy( upname[unum], name );
      team = uteam[unum];
    }
  
  /* Figure out which team he's on. */
  if ( uooption[unum][OOPT_MULTIPLE] && ! showteam )
    ch = 'M';
  else
    ch = chrteams[team];
  
  sprintf( junk, "%-12s %c%-21.21s %c %6.1f",
	 cuname[unum],
	 ch2,
	 name,
	 ch,
	 urating[unum] );
  
  fmtminutes( ( ustats[unum][USTAT_SECONDS] + 30 ) / 60, timstr );
  
  sprintf( buf, "%s %5d %5d %5d %5d %9s",
	 junk,
	 ustats[unum][USTAT_WINS],
	 ustats[unum][USTAT_LOSSES],
	 ustats[unum][USTAT_MAXKILLS],
	 ustats[unum][USTAT_ENTRIES],
	 timstr );
  
  return;
  
}


/*  userlist - display the user list */
/*  SYNOPSIS */
/*    userlist( godlike ) */
void userlist( int godlike, int snum )
{
  int i, j, unum, nu, fuser, fline, lline, lin;
  static int uvec[MAXUSERS];
  int ch;
  char *hd1="U S E R   L I S T";

  /* Sort user numbers into uvec() in an insertion sort on urating(). */
  
  for (i=0; i<MAXUSERS; i++)
    uvec[i] = i;
  
  nu = 0;
  
				/* sort the user list */
  for ( unum = 0; unum < MAXUSERS; unum++)
    if ( ulive[unum])
      {
	for ( i = 0; i < nu; i++ )
	  if ( urating[uvec[i]] < urating[unum] )
	    {
	      for ( j = nu - 1; j >= i; j = j - 1 )
		uvec[j+1] = uvec[j];
	      break;
	    }
	uvec[i] = unum;
	nu++;
      }

  /* Do some screen setup. */
  cdclear();
  lin = 0;
  attrset(LabelColor);  /* dwp */
  cdputc( hd1, lin );
  
  lin = lin + 3;        /* FIXME - hardcoded??? - dwp */
  userline( -1, -1, cbuf, FALSE, FALSE );
  cdputs( cbuf, lin, 1 );
  
  for ( j = 0; cbuf[j] != EOS; j = j + 1 )
    if ( cbuf[j] != ' ' )
      cbuf[j] = '-';
  lin = lin + 1;
  cdputs( cbuf, lin, 1 );
  attrset(0);          /* dwp */
  
  fline = lin + 1;				/* first line to use */
  lline = MSG_LIN1;				/* last line to use */
  fuser = 0;					/* first user in uvec */
  
  while (TRUE) /* repeat-while */
    {
      if ( ! godlike )
	if ( ! stillalive( csnum ) )
	  break;
      i = fuser;
      cdclrl( fline, lline - fline + 1 );
      lin = fline;
      while ( i < nu && lin <= lline )
	{
	  userline( uvec[i], -1, cbuf, godlike, FALSE );
	  
	  /* determine color */
	  if ( snum > 0 && snum <= MAXSHIPS ) /* we're a valid ship */
	    {
		if ( strcmp(cuname[uvec[i]],
			    cuname[suser[snum]]) == 0 )    /* it's ours */
		  attrset(A_BOLD);
		else if (swar[snum][uteam[uvec[i]]]) /* we're at war with it */
		  attrset(RedLevelColor);
		else if (steam[snum] == uteam[uvec[i]] && !selfwar(snum))
		  attrset(GreenLevelColor); /* it's a team ship */
		else
		  attrset(YellowLevelColor);
	    }
	  else if (godlike)/* we are running conqoper */
	    attrset(YellowLevelColor); /* bland view */
	  else			/* we don't have a ship yet */
	    {
	      if ( strcmp(cuname[uvec[i]],
			  cuname[cunum]) == 0 )    /* it's ours */
		attrset(A_BOLD);
	      else if (uwar[cunum][uteam[uvec[i]]]) /* we're war with them */
		attrset(RedLevelColor);	            /* (might be selfwar) */
	      else if (uteam[cunum] == uteam[uvec[i]]) /* team ship */
		attrset(GreenLevelColor);
	      else
		attrset(YellowLevelColor);
	    }
	  
	  cdputs( cbuf, lin, 1 );
	  attrset(0);
	  i = i + 1;
	  lin = lin + 1;
	}
      if ( i >= nu )
	{
	  /* We're displaying the last page. */
	  putpmt( "--- press space when done ---", MSG_LIN2 );
	  cdrefresh();
	  if ( iogtimed( &ch, 1 ) )
	    {
	      if ( ch == TERM_EXTRA )
		fuser = 0;			/* move to first page */
	      else
		break;
	    }
	}
      else
	{
	  /* There are users left to display. */
	  putpmt( "--- press space for more ---", MSG_LIN2 );
	  cdrefresh();
	  if ( iogtimed( &ch, 1 ) )
	    if ( ch == TERM_EXTRA )
	      fuser = 0;			/* move to first page */
	    else if ( ch == ' ' )
	      fuser = i;			/* move to next page */
	    else
	      break;
	}
    }
  
  return;
  
}


/*  userstats - display the user list */
/*  SYNOPSIS */
/*    userstats( godlike, snum ) */
void userstats( int godlike , int snum )
{
  int i, j, unum, nu, fuser, fline, lline, lin;
  static int uvec[MAXUSERS];
  int ch;
  char *hd1="M O R E   U S E R   S T A T S";
  char *hd2="name         cpu  conq coup geno  taken bombed/shot  shots  fired   last entry";
  char *hd3="planets  armies    phaser  torps";

  for (i=0; i<MAXUSERS; i++)
    uvec[i] = i;
  
  nu = 0;
  
  for ( unum = 0; unum < MAXUSERS; unum++)
    if ( ulive[unum])
      {
	for ( i = 0; i < nu; i++ )
	  if ( urating[uvec[i]] < urating[unum] )
	    {
	      for ( j = nu - 1; j >= i; j = j - 1 )
		uvec[j+1] = uvec[j];
	      break;
	    }
	uvec[i] = unum;
	nu++;
      }
  
  /* Do some screen setup. */
  cdclear();
  lin = 1;
  attrset(LabelColor);  /* dwp */
  cdputc( hd1, lin );
  
  lin = lin + 2;
  cdputs( hd3, lin, 34 );
  
  c_strcpy( hd2, cbuf );
  lin = lin + 1;
  cdputs( cbuf, lin, 1 );
  
  for ( j = 0; cbuf[j] != EOS; j = j + 1 )
    if ( cbuf[j] != ' ' )
      cbuf[j] = '-';
  lin = lin + 1;
  cdputs( cbuf, lin, 1 );
  attrset(0);          /* dwp */
  
  fline = lin + 1;				/* first line to use */
  lline = MSG_LIN1;				/* last line to use */
  fuser = 0;					/* first user in uvec */
  
  while (TRUE) /* repeat-while */
    {
      if ( ! godlike )
	if ( ! stillalive( csnum ) )
	  break;
      i = fuser;
      cdclrl( fline, lline - fline + 1 );
      lin = fline;
      while ( i < nu && lin <= lline )
	{
	  statline( uvec[i], cbuf );

	/* determine color */
	if ( snum > 0 && snum <= MAXSHIPS ) /* we're a valid ship */
	  {
	    if ( strcmp(cuname[uvec[i]], 
			cuname[suser[snum]]) == 0)    /* it's ours */
	      attrset(A_BOLD);
	    else if (swar[snum][uteam[uvec[i]]]) /* we're at war with it */
	      attrset(RedLevelColor);
	    else if (steam[snum] == uteam[uvec[i]] && !selfwar(snum))
	      attrset(GreenLevelColor); /* it's a team ship */
	    else
	      attrset(YellowLevelColor);
	  }
	else if (godlike)/* we are running conqoper */ 
	  { 
	    attrset(YellowLevelColor); /* bland view */
	  }
	else
	  {
	    if ( strcmp(cuname[uvec[i]],
			cuname[cunum]) == 0 )    /* it's ours */
	      attrset(A_BOLD);
	    else if (uwar[cunum][uteam[uvec[i]]]) /* we're war with them */
	      attrset(RedLevelColor);             /* (might be selfwar) */
	    else if (uteam[cunum] == uteam[uvec[i]]) /* team ship */
	      attrset(GreenLevelColor);
	    else
	      attrset(YellowLevelColor);
	  }

	  cdputs( cbuf, lin, 1 );
	  attrset(0);
	  i = i + 1;
	  lin = lin + 1;
	}
      if ( i >= nu )
	{
	  /* We're displaying the last page. */
	  putpmt( "--- press space when done ---", MSG_LIN2 );
	  cdrefresh();
	  if ( iogtimed( &ch, 1 ) )
	    {
	      if ( ch == TERM_EXTRA )
		fuser = 0;			/* move to first page */
	      else
		break;
	    }
	}
      else
	{
	  /* There are users left to display. */
	  putpmt( "--- press space for more ---", MSG_LIN2 );
	  cdrefresh();
	  if ( iogtimed( &ch, 1 ) )
	    {
	      if ( ch == TERM_EXTRA )
		fuser = 0;			/* move to first page */
	      else if ( ch == ' ' )
		fuser = i;			/* move to next page */
	      else
		break;
	    }
	}
    }
  
  return;
  
}


/*  statline - format a user stat line */
/*  SYNOPSIS */
/*    int unum */
/*    char buf() */
/*    statline( unum, buf ) */
void statline( int unum, char *buf )
{
  int i, j;
  char ch, junk[MSGMAXLINE], percent[MSGMAXLINE], morejunk[MSGMAXLINE];
  
  if ( unum < 0 || unum >= MAXUSERS )
    {
      buf[0] = EOS;
      return;
    }
  if ( ! ulive[unum] )
    {
      buf[0] = EOS;
      return;
    }
  
  if ( ustats[unum][USTAT_SECONDS] == 0 )
    c_strcpy( "- ", percent );
  else
    {
      i = 1000 * ustats[unum][USTAT_CPUSECONDS] / ustats[unum][USTAT_SECONDS];
      sprintf( percent, "%3d%%", (i + 5) / 10 );
    }
  
  sprintf( junk, "%-12s %4s %4d %4d %4d",
	 cuname[unum],
	 percent,
	 ustats[unum][USTAT_CONQUERS],
	 ustats[unum][USTAT_COUPS],
	 ustats[unum][USTAT_GENOCIDE] );
  
  sprintf( buf, "%s %6d %6d %4d %6d %5d",
	 junk,
	 ustats[unum][USTAT_CONQPLANETS],
	 ustats[unum][USTAT_ARMBOMB],
	 ustats[unum][USTAT_ARMSHIP],
	 ustats[unum][USTAT_PHASERS],
	 ustats[unum][USTAT_TORPS] );
  
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
  
  
  
  sprintf( junk, " %16.16s", ulastentry[unum] );
  
  
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
  
  return;
  
}


/*  zeroplanet - zero a planet (DOES SPECIAL LOCKING) */
/*  SYNOPSIS */
/*    int pnum, snum */
/*    zeroplanet( pnum, snum ) */
/*  NOTE */
/*    This routines ASSUMES you have the common area locked before you it. */
void zeroplanet( int pnum, int snum )
{
  int oteam, i; 
  
  oteam = pteam[pnum];
  pteam[pnum] = TEAM_NOTEAM;
  parmies[pnum] = 0;
  
  /* Make the planet not scanned. */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    pscanned[pnum][i] = FALSE;
  
  if ( oteam != TEAM_SELFRULED && oteam != TEAM_NOTEAM )
    {
      /* Check whether that was the last planet owned by the vanquished. */
      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
	if ( pteam[i] == oteam )
	  return;
      /* Yes. */
      couptime[oteam] = rndint( MIN_COUP_MINUTES, MAX_COUP_MINUTES );
      tcoupinfo[oteam] = FALSE;		/* lost coup info */
      if ( snum > 0 && snum <= MAXSHIPS )
	{
	  ustats[suser[snum]][USTAT_GENOCIDE] =
	    ustats[suser[snum]][USTAT_GENOCIDE] + 1;
	  tstats[steam[snum]][TSTAT_GENOCIDE] =
	    tstats[steam[snum]][TSTAT_GENOCIDE] + 1;
	}
    }
  
  return;
  
}

