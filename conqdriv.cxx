#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q D R I V */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

#define NOEXTERN_GLOBALS
#include "global.h"

#define NOEXTERN_CONF
#include "conf.h"

#include "conqdef.h"
#include "cb.h"
#include "conqlb.h"
#include "rndlb.h"
#include "conqutil.h"
#include "conqai.h"
#include "conqunix.h"

#define NOEXTERN_CONTEXT
#include "context.h"

#include "sem.h"

#define SUBMIN_SECONDS 5 	/* seconds between planet movement */
#define MINUTE_SECONDS 60
#define FIVEMINUTE_SECONDS 300

void iterdrive( int *ship );
void secdrive( int *ship );
void mindrive(void);
void fivemindrive(void);
void SigTerminate(int sig);

/*  conqdriv - main program (DOES LOCKING) */
int main(int argc, char *argv[])
{
    int s, i, j, pid, drivtenths, ship[MAXSHIPS];
    int ctime, etime, cacc, eacc;
    int force;
    int arg;

    /* First things first. */

    if ((ConquestGID = getConquestGID()) == -1)
    {
        fprintf(stderr, "conqdriv: getConquestGID() failed\n");
        exit(1);
    }


    if (GetSysConf(false) == -1)
    {
#ifdef DEBUG_CONFIG
        utLog("%s@%d: main(): GetSysConf() returned -1.", __FILE__, __LINE__);
#endif
/*      exit(1);*/

        ;
    }


    if (setgid(ConquestGID) == -1)
    {
        utLog("conqdriv: setgid(%d): %s",
              ConquestGID,
              strerror(errno));
        fprintf(stderr, "conqdriv: setgid(): failed\n");
        exit(1);
    }

    if (semInit() == -1)
    {
        fprintf(stderr, "semInit() failed to get semaphores. exiting.\n");
        exit(1);
    }

    /* close all file descriptors */
    for (i=0; i < FOPEN_MAX; i++)
        close(i);

    cbMap();

    if ( *cbRevision != COMMONSTAMP )
        utLog("conqdriv:ERROR:common block mismatch");

    initstats( &ctime, &etime );
    cacc = 0;
    eacc = 0;

    cbLock(&cbConqInfo->lockword);
    utGetSecs( &cbDriver->drivtime );		/* prevent driver timeouts */
    utGetSecs( &cbDriver->playtime );

    /* Look for the force flag. */

    force = false;

    while ((arg = getopt(argc, argv, "f")) != EOF)
    {
        switch (arg)
	{
	case 'f':
            force = isagod(-1);
            break;
	}
    }

    if ( ! force )
    {
        /* Make sure we're supposed to be starting. */
        if ( cbDriver->drivstat != DRS_RESTART )
	{
            cbUnlock(&cbConqInfo->lockword);
            utLog("conqdriv: we shouldn't be starting: drivstat = %d\n", cbDriver->drivstat);
	}

        if ( cbDriver->drivpid != 0 )
	{
            cbUnlock(&cbConqInfo->lockword);
            utLog("conqdriv: cbDriver->drivpid != 0, drivpid = %d", cbDriver->drivpid);
	}
    }
    else
    {
        /* Kill the other driver. */
        if ( cbDriver->drivstat == DRS_RUNNING )
	{
            cbDriver->drivstat = DRS_KAMIKAZE;
            i = TIMEOUT_DRIVER * ITER_TENTHS;
            while ( cbDriver->drivstat != DRS_OFF && i > 0 )
	    {
                utSleep( ITER_SECONDS );
                i = i - ITER_TENTHS;
	    }
	}
    }


    cbDriver->drivstat = DRS_STARTING;		/* show intent of becoming "the" driver */
    rndini();			/* init random numbers */
    pid = getpid(); /* store our pid */
    cbDriver->drivpid = pid;

    utStrncpy(cbDriver->drivowner, clbGetUserLogname(), MAXUSERNAME);

    /* Start within bounds. */
    cbDriver->drivsecs = utModPlusOne( cbDriver->drivsecs, FIVEMINUTE_SECONDS );

    /* Special hack to cause the one second fuse to expire upon entry. */
    drivtenths = 10;

    cbDriver->drivstat = DRS_RUNNING;

    cbUnlock(&cbConqInfo->lockword);

    if ( force )
    {
        utLog( "My Lord, driver %d reporting. I have assumed control.", pid );
    }
    else
    {
#ifdef DEBUG_SIG
        utLog("conqdriv(): *DRIVER STARTING* detaching stdio and setting signals");
#endif

        close(0);
        close(1);
        close(2);

        signal(SIGINT, SIG_IGN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTERM, (void (*)(int))SigTerminate);
        signal(SIGHUP, (void (*)(int))SigTerminate);
    }
    for ( s = 0; s < MAXSHIPS; s++ )
        ship[s] = s;

    while ( pid == cbDriver->drivpid && cbDriver->drivstat != DRS_KAMIKAZE )
    {

        if ( drivtenths >= 10 )
	{
            /* Finished a second. */
            drivtenths = 0;

            /* Check for player timeout. */
            if ( utDeltaSecs( cbDriver->playtime, &(cbDriver->drivtime) ) >=
                 TIMEOUT_PLAYER )
	    {
                cbDriver->drivpid = 0;
                cbDriver->drivstat = DRS_OFF;
                cbDriver->drivowner[0] = 0;
                upchuck();
                utLog("conqdriv:player timeout: utDeltaSecs(cbDriver->playtime, &(cbDriver->drivtime)) = %d\n", utDeltaSecs(cbDriver->playtime, &(cbDriver->drivtime)));
                break;
	    }

            if ( cbDriver->drivstat == DRS_RUNNING )
	    {
                /* Randomize ship ordering. */
                for ( s = 0; s < MAXSHIPS; s = s + 1 )
		{
                    i = rndint( 0, MAXSHIPS - 1 );
                    j = ship[i];
                    ship[i] = ship[s];
                    ship[s] = j;
		}

                /* Do the big things first to sync the small things. */
                cbDriver->drivsecs = utModPlusOne( cbDriver->drivsecs + 1, FIVEMINUTE_SECONDS );
                if ( mod( cbDriver->drivsecs, FIVEMINUTE_SECONDS ) == 0 )
                    fivemindrive();
                if ( mod( cbDriver->drivsecs, MINUTE_SECONDS ) == 0 )
                    mindrive();
                if ( mod( cbDriver->drivsecs, SUBMIN_SECONDS ) == 0 )
                {
                    upstats( &ctime, &etime, &cacc, &eacc,
                             &cbConqInfo->dcpuseconds, &cbConqInfo->delapsedseconds );
                }

                secdrive( ship );

                /* Update the common block every minute. */
                if ( mod( cbDriver->drivsecs, MINUTE_SECONDS ) == 0 )
                    upchuck();
	    }
	}
        if ( cbDriver->drivstat == DRS_RUNNING )
        {
            clbPlanetDrive(0.1);
            iterdrive( ship );
        }
        utSleep( ITER_SECONDS );
        drivtenths = drivtenths + ITER_TENTHS;
    }

    /* See if we should turn off. */
    if ( cbDriver->drivstat == DRS_KAMIKAZE )
    {
        cbDriver->drivpid = 0;
        cbDriver->drivstat = DRS_OFF;
        cbDriver->drivowner[0] = 0;
        utLog( "conqdriv:DRS_KAMIKAZE: cbDriver->drivstat = %d\n", cbDriver->drivstat);
    }

    /* Make last minute driver stats update. */
    upstats( &ctime, &etime, &cacc, &eacc, &cbConqInfo->dcpuseconds,
             &cbConqInfo->delapsedseconds );

    exit(0);
}


