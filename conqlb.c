#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                                 C O N Q L B */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conqlb.h"
#include "conqutil.h"
#include "conqunix.h"
#define NOEXTERN_CONQDATA
#include "conqdata.h"
#include "conqai.h"
#include "rndlb.h"

#include "conf.h"
#include "global.h"
#include "user.h"
#include "color.h"
#include "conqinit.h"


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
    Users[unum].stats[USTAT_WINS] += (int)Ships[snum].kills;
    Teams[team].stats[TSTAT_WINS] = Teams[team].stats[TSTAT_WINS]
        + (int)Ships[snum].kills;

    /* Update max kills. */
    i = (int)Ships[snum].kills;
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
void clbDamage( int snum, real dam, killedBy_t kb, uint16_t detail )
{
    real mw;

    Ships[snum].damage = Ships[snum].damage + dam;
    if ( Ships[snum].damage >= 100.0 )
        clbKillShip( snum, kb, detail );
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

    for ( i = 0; i < MAXSHIPS; i++ )
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
void clbHit( int snum, real ht, killedBy_t kb, uint16_t detail )
{
    if ( ht > 0.0 )
    {
        if ( SSHUP(snum) && ! SREPAIR(snum) )
	{
            if ( ht > Ships[snum].shields )
	    {
                clbDamage( snum, ht-Ships[snum].shields, kb, detail );
                Ships[snum].shields = 0.0;
	    }
            else
	    {
                Ships[snum].shields = Ships[snum].shields - ht;
	    }
	}
        else
	{
            clbDamage( snum, ht, kb, detail );
	}
    }

    return;
}


/*  kill a ship */
/*  Note: This routines ASSUMES you have the common locked before you use it. */
void clbIKill(int snum, killedBy_t kb, uint16_t detail)
{
    int i, unum, team, kunum, kteam;
    real tkills;

    /* Only procede if the ship is alive */
    if ( Ships[snum].status != SS_LIVE )
        return;

    /* The ship is alive; kill it. */
    Ships[snum].killedBy = kb;
    Ships[snum].killedByDetail = detail;
    Ships[snum].status = SS_DYING;

    unum = Ships[snum].unum;
    team = Ships[snum].team;

    /* Detonate all torpedos. */

    for ( i = 0; i < MAXTORPS; i++ )
        if ( Ships[snum].torps[i].status == TS_LIVE )
            Ships[snum].torps[i].status = TS_DETONATE;

    /* Release any tows. */
    if ( Ships[snum].towing != 0 )
        Ships[Ships[snum].towing].towedby = 0;
    if ( Ships[snum].towedby != 0 )
        Ships[Ships[snum].towedby].towing = 0;

    /* Zero team scan fuses. */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        Ships[snum].scanned[i] = 0;

    if ( kb == KB_CONQUER )
        Ships[snum].kills = Ships[snum].kills + CONQUER_KILLS;
    else if ( kb == KB_GOTDOOMSDAY )
        Ships[snum].kills = Ships[snum].kills + DOOMSDAY_KILLS;
    else if ( kb == KB_SHIP && detail < MAXSHIPS ) // if a ship did the killing
    {
        kunum = Ships[detail].unum;
        kteam = Ships[detail].team;
        tkills = 1.0 + ((Ships[snum].kills + Ships[snum].strkills) * KILLS_KILLS);
        if ( Ships[snum].armies > 0 )
	{
            /* Keep track of carried armies killed - they are special. */
            tkills = tkills + Ships[snum].armies * ARMY_KILLS;
            Users[kunum].stats[USTAT_ARMSHIP] += Ships[snum].armies;
            Teams[kteam].stats[TSTAT_ARMSHIP] += Ships[snum].armies;
	}

        /* Kills accounting. */
        if ( Ships[detail].status == SS_LIVE )
            Ships[detail].kills = Ships[detail].kills + tkills;
        else
	{
            /* Have to do some hacking when our killer is dead. */
            Users[kunum].stats[USTAT_WINS] -= (int)Ships[detail].kills;
            Teams[kteam].stats[TSTAT_WINS] =
                Teams[kteam].stats[TSTAT_WINS] - (int)Ships[detail].kills;
            Ships[detail].kills = Ships[detail].kills + tkills;
            clbChalkup( detail );
	}

        /* Sticky war logic. */
        /* should set sticky war too. -JET */

        if ( ! Ships[detail].war[kteam] )
	{
            Ships[detail].war[team] = TRUE;
            Ships[detail].rwar[team] = TRUE;
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

    /* set the ship dead-like :) */
    Ships[snum].status = SS_DEAD;

    /* setup dead timeout timer. For robots and vacant ships, that's 5
     *  seconds - this gives enough time for detonating torps, just like
     *  regular players
     */
    if (SROBOT(snum) || SVACANT(snum))
        Ships[snum].sdfuse = -5;
    else
        Ships[snum].sdfuse = -TIMEOUT_PLAYER;

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
void clbKillShip(int snum, killedBy_t kb, uint16_t detail)
{
    int sendmesg = FALSE;
    char msgbuf[BUFFER_SIZE_256];

#if defined(DO_EXPLODING_SHIPS)
    /* launch all torps - sorta, we'll use 'explode' mode... */
    clbLaunch(snum, 0.0, EXPLODESHIP_TORP_COUNT, LAUNCH_EXPLODE);
#endif

    /* internal routine. */
    PVLOCK(&ConqInfo->lockword);
    clbIKill( snum, kb, detail );
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
    case KB_LIGHTNING:
        sprintf(msgbuf, "%c%d (%s) was destroyed by a lightning bolt.",
                Teams[Ships[snum].team].teamchar,
                snum,
                Ships[snum].alias);
        sendmesg = TRUE;

        break;

    case KB_SHIP:
        if (detail >= 0 && detail < MAXSHIPS)
	{
            sprintf(msgbuf, "%c%d (%s) was kill %.1f for %c%d (%s).",
                    Teams[Ships[snum].team].teamchar,
                    snum,
                    Ships[snum].alias,
                    Ships[detail].kills,
                    Teams[Ships[detail].team].teamchar,
                    detail,
                    Ships[detail].alias);
            sendmesg = TRUE;

	}
        break;

    case KB_PLANET:
        if (detail >= 0 && detail < MAXPLANETS)
	{
            sprintf(msgbuf, "%c%d (%s) was destroyed by %s",
                    Teams[Ships[snum].team].teamchar,
                    snum,
                    Ships[snum].alias,
                    Planets[detail].name);

            sendmesg = TRUE;

            if ( Planets[detail].type == PLANET_SUN )
	    {
                strcat(msgbuf , "'s solar radiation.") ;
	    }
            else
	    {
                strcat(msgbuf , "'s planetary defenses.") ;
	    }
	}
        break;

    default:
        utLog("%s: Invalid killedby: snum %d, kb %d, detail %d", __FUNCTION__,
              snum, (int)kb, (int)detail);
    }

    if (sendmesg == TRUE)
        clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, msgbuf);

    return;

}

/* see if we could launch some if we wanted too... */
/* does NO LOCKING, so only use from the client */
int clbCheckLaunch(int snum, int number)
{
    int i;

    if (Ships[snum].wfuse > 0)
        return FALSE;               /* weapons overloaded */

    if (number == 0)
        return TRUE;

    for ( i = 0; i < MAXTORPS; i++ )
        if ( Ships[snum].torps[i].status == TS_OFF )
            return TRUE;

    return FALSE;
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

    Ships[snum].lock = LOCK_PLANET;
    Ships[snum].lockDetail = pnum;
    Ships[snum].dwarp = 0.0;

    /* Find bearing to planet. */
    beer = utAngle( Ships[snum].x, Ships[snum].y,
                    Planets[pnum].x, Planets[pnum].y );
    if ( Ships[snum].head < ( beer - 180.0 ) )
        beer = beer - 360.0;

    /* Check beer head to determine orbit direction. */
    if ( beer <= Ships[snum].head )
    {
        Ships[snum].warp = ORBIT_CW;
        Ships[snum].head = utMod360( beer + 90.0 );
    }
    else
    {
        Ships[snum].warp = ORBIT_CCW;
        Ships[snum].head = utMod360( beer - 90.0 );
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
    for ( k = 0; k < MAXSHIPS; k++ )
        if ( Ships[k].status == SS_LIVE && k != snum )
            if ( satwar(snum, k ) )
            {
                dis = dist( Ships[snum].x, Ships[snum].y, Ships[k].x, Ships[k].y );
                if ( dis <= PHASER_DIST )
                {
                    ang = utAngle( Ships[snum].x, Ships[snum].y, Ships[k].x, Ships[k].y );
                    if ( fabs( dir - ang ) <= PHASER_SPREAD )
                    {
                        clbHit( k, clbPhaserHit( snum, dis ),
                                KB_SHIP, (uint16_t)snum );
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
    for ( i = 0; i < MAXUSERS; i++ )
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
            utStcpn( lname, Users[i].username, MAXUSERNAME );
            utStcpn( rname, Users[i].alias, MAXUSERPNAME );
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

        for ( i = 0; i < MAXHISTLOG; i++ )
            if ( unum == History[i].histunum )
                History[i].histunum = -1;
    }
    PVUNLOCK(&ConqInfo->lockword);

    if (isoper != TRUE)
        utLog("INFO: %s (%s) has resigned",
              usrname, usralias);

    return;
}




/*  takeplanet - take a planet (DOES SPECIAL LOCKING)
 *  SYNOPSIS
 *    int pnum, snum
 *    clbTakePlanet( pnum, snum )
 *  Note: This routines ASSUMES you have the common locked before you call
 *  it. Returns team that was genocided, -1 otherwise.
 */
int clbTakePlanet( int pnum, int snum )
{
    int i;
    char buf[MSGMAXLINE];
    int oteam, didgeno;
    int rv = -1;

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

        for ( i = 0; i < MAXPLANETS; i++ )
        {
            if ( PVISIBLE(i) && (Planets[i].team == oteam)
                 && Planets[i].armies > 0)
            {
                didgeno = 0;
                break;
            }
        }
        /* Yes. */
        if ( didgeno && (snum >= 0 && snum < MAXSHIPS) )
        {
            rv = oteam;
            Users[Ships[snum].unum].stats[USTAT_GENOCIDE] += 1;
            Teams[Ships[snum].team].stats[TSTAT_GENOCIDE] += 1;

            sprintf(buf, "%c%d (%s) genocided the %s team!",
                    Teams[Ships[snum].team].teamchar,
                    snum,
                    Ships[snum].alias,
                    Teams[oteam].name);

            clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, buf);
            utLog(buf);
        }

    }


    sprintf( buf, "All hail the liberating %s armies.  Thanks, ",
             Teams[Ships[snum].team].name );
    utAppendShip(buf , snum) ;
    utAppendChar(buf , '!') ;

    /* Check whether the universe has been conquered. */
    for ( i = 0; i < NUMCONPLANETS; i++ )
        if ( Planets[i].type == PLANET_CLASSM || Planets[i].type == PLANET_DEAD )
            if ( Planets[i].team != Ships[snum].team || ! PVISIBLE(i) )
            {
                /* No. */
                clbStoreMsg( MSG_FROM_PLANET, pnum,
                             MSG_TO_TEAM, Ships[snum].team, buf );
                return rv;
            }
    /* Yes! */
    utFormatTime( ConqInfo->conqtime, 0 );
    utStcpn( Ships[snum].alias, ConqInfo->conqueror, MAXUSERPNAME );
    ConqInfo->lastwords[0] = 0;
    Users[Ships[snum].unum].stats[USTAT_CONQUERS] += 1;
    Teams[Ships[snum].team].stats[TSTAT_CONQUERS] += 1;
    utStcpn( Teams[Ships[snum].team].name, ConqInfo->conqteam, MAXTEAMNAME );

    utLog("INFO: %s (%s) has Conquered the Universe!",
          Users[Ships[snum].unum].username,
          Ships[snum].alias);

    clbIKill( snum, KB_CONQUER, 0 );
    for ( i = 0; i < MAXSHIPS; i++ )
        if ( Ships[i].status == SS_LIVE )
            clbIKill( i, KB_NEWGAME, 0 );

    PVUNLOCK(&ConqInfo->lockword);
    clbInitGame();
    PVLOCK(&ConqInfo->lockword);

    return -1;                    /* doesn't matter if geno happened if
                                     universe was conquered */

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
        strcpy(buf , hd1) ;
        return;
    }
    if ( ! Users[unum].live )
    {
        buf[0] = 0;
        return;
    }

    ch2 = ' ';

    if ( ch2 != '+' && isagod(unum))
        ch2 = '+';

    /* If we were given a valid ship number, use it's information. */
    if ( snum >= 0 && snum < MAXSHIPS )
    {
        strcpy(name , Ships[snum].alias) ;
        team = Ships[snum].team;
    }
    else
    {

        strcpy(name , Users[unum].alias) ;
        team = Users[unum].team;
    }

    /* Figure out which team he's on. */
    if ( Users[unum].ooptions[OOPT_MULTIPLE] && ! showteam )
        ch = 'M';
    else
        ch = Teams[team].teamchar;

    strncpy(tname, Users[unum].username, MAXUSERNAME - 1);
    tname[MAXUSERNAME - 1] = 0;

    sprintf( junk, "%-12.12s %c%-21.21s %c %6.1f",
             tname,
             ch2,
             name,
             ch,
             Users[unum].rating );

    utFormatMinutes( ( Users[unum].stats[USTAT_SECONDS] + 30 ) / 60, timstr );

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
        buf[0] = 0;
        return;
    }
    if ( ! Users[unum].live )
    {
        buf[0] = 0;
        return;
    }

    if ( Users[unum].stats[USTAT_SECONDS] == 0 )
        strcpy(percent , "- ") ;
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
    ch = 0;
    for ( i = 9; buf[i] != 0; i++ )
    {
        if ( buf[i] == '0' )
            if ( ch == ' ' )
                if ( buf[i+1] == ' ' || buf[i+1] == 0 )
                    buf[i] = '-';
        ch = buf[i];
    }

    if (Users[unum].lastentry == 0) /* never */
    {
        sprintf(junk, " %13.13s", "never");
        strcat(buf , junk) ;
    }
    else
    {				/* format it properly */
        utFormatTime(datestr, Users[unum].lastentry);

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
        morejunk[j] = 0;

        strcat(buf , morejunk) ;
    }

    return;
}


/*  clbZeroPlanet - zero a planet (DOES SPECIAL LOCKING)
 *  SYNOPSIS
 *    int pnum, snum
 *    clbZeroPlanet( pnum, snum )
 *  NOTE
 *    This routines ASSUMES you have the common area locked before you call
 *    it.  Returns team that was genocided, -1 otherwise.
 */
int clbZeroPlanet( int pnum, int snum )
{
    int oteam, i;
    int didgeno = FALSE;
    char buf[MSGMAXLINE];

    oteam = Planets[pnum].team;
    Planets[pnum].team = TEAM_NOTEAM;
    Planets[pnum].armies = 0;

    /* Make the planet not scanned. */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        Planets[pnum].scanned[i] = FALSE;

    /* check for genos here */
    if ( oteam != TEAM_SELFRULED && oteam != TEAM_NOTEAM )
    {
        didgeno = TRUE;

        /* Check whether that was the last planet owned by the vanquished. */
        for ( i = 0; i < MAXPLANETS; i++ )
        {
            if ( PVISIBLE(i) && (Planets[i].team == oteam) &&
                 Planets[i].armies > 0)
            {
                didgeno = FALSE;
                break;
            }
        }

        /* Yes. */
        if (didgeno && (snum >= 0 && snum < MAXSHIPS))
        {
            Teams[oteam].couptime = rndint( MIN_COUP_MINUTES, MAX_COUP_MINUTES );
            Teams[oteam].coupinfo = FALSE;		/* lost coup info */


            if ( snum >= 0 && snum < MAXSHIPS )
            {
                Users[Ships[snum].unum].stats[USTAT_GENOCIDE] += 1;
                Teams[Ships[snum].team].stats[TSTAT_GENOCIDE] += 1;

                sprintf(buf, "%c%d (%s) genocided the %s team!",
                        Teams[Ships[snum].team].teamchar,
                        snum,
                        Ships[snum].alias,
                        Teams[oteam].name);

                clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, buf);
                utLog(buf);
            }
        }
    }

    if (didgeno)
        return oteam;
    else
        return -1;

}

