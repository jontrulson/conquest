#include "c_defs.h"

/************************************************************************
 *
 * packet recEvent/pkt generation for the server
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "conf.h"
#include "server.h"
#include "context.h"
#include "serverpkt.h"
#include "conqlb.h"

/* Here, we maintain 2 copies of potential packets, privileged and
   unpriveleged.  We fill the packets, and then return a pointer to a static
   packet if it's different from the last one we processed, else NULL

   Note, seperate rec copies are only kept for those packets that
   can have restrictions (ship, torp, etc), since server recordings are
   not subject to those restrictions.
*/

/* packet send */
static spShip_t pktShip[MAXSHIPS + 1];
static spShipSml_t pktShipSml[MAXSHIPS + 1];
static spShipLoc_t pktShipLoc[MAXSHIPS + 1];
static spPlanet_t pktPlanet[NUMPLANETS + 1];
static spPlanetSml_t pktPlanetSml[NUMPLANETS + 1];
static spPlanetLoc_t pktPlanetLoc[NUMPLANETS + 1];
static spPlanetLoc2_t pktPlanetLoc2[NUMPLANETS + 1];
static spUser_t pktUser[MAXUSERS];
static spTorp_t pktTorp[MAXSHIPS + 1][MAXTORPS];
static spTorpLoc_t pktTorpLoc[MAXSHIPS + 1][MAXTORPS];
static spTorpEvent_t pktTorpEvent[MAXSHIPS + 1][MAXTORPS];
static spTeam_t pktTeam[NUMALLTEAMS];
static spConqInfo_t pktConqInfo;
static spHistory_t pktHistory[MAXHISTLOG];
static spDoomsday_t pktDoomsday;
static spPlanetInfo_t pktPlanetInfo[NUMPLANETS + 1];

/* recording */
static spShip_t recShip[MAXSHIPS + 1];
static spShipSml_t recShipSml[MAXSHIPS + 1];
static spShipLoc_t recShipLoc[MAXSHIPS + 1];
static spPlanet_t recPlanet[NUMPLANETS + 1];
static spPlanetSml_t recPlanetSml[NUMPLANETS + 1];
static spPlanetLoc_t recPlanetLoc[NUMPLANETS + 1];
static spPlanetLoc2_t recPlanetLoc2[NUMPLANETS + 1];
static spTorp_t recTorp[MAXSHIPS + 1][MAXTORPS];
static spTorpLoc_t recTorpLoc[MAXSHIPS + 1][MAXTORPS];
static spTorpEvent_t recTorpEvent[MAXSHIPS + 1][MAXTORPS];
static spTeam_t recTeam[NUMALLTEAMS];
static spDoomsday_t recDoomsday;
static spPlanetInfo_t recPlanetInfo[NUMPLANETS + 1];

/* memset everything to 0 */
void spktInit(void)
{
  spktInitPkt();
  spktInitRec();

  return;
}

void spktInitPkt(void)
{
  /* server pkt */
  memset((void *)pktShip, 0, sizeof(spShip_t) * (MAXSHIPS + 1));
  memset((void *)pktShipSml, 0, sizeof(spShipSml_t) * (MAXSHIPS + 1));
  memset((void *)pktShipLoc, 0, sizeof(spShipLoc_t) * (MAXSHIPS + 1));
  memset((void *)pktPlanet, 0, sizeof(spPlanet_t) * (NUMPLANETS + 1));
  memset((void *)pktPlanetSml, 0, sizeof(spPlanetSml_t) * (NUMPLANETS + 1));
  memset((void *)pktPlanetLoc, 0, sizeof(spPlanetLoc_t) * (NUMPLANETS + 1));
  memset((void *)pktPlanetLoc2, 0, sizeof(spPlanetLoc2_t) * (NUMPLANETS + 1));
  memset((void *)pktUser, 0, sizeof(spUser_t) * MAXUSERS);
  memset((void *)pktTorp, 0,  sizeof(spTorp_t) * (MAXSHIPS + 1) * MAXTORPS);
  memset((void *)pktTorpLoc, 0,  
         sizeof(spTorpLoc_t) * (MAXSHIPS + 1) * MAXTORPS);
  memset((void *)pktTorpEvent, 0,  
         sizeof(spTorpEvent_t) * (MAXSHIPS + 1) * MAXTORPS);
  memset((void *)pktTeam, 0,  sizeof(spTeam_t) * NUMALLTEAMS);
  memset((void *)&pktConqInfo, 0,  sizeof(spConqInfo_t));
  memset((void *)pktHistory, 0,  sizeof(spHistory_t) * MAXHISTLOG);
  memset((void *)&pktDoomsday, 0,  sizeof(spDoomsday_t));
  memset((void *)pktPlanetInfo, 0, sizeof(spPlanetInfo_t) * (NUMPLANETS + 1));
  return;
}