/*  iterdrive - drive the universe one iteration */
/*  SYNOPSIS */
/*    int ship(MAXSHIPS) */
/*    iterdrive( ship ) */
void iterdrive( int *ship )
{
    int s, t, i, k;
    real h, ad, x, dis, ht, z;
    char buf[MSGMAXLINE];
    real warp;

    /* Drive the ships. */
    for ( s = 0; s < MAXSHIPS; s++ )
    {
        i = ship[s];
        if ( cbShips[i].status == SS_LIVE )
	{
            /* Phaser fuses. */
            if ( cbShips[i].pfuse > 0 )
                cbShips[i].pfuse = max( 0, cbShips[i].pfuse - ITER_TENTHS );

            warp = cbShips[i].warp;

            // If we are locked onto a planet
            if ( cbShips[i].lock == LOCK_PLANET
                 && cbShips[i].lockDetail < MAXPLANETS )
            {
                int pnum = cbShips[i].lockDetail;
                if ( warp < 0.0 )
                {
                    /* Orbiting. */
                    if ( warp == ORBIT_CW )
                    {
                        /* Orbiting clockwise. */
                        cbShips[i].head =
                            utMod360( cbShips[i].head - (ORBIT_FAC/10.0) );

                        /* adjust the ships obital position */
                        clbAdjOrbitalPosition(i);
                    }
                    else if ( warp == ORBIT_CCW )
                    {
                        /* Orbiting counter-clockwise. */
                        cbShips[i].head =
                            utMod360( cbShips[i].head + (ORBIT_FAC/10.0) );

                        /* adjust the ships obital position */
                        clbAdjOrbitalPosition(i);
                    }
                }
                else
                {
                    /* Cruising, locked on; update ship's desired heading. */
                    cbShips[i].dhead = (real) utAngle(cbShips[i].x,
                                                    cbShips[i].y,
                                                    cbPlanets[pnum].x,
                                                    cbPlanets[pnum].y);
                }
            }

            /* Turning. */
            if ( cbShips[i].warp >= 0.0 && cbShips[i].dhead != cbShips[i].head )
	    {
                h = cbShips[i].head;
                ad = utSubAngle( h, cbShips[i].dhead );
                x = (real)max( 210.0 - ((cbShips[i].warp*20.0)/engeff( i )), 2.0 ) * (real)ITER_SECONDS;
                if ( fabs( ad ) <= x )
                    cbShips[i].head = cbShips[i].dhead;
                else if ( ad < 0.0 )
		{
                    z = h + x;

                    if (z > 360.0)
                        z = fabs(z) - 360.0;
                    cbShips[i].head = (real) z;
		}
                else
		{
                    z = h - x;
                    if (z < 0.0)
                        z = 360.0 - fabs(z);
                    cbShips[i].head = (real) z;
		}

                clbFixDeltas( i );
	    }

            /* Movement. */
            if ( STOWEDBY(i) )
	    {
                /* Being towed; slowly (grossly?) align with our tower. */
                int j = cbShips[i].towedby;

                cbShips[i].warp = 0.0;
                cbShips[i].dwarp = 0.0;
                h = utAngle( cbShips[i].x, cbShips[i].y,
                             cbShips[j].x, cbShips[j].y );
                ad = utSubAngle( h, cbShips[j].head );
                if ( ad < 0.0 )
                    h = h - max( ad, -10.0*ITER_SECONDS );
                else
                    h = h - min( ad, +10.0*ITER_SECONDS );

                cbShips[i].x = cbShips[j].x + TOW_DIST * cosd(h+180.0);
                cbShips[i].y = cbShips[j].y + TOW_DIST * sind(h+180.0);
	    }
            else if ( cbShips[i].warp >= 0.0 )
	    {
                /* Cruising. */
                x = min( cbShips[i].dwarp, maxwarp( i ) );
                if ( cbShips[i].warp != x )
		{
                    cbShips[i].warp = clbNewWarp( i, x );
                    clbFixDeltas( i );
		}

                cbShips[i].x = cbShips[i].x + cbShips[i].dx;
                cbShips[i].y = cbShips[i].y + cbShips[i].dy;

                /* If we're locked onto a planet but not orbiting it see if */
                /* we are close enough to orbit. */
                if ( cbShips[i].lock == LOCK_PLANET
                     && cbShips[i].lockDetail < MAXPLANETS)
		{
                    int j = cbShips[i].lockDetail;
                    /* Make sure the planet is still real. */
                    if ( ! PVISIBLE(j) )
                        cbShips[i].lock = LOCK_NONE;
                    else if ( cbShips[i].warp >= 0.0 )
		    {
                        /* Still moving; if we're going slow enough to orbit, */
                        /*  check if we're close enough to do so. Otherwise, */
                        /*  check to see if it's time to slow down yet. */
                        dis = dist( cbShips[i].x, cbShips[i].y,
                                    cbPlanets[j].x, cbPlanets[j].y );
                        if ( cbShips[i].warp <= MAX_ORBIT_WARP )
			{
                            /* Going slow enough to orbit. */
                            if ( dis <= (real)(cbPlanets[j].size / 2) )
			    {
                                /* Close enough to orbit. */
                                clbOrbit( i, j );
                                sprintf( buf,
                                         "Coming into orbit around %s.",
                                         cbPlanets[j].name );
                                clbStoreMsgf( MSG_FROM_COMP, 0,
                                              MSG_TO_SHIP, i,
                                              buf, MSG_FLAGS_TERSABLE );
			    }
			}
                        else if ( ( dis - (real)(cbPlanets[j].size / 2) ) <=
                                  utAccurateDist( cbShips[i].warp, MAX_ORBIT_WARP,
                                                  cbShipTypes[cbShips[i].shiptype].accelfac *
                                                  engeff( i ) ) )
			{
                            /* Time to slow down. */
                            if ( cbShips[i].dwarp > MAX_ORBIT_WARP )
			    {
                                cbShips[i].dwarp = MAX_ORBIT_WARP;
                                /* "WARNING, WILL ROBINSON!!" */
                                sprintf( buf,
                                         "Approaching %s - commencing orbital insertion maneuver.",
                                         cbPlanets[j].name );
                                clbStoreMsgf( MSG_FROM_COMP, 0,
                                              MSG_TO_SHIP, i,
                                              buf, MSG_FLAGS_TERSABLE );
			    }
			}
		    }
		}
	    }
	}
    }

    /* Drive the torps. */
    clbTorpDrive(ITER_SECONDS);
    for ( s = 0; s < MAXSHIPS; s++ )
    {
        i = ship[s];
        if ( cbShips[i].status != SS_OFF )
	{
            for ( int j=0; j < MAXTORPS; j++ )
	    {
                if ( cbShips[i].torps[j].status == TS_DETONATE )
		{
                    /* Detonate. */
                    cbShips[i].torps[j].fuse = FIREBALL_FUSE;
                    cbShips[i].torps[j].status = TS_FIREBALL;
                    for ( t = 0; t < MAXSHIPS; t++ )
		    {
                        k = ship[t];

                        if ( cbShips[k].status == SS_LIVE && k != i )
                            if ( cbShips[i].torps[j].war[cbShips[k].team] ||
                                 cbShips[k].war[cbShips[i].team] )
                            {
                                ht = utExplosionHits( (real)(TORPEDO_HIT *
                                                             cbShips[i].torps[j].mult),
                                                      dist( cbShips[i].torps[j].x,
                                                            cbShips[i].torps[j].y,
                                                            cbShips[k].x, cbShips[k].y ) );
                                if ( ht > 0.0 )
                                    clbHit( k, ht, KB_SHIP, (uint16_t)i );
                            }
		    }
		}
	    }
	}
    }

    /* Drive the planet eater. */
    if ( DOOM_LIVE() )
    {

        /* We want to creep up on our target, without sitting and spinning
         * around on top of it, so only move when we are too far away
         * from our target to inflict damage.
         */

        /* planet lock */
        if (cbDoomsday->lock == LOCK_PLANET
            && cbDoomsday->lockDetail < MAXPLANETS
            && (distf( cbDoomsday->x, cbDoomsday->y,
                       cbPlanets[cbDoomsday->lockDetail].x,
                       cbPlanets[cbDoomsday->lockDetail].y )
                >= DOOMSDAY_DIST) )
        {
            // can't be attacking if we aren't in range
            DOOMCLR(DOOM_F_ATTACKING);
            cbDoomsday->x = cbDoomsday->x + cbDoomsday->dx;
            cbDoomsday->y = cbDoomsday->y + cbDoomsday->dy;
        }

        /* ship lock */
        if (cbDoomsday->lock == LOCK_SHIP
            && cbDoomsday->lockDetail < MAXSHIPS
            && (distf( cbDoomsday->x, cbDoomsday->y,
                       cbShips[cbDoomsday->lock].x,
                       cbShips[cbDoomsday->lock].y )
                >= DOOMSDAY_DIST) )
        {
            // can't be attacking if we aren't in range
            DOOMCLR(DOOM_F_ATTACKING);
            cbDoomsday->x = cbDoomsday->x + cbDoomsday->dx;
            cbDoomsday->y = cbDoomsday->y + cbDoomsday->dy;
        }

    }

    return;

}


