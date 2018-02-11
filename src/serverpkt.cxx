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

#include "conqdef.h"
#include "cb.h"
#include "conf.h"
#include "server.h"
#include "context.h"
#include "serverpkt.h"
#include "conqlb.h"
#include "conqutil.h"
#include "rndlb.h"
#include "initvec.h"

#include <vector>

using namespace std;

/* Here, we maintain 2 copies of potential packets, privileged and
   unpriveleged.  We fill the packets, and then return a pointer to a static
   packet if it's different from the last one we processed, else NULL

   Note, seperate rec copies are only kept for those packets that
   can have restrictions (ship, torp, etc), since server recordings are
   not subject to those restrictions.

   This is pretty non-optimal and should be handled better via a per-item
   update counter instead of wholesale copy/compare.
*/

/* packet send */
static vector<spShip_t> pktShip;
static vector<spShipSml_t> pktShipSml;
static vector<spShipLoc_t> pktShipLoc;
static vector<spPlanet_t> pktPlanet;
static vector<spPlanetSml_t> pktPlanetSml;
static vector<spPlanetLoc_t> pktPlanetLoc;
static vector<spUser_t> pktUser;
static vector<vector<spTorp_t>> pktTorp;
static vector<vector<spTorpLoc_t>> pktTorpLoc;
static vector<vector<spTorpEvent_t>> pktTorpEvent;
static vector<spTeam_t> pktTeam;
static spConqInfo_t pktcbConqInfo;
static vector<spHistory_t> pktHistory;
static spDoomsday_t pktDoomsday;
static vector<spPlanetInfo_t> pktPlanetInfo;

/* recording */
static vector<spShip_t> recShip;
static vector<spShipSml_t> recShipSml;
static vector<spShipLoc_t> recShipLoc;
static vector<spPlanet_t> recPlanet;
static vector<spPlanetSml_t> recPlanetSml;
static vector<spPlanetLoc_t> recPlanetLoc;
static vector<vector<spTorp_t>> recTorp;
static vector<vector<spTorpLoc_t>> recTorpLoc;
static vector<vector<spTorpEvent_t>> recTorpEvent;
static vector<spTeam_t> recTeam;
static spDoomsday_t recDoomsday;
static vector<spPlanetInfo_t> recPlanetInfo;

/* allocate and initialize everything */
void spktInit(void)
{
    spktInitPkt();
    spktInitRec();

    return;
}

void spktInitPkt(void)
{
    /* server pkt */
    _INIT_VEC1D(pktShip, spShip_t, cbLimits.maxShips());
    _INIT_VEC1D(pktShipSml, spShipSml_t, cbLimits.maxShips());
    _INIT_VEC1D(pktShipLoc, spShipLoc_t, cbLimits.maxShips());
    _INIT_VEC1D(pktPlanet, spPlanet_t, cbLimits.maxPlanets());
    _INIT_VEC1D(pktPlanetSml, spPlanetSml_t, cbLimits.maxPlanets());
    _INIT_VEC1D(pktPlanetLoc, spPlanetLoc_t, cbLimits.maxPlanets());
    _INIT_VEC1D(pktUser, spUser_t, cbLimits.maxUsers());

    _INIT_VEC2D(pktTorp, spTorp_t, cbLimits.maxShips(), cbLimits.maxTorps());
    _INIT_VEC2D(pktTorpLoc, spTorpLoc_t, cbLimits.maxShips(),
                cbLimits.maxTorps());
    _INIT_VEC2D(pktTorpEvent, spTorpEvent_t, cbLimits.maxShips(),
                cbLimits.maxTorps());

    _INIT_VEC1D(pktTeam, spTeam_t, NUMALLTEAMS);

    pktcbConqInfo = {};

    _INIT_VEC1D(pktHistory, spHistory_t, cbLimits.maxHist());

    pktDoomsday= {};

    _INIT_VEC1D(pktPlanetInfo, spPlanetInfo_t, cbLimits.maxPlanets());

    return;
}