void spktInitRec(void)
{
  /* recording */
  memset((void *)recShip, 0, sizeof(spShip_t) * (MAXSHIPS + 1));
  memset((void *)recShipSml, 0, sizeof(spShipSml_t) * (MAXSHIPS + 1));
  memset((void *)recShipLoc, 0, sizeof(spShipLoc_t) * (MAXSHIPS + 1));
  memset((void *)recPlanet, 0, sizeof(spPlanet_t) * (NUMPLANETS + 1));
  memset((void *)recPlanetSml, 0, sizeof(spPlanetSml_t) * (NUMPLANETS + 1));
  memset((void *)recPlanetLoc, 0, sizeof(spPlanetLoc_t) * (NUMPLANETS + 1));
  memset((void *)recPlanetLoc2, 0, sizeof(spPlanetLoc2_t) * (NUMPLANETS + 1));
  memset((void *)recTorp, 0,  
         sizeof(spTorp_t) * (MAXSHIPS + 1) * MAXTORPS);
  memset((void *)recTorpLoc, 0,  
         sizeof(spTorpLoc_t) * (MAXSHIPS + 1) * MAXTORPS);
  memset((void *)recTorpEvent, 0,  
         sizeof(spTorpEvent_t) * (MAXSHIPS + 1) * MAXTORPS);
  memset((void *)recTeam, 0,  sizeof(spTeam_t) * NUMALLTEAMS);
  memset((void *)&recDoomsday, 0, sizeof(spDoomsday_t));
  memset((void *)recPlanetInfo, 0, sizeof(spPlanetInfo_t) * (NUMPLANETS + 1));

  return;
}



/* non priv */
spUser_t *spktUser(Unsgn16 unum)
{
  static spUser_t suser;
  int i;

  memset((void *)&suser, 0, sizeof(spUser_t));

  suser.type = SP_USER;
  suser.team = (Unsgn8)Users[unum].team;
  suser.unum = htons(unum);

  if (Users[unum].live)
    suser.flags |= SPUSER_FLAGS_LIVE;

  for (i=0; i<NUMPLAYERTEAMS; i++)
    if (Users[unum].war[i])
      suser.war |= (1 << i);

  suser.rating = (Sgn16)htons((Unsgn16)(Users[unum].rating * 100.0));
  suser.lastentry = (Unsgn32)htonl((Unsgn32)Users[unum].lastentry);
  
  for (i=0; i<OOPT_TOTALOOPTION; i++)
    suser.ooptions[i] = (Sgn32)htonl(Users[unum].ooptions[i]);

  for (i=0; i<USTAT_TOTALSTATS; i++)
    suser.stats[i] = (Sgn32)htonl(Users[unum].stats[i]);

  strncpy(suser.username, Users[unum].username, MAXUSERNAME - 1);
  strncpy(suser.alias, Users[unum].alias, MAXUSERPNAME - 1);

  if (memcmp((void *)&suser, (void *)&pktUser[unum], sizeof(spUser_t)))
    {
      pktUser[unum] = suser;
      return &suser;
    }

  return NULL;
}

