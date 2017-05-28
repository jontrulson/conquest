#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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
#include "conqcom.h"
#include "conqlb.h"
#include "conqutil.h"

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
    int s, i, j, pid, drivtenths, ship[MAXSHIPS + 1];
    int ctime, etime, cacc, eacc;
    int force;
    int arg;

    /* First things first. */

    if ((ConquestGID = getConquestGID()) == -1)
    {
        fprintf(stderr, "conqdriv: getConquestGID() failed\n");
        exit(1);
    }


    if (GetSysConf(FALSE) == -1)
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

    map_common();

    if ( *CBlockRevision != COMMONSTAMP )
        utLog("conqdriv:ERROR:common block mismatch");

    initstats( &ctime, &etime );
    cacc = 0;
    eacc = 0;

    PVLOCK(&ConqInfo->lockword);
    utGetSecs( &Driver->drivtime );		/* prevent driver timeouts */
    utGetSecs( &Driver->playtime );

    /* Look for the force flag. */

    force = FALSE;

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
        if ( Driver->drivstat != DRS_RESTART )
	{
            PVUNLOCK(&ConqInfo->lockword);
            utLog("conqdriv: we shouldn't be starting: drivstat = %d\n", Driver->drivstat);
	}

        if ( Driver->drivpid != 0 )
	{
            PVUNLOCK(&ConqInfo->lockword);
            utLog("conqdriv: Driver->drivpid != 0, drivpid = %d", Driver->drivpid);
	}
    }
    else
    {
        /* Kill the other driver. */
        if ( Driver->drivstat == DRS_RUNNING )
	{
            Driver->drivstat = DRS_KAMIKAZE;
            i = TIMEOUT_DRIVER * ITER_TENTHS;
            while ( Driver->drivstat != DRS_OFF && i > 0 )
	    {
                utSleep( ITER_SECONDS );
                i = i - ITER_TENTHS;
	    }
	}
    }


    Driver->drivstat = DRS_STARTING;		/* show intent of becoming "the" driver */
    rndini( 0, 0 );			/* init random numbers */
    pid = getpid(); /* store our pid */
    Driver->drivpid = pid;

    strncpy(Driver->drivowner, clbGetUserLogname(), MAXUSERNAME);
    Driver->drivowner[MAXUSERNAME - 1] = 0;

    /* Start within bounds. */
    Driver->drivsecs = utModPlusOne( Driver->drivsecs, FIVEMINUTE_SECONDS );

    /* Special hack to cause the one second fuse to expire upon entry. */
    drivtenths = 10;

    Driver->drivstat = DRS_RUNNING;

    PVUNLOCK(&ConqInfo->lockword);

    if ( force )
    {
        utLog( "My Lord, driver %d reporting. I have assumed control.", pid );
        utError( "My Lord, driver %d reporting. I have assumed control.", pid );
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
    for ( s = 1; s <= MAXSHIPS; s = s + 1 )
        ship[s] = s;

    while ( pid == Driver->drivpid && Driver->drivstat != DRS_KAMIKAZE )
    {

        if ( drivtenths >= 10 )
	{
            /* Finished a second. */
            drivtenths = 0;

            /* Check for player timeout. */
            if ( utDeltaSecs( Driver->playtime, &(Driver->drivtime) ) >=
                 TIMEOUT_PLAYER )
	    {
                Driver->drivpid = 0;
                Driver->drivstat = DRS_OFF;
                Driver->drivowner[0] = 0;
                upchuck();
                utLog("conqdriv:player timeout: utDeltaSecs(Driver->playtime, &(Driver->drivtime)) = %d\n", utDeltaSecs(Driver->playtime, &(Driver->drivtime)));
                break;
	    }

            if ( Driver->drivstat == DRS_RUNNING )
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
                Driver->drivsecs = utModPlusOne( Driver->drivsecs + 1, FIVEMINUTE_SECONDS );
                if ( mod( Driver->drivsecs, FIVEMINUTE_SECONDS ) == 0 )
                    fivemindrive();
                if ( mod( Driver->drivsecs, MINUTE_SECONDS ) == 0 )
                    mindrive();
                if ( mod( Driver->drivsecs, SUBMIN_SECONDS ) == 0 )
                {
                    upstats( &ctime, &etime, &cacc, &eacc,
                             &ConqInfo->dcpuseconds, &ConqInfo->delapsedseconds );
                }

                secdrive( ship );

                /* Update the common block every minute. */
                if ( mod( Driver->drivsecs, MINUTE_SECONDS ) == 0 )
                    upchuck();
	    }
	}
        if ( Driver->drivstat == DRS_RUNNING )
        {
            clbPlanetDrive(0.1);
            iterdrive( ship );
        }
        utSleep( ITER_SECONDS );
        drivtenths = drivtenths + ITER_TENTHS;
    }

    /* See if we should turn off. */
    if ( Driver->drivstat == DRS_KAMIKAZE )
    {
        Driver->drivpid = 0;
        Driver->drivstat = DRS_OFF;
        Driver->drivowner[0] = 0;
        utLog( "conqdriv:DRS_KAMIKAZE: Driver->drivstat = %d\n", Driver->drivstat);
    }

    /* Make last minute driver stats update. */
    upstats( &ctime, &etime, &cacc, &eacc, &ConqInfo->dcpuseconds,
             &ConqInfo->delapsedseconds );

    exit(0);
}