void spktInitRec(void)
{
    /* recording */
    _INIT_VEC1D(recShip, spShip_t, cbLimits.maxShips());
    _INIT_VEC1D(recShipSml, spShipSml_t, cbLimits.maxShips());
    _INIT_VEC1D(recShipLoc, spShipLoc_t, cbLimits.maxShips());
    _INIT_VEC1D(recPlanet, spPlanet_t, cbLimits.maxPlanets());
    _INIT_VEC1D(recPlanetSml, spPlanetSml_t, cbLimits.maxPlanets());
    _INIT_VEC1D(recPlanetLoc, spPlanetLoc_t, cbLimits.maxPlanets());

    _INIT_VEC2D(recTorp, spTorp_t, cbLimits.maxShips(), cbLimits.maxTorps());
    _INIT_VEC2D(recTorpLoc, spTorpLoc_t, cbLimits.maxShips(),
                cbLimits.maxTorps());
    _INIT_VEC2D(recTorpEvent, spTorpEvent_t, cbLimits.maxShips(),
                cbLimits.maxTorps());

    _INIT_VEC1D(recTeam,  spTeam_t, NUMALLTEAMS);

    recDoomsday = {};

    _INIT_VEC1D(recPlanetInfo, spPlanetInfo_t, cbLimits.maxPlanets());

    return;
}



/* non priv */
spUser_t *spktUser(uint16_t unum)
{
    static spUser_t suser;
    int i;

    memset((void *)&suser, 0, sizeof(spUser_t));

    suser.type = SP_USER;
    suser.team = (uint8_t)cbUsers[unum].team;
    suser.unum = htons(unum);
    suser.userType = (uint8_t)cbUsers[unum].type;
    suser.flags = htons(cbUsers[unum].flags);
    suser.opFlags = htons(cbUsers[unum].opFlags);

    for (i=0; i<NUMPLAYERTEAMS; i++)
        if (cbUsers[unum].war[i])
            suser.war |= (1 << i);

    suser.rating = (int16_t)htons((uint16_t)(cbUsers[unum].rating * 10.0));
    suser.lastentry = (uint32_t)htonl((uint32_t)cbUsers[unum].lastentry);

    for (i=0; i<USTAT_TOTALSTATS; i++)
        suser.stats[i] = (int32_t)htonl(cbUsers[unum].stats[i]);

    utStrncpy((char *)suser.username, cbUsers[unum].username, MAXUSERNAME);
    utStrncpy((char *)suser.alias, cbUsers[unum].alias, MAXUSERNAME);

    if (memcmp((void *)&suser, (void *)&pktUser[unum], sizeof(spUser_t)))
    {
        pktUser[unum] = suser;
        return &suser;
    }

    return NULL;
}

/* PRIV */
spShip_t *spktShip(uint8_t snum, int rec)
{
    int i;
    int mysnum = Context.snum;
    int myteam = cbShips[mysnum].team;
    static spShip_t sship;

    memset((void *)&sship, 0, sizeof(spShip_t));

    sship.type = SP_SHIP;
    sship.status = cbShips[snum].status;
    sship.snum = snum;
    sship.team = cbShips[snum].team;
    sship.unum = htons(cbShips[snum].unum);
    sship.shiptype = cbShips[snum].shiptype;

    /* RESTRICT */
    /* really only valid for own ship */
    if ((mysnum == snum) || rec)
    {
        for (i=0; i<NUMPLAYERTEAMS; i++)
	{
            if (cbShips[snum].war[i])
                sship.war |= (1 << i);
            if (cbShips[snum].rwar[i])
                sship.rwar |= (1 << i);
	}

        sship.killedBy = (uint8_t)cbShips[snum].killedBy;
        sship.killedByDetail = htons(cbShips[snum].killedByDetail);

        // encode the srpwar (Self Ruled Planet War) bits
        for (i=0; i<cbLimits.maxPlanets(); i++)
        {
            if (cbShips[snum].srpwar[i])
            {
                int word = i / (sizeof(uint32_t) * 8);
                int bit = i % (sizeof(uint32_t) * 8);

                sship.srpwar[word] |= (1 << bit);
            }
        }

        // Now reformat for network transmission
        for (i=0; i<PROTO_SRPWAR_BIT_WORDS; i++)
            sship.srpwar[i] = htonl(sship.srpwar[i]);

    }
    else
    {
        /* RESTRICT */
        /* we only send the war stats relating to our team */
        if (cbShips[snum].war[myteam])
            sship.war |= (1 << myteam);

        if (cbShips[snum].rwar[myteam])
            sship.rwar |= (1 << myteam);
    }

    /* for robots, we need to account for strkills as well.
       we won't bother adding strkills to the packet, as the client doesn't
       really need to care whether these are 'real' kills vs. random ones. */
    if (SROBOT(snum))
        sship.kills =
            htonl((uint32_t)((cbShips[snum].kills + cbShips[snum].strkills) * 10.0));
    else
        sship.kills = htonl((uint32_t)(cbShips[snum].kills * 10.0));

    for (i=0; i<NUMPLAYERTEAMS; i++)
        sship.scanned[i] = (uint8_t)cbShips[snum].scanned[i];

    utStrncpy((char *)sship.alias, cbShips[snum].alias, MAXUSERNAME);

    if (rec)
    {
        if (memcmp((void *)&sship, (void *)&recShip[snum], sizeof(spShip_t)))
        {
            recShip[snum] = sship;
            return &sship;
        }
    }
    else
    {
        if (memcmp((void *)&sship, (void *)&pktShip[snum], sizeof(spShip_t)))
        {
            pktShip[snum] = sship;
            return &sship;
        }
    }

    return NULL;
}

