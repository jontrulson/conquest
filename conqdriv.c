
/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 06:35:42  jon
 * Initial revision
 *
 * Revision 1.14  1996/11/23  06:34:27  jon
 * - tweaked secdriv() to multiply the cooldown factors of weap and
 *   engines by 3 if orbiting a friendly populated planet.
 *
 * Revision 1.13  1996/07/02  19:48:40  jon
 * - code cleanup
 * - fixed macro expansion bug that sometimes caused a Mathusian
 *   Crisis to set a planet's population to 0.
 * - disabled Repop/depop on suns.  Their 'effective' army strength
 *   is fixed at a semi random value during planet init.  This may
 *   be tuned in the future.
 *
 * Revision 1.12  1996/05/25  00:34:58  jon
 * - added GetSysConf() call
 * - changed isagod() calls to new style
 * - added sysconf_NoDoomsday check - replaces NO_DOOMSDAY define in
 *   defs.h
 * - added clog call to signal handler
 * - added trap for SIGHUP
 *
 * Revision 1.11  1996/05/02  01:10:48  jon
 * - added call to GetSem() if USE_SEMS is defined
 *
 * Revision 1.10  1996/04/28  05:56:36  jon
 * - removed VERSION_NUMBER and VERSION_DATE, handled by version.c now
 * - secdrive() modified so that fuel usage depends on *actual* warp,
 *   *not* on desired warp.
 *
 * Revision 1.9  1996/03/31  23:44:06  jon
 * - added calls to GetConquestGID().
 * - added priority capability.  Now when conqdriv has the tshar priv,
 *   it will set up it's priority to CONQDRIV_PRI if SET_PRIORITY is
 *   #defined (defs.h).  Makes the driver much more snappy, and doesn't
 *   appear to affect system performance noticibly.
 *
 * Revision 1.8  1996/03/16  21:53:33  jon
 * cerror() -> clog() changes.
 *
 * Revision 1.7  1995/02/08  05:03:33  jon
 * global change of all _And_'s to &&'s and _Or_ to ||'s.  other minor
 * 'corrections' and cleanups.
 *
 * Revision 1.6  1995/02/02  04:24:32  jon
 * ieterdrive() - fixed typo in array index.
 *
 * Revision 1.5  1995/01/30  06:03:15  jon
 * added signal trapping capability so that KB interrupts are ignored,
 * and conqdriv will die gracefully when getting a SIGTERM.
 *
 * Revision 1.4  1995/01/14  22:53:29  jon
 * rmoved NOLOCKON stuff
 *
 * Revision 1.3  1995/01/09  00:46:44  jon
 * Added PVLOCK/UNLOCK capability, fixed some minor bugs, changed cloak
 * code so that a cloaked, warp 0 ship is invisible to alert scanners.
 * Increase torp fuse to 50 seconds, thereby increasing range.  Added
 * msync() to implement the upchuck() function.
 *
 * Revision 1.2  1995/01/02  00:58:58  jon
 * Changed references to NUMPLANETS and MAXSHIPS so that valid
 * values would be between 1 and NUMPLANETS/MAXSHIPS.
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 *
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#include "c_defs.h"
/*                               C O N Q D R I V */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"

static char *conqdrivId = "$Id$";

#define SUBMIN_SECONDS 5 	/* seconds between planet movement */
#define MINUTE_SECONDS 60 
#define FIVEMINUTE_SECONDS 300 


/*##  conqdriv - main program (DOES LOCKING) */
main(int argc, char *argv[])
{
  int s, i, j, pid, drivtenths, ship[MAXSHIPS + 1];
  int ctime, etime, cacc, eacc;
  int force;
  char buf[ARGBUFSIZE];
  int arg;
  
  /* First things first. */
  
  if ((ConquestUID = GetConquestUID()) == ERR)
    {
      fprintf(stderr, "conqdriv: GetConquestUID() failed\n");
      exit(1);
    }
  
  if ((ConquestGID = GetConquestGID()) == ERR)
    {
      fprintf(stderr, "conqdriv: GetConquestGID() failed\n");
      exit(1);
    }
  
  
  if (GetSysConf(FALSE) == ERR)
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
/*      exit(1);*/
    }
  
  
  if (setgid(ConquestGID) == -1)
    {
      clog("conqdriv: setgid(%d): %s", 
	   ConquestGID,
	   sys_errlist[errno]);
      fprintf(stderr, "conqdriv: setgid(): failed\n");
      exit(1);
    }
  