/* PRIV */
spShip_t *spktShip(Unsgn8 snum, int rec)
{
  int i;
  int mysnum = Context.snum;
  int myteam = Ships[mysnum].team;
  static spShip_t sship;

  memset((void *)&sship, 0, sizeof(spShip_t));

  sship.type = SP_SHIP;
  sship.status = Ships[snum].status;
  sship.snum = snum;
  sship.team = Ships[snum].team;
  sship.unum = htons(Ships[snum].unum);
  sship.shiptype = Ships[snum].shiptype;

  /* RESTRICT */
  /* really only valid for own ship */
  if ((mysnum == snum) || rec)
    {
      sship.towing = Ships[snum].towing;
      sship.towedby = Ships[snum].towedby;

      for (i=0; i<NUMPLAYERTEAMS; i++)
	{
	  if (Ships[snum].war[i])
	    sship.war |= (1 << i);
	  if (Ships[snum].rwar[i])
	    sship.rwar |= (1 << i);
	}

      sship.killedby = (Sgn16)htons(Ships[snum].killedby);

      for (i=1; i<=NUMPLANETS; i++)
	sship.srpwar[i] = (Unsgn8)Ships[snum].srpwar[i];
    }
  else
    {
      /* RESTRICT */
      /* we only send the war stats relating to our team */
      if (Ships[snum].war[myteam])
	  sship.war |= (1 << myteam);
      
      if (Ships[snum].rwar[myteam])
	sship.rwar |= (1 << myteam);
    }      

  /* for robots, we need to account for strkills as well.
     we won't bother adding strkills to the packet, as the client doesn't
     really need to care whether these are 'real' kills vs. random ones. */
  if (SROBOT(snum))
    sship.kills = 
      htonl((Unsgn32)((Ships[snum].kills + Ships[snum].strkills) * 10.0));
  else
    sship.kills = htonl((Unsgn32)(Ships[snum].kills * 10.0));

  for (i=0; i<NUMPLAYERTEAMS; i++)
    sship.scanned[i] = (Unsgn8)Ships[snum].scanned[i];

  strncpy(sship.alias, Ships[snum].alias, MAXUSERPNAME - 1);

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
spShipSml_t *spktShipSml(Unsgn8 snum, int rec)
{
  int mysnum = Context.snum;
  static spShipSml_t sshipsml;
  int canscan = FALSE;
  real dis;
  Unsgn16 sflags = 0;		/* ship flags we are allowed to see */
  Unsgn16 scanflag = 0;         /* set to SHIP_F_SCANDIST if < ACCINFO_DIST */

  memset((void *)&sshipsml, 0, sizeof(spShipSml_t));

  sflags = SHIP_F_NONE;		/* can't see anything by default.  make sure
                                   SHIP_F_MAP never sneaks in */

  sshipsml.type = SP_SHIPSML;
  sshipsml.snum = snum;

  /* can always see these */
  sflags |= (SHIP_F_CLOAKED | SHIP_F_ROBOT | SHIP_F_VACANT | SHIP_F_SCANDIST);

  if ((snum == mysnum) || rec)
    {			     /* really only useful for our own ship */
      sflags |= (SHIP_F_REPAIR | SHIP_F_TALERT);

      sshipsml.action = (Unsgn8)Ships[snum].action;
      sshipsml.lastblast = htons((Unsgn16)(Ships[snum].lastblast * 100.0));
      sshipsml.fuel = htons((Unsgn16)Ships[snum].fuel);
      sshipsml.lock = (Sgn16)htons((Unsgn16)Ships[snum].lock);
      sshipsml.sdfuse = (Sgn16)htons((Unsgn16)Ships[snum].sdfuse);
      sshipsml.wfuse = (Sgn8)Ships[snum].wfuse;
      sshipsml.efuse = (Sgn8)Ships[snum].efuse;
      sshipsml.walloc = Ships[snum].weapalloc;
      sshipsml.etemp = (Unsgn8)Ships[snum].etemp;
      sshipsml.wtemp = (Unsgn8)Ships[snum].wtemp;
      dis = 0.0;	      /* we are very close to ourselves ;-) */
      canscan = TRUE;	      /* can always scan ourselves */
    }
  else
    {
      if (!SCLOAKED(snum))
        dis = (real) dist(Ships[mysnum].x, Ships[mysnum].y, 
                          Ships[snum].x, 
                          Ships[snum].y );
      else
        dis = (real) dist(Ships[mysnum].x, Ships[mysnum].y,
                          rndnor(Ships[snum].x, CLOAK_SMEAR_DIST),
                          rndnor(Ships[snum].y, CLOAK_SMEAR_DIST));
      
      /* if in accurate scanning distance (regardless of cloak) set the
         SCANDIST flag.  */

      if (dis < ACCINFO_DIST)
        scanflag = SHIP_F_SCANDIST;

      /* help the driver, set scanned fuse */
      if ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) && ! selfwar(mysnum) )
	Ships[snum].scanned[Ships[mysnum].team] = SCANNED_FUSE;

      /* if within accurate dist and not cloaked, or
	 if ship scanned by my team and not selfwar, or
	 if not at war with ship. */

      canscan = ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) ||
		  ( (Ships[snum].scanned[Ships[mysnum].team] > 0) && 
		    ! selfwar(mysnum) ) ||
		  !satwar(snum, mysnum));
    }

  if (canscan)
    {				/* if we get all the stats */
      sflags |= SHIP_F_SHUP;

      sshipsml.shields = (Unsgn8)Ships[snum].shields;
      sshipsml.damage = (Unsgn8)Ships[snum].damage;
      sshipsml.armies = Ships[snum].armies;

      /* so we can disp phasers in graphical client ;-) */
      sshipsml.lastphase = htons((Unsgn16)(Ships[snum].lastphase * 100.0));
      sshipsml.pfuse = (Sgn8)Ships[snum].pfuse;
    }

  /* only send those we are allowed to see */
  sshipsml.flags = (Unsgn16)htons(((Ships[snum].flags | scanflag) & sflags));

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
spShipLoc_t *spktShipLoc(Unsgn8 snum, int rec)
{
  int mysnum = Context.snum;
  static spShipLoc_t sshiploc;
  real x, y;
  int canscan = FALSE;
  real dis;
  static const Unsgn32 maxtime = 5000;  /* 5 seconds */
  static Unsgn32 lasttime = 0;
  Unsgn32 thetime = clbGetMillis();
  static int forceMyShip = TRUE;

  memset((void *)&sshiploc, 0, sizeof(spShipLoc_t));

  sshiploc.type = SP_SHIPLOC;
  sshiploc.snum = snum;
  sshiploc.warp = (Sgn8)(Ships[snum].warp * 10.0);

  /* we need to ensure that if we are doing UDP, and we haven't
     updated a ship in awhile (causing UDP traffic), force an update
     of your ship even if it isn't neccessary.  This should help with
     those firewalls that disconnect a UDP connection when there has
     been no trafic on it for a while. */
  if (sInfo.doUDP && (snum == mysnum) && ((thetime - lasttime) > maxtime))
    {
      lasttime = thetime;
      forceMyShip = TRUE;
    }

  /* RESTRICT */
  if ((snum == mysnum) || rec)
    {				/* we get everything */
      sshiploc.head = htons((Unsgn16)(Ships[snum].head * 10.0));
      x = Ships[snum].x;
      y = Ships[snum].y;
    }
  else
    {
      if (SCLOAKED(snum))
        {
          if (Ships[snum].warp == 0.0)
            {
              x = 1e7;
              y = 1e7;
            }
          else
            { /* if your cloaked, and moving, get smeared x/y */
              x = rndnor( Ships[snum].x, CLOAK_SMEAR_DIST );
              y = rndnor( Ships[snum].y, CLOAK_SMEAR_DIST );
            }
        }
      else
	{			/* not cloaked */
          dis = (real) dist(Ships[mysnum].x, Ships[mysnum].y, 
                            Ships[snum].x, 
                            Ships[snum].y );
          
          canscan = ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) ||
                      ( (Ships[snum].scanned[Ships[mysnum].team] > 0) && 
                        ! selfwar(mysnum) ) ||
                      !satwar(snum, mysnum));

	  if (canscan)		/* close or friendly */
	    sshiploc.head = htons((Unsgn16)(Ships[snum].head * 10.0));

	  x = Ships[snum].x;
	  y = Ships[snum].y;

	}
    }
  
  sshiploc.x = (Sgn32)htonl((Sgn32)(x * 1000.0));
  sshiploc.y = (Sgn32)htonl((Sgn32)(y * 1000.0));
  
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
          /* if we are doing udp, and we are going to send a packet,
             reset the timer so we won't need a nother force (as we do
             not need one now :) */
          if (sInfo.doUDP)
            lasttime = thetime;
          forceMyShip = FALSE;
          return &sshiploc;
        }
    }

  return NULL;
}