/* PRIV */
spShipSml_t *spktShipSml(uint8_t snum, int rec)
{
    int mysnum = Context.snum;
    static spShipSml_t sshipsml;
    int canscan = false;
    real dis;
    uint16_t sflags = 0;		/* ship flags we are allowed to see */
    uint16_t scanflag = 0;         /* set to SHIP_F_SCANDIST if < ACCINFO_DIST */

    memset((void *)&sshipsml, 0, sizeof(spShipSml_t));

    sflags = SHIP_F_NONE;		/* can't see anything by default.  make sure
                                           SHIP_F_MAP never sneaks in */

    sshipsml.type = SP_SHIPSML;
    sshipsml.snum = snum;

    /* can always see these */
    sflags |= (SHIP_F_CLOAKED | SHIP_F_ROBOT | SHIP_F_VACANT | SHIP_F_SCANDIST);

    if ((snum == mysnum) || rec)
    {			     /* really only useful for our own ship */
        sflags |= (SHIP_F_REPAIR | SHIP_F_TALERT | SHIP_F_BOMBING
                   | SHIP_F_TOWEDBY | SHIP_F_TOWING);

        sshipsml.towing = cbShips[snum].towing;
        sshipsml.towedby = cbShips[snum].towedby;

        sshipsml.action = (uint8_t)cbShips[snum].action;
        sshipsml.lastblast = htons((uint16_t)(cbShips[snum].lastblast * 100.0));
        sshipsml.fuel = htons((uint16_t)cbShips[snum].fuel);
        sshipsml.lock = (uint8_t)cbShips[snum].lock;
        sshipsml.lockDetail = htons(cbShips[snum].lockDetail);
        sshipsml.sdfuse = (int16_t)htons((uint16_t)cbShips[snum].sdfuse);
        sshipsml.wfuse = (int8_t)cbShips[snum].wfuse;
        sshipsml.efuse = (int8_t)cbShips[snum].efuse;
        sshipsml.walloc = cbShips[snum].weapalloc;
        sshipsml.etemp = (uint8_t)cbShips[snum].etemp;
        sshipsml.wtemp = (uint8_t)cbShips[snum].wtemp;
        canscan = true;	      /* can always scan ourselves */
    }
    else
    {
        if (!SCLOAKED(snum))
            dis = (real) dist(cbShips[mysnum].x, cbShips[mysnum].y,
                              cbShips[snum].x,
                              cbShips[snum].y );
        else
            dis = (real) dist(cbShips[mysnum].x, cbShips[mysnum].y,
                              rndnor(cbShips[snum].x, CLOAK_SMEAR_DIST),
                              rndnor(cbShips[snum].y, CLOAK_SMEAR_DIST));

        /* if in accurate scanning distance (regardless of cloak) set the
           SCANDIST flag. */

        if (dis < ACCINFO_DIST)
            scanflag = SHIP_F_SCANDIST;

        /* help the driver, set scanned fuse */
        if ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) && ! selfwar(mysnum) )
            cbShips[snum].scanned[cbShips[mysnum].team] = SCANNED_FUSE;

        /* if within accurate dist and not cloaked, or
           if ship scanned by my team and not selfwar, or
           if not at war with ship. */

        canscan = ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) ||
                    ( (cbShips[snum].scanned[cbShips[mysnum].team] > 0) &&
                      ! selfwar(mysnum) ) ||
                    !satwar(snum, mysnum));
    }

    if (canscan)
    {				/* if we get all the stats */
        sflags |= SHIP_F_SHUP | SHIP_F_BOMBING | SHIP_F_REPAIR;

        sshipsml.shields = (uint8_t)cbShips[snum].shields;
        sshipsml.damage = (uint8_t)cbShips[snum].damage;
        sshipsml.armies = cbShips[snum].armies;

        /* so we can do bombing */
        sshipsml.lock = (uint8_t)cbShips[snum].lock;
        sshipsml.lockDetail = htons(cbShips[snum].lockDetail);

        /* so we can disp phasers in graphical client ;-) */
        sshipsml.lastphase = htons((uint16_t)(cbShips[snum].lastphase * 100.0));
        sshipsml.pfuse = (int8_t)cbShips[snum].pfuse;
    }

    /* only send those we are allowed to see */
    sshipsml.flags = htonl(((cbShips[snum].flags | scanflag) & sflags));

    if (rec)
    {
        if (memcmp((void *)&sshipsml, (void *)&recShipSml[snum],
                   sizeof(spShipSml_t)))
        {
            recShipSml[snum] = sshipsml;
            return &sshipsml;
        }
    }
    else
    {
        if (memcmp((void *)&sshipsml, (void *)&pktShipSml[snum],
                   sizeof(spShipSml_t)))
        {
            pktShipSml[snum] = sshipsml;
            return &sshipsml;
        }
    }

    return NULL;
}