#ifdef USE_SEMS
  if (GetSem() == ERR)
    {
      fprintf(stderr, "GetSem() failed to get semaphores. exiting.\n");
      exit(1);
    }
#endif
  
  map_common();
  
  if ( *commonrev != COMMONSTAMP )
    error("common block mismatch");
  
#ifdef SET_PRIORITY
  /* Increase our priority a bit */
  
  if (nice(CONQDRIV_PRI) == -1)
    {
      clog("conqdriv: main(): nice(CONQDRIV_PRI (%d)): failed: %s",
	   CONQDRIV_PRI,
	   sys_errlist[errno]);
    }
  else
    clog("conqdriv: main(): nice(CONQDRIV_PRI (%d)): succeeded.",
	 CONQDRIV_PRI);
#endif
  
  initstats( &ctime, &etime );
  cacc = 0;
  eacc = 0;
  
  PVLOCK(lockword);
  gsecs( drivtime );		/* prevent driver timeouts */
  gsecs( playtime );
  cpid = getpid();
  
  /* Look for the force flag. */
  
  force = FALSE;
  
  while ((arg = getopt(argc, argv, "f")) != EOF)
    {
      switch (arg)
	{
	case 'f':
	  force = isagod(NULL);
	  break;
	}
    }
  
  if ( ! force )
    {
      /* Make sure we're supposed to be starting. */
      if ( *drivstat != DRS_RESTART )
	{
	  PVUNLOCK(lockword);
	  clog("conqdriv: we shouldn't be starting: drivstat = %d\n", *drivstat);
	  error("conqdriv: We shouldn't be starting...");
	}
      
      if ( *drivpid != 0 )
	{
	  PVUNLOCK(lockword);
	  clog("conqdriv: *drivpid != 0, drivpid = %d", *drivpid);
	  error("conqdriv: drivpid != 0");
	}
    }
  else
    {
      /* Kill the other driver. */
      if ( *drivstat == DRS_RUNNING )
	{
	  *drivstat = DRS_KAMIKAZE;
	  i = TIMEOUT_DRIVER * ITER_TENTHS;
	  while ( *drivstat != DRS_OFF && i > 0 )
	    {
	      c_sleep( ITER_SECONDS );
	      i = i - ITER_TENTHS;
	    }
	}
    }
  
  *drivstat = DRS_STARTING;		/* show intent of becoming "the" driver */
  rndini( 0, 0 );			/* init random numbers */
  pid = getpid(); /* store our pid */
  *drivpid = pid;
  glname( drivowner );		/* store our username */
  
  /* Start within bounds. */
  *drivsecs = modp1( *drivsecs, FIVEMINUTE_SECONDS );
  
  /* Special hack to cause the one second fuse to expire upon entry. */
  drivtenths = 10;
  
  *drivstat = DRS_RUNNING;
  
  PVUNLOCK(lockword);
  
  if ( force )
    {
      clog( "My Lord, driver %d reporting. I have assumed control.", pid );
      cerror( "My Lord, driver %d reporting. I have assumed control.", pid );
    }
  else
    {
#ifdef DEBUG_SIG
      clog("conqdriv(): *DRIVER STARTING* detaching stdio and setting signals");
#endif
      
      close(0);
      close(1);
      close(2);
      
      signal(SIGINT, SIG_IGN);
      signal(SIGQUIT, SIG_IGN);
      signal(SIGTERM, (void (*)(int))SigTerminate);
      signal(SIGHUP, (void (*)(int))SigTerminate);
    }
  for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    ship[s] = s;
  
  while ( pid == *drivpid && *drivstat != DRS_KAMIKAZE )
    {
      if ( drivtenths >= 10 )
	{
	  /* Finished a second. */
	  drivtenths = 0;
	  
	  /* Check for player timeout. */
	  if ( dsecs( *playtime, drivtime ) >= TIMEOUT_PLAYER )
	    {
	      *drivpid = 0;
	      *drivstat = DRS_OFF;
	      drivowner[0] = EOS;
	      upchuck();
	      clog("conqdriv:player timeout: dsecs(*playtime, drivtime) = %d\n", dsecs(*playtime, drivtime));
	      break;
	    }
	  
	  if ( *drivstat == DRS_RUNNING )
	    {
	      /* Randomize ship ordering. */
	      for ( s = 1; s <= MAXSHIPS; s = s + 1 )
		{
		  i = rndint( 1, MAXSHIPS );
		  j = ship[i];
		  ship[i] = ship[s];
		  ship[s] = j;
		}
	      
	      /* Do the big things first to sync the small things. */
	      *drivsecs = modp1( *drivsecs + 1, FIVEMINUTE_SECONDS );
	      if ( mod( *drivsecs, FIVEMINUTE_SECONDS ) == 0 )
		fivemindrive(); 
	      if ( mod( *drivsecs, MINUTE_SECONDS ) == 0 )
		mindrive();	
	      if ( mod( *drivsecs, SUBMIN_SECONDS ) == 0 )
		{
		  submindrive();
		  upstats( &ctime, &etime, &cacc, &eacc,
			  dcpuseconds, delapsedseconds );
		}
	      secdrive( ship );
	      
	      /* Update the common block every minute. */
	      if ( mod( *drivsecs, MINUTE_SECONDS ) == 0 )
		upchuck();
	    }
	}
      if ( *drivstat == DRS_RUNNING )
	iterdrive( ship );
      c_sleep( ITER_SECONDS );
      drivtenths = drivtenths + ITER_TENTHS;
    }
  
  /* See if we should turn off. */
  if ( *drivstat == DRS_KAMIKAZE )
    {
      *drivpid = 0;
      *drivstat = DRS_OFF;
      drivowner[0] = EOS;
      clog( "conqdriv:DRS_KAMIKAZE: *drivstat = %d\n", *drivstat);
    }
  
  /* Make last minute driver stats update. */
  upstats( &ctime, &etime, &cacc, &eacc, dcpuseconds, delapsedseconds );
  
}