/*  secdrive - drive the one-second interval items (DOES LOCKING) */
/*  SYNOPSIS */
/*    int ship(MAXSHIPS) */
/*    secdrive( ship ) */
void secdrive( int *ship )
{
    int s, t, i, j, k;
    real dis, repair, inc, dec;
    real x;
    int talert[MAXSHIPS];

    for ( s = 0; s < MAXSHIPS; s++ )
    {
        i = ship[s];
        if ( cbShips[i].status == SS_OFF )
            continue; /*next;*/

        if ( cbShips[i].status != SS_LIVE )
	{
            /* Turn off timed out ships. */
            if ( cbShips[i].sdfuse < 0 )
	    {
                /* This code may be too safe... */
                cbLock(&cbConqInfo->lockword);
                if ( cbShips[i].status != SS_LIVE && cbShips[i].sdfuse < 0 )
		{
                    cbShips[i].sdfuse = cbShips[i].sdfuse + 1;
                    if ( cbShips[i].sdfuse == 0 )
		    {
                        cbShips[i].sdfuse = 0;
                        cbShips[i].killedBy = KB_NONE;
                        cbShips[i].killedByDetail = 0;
                        cbShips[i].status = SS_OFF;
		    }
		}
                cbUnlock(&cbConqInfo->lockword);
                continue; /* next;*/
	    }
	}

        /* The ship is alive */

        /* see if we've been kicked out. */
        if ( UBANNED(cbShips[i].unum) )
	{
            clbKillShip( i, KB_SHIT, 0 );
            continue; /* next;*/
	}
        if ( cbConqInfo->closed )
            if ( ! UPLAYWHENCLOSED(cbShips[i].unum) )
            {
                clbKillShip( i, KB_EVICT, 0 );
                continue; /*next;*/
            }

        /* The ship is still alive. */
        if ( SROBOT(i) )
            if ( ! cbConqInfo->externrobots )
                robotai( i );

        /* Ships - Teams. */
        for ( j = 0; j <  NUMPLAYERTEAMS; j++ )
            if ( cbShips[i].scanned[j] > 0 )
                cbShips[i].scanned[j] = cbShips[i].scanned[j] - 1;

        /* Ships, planets and suns scans. */
        for ( j = 0; j < MAXPLANETS; j++ )
            if ( PVISIBLE(j) )
            {
                /* Do we scan the planet? */
                dis = dist( cbShips[i].x, cbShips[i].y, cbPlanets[j].x, cbPlanets[j].y );
                if ( dis <= PLANET_DIST_OFF + ((real)cbPlanets[j].size / 2))
                {
                    k = cbShips[i].team;
                    if ( k >= 0 && k < NUMPLAYERTEAMS )
                        cbPlanets[j].scanned[k] = true;

                    /* Planet armies (and suns) get to do damage here. */
                    if ( clbSPWar( i,j ) )
                        clbHit( i,
                                rndnor( PLANET_HIT + cbPlanets[j].armies * ARMY_HIT, 1.0 ),
                                KB_PLANET, (uint16_t)j );
                }

                /* Does the planet scan us? */
                if ( dis <= ACCINFO_DIST )
                    if ( ! SCLOAKED(i) )
                    {
                        k = cbPlanets[j].team;
                        if ( k >= 0 && k <  NUMPLAYERTEAMS )
                            cbShips[i].scanned[k] = SCANNED_FUSE;
                    }
            }

        /* Planet eater. */
        if ( DOOM_LIVE() )
        {
            if ( dist( cbShips[i].x, cbShips[i].y,
                       cbDoomsday->x, cbDoomsday->y ) <= DOOMSDAY_DIST )
            {
                clbHit( i, rndnor( DOOMSDAY_HIT, 1.0 ), KB_DOOMSDAY, 0 );
            }
        }

        /* Negative energy barrier. */
        if ( fabs( cbShips[i].x ) >= NEGENB_DIST || fabs(cbShips[i].y) >= NEGENB_DIST )
            if ( fabs( cbShips[i].x ) <= NEGENBEND_DIST &&
                 fabs( cbShips[i].y ) <= NEGENBEND_DIST )
                clbHit( i, NEGENB_HIT, KB_NEGENB, 0 );

        /* Shields. */
        if ( cbShips[i].shields < 100.0 )
	{
            /* Shields repair twice as fast when they're down. */
            x = SHIELD_FAC;
            if ( ! SSHUP(i) )
                x = x * 2.0;
            cbShips[i].shields = min( 100.0, cbShips[i].shields + x );
	}

        /* Repair. */
        repair = REPAIR_FAC;
        if ( SREPAIR(i) )
	{
            SFCLR(i, SHIP_F_CLOAKED);
            if ( cbShips[i].warp >= 0.0 )
                cbShips[i].dwarp = 0.0;
            repair = repair * RMODE_REPAIR_MULT;
	}
        // orbiting a friendly planet...
        if ( cbShips[i].warp < 0.0
             && cbShips[i].lock == LOCK_PLANET
             && cbShips[i].lockDetail < MAXPLANETS
             && !clbSPWar(i, cbShips[i].lockDetail)
             && cbPlanets[cbShips[i].lockDetail].armies > 0)
        {
            repair = repair * PLANET_REPAIR_MULT;
        }

        cbShips[i].damage = cbShips[i].damage - repair;
        if ( cbShips[i].damage < 0.0 )
	{
            cbShips[i].damage = 0.0;
            if ( SREPAIR(i) )
                SFCLR(i, SHIP_F_REPAIR);
	}

        /* Weapons/engines down fuses. */
        if ( cbShips[i].wfuse > 0 )
	{
            cbShips[i].wfuse = cbShips[i].wfuse - 1;
            if ( cbShips[i].wfuse <= 0 )
                clbStoreMsgf( MSG_FROM_COMP, 0,
                              MSG_TO_SHIP, i,
                              "Weapons are back on-line.",
                              MSG_FLAGS_TERSABLE);
	}
        if ( cbShips[i].efuse > 0 )
	{
            cbShips[i].efuse = cbShips[i].efuse - 1;
            cbShips[i].dwarp = 0.0;
            if ( cbShips[i].efuse <= 0 )
                clbStoreMsgf( MSG_FROM_COMP, 0,
                              MSG_TO_SHIP, i,
                              "Engine power has been restored.",
                              MSG_FLAGS_TERSABLE);
	}

        /* Fuel. */
        inc = FUEL_FAC;
        dec = 0.0;
        if ( cbShips[i].warp < 0.0 )
	{
            /* You get fuel for orbiting friendly, populated class M, */
            /*  with shields down. */
            if ( ! SSHUP(i) || SREPAIR(i) )
                if ( cbPlanets[cbShips[i].lockDetail].type == PLANET_CLASSM )
                    if ( ! clbSPWar( i, cbShips[i].lockDetail ) )
                        if ( cbPlanets[cbShips[i].lockDetail].armies > 0 )
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
            if ( SSHUP(i) )
                dec = dec + cbShips[i].warp * FUELWARP_FAC * FUELSHIELDS_MULT;
            else
                dec = dec + cbShips[i].warp * FUELWARP_FAC;

            dec = dec + cbShips[i].warp * cbShips[i].armies * FUELWARPARMY_FAC;

            if ( cbShips[i].towing != 0 )
	    {
                if ( SSHUP(cbShips[i].towing) )
                    dec = dec + cbShips[i].warp * FUELWARP_FAC * FUELSHIELDS_MULT;
                else
                    dec = dec + cbShips[i].warp * FUELWARP_FAC;
                dec = dec + cbShips[i].warp * cbShips[cbShips[i].towing].armies *
                    FUELWARPARMY_FAC;
	    }
	}
        /* Cloaking. */
        if ( SCLOAKED(i) )
	{
            SFCLR(i, SHIP_F_REPAIR);
            dec = dec + CLOAK_FUEL;
            if ( cbShips[i].warp > 0.0 )
                dec = dec + cbShips[i].dwarp * CLOAK_WARP_FUEL;
	}
        cbShips[i].fuel = min( 999.0, cbShips[i].fuel + inc );
        if ( dec > 0.0 )
            clbUseFuel( i, dec, false, true );

        /* Cool-down. */
        if ( cbShips[i].warp < 0.0 && !clbSPWar(i, cbShips[i].lockDetail)
             &&  cbPlanets[cbShips[i].lockDetail].armies > 0)
	{			    /* orbiting a friendly populated planet */
            cbShips[i].wtemp = max( 0.0, cbShips[i].wtemp - (WEAPON_COOL_FAC * PLANET_REPAIR_MULT));
            cbShips[i].etemp = max( 0.0, cbShips[i].etemp - (ENGINE_COOL_FAC * PLANET_REPAIR_MULT));
	}
        else
	{
            cbShips[i].wtemp = max( 0.0, cbShips[i].wtemp - WEAPON_COOL_FAC );
            cbShips[i].etemp = max( 0.0, cbShips[i].etemp - ENGINE_COOL_FAC );
	}
    }

    /* Torp alert logic. */
    for ( i = 0; i < MAXSHIPS; i++ )
        talert[i] = false;
    for ( s = 0; s < MAXSHIPS; s++ )
    {
        i = ship[s];
        if ( cbShips[i].status != SS_OFF )
	{
            /* Torpedoes. */
            for ( j = 0; j < MAXTORPS; j++ )
                if ( cbShips[i].torps[j].status != TS_OFF )
                {
                    /* Torpedo fuses. */
                    cbShips[i].torps[j].fuse = cbShips[i].torps[j].fuse - 1;
                    if ( cbShips[i].torps[j].fuse <= 0 )
                    {
                        if ( cbShips[i].torps[j].status == TS_LIVE )
                            clbDetonate( i, j );
                        else if ( cbShips[i].torps[j].status == TS_FIREBALL )
                            cbShips[i].torps[j].status = TS_OFF;
                    }
                    else
                    {
                        if ( cbShips[i].torps[j].status == TS_LIVE )
                        {
                            /* Proximity check. */
                            for ( t = 0; t < MAXSHIPS; t++ )
                            {
                                k = ship[t];
                                if ( cbShips[k].status == SS_LIVE && k != i )
                                    if ( cbShips[i].torps[j].war[cbShips[k].team] || cbShips[k].war[cbShips[i].team] )
                                    {
                                        dis = distf( cbShips[i].torps[j].x, cbShips[i].torps[j].y,
                                                     cbShips[k].x, cbShips[k].y );
                                        if ( dis <= TORPEDO_PROX )
                                        {
                                            clbDetonate( i, j );
                                            break;
                                        }
                                        else if ( dis <= ALERT_DIST )
                                            talert[k] = true;
                                    }
                            }
                        }
                        if ( cbShips[i].torps[j].status == TS_LIVE )
                        {
                            /* Proximity check for the doomsday machine. */
                            if ( DOOM_LIVE() )
                                if ( distf( cbShips[i].torps[j].x,
                                            cbShips[i].torps[j].y,
                                            cbDoomsday->x, cbDoomsday->y ) <= (TORPEDO_PROX * 3.0))
                                {
                                    clbDetonate( i, j );
                                    break;
                                }
                        }
                    }
                }
	}
    }
    /* Finish up torp alert logic. */
    for ( i = 0; i < MAXSHIPS; i++ )
        if (talert[i])
            SFSET(i, SHIP_F_TALERT);
        else
            SFCLR(i, SHIP_F_TALERT);

    /* Planet eater. */
    if ( DOOM_LIVE() )
    {
        if (cbDoomsday->lock == LOCK_PLANET && cbDoomsday->lockDetail < MAXPLANETS)
	{
            /* Planet. */
            if ( distf( cbDoomsday->x, cbDoomsday->y,
                        cbPlanets[cbDoomsday->lockDetail].x,
                        cbPlanets[cbDoomsday->lockDetail].y ) <= DOOMSDAY_DIST )
	    {
                DOOMSET(DOOM_F_ATTACKING);
                /* Decrement armies. */
                if ( rnd() <= 0.1 )
                    clbIntrude( -1 /*doomsday*/, cbDoomsday->lockDetail );
                cbLock(&cbConqInfo->lockword);
                cbPlanets[cbDoomsday->lockDetail].armies =
                    cbPlanets[cbDoomsday->lockDetail].armies - 1;
                if ( cbPlanets[cbDoomsday->lockDetail].armies <= 0 )
		{
                    cbPlanets[cbDoomsday->lockDetail].uninhabtime =
                        rndint( MIN_UNINHAB_MINUTES, MAX_UNINHAB_MINUTES );
                    clbZeroPlanet( cbDoomsday->lockDetail, 0 );
                    clbDoomFind();
		}
                cbUnlock(&cbConqInfo->lockword);
	    }
	}
        else if ( cbDoomsday->lock == LOCK_SHIP
                  && cbDoomsday->lockDetail < MAXSHIPS)
	{
            /* Ship. */
            if ( cbShips[cbDoomsday->lockDetail].status != SS_LIVE )
                clbDoomFind();
            else if ( distf( cbDoomsday->x, cbDoomsday->y,
                             cbShips[cbDoomsday->lockDetail].x,
                             cbShips[cbDoomsday->lockDetail].y ) <= DOOMSDAY_DIST )
            {
                /* clever doomsday tractors (was .warp, set .dwarp instead).*/
                cbShips[cbDoomsday->lockDetail].dwarp = 0.0;
            }
	}

        /* Update heading. */
        if ( cbDoomsday->lock == LOCK_PLANET )
        {
            cbDoomsday->heading =
                utAngle( cbDoomsday->x, cbDoomsday->y,
                         cbPlanets[cbDoomsday->lockDetail].x,
                         cbPlanets[cbDoomsday->lockDetail].y );
        }
        else if ( cbDoomsday->lock == LOCK_SHIP )
        {
            cbDoomsday->heading =
                utAngle( cbDoomsday->x, cbDoomsday->y,
                         cbShips[cbDoomsday->lockDetail].x,
                         cbShips[cbDoomsday->lockDetail].y );
        }

        cbDoomsday->dx =
            DOOMSDAY_WARP * MM_PER_SEC_PER_WARP * ITER_SECONDS
            * cosd(cbDoomsday->heading);
        cbDoomsday->dy =
            DOOMSDAY_WARP * MM_PER_SEC_PER_WARP * ITER_SECONDS
            * sind(cbDoomsday->heading);
    }

    return;

}