/* PRIV */
spShipLoc_t *spktShipLoc(uint8_t snum, int rec)
{
    int mysnum = Context.snum;
    static spShipLoc_t sshiploc;
    real x, y;
    int canscan = false;
    real dis;
    static const uint32_t maxtime = 5000;  /* 5 seconds */
    static uint32_t lasttime = 0;
    uint32_t thetime = clbGetMillis();
    bool forceMyShip = false;

    memset((void *)&sshiploc, 0, sizeof(spShipLoc_t));

    sshiploc.type = SP_SHIPLOC;
    sshiploc.snum = snum;
    sshiploc.warp = (int8_t)(cbShips[snum].warp * 10.0);

    /* we need to ensure that if we are doing UDP, and we haven't
       updated a ship in awhile (causing UDP traffic), force an update
       of your ship even if it isn't neccessary.  This should help with
       those firewalls that disconnect a UDP connection when there has
       been no trafic on it for a while. */
    if (sInfo.doUDP && !rec && (snum == mysnum)
        && ((thetime - lasttime) > maxtime))
    {
        lasttime = thetime;
        forceMyShip = true;
    }

    /* RESTRICT */
    if ((snum == mysnum) || rec)
    {				/* we get everything */
        sshiploc.head = htons((uint16_t)(cbShips[snum].head * 10.0));
        x = cbShips[snum].x;
        y = cbShips[snum].y;
    }
    else
    {
        if (SCLOAKED(snum))
        {
            if (cbShips[snum].warp == 0.0)
            {
                x = MAX_UNIVERSAL_COORD;
                y = MAX_UNIVERSAL_COORD;
            }
            else
            { /* if your cloaked, and moving, get smeared x/y */
                x = rndnor( cbShips[snum].x, CLOAK_SMEAR_DIST );
                y = rndnor( cbShips[snum].y, CLOAK_SMEAR_DIST );
            }
        }
        else
	{			/* not cloaked */
            dis = (real) dist(cbShips[mysnum].x, cbShips[mysnum].y,
                              cbShips[snum].x,
                              cbShips[snum].y );

            canscan = ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) ||
                        ( (cbShips[snum].scanned[cbShips[mysnum].team] > 0) &&
                          ! selfwar(mysnum) ) ||
                        !satwar(snum, mysnum));

            if (canscan)		/* close or friendly */
                sshiploc.head = htons((uint16_t)(cbShips[snum].head * 10.0));

            x = cbShips[snum].x;
            y = cbShips[snum].y;

	}
    }

    sshiploc.x = (int32_t)htonl((int32_t)(x * 10.0));
    sshiploc.y = (int32_t)htonl((int32_t)(y * 10.0));

    if (rec)
    {
        if (memcmp((void *)&sshiploc, (void *)&recShipLoc[snum],
                   sizeof(spShipLoc_t)))
        {
            recShipLoc[snum] = sshiploc;
            return &sshiploc;
        }
    }
    else
    {
        if (forceMyShip || memcmp((void *)&sshiploc, (void *)&pktShipLoc[snum],
                                  sizeof(spShipLoc_t)))
        {
            pktShipLoc[snum] = sshiploc;
            return &sshiploc;
        }
    }

    return NULL;
}