/*##  iterdrive - drive the universe one iteration */
/*  SYNOPSIS */
/*    int ship(MAXSHIPS) */
/*    iterdrive( ship ) */
void iterdrive( int ship[] )
{
  int s, t, i, j, k;
  real h, ad, x, dis, ht;
  char buf[MSGMAXLINE];
  
  /* Drive the ships. */
  for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
      i = ship[s];
      if ( sstatus[i] == SS_LIVE )
	{
	  /* Phaser fuses. */
	  if ( spfuse[i] > 0 )
	    spfuse[i] = max( 0, spfuse[i] - ITER_TENTHS );
	  
	  /* Turning. */
	  if ( swarp[i] >= 0.0 && sdhead[i] != shead[i] )
	    {
	      h = shead[i];
	      ad = subang( h, sdhead[i] );
	      x = (real)max( 210.0 - ((swarp[i]*20.0)/engeff( i )), 2.0 ) * ITER_SECONDS;
	      if ( abs( ad ) <= x )
		shead[i] = sdhead[i];
	      else if ( ad < 0.0 )
		shead[i] = h + x;
	      else
		shead[i] = h - x;
	      
	      fixdeltas( i );
	    }
	  
	  /* Movement. */
	  j = stowedby[i];
	  if ( j != 0 )
	    {
	      /* Being towed; slowly (grossly?) align with our tower. */
	      swarp[i] = 0.0;
	      sdwarp[i] = 0.0;
	      h = angle( sx[i], sy[i], sx[j], sy[j] );
	      ad = subang( h, shead[j] );
	      if ( ad < 0.0 )
		h = h - max( ad, -10.0*ITER_SECONDS );
	      else
		h = h - min( ad, +10.0*ITER_SECONDS );
	      
	      sx[i] = sx[j] + TOW_DIST * cosd(h+180.0);
	      sy[i] = sy[j] + TOW_DIST * sind(h+180.0);
	    }
	  else if ( swarp[i] >= 0.0 )
	    {
	      /* Cruising. */
	      x = min( sdwarp[i], maxwarp( i ) );
	      if ( swarp[i] != x )
		{
		  swarp[i] = newarp( i, x );
		  fixdeltas( i );
		}
	      
	      sx[i] = sx[i] + sdx[i];
	      sy[i] = sy[i] + sdy[i];
	      
	      /* If we're locked onto a planet but not orbiting it see if */
	      /* we are close enough to orbit. */
	      j = -slock[i];
	      if ( j > 0)
		{
		  /* Make sure the planet is still real. */
		  if ( ! preal[j] )
		    slock[i] = 0;
		  else if ( swarp[i] >= 0.0 )
		    {
		      /* Still moving; if we're going slow enough to orbit, */
		      /*  check if we're close enough to do so. Otherwise, */
		      /*  check to see if it's time to slow down yet. */
		      dis = dist( sx[i], sy[i], px[j], py[j] );
		      if ( swarp[i] <= MAX_ORBIT_WARP )
			{
			  /* Going slow enough to orbit. */
			  if ( dis <= ORBIT_DIST )
			    {
			      /* Close enough to orbit. */
			      orbit( i, j );
			      if ( ! soption[i][OPT_TERSE] )
				{
				  sprintf( buf,
					 "Coming into orbit around %s.",
					 pname[j] );
				  stormsg( MSG_COMP, i, buf );
				}
			    }
			}
		      else if ( ( dis - ORBIT_DIST ) <=
			       acdist( swarp[i], MAX_ORBIT_WARP,
				      accelfac[steam[i]] * engeff( i ) ) )
			{
			  /* Time to slow down. */
			  if ( sdwarp[i] > MAX_ORBIT_WARP )
			    {
			      sdwarp[i] = MAX_ORBIT_WARP;
			      if ( ! soption[i][OPT_TERSE] )
				{
				  /* "WARNING, WILL ROBINSON!!" */
				  sprintf( buf,
					 "Approaching %s - commencing orbital insertion maneuver.",
					 pname[j] );
				  stormsg( MSG_COMP, i, buf );
				}
			    }
			}
		    }
		}
	    }
	}
    }
  
  /* Drive the torps. */
  for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
      i = ship[s];
      if ( sstatus[i] != SS_OFF )
	{
	  for ( j = 0; j < MAXTORPS; j = j + 1 )
	    if ( tstatus[i][j] == TS_LIVE )
	      {
		/* Movement. */
		tx[i][j] = tx[i][j] + tdx[i][j];
		ty[i][j] = ty[i][j] + tdy[i][j];
	      }
	    else if ( tstatus[i][j] == TS_DETONATE )
	      {
		/* Detonate. */
		tfuse[i][j] = FIREBALL_FUSE;
		tstatus[i][j] = TS_FIREBALL;
		for ( t = 1; t <= MAXSHIPS; t = t + 1 )
		  {
		    k = ship[t];
		    if ( sstatus[k] == SS_LIVE && k != i )
		      if ( twar[i][j][steam[k]] || swar[k][steam[i]] )
			{
			  ht = explosion( TORPEDO_HIT * tmult[i][j],
					 dist( tx[i][j], ty[i][j], sx[k], sy[k] ) );
			  if ( ht > 0.0 )
			    hit( k, ht, i );
			}
		  }
	      }
	}
    }
  
  /* Drive the planet eater. */
  if ( *dstatus == DS_LIVE )
    {
      *dx = *dx + *ddx;
      *dy = *dy + *ddy;
    }
  
  return;
  
}