/*  iterdrive - drive the universe one iteration */
/*  SYNOPSIS */
/*    int ship(MAXSHIPS) */
/*    iterdrive( ship ) */
void iterdrive( int *ship )
{
    int s, t, i, j, k;
    real h, ad, x, dis, ht, z;
    char buf[MSGMAXLINE];
    real warp;
    int pnum;

    /* Drive the ships. */
    for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
        i = ship[s];
        if ( Ships[i].status == SS_LIVE )
	{
            /* Phaser fuses. */
            if ( Ships[i].pfuse > 0 )
                Ships[i].pfuse = max( 0, Ships[i].pfuse - ITER_TENTHS );

            warp = Ships[i].warp;
            pnum = -Ships[i].lock;
            if ( pnum > 0 && pnum <= NUMPLANETS )
            {
                if ( warp < 0.0 )
                {
                    /* Orbiting. */
                    if ( warp == ORBIT_CW )
                    {
                        /* Orbiting clockwise. */
                        Ships[i].head =
                            utMod360( Ships[i].head - (ORBIT_FAC/10.0) );

                        /* adjust the ships obital position */
                        clbAdjOrbitalPosition(i);
                    }
                    else if ( warp == ORBIT_CCW )
                    {
                        /* Orbiting counter-clockwise. */
                        Ships[i].head =
                            utMod360( Ships[i].head + (ORBIT_FAC/10.0) );

                        /* adjust the ships obital position */
                        clbAdjOrbitalPosition(i);
                    }
                }
                else
                {
                    /* Cruising, locked on; update ship's desired heading. */
                    Ships[i].dhead = (real) utAngle(Ships[i].x, Ships[i].y, Planets[pnum].x, Planets[pnum].y);
                }
            }

            /* Turning. */
            if ( Ships[i].warp >= 0.0 && Ships[i].dhead != Ships[i].head )
	    {
                h = Ships[i].head;
                ad = utSubAngle( h, Ships[i].dhead );
                x = (real)max( 210.0 - ((Ships[i].warp*20.0)/engeff( i )), 2.0 ) * (real)ITER_SECONDS;
                if ( fabs( ad ) <= x )
                    Ships[i].head = Ships[i].dhead;
                else if ( ad < 0.0 )
		{
                    z = h + x;

                    if (z > 360.0)
                        z = fabs(z) - 360.0;
                    Ships[i].head = (real) z;
		}
                else
		{
                    z = h - x;
                    if (z < 0.0)
                        z = 360.0 - fabs(z);
                    Ships[i].head = (real) z;
		}

                clbFixDeltas( i );
	    }

            /* Movement. */
            j = Ships[i].towedby;
            if ( j != 0 )
	    {
                /* Being towed; slowly (grossly?) align with our tower. */
                Ships[i].warp = 0.0;
                Ships[i].dwarp = 0.0;
                h = utAngle( Ships[i].x, Ships[i].y, Ships[j].x, Ships[j].y );
                ad = utSubAngle( h, Ships[j].head );
                if ( ad < 0.0 )
                    h = h - max( ad, -10.0*ITER_SECONDS );
                else
                    h = h - min( ad, +10.0*ITER_SECONDS );

                Ships[i].x = Ships[j].x + TOW_DIST * cosd(h+180.0);
                Ships[i].y = Ships[j].y + TOW_DIST * sind(h+180.0);
	    }
            else if ( Ships[i].warp >= 0.0 )
	    {
                /* Cruising. */
                x = min( Ships[i].dwarp, maxwarp( i ) );
                if ( Ships[i].warp != x )
		{
                    Ships[i].warp = clbNewWarp( i, x );
                    clbFixDeltas( i );
		}

                Ships[i].x = Ships[i].x + Ships[i].dx;
                Ships[i].y = Ships[i].y + Ships[i].dy;

                /* If we're locked onto a planet but not orbiting it see if */
                /* we are close enough to orbit. */
                j = -Ships[i].lock;
                if ( j > 0)
		{
                    /* Make sure the planet is still real. */
                    if ( ! PVISIBLE(j) )
                        Ships[i].lock = 0;
                    else if ( Ships[i].warp >= 0.0 )
		    {
                        /* Still moving; if we're going slow enough to orbit, */
                        /*  check if we're close enough to do so. Otherwise, */
                        /*  check to see if it's time to slow down yet. */
                        dis = dist( Ships[i].x, Ships[i].y, Planets[j].x, Planets[j].y );
                        if ( Ships[i].warp <= MAX_ORBIT_WARP )
			{
                            /* Going slow enough to orbit. */
                            if ( dis <= ORBIT_DIST )
			    {
                                /* Close enough to orbit. */
                                clbOrbit( i, j );
                                sprintf( buf,
                                         "Coming into orbit around %s.",
                                         Planets[j].name );
                                clbStoreMsgf( MSG_COMP, i, buf, MSG_FLAGS_TERSABLE );
			    }
			}
                        else if ( ( dis - ORBIT_DIST ) <=
                                  utAccurateDist( Ships[i].warp, MAX_ORBIT_WARP,
                                                  ShipTypes[Ships[i].shiptype].accelfac *
                                                  engeff( i ) ) )
			{
                            /* Time to slow down. */
                            if ( Ships[i].dwarp > MAX_ORBIT_WARP )
			    {
                                Ships[i].dwarp = MAX_ORBIT_WARP;
                                /* "WARNING, WILL ROBINSON!!" */
                                sprintf( buf,
                                         "Approaching %s - commencing orbital insertion maneuver.",
                                         Planets[j].name );
                                clbStoreMsgf( MSG_COMP, i, buf, MSG_FLAGS_TERSABLE );
			    }
			}
		    }
		}
	    }
	}
    }

    /* Drive the torps. */
    clbTorpDrive(ITER_SECONDS);
    for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
        i = ship[s];
        if ( Ships[i].status != SS_OFF )
	{
            for ( j = 0; j < MAXTORPS; j = j + 1 )
	    {
                if ( Ships[i].torps[j].status == TS_DETONATE )
		{
                    /* Detonate. */
                    Ships[i].torps[j].fuse = FIREBALL_FUSE;
                    Ships[i].torps[j].status = TS_FIREBALL;
                    for ( t = 1; t <= MAXSHIPS; t = t + 1 )
		    {
                        k = ship[t];

                        if ( Ships[k].status == SS_LIVE && k != i )
                            if ( Ships[i].torps[j].war[Ships[k].team] ||
                                 Ships[k].war[Ships[i].team] )
                            {
                                ht = utExplosionHits( (real)(TORPEDO_HIT *
                                                             Ships[i].torps[j].mult),
                                                      dist( Ships[i].torps[j].x,
                                                            Ships[i].torps[j].y,
                                                            Ships[k].x, Ships[k].y ) );
                                if ( ht > 0.0 )
                                    clbHit( k, ht, i );
                            }
		    }
		}
	    }
	}
    }

    /* Drive the planet eater. */
    if ( Doomsday->status == DS_LIVE )
    {

        /* We want to creep up on our target, without sitting and spinning
         * around on top of it, so only move when we are too far away
         * from our target to inflict damage.
         */

        /* planet lock */
        if (Doomsday->lock < 0 && (distf( Doomsday->x, Doomsday->y,
                                          Planets[-Doomsday->lock].x,
                                          Planets[-Doomsday->lock].y )
                                   >= DOOMSDAY_DIST) )
        {
            Doomsday->x = Doomsday->x + Doomsday->dx;
            Doomsday->y = Doomsday->y + Doomsday->dy;
        }

        /* ship lock */
        if (Doomsday->lock > 0 && (distf( Doomsday->x, Doomsday->y,
                                          Ships[Doomsday->lock].x,
                                          Ships[Doomsday->lock].y )
                                   >= DOOMSDAY_DIST) )
        {
            Doomsday->x = Doomsday->x + Doomsday->dx;
            Doomsday->y = Doomsday->y + Doomsday->dy;
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
    int talert[MAXSHIPS + 1];

    for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
        i = ship[s];
        if ( Ships[i].status == SS_OFF )
            continue; /*next;*/

        if ( Ships[i].status != SS_LIVE )
	{
            /* Turn off timed out ships. */
            if ( Ships[i].sdfuse < 0 )
	    {
                /* This code may be too safe... */
                PVLOCK(&ConqInfo->lockword);
                if ( Ships[i].status != SS_LIVE && Ships[i].sdfuse < 0 )
		{
                    Ships[i].sdfuse = Ships[i].sdfuse + 1;
                    if ( Ships[i].sdfuse == 0 )
		    {
                        Ships[i].sdfuse = 0;
                        Ships[i].killedby = 0;
                        Ships[i].status = SS_OFF;
		    }
		}
                PVUNLOCK(&ConqInfo->lockword);
                continue; /* next;*/
	    }
	}

        /* The ship is alive */

        /* see if we've been kicked out. */
        if ( Users[Ships[i].unum].ooptions[OOPT_SHITLIST] )
	{
            clbKillShip( i, KB_SHIT );
            continue; /* next;*/
	}
        if ( ConqInfo->closed )
            if ( ! Users[Ships[i].unum].ooptions[OOPT_PLAYWHENCLOSED] )
            {
                clbKillShip( i, KB_EVICT );
                continue; /*next;*/
            }

        /* The ship is still alive. */
        if ( SROBOT(i) )
            if ( ! ConqInfo->externrobots )
                robotai( i );

        /* Ships - Teams. */
        for ( j = 0; j <  NUMPLAYERTEAMS; j = j + 1 )
            if ( Ships[i].scanned[j] > 0 )
                Ships[i].scanned[j] = Ships[i].scanned[j] - 1;

        /* Ships, planets and suns scans. */
        for ( j = 1; j <= NUMPLANETS; j = j + 1 )
            if ( PVISIBLE(j) )
            {
                /* Do we scan the planet? */
                dis = dist( Ships[i].x, Ships[i].y, Planets[j].x, Planets[j].y );
                if ( dis <= PLANET_DIST )
                {
                    k = Ships[i].team;
                    if ( k >= 0 && k < NUMPLAYERTEAMS )
                        Planets[j].scanned[k] = TRUE;

                    /* Planet armies (and suns) get to do damage here. */
                    if ( clbSPWar( i,j ) )
                        clbHit( i,
                                rndnor( PLANET_HIT + Planets[j].armies * ARMY_HIT, 1.0 ),
                                -j );
                }

                /* Does the planet scan us? */
                if ( j <= NUMPLANETS )
                    if ( dis <= ACCINFO_DIST )
                        if ( ! SCLOAKED(i) )
                        {
                            k = Planets[j].team;
                            if ( k >= 0 && k <  NUMPLAYERTEAMS )
                                Ships[i].scanned[k] = SCANNED_FUSE;
                        }
            }

        /* Planet eater. */
        if ( Doomsday->status == DS_LIVE )
            if ( dist( Ships[i].x, Ships[i].y, Doomsday->x, Doomsday->y ) <= DOOMSDAY_DIST )
            {
                clbHit( i, rndnor( DOOMSDAY_HIT, 1.0 ), KB_DOOMSDAY );
            }


        /* Negative energy barrier. */
        if ( fabs( Ships[i].x ) >= NEGENB_DIST || fabs(Ships[i].y) >= NEGENB_DIST )
            if ( fabs( Ships[i].x ) <= NEGENBEND_DIST &&
                 fabs( Ships[i].y ) <= NEGENBEND_DIST )
                clbHit( i, NEGENB_HIT, KB_NEGENB );

        /* Shields. */
        if ( Ships[i].shields < 100.0 )
	{
            /* Shields repair twice as fast when they're down. */
            x = SHIELD_FAC;
            if ( ! SSHUP(i) )
                x = x * 2.0;
            Ships[i].shields = min( 100.0, Ships[i].shields + x );
	}

        /* Repair. */
        repair = REPAIR_FAC;
        if ( SREPAIR(i) )
	{
            SFCLR(i, SHIP_F_CLOAKED);
            if ( Ships[i].warp >= 0.0 )
                Ships[i].dwarp = 0.0;
            repair = repair * RMODE_REPAIR_MULT;
	}
        if ( Ships[i].warp < 0.0 )                        /* orbiting */
            if ( ! clbSPWar( i,-Ships[i].lock ) )              /* a friendly */
                if ( Planets[-Ships[i].lock].armies > 0 )	/* populated planet */
                    repair = repair * PLANET_REPAIR_MULT;
        Ships[i].damage = Ships[i].damage - repair;
        if ( Ships[i].damage < 0.0 )
	{
            Ships[i].damage = 0.0;
            if ( SREPAIR(i) )
                SFCLR(i, SHIP_F_REPAIR);
	}

        /* Weapons/engines down fuses. */
        if ( Ships[i].wfuse > 0 )
	{
            Ships[i].wfuse = Ships[i].wfuse - 1;
            if ( Ships[i].wfuse <= 0 )
                clbStoreMsgf( MSG_COMP, i, "Weapons are back on-line.",
                              MSG_FLAGS_TERSABLE);
	}
        if ( Ships[i].efuse > 0 )
	{
            Ships[i].efuse = Ships[i].efuse - 1;
            Ships[i].dwarp = 0.0;
            if ( Ships[i].efuse <= 0 )
                clbStoreMsgf( MSG_COMP, i, "Engine power has been restored.",
                              MSG_FLAGS_TERSABLE);
	}

        /* Fuel. */
        inc = FUEL_FAC;
        dec = 0.0;
        if ( Ships[i].warp < 0.0 )
	{
            /* You get fuel for orbiting friendly, populated class M, */
            /*  with shields down. */
            if ( ! SSHUP(i) || SREPAIR(i) )
                if ( Planets[-Ships[i].lock].type == PLANET_CLASSM )
                    if ( ! clbSPWar( i,-Ships[i].lock ) )
                        if ( Planets[-Ships[i].lock].armies > 0 )
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
                dec = dec + Ships[i].warp * FUELWARP_FAC * FUELSHIELDS_MULT;
            else
                dec = dec + Ships[i].warp * FUELWARP_FAC;

            dec = dec + Ships[i].warp * Ships[i].armies * FUELWARPARMY_FAC;

            if ( Ships[i].towing != 0 )
	    {
                if ( SSHUP(Ships[i].towing) )
                    dec = dec + Ships[i].warp * FUELWARP_FAC * FUELSHIELDS_MULT;
                else
                    dec = dec + Ships[i].warp * FUELWARP_FAC;
                dec = dec + Ships[i].warp * Ships[Ships[i].towing].armies *
                    FUELWARPARMY_FAC;
	    }
	}
        /* Cloaking. */
        if ( SCLOAKED(i) )
	{
            SFCLR(i, SHIP_F_REPAIR);
            dec = dec + CLOAK_FUEL;
            if ( Ships[i].warp > 0.0 )
                dec = dec + Ships[i].dwarp * CLOAK_WARP_FUEL;
	}
        Ships[i].fuel = min( 999.0, Ships[i].fuel + inc );
        if ( dec > 0.0 )
            clbUseFuel( i, dec, FALSE, TRUE );

        /* Cool-down. */
        if ( Ships[i].warp < 0.0 && !clbSPWar( i,-Ships[i].lock)
             &&  Planets[-Ships[i].lock].armies > 0)
	{			    /* orbiting a friendly populated planet */
            Ships[i].wtemp = max( 0.0, Ships[i].wtemp - (WEAPON_COOL_FAC * PLANET_REPAIR_MULT));
            Ships[i].etemp = max( 0.0, Ships[i].etemp - (ENGINE_COOL_FAC * PLANET_REPAIR_MULT));
	}
        else
	{
            Ships[i].wtemp = max( 0.0, Ships[i].wtemp - WEAPON_COOL_FAC );
            Ships[i].etemp = max( 0.0, Ships[i].etemp - ENGINE_COOL_FAC );
	}
    }

    /* Torp alert logic. */
    for ( i = 1; i <= MAXSHIPS; i = i + 1 )
        talert[i] = FALSE;
    for ( s = 1; s <= MAXSHIPS; s = s + 1 )
    {
        i = ship[s];
        if ( Ships[i].status != SS_OFF )
	{
            /* Torpedoes. */
            for ( j = 0; j < MAXTORPS; j = j + 1 )
                if ( Ships[i].torps[j].status != TS_OFF )
                {
                    /* Torpedo fuses. */
                    Ships[i].torps[j].fuse = Ships[i].torps[j].fuse - 1;
                    if ( Ships[i].torps[j].fuse <= 0 )
                    {
                        if ( Ships[i].torps[j].status == TS_LIVE )
                            clbDetonate( i, j );
                        else if ( Ships[i].torps[j].status == TS_FIREBALL )
                            Ships[i].torps[j].status = TS_OFF;
                    }
                    else
                    {
                        if ( Ships[i].torps[j].status == TS_LIVE )
                        {
                            /* Proximity check. */
                            for ( t = 1; t <= MAXSHIPS; t = t + 1 )
                            {
                                k = ship[t];
                                if ( Ships[k].status == SS_LIVE && k != i )
                                    if ( Ships[i].torps[j].war[Ships[k].team] || Ships[k].war[Ships[i].team] )
                                    {
                                        dis = distf( Ships[i].torps[j].x, Ships[i].torps[j].y,
                                                     Ships[k].x, Ships[k].y );
                                        if ( dis <= TORPEDO_PROX )
                                        {
                                            clbDetonate( i, j );
                                            break;
                                        }
                                        else if ( dis <= ALERT_DIST )
                                            talert[k] = TRUE;
                                    }
                            }
                        }
                        if ( Ships[i].torps[j].status == TS_LIVE )
                        {
                            /* Proximity check for the doomsday machine. */
                            if ( Doomsday->status == DS_LIVE )
                                if ( distf( Ships[i].torps[j].x, Ships[i].torps[j].y, Doomsday->x, Doomsday->y ) <=
                                     (TORPEDO_PROX * 3.0))
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
    for ( i = 1; i <= MAXSHIPS; i = i + 1 )
        if (talert[i])
            SFSET(i, SHIP_F_TALERT);
        else
            SFCLR(i, SHIP_F_TALERT);

    /* Planet eater. */
    if ( Doomsday->status == DS_LIVE )
    {
        if ( Doomsday->lock < 0 )
	{
            /* Planet. */
            if ( distf( Doomsday->x, Doomsday->y, Planets[-Doomsday->lock].x, Planets[-Doomsday->lock].y ) <= DOOMSDAY_DIST )
	    {
                /* Decrement armies. */
                if ( rnd() <= 0.1 )
                    clbIntrude( MSG_DOOM, -Doomsday->lock );
                PVLOCK(&ConqInfo->lockword);
                Planets[-Doomsday->lock].armies = Planets[-Doomsday->lock].armies - 1;
                if ( Planets[-Doomsday->lock].armies <= 0 )
		{
                    Planets[-Doomsday->lock].uninhabtime = rndint( MIN_UNINHAB_MINUTES,
                                                                   MAX_UNINHAB_MINUTES );
                    clbZeroPlanet( -Doomsday->lock, 0 );
                    clbDoomFind();
		}
                PVUNLOCK(&ConqInfo->lockword);
	    }
	}
        else if ( Doomsday->lock > 0 )
	{
            /* Ship. */
            if ( Ships[Doomsday->lock].status != SS_LIVE )
                clbDoomFind();
            else if ( distf( Doomsday->x, Doomsday->y, Ships[Doomsday->lock].x, Ships[Doomsday->lock].y ) <= DOOMSDAY_DIST )
                Ships[Doomsday->lock].warp = 0.0;	/* clever doomsday tractors */
	}

        /* Update heading. */
        if ( Doomsday->lock < 0 )
            Doomsday->heading = utAngle( Doomsday->x, Doomsday->y, Planets[-Doomsday->lock].x, Planets[-Doomsday->lock].y );
        else if ( Doomsday->lock > 0 )
            Doomsday->heading = utAngle( Doomsday->x, Doomsday->y, Ships[Doomsday->lock].x, Ships[Doomsday->lock].y );
        Doomsday->dx = DOOMSDAY_WARP * MM_PER_SEC_PER_WARP * ITER_SECONDS * cosd(Doomsday->heading);
        Doomsday->dy = DOOMSDAY_WARP * MM_PER_SEC_PER_WARP * ITER_SECONDS * sind(Doomsday->heading);
    }

    return;

}

/*  mindrive - drive the one-minute interval items */
/*  SYNOPSIS */
/*    mindrive */
void mindrive(void)
{

    int i;

    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
        /* Decrement couptime(). */
        if ( Teams[i].couptime > 0 )
            Teams[i].couptime = Teams[i].couptime - 1;
    }

    /* cleanup any unliving ships */
    clbCheckShips(TRUE);

    for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    {
        /* Decrement puninhabtime(). */
        if ( Planets[i].uninhabtime > 0 )
            Planets[i].uninhabtime = Planets[i].uninhabtime - 1;
    }


    if ( Doomsday->status == DS_LIVE )
        clbDoomFind();
    else if ( rnd() < DOOMSDAY_PROB )
    {
        if (SysConf.NoDoomsday == FALSE)
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
    PVLOCK(&ConqInfo->lockword);
    for ( i = 1; i <= NUMPLANETS; i = i + 1 )
        if (Planets[i].type != PLANET_SUN)
        {
            if ( Planets[i].armies > 0 && Planets[i].team != TEAM_GOD )
            {
                if ( Planets[i].armies < SPARSE_THRESH )
                {
                    if ( rnd() <= SPARSE_REPOP_PROB )
                        Planets[i].armies = Planets[i].armies + 1;
                }
                else
                {
                    if ( Planets[i].type == PLANET_CLASSM )
                        thresh = MALTHUS_M_THRESH;
                    else
                        thresh = MALTHUS_D_THRESH;

                    if ( Planets[i].armies >= thresh && rnd() <= MALTHUS_PROB )
                    {
                        r = rnd();	/* save to avoid the unfortunate side effects of
                                           max() bieng a macro (rnd() actually got exc'd
                                           twice, resluting in an occasional 0 return val) */

                        Planets[i].armies = max( round( Planets[i].armies * r ), 1 );
                    }
                    else
                    {
                        Planets[i].armies = Planets[i].armies +
                            rndint( REPOP_LOWER_BOUND, REPOP_UPPER_BOUND );
                    }
                }
            }
        }
    PVUNLOCK(&ConqInfo->lockword);

    return;

}

void SigTerminate(int sig)
{
    Driver->drivstat = DRS_KAMIKAZE;

    utLog("conqdriv: Terminating on signal %d", sig);

    return;
}