spPlanet_t *spktPlanet(uint8_t pnum, int rec)
{
    int snum = Context.snum;
    int team = cbShips[snum].team;
    static spPlanet_t splan;

    memset((void *)&splan, 0, sizeof(spPlanet_t));

#if defined(DEBUG_SERVERSEND)
    utLog("sendPlanet: pnum = %d",
          pnum);
#endif

    splan.type = SP_PLANET;
    splan.pnum = pnum;
    splan.ptype = cbPlanets[pnum].type;

    /* RESTRICT */
    if (cbPlanets[pnum].scanned[team] || rec)
        splan.team = cbPlanets[pnum].team;
    else
        splan.team = TEAM_SELFRULED; /* until we know for sure... */

    // who's homeworld is this (if a homeplanet)?
    splan.defendteam = cbPlanets[pnum].defendteam;

    // how big is it?
    splan.size = htons(cbPlanets[pnum].size);

    utStrncpy((char *)splan.name, cbPlanets[pnum].name, MAXPLANETNAME);

    if (rec)
    {
        if (memcmp((void *)&splan, (void *)&recPlanet[pnum],
                   sizeof(spPlanet_t)))
        {
            recPlanet[pnum] = splan;
            return &splan;
        }
    }
    else
    {
        if (memcmp((void *)&splan, (void *)&pktPlanet[pnum],
                   sizeof(spPlanet_t)))
        {
            pktPlanet[pnum] = splan;
            return &splan;
        }
    }

    return NULL;
}

spPlanetSml_t *spktPlanetSml(uint8_t pnum, int rec)
{
    int i;
    int snum = Context.snum;
    int team = cbShips[snum].team;
    static spPlanetSml_t splansml;

    memset((void *)&splansml, 0, sizeof(spPlanetSml_t));

    splansml.type = SP_PLANETSML;
    splansml.pnum = pnum;

    /* RESTRICT */
    if (rec)
    {
        for (i=0; i < NUMPLAYERTEAMS; i++)
            if (cbPlanets[pnum].scanned[i])
                splansml.scanned |= (1 << i);

        splansml.uninhabtime = (uint8_t)cbPlanets[pnum].uninhabtime;
    }
    else
    {
        if (cbPlanets[pnum].scanned[team])
        {
            splansml.scanned |= (1 << team);
            splansml.uninhabtime = (uint8_t)cbPlanets[pnum].uninhabtime;
        }
    }

    if (rec)
    {
        if (memcmp((void *)&splansml, (void *)&recPlanetSml[pnum],
                   sizeof(spPlanetSml_t)))
        {
            recPlanetSml[pnum] = splansml;
            return &splansml;
        }
    }
    else
    {
        if (memcmp((void *)&splansml, (void *)&pktPlanetSml[pnum],
                   sizeof(spPlanetSml_t)))
        {
            pktPlanetSml[pnum] = splansml;
            return &splansml;
        }
    }

    return NULL;
}