/*##  secdrive - drive the one-second interval items (DOES LOCKING) */
/*  SYNOPSIS */
/*    int ship(MAXSHIPS) */
/*    secdrive( ship ) */
void secdrive( int ship[] )
{
  int s, t, i, j, k, pnum, ctemp;
  real dis, repair, inc, dec;
  real x, warp;
  int talert[MAXSHIPS + 1], l;
  char buf[MSGMAXLINE];
  
  for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
      i = ship[s];
      if ( sstatus[i] == SS_OFF )
	continue; /*next;*/
      
      if ( sstatus[i] != SS_LIVE )
	{
	  /* Turn off timed out ships. */
	  if ( ssdfuse[i] < 0 )
	    {
	      /* This code may be too safe... */
	      PVLOCK(lockword);
	      if ( sstatus[i] != SS_LIVE && ssdfuse[i] < 0 )
		{
		  ssdfuse[i] = ssdfuse[i] + 1;
		  if ( ssdfuse[i] == 0 )
		    {
		      ssdfuse[i] = 0;
		      skilledby[i] = 0;
		      sstatus[i] = SS_OFF;
		    }
		}
	      PVUNLOCK(lockword);
	      continue; /* next;*/
	    }
	}
      
      /* The ship is alive; see if we've been kicked out. */
      if ( uooption[suser[i]][OOPT_SHITLIST] )
	{
	  killship( i, KB_SHIT );
	  continue; /* next;*/
	}
      if ( *closed )
	if ( ! uooption[suser[i]][OOPT_PLAYWHENCLOSED] )
	  {
	    killship( i, KB_EVICT );
	    continue; /*next;*/
	  }
      
      /* The ship is still alive. */
      if ( srobot[i] )
	if ( ! *externrobots )
	  robotai( i );
      
      /* Ship movement again. */
      warp = swarp[i];
      pnum = -slock[i];
      if ( pnum > 0 && pnum <= NUMPLANETS )
	if ( warp < 0.0 )
	  {
	    /* Orbiting. */
	    if ( warp == ORBIT_CW )
	      {
		/* Orbiting clockwise. */
		shead[i] = mod360( shead[i] - ORBIT_FAC );
		
		sx[i] = (real)(px[pnum] + (ORBIT_DIST * 
					   cosd(shead[i]+90.0)));
		
		sy[i] = (real)(py[pnum] + (ORBIT_DIST * 
					   sind(shead[i]+90.0)));
	      }
	    else if ( warp == ORBIT_CCW )
	      {
		/* Orbiting counter-clockwise. */
		shead[i] = mod360( shead[i] + ORBIT_FAC );
		sx[i] = (real)(px[pnum] + ORBIT_DIST * 
			       cosd(shead[i]-90.0));
		sy[i] = (real)(py[pnum] + ORBIT_DIST * 
			       sind(shead[i]-90.0));
	      }
	  }
	else
	  {
	    /* Cruising, locked on; update ship's desired heading. */
	    sdhead[i] = (real) angle(sx[i], sy[i], px[pnum], py[pnum]);
	  }
      
      /* Ships - Teams. */
      for ( j = 0; j <  NUMTEAMS; j = j + 1 )
	if ( sscanned[i][j] > 0 )
	  sscanned[i][j] = sscanned[i][j] - 1;
      
      /* Ships, planets and suns scans. */
      for ( j = 1; j <= NUMPLANETS; j = j + 1 )
	if ( preal[j] )
	  {
	    /* Do we scan the planet? */
	    dis = dist( sx[i], sy[i], px[j], py[j] );
	    if ( dis <= PLANET_DIST )
	      {
		k = steam[i];
		if ( k >= 0 && k < NUMTEAMS )
		  pscanned[j][k] = TRUE;
		
		/* Planet armies (and suns) get to do damage here. */
		if ( spwar( i,j ) )
		  hit( i,
		      rndnor( PLANET_HIT + parmies[j] * ARMY_HIT, 1.0 ),
		      -j );
	      }
	    
	    /* Does the planet scan us? */
	    if ( j <= NUMPLANETS )
	      if ( dis <= ACCINFO_DIST )
		if ( ! scloaked[i] )
		  {
		    k = pteam[j];
		    if ( k >= 0 && k <  NUMTEAMS )
		      sscanned[i][k] = SCANNED_FUSE;
		  }
	  }
      
      /* Planet eater. */
      if ( *dstatus == DS_LIVE )
	if ( dist( sx[i], sy[i], *dx, *dy ) <= DOOMSDAY_DIST )
	  hit( i, rndnor( DOOMSDAY_HIT, 1.0 ), KB_DOOMSDAY );
      
      /* Negative energy barrier. */
      if ( abs( sx[i] ) >= NEGENB_DIST || abs(sy[i]) >= NEGENB_DIST )
	if ( abs( sx[i] ) <= NEGENBEND_DIST &&
	    abs( sy[i] ) <= NEGENBEND_DIST )
	  hit( i, NEGENB_HIT, KB_NEGENB );
      
      /* Shields. */
      if ( sshields[i] < 100.0 )
	{
	  /* Shields repair twice as fast when they're down. */
	  x = SHIELD_FAC;
	  if ( ! sshup[i] )
	    x = x * 2.0;
	  sshields[i] = min( 100.0, sshields[i] + x );
	}
      
      /* Repair. */
      repair = REPAIR_FAC;
      if ( srmode[i] )
	{
	  scloaked[i] = FALSE;
	  if ( swarp[i] >= 0.0 )
	    sdwarp[i] = 0.0;
	  repair = repair * RMODE_REPAIR_MULT;
	}
      if ( swarp[i] < 0.0 )			/* orbiting */
	if ( ! spwar( i,-slock[i] ) )	/* a friendly */
	  if ( parmies[-slock[i]] > 0 )	/* populated planet */
	    repair = repair * PLANET_REPAIR_MULT;
      sdamage[i] = sdamage[i] - repair;
      if ( sdamage[i] < 0.0 )
	{
	  sdamage[i] = 0.0;
	  if ( srmode[i] )
	    srmode[i] = FALSE;
	}
      
      /* Weapons/engines down fuses. */
      if ( swfuse[i] > 0 )
	{
	  swfuse[i] = swfuse[i] - 1;
	  if ( swfuse[i] <= 0 )
	    if ( ! soption[i][OPT_TERSE] )
	      stormsg( MSG_COMP, i, "Weapons are back on-line." );
	}
      if ( sefuse[i] > 0 )
	{
	  sefuse[i] = sefuse[i] - 1;
	  sdwarp[i] = 0.0;
	  if ( sefuse[i] <= 0 )
	    if ( ! soption[i][OPT_TERSE] )
	      stormsg( MSG_COMP, i, "Engine power has been restored." );
	}
      
      /* Fuel. */
      inc = FUEL_FAC;
      dec = 0.0;
      if ( swarp[i] < 0.0 )
	{
	  /* You get fuel for orbiting friendly, populated class M, */
	  /*  with shields down. */
	  if ( ! sshup[i] || srmode[i] )
	    if ( ptype[-slock[i]] == PLANET_CLASSM )
	      if ( ! spwar( i,-slock[i] ) )
		if ( parmies[-slock[i]] > 0 )
		  inc = inc * MPLANET_FUEL_MULT;
	}
      else
	{
	  /* Cruising. */
	  
	  /* JET- changed code below to compute based on
	   * swarp instead of sdwarp.  I think this was a bug.
	   * ie: your fuel consumption should be based on how
	   * fast your going, not how fast you *want* to go.
	   */
	  if ( sshup[i] )
	    dec = dec + swarp[i] * FUELWARP_FAC * FUELSHIELDS_MULT;
	  else
	    dec = dec + swarp[i] * FUELWARP_FAC;
	  
	  dec = dec + swarp[i] * sarmies[i] * FUELWARPARMY_FAC;
	  
	  if ( stowing[i] != 0 )
	    {
	      if ( sshup[stowing[i]] )
		dec = dec + swarp[i] * FUELWARP_FAC * FUELSHIELDS_MULT;
	      else
		dec = dec + swarp[i] * FUELWARP_FAC;
	      dec = dec + swarp[i] * sarmies[stowing[i]] * FUELWARPARMY_FAC;
	    }
	}
      /* Cloaking. */
      if ( scloaked[i] )
	{
	  srmode[i] = FALSE;
	  dec = dec + CLOAK_FUEL;
	  if ( swarp[i] > 0.0 )
	    dec = dec + sdwarp[i] * CLOAK_WARP_FUEL;
	}
      sfuel[i] = min( 999.0, sfuel[i] + inc );
      if ( dec > 0.0 )
	l = usefuel( i, dec, FALSE );
      
      /* Cool-down. */
      if ( swarp[i] < 0.0 && ! spwar( i,-slock[i]) &&  parmies[-slock[i]] > 0)	
	{			        /* orbiting a friendly populated planet */
	  swtemp[i] = max( 0.0, swtemp[i] - (WEAPON_COOL_FAC * PLANET_REPAIR_MULT));
	  setemp[i] = max( 0.0, setemp[i] - (ENGINE_COOL_FAC * PLANET_REPAIR_MULT));
	}
      else
	{
	  swtemp[i] = max( 0.0, swtemp[i] - WEAPON_COOL_FAC );
	  setemp[i] = max( 0.0, setemp[i] - ENGINE_COOL_FAC );
	}
    }

  /* Torp alert logic. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    talert[i] = FALSE;
  for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
      i = ship[s];
      if ( sstatus[i] != SS_OFF )
	{
	  /* Torpedoes. */
	  for ( j = 0; j < MAXTORPS; j = j + 1 )
	    if ( tstatus[i][j] != TS_OFF )
	      {
		/* Torpedo fuses. */
		tfuse[i][j] = tfuse[i][j] - 1;
		if ( tfuse[i][j] <= 0 )
		  {
		    if ( tstatus[i][j] == TS_LIVE )
		      detonate( i, j );
		    else if ( tstatus[i][j] == TS_FIREBALL )
		      tstatus[i][j] = TS_OFF;
		  }
		else
		  {
		    if ( tstatus[i][j] == TS_LIVE )
		      {
			/* Proximity check. */
			for ( t = 1; t <= MAXSHIPS; t = t + 1 )
			  {
			    k = ship[t];
			    if ( sstatus[k] == SS_LIVE && k != i )
			      if ( twar[i][j][steam[k]] || swar[k][steam[i]] )
				{
				  dis = distf( tx[i][j], ty[i][j],
					      sx[k], sy[k] );
				  if ( dis <= TORPEDO_PROX )
				    {
				      detonate( i, j );
				      break;
				    }
				  else if ( dis <= ALERT_DIST )
				    talert[k] = TRUE;
				}
			  }
		      }
		    if ( tstatus[i][j] == TS_LIVE )
		      {
			/* Proximity check for the doomsday machine. */
			if ( *dstatus == DS_LIVE )
			  if ( distf( tx[i][j], ty[i][j], *dx, *dy ) <=
			      TORPEDO_PROX )
			    {
			      detonate( i, j );
			      break;
			    }
		      }
		  }
	      }
	}
    }
  /* Finish up torp alert logic. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    stalert[i] = talert[i];
  
  /* Planet eater. */
  if ( *dstatus == DS_LIVE )
    {
      if ( *dlock < 0 )
	{
	  /* Planet. */
	  if ( distf( *dx, *dy, px[-*dlock], py[-*dlock] ) <= DOOMSDAY_DIST )
	    {
	      /* Decrement armies. */
	      if ( rnd( 0 ) <= 0.1 )
		intrude( MSG_DOOM, -*dlock );
	      PVLOCK(lockword);
	      parmies[-*dlock] = parmies[-*dlock] - 1;
	      if ( parmies[-*dlock] <= 0 )
		{
		  puninhabtime[-*dlock] = rndint( MIN_UNINHAB_MINUTES,
						 MAX_UNINHAB_MINUTES );
		  zeroplanet( -*dlock, 0 );
		  doomfind();
		}
	      PVUNLOCK(lockword);
	    }
	}
      else if ( *dlock > 0 )
	{
	  /* Ship. */
	  if ( sstatus[*dlock] != SS_LIVE )
	    doomfind();
	  else if ( distf( *dx, *dy, sx[*dlock], sy[*dlock] ) <= DOOMSDAY_DIST )
	    swarp[*dlock] = 0.0;	/* clever doomsday tractors */
	}
      
      /* Update heading. */
      if ( *dlock < 0 )
	*dhead = angle( *dx, *dy, px[-*dlock], py[-*dlock] );
      else if ( *dlock > 0 )
	*dhead = angle( *dx, *dy, sx[*dlock], sy[*dlock] );
      *ddx = DOOMSDAY_WARP * MM_PER_SEC_PER_WARP * ITER_SECONDS * cosd(*dhead);
      *ddy = DOOMSDAY_WARP * MM_PER_SEC_PER_WARP * ITER_SECONDS * sind(*dhead);
    }
  
  return;
  
}