/* we have the potential to send 3 packets here. */
spPlanet_t *spktPlanet(Unsgn8 pnum, int rec)
{
  int snum = Context.snum;
  int team = Ships[snum].team;
  static spPlanet_t splan;

  memset((void *)&splan, 0, sizeof(spPlanet_t));

#if defined(DEBUG_SERVERSEND)
  clog("sendPlanet: pnum = %d",
       pnum);
#endif

  splan.type = SP_PLANET;
  splan.pnum = pnum;
  splan.ptype = Planets[pnum].type;

  /* RESTRICT */
  if (Planets[pnum].scanned[team] || rec)
    splan.team = Planets[pnum].team;
  else
    splan.team = TEAM_SELFRULED; /* until we know for sure... */

  strncpy(splan.name, Planets[pnum].name, MAXPLANETNAME);

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

spPlanetSml_t *spktPlanetSml(Unsgn8 pnum, int rec)
{
  int i;
  int snum = Context.snum;
  int team = Ships[snum].team;
  static spPlanetSml_t splansml;

  memset((void *)&splansml, 0, sizeof(spPlanetSml_t));

  splansml.type = SP_PLANETSML;
  splansml.pnum = pnum;

  /* RESTRICT */
  if (rec)
    {
      for (i=0; i < NUMPLAYERTEAMS; i++)
        if (Planets[pnum].scanned[i])
          splansml.scanned |= (1 << i);

      splansml.uninhabtime = (Unsgn8)Planets[pnum].uninhabtime;
    }
  else
    {
      if (Planets[pnum].scanned[team])
        {
          splansml.scanned |= (1 << team);
          splansml.uninhabtime = (Unsgn8)Planets[pnum].uninhabtime;
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

spPlanetLoc_t *spktPlanetLoc(Unsgn8 pnum, int rec, int force)
{
  int snum = Context.snum;
  int team = Ships[snum].team;
  static spPlanetLoc_t splanloc;
  real dx, dy;
  static real px[NUMPLANETS] = {}; /* saved x/y */
  static real py[NUMPLANETS] = {}; 
  
  memset((void *)&splanloc, 0, sizeof(spPlanetLoc_t));

  splanloc.type = SP_PLANETLOC;
  splanloc.pnum = pnum;

  /* RESTRICT */
  if (Planets[pnum].scanned[team] || rec)
    splanloc.armies = htons(Planets[pnum].armies);

  dx = (real)fabs(Planets[pnum].x - px[pnum]);
  dy = (real)fabs(Planets[pnum].y - py[pnum]);


  /* we try to be clever here by reducing the pkt count.  If armies are
     the same, and an average delta of the planet's movement is below
     an empirically determined value, don't bother sending the packet.
     
     The idea is that fast moving planets will be updated more frequently
     than slower moving ones, hopefully reducing the packet count required
     for the appearence of smoother movement to the user.
   */
  if ((splanloc.armies == pktPlanetLoc[pnum].armies) &&
      ((dx + dy) / 2.0) < 3.0)
    {
#if 0
      clog("REJECT: %s dx = %f dy = %f [%f]", Planets[pnum].name,
           dx, dy,
           ((dx + dy) / 2.0));
#endif
      
      if (!rec && !force)
        return NULL;
    }

  if (!rec)
    {
      px[pnum] = Planets[pnum].x;
      py[pnum] = Planets[pnum].y;
    }
  
#if 0
  clog("%s dx = %f dy = %f [%f]", Planets[pnum].name,
       dx, dy,
       ((dx + dy) / 2.0));
#endif
  
  splanloc.x = (Sgn32)htonl((Sgn32)(Planets[pnum].x * 1000.0));
  splanloc.y = (Sgn32)htonl((Sgn32)(Planets[pnum].y * 1000.0));

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

spPlanetLoc2_t *spktPlanetLoc2(Unsgn8 pnum, int rec, int force)
{
  int snum = Context.snum;
  int team = Ships[snum].team;
  static spPlanetLoc2_t splanloc2;
  Unsgn32 iternow = clbGetMillis(); /* we send the loc2 packets only every 5 secs */
  const Unsgn32 iterwait = 5000.0; /* ms */
  static Unsgn32 tstart[NUMPLANETS] = {}; /* saved time deltas */
  int tooearly = FALSE;
  
  /* 
   * We have to handle the case where a planet has just been freshly
   * scanned (and therefore a real army count is available).  We want
   * the client to see the true army count as soon as possible.
   */

  if (!force && (tstart[pnum] != 0 && ((iternow - tstart[pnum]) < iterwait)))
    tooearly = TRUE;
  
  memset((void *)&splanloc2, 0, sizeof(spPlanetLoc2_t));

  splanloc2.type = SP_PLANETLOC2;
  splanloc2.pnum = pnum;

  /* RESTRICT */
  if (Planets[pnum].scanned[team] || rec)
    splanloc2.armies = htons(Planets[pnum].armies);

  if (splanloc2.armies == pktPlanetLoc2[pnum].armies && tooearly)
    return NULL;

  tstart[pnum] = iternow; 

  splanloc2.x = (Sgn32)htonl((Sgn32)(Planets[pnum].x * 1000.0));
  splanloc2.y = (Sgn32)htonl((Sgn32)(Planets[pnum].y * 1000.0));
  splanloc2.orbang = (Unsgn16)htons((Unsgn16)(Planets[pnum].orbang * 100.0));

  if (rec)
    {
      if (memcmp((void *)&splanloc2, (void *)&recPlanetLoc2[pnum],
                 sizeof(spPlanetLoc2_t)))
        {
          recPlanetLoc2[pnum] = splanloc2;
          return &splanloc2;
        }
    }
  else
    {
      if (memcmp((void *)&splanloc2, (void *)&pktPlanetLoc2[pnum],
                 sizeof(spPlanetLoc2_t)))
        {
          pktPlanetLoc2[pnum] = splanloc2;
          return &splanloc2;
        }
    }

  return NULL;
}



/* non priv */
spTorp_t *spktTorp(Unsgn8 tsnum, Unsgn8 tnum, int rec)
{
  static spTorp_t storp;

  memset((void *)&storp, 0, sizeof(spTorp_t));

  storp.type = SP_TORP;
  storp.snum = tsnum;
  storp.tnum = tnum;
  storp.status = (Unsgn8)Ships[tsnum].torps[tnum].status;

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
spTorpLoc_t *spktTorpLoc(Unsgn8 tsnum, Unsgn8 tnum, int rec)
{
  int i;
  int snum = Context.snum;
  int team = Ships[snum].team;
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

  x = Ships[tsnum].torps[tnum].x;
  y = Ships[tsnum].torps[tnum].y;

  if (Ships[tsnum].torps[tnum].war[team] && !rec)
    {				/* it's at war with us. bastards. */
      /* see if it's close enough to scan */
      dis = (real) dist(Ships[snum].x, Ships[snum].y, 
			Ships[tsnum].torps[tnum].x, 
			Ships[tsnum].torps[tnum].y );

      if (dis > ACCINFO_DIST)
        {                       /* in the bermuda triangle */
          x = 1e7;
          y = 1e7;
        }
    }

  storploc.x = (Sgn32)htonl((Sgn32)(x * 1000.0));
  storploc.y = (Sgn32)htonl((Sgn32)(y * 1000.0));

  if (rec)
    {
      for (i=0; i < NUMPLAYERTEAMS; i++)
        if (Ships[tsnum].torps[tnum].war[i])
          storploc.war |= (1 << i);
    }
  else
    {
      /* RESTRICT */
      /* only send 'war' status as it relates to our team */
      if (Ships[tsnum].torps[tnum].war[team])
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
spTorpEvent_t *spktTorpEvent(Unsgn8 tsnum, Unsgn8 tnum, int rec)
{
  int i;
  int snum = Context.snum;
  int team = Ships[snum].team;
  static spTorpEvent_t storpev;
  real x, y, dx, dy;

  memset((void *)&storpev, 0, sizeof(spTorpEvent_t));

  storpev.type = SP_TORPEVENT;
  storpev.snum = tsnum;
  storpev.tnum = tnum;
  storpev.status = (Unsgn8)Ships[tsnum].torps[tnum].status;

  /* RESTRICT */
  /* we can always see friendly torps.  enemy torps can only be seen if 
     within ACCINFO_DIST of your ship.  torp war stat only applies to
     your ship. */

  x = Ships[tsnum].torps[tnum].x;
  y = Ships[tsnum].torps[tnum].y;
  dx = Ships[tsnum].torps[tnum].dx;
  dy = Ships[tsnum].torps[tnum].dy;

  storpev.x = (Sgn32)htonl((Sgn32)(x * 1000.0));
  storpev.y = (Sgn32)htonl((Sgn32)(y * 1000.0));
  storpev.dx = (Sgn32)htonl((Sgn32)(dx * 1000.0));
  storpev.dy = (Sgn32)htonl((Sgn32)(dy * 1000.0));

  if (rec)
    {
      for (i=0; i < NUMPLAYERTEAMS; i++)
        if (Ships[tsnum].torps[tnum].war[i])
          storpev.war |= (1 << i);
    }
  else
    {
      /* RESTRICT */
      /* only send 'war' status as it relates to our team */
      if (Ships[tsnum].torps[tnum].war[team])
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
spTeam_t *spktTeam(Unsgn8 team, int force, int rec)
{
  int snum = Context.snum;
  static spTeam_t steam;
  int i;

  memset((void *)&steam, 0, sizeof(spTeam_t));

  steam.type = SP_TEAM;
  steam.team = team;
  steam.homesun = Teams[team].homesun;

  /* RESTRICT */
  if ((Ships[snum].team == team) || rec)
    {				/* we only send this stuff for our team */
      if (Teams[team].coupinfo)
	steam.flags |= SPTEAM_FLAGS_COUPINFO;
      
      steam.couptime = (Unsgn8)Teams[team].couptime;
    }

  for (i=0; i<3; i++)
    steam.teamhplanets[i] = (Unsgn8)Teams[team].teamhplanets[i];

  steam.homeplanet = Teams[team].homeplanet;

  for (i=0; i<MAXTSTATS; i++)
    steam.stats[i] = (Unsgn32)htonl(Teams[team].stats[i]);

  strncpy(steam.name, Teams[team].name, MAXTEAMNAME - 1);
  
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

spConqInfo_t *spktConqInfo(int force)
{
  static spConqInfo_t spci;

  memset((void *)&spci, 0, sizeof(spConqInfo_t));

  spci.type = SP_CONQINFO;

  strncpy(spci.conqueror, ConqInfo->conqueror, MAXUSERPNAME - 1);
  strncpy(spci.conqteam, ConqInfo->conqteam, MAXTEAMNAME - 1);
  strncpy(spci.conqtime, ConqInfo->conqtime, DATESIZE - 1);
  strncpy(spci.lastwords, ConqInfo->lastwords, MAXLASTWORDS - 1);
  
  if (memcmp((void *)&spci, (void *)&pktConqInfo,
             sizeof(spConqInfo_t)) || force)
    {
      pktConqInfo = spci;
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

  hist.histptr = ConqInfo->histptr;

  hist.unum = (Unsgn16)htons((Unsgn16)History[hnum].histunum);

  hist.elapsed = (Unsgn32)htonl((Unsgn32)History[hnum].elapsed);
  hist.histlog = (Unsgn32)htonl((Unsgn32)History[hnum].histlog);

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
  dd.status = (Unsgn8)Doomsday->status;
  dd.heading = htons((Unsgn16)(Doomsday->heading * 10.0));
  dd.x = (Sgn32)htonl((Sgn32)(Doomsday->x * 1000.0));
  dd.y = (Sgn32)htonl((Sgn32)(Doomsday->y * 1000.0));

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

spPlanetInfo_t *spktPlanetInfo(Unsgn8 pnum, int rec)
{
  static spPlanetInfo_t splaninfo;

  memset((void *)&splaninfo, 0, sizeof(spPlanetInfo_t));

  splaninfo.flags = SPPLANETINFO_FLAGS_FVALID;

  splaninfo.type = SP_PLANETINFO;
  splaninfo.pnum = pnum;

  if (Planets[pnum].real)
    splaninfo.flags |= SPPLANETINFO_FLAGS_REAL;

  splaninfo.primary = (Unsgn8)Planets[pnum].primary;

  splaninfo.orbrad = (Unsgn32)htonl((Unsgn32)(Planets[pnum].orbrad * 10.0));
  splaninfo.orbvel = (Sgn32)htonl((Sgn32)(Planets[pnum].orbvel * 100.0)); 

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

