//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "c_defs.h"

#include <string>
#include <vector>
#include "fmt/format.h"

#include "conqdef.h"
#include "cb.h"
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

using namespace std;

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

    unum = cbShips[snum].unum;
    team = cbShips[snum].team;

    /* Update wins. */
    cbUsers[unum].stats[USTAT_WINS] += (int)cbShips[snum].kills;
    cbTeams[team].stats[TSTAT_WINS] = cbTeams[team].stats[TSTAT_WINS]
        + (int)cbShips[snum].kills;

    /* Update max kills. */
    i = (int)cbShips[snum].kills;
    if ( i > cbUsers[unum].stats[USTAT_MAXKILLS] )
        cbUsers[unum].stats[USTAT_MAXKILLS] = i;

    /* Update rating. */
    l = cbUsers[unum].stats[USTAT_LOSSES];
    if ( l == 0 )
        l = 1;
    w = cbUsers[unum].stats[USTAT_WINS];
    m = cbUsers[unum].stats[USTAT_MAXKILLS];
    cbUsers[unum].rating = ( w / l ) + ( m / 4.0 );
    x = w - l;
    if ( x >= 0.0 )
        cbUsers[unum].rating += pow((real) x, (real) ( 1.0 / 3.0 ));
    else
        cbUsers[unum].rating -= pow((real) -x, (real) ( 1.0 / 3.0 ));

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

    cbShips[snum].damage = cbShips[snum].damage + dam;
    if ( cbShips[snum].damage >= 100.0 )
        clbKillShip( snum, kb, detail );
    else
    {
        mw = maxwarp( snum );
        cbShips[snum].dwarp = min( cbShips[snum].dwarp, mw );
    }

    return;

}

/*  detonate - blow up a torpedo (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, tnum */
/*    clbDetonate( snum, tnum ) */
void clbDetonate( int snum, int tnum )
{

    cbLock(&cbConqInfo->lockword);
    if ( cbShips[snum].torps[tnum].status == TS_LIVE )
        cbShips[snum].torps[tnum].status = TS_DETONATE;
    cbUnlock(&cbConqInfo->lockword);

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

    if ( ! clbUseFuel( snum, DETONATE_FUEL, true, true ) )
        return ( false );

    for ( i = 0; i < cbLimits.maxShips(); i++ )
        if ( cbShips[i].status != SS_OFF && i != snum )
            for ( j = 0; j < cbLimits.maxTorps(); j = j + 1 )
                if ( cbShips[i].torps[j].status == TS_LIVE )
                    if ( cbShips[i].torps[j].war[cbShips[snum].team] || cbShips[snum].war[cbShips[i].team] )
                        if ( dist( cbShips[snum].x, cbShips[snum].y, cbShips[i].torps[j].x, cbShips[i].torps[j].y ) <=
                             DETONATE_DIST )
                            clbDetonate( i, j );

    return ( true );

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
            if ( ht > cbShips[snum].shields )
	    {
                clbDamage( snum, ht-cbShips[snum].shields, kb, detail );
                cbShips[snum].shields = 0.0;
	    }
            else
	    {
                cbShips[snum].shields = cbShips[snum].shields - ht;
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
    if ( cbShips[snum].status != SS_LIVE )
        return;

    /* The ship is alive; kill it. */
    cbShips[snum].killedBy = kb;
    cbShips[snum].killedByDetail = detail;
    cbShips[snum].status = SS_DYING;

    unum = cbShips[snum].unum;
    team = cbShips[snum].team;

    /* Detonate all torpedos. */

    for ( i = 0; i < cbLimits.maxTorps(); i++ )
        if ( cbShips[snum].torps[i].status == TS_LIVE )
            cbShips[snum].torps[i].status = TS_DETONATE;

    /* Release any tows. */
    if ( STOWING(snum) )
    {
        SFCLR(cbShips[snum].towing, SHIP_F_TOWEDBY);
        SFCLR(snum, SHIP_F_TOWING);
    }

    if ( STOWEDBY(snum) )
    {
        SFCLR(cbShips[snum].towedby, SHIP_F_TOWING);
        SFCLR(snum, SHIP_F_TOWEDBY);
    }

    /* Zero team scan fuses. */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        cbShips[snum].scanned[i] = 0;

    if ( kb == KB_CONQUER )
        cbShips[snum].kills = cbShips[snum].kills + CONQUER_KILLS;
    else if ( kb == KB_GOTDOOMSDAY )
        cbShips[snum].kills = cbShips[snum].kills + DOOMSDAY_KILLS;
    else if ( kb == KB_SHIP && detail < cbLimits.maxShips() )
    {
        // if a ship did the killing
        kunum = cbShips[detail].unum;
        kteam = cbShips[detail].team;
        tkills = 1.0 + ((cbShips[snum].kills + cbShips[snum].strkills) * KILLS_KILLS);
        if ( cbShips[snum].armies > 0 )
	{
            /* Keep track of carried armies killed - they are special. */
            tkills = tkills + cbShips[snum].armies * ARMY_KILLS;
            cbUsers[kunum].stats[USTAT_ARMSHIP] += cbShips[snum].armies;
            cbTeams[kteam].stats[TSTAT_ARMSHIP] += cbShips[snum].armies;
	}

        /* Kills accounting. */
        if ( cbShips[detail].status == SS_LIVE )
            cbShips[detail].kills = cbShips[detail].kills + tkills;
        else
	{
            /* Have to do some hacking when our killer is dead. */
            cbUsers[kunum].stats[USTAT_WINS] -= (int)cbShips[detail].kills;
            cbTeams[kteam].stats[TSTAT_WINS] =
                cbTeams[kteam].stats[TSTAT_WINS] - (int)cbShips[detail].kills;
            cbShips[detail].kills = cbShips[detail].kills + tkills;
            clbChalkup( detail );
	}

        /* Sticky war logic. */

        if ( cbShips[detail].war[team] )
	{
            cbShips[detail].rwar[team] = true;
	}
    }

    /* Kills accounting. */
    clbChalkup( snum );
    if ( kb != KB_SELF && kb != KB_CONQUER && kb != KB_NEWGAME &&
         kb != KB_EVICT && kb != KB_SHIT && kb != KB_GOD )
    {
        /* Update losses. */
        cbUsers[unum].stats[USTAT_LOSSES] += 1;
        cbTeams[team].stats[TSTAT_LOSSES] += 1;
    }

    /* set the ship dead-like :) */
    cbShips[snum].status = SS_DEAD;

    /* setup dead timeout timer. For robots and vacant ships, that's 5
     *  seconds - this gives enough time for detonating torps, just like
     *  regular players
     */
    if (SROBOT(snum) || SVACANT(snum))
        cbShips[snum].sdfuse = -5;
    else
        cbShips[snum].sdfuse = -TIMEOUT_PLAYER;

    return;
}


/* ETAstr - return a string indicating ETA to a target */
std::string clbETAStr(real warp, real distance)
{
    if (warp <= 0.0)
    {
        return "never";
    }

    real mins = 0.0;
    real secs = (distance / (warp * (real)MM_PER_SEC_PER_WARP));

    if (secs > 60.0)
    {
        mins = secs / 60.0;
        secs = 0.0;
    }

    std::string retstr;
    if (mins != 0.0)
        retstr = fmt::format("{:.1f} minutes", mins);
    else
        retstr = fmt::format("{:.1f} seconds", secs);

    return retstr;
}


/*  kill - kill a ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, kb */
/*    kill( snum, kb ) */
void clbKillShip(int snum, killedBy_t kb, uint16_t detail)
{
    int sendmesg = false;
    std::string msgbuf;

    /* launch all torps - sorta, we'll use 'explode' mode... */
    clbLaunch(snum, 0.0, EXPLODESHIP_TORP_COUNT, LAUNCH_EXPLODE);

    /* internal routine. */
    cbLock(&cbConqInfo->lockword);
    clbIKill( snum, kb, detail );
    cbUnlock(&cbConqInfo->lockword);

    /* send a msg to all... */
    sendmesg = false;

    /* Figure out why we died. */
    std::string sbuf = fmt::format("{} ({})", utShipStr(snum),
                                   cbShips[snum].alias);

    switch ( kb )
    {
    case KB_SELF:
        msgbuf = fmt::format("{} has self-destructed.", sbuf);
        sendmesg = true;

        break;
    case KB_NEGENB:
        msgbuf = fmt::format("{} was destroyed by the negative "
                             "energy barrier.",
                             sbuf);
        sendmesg = true;

        break;

    case KB_GOD:
        msgbuf = fmt::format("{} was killed by an act of GOD.", sbuf);
        sendmesg = true;

        break;
    case KB_DOOMSDAY:
        msgbuf = fmt::format("{} was eaten by the doomsday machine.", sbuf);
        sendmesg = true;

        break;
    case KB_GOTDOOMSDAY:
        msgbuf = fmt::format("{} killed the doomsday machine!", sbuf);
        sendmesg = true;

        break;
    case KB_LIGHTNING:
        msgbuf = fmt::format("{} was destroyed by a lightning bolt.", sbuf);
        sendmesg = true;

        break;

    case KB_SHIP:
        if (detail < cbLimits.maxShips())
	{
            msgbuf = fmt::format("{} was kill {:.1f} for {}{} ({}).",
                                 sbuf,
                                 cbShips[detail].kills,
                                 cbTeams[cbShips[detail].team].teamchar,
                                 detail,
                                 cbShips[detail].alias);
            sendmesg = true;

	}
        break;

    case KB_PLANET:
        if (detail < cbLimits.maxPlanets())
	{
            msgbuf = fmt::format("{} was destroyed by {}",
                                 sbuf,
                                 cbPlanets[detail].name);

            sendmesg = true;

            if ( cbPlanets[detail].type == PLANET_SUN )
	    {
                msgbuf += "'s solar radiation.";
	    }
            else
	    {
                msgbuf += "'s planetary defenses.";
	    }
	}
        break;

    default:
        utLog("%s: Invalid killedby: snum %d, kb %d, detail %d", __FUNCTION__,
              snum, (int)kb, (int)detail);
    }

    if (sendmesg == true)
        clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, msgbuf.c_str());

    return;

}

/* see if we could launch some if we wanted too... */
/* does NO LOCKING, so only use from the client */
int clbCheckLaunch(int snum, int number)
{
    int i;

    if (cbShips[snum].wfuse > 0)
        return false;               /* weapons overloaded */

    if (number == 0)
        return true;

    for ( i = 0; i < cbLimits.maxTorps(); i++ )
        if ( cbShips[snum].torps[i].status == TS_OFF )
            return true;

    return false;
}