/*##  submindrive - drive the sub-minute interval items */
/*  SYNOPSIS */
/*    submindrive */
void submindrive(void)
{
  
  int i;
  real speed;
  
  for ( i = NUMPLANETS; i > 0; i = i - 1 )
    {
      /* Advance porbang(). */
      if ( pprimary[i] != 0 )
	{
	  porbang[i] = mod360( porbang[i] + porbvel[i] *
			      SUBMIN_SECONDS / 60.0 );
	  px[i] = px[pprimary[i]] + porbrad[i] * cosd(porbang[i]);
	  py[i] = py[pprimary[i]] + porbrad[i] * sind(porbang[i]);
	  
	}
      else if ( porbvel[i] != 0.0 )
	{
	  /* Special hack for planets to move in a straight line. */
	  speed = porbvel[i] * MM_PER_SEC_PER_WARP * SUBMIN_SECONDS;
	  px[i] = px[i] + speed * cosd(porbang[i]);
	  py[i] = py[i] + speed * sind(porbang[i]);
	}
    }
  
  return;
  
}


/*##  mindrive - drive the one-minute interval items */
/*  SYNOPSIS */
/*    mindrive */
void mindrive(void)
{
  
  int i;
  
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    {
      /* Decrement couptime(). */
      if ( couptime[i] > 0 )
	couptime[i] = couptime[i] - 1;
    }
  
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    {
      /* Decrement puninhabtime(). */
      if ( puninhabtime[i] > 0 )
	puninhabtime[i] = puninhabtime[i] - 1;
    }
  
  
  if ( *dstatus == DS_LIVE )
    doomfind();
  else if ( rnd( 0 ) < DOOMSDAY_PROB )
    {
      if (sysconf_NoDoomsday == FALSE)
	doomsday();
    }
  
  return;
  
}