/*  mindrive - drive the one-minute interval items */
/*  SYNOPSIS */
/*    mindrive */
void mindrive(void)
{

    int i;

    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    {
        /* Decrement couptime(). */
        if ( cbTeams[i].couptime > 0 )
            cbTeams[i].couptime = cbTeams[i].couptime - 1;
    }

    /* cleanup any unliving ships */
    clbCheckShips(true);

    for ( i = 0; i < MAXPLANETS; i++ )
    {
        /* Decrement puninhabtime(). */
        if ( cbPlanets[i].uninhabtime > 0 )
            cbPlanets[i].uninhabtime = cbPlanets[i].uninhabtime - 1;
    }


    if ( DOOM_LIVE() )
        clbDoomFind();
    else if ( rnd() < DOOMSDAY_PROB )
    {
        if (SysConf.NoDoomsday == false)
            clbDoomsday();
    }

    return;

}


/*  fivemindrive - drive the five-minute interval items (DOES LOCKING) */
/*  SYNOPSIS */
/*    fivemindrive */
void fivemindrive(void)
{
    int i, thresh;
    real r;

    /* Drive the planets. */
    cbLock(&cbConqInfo->lockword);
    for ( i = 0; i < MAXPLANETS; i++ )
        if (cbPlanets[i].type != PLANET_SUN)
        {
            if ( cbPlanets[i].armies > 0 && cbPlanets[i].team != TEAM_GOD )
            {
                if ( cbPlanets[i].armies < SPARSE_THRESH )
                {
                    if ( rnd() <= SPARSE_REPOP_PROB )
                        cbPlanets[i].armies = cbPlanets[i].armies + 1;
                }
                else
                {
                    if ( cbPlanets[i].type == PLANET_CLASSM )
                        thresh = MALTHUS_M_THRESH;
                    else
                        thresh = MALTHUS_D_THRESH;

                    if ( cbPlanets[i].armies >= thresh && rnd() <= MALTHUS_PROB )
                    {
                        r = rnd();	/* save to avoid the unfortunate side effects of
                                           max() bieng a macro (rnd() actually got exc'd
                                           twice, resluting in an occasional 0 return val) */

                        cbPlanets[i].armies = max( round( cbPlanets[i].armies * r ), 1 );
                    }
                    else
                    {
                        cbPlanets[i].armies = cbPlanets[i].armies +
                            rndint( REPOP_LOWER_BOUND, REPOP_UPPER_BOUND );
                    }
                }
            }
        }
    cbUnlock(&cbConqInfo->lockword);

    return;

}

void SigTerminate(int sig)
{
    cbDriver->drivstat = DRS_KAMIKAZE;

    utLog("conqdriv: Terminating on signal %d", sig);

    return;
}