spPlanetLoc_t *spktPlanetLoc(uint8_t pnum, int rec, int force)
{
    int snum = Context.snum;
    int team = cbShips[snum].team;
    static spPlanetLoc_t splanloc;
    // we send packets only every 5 secs in most cases
    uint32_t iternow = clbGetMillis();
    static const uint32_t iterwait = 5000; /* ms */
    static uint32_t tstart[ABS_MAXPLANETS] = {}; /* saved time deltas */
    bool tooearly = false;

    /*
     * We have to handle the case where a planet has just been freshly
     * scanned (and therefore a real army count is available).  We want
     * the client to see the true army count as soon as possible.
     */

    if (!force && (tstart[pnum] != 0 && ((iternow - tstart[pnum]) < iterwait)))
        tooearly = true;

    memset((void *)&splanloc, 0, sizeof(spPlanetLoc_t));

    splanloc.type = SP_PLANETLOC;
    splanloc.pnum = pnum;

    /* RESTRICT */
    if (cbPlanets[pnum].scanned[team] || rec)
        splanloc.armies = htons(cbPlanets[pnum].armies);

    // Do a tooearly check here - to avoid jitter, if we are in orbit
    // around a planet, always send it's location data.
    if (cbShips[snum].warp < 0)
    {
        // we are in orbit
        if (pnum == cbShips[snum].lockDetail)
            tooearly = false;
    }

    if (splanloc.armies == pktPlanetLoc[pnum].armies && tooearly && !rec)
        return NULL;

    // only update time for actual packets sent
    if (!rec)
        tstart[pnum] = iternow;

    splanloc.x = (int32_t)htonl((int32_t)(cbPlanets[pnum].x * 10.0));
    splanloc.y = (int32_t)htonl((int32_t)(cbPlanets[pnum].y * 10.0));
    splanloc.orbang = htons((uint16_t)(cbPlanets[pnum].orbang * 100.0));

    if (rec)
    {
        if (memcmp((void *)&splanloc, (void *)&recPlanetLoc[pnum],
                   sizeof(spPlanetLoc_t)))
        {
            recPlanetLoc[pnum] = splanloc;
            return &splanloc;
        }
    }
    else
    {
        if (memcmp((void *)&splanloc, (void *)&pktPlanetLoc[pnum],
                   sizeof(spPlanetLoc_t)))
        {
            pktPlanetLoc[pnum] = splanloc;
            return &splanloc;
        }
    }

    return NULL;
}


/* non priv */
spTorp_t *spktTorp(uint8_t tsnum, uint8_t tnum, int rec)
{
    static spTorp_t storp;

    memset((void *)&storp, 0, sizeof(spTorp_t));

    storp.type = SP_TORP;
    storp.snum = tsnum;
    storp.tnum = tnum;
    storp.status = (uint8_t)cbShips[tsnum].torps[tnum].status;

    if (rec)
    {
        if (memcmp((void *)&storp, (void *)&(recTorp[tsnum][tnum]),
                   sizeof(spTorp_t)))
        {
            recTorp[tsnum][tnum] = storp;
            return &storp;
        }
    }
    else
    {
        if (memcmp((void *)&storp, (void *)&(pktTorp[tsnum][tnum]),
                   sizeof(spTorp_t)))
        {
            pktTorp[tsnum][tnum] = storp;
            return &storp;
        }
    }

    return NULL;
}

/* PRIV */
spTorpLoc_t *spktTorpLoc(uint8_t tsnum, uint8_t tnum, int rec)
{
    int i;
    int snum = Context.snum;
    int team = cbShips[snum].team;
    static spTorpLoc_t storploc;
    real dis;
    real x, y;

    memset((void *)&storploc, 0, sizeof(spTorpLoc_t));

    storploc.type = SP_TORPLOC;
    storploc.snum = tsnum;
    storploc.tnum = tnum;

    /* RESTRICT */
    /* we can always see friendly torps.  enemy torps can only be seen if
       within ACCINFO_DIST of your ship.  torp war stat only applies to
       your ship. */

    x = cbShips[tsnum].torps[tnum].x;
    y = cbShips[tsnum].torps[tnum].y;

    if (cbShips[tsnum].torps[tnum].war[team] && !rec)
    {				/* it's at war with us. bastards. */
        /* see if it's close enough to scan */
        dis = (real) dist(cbShips[snum].x, cbShips[snum].y,
                          cbShips[tsnum].torps[tnum].x,
                          cbShips[tsnum].torps[tnum].y );

        if (dis > ACCINFO_DIST)
        {                       /* in the bermuda triangle */
            x = MAX_UNIVERSAL_COORD;
            y = MAX_UNIVERSAL_COORD;
        }
    }

    storploc.x = (int32_t)htonl((int32_t)(x * 10.0));
    storploc.y = (int32_t)htonl((int32_t)(y * 10.0));

    if (rec)
    {
        for (i=0; i < NUMPLAYERTEAMS; i++)
            if (cbShips[tsnum].torps[tnum].war[i])
                storploc.war |= (1 << i);
    }
    else
    {
        /* RESTRICT */
        /* only send 'war' status as it relates to our team */
        if (cbShips[tsnum].torps[tnum].war[team])
            storploc.war |= (1 << team);
    }

    if (rec)
    {
        if (memcmp((void *)&storploc, (void *)&(recTorpLoc[tsnum][tnum]),
                   sizeof(spTorpLoc_t)))
        {
            recTorpLoc[tsnum][tnum] = storploc;
            return &storploc;
        }
    }
    else
    {
        if (memcmp((void *)&storploc, (void *)&(pktTorpLoc[tsnum][tnum]),
                   sizeof(spTorpLoc_t)))
        {
            pktTorpLoc[tsnum][tnum] = storploc;
            return &storploc;
        }
    }

    return NULL;
}