/*##  fivemindrive - drive the five-minute interval items (DOES LOCKING) */
/*  SYNOPSIS */
/*    fivemindrive */
void fivemindrive(void)
{
  int i, thresh;
  real r;
  
  /* Drive the planets. */
  PVLOCK(lockword);
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if (ptype[i] != PLANET_SUN)
      if ( parmies[i] > 0 && pteam[i] != TEAM_GOD )
	if ( parmies[i] < SPARSE_THRESH )
	  {
	    if ( rnd( 0 ) <= SPARSE_REPOP_PROB )
	      parmies[i] = parmies[i] + 1;
	  }
	else
	  {
	    if ( ptype[i] == PLANET_CLASSM )
	      thresh = MALTHUS_M_THRESH;
	    else
	      thresh = MALTHUS_D_THRESH;
	    
	    if ( parmies[i] >= thresh && rnd( 0 ) <= MALTHUS_PROB )
	      {
		r = rnd(0);	/* save to avoid the unfortunate side effects of
				   max() bieng a macro (rnd() actually got exc'd
				   twice, resluting in an occasional 0 return val) */
		
		parmies[i] = max( round( parmies[i] * r ), 1 );
	      }
	    else
	      {
		parmies[i] = parmies[i] +
		  rndint( REPOP_LOWER_BOUND, REPOP_UPPER_BOUND );
	      }
	  }
  PVUNLOCK(lockword);
  
  return;
  
}

void SigTerminate(int sig)
{
  *drivstat = DRS_KAMIKAZE;
  
  clog("conqdriv: Terminating on signal %d", sig);
  
  return;
}