char *clbWarPrompt(int snum, int twar[])
{
    static char wbuf[BUFFER_SIZE_256];
    static char *fmt = "Press [TAB] when done, [ESC] to abort:  Peace: %c %c %c %c  War: %c %c %c %c";
    int i;
    char ch, peace[NUMPLAYERTEAMS], war[NUMPLAYERTEAMS];

    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
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
    msgFrom_t from = Msgs[msgnum].from;
    uint16_t fromDetail = Msgs[msgnum].fromDetail;
    msgTo_t to = Msgs[msgnum].to;
    uint16_t toDetail = Msgs[msgnum].toDetail;

    if (from == MSG_FROM_NOONE && to == MSG_TO_NOONE)
    {				/* uninitialized msgs */
        return(FALSE);		/* no point in reading it */
    }

    /* If we're GOD, we can read it. unless it's a COMP MSG*/
    if (snum < 0)
    {
        if (from != MSG_FROM_COMP)
        {
            return ( TRUE );
        }
        else
            return FALSE;
    }

    // snum should never be <0 at this point

    /* It's to us. */
    if ( to == MSG_TO_SHIP && (int)toDetail == snum )
        return(TRUE);

    /* It's from us */
    if (from == MSG_FROM_SHIP && (int)fromDetail == snum)
        return(TRUE);

    /* if it's to god, or implementors and we are an oper... */
    if ((to == MSG_TO_GOD || to == MSG_TO_IMPLEMENTORS)
        && Users[Ships[snum].unum].ooptions[OOPT_OPER])
        return TRUE;

    /* It's to everybody. */
    if ( to == MSG_TO_ALL )
        return(TRUE);

    /* Only check these if we're a ship. */
    if ( snum >= 0 && snum < MAXSHIPS )
    {
        /* We can only read team messages if we're not self-war. */
        if ( to == MSG_TO_TEAM
             && toDetail < NUMALLTEAMS
             && ((int)toDetail == Ships[snum].team)
             && !selfwar(snum) )
            return ( TRUE );

        /* see if it's a message to friendly ships from another ship */

        if (to == MSG_TO_FRIENDLY
            && from == MSG_FROM_SHIP
            && fromDetail >= 0
            && fromDetail < MAXSHIPS)
	{
            if (Ships[snum].war[Ships[fromDetail].team] == FALSE &&
                Ships[fromDetail].war[Ships[snum].team] == FALSE)
                return TRUE;
	}
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

    for ( i = 0; i < MAXSHIPS; i++ )
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
    Doomsday->lock = LOCK_NONE;
    Doomsday->lockDetail = 0;

    for ( i = 0; i < MAXPLANETS; i++ )
        if ( PVISIBLE(i) )
            if ( Planets[i].armies > 0 && Planets[i].team != TEAM_NOTEAM )
            {
                taste = Planets[i].armies * BOMBARD_KILLS / dist(Doomsday->x, Doomsday->y, Planets[i].x, Planets[i].y);
                if ( taste > tastiness )
                {
                    tastiness = taste;
                    Doomsday->lock = LOCK_PLANET;
                    Doomsday->lockDetail = i;
                }
            }

    for ( i = 0; i < MAXSHIPS; i++ )
        if ( Ships[i].status == SS_LIVE)
        {
            taste = ( 1.0 +
                      Ships[i].kills * KILLS_KILLS +
                      Ships[i].armies * ARMY_KILLS ) / dist(Doomsday->x, Doomsday->y, Ships[i].x, Ships[i].y);
            if ( taste > tastiness )
            {
                tastiness = taste;
                Doomsday->lock = LOCK_SHIP;
                Doomsday->lockDetail = i;
            }
        }

    if ( Doomsday->lock == LOCK_PLANET )
        Doomsday->heading = utAngle( Doomsday->x, Doomsday->y, Planets[Doomsday->lockDetail].x, Planets[Doomsday->lockDetail].y );
    else if ( Doomsday->lock == LOCK_SHIP )
        Doomsday->heading = utAngle( Doomsday->x, Doomsday->y, Ships[Doomsday->lockDetail].x, Ships[Doomsday->lockDetail].y );

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

    for ( i = 0; i < MAXPLANETS; i++ )
        if ( PVISIBLE(i) &&
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
    int vacantShips[MAXSHIPS];
    int numvacant;

    /* maybe free up some slots... */
    clbCheckShips(FALSE);

    PVLOCK(&ConqInfo->lockword);
    *snum = -1;
    numvacant = 0;

    for ( i = 0; i < MAXSHIPS; i++ )
    {
        /* first, look for reserved ships that have no valid pid */
        if ( Ships[i].status == SS_RESERVED )
            if (!checkPID(Ships[i].pid))
            {
                Ships[i].status = SS_OFF; /* no-one there, turn it off */
                utLog("INFO: clbFindShip(): turned off reserved ship %d\n",
                      i);
            }

        /* if it's vacant and not an oper, save it for later */
        if (Ships[i].status == SS_LIVE &&
            SVACANT(i) &&
            !Users[Ships[i].unum].ooptions[OOPT_OPER])
            vacantShips[numvacant++] = i;

        /* if it's off, grab it */
        if ( Ships[i].status == SS_OFF )
        {
            *snum = i;
            break;
        }
    }

    if (*snum == -1)
    {
        /* we didn't find one.  If there were vacant ships, pick one and
         *  steal it.
         */
        if (numvacant)
        {
            if (numvacant == 1)
                *snum = vacantShips[0];
            else
                *snum = vacantShips[rndint(0, numvacant - 1)];

            utLog("INFO: clbFindShip: stealing vacant ship %d", *snum);
            clbIKill( *snum,  KB_GOD, 0 );
        }
    }

    if (*snum != -1)
    {
        /* we found one, so complete the takeover */
        clbZeroShip( *snum );
        Ships[*snum].status = SS_RESERVED;
        Ships[*snum].lastmsg = LMSG_NEEDINIT;
        Ships[*snum].sdfuse = -TIMEOUT_PLAYER;
        Ships[*snum].ctime = 0;
        Ships[*snum].etime = 0;
        Ships[*snum].cacc = 0;
        Ships[*snum].eacc = 0;
    }

    PVUNLOCK(&ConqInfo->lockword);

    return ( (*snum != -1) ? TRUE : FALSE );

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

    *sorpnum = -1;					/* zero nearest */
    *xsorpnum = -1;				/* zero second nearest */
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
        for ( i = 0; i < MAXSHIPS; i++ )
        {
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
        for ( i = 0; i < MAXPLANETS; i++ )
	{
            /* Only can look for "real" planets. */
            if ( ! PVISIBLE(i) )
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
        for ( i = 0; i < NUMPLAYERTEAMS; i++ )
            if ( Ships[snum].war[i] )
            {
                peaceful = FALSE;
                break;
            }

        /* Loop through the planets. */
        for ( i = 0; i < MAXPLANETS; i++ )
	{
            /* Only can look for "real" planets. */
            if ( ! PVISIBLE(i) )
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

    return ( *sorpnum != -1 );

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
    for ( i = 0; i < MAXUSERS; i++ )
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

    /* Turn off the universe. It will be turned back on in initUniverse() */
    ConqInfo->closed = TRUE;

    /* reset the lockwords  */
    ConqInfo->lockword = 0;
    ConqInfo->lockmesg = 0;

    /* Zero team stats. */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        for ( j = 0; j < MAXTSTATS; j = j + 1 )
            Teams[i].stats[j] = 0;

    /* De-register all users. */
    for ( i = 0; i < MAXUSERS; i++ )
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

    utStcpn( "never", ConqInfo->lastupchuck, DATESIZE );
    utFormatTime( ConqInfo->inittime, 0 );
    utFormatTime( ConqInfo->conqtime, 0 );
    utStcpn( "GOD", ConqInfo->conqueror, MAXUSERPNAME );
    utStcpn( "self ruled", ConqInfo->conqteam, MAXTEAMNAME );
    utStcpn( "Let there be light...", ConqInfo->lastwords, MAXLASTWORDS );

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
    Doomsday->lock = LOCK_NONE;
    Doomsday->lockDetail = 0;
    utStcpn( "Doomsday Machine", Doomsday->name, MAXUSERPNAME );

    /* Un-twiddle the lockword. */
    PVUNLOCK(&ConqInfo->lockword);

    /* Set up the physical universe. */
    cqiInitPlanets();

    return;

}


/*  initmsgs - initialize the message data structures */
/*  SYNOPSIS */
/*    initmsgs */
void clbInitMsgs(void)
{
    /* Zero the message buffer. */
    memset(Msgs, 0, MAXMESSAGES * sizeof(Msg_t));

    ConqInfo->lastmsg = 0;
    ConqInfo->glastmsg = ConqInfo->lastmsg;

    return;

}


/*  initrobots - initialize robots */
/*  SYNOPSIS */
/*    initrobots */
void clbInitRobots(void)
{
    int i, j, unum;

    /* SETROBOT( name, pname, team ) */
#define SETROBOT(x, y, z)                                               \
    {                                                                   \
        if ( clbGetUserNum( &unum, x, UT_LOCAL ) )                      \
            utStcpn( y, Users[unum].alias, MAXUSERPNAME );              \
        else if ( clbRegister( x, y, z, &unum ) )                       \
        {                                                               \
            Users[unum].robot = TRUE;                                   \
            Users[unum].ooptions[OOPT_MULTIPLE] = TRUE;                 \
            Users[unum].multiple = MAXSHIPS;                            \
            Users[unum].type = UT_LOCAL; /* robots are always local */  \
        }                                                               \
    }

    /* Create robot guardians. */
    SETROBOT( "Romulan", "Colossus", TEAM_ROMULAN );
    SETROBOT( "Orion", "HAL 9000", TEAM_ORION );
    SETROBOT( "Federation", "M-5", TEAM_FEDERATION );
    SETROBOT( "Klingon", "Guardian", TEAM_KLINGON );

    /* Copy the strategy table. */
    for ( i = 0; i < MAX_VAR; i++ )
        for ( j = 0; j < 10; j = j + 1 )
            Robot->rstrat[i][j] = trstrat[i][j];

    /* Copy the action vector. */
    for ( i = 0; i < 32; i++ )
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

    /* Ships[snum].status                 # never modified here */
    Ships[snum].killedBy = KB_NONE;
    Ships[snum].killedByDetail = 0;
    /* Ships[snum].user                   # setup in menu() or newrob() */
    /* Ships[snum].team                   # setup in menu() or newrob() */
    /* Ships[snum].pid                    # setup in menu() or newrob() */
    /* Ships[snum].shiptype               # setup in newship and newrob */

    /* init to 0.0, newrob will init this for robots to make them
     *   stronger, faster.. ;-)
     */
    Ships[snum].strkills = 0.0;
    Ships[snum].x = 0.0;
    Ships[snum].y = 0.0;
    Ships[snum].dx = 0.0;
    Ships[snum].dy = 0.0;
    Ships[snum].head = 0.0;
    Ships[snum].dhead = 0.0;
    Ships[snum].warp = 0.0;
    Ships[snum].dwarp = 0.0;
    Ships[snum].lock = LOCK_NONE;
    Ships[snum].lockDetail = 0;
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
    /* Ships[snum].option                 # setup in menu() */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    {
        /* Ships[snum].rwar               # setup in menu() or newrob() */
        /* Ships[snum].war                # setup in menu() or newrob() */
        Ships[snum].scanned[i] = 0;
    }
    for ( i = 0; i < MAXPLANETS; i++ )
        Ships[snum].srpwar[i] = FALSE;
    /* Ships[snum].sdfuse                 # setup in clbFindShip() */
    PVLOCK(&ConqInfo->lockmesg);
    if ( Ships[snum].lastmsg == LMSG_NEEDINIT )
    {
        Ships[snum].lastmsg = ConqInfo->lastmsg;
        Ships[snum].alastmsg = Ships[snum].lastmsg;
    }
    PVUNLOCK(&ConqInfo->lockmesg);
    Ships[snum].towing = 0;
    Ships[snum].towedby = 0;
    Ships[snum].lastblast = 0.0;
    Ships[snum].lastphase = 0.0;
    Ships[snum].pfuse = 0;
    Ships[snum].action = 0;

    Ships[snum].flags = SHIP_F_NONE;
    SFSET(snum, SHIP_F_SHUP);

    /* Ships[snum].alias                  # setup in menu() or newrob() */

    /* Zero torpedos. */
    for ( i = 0; i < MAXTORPS; i++ )
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

    /* time stamp for this entry */
    Users[unum].lastentry = time(0);

    Users[unum].stats[USTAT_ENTRIES] += 1;
    Teams[Ships[snum].team].stats[TSTAT_ENTRIES] += 1;

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

    utStcpn( "Scout", ShipTypes[ST_SCOUT].name, MAXSTNAME );
    ShipTypes[ST_SCOUT].armylim = 7;
    ShipTypes[ST_SCOUT].warplim = 10.0;
    ShipTypes[ST_SCOUT].engfac = 1.2;
    ShipTypes[ST_SCOUT].accelfac = 1.6;
    ShipTypes[ST_SCOUT].weafac = 0.83;
    ShipTypes[ST_SCOUT].torpwarp = 14.0;

    utStcpn( "Destroyer", ShipTypes[ST_DESTROYER].name, MAXSTNAME );
    ShipTypes[ST_DESTROYER].armylim = 9;
    ShipTypes[ST_DESTROYER].warplim = 9.0;
    ShipTypes[ST_DESTROYER].engfac = 1.0;
    ShipTypes[ST_DESTROYER].accelfac = 1.0;
    ShipTypes[ST_DESTROYER].weafac = 1.0;
    ShipTypes[ST_DESTROYER].torpwarp = 12.0;

    utStcpn( "Cruiser", ShipTypes[ST_CRUISER].name, MAXSTNAME );
    ShipTypes[ST_CRUISER].armylim = 11;
    ShipTypes[ST_CRUISER].warplim = 8.0;
    ShipTypes[ST_CRUISER].engfac = 0.8;
    ShipTypes[ST_CRUISER].accelfac = 0.8;
    ShipTypes[ST_CRUISER].weafac = 1.17;
    ShipTypes[ST_CRUISER].torpwarp = 10.0;

    Teams[TEAM_FEDERATION].shiptype = ST_DESTROYER;
    Teams[TEAM_KLINGON].shiptype = ST_DESTROYER;
    Teams[TEAM_ROMULAN].shiptype = ST_CRUISER;
    Teams[TEAM_ORION].shiptype = ST_SCOUT;

    utStcpn( "Federation", Teams[TEAM_FEDERATION].name, MAXTEAMNAME );
    utStcpn( "Romulan", Teams[TEAM_ROMULAN].name, MAXTEAMNAME );
    utStcpn( "Klingon", Teams[TEAM_KLINGON].name, MAXTEAMNAME );
    utStcpn( "Orion", Teams[TEAM_ORION].name, MAXTEAMNAME );
    utStcpn( "self ruled", Teams[TEAM_SELFRULED].name, MAXTEAMNAME );
    utStcpn( "non", Teams[TEAM_NOTEAM].name, MAXTEAMNAME );
    utStcpn( "GOD", Teams[TEAM_GOD].name, MAXTEAMNAME );
    utStcpn( "Empire", Teams[TEAM_EMPIRE].name, MAXTEAMNAME );

    ConqInfo->chrplanets[PLANET_CLASSM] = 'M';
    ConqInfo->chrplanets[PLANET_DEAD] = 'D';
    ConqInfo->chrplanets[PLANET_SUN] = 'S';
    ConqInfo->chrplanets[PLANET_MOON] = 'm';
    ConqInfo->chrplanets[PLANET_GHOST] = 'G';
    ConqInfo->chrplanets[PLANET_CLASSA] = 'A';
    ConqInfo->chrplanets[PLANET_CLASSO] = 'O';
    ConqInfo->chrplanets[PLANET_CLASSZ] = 'Z';

    utStcpn( "class M planet", ConqInfo->ptname[PLANET_CLASSM], MAXPTYPENAME );
    utStcpn( "dead planet", ConqInfo->ptname[PLANET_DEAD], MAXPTYPENAME );
    utStcpn( "sun", ConqInfo->ptname[PLANET_SUN], MAXPTYPENAME );
    utStcpn( "moon", ConqInfo->ptname[PLANET_MOON], MAXPTYPENAME );
    utStcpn( "ghost planet", ConqInfo->ptname[PLANET_GHOST], MAXPTYPENAME );
    utStcpn( "class A planet", ConqInfo->ptname[PLANET_CLASSA], MAXPTYPENAME );
    utStcpn( "class O planet", ConqInfo->ptname[PLANET_CLASSO], MAXPTYPENAME );
    utStcpn( "class Z planet", ConqInfo->ptname[PLANET_CLASSZ], MAXPTYPENAME );

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
    Driver->drivowner[0] = 0;

    /* Initialize user history stuff. */
    ConqInfo->histptr = 0;
    for ( i = 0; i < MAXHISTLOG; i++ )
    {
        History[i].histunum = -1;
        History[i].histlog = 0;
        History[i].username[0] = 0;
    }

    /* Un-twiddle the lockword. */
    PVUNLOCK(&ConqInfo->lockword);

    clbInitGame();
    clbClearShips();
    clbInitMsgs();

    /* Turn the universe back on (turned off in initEverything()). */
    ConqInfo->closed = FALSE;

    return;

}


/*  intrude - possibly send an intruder alert */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    clbIntrude( snum, pnum ) */
void clbIntrude( int snum, int pnum )
{
    char buf[MSGMAXLINE];
    char *atta=" attacking";
    char *armeq=", armies=";

    if ( PVISIBLE(pnum) &&
         Planets[pnum].team != TEAM_SELFRULED &&
         Planets[pnum].team != TEAM_NOTEAM )
    {
        if ( snum < 0 ) // doomsday
	{
            strcpy(buf , Doomsday->name) ;
            utToUpperCase( Doomsday->name );
            strcat(buf , atta) ;
            strcat(buf , armeq) ;
            utAppendInt(buf , Planets[pnum].armies) ;
            clbStoreMsgf( MSG_FROM_PLANET, pnum,
                          MSG_TO_TEAM, Planets[pnum].team,
                          buf, MSG_FLAGS_INTRUDER );
	}
        else if ( Ships[snum].war[Planets[pnum].team] ) // ship
	{
            strcpy(buf , "INTRUDER ALERT - ") ;
            utAppendShip(buf , snum) ;
            strcat(buf , atta) ;
            strcat(buf , armeq) ;
            utAppendInt(buf , Planets[pnum].armies) ;
            clbStoreMsgf( MSG_FROM_PLANET, pnum,
                          MSG_TO_TEAM, Planets[pnum].team,
                          buf, MSG_FLAGS_INTRUDER );
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
    ConqInfo->histptr = utModPlusOne( ConqInfo->histptr + 1, MAXHISTLOG );
    /* time stamp for this entry */
    History[ConqInfo->histptr].histlog = time(0);
    History[ConqInfo->histptr].elapsed = (time_t)0;
    History[ConqInfo->histptr].histunum = unum;
    strncpy(History[ConqInfo->histptr].username,
            Users[unum].username,
            MAXUSERNAME - 1);
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
        for ( *pnum = 0; *pnum < MAXPLANETS; *pnum = *pnum + 1 )
            if ( utStringMatch( str, Planets[*pnum].name, FALSE ) )
                return ( TRUE );
    }
    else
    {
        for ( *pnum = 0; *pnum < MAXPLANETS; *pnum = *pnum + 1 )
            if ( PVISIBLE(*pnum) )
                if ( utStringMatch( str, Planets[*pnum].name, FALSE ) )
                    return ( TRUE );
    }

    return ( FALSE );

}


/*  putship - place a ship in the universe */
/*  SYNOPSIS */
/*    int snum */
/*    real basex, basey */
/*    clbPutShip( snum, basex, basey ) */
void clbPutShip( int snum, real basex, real basey )
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
        for ( i = 0; i < MAXSHIPS; i++ )
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
    utLog( "clbPutShip(): Failed retry maximum on ship %d", snum );

    return;

}


/* fmtmsg - format a message string -JET */
int clbFmtMsg(msgFrom_t from, uint16_t fromDetail, msgTo_t to,
              uint16_t toDetail, char *buf)
{

    buf[0] = '\0';

    /* Format who the message is from. */
    if ( from == MSG_FROM_SHIP && fromDetail < MAXSHIPS )
    {
        utAppendShip(buf , (int)fromDetail) ;
    }
    else if ( from == MSG_FROM_PLANET && fromDetail < MAXPLANETS )
        strcpy(buf , Planets[fromDetail].name) ;
    else
    {
        switch ( from )
         {
         case MSG_FROM_NOONE:
             strcpy(buf , "nobody") ;
             break;
         case MSG_FROM_GOD:
             strcpy(buf , "GOD") ;
             break;
         case MSG_FROM_DOOM:
             strcat(buf, "The ");
             strcat(buf, Doomsday->name);
             break;
         case MSG_FROM_COMP:
             strcpy(buf , "Comp") ;
             break;
         default:
             strcpy(buf , "???") ;
             break;
         }
    }

    strcat(buf , "->") ;

    /* Format who the message is to. */
    if ( to == MSG_TO_SHIP && toDetail < MAXSHIPS )
        utAppendShip(buf , (int)toDetail) ;
    else if ( to == MSG_TO_TEAM && toDetail < NUMPLAYERTEAMS )
    {
        utAppendChar(buf , Teams[toDetail].teamchar) ;
    }
    else
    {
        switch ( to )
	{
	case MSG_TO_NOONE:
            strcat(buf , "nobody") ;
            break;
	case MSG_TO_GOD:
            strcat(buf , "GOD") ;
            break;
	case MSG_TO_ALL:
            strcat(buf , "ALL") ;
            break;
	case MSG_TO_IMPLEMENTORS:
            strcat(buf , "IMPs") ;
            break;
	case MSG_TO_FRIENDLY:
            strcat(buf , "FRIEND") ;
            break;
	default:
            strcat(buf , "???") ;
            break;
	}
    }

    return(TRUE);
}


/* cmpplanet - compare planet names based on index passed via qsort() */
static int cmpplanet(void *cmp1, void *cmp2)
{
    int *icmp1, *icmp2;

    icmp1 = (int *) cmp1;
    icmp2 = (int *) cmp2;

    return(strcmp(Planets[*icmp1].name, Planets[*icmp2].name));
}

/*  sortplanets - sort planets by name */
/*  SYNOPSIS */
/*    int sv(MAXPLANETS) */
/*    clbSortPlanets( sv ) */
/* This routine ASSUMES that the sort vector is initialized, */
/* for the reason that it is fastest to sort a list that is */
/* already sorted. */
void clbSortPlanets( int sv[] )
{
    qsort(sv, MAXPLANETS, sizeof(int),
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


    if ( ! PVISIBLE(pnum) )
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


    if (snum < 0 || snum >= MAXSHIPS)
        return(TRUE);

    /* Look for religious trouble or the "closed" sign in the window. */
    if ( Users[Ships[snum].unum].ooptions[OOPT_SHITLIST] )
    {
        if ( Ships[snum].status == SS_LIVE )
            clbKillShip( snum, KB_SHIT, 0 );

        return ( FALSE );
    }
    if ( ConqInfo->closed && ! Users[Ships[snum].unum].ooptions[OOPT_PLAYWHENCLOSED] )
    {
        if ( Ships[snum].status == SS_LIVE )
            clbKillShip(snum, KB_EVICT, 0);

        return ( FALSE );
    }

    if ( Ships[snum].status == SS_RESERVED || Ships[snum].status == SS_ENTERING )
        return ( TRUE );

    return ( Ships[snum].status == SS_LIVE );

}

/* wrapper for stormsg.  Most routines just use this version. */
void clbStoreMsg( msgFrom_t from, uint16_t fromDetail,
                  msgTo_t to, uint16_t toDetail, char *msg )
{
    clbStoreMsgf(from, fromDetail, to, toDetail, msg, MSG_FLAGS_NONE);
    return;
}

/*  stormsgf - store a message in the msg buffer with flags (DOES LOCKING) */
void clbStoreMsgf( msgFrom_t from, uint16_t fromDetail,
                   msgTo_t to, uint16_t toDetail,
                   char *msg, unsigned char flags )
{
    int nlastmsg, i;
    char buf[128];

    /* don't do this if invalid common block */
    if (*CBlockRevision != COMMONSTAMP)
        return;

    // do some basic checks on the to and from
    if (from >= MSG_FROM_MAX || to >= MSG_TO_MAX)
    {
        utLog("%s: In valid to or from enumerants: to %d from %d",
              __FUNCTION__, to, from);
        return;
    }

    /* don't bother with tersables/feedbacks to robots */
    if (to == MSG_TO_SHIP && toDetail < MAXSHIPS && SROBOT(toDetail))
        if (flags & (MSG_FLAGS_TERSABLE | MSG_FLAGS_FEEDBACK))
            return;

    PVLOCK(&ConqInfo->lockmesg);
    nlastmsg = utModPlusOne( ConqInfo->lastmsg + 1, MAXMESSAGES );
    utStcpn( msg, Msgs[nlastmsg].msgbuf, MESSAGE_SIZE );
    Msgs[nlastmsg].from = from;
    Msgs[nlastmsg].fromDetail = fromDetail;
    Msgs[nlastmsg].to = to;
    Msgs[nlastmsg].toDetail = toDetail;
    Msgs[nlastmsg].flags = flags;
    ConqInfo->lastmsg = nlastmsg;

    /* Remove allowable last message restrictions. */
    for ( i = 0; i < MAXSHIPS; i++ )
        if ( nlastmsg == Ships[i].alastmsg )
            Ships[i].alastmsg = LMSG_READALL;

    PVUNLOCK(&ConqInfo->lockmesg);

    if (SysConf.LogMessages == TRUE || to == MSG_TO_GOD
        || from == MSG_FROM_GOD)
    {
        clbFmtMsg(from, fromDetail, to, toDetail, buf);
        utLog("MSG: %s: %s",
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
                    clbStoreMsgf( MSG_FROM_COMP, 0,
                                  MSG_TO_SHIP, snum,
                                  "Weapons overload.",
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
                    clbStoreMsgf( MSG_FROM_COMP, 0,
                                  MSG_TO_SHIP, snum,
                                  "Engines super-heated.",
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
    Ships[snum].killedBy = KB_NONE;
    Ships[snum].killedByDetail = 0;
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
    Ships[snum].lock = LOCK_NONE;
    Ships[snum].lockDetail = 0;
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
    Ships[snum].shiptype = ST_SCOUT;
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    {
        Ships[snum].rwar[i] = FALSE;
        Ships[snum].war[i] = FALSE;
        Ships[snum].scanned[i] = 0;
    }
    for ( i = 0; i < MAXPLANETS; i++ )
        Ships[snum].srpwar[i] = FALSE;
    Ships[snum].sdfuse = 0;
    Ships[snum].lastmsg = 0;
    Ships[snum].alastmsg = 0;
    Ships[snum].towing = 0;
    Ships[snum].towedby = 0;
    Ships[snum].lastblast = 0.0;
    Ships[snum].lastphase = 0.0;
    Ships[snum].pfuse = 0;
    Ships[snum].action = 0;
    for ( i = 0; i < MAXUSERPNAME; i++ )
        Ships[snum].alias[i] = 0;
    Ships[snum].ctime = 0;
    Ships[snum].etime = 0;
    Ships[snum].cacc = 0;
    Ships[snum].eacc = 0;

    Ships[snum].flags = SHIP_F_NONE;
    SFSET(snum, SHIP_F_SHUP);

    for ( i = 0; i < MAXTORPS; i++ )
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

/* for MINGW - we are only building the client, so the 'username' is
 * irrelevant, so return something useless :)
 */
#if defined(MINGW)
char *clbGetUserLogname(void)
{
    return "MinGW";
}
#else


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
            utLog("ERROR: clbGetUserLogname(): getpwuid(geteuid()) failed: %s",
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
#endif  /* MINGW */

/*  planetdrive - move the planets based on interval */
/*  SYNOPSIS */
/*    planetdrive */
void clbPlanetDrive(real itersec)
{
    int i;
    real speed;

    for ( i = 0; i <= MAXPLANETS; i++ )
    {
        /* Advance porbang() if planet is not stationary. */
        if ( Planets[i].primary != i ) // not orbiting itself...
	{

            Planets[i].orbang = utMod360( Planets[i].orbang +
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
    static int ship[MAXSHIPS];
    static int FirstTime = TRUE;

    if (FirstTime)
    {
        FirstTime = FALSE;
        /* Randomize ship ordering. */

        for ( s = 0; s < MAXSHIPS; s++ )
            ship[s] = s;

        for ( s = 0; s < MAXSHIPS; s++ )
        {
            i = rndint( 0, MAXSHIPS - 1);
            j = ship[i];
            ship[i] = ship[s];
            ship[s] = j;

        }
    }

    for ( s = 0; s < MAXSHIPS; s++ )
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

/* compute a ship's proper position when orbiting */
void clbAdjOrbitalPosition(int snum)
{
    if (snum >= 0 && snum < MAXSHIPS && Ships[snum].warp < 0.0
        && Ships[snum].lock == LOCK_PLANET)
    {
        int pnum = Ships[snum].lockDetail;

        if ( pnum >= 0 && pnum < MAXPLANETS )
        {
            /* ajust the ship's X/Y based on orbit direction and heading */
            if ( Ships[snum].warp == ORBIT_CW )
            {
                Ships[snum].x =
                    (real)(Planets[pnum].x + (ORBIT_DIST *
                                              cosd(Ships[snum].head + 90.0)));

                Ships[snum].y =
                    (real)(Planets[pnum].y + (ORBIT_DIST *
                                              sind(Ships[snum].head + 90.0)));
            }
            else if ( Ships[snum].warp == ORBIT_CCW )
            {
                /* Orbiting counter-clockwise. */
                Ships[snum].x =
                    (real)(Planets[pnum].x + (ORBIT_DIST *
                                              cosd(Ships[snum].head - 90.0)));

                Ships[snum].y =
                    (real)(Planets[pnum].y + (ORBIT_DIST *
                                              sind(Ships[snum].head - 90.0)));

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

#define TIMEDELTA(dest, src1, src2) {                                   \
        if(((dest).tv_usec = (src1).tv_usec - (src2).tv_usec) < 0) {    \
            (dest).tv_usec += 1000000;                                  \
            (dest).tv_sec = (src1).tv_sec - (src2).tv_sec - 1;          \
        } else {                                                        \
            (dest).tv_sec = (src1).tv_sec - (src2).tv_sec;              \
        }                                                               \
    }

/* return time in milliseconds */
uint32_t clbGetMillis(void)
{
    static int firsttime = TRUE;
    static struct timeval start;
    struct timeval elapsed, now;
    uint32_t elapse;

    if (firsttime)
    {
        firsttime = FALSE;

        GETTIMEOFDAY(&start);
    }

    GETTIMEOFDAY(&now);
    TIMEDELTA(elapsed, now, start);
    /* Return elapsed milliseconds. */
    elapse = (uint32_t) ((elapsed.tv_sec * 1000) + (elapsed.tv_usec / 1000));

    /* don't return 0 */
    if (elapse == 0)
        elapse = 1;

    return elapse;
}


#if defined(MINGW)
void clbBlockAlarm(void)
{
    return;
}
void clbUnblockAlarm(void)
{
    return;
}
#else
void clbBlockAlarm(void)
{
    sigset_t newmask;

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
    sigprocmask(SIG_BLOCK, &newmask, NULL);

    return;
}

void clbUnblockAlarm(void)
{
    sigset_t newmask;

    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &newmask, NULL);

    return;
}

#endif  /* MINGW */

/* may use LOCKING */
void clbCheckShips(int isDriver)
{
    int i;

    /* look for vacant ships that aren't marked vacant */
    for (i=0; i<MAXSHIPS; i++)
    {
        if (Ships[i].status == SS_LIVE && !SVACANT(i))
            if (Ships[i].pid > 0 && !checkPID(Ships[i].pid))
            {
                utLog("INFO: clbCheckShips(isDriver=%d): marking ship %d as VACANT",
                      isDriver, i);
                SFSET(i, SHIP_F_VACANT);
            }

        /* if the ship is VACANT, and vacants aren't allowed, kill them. */
        if (!SysConf.AllowVacant && SVACANT(i) && Ships[i].status == SS_LIVE )
        {
            /* when the driver calls this function, and the ship must be
             *  killed, then we'll do it 'normally' so players can know
             *  about it.  Otherwise just turn it off.
             */
            if (isDriver)
            {
                utLog("INFO: clbCheckShips(isDriver=%d): killing VACANT ship %d",
                      isDriver, i);
                clbKillShip(i, KB_GOD, 0);
            }
            else
            {
                /* just turn it off and log it */
                utLog("INFO: clbCheckShips(isDriver=%d): turning off VACANT ship %d",
                      isDriver, i);
                PVLOCK(&ConqInfo->lockword);
                clbIKill(i,  KB_GOD, 0);
                clbZeroShip( i );
                PVUNLOCK(&ConqInfo->lockword);
            }
        }
    }

    return;
}