/* PRIV */
spTorpEvent_t *spktTorpEvent(uint8_t tsnum, uint8_t tnum, int rec)
{
    int i;
    int snum = Context.snum;
    int team = cbShips[snum].team;
    static spTorpEvent_t storpev;
    real x, y, dx, dy;

    memset((void *)&storpev, 0, sizeof(spTorpEvent_t));

    storpev.type = SP_TORPEVENT;
    storpev.snum = tsnum;
    storpev.tnum = tnum;
    storpev.status = (uint8_t)cbShips[tsnum].torps[tnum].status;

    /* RESTRICT */
    /* torp war stat only applies to your ship. */

    x = cbShips[tsnum].torps[tnum].x;
    y = cbShips[tsnum].torps[tnum].y;
    dx = cbShips[tsnum].torps[tnum].dx;
    dy = cbShips[tsnum].torps[tnum].dy;

    storpev.x = (int32_t)htonl((int32_t)(x * 10.0));
    storpev.y = (int32_t)htonl((int32_t)(y * 10.0));
    storpev.dx = (int32_t)htonl((int32_t)(dx * 100.0));
    storpev.dy = (int32_t)htonl((int32_t)(dy * 100.0));

    if (rec)
    {
        for (i=0; i < NUMPLAYERTEAMS; i++)
            if (cbShips[tsnum].torps[tnum].war[i])
                storpev.war |= (1 << i);
    }
    else
    {
        /* RESTRICT */
        /* only send 'war' status as it relates to our team */
        if (cbShips[tsnum].torps[tnum].war[team])
            storpev.war |= (1 << team);
    }

    /* we only do these if the torp status changed */
    if (rec)
    {
        if (storpev.status != recTorpEvent[tsnum][tnum].status)
        {
            recTorpEvent[tsnum][tnum] = storpev;
            return &storpev;
        }
    }
    else
    {
        if (storpev.status != pktTorpEvent[tsnum][tnum].status)
        {
            pktTorpEvent[tsnum][tnum] = storpev;
            return &storpev;
        }
    }

    return NULL;
}

/* PRIV */
spTeam_t *spktTeam(uint8_t team, int force, int rec)
{
    int snum = Context.snum;
    static spTeam_t steam;
    int i;

    memset((void *)&steam, 0, sizeof(spTeam_t));

    steam.type = SP_TEAM;
    steam.team = team;
    steam.homeplanet = (uint8_t)cbTeams[team].homeplanet;

    /* RESTRICT */
    uint16_t tflags = TEAM_F_NONE;
    if ((cbShips[snum].team == team) || rec)
    {				/* we only send this stuff for our team */
        tflags |= TEAM_F_COUPINFO;

        steam.couptime = (uint8_t)cbTeams[team].couptime;
    }

    // restrict flags to allowed flags
    steam.flags = htons(cbTeams[team].flags & tflags);

    for (i=0; i<MAXTSTATS; i++)
        steam.stats[i] = (uint32_t)htonl(cbTeams[team].stats[i]);

    utStrncpy((char *)steam.name, cbTeams[team].name, MAXTEAMNAME);

    if (rec)
    {
        if (memcmp((void *)&steam, (void *)&recTeam[team],
                   sizeof(spTeam_t)) || force)
        {
            recTeam[team] = steam;
            return &steam;
        }
    }
    else
    {
        if (memcmp((void *)&steam, (void *)&pktTeam[team],
                   sizeof(spTeam_t)) || force)
        {
            pktTeam[team] = steam;
            return &steam;
        }
    }

    return NULL;
}