/*  launch - create new torpedo(s) for a ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    flag = clbLaunch( snum, dir, number_of_torps, launch_type ) */
int clbLaunch( int snum, real dir, int number, int ltype )
{
    int i, j;
    real speed = 0.0, adir = 0.0;
    int tnum, numfired;
    std::vector<int> tslot;

    /* Stop repairing. */
    SFCLR(snum, SHIP_F_REPAIR);

    /* Remember this important direction. */
    cbShips[snum].lastblast = dir;

    // Set up last fired phaser direction, but only if it's not already firing
    if ( !cbShips[snum].pfuse )
        cbShips[snum].lastphase = dir;

    numfired = 0;
    tnum = number;

    /* Find free torp(s). */
    cbLock(&cbConqInfo->lockword);
    for ( i = 0; i < cbLimits.maxTorps() && tnum != 0; i++ )
        if ( cbShips[snum].torps[i].status == TS_OFF )
        {
            /* Found one. */
            cbShips[snum].torps[i].status = TS_LAUNCHING;
            tslot.push_back(i);
            tnum--;
        }
    cbUnlock(&cbConqInfo->lockword);

    if (tslot.empty())
    {				/* couldn't find even one */
        return(false);
    }

    for (i=0; i<tslot.size(); i++)
    {
        /* Use fuel. */
        if ( clbUseFuel( snum, TORPEDO_FUEL, true, true ) == false)
	{
            cbShips[snum].torps[tslot[i]].status = TS_OFF;
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
            cbShips[snum].torps[tslot[i]].fuse = 1; /* shouldn't last long */
            /* should be close to the ship */
            cbShips[snum].torps[tslot[i]].x =
                rndnor( cbShips[snum].x, EXPLODESHIP_TORP_SPREAD );
            cbShips[snum].torps[tslot[i]].y =
                rndnor( cbShips[snum].y, EXPLODESHIP_TORP_SPREAD );
            /* no movement, direction, or deltas */
            cbShips[snum].torps[tslot[i]].dx = 0.0;
            cbShips[snum].torps[tslot[i]].dy = 0.0;

            /* strength of explosion depends on the average of engine
               and weap efficiency.  This prevents one side from
               having an explosive adv over another, while allowing
               greater kills to matter.  */
            cbShips[snum].torps[tslot[i]].mult = (( (real)engeff(snum) +
                                                  (real)weaeff(snum) ) / 2.0);
	}
        else
	{
            cbShips[snum].torps[tslot[i]].fuse = TORPEDO_FUSE;
            cbShips[snum].torps[tslot[i]].x = rndnor( cbShips[snum].x, 100.0 );
            cbShips[snum].torps[tslot[i]].y = rndnor( cbShips[snum].y, 100.0 );
            speed = cbShipTypes[cbShips[snum].shiptype].torpwarp * MM_PER_SEC_PER_WARP *
                ITER_SECONDS;
            adir = rndnor( dir, 2.0 );
            cbShips[snum].torps[tslot[i]].dx = (real) (speed * cosd(adir));
            cbShips[snum].torps[tslot[i]].dy = (real)(speed * sind(adir));
            cbShips[snum].torps[tslot[i]].mult = (real)weaeff( snum );
	}

        for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
	{
            if (ltype == LAUNCH_EXPLODE)
	    {			/* if our ship is exploding we're at war
				   with everything. */
                cbShips[snum].torps[tslot[i]].war[j] = true;
	    }
            else
                cbShips[snum].torps[tslot[i]].war[j] = cbShips[snum].war[j]; /* just enemies */
	}

        cbShips[snum].torps[tslot[i]].status = TS_LIVE;
    }

    if (numfired == 0)
    {				/* couldn't fire any. bummer dude. */
        return(false);
    }
    else
    {				/* torps away! */
        /* Update stats. */
        cbLock(&cbConqInfo->lockword);
        cbUsers[cbShips[snum].unum].stats[USTAT_TORPS] += numfired;
        cbTeams[cbShips[snum].team].stats[TSTAT_TORPS] += numfired;
        cbUnlock(&cbConqInfo->lockword);

        if (numfired == number)
	{			/* fired all requested */
            return ( true );
	}
        else
	{
            /* fired some, but not all */
            return(false);
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

    cbShips[snum].lock = LOCK_PLANET;
    cbShips[snum].lockDetail = pnum;
    cbShips[snum].dwarp = 0.0;

    /* Find bearing to planet. */
    beer = utAngle( cbShips[snum].x, cbShips[snum].y,
                    cbPlanets[pnum].x, cbPlanets[pnum].y );
    if ( cbShips[snum].head < ( beer - 180.0 ) )
        beer = beer - 360.0;

    /* Check beer head to determine orbit direction. */
    if ( beer <= cbShips[snum].head )
    {
        cbShips[snum].warp = ORBIT_CW;
        cbShips[snum].head = utMod360( beer + 90.0 );
    }
    else
    {
        cbShips[snum].warp = ORBIT_CCW;
        cbShips[snum].head = utMod360( beer - 90.0 );
    }

    if (SysConf.NoDrift)
    {
        // be sure to clear deltas
        cbShips[snum].dx = 0.0;
        cbShips[snum].dy = 0.0;
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
    cbShips[snum].lastblast = dir;

    /* Stop repairing. */
    SFCLR(snum, SHIP_F_REPAIR);

    /* See if ok to fire. */
    if ( cbShips[snum].pfuse > 0 )
        return ( false );

    /* Try to use fuel for this shot. */
    if ( ! clbUseFuel( snum, PHASER_FUEL, true, true ) )
        return ( false );

    /* Update stats. */
    cbLock(&cbConqInfo->lockword);
    cbUsers[cbShips[snum].unum].stats[USTAT_PHASERS] += 1;
    cbTeams[cbShips[snum].team].stats[TSTAT_PHASERS] += 1;
    cbUnlock(&cbConqInfo->lockword);

    /* Set up last fired direction. */
    cbShips[snum].lastphase = dir;

    /* Start phaser fuse. */
    cbShips[snum].pfuse = PHASER_TENTHS;

    /* See what we can hit. */
    for ( k = 0; k < cbLimits.maxShips(); k++ )
        if ( cbShips[k].status == SS_LIVE && k != snum )
            if ( satwar(snum, k ) )
            {
                dis = dist( cbShips[snum].x, cbShips[snum].y, cbShips[k].x, cbShips[k].y );
                if ( dis <= PHASER_DIST )
                {
                    ang = utAngle( cbShips[snum].x, cbShips[snum].y, cbShips[k].x, cbShips[k].y );
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

    return ( true );

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
int clbRegister( const std::string& lname, const std::string& rname,
                 int team, int *unum )
{
    int i, j;

    cbLock(&cbConqInfo->lockword);
    for ( i = 0; i < cbLimits.maxUsers(); i++ )
        if ( !ULIVE(i) )
        {
            // clear all user flags
            cbUsers[i].flags = USER_F_LIVE; /* only for now */
            cbUsers[i].opFlags = USER_OP_NONE;

            cbUnlock(&cbConqInfo->lockword);

            cbUsers[i].rating = 0.0;
            cbUsers[i].team = team;

            // default to a normal client player
            cbUsers[i].type = USERTYPE_NORMAL;

            for ( j = 0; j < MAXUSTATS; j = j + 1 )
                cbUsers[i].stats[j] = 0;

            for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
                cbUsers[i].war[j] = true;
            cbUsers[i].war[cbUsers[i].team] = false;

            cbUsers[i].lastentry = 0;	/* never */
            utStrncpy( cbUsers[i].username, lname.c_str(), MAX_USERNAME );
            utStrncpy( cbUsers[i].alias, rname.c_str(), MAX_USERNAME );
            *unum = i;
            return ( true );
        }

    cbUnlock(&cbConqInfo->lockword);

    return ( false );

}


/*  resign - remove a user from the user list (DOES LOCKING) */
/*  SYNOPSIS */
/*    int unum */
/*    clbResign( unum ) */
void clbResign( int unum, bool isoper )
{
    if ( !(unum >= 0 && unum < cbLimits.maxUsers()) )
        return;

    cbLock(&cbConqInfo->lockword);

    UFCLR(unum, USER_F_LIVE);

    for ( int i = 0; i < cbLimits.maxHist(); i++ )
        if ( unum == cbHistory[i].unum )
            cbHistory[i].unum = -1;

    cbUnlock(&cbConqInfo->lockword);

    if (!isoper)
        utLog("INFO: %s (%s) has resigned",
              cbUsers[unum].username, cbUsers[unum].alias);

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
    std::string buf;
    int oteam, didgeno;
    int rv = -1;

    oteam = cbPlanets[pnum].team;
    cbPlanets[pnum].team = cbShips[snum].team;
    cbPlanets[pnum].armies = 1;
    cbShips[snum].kills = cbShips[snum].kills + PLANET_KILLS;
    cbUsers[cbShips[snum].unum].stats[USTAT_CONQPLANETS] += 1;
    cbTeams[cbShips[snum].team].stats[TSTAT_CONQPLANETS] += 1;


    /* Check here for genocides */

    if ( oteam != TEAM_SELFRULED && oteam != TEAM_NOTEAM )
    {
        /* Check whether that was the last planet owned by the vanquished. */

        didgeno = 1;

        for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        {
            if ( PVISIBLE(i) && (cbPlanets[i].team == oteam)
                 && cbPlanets[i].armies > 0)
            {
                // nope
                didgeno = 0;
                break;
            }
        }
        /* Yes. */
        if ( didgeno && (snum >= 0 && snum < cbLimits.maxShips()) )
        {
            rv = oteam;
            cbUsers[cbShips[snum].unum].stats[USTAT_GENOCIDE] += 1;
            cbTeams[cbShips[snum].team].stats[TSTAT_GENOCIDE] += 1;

            buf = fmt::format("{}{} ({}) genocided the {} team!",
                              cbTeams[cbShips[snum].team].teamchar,
                              snum,
                              cbShips[snum].alias,
                              cbTeams[oteam].name);

            clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, buf.c_str());
            utLog("%s: GENO: %s", __FUNCTION__, buf.c_str());

            // Here we do a check for vacant ships holding the vanquished
            // armies.  This is a trick often used to avoid having to do a
            // coup.  Simply beam up some armies, then go somewhere and
            // cloak, then quit (VACANT).  It's also a lame cheat.
            //
            // So what we do here is if VACANT ships are allowed, we'll go
            // through the ship list, looking for vacant ships carrying
            // the vanquished armies.  We'll log it, then set their army
            // count to 0.
            if (SysConf.AllowVacant)
            {
                for (int i=0; i<cbLimits.maxShips(); i++)
                {
                    if (SVACANT(i) && cbShips[i].team == oteam
                        && cbShips[i].armies > 0)
                    {
                        utLog("%s: Deleted %d armies on vacant ship %d",
                              __FUNCTION__,
                              cbShips[i].armies,
                              i);
                        cbShips[i].armies = 0;
                    }
                }
            }

        }

    }


    buf = fmt::format("All hail the liberating {} armies.  Thanks, ",
                      cbTeams[cbShips[snum].team].name );
    utAppendShip(buf , snum) ;
    buf += '!';

    /* Check whether the universe has been conquered. */
    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        if ( PCORE(i) && PVISIBLE(i) )
            if ( cbPlanets[i].team != cbShips[snum].team )
            {
                /* No. */
                clbStoreMsg( MSG_FROM_PLANET, pnum,
                             MSG_TO_TEAM, cbShips[snum].team, buf.c_str());
                return rv;
            }
    /* Yes! */
    utFormatTime( cbConqInfo->conqtime, 0 );
    utStrncpy( cbConqInfo->conqueror, cbShips[snum].alias, MAX_USERNAME );
    cbConqInfo->lastwords[0] = 0;
    cbUsers[cbShips[snum].unum].stats[USTAT_CONQUERS] += 1;
    cbTeams[cbShips[snum].team].stats[TSTAT_CONQUERS] += 1;
    utStrncpy( cbConqInfo->conqteam, cbTeams[cbShips[snum].team].name, MAXTEAMNAME );

    utLog("INFO: %s (%s) has Conquered the Universe!",
          cbUsers[cbShips[snum].unum].username,
          cbShips[snum].alias);

    clbIKill( snum, KB_CONQUER, 0 );
    for ( i = 0; i < cbLimits.maxShips(); i++ )
        if ( cbShips[i].status == SS_LIVE )
            clbIKill( i, KB_NEWGAME, 0 );

    cbUnlock(&cbConqInfo->lockword);
    clbInitGame(false);
    cbLock(&cbConqInfo->lockword);

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
void clbUserline( int unum, int snum, std::string& buf,
                  bool showgods, bool showteam )
{
    int team;
    static const std::string hd1 =
        " name          pseudonym           team  skill  wins  loss mxkls ships     time";

    if ( unum < 0 || unum >= cbLimits.maxUsers() )
    {
        buf = hd1;
        return;
    }

    if ( !ULIVE(unum) )
    {
        buf.clear();
        return;
    }

    // default, (normal user) is blank
    std::string uTypeAndUname;

    if (cbUsers[unum].type == USERTYPE_BUILTIN)
    {
        // mutually exclusive to oper, which implies USERTYPE_NORMAL
        uTypeAndUname = '-';
    }
    else
    {
        // operator
        if (isagod(unum))
            uTypeAndUname = '+';
    }

    uTypeAndUname += cbUsers[unum].username;

    std::string name;
    /* If we were given a valid ship number, use it's information. */
    if ( snum >= 0 && snum < cbLimits.maxShips() )
    {
        name = cbShips[snum].alias;
        team = cbShips[snum].team;
    }
    else
    {

        name = cbUsers[unum].alias;
        team = cbUsers[unum].team;
    }

    /* Figure out which team he's on. */
    char ch = cbTeams[team].teamchar;


    std::string usrData = fmt::format("{:<12.12s}   {:<21.21s}  {} {:6.1f}",
                                      uTypeAndUname,
                                      name,
                                      ch,
                                      cbUsers[unum].rating);

    std::string timstr;
    utFormatMinutes( (cbUsers[unum].stats[USTAT_SECONDS] + 30 ) / 60,
                     timstr );

    buf = fmt::format("{} {:5d} {:5d} {:5d} {:5d} {:>8s}",
                      usrData,
                      cbUsers[unum].stats[USTAT_WINS],
                      cbUsers[unum].stats[USTAT_LOSSES],
                      cbUsers[unum].stats[USTAT_MAXKILLS],
                      cbUsers[unum].stats[USTAT_ENTRIES],
                      timstr );

    return;
}



/*  statline - format a user stat line */
/*  SYNOPSIS */
/*    int unum */
/*    char buf() */
/*    clbStatline( unum, buf ) */
void clbStatline( int unum, std::string& buf )
{
    int i, j;
    char ch;
    static const std::string hd1 = "planets      armies phaser  torps";
    static const std::string hd2 =
        "name         cpu  conq coup geno  taken bombed/shot  shots  fired    last entry";

    if ( unum < 0 || unum >= cbLimits.maxUsers() )
    {
        // caller is requesting headers (or somebody really screwed up)
        if (unum == STATLINE_HDR1)
            buf = hd1;
        else if (unum == STATLINE_HDR2)
            buf = hd2;
        else
            buf.clear();
        return;
    }

    if ( !ULIVE(unum) )
    {
        buf.clear();
        return;
    }

    std::string percent;
    if ( cbUsers[unum].stats[USTAT_SECONDS] == 0 )
        percent = "   -";
    else
    {
        i = 1000 * cbUsers[unum].stats[USTAT_CPUSECONDS]
            / cbUsers[unum].stats[USTAT_SECONDS];
        percent = fmt::format("{:3d}%", (i + 5) / 10);
    }

    std::string tmp;
    tmp = fmt::format("{:<12.12s}{:4s}  {:4d} {:4d} {:4d}",
                      cbUsers[unum].username,
                      percent,
                      cbUsers[unum].stats[USTAT_CONQUERS],
                      cbUsers[unum].stats[USTAT_COUPS],
                      cbUsers[unum].stats[USTAT_GENOCIDE] );

    buf = fmt::format("{} {:6d} {:6d} {:4d} {:6d}  {:5d}",
                      tmp,
                      cbUsers[unum].stats[USTAT_CONQPLANETS],
                      cbUsers[unum].stats[USTAT_ARMBOMB],
                      cbUsers[unum].stats[USTAT_ARMSHIP],
                      cbUsers[unum].stats[USTAT_PHASERS],
                      cbUsers[unum].stats[USTAT_TORPS] );

    if (cbUsers[unum].lastentry == 0) /* never */
    {
        tmp = fmt::format(" {:>13.13s}", "never");
        buf += tmp;
    }
    else
    {				/* format it properly */
        utFormatTime(tmp, cbUsers[unum].lastentry);

        // remove the seconds (:nn), after skipping over the first ':'
        size_t pos = tmp.find(":", 4);
        tmp.erase(pos, 3);
        tmp = fmt::format(" {:>13.13s}", tmp);

        buf += tmp;
    }

    return;
}

void clbShipline( int snum, bool isoper, std::string& buf )
{
    static const std::string hd =
        "ship  name          pseudonym              kills     type";
    static const std::string hd_oper =
        "ship  name          pseudonym              kills      pid";

    if ( snum < 0 || snum >= cbLimits.maxShips() )
    {
        // caller is requesting headers
        if (snum == SHIPLINE_HDR)
            buf = hd;
        else if (snum == SHIPLINE_HDR_OPER)
            buf = hd_oper;
        else
            buf.clear();
        return;
    }

    buf.clear();

    std::string sbuf;
    utAppendShip(sbuf, snum) ;
    sbuf += " ";
    sbuf += cbShipTypes[cbShips[snum].shiptype].name[0];

    int unum = cbShips[snum].unum;
    if ( unum >= 0 && unum < cbLimits.maxUsers())
    {
        std::string pidbuf;
        if (SROBOT(snum)) /* robot */
            pidbuf = " ROBOT";
        else if (SVACANT(snum))
            pidbuf = "VACANT";
        else
        {
            if (isoper)
                pidbuf = fmt::format("{:6d}", cbShips[snum].pid);
            else
                pidbuf = "  LIVE";
        }

        buf =
            fmt::format("{:<5s} {:<13.13s} {:<21.21s} "
                        "{:6.1f}   {:6s}",
                        sbuf,
                        cbUsers[unum].username,
                        cbShips[snum].alias,
                        (cbShips[snum].kills + cbShips[snum].strkills),
                        pidbuf);
    }
    else
        buf = fmt::format("{:<5s} {:<13.13s} {:<21.21s} "
                          "{:8s}   {:6s}",
                          sbuf,
                          "",
                          "",
                          "",
                          "");

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
    int didgeno = false;

    oteam = cbPlanets[pnum].team;
    cbPlanets[pnum].team = TEAM_NOTEAM;
    cbPlanets[pnum].armies = 0;

    /* Make the planet not scanned. */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        cbPlanets[pnum].scanned[i] = false;

    /* check for genos here */
    if ( oteam != TEAM_SELFRULED && oteam != TEAM_NOTEAM )
    {
        didgeno = true;

        /* Check whether that was the last planet owned by the vanquished. */
        for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        {
            if ( PVISIBLE(i) && (cbPlanets[i].team == oteam) &&
                 cbPlanets[i].armies > 0)
            {
                didgeno = false;
                break;
            }
        }

        /* Yes. */
        if (didgeno && (snum >= 0 && snum < cbLimits.maxShips()))
        {
            cbTeams[oteam].couptime = rndint( MIN_COUP_MINUTES, MAX_COUP_MINUTES );
            TEAM_CLRF(oteam, TEAM_F_COUPINFO);  // lost coup info

            if ( snum >= 0 && snum < cbLimits.maxShips() )
            {
                cbUsers[cbShips[snum].unum].stats[USTAT_GENOCIDE] += 1;
                cbTeams[cbShips[snum].team].stats[TSTAT_GENOCIDE] += 1;

                std::string buf;
                buf = fmt::format("{}{} ({}) genocided the {} team!",
                                  cbTeams[cbShips[snum].team].teamchar,
                                  snum,
                                  cbShips[snum].alias,
                                  cbTeams[oteam].name);

                clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, buf.c_str());
                utLog("GENO: %s", buf.c_str());
            }
        }
    }

    if (didgeno)
        return oteam;
    else
        return -1;

}

std::string clbWarPrompt(int snum, int twar[])
{
    char peace[NUMPLAYERTEAMS], war[NUMPLAYERTEAMS];

    for ( int i=0; i<NUMPLAYERTEAMS; i++ )
    {
        if ( twar[i] )
        {
            char ch;

            if ( cbShips[snum].rwar[i] )
                ch = cbTeams[i].teamchar;
            else
                ch = (char)tolower(cbTeams[i].teamchar);

            peace[i] = ' ';
            war[i] = ch;
        }
        else
        {
            peace[i] = (char)tolower(cbTeams[i].teamchar);
            war[i] = ' ';
        }

    }

    return fmt::format("Press [TAB] when done, [ESC] to abort:  "
                       "Peace: {} {} {} {}  War: {} {} {} {}",
                       peace[0],
                       peace[1],
                       peace[2],
                       peace[3],
                       war[0],
                       war[1],
                       war[2],
                       war[3]);
}

/*  canread - determine if a message is readable */
/*  SYNOPSIS */
/*    int ok, canread */
/*    int snum, msgnum */
/*    ok = clbCanRead( snum, msgnum ) */
int clbCanRead( int snum, int msgnum )
{
    msgFrom_t from = cbMsgs[msgnum].from;
    uint16_t fromDetail = cbMsgs[msgnum].fromDetail;
    msgTo_t to = cbMsgs[msgnum].to;
    uint16_t toDetail = cbMsgs[msgnum].toDetail;

    if (from == MSG_FROM_NOONE && to == MSG_TO_NOONE)
    {				/* uninitialized msgs */
        return(false);		/* no point in reading it */
    }

    /* If we're GOD, we can read it, unless it's a feedback, tersable,
     * or robot insult */
    if (snum < 0)
    {
        if (cbMsgs[msgnum].flags & (MSG_FLAGS_FEEDBACK
                                    | MSG_FLAGS_TERSABLE
                                    | MSG_FLAGS_ROBOT))
            return false;
        else
            return true;
    }

    // snum should never be <0 at this point

    /* It's to us. */
    if ( to == MSG_TO_SHIP && (int)toDetail == snum )
        return(true);

    /* It's from us */
    if (from == MSG_FROM_SHIP && (int)fromDetail == snum)
        return(true);

    /* if it's to god, or implementors and we are an oper... */
    if ((to == MSG_TO_GOD || to == MSG_TO_IMPLEMENTORS)
        && UISOPER(cbShips[snum].unum))
        return true;

    /* It's to everybody. */
    if ( to == MSG_TO_ALL )
        return(true);

    /* Only check these if we're a ship. */
    if ( snum >= 0 && snum < cbLimits.maxShips() )
    {
        /* We can only read team messages if we're not self-war. */
        if ( to == MSG_TO_TEAM
             && toDetail < NUMALLTEAMS
             && ((int)toDetail == cbShips[snum].team)
             && !selfwar(snum) )
            return ( true );

        /* see if it's a message to friendly ships from another ship */

        if (to == MSG_TO_FRIENDLY
            && from == MSG_FROM_SHIP
            && fromDetail < cbLimits.maxShips())
	{
            if (cbShips[snum].war[cbShips[fromDetail].team] == false &&
                cbShips[fromDetail].war[cbShips[snum].team] == false)
                return true;
	}
    }

    /* If we got here, we can't read it. */
    return ( false );

}


/*  clearships - reset ships and torpedoes */
/*  SYNOPSIS */
/*    clearships */
void clbClearShips(void)
{

    int i;

    for ( i = 0; i < cbLimits.maxShips(); i++ )
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
    *col = iround( (Context.maxcol-STAT_COLS)/2
                   + (x-cenx) / scale * WIDTH_FAC ) + STAT_COLS;

    *lin = iround( (DISPLAY_LINS/2+1) - (y-ceny) / scale );
    if ( *lin < 0 || *lin > DISPLAY_LINS || *col <= STAT_COLS
         || *col > Context.maxcol )
        return ( false );

    return ( true );

}


/*  doomfind - find a planet or ship for the doomsday machine to head for */
/*  SYNOPSIS */
/*    doomfind */
void clbDoomFind(void)
{
    int i;
    real taste = 0.0, tastiness = 0.0;

    cbDoomsday->lock = LOCK_NONE;
    cbDoomsday->lockDetail = 0;
    DOOMCLR(DOOM_F_ATTACKING); // not attacking while looking for a
                               // new target

    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        if ( PVISIBLE(i) )
            if ( cbPlanets[i].armies > 0 && cbPlanets[i].team != TEAM_NOTEAM )
            {
                taste = cbPlanets[i].armies * BOMBARD_KILLS / dist(cbDoomsday->x, cbDoomsday->y, cbPlanets[i].x, cbPlanets[i].y);
                if ( taste > tastiness )
                {
                    tastiness = taste;
                    cbDoomsday->lock = LOCK_PLANET;
                    cbDoomsday->lockDetail = i;
                }
            }

    for ( i = 0; i < cbLimits.maxShips(); i++ )
        if ( cbShips[i].status == SS_LIVE)
        {
            taste = ( 1.0 +
                      cbShips[i].kills * KILLS_KILLS +
                      cbShips[i].armies * ARMY_KILLS ) / dist(cbDoomsday->x, cbDoomsday->y, cbShips[i].x, cbShips[i].y);
            if ( taste > tastiness )
            {
                tastiness = taste;
                cbDoomsday->lock = LOCK_SHIP;
                cbDoomsday->lockDetail = i;
            }
        }

    if ( cbDoomsday->lock == LOCK_PLANET )
        cbDoomsday->heading = utAngle( cbDoomsday->x, cbDoomsday->y, cbPlanets[cbDoomsday->lockDetail].x, cbPlanets[cbDoomsday->lockDetail].y );
    else if ( cbDoomsday->lock == LOCK_SHIP )
        cbDoomsday->heading = utAngle( cbDoomsday->x, cbDoomsday->y, cbShips[cbDoomsday->lockDetail].x, cbShips[cbDoomsday->lockDetail].y );

    return;

}


/*  doomsday - start the doomsday device */
/*  SYNOPSIS */
/*    doomsday */
void clbDoomsday(void)
{

    cbDoomsday->heading = rnduni( 0.0, 360.0 );
    cbDoomsday->x = DOOMSDAY_START_DIST * cosd(cbDoomsday->heading);
    cbDoomsday->y = DOOMSDAY_START_DIST * sind(cbDoomsday->heading);
    clbDoomFind();
    DOOMSET(DOOM_F_LIVE);

    return;

}


/*  findorbit - find a planet for a ship to orbit */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    int flag, findorbit */
/*    flag = clbFindOrbit( snum, pnum ) */
bool clbFindOrbit( int snum, int *pnum )
{
    int i;

    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        if ( PVISIBLE(i) &&
             ( dist( cbShips[snum].x, cbShips[snum].y,
                     cbPlanets[i].x,
                     cbPlanets[i].y ) <= (real)cbPlanets[i].size ) )
        {
            *pnum = i;
            return ( true );
        }

    /* Didn't find one. */
    return ( false );

}


/*  findship - find a free ship and reserve it (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    int truth, findship */
/*    truth = clbFindShip( snum ) */
int clbFindShip( int *snum )
{
    int i;
    std::vector<int> vacantShips;

    /* maybe free up some slots... */
    clbCheckShips(false);

    cbLock(&cbConqInfo->lockword);
    *snum = -1;

    for ( i = 0; i < cbLimits.maxShips(); i++ )
    {
        /* first, look for reserved ships that have no valid pid */
        if ( cbShips[i].status == SS_RESERVED )
            if (!checkPID(cbShips[i].pid))
            {
                clbZeroShip(i);  /* no-one there, turn it off */
                utLog("INFO: clbFindShip(): turned off reserved ship %d\n",
                      i);
            }

        /* if it's vacant and not an oper, save it for later */
        if (cbShips[i].status == SS_LIVE &&
            SVACANT(i) &&
            !UISOPER(cbShips[i].unum))
            vacantShips.push_back(i);

        /* if it's off, grab it */
        if ( cbShips[i].status == SS_OFF )
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
        if (vacantShips.size())
        {
            if (vacantShips.size() == 1)
                *snum = vacantShips[0];
            else
                *snum = vacantShips[rndint(0, vacantShips.size() - 1)];

            utLog("INFO: clbFindShip: stealing vacant ship %d", *snum);
            clbIKill( *snum,  KB_GOD, 0 );
        }
    }

    if (*snum != -1)
    {
        /* we found one, so complete the takeover */
        utLog("%s: Grabbing ship slot %d", __FUNCTION__, *snum);
        clbZeroShip( *snum );
        cbShips[*snum].status = SS_RESERVED;
        cbShips[*snum].lastmsg = LMSG_NEEDINIT;
        cbShips[*snum].sdfuse = -TIMEOUT_PLAYER;
        cbShips[*snum].ctime = 0;
        cbShips[*snum].etime = 0;
        cbShips[*snum].cacc = 0;
        cbShips[*snum].eacc = 0;
    }

    cbUnlock(&cbConqInfo->lockword);

    return ( (*snum != -1) ? true : false );

}


/*  findspecial - search for nearest some-thing */
/*  SYNOPSIS */
/*    int flag, findspecial */
/*    int snum, token, count, sorpnum, xsorpnum */
/*    flag = clbFindSpecial( snum, token, count, sorpnum, xsorpnum ) */
int clbFindSpecial( int snum, int token, int count, int *sorpnum,
                    int *xsorpnum )
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
        for ( i = 0; i < cbLimits.maxShips(); i++ )
        {
            if ( i != snum && cbShips[i].status == SS_LIVE )
            {
                switch ( token )
                {
                case SPECIAL_ENEMYSHIP:
                    valid = satwar(snum, i);
                    break;
                case SPECIAL_SHIP:
                    valid = true;
                    break;
                case SPECIAL_TEAMSHIP:
                    valid = ( cbShips[i].team == cbShips[snum].team &&
                              ! satwar(snum, i) );
                    break;
                default:
                    return ( false );		/* this can't happen */
                }
                if ( valid )
                {
                    if (SCLOAKED(i) && cbShips[i].warp == 0.0 &&
                        satwar(snum, i) &&
                        SROBOT(snum))
                        continue; /* nobody here but us chickens... */
                    td = dist(cbShips[snum].x, cbShips[snum].y, cbShips[i].x, cbShips[i].y);
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
        if (cbShips[snum].team < NUMPLAYERTEAMS) // better be...
            *sorpnum = cbTeams[cbShips[snum].team].homeplanet;
        else
            return false;

        break;
    case SPECIAL_WEAKPLANET:
        /* Weakest non-team planet. */
        for ( i = 0; i < cbLimits.maxPlanets(); i++ )
	{
            /* Only can look for "real" planets. */
            if ( ! PVISIBLE(i) )
                continue; /* jet next;*/
            /* Ignore suns and moons. */
            if ( cbPlanets[i].type == PLANET_SUN || cbPlanets[i].type == PLANET_MOON )
                continue;

            valid = ( cbPlanets[i].scanned[cbShips[snum].team] &&
                      cbPlanets[i].team != cbShips[snum].team );

            /* Handle army threshold logic. */
            if ( valid )
                valid = ( cbPlanets[i].armies >= count );

            if ( valid )
	    {
                ta = cbPlanets[i].armies;
                tu = cbPlanets[i].uninhabtime;
                td = dist(cbShips[snum].x, cbShips[snum].y, cbPlanets[i].x, cbPlanets[i].y);

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
        peaceful = true;
        for ( i = 0; i < NUMPLAYERTEAMS; i++ )
            if ( cbShips[snum].war[i] )
            {
                peaceful = false;
                break;
            }

        /* Loop through the planets. */
        for ( i = 0; i < cbLimits.maxPlanets(); i++ )
	{
            /* Only can look for "real" planets. */
            if ( ! PVISIBLE(i) )
                continue;
            /* Ignore suns and moons. */
            if ( cbPlanets[i].type == PLANET_SUN || cbPlanets[i].type == PLANET_MOON )
                continue;
            switch ( token )
	    {
	    case SPECIAL_ARMYPLANET:
                valid = ( cbPlanets[i].team == cbShips[snum].team );
                break;
	    case SPECIAL_ENEMYPLANET:
                valid = ( ! cbPlanets[i].scanned[cbShips[snum].team] ||
                          ( cbPlanets[i].armies > 0 &&
                            clbSPWar( snum, i ) &&
                            cbPlanets[i].type != PLANET_MOON ) );
                break;
	    case SPECIAL_FUELPLANET:
                valid = ( ( cbPlanets[i].scanned[cbShips[snum].team] || peaceful ) &&
                          ! clbSPWar( snum, i ) &&
                          cbPlanets[i].armies > 0 &&
                          cbPlanets[i].type == PLANET_CLASSM );
                break;
	    case SPECIAL_PLANET:
                valid = true;
                break;
	    case SPECIAL_REPAIRPLANET:
                valid = ( ( cbPlanets[i].scanned[cbShips[snum].team] || peaceful ) &&
                          ! clbSPWar( snum, i ) &&
                          cbPlanets[i].armies > 0 &&
                          cbPlanets[i].type != PLANET_MOON );
                break;
	    case SPECIAL_TEAMPLANET:
                valid = ( cbPlanets[i].team == cbShips[snum].team );
                break;
	    default:
                return ( false );		/* this can't happen */
	    }
            /* Handle army threshold logic. */
            if ( valid )
	    {
                switch ( token )
		{
		case SPECIAL_ARMYPLANET:
                    valid = ( ( cbPlanets[i].armies - 3 ) >= count );
                    break;
		case SPECIAL_PLANET:
		case SPECIAL_ENEMYPLANET:
                    valid = ( ! cbPlanets[i].scanned[cbShips[snum].team] ||
                              cbPlanets[i].armies >= count );
                    break;
		case SPECIAL_FUELPLANET:
		case SPECIAL_REPAIRPLANET:
		case SPECIAL_TEAMPLANET:
                    valid = ( cbPlanets[i].armies >= count );
                    break;
		default:
                    return ( false );	/* this can't happen */
		}
	    }
            if ( valid )
	    {
                td = dist(cbShips[snum].x, cbShips[snum].y, cbPlanets[i].x, cbPlanets[i].y);
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
        return ( false );			/* this can't happen */
    }

    return ( *sorpnum != -1 );

}


/*  fixdeltas - update ship dx and dy */
/*  SYNOPSIS */
/*    int snum */
/*    clbFixDeltas( snum ) */
void clbFixDeltas( int snum )
{
    real speed;

    speed = cbShips[snum].warp * MM_PER_SEC_PER_WARP * ITER_SECONDS;
    cbShips[snum].dx = speed * cosd(cbShips[snum].head);
    cbShips[snum].dy = speed * sind(cbShips[snum].head);

    return;

}


/*  gunum - get the user number of the specified user */
/*  SYNOPSIS */
/*    int truth, gunum */
/*    int unum */
/*    char lname() */
/*    truth = clbGetUserNum( unum, lname ) */
bool clbGetUserNum( int *unum, const std::string& lname, userTypes_t ltype )
{
    int i;

    *unum = -1;
    for ( i = 0; i < cbLimits.maxUsers(); i++ )
        if ( ULIVE(i) )
        {
            if ( strcmp( lname.c_str(), cbUsers[i].username ) == 0
                 && (ltype == USERTYPE_ANY || cbUsers[i].type == ltype) )
            {
                *unum = i;
                return ( true );
            }
        }

    return ( false );

}

/*  initeverything - initialize (with extra cheese and tomato) (DOES LOCKING) */
/*  SYNOPSIS */
/*    initeverything */
void clbInitEverything(bool cbIsLocal)
{

    int i, j;

    /* Twiddle the lockword (only for the real deal). */
    if (!cbIsLocal)
    {
        cbUnlock(&cbConqInfo->lockword);
        cbUnlock(&cbConqInfo->lockmesg);
    }

    /* Zero EVERYTHING. */
    clbZeroEverything();

    if (!cbIsLocal)
        cbLock(&cbConqInfo->lockword);

    /* reset the lockwords  */
    cbConqInfo->lockword = 0;
    cbConqInfo->lockmesg = 0;

    /* Zero team stats. */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        for ( j = 0; j < MAX_TEAM_STATS; j = j + 1 )
            cbTeams[i].stats[j] = 0;

    /* De-register all users. */
    for ( i = 0; i < cbLimits.maxUsers(); i++ )
    {
        UFCLR(i, USER_F_LIVE);
        cbUsers[i].type = USERTYPE_NORMAL;
    }

    cbConqInfo->celapsedseconds = 0;
    cbConqInfo->ccpuseconds = 0;
    cbConqInfo->delapsedseconds = 0;
    cbConqInfo->dcpuseconds = 0;
    cbConqInfo->relapsedseconds = 0;
    cbConqInfo->rcpuseconds = 0;
    cbConqInfo->raccum = 0;

    utStrncpy( cbConqInfo->lastupchuck, "never", MAXDATESIZE );
    utFormatTime( cbConqInfo->inittime, 0 );
    utFormatTime( cbConqInfo->conqtime, 0 );
    utStrncpy( cbConqInfo->conqueror, "GOD", MAX_USERNAME );
    utStrncpy( cbConqInfo->conqteam, "self ruled", MAXTEAMNAME );
    utStrncpy( cbConqInfo->lastwords, "Let there be light...", MAX_LASTWORDS );

    /* Un-twiddle the lockwords. */
    if (!cbIsLocal)
    {
        cbUnlock(&cbConqInfo->lockword);
        cbUnlock(&cbConqInfo->lockmesg);
    }

    clbInitRobots();
    clbInitUniverse(cbIsLocal);

    return;

}


/*  initgame - initialize the game-permanent variables */
/*  SYNOPSIS */
/*    initgame */
void clbInitGame(bool cbIsLocal)
{
    /* Twiddle the lockword. */
    if (!cbIsLocal)
    {
        cbUnlock(&cbConqInfo->lockword);
        cbLock(&cbConqInfo->lockword);
    }

    /* Driver. */
    cbDriver->drivsecs = 0;

    /* Doomsday machine. */
    DOOMCLR(DOOM_F_LIVE);
    cbDoomsday->x = 0.0;
    cbDoomsday->y = 0.0;
    cbDoomsday->dx = 0.0;
    cbDoomsday->dy = 0.0;
    cbDoomsday->heading = 0.0;
    cbDoomsday->lock = LOCK_NONE;
    cbDoomsday->lockDetail = 0;
    cbDoomsday->eaterType = EATER_DOOMSDAY;
    utStrncpy( cbDoomsday->name, "Doomsday Machine", MAX_USERNAME );

    /* Un-twiddle the lockword. */
    if (!cbIsLocal)
        cbUnlock(&cbConqInfo->lockword);

    /* Set up the physical universe, but only on the server */
    if (!cbIsLocal)
        cqiInitPlanets();

    return;

}


/*  initmsgs - initialize the message data structures */
/*  SYNOPSIS */
/*    initmsgs */
void clbInitMsgs(void)
{
    /* Zero the message buffer. */
    memset(cbMsgs, 0, cbLimits.maxMsgs() * sizeof(Msg_t));

    cbConqInfo->lastmsg = 0;
    cbConqInfo->glastmsg = cbConqInfo->lastmsg;

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
        if ( clbGetUserNum( &unum, x, USERTYPE_BUILTIN ) )              \
            utStrncpy( cbUsers[unum].alias, y, MAX_USERNAME );            \
        else if ( clbRegister( x, y, z, &unum ) )                       \
        {                                                               \
            UFSET(unum, USER_F_ROBOT);                                  \
            cbUsers[unum].type = USERTYPE_BUILTIN; /* always builtin */   \
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
            cbRobot->rstrat[i][j] = trstrat[i][j];

    /* Copy the action vector. */
    for ( i = 0; i < 32; i++ )
        cbRobot->rvec[i] = trvec[i];

    cbConqInfo->externrobots = false;	/* XXX temporary */

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

    /* cbShips[snum].status                 # never modified here */
    cbShips[snum].killedBy = KB_NONE;
    cbShips[snum].killedByDetail = 0;
    /* cbShips[snum].user                   # setup in menu() or newrob() */
    /* cbShips[snum].team                   # setup in menu() or newrob() */
    /* cbShips[snum].pid                    # setup in menu() or newrob() */
    /* cbShips[snum].shiptype               # setup in newship and newrob */

    /* init to 0.0, newrob will init this for robots to make them
     *   stronger, faster.. ;-)
     */
    cbShips[snum].strkills = 0.0;
    cbShips[snum].x = 0.0;
    cbShips[snum].y = 0.0;
    cbShips[snum].dx = 0.0;
    cbShips[snum].dy = 0.0;
    cbShips[snum].head = 0.0;
    cbShips[snum].dhead = 0.0;
    cbShips[snum].warp = 0.0;
    cbShips[snum].dwarp = 0.0;
    cbShips[snum].lock = LOCK_NONE;
    cbShips[snum].lockDetail = 0;
    cbShips[snum].shields = 100.0;
    cbShips[snum].kills = 0.0;
    cbShips[snum].damage = 0.0;
    cbShips[snum].fuel = 999.0;
    cbShips[snum].wtemp = 0.0;
    cbShips[snum].etemp = 0.0;
    cbShips[snum].wfuse = 0;
    cbShips[snum].efuse = 0;
    cbShips[snum].weapalloc = 40;
    cbShips[snum].engalloc = 100 - cbShips[snum].weapalloc;
    cbShips[snum].armies = 0;
    /* cbShips[snum].option                 # setup in menu() */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    {
        /* cbShips[snum].rwar               # setup in menu() or newrob() */
        /* cbShips[snum].war                # setup in menu() or newrob() */
        cbShips[snum].scanned[i] = 0;
    }
    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        cbShips[snum].srpwar[i] = false;
    /* cbShips[snum].sdfuse                 # setup in clbFindShip() */
    cbLock(&cbConqInfo->lockmesg);
    if ( cbShips[snum].lastmsg == LMSG_NEEDINIT )
    {
        cbShips[snum].lastmsg = cbConqInfo->lastmsg;
        cbShips[snum].alastmsg = cbShips[snum].lastmsg;
    }
    cbUnlock(&cbConqInfo->lockmesg);

    SFCLR(snum, SHIP_F_TOWING);
    SFCLR(snum, SHIP_F_TOWEDBY);
    cbShips[snum].towing = 0; // unnecessary, but...
    cbShips[snum].towedby = 0;

    cbShips[snum].lastblast = 0.0;
    cbShips[snum].lastphase = 0.0;
    cbShips[snum].pfuse = 0;
    cbShips[snum].action = 0;

    cbShips[snum].flags = SHIP_F_NONE;
    SFSET(snum, SHIP_F_SHUP);

    /* cbShips[snum].alias                  # setup in menu() or newrob() */

    /* Zero torpedos. */
    for ( i = 0; i < cbLimits.maxTorps(); i++ )
    {
        cbShips[snum].torps[i].status = TS_OFF;
        cbShips[snum].torps[i].fuse = 0;
        cbShips[snum].torps[i].x = 0.0;
        cbShips[snum].torps[i].y = 0.0;
        cbShips[snum].torps[i].dx = 0.0;
        cbShips[snum].torps[i].dy = 0.0;
        cbShips[snum].torps[i].mult = 0.0;
        for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
            cbShips[snum].torps[i].war[j] = false;
    }

    /* Update user some stats. */

    /* time stamp for this entry */
    cbUsers[unum].lastentry = time(0);

    cbUsers[unum].stats[USTAT_ENTRIES] += 1;
    cbTeams[cbShips[snum].team].stats[TSTAT_ENTRIES] += 1;

    return;

}


/*  inituniverse - initialize (without cheese and tomato) (DOES LOCKING) */
/*  SYNOPSIS */
/*    inituniverse */
void clbInitUniverse(bool cbIsLocal)
{

    int i;

    /* Twiddle the lockword. */
    if (!cbIsLocal)
    {
        cbUnlock(&cbConqInfo->lockword);
        cbLock(&cbConqInfo->lockword);
    }

    utStrncpy( cbShipTypes[ST_SCOUT].name, "Scout", MAXSTNAME );
    cbShipTypes[ST_SCOUT].armyMax = 7;
    cbShipTypes[ST_SCOUT].warpMax = 10;
    cbShipTypes[ST_SCOUT].shieldMax = 100;
    cbShipTypes[ST_SCOUT].damageMax = 100;
    cbShipTypes[ST_SCOUT].torpMax = 9;
    cbShipTypes[ST_SCOUT].fuelMax = 999;
    cbShipTypes[ST_SCOUT].engfac = 1.2;
    cbShipTypes[ST_SCOUT].accelfac = 1.6;
    cbShipTypes[ST_SCOUT].weafac = 0.83;
    cbShipTypes[ST_SCOUT].torpwarp = 14.0;
    cbShipTypes[ST_SCOUT].size = 210.0;

    utStrncpy( cbShipTypes[ST_DESTROYER].name, "Destroyer", MAXSTNAME );
    cbShipTypes[ST_DESTROYER].armyMax = 9;
    cbShipTypes[ST_DESTROYER].warpMax = 9;
    cbShipTypes[ST_DESTROYER].shieldMax = 100;
    cbShipTypes[ST_DESTROYER].damageMax = 100;
    cbShipTypes[ST_DESTROYER].torpMax = 9;
    cbShipTypes[ST_DESTROYER].fuelMax = 999;
    cbShipTypes[ST_DESTROYER].engfac = 1.0;
    cbShipTypes[ST_DESTROYER].accelfac = 1.0;
    cbShipTypes[ST_DESTROYER].weafac = 1.0;
    cbShipTypes[ST_DESTROYER].torpwarp = 12.0;
    cbShipTypes[ST_DESTROYER].size = 225.0;

    utStrncpy( cbShipTypes[ST_CRUISER].name, "Cruiser", MAXSTNAME );
    cbShipTypes[ST_CRUISER].armyMax = 11;
    cbShipTypes[ST_CRUISER].warpMax = 8;
    cbShipTypes[ST_CRUISER].shieldMax = 100;
    cbShipTypes[ST_CRUISER].damageMax = 100;
    cbShipTypes[ST_CRUISER].torpMax = 9;
    cbShipTypes[ST_CRUISER].fuelMax = 999;
    cbShipTypes[ST_CRUISER].engfac = 0.8;
    cbShipTypes[ST_CRUISER].accelfac = 0.8;
    cbShipTypes[ST_CRUISER].weafac = 1.17;
    cbShipTypes[ST_CRUISER].torpwarp = 10.0;
    cbShipTypes[ST_CRUISER].size = 240.0;

    cbTeams[TEAM_FEDERATION].shiptype = ST_DESTROYER;
    cbTeams[TEAM_KLINGON].shiptype = ST_DESTROYER;
    cbTeams[TEAM_ROMULAN].shiptype = ST_CRUISER;
    cbTeams[TEAM_ORION].shiptype = ST_SCOUT;

    utStrncpy( cbTeams[TEAM_FEDERATION].name, "Federation", MAXTEAMNAME );
    utStrncpy( cbTeams[TEAM_ROMULAN].name, "Romulan", MAXTEAMNAME );
    utStrncpy( cbTeams[TEAM_KLINGON].name, "Klingon", MAXTEAMNAME );
    utStrncpy( cbTeams[TEAM_ORION].name, "Orion", MAXTEAMNAME );
    utStrncpy( cbTeams[TEAM_SELFRULED].name, "self ruled", MAXTEAMNAME );
    utStrncpy( cbTeams[TEAM_NOTEAM].name, "non", MAXTEAMNAME );
    utStrncpy( cbTeams[TEAM_GOD].name, "GOD", MAXTEAMNAME );
    utStrncpy( cbTeams[TEAM_EMPIRE].name, "Empire", MAXTEAMNAME );

    cbConqInfo->chrplanets[PLANET_CLASSM] = 'M';
    cbConqInfo->chrplanets[PLANET_DEAD] = 'D';
    cbConqInfo->chrplanets[PLANET_SUN] = 'S';
    cbConqInfo->chrplanets[PLANET_MOON] = 'm';
    cbConqInfo->chrplanets[PLANET_GHOST] = 'G';
    cbConqInfo->chrplanets[PLANET_CLASSA] = 'A';
    cbConqInfo->chrplanets[PLANET_CLASSO] = 'O';
    cbConqInfo->chrplanets[PLANET_CLASSZ] = 'Z';

    utStrncpy( cbConqInfo->ptname[PLANET_CLASSM], "class M planet",
               MAXPTYPENAME );
    utStrncpy( cbConqInfo->ptname[PLANET_DEAD], "dead planet", MAXPTYPENAME );
    utStrncpy( cbConqInfo->ptname[PLANET_SUN], "sun", MAXPTYPENAME );
    utStrncpy( cbConqInfo->ptname[PLANET_MOON], "moon", MAXPTYPENAME );
    utStrncpy( cbConqInfo->ptname[PLANET_GHOST], "ghost planet", MAXPTYPENAME );
    utStrncpy( cbConqInfo->ptname[PLANET_CLASSA], "class A planet",
               MAXPTYPENAME );
    utStrncpy( cbConqInfo->ptname[PLANET_CLASSO], "class O planet",
               MAXPTYPENAME );
    utStrncpy( cbConqInfo->ptname[PLANET_CLASSZ], "class Z planet",
               MAXPTYPENAME );

    cbTeams[TEAM_FEDERATION].teamchar = 'F';
    cbTeams[TEAM_ROMULAN].teamchar = 'R';
    cbTeams[TEAM_KLINGON].teamchar = 'K';
    cbTeams[TEAM_ORION].teamchar = 'O';
    cbTeams[TEAM_SELFRULED].teamchar = '-';
    cbTeams[TEAM_NOTEAM].teamchar = ' ';
    cbTeams[TEAM_GOD].teamchar = 'G';
    cbTeams[TEAM_EMPIRE].teamchar = 'E';

    cbTeams[TEAM_FEDERATION].torpchar = '*';
    cbTeams[TEAM_ROMULAN].torpchar = '@';
    cbTeams[TEAM_KLINGON].torpchar = '+';
    cbTeams[TEAM_ORION].torpchar = '.';

    /* Initialize driver variables. */
    cbDriver->drivcnt = 0;
    cbDriver->drivowner[0] = 0;

    /* Initialize user history stuff. */
    cbConqInfo->histptr = 0;
    for ( i = 0; i < cbLimits.maxHist(); i++ )
    {
        cbHistory[i].unum = -1;
        cbHistory[i].enterTime = 0;
        cbHistory[i].elapsed = 0;
        cbHistory[i].username[0] = 0;
    }

    /* Un-twiddle the lockword. */
    if (!cbIsLocal)
        cbUnlock(&cbConqInfo->lockword);

    clbInitGame(cbIsLocal);
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
    std::string buf;
    static const std::string atta=" attacking";
    static const std::string armeq=", armies=";

    if ( PVISIBLE(pnum) &&
         cbPlanets[pnum].team != TEAM_SELFRULED &&
         cbPlanets[pnum].team != TEAM_NOTEAM )
    {
        if ( snum < 0 ) // doomsday
	{
            buf = cbDoomsday->name;
            buf += atta;
            buf += armeq;
            buf += std::to_string(cbPlanets[pnum].armies);
            clbStoreMsgf( MSG_FROM_PLANET, pnum,
                          MSG_TO_TEAM, cbPlanets[pnum].team,
                          buf.c_str(), MSG_FLAGS_INTRUDER );
	}
        else if ( cbShips[snum].war[cbPlanets[pnum].team] ) // ship
	{
            buf = "INTRUDER ALERT - ";
            buf += utShipStr(snum);
            buf += atta;
            buf += armeq;
            buf += std::to_string(cbPlanets[pnum].armies);
            clbStoreMsgf( MSG_FROM_PLANET, pnum,
                          MSG_TO_TEAM, cbPlanets[pnum].team,
                          buf.c_str(), MSG_FLAGS_INTRUDER );
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

    cbLock(&cbConqInfo->lockword);
    cbConqInfo->histptr = mod( cbConqInfo->histptr + 1, cbLimits.maxHist() );
    /* time stamp for this entry */
    cbHistory[cbConqInfo->histptr].enterTime = time(0);
    cbHistory[cbConqInfo->histptr].elapsed = (time_t)0;
    cbHistory[cbConqInfo->histptr].unum = unum;
    utStrncpy(cbHistory[cbConqInfo->histptr].username,
              cbUsers[unum].username, MAX_USERNAME);
    hnum = cbConqInfo->histptr;

    cbUnlock(&cbConqInfo->lockword);
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

    x = dwarp - cbShips[snum].warp;
    acc = (real) cbShipTypes[cbShips[snum].shiptype].accelfac *
        (real) engeff( snum ) * ITER_SECONDS;
    if ( acc >= fabs( x ) )
        return ( dwarp );			/* close enough (or equal) */
    else if ( x < 0.0 )
        return ( cbShips[snum].warp - acc );

    return ( cbShips[snum].warp + acc );

}


/*  planmatch - check if a string matches a planet name */
bool clbPlanetMatch( const std::string& str, int *pnum, bool godlike )
{
    for ( *pnum = 0; *pnum < cbLimits.maxPlanets(); *pnum = *pnum + 1 )
        if ( PVISIBLE(*pnum) || godlike )
            if ( utStringMatch( str, cbPlanets[*pnum].name, false ) )
                return ( true );

    return ( false );
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
        cbShips[snum].x = rndnor( basex, smear );
        cbShips[snum].y = rndnor( basey, smear );
        for ( i = 0; i < cbLimits.maxShips(); i++ )
            if ( cbShips[i].status == SS_LIVE )
                if ( satwar( i, snum ) &&
                     i != snum &&
                     dist( cbShips[snum].x, cbShips[snum].y, cbShips[i].x, cbShips[i].y ) <= ENTRY_ENEMY_DIST )
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
              uint16_t toDetail, std::string& buf)
{

    buf.clear();

    /* Format who the message is from. */
    if ( from == MSG_FROM_SHIP && fromDetail < cbLimits.maxShips() )
    {
        buf = utShipStr((int)fromDetail);
    }
    else if ( from == MSG_FROM_PLANET && fromDetail < cbLimits.maxPlanets() )
        buf = cbPlanets[fromDetail].name;
    else
    {
        switch ( from )
         {
         case MSG_FROM_NOONE:
             buf = "nobody";
             break;
         case MSG_FROM_GOD:
             buf = "GOD";
             break;
         case MSG_FROM_DOOM:
             buf = fmt::format("The {}", cbDoomsday->name);
             break;
         case MSG_FROM_COMP:
             buf = "Comp";
             break;
         default:
             buf = "???";
             break;
         }
    }

    buf += "->";

    /* Format who the message is to. */
    if ( to == MSG_TO_SHIP && toDetail < cbLimits.maxShips() )
        buf += utShipStr((int)toDetail);
    else if ( to == MSG_TO_TEAM && toDetail < NUMPLAYERTEAMS )
    {
        buf += cbTeams[toDetail].teamchar;
    }
    else
    {
        switch ( to )
	{
	case MSG_TO_NOONE:
            buf += "nobody";
            break;
	case MSG_TO_GOD:
            buf += "GOD";
            break;
	case MSG_TO_ALL:
            buf += "ALL";
            break;
	case MSG_TO_IMPLEMENTORS:
            buf += "IMPs";
            break;
	case MSG_TO_FRIENDLY:
            buf += "FRIEND";
            break;
	default:
            buf += "???";
            break;
	}
    }

    return(true);
}


// sort in ascending order based on name
static bool cmpplanet(int cmp1, int cmp2)
{
    return std::string(cbPlanets[cmp1].name)
        < std::string(cbPlanets[cmp2].name);
}

/*  sortplanets - sort planets by name */
/*  SYNOPSIS */
/*    int sv(MAXPLANETS) */
/*    clbSortPlanets( sv ) */
/* This routine ASSUMES that the sort vector is initialized, */
/* for the reason that it is fastest to sort a list that is */
/* already sorted. */
void clbSortPlanets(std::vector<int>& sv)
{
    std::sort(sv.begin(), sv.end(), cmpplanet);

    return;
}

// sort in descending order based on rating
static bool cmpuser(int cmp1, int cmp2)
{
    return cbUsers[cmp1].rating > cbUsers[cmp2].rating;
}

/*  sortusers - sort users by skill */
void clbSortUsers(std::vector<int>& uv)
{
    std::sort(uv.begin(), uv.end(), cmpuser);

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
        return ( false );		/* can't be at war unless it's real */
    else if ( cbPlanets[pnum].type == PLANET_SUN )
        return ( true );		/* always at war with suns */
    else if ( cbPlanets[pnum].type == PLANET_MOON )
        return ( false );		/* never at war with moons */
    else if ( cbPlanets[pnum].armies <= 0 )
        return ( false );		/* can't have war without armies */
    else switch ( cbPlanets[pnum].team )	/* jet added breaks */
         {
         case TEAM_FEDERATION:
         case TEAM_ROMULAN:
         case TEAM_KLINGON:
         case TEAM_ORION:
             if ( cbPlanets[pnum].team == cbShips[snum].team )
                 return ( false );
             else
                 return ( cbShips[snum].war[cbPlanets[pnum].team] );
             break;
         default:
             return ( cbShips[snum].srpwar[pnum] );
         }

}


/*  stillalive - determine if a ship is still alive */
int clbStillAlive( int snum )
{
    if (snum < 0 || snum >= cbLimits.maxShips())
        return(true);

    if ( cbShips[snum].status == SS_RESERVED
         || cbShips[snum].status == SS_ENTERING )
        return ( true );

    return ( cbShips[snum].status == SS_LIVE );
}

/* wrapper for stormsg.  Most routines just use this version. */
void clbStoreMsg( msgFrom_t from, uint16_t fromDetail,
                  msgTo_t to, uint16_t toDetail,
                  const std::string& msg )
{
    clbStoreMsgf(from, fromDetail, to, toDetail, msg, MSG_FLAGS_NONE);
    return;
}

/*  stormsgf - store a message in the msg buffer with flags (DOES LOCKING) */
void clbStoreMsgf( msgFrom_t from, uint16_t fromDetail,
                   msgTo_t to, uint16_t toDetail,
                   const std::string& msg, unsigned char flags )
{
    int nlastmsg, i;

    /* don't do this if invalid common block */
    if (*cbRevision != COMMONSTAMP)
        return;

    // do some basic checks on the to and from
    if (from >= MSG_FROM_MAX || to >= MSG_TO_MAX)
    {
        utLog("%s: In valid to or from enumerants: to %d from %d",
              __FUNCTION__, to, from);
        return;
    }

    /* don't bother with tersables/feedbacks to robots */
    if (to == MSG_TO_SHIP && toDetail < cbLimits.maxShips() && SROBOT(toDetail))
        if (flags & (MSG_FLAGS_TERSABLE | MSG_FLAGS_FEEDBACK))
            return;

    cbLock(&cbConqInfo->lockmesg);

    nlastmsg = mod( cbConqInfo->lastmsg + 1, cbLimits.maxMsgs() );
    utStrncpy( cbMsgs[nlastmsg].msgbuf, msg.c_str(), MESSAGE_SIZE );
    cbMsgs[nlastmsg].from = from;
    cbMsgs[nlastmsg].fromDetail = fromDetail;
    cbMsgs[nlastmsg].to = to;
    cbMsgs[nlastmsg].toDetail = toDetail;
    cbMsgs[nlastmsg].flags = flags;
    cbConqInfo->lastmsg = nlastmsg;

    /* Remove allowable last message restrictions. */
    for ( i = 0; i < cbLimits.maxShips(); i++ )
        if ( nlastmsg == cbShips[i].alastmsg )
            cbShips[i].alastmsg = LMSG_READALL;

    cbUnlock(&cbConqInfo->lockmesg);

    if (SysConf.LogMessages == true || to == MSG_TO_GOD
        || from == MSG_FROM_GOD)
    {
        std::string fbuf;
        clbFmtMsg(from, fromDetail, to, toDetail, fbuf);
        utLog("MSG: %s: %s", fbuf.c_str(), msg.c_str());
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
        return ( false );
    if ( weapon )
    {
        if ( cbShips[snum].wfuse > 0 )
            return ( false );
    }
    else
    {
        if ( cbShips[snum].efuse > 0 )
	{
            if (forreal)
                cbShips[snum].dwarp = 0.0;
            return ( false );
	}
    }

    if (forreal)
        cbShips[snum].fuel = cbShips[snum].fuel - fuel;

    if ( cbShips[snum].fuel < 0.0 )
    {
        /* When you're out of gas, you're out of fun... */
        if (forreal)
	{
            cbShips[snum].fuel = 0.0;
            SFCLR(snum, SHIP_F_CLOAKED);
            cbShips[snum].dwarp = 0.0;
	}
        return ( false );
    }
    else if ( cbShips[snum].fuel > 999.0 )
    {
        if (forreal)
            cbShips[snum].fuel = 999.0;
    }

    /* Temperature. */
    if ( weapon )
    {
        if (forreal)
	{
            /* the server will send this message if needed */
            cbShips[snum].wtemp += ((fuel * TEMPFUEL_FAC) / weaeff ( snum ));
            if ( cbShips[snum].wtemp < 0.0 )
                cbShips[snum].wtemp = 0.0;
            else if ( cbShips[snum].wtemp >= 100.0 )
                if ( rnd() < WEAPON_DOWN_PROB )
                {
                    cbShips[snum].wfuse = rndint( MIN_DOWN_FUSE, MAX_DOWN_FUSE );
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
            cbShips[snum].etemp = cbShips[snum].etemp + fuel * TEMPFUEL_FAC / engeff( snum );
            if ( cbShips[snum].etemp < 0.0 )
                cbShips[snum].etemp = 0.0;
            else if ( cbShips[snum].etemp >= 100.0 )
                if ( rnd() < ENGINE_DOWN_PROB )
                {
                    cbShips[snum].efuse = rndint( MIN_DOWN_FUSE, MAX_DOWN_FUSE );
                    clbStoreMsgf( MSG_FROM_COMP, 0,
                                  MSG_TO_SHIP, snum,
                                  "Engines super-heated.",
                                  MSG_FLAGS_TERSABLE);
                }
	}
    }

    return ( true );

}


/*  zeroeverything - initialize the common block to zeros */
/*  SYNOPSIS */
/*    zeroeverything */
void clbZeroEverything(void)
{

    cbZero();

    return;

}


/*  zeroship - turn OFF a ship and its torpedos */
/*  SYNOPSIS */
/*    int snum */
/*    clbZeroShip( snum ) */
void clbZeroShip( int snum )
{
    int i, j;

    cbShips[snum].status = SS_OFF;
    cbShips[snum].killedBy = KB_NONE;
    cbShips[snum].killedByDetail = 0;
    cbShips[snum].unum = -1;
    cbShips[snum].team = TEAM_NOTEAM;
    cbShips[snum].pid = 0;
    cbShips[snum].x = 0.0;
    cbShips[snum].y = 0.0;
    cbShips[snum].dx = 0.0;
    cbShips[snum].dy = 0.0;
    cbShips[snum].head = 0.0;
    cbShips[snum].dhead = 0.0;
    cbShips[snum].warp = 0.0;
    cbShips[snum].dwarp = 0.0;
    cbShips[snum].lock = LOCK_NONE;
    cbShips[snum].lockDetail = 0;
    cbShips[snum].shields = 0.0;
    cbShips[snum].kills = 0.0;
    cbShips[snum].damage = 0.0;
    cbShips[snum].fuel = 0.0;
    cbShips[snum].etemp = 0.0;
    cbShips[snum].wtemp = 0.0;
    cbShips[snum].wfuse = 0;
    cbShips[snum].efuse = 0;
    cbShips[snum].weapalloc = 0;
    cbShips[snum].engalloc = 0;
    cbShips[snum].armies = 0;
    cbShips[snum].shiptype = ST_SCOUT;
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    {
        cbShips[snum].rwar[i] = false;
        cbShips[snum].war[i] = false;
        cbShips[snum].scanned[i] = 0;
    }
    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        cbShips[snum].srpwar[i] = false;
    cbShips[snum].sdfuse = 0;
    cbShips[snum].lastmsg = 0;
    cbShips[snum].alastmsg = 0;
    cbShips[snum].towing = 0;
    cbShips[snum].towedby = 0;
    cbShips[snum].lastblast = 0.0;
    cbShips[snum].lastphase = 0.0;
    cbShips[snum].pfuse = 0;
    cbShips[snum].action = 0;
    for ( i = 0; i < MAX_USERNAME; i++ )
        cbShips[snum].alias[i] = 0;
    cbShips[snum].ctime = 0;
    cbShips[snum].etime = 0;
    cbShips[snum].cacc = 0;
    cbShips[snum].eacc = 0;

    cbShips[snum].flags = SHIP_F_NONE;
    SFSET(snum, SHIP_F_SHUP);

    for ( i = 0; i < cbLimits.maxTorps(); i++ )
    {
        cbShips[snum].torps[i].status = TS_OFF;
        cbShips[snum].torps[i].fuse = 0;
        cbShips[snum].torps[i].x = 0.0;
        cbShips[snum].torps[i].y = 0.0;
        cbShips[snum].torps[i].dx = 0.0;
        cbShips[snum].torps[i].dy = 0.0;
        cbShips[snum].torps[i].mult = 0.0;
        for ( j = 0; j < NUMPLAYERTEAMS; j = j + 1 )
            cbShips[snum].torps[i].war[j] = false;
    }

    return;

}

/* for MINGW - we are only building the client, so the 'username' is
 * irrelevant, so return something useless :)
 */
#if defined(MINGW)
const char *clbGetUserLogname(void)
{
    return "MinGW";
}
#else


/* get the current username.  We really only need to look it up once
   per session... returns a pointer to a static string. */
const char *clbGetUserLogname(void)
{
    struct passwd *pwd = NULL;
    static std::string pwname;

    if (pwname.empty())
    {
        if ((pwd = getpwuid(geteuid())) == NULL)
	{
            utLog("ERROR: clbGetUserLogname(): getpwuid(geteuid()) failed: %s",
                  strerror(errno));

            pwname.clear();
	}
        else
	{
            pwname = pwd->pw_name;
	}
    }

    return pwname.c_str();
}
#endif  /* MINGW */

/*  planetdrive - move the planets based on interval */
/*  SYNOPSIS */
/*    planetdrive */
void clbPlanetDrive(real itersec)
{
    int i;
    real speed;

    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
    {
        /* Advance porbang() if planet is not stationary. */
        if ( cbPlanets[i].primary != i ) // not orbiting itself...
	{

            cbPlanets[i].orbang = utMod360( cbPlanets[i].orbang +
                                          cbPlanets[i].orbvel *
                                          itersec / 60.0 );

            cbPlanets[i].x = clbFixCoord(cbPlanets[cbPlanets[i].primary].x +
                                         cbPlanets[i].orbrad
                                         * cosd(cbPlanets[i].orbang));
            cbPlanets[i].y = clbFixCoord(cbPlanets[cbPlanets[i].primary].y +
                                         cbPlanets[i].orbrad
                                         * sind(cbPlanets[i].orbang));

	}
        else if ( cbPlanets[i].orbvel != 0.0 )
	{
            /* Special hack for planets to move in a straight line. */
            speed = cbPlanets[i].orbvel * MM_PER_SEC_PER_WARP * itersec;
            cbPlanets[i].x =
                clbFixCoord(cbPlanets[i].x + speed * cosd(cbPlanets[i].orbang));
            cbPlanets[i].y =
                clbFixCoord(cbPlanets[i].y + speed * sind(cbPlanets[i].orbang));
	}
    }

    return;

}

/*  torpdrive - move the torps based on interval */
void clbTorpDrive(real itersec)
{
    int s, i, j;
    static bool FirstTime = true;
    static vector<int> ship;

    if (FirstTime)
    {
        FirstTime = false;
        /* Randomize ship ordering. */

        ship.clear();
        for ( s = 0; s < cbLimits.maxShips(); s++ )
            ship.push_back(s);

        for ( s = 0; s < cbLimits.maxShips(); s++ )
        {
            i = rndint( 0, cbLimits.maxShips() - 1);
            j = ship[i];
            ship[i] = ship[s];
            ship[s] = j;

        }
    }

    for ( s = 0; s < cbLimits.maxShips(); s++ )
    {
        i = ship[s];
        if ( cbShips[i].status != SS_OFF )
	{
            for ( j = 0; j < cbLimits.maxTorps(); j = j + 1 )
	    {
                if ( cbShips[i].torps[j].status == TS_LIVE )
		{
                    /* Movement. */
                    cbShips[i].torps[j].x =
                        clbFixCoord(cbShips[i].torps[j].x
                                    + (cbShips[i].torps[j].dx
                                       * (itersec / ITER_SECONDS)));
                    cbShips[i].torps[j].y =
                        clbFixCoord(cbShips[i].torps[j].y
                                    + (cbShips[i].torps[j].dy
                                       * (itersec / ITER_SECONDS)));

		}
            }
        }
    }

    return;
}

/* compute a ship's proper position when orbiting */
void clbAdjOrbitalPosition(int snum)
{
    if (snum >= 0 && snum < cbLimits.maxShips() && cbShips[snum].warp < 0.0
        && cbShips[snum].lock == LOCK_PLANET)
    {
        int pnum = cbShips[snum].lockDetail;

        if ( pnum >= 0 && pnum < cbLimits.maxPlanets() )
        {
            /* ajust the ship's X/Y based on orbit direction and heading */
            if ( cbShips[snum].warp == ORBIT_CW )
            {
                cbShips[snum].x =
                    (real)(cbPlanets[pnum].x + ((real)(cbPlanets[pnum].size / 2)*
                                              cosd(cbShips[snum].head + 90.0)));

                cbShips[snum].y =
                    (real)(cbPlanets[pnum].y + ((real)(cbPlanets[pnum].size / 2)*
                                              sind(cbShips[snum].head + 90.0)));
            }
            else if ( cbShips[snum].warp == ORBIT_CCW )
            {
                /* Orbiting counter-clockwise. */
                cbShips[snum].x =
                    (real)(cbPlanets[pnum].x + ((real)(cbPlanets[pnum].size / 2)*
                                              cosd(cbShips[snum].head - 90.0)));

                cbShips[snum].y =
                    (real)(cbPlanets[pnum].y + ((real)(cbPlanets[pnum].size / 2) *
                                              sind(cbShips[snum].head - 90.0)));

            }
        }
    }
    return;
}

// return time in milliseconds since first call
uint32_t clbGetMillis(void)
{
    static bool firsttime = true;
    static struct timeval start;
    struct timeval elapsed, now;
    uint32_t elapse;

    if (firsttime)
    {
        firsttime = false;

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
    for (i=0; i<cbLimits.maxShips(); i++)
    {
        if (cbShips[i].status == SS_LIVE && !SVACANT(i))
            if (cbShips[i].pid > 0 && !checkPID(cbShips[i].pid))
            {
                utLog("INFO: clbCheckShips(isDriver=%d): marking ship %d as VACANT",
                      isDriver, i);
                SFSET(i, SHIP_F_VACANT);
            }

        /* if the ship is VACANT, and vacants aren't allowed, kill them. */
        if (!SysConf.AllowVacant && SVACANT(i) && cbShips[i].status == SS_LIVE )
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
                cbLock(&cbConqInfo->lockword);
                clbIKill(i,  KB_GOD, 0);
                clbZeroShip( i );
                cbUnlock(&cbConqInfo->lockword);
            }
        }
    }

    return;
}

// Find a team's home sun, if they have one
bool clbFindTeamHomeSun(int team, int *pnum)
{
    if (team < 0 || team >= NUMPLAYERTEAMS || !pnum)
        return false;

    int primary = cbPlanets[cbTeams[team].homeplanet].primary;
    if (cbPlanets[primary].type == PLANET_SUN)
    {
        *pnum = primary;
        return true;
    }

    // if we're here, we failed.
    return false;
}

// here we check x and y coords to ensure they fit into the universe
// (MAX_UNIVERSAL_COORD) wrapping around if necessary...
real clbFixCoord(real coord)
{
    if (coord < -MAX_UNIVERSAL_COORD)
        return MAX_UNIVERSAL_COORD;

    if (coord > MAX_UNIVERSAL_COORD)
        return -MAX_UNIVERSAL_COORD;

    return coord;
}


const vector<int>& clbGetEnabledTeams()
{
    static vector<int> enabledTeams;

    enabledTeams.clear();

    for (int j=0; j<NUMPLAYERTEAMS; j++)
        if (TEAM_ENABLED(j))
            enabledTeams.push_back(j);

    return enabledTeams;
}