spConqInfo_t *spktcbConqInfo(int force)
{
    static spConqInfo_t spci;

    memset((void *)&spci, 0, sizeof(spConqInfo_t));

    spci.type = SP_CONQINFO;

    utStrncpy((char *)spci.conqueror, cbConqInfo->conqueror, MAXUSERNAME);
    utStrncpy((char *)spci.conqteam, cbConqInfo->conqteam, MAXTEAMNAME);
    utStrncpy((char *)spci.conqtime, cbConqInfo->conqtime, MAXDATESIZE);
    utStrncpy((char *)spci.lastwords, cbConqInfo->lastwords, MAXLASTWORDS);

    if (memcmp((void *)&spci, (void *)&pktcbConqInfo,
               sizeof(spConqInfo_t)) || force)
    {
        pktcbConqInfo = spci;
        return &spci;
    }

    return NULL;
}

spHistory_t *spktHistory(int hnum)
{
    static spHistory_t hist;

    memset((void *)&hist, 0, sizeof(spHistory_t));

    hist.type = SP_HISTORY;
    hist.hnum = hnum;

    hist.histptr = cbConqInfo->histptr;

    hist.unum = (uint16_t)htons((uint16_t)cbHistory[hnum].unum);

    hist.elapsed = (uint32_t)htonl((uint32_t)cbHistory[hnum].elapsed);
    hist.enterTime = (uint32_t)htonl((uint32_t)cbHistory[hnum].enterTime);

    utStrncpy((char *)hist.username, cbHistory[hnum].username, MAXUSERNAME);

    if (memcmp((void *)&hist, (void *)&pktHistory[hnum], sizeof(spHistory_t)))
    {
        pktHistory[hnum] = hist;
        return &hist;
    }

    return NULL;
}

spDoomsday_t *spktDoomsday(int rec)
{
    static spDoomsday_t dd;

    memset((void *)&dd, 0, sizeof(spDoomsday_t));

    dd.type = SP_DOOMSDAY;
    dd.heading = htons((uint16_t)(cbDoomsday->heading * 10.0));
    dd.x = (int32_t)htonl((int32_t)(cbDoomsday->x * 10.0));
    dd.y = (int32_t)htonl((int32_t)(cbDoomsday->y * 10.0));

    dd.eaterType = static_cast<uint8_t>(cbDoomsday->eaterType);
    dd.flags = cbDoomsday->flags;

    if (rec)
    {
        if (memcmp((void *)&dd, (void *)&recDoomsday,
                   sizeof(spDoomsday_t)))
        {
            recDoomsday = dd;
            return &dd;
        }
    }
    else
    {
        if (memcmp((void *)&dd, (void *)&pktDoomsday,
                   sizeof(spDoomsday_t)))
        {
            pktDoomsday = dd;
            return &dd;
        }
    }

    return NULL;
}

spPlanetInfo_t *spktPlanetInfo(uint8_t pnum, int rec)
{
    static spPlanetInfo_t splaninfo;

    memset((void *)&splaninfo, 0, sizeof(spPlanetInfo_t));

    splaninfo.type = SP_PLANETINFO;
    splaninfo.pnum = pnum;
    splaninfo.flags = htonl(cbPlanets[pnum].flags);

    splaninfo.primary = (uint8_t)cbPlanets[pnum].primary;

    splaninfo.orbrad = (uint32_t)htonl((uint32_t)(cbPlanets[pnum].orbrad * 10.0));
    splaninfo.orbvel = (int32_t)htonl((int32_t)(cbPlanets[pnum].orbvel * 100.0));

    if (rec)
    {
        if (memcmp((void *)&splaninfo, (void *)&recPlanetInfo[pnum],
                   sizeof(spPlanetInfo_t)))
        {
            recPlanetInfo[pnum] = splaninfo;
            return &splaninfo;
        }
    }
    else
    {
        if (memcmp((void *)&splaninfo, (void *)&pktPlanetInfo[pnum],
                   sizeof(spPlanetInfo_t)))
        {
            pktPlanetInfo[pnum] = splaninfo;
            return &splaninfo;
        }
    }

    return NULL;
}
