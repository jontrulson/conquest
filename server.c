#include "c_defs.h"

/************************************************************************
 *
 * server specific stuff
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "conqlb.h"
#include "conf.h"
#include "server.h"
#include "serverpkt.h"
#include "context.h"
#include "record.h"

int sendClientStat(int sock, Unsgn8 flags, Unsgn8 snum, Unsgn8 team, 
		   Unsgn16 unum, Unsgn8 esystem)
{
  spClientStat_t scstat;

  memset((void *)&scstat, 0, sizeof(spClientStat_t));

#if defined(DEBUG_SERVERCLNTSTAT)
  clog("sendClientStat: snum = %d, unum = %d, flags = 0x%x, team = %d",
       snum, unum, flags, team);
#endif

  scstat.type = SP_CLIENTSTAT;
  scstat.flags = flags;
  scstat.snum = snum;
  scstat.team = team;
  scstat.unum = htons(unum);
  scstat.esystem = esystem;

  if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)&scstat) <= 0)
    return FALSE;
  else
    return TRUE;
}

int sendUser(int sock, Unsgn16 unum)
{
  spUser_t *suser;

#if defined(DEBUG_SERVERSEND)
  clog("sendUser: unum = %d",
       unum);
#endif

  /* not really any priv bits, but in case we add some later */
  if ((suser = spktUser(unum)))
    {
      if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)suser) <= 0)
        return FALSE;

      if (Context.recmode == RECMODE_ON)
        recordWriteEvent((Unsgn8 *)suser);
    }

  return TRUE;
}

/* we have the potential to send a few packets here. */
int sendShip(int sock, Unsgn8 snum)
{
  spShip_t *sship;
  spShipSml_t *sshipsml;
  spShipLoc_t *sshiploc;

#if defined(DEBUG_SERVERSEND)
  clog("sendShip: snum = %d",
       snum);
#endif

  /* SP_SHIP */
  if (Context.recmode == RECMODE_ON)
    {
      if ((sship = spktShip(snum, TRUE)))
        recordWriteEvent((Unsgn8 *)sship);
    }

  if ((sship = spktShip(snum, FALSE)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)sship) <= 0)
      return FALSE;

  /* SP_SHIPSML */
  if (Context.recmode == RECMODE_ON)
    {
      if ((sshipsml = spktShipSml(snum, TRUE)))
        recordWriteEvent((Unsgn8 *)sshipsml);
    }

  if ((sshipsml = spktShipSml(snum, FALSE)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)sshipsml) <= 0)
      return FALSE;

  /* SP_SHIPLOC */

  if (Context.recmode == RECMODE_ON)
    {
      if ((sshiploc = spktShipLoc(snum, TRUE)))
        recordWriteEvent((Unsgn8 *)sshiploc);
    }
  
  if ((sshiploc = spktShipLoc(snum, FALSE)))
    {
      if (sInfo.doUDP)
        {
          if (writePacket(PKT_TOCLIENT, sInfo.usock, (Unsgn8 *)sshiploc) <= 0)
            return FALSE;
        }
      else
        {
          if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)sshiploc) <= 0)
            return FALSE;
        }
    }
  return TRUE;
}

/* we have the potential to send 3 packets here. */
int sendPlanet(int sock, Unsgn8 pnum)
{
  spPlanet_t *splan;
  spPlanetSml_t *splansml;
  spPlanetLoc_t *splanloc;

#if defined(DEBUG_SERVERSEND)
  clog("sendPlanet: pnum = %d",
       pnum);
#endif

  /* SP_PLANET */
  if (Context.recmode == RECMODE_ON)
    {
      if ((splan = spktPlanet(pnum, TRUE)))
        recordWriteEvent((Unsgn8 *)splan);
    }

  if ((splan = spktPlanet(pnum, FALSE)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)splan) <= 0)
      return FALSE;

  /* SP_PLANETSML */
  if (Context.recmode == RECMODE_ON)
    {
      if ((splansml = spktPlanetSml(pnum, TRUE)))
        recordWriteEvent((Unsgn8 *)splansml);
    }

  if ((splansml = spktPlanetSml(pnum, FALSE)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)splansml) <= 0)
      return FALSE;

  /* SP_PLANETLOC */
  if (Context.recmode == RECMODE_ON)
    {
      if ((splanloc = spktPlanetLoc(pnum, TRUE)))
        recordWriteEvent((Unsgn8 *)splanloc);
    }

  if ((splanloc = spktPlanetLoc(pnum, FALSE)))
    {
      if (sInfo.doUDP)
        {
          if (writePacket(PKT_TOCLIENT, sInfo.usock, (Unsgn8 *)splanloc) <= 0)
            return FALSE;
        }
      else
        {
          if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)splanloc) <= 0)
            return FALSE;
        }

    }
  return TRUE;
}

/* send a status packet */
int sendServerStat(int sock)
{
  int i;
  int numusers = 0;
  int numships = 0;
  int numshipsactive = 0;
  int numshipsvacant = 0;
  int numshipsrobot = 0;
  spServerStat_t sStat;

  memset((void *)&sStat, 0, sizeof(spServerStat_t));

#if defined(DEBUG_SERVERSEND)
  clog("sendServerStats: ENTER");
#endif

  /* get total users */
  for ( i = 0; i < MAXUSERS; i++)
    if ( Users[i].live)
      numusers++;
  
  /* count ships */
  for ( i = 1; i <= MAXSHIPS; i++ )
    {
      if ( Ships[i].status == SS_LIVE )
	{
	  numships++;
	  
	  if (SROBOT(i))
	    numshipsrobot++;
	  else
	    {
	      if (SVACANT(i))
		numshipsvacant++;
	      else
		numshipsactive++;
	    }
	}
    }
  

  sStat.type = SP_SERVERSTAT;
  sStat.numactive = numshipsactive;
  sStat.numvacant = numshipsvacant;
  sStat.numrobot = numshipsrobot;
  sStat.numtotal = numships;
  sStat.numusers = htons(numusers);

  /* send out the current flags */
  sStat.flags = getServerFlags();

  sStat.flags = (Unsgn32)htonl(sStat.flags);

  sStat.servertime = (Unsgn32)htonl(getnow(NULL, 0));
  
  if (!writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)&sStat))
    {
      clog("sendServerStats: writePacket failed\n");
      return FALSE;
    }

  return TRUE;
}

int sendTorp(int sock, Unsgn8 tsnum, Unsgn8 tnum)
{
  spTorp_t *storp;
  spTorpLoc_t *storploc;

  /* no point in sending torp data if we're not playing */
  if (sInfo.state != SVR_STATE_PLAY)
    return TRUE;

  if (tsnum <= 0 || tsnum > MAXSHIPS)
    return FALSE;
  
  if (tnum >= MAXTORPS)
    return FALSE;

#if defined(DEBUG_SERVERSEND)
  clog("sendTorp: %d %d", tsnum, tnum);
#endif

  /* SP_TORP */

  if (Context.recmode == RECMODE_ON)
    {
      if ((storp = spktTorp(tsnum, tnum, TRUE)))
        recordWriteEvent((Unsgn8 *)storp);
    }

  if ((storp = spktTorp(tsnum, tnum, FALSE)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)storp) <= 0)
      return FALSE;
  
  /* SP_TORPLOC */

  if (Context.recmode == RECMODE_ON)
    {
      if ((storploc = spktTorpLoc(tsnum, tnum, TRUE)))
        recordWriteEvent((Unsgn8 *)storploc);
    }

  if ((storploc = spktTorpLoc(tsnum, tnum, FALSE)))
    {
      if (sInfo.doUDP)
        {
          if (writePacket(PKT_TOCLIENT, sInfo.usock, (Unsgn8 *)storploc) <= 0)
            return FALSE;
        }
      else
        {
          if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)storploc) <= 0)
            return FALSE;
        }
    }

  return TRUE;
}

int sendMessage(int sock, Msg_t *msg)
{
  spMessage_t smsg;

  if (sInfo.state != SVR_STATE_PLAY)
    return TRUE;

  if (!msg)
    return TRUE;

  memset((void *)&smsg, 0, sizeof(spMessage_t));

#if defined(DEBUG_SERVERSEND) 
  clog("sendMessage: to = %d, from = %d, flags = 0x%02x, msg = '%s'",
       msg->msgto, msg->msgfrom, msg->flags, msg->msgbuf);
#endif

  smsg.type = SP_MESSAGE;
  
  smsg.from = (Sgn16)htons(msg->msgfrom);
  smsg.to = (Sgn16)htons(msg->msgto);
  smsg.flags = msg->flags;
  
  strncpy(smsg.msg, msg->msgbuf, MESSAGE_SIZE - 1);

  /* don't record feeback or tersable msgs */
  if (Context.recmode == RECMODE_ON)
    if (!(smsg.flags & MSG_FLAGS_FEEDBACK) && 
        !(smsg.flags & MSG_FLAGS_TERSABLE))
      recordWriteEvent((Unsgn8 *)&smsg);

  if (!writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)&smsg))
    {
      clog("sendMessage: writePacket failed\n");
      return FALSE;
    }

  return TRUE;
}

int sendTeam(int sock, Unsgn8 team, int force)
{
  spTeam_t *steam;

#if defined(DEBUG_SERVERSEND)
  clog("sendTeam: team = %d, f = %d", team, force);
#endif

  if (Context.recmode == RECMODE_ON)
    {
      if ((steam = spktTeam(team, force, TRUE)))
        recordWriteEvent((Unsgn8 *)steam);
    }

  if ((steam = spktTeam(team, force, FALSE)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)steam) <= 0)
      return FALSE;

  return TRUE;
}

int sendConqInfo(int sock, int force)
{
  spConqInfo_t *spci;

#if defined(DEBUG_SERVERSEND)
  clog("sendConqInfo: f = %d", force);
#endif

  if ((spci = spktConqInfo(force)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)spci) <= 0)
      return FALSE;

  return TRUE;
}

int sendHistory(int sock, int hnum)
{
  spHistory_t *shist;

  if (hnum < 0 || hnum > MAXHISTLOG)
    return FALSE;

#if defined(DEBUG_SERVERSEND)
  clog("sendHistory: hnum = %d", hnum);
#endif

  if ((shist = spktHistory(hnum)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)shist) <= 0)
      return FALSE;

  return TRUE;
}

int sendDoomsday(int sock)
{
  spDoomsday_t *dd;

#if defined(DEBUG_SERVERSEND)
  clog("sendDoomsday");
#endif

  if (Context.recmode == RECMODE_ON)
    {
      if ((dd = spktDoomsday(TRUE)))
        recordWriteEvent((Unsgn8 *)dd);
    }

  if ((dd = spktDoomsday(FALSE)))
    if (writePacket(PKT_TOCLIENT, sock, (Unsgn8 *)dd) <= 0)
      return FALSE;

  return TRUE;
}
  

void procSetName(Unsgn8 *buf)
{
  cpSetName_t *cpsetn = (cpSetName_t *)buf;

  if (!validPkt(CP_SETNAME, cpsetn))
    return;

  cpsetn->alias[MAXUSERPNAME - 1] = 0;

  strncpy(Users[Context.unum].alias, cpsetn->alias, MAXUSERPNAME);

  if (Context.snum > 0 && Context.snum <= MAXSHIPS)
    strncpy(Ships[Context.snum].alias, cpsetn->alias, MAXUSERPNAME);

  return;
}

void procSetCourse(Unsgn8 *buf)
{
  cpSetCourse_t *csc = (cpSetCourse_t *)buf;
  int lock;
  real dir;

  if (!validPkt(CP_SETCOURSE, csc))
    return;

  lock = (int)csc->lock;
  dir = (real)((real)ntohs(csc->head) / 100.0);

#if defined(DEBUG_SERVERPROC)
  clog("PROC SETCOURSE: lock = %d, head = %f",
       lock, dir);
#endif

  /* now we need to do a few checks to make sure it's legal ;-) */

  if (dir < 0.0)
    dir = 0.0;

  if (dir > 359.9)
    dir = 359.9;

  if (lock > 0)			/* could lock onto ships someday... */
    lock = 0;			/* but not today. */

  if (lock < 0 && (-lock > NUMPLANETS))
    lock = 0;

  /* always applies to our own ship */
  if ( Ships[Context.snum].warp < 0.0 )	/* if orbitting */
    Ships[Context.snum].warp = 0.0;	/* break orbit */
  Ships[Context.snum].dhead = dir;	/* set direction first to avoid */
  Ships[Context.snum].lock = lock;	/*  a race in display() */

  return;
}

void procSetWarp(cpCommand_t *swarp)
{
  int snum = Context.snum;		/* we always use our own ship */
  char cbuf[BUFFER_SIZE];
  real warp;
  real mw;

  if (!validPkt(CP_COMMAND, swarp))
    return;

  if (swarp->cmd != CPCMD_SETWARP)
    return;

  warp = (real)ntohs(swarp->detail);

#if defined(DEBUG_SERVERPROC)
  clog("PROC SETWARP: warp = %f", warp);
#endif

  if ( Ships[snum].dwarp == 0.0 && warp != 0.0 )
    {
      /* See if engines are working. */
      if ( Ships[snum].efuse > 0 )
	{
	  clbStoreMsgf(MSG_COMP, snum, 
                   "Engines are currently overloaded.", 
                   MSG_FLAGS_FEEDBACK);
	  return;
	}
            /* No charge if already warp 0. */
      if ( clbUseFuel( snum, ENGINES_ON_FUEL, FALSE, TRUE ) == FALSE)
	{
	  clbStoreMsgf(MSG_COMP, snum, 
                   "We don't have enough fuel.", 
                   MSG_FLAGS_FEEDBACK);
	  return;
	}
      
      /* Don't stop repairing if changing to warp 0. */
      SFCLR(snum, SHIP_F_REPAIR);
    }
  
  /* If orbitting, break orbit. */
  if ( Ships[snum].warp < 0.0 )
    {
      Ships[snum].warp = 0.0;
      Ships[snum].lock = 0;
      Ships[snum].dhead = Ships[snum].head;
    }
  
  /* Handle ship limitations. */
  Ships[snum].dwarp = min( warp, ShipTypes[Ships[snum].shiptype].warplim );
  
  /* Warn about damage limitations. */
  mw = maxwarp( snum );
  if ( around( Ships[snum].dwarp ) > mw )
    {
      sprintf(cbuf, 
              "(Due to damage, warp is currently limited to %.1f.)", mw);
      clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
    }

  return;
}

void procSetShields(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int shup;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_SETSHIELDS)
    return;

  shup = (((int)ntohs(cmd->detail)) ? TRUE : FALSE);

#if defined(DEBUG_SERVERPROC)
  clog("PROC SETSHIELDS: sh = %d", shup);
#endif

  if (shup)
    {
      SFSET(snum, SHIP_F_SHUP);
      SFCLR(snum, SHIP_F_REPAIR);
    }
  else
    SFCLR(snum, SHIP_F_SHUP);

  return;
}

void procAlloc(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int alloc;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_ALLOC)
    return;

  /* we only deal with weapons alloc.  we compute engalloc based on this */
  alloc = (int)ntohs(cmd->detail);

  if (alloc < 30)
    alloc = 30;

  if (alloc > 70)
    alloc = 70;

#if defined(DEBUG_SERVERPROC)
  clog("PROC ALLOC: (w)alloc = %d", alloc);
#endif

  Ships[snum].weapalloc = alloc;
  Ships[snum].engalloc = (100 - alloc);

  return;
}

 
void procCloak(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  string nofuel="Not enough fuel to engage cloaking device.";

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_CLOAK)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC CLOAK");
#endif


  if ( SCLOAKED(snum) )
    {
      SFCLR(snum, SHIP_F_CLOAKED);
      return;
    }
  if ( Ships[snum].efuse > 0 )
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "Engines are currently overloaded.", 
               MSG_FLAGS_FEEDBACK);
      return;
    }
  if ( Ships[snum].fuel < CLOAK_ON_FUEL )
    {
      clbStoreMsgf(MSG_COMP, snum, nofuel, MSG_FLAGS_FEEDBACK);
      return;
    }

  SFCLR(snum, SHIP_F_REPAIR);
  if ( ! clbUseFuel( snum, CLOAK_ON_FUEL, FALSE, TRUE ) )
    {
      clbStoreMsgf(MSG_COMP, snum, nofuel, MSG_FLAGS_FEEDBACK);
      return;
    }

  SFSET(snum, SHIP_F_CLOAKED);

  return;
  
}

void procDetSelf(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int j;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_DETSELF)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC DETSELF");
#endif

  for ( j = 0; j < MAXTORPS; j++ )
    clbDetonate( snum, j );

  return;
}

void procDetEnemy(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_DETENEMY)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC DETENEMY");
#endif

  clbEnemyDet( snum );

  return;
}

void procDistress(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  char buf[128], cbuf[128];
  real x;
  int i, isorb = FALSE;
  int tofriendly;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_DISTRESS)
    return;

  tofriendly = (int)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
  clog("PROC DISTRESS: tofriendly = %d", tofriendly);
#endif

  sprintf( cbuf,
	   "sh=%d %c, dam=%d, fuel=%d, temp=",
	   round(Ships[snum].shields),
	   (SSHUP(snum)) ? 'U' : 'D',
	   round(Ships[snum].damage),
	   round(Ships[snum].fuel) );

  i = round(Ships[snum].wtemp);
  if ( i < 100 )
    appint( i, cbuf );
  else
    appstr( "**", cbuf );
  appchr( '/', cbuf );
  i = round(Ships[snum].etemp);
  if ( i < 100 )
    appint( i, cbuf );
  else
    appstr( "**", cbuf );
  i = Ships[snum].armies;

  if ( i > 0 )
    {
      appstr( ", arm=", cbuf );
      appint( i, cbuf );
    }

  if ( Ships[snum].wfuse > 0 )
    appstr( ", -weap", cbuf );

  if ( Ships[snum].efuse > 0 )
    appstr( ", -eng", cbuf );
  
  /* warp */
  x = Ships[snum].warp;
  if ( x >= 0.0 )
    {
      sprintf( buf, ", warp=%.1f", x );
      appstr(buf, cbuf);
      isorb = FALSE;
    }
  else
    {
      sprintf( buf, ", orbiting %.3s", 
	       Planets[-Ships[snum].lock].name );
      appstr(buf, cbuf);
      isorb = TRUE;
    }
  
  /* heading */
  
  if (isorb == FALSE)
    {
      i = Ships[snum].lock;
      
      if ( i >= 0 || Ships[snum].warp < 0.0)
	i = round( Ships[snum].head );
      
      if ( -i > 0 && -i <= NUMPLANETS)
	sprintf( buf, ", head=%.3s", Planets[-i].name );
      else
	sprintf( buf, ", head=%d", i );
      
      appstr(buf, cbuf);
    }
  
  if (tofriendly)
    clbStoreMsg( snum, MSG_FRIENDLY, cbuf );
  else
    clbStoreMsg( snum, -Ships[snum].team, cbuf );
  
  return;
  
}

void procFirePhaser(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  real dir;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_FIREPHASER)
    return;

  dir = (real)ntohs(cmd->detail) / 100.0;

  if (dir < 0)
    dir = 0.0;

  if (dir > 359.9)
    dir = 359.9;

#if defined(DEBUG_SERVERPROC)
  clog("PROC FIREPHASER: dir = %f", dir);
#endif

  if ( SCLOAKED(snum) )
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "The cloaking device is using all available power.",
               MSG_FLAGS_FEEDBACK);
      return;
    }

  if ( Ships[snum].wfuse > 0 )
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "Weapons are currently overloaded.", 
               MSG_FLAGS_FEEDBACK);
      return;
    }

  if ( Ships[snum].fuel < PHASER_FUEL )
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "Not enough fuel to fire phasers.", 
               MSG_FLAGS_FEEDBACK);
      return;
    }
  
  if ( !clbPhaser( snum, dir ) )
    clbStoreMsgf(MSG_COMP, snum, ">PHASERS DRAINED<", MSG_FLAGS_FEEDBACK);

  return;
}

void procOrbit(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int pnum;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_ORBIT)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC ORBIT");
#endif

  if ( ( Ships[snum].warp == ORBIT_CW ) || ( Ships[snum].warp == ORBIT_CCW ) )
    return;

  if ( ! clbFindOrbit( snum, &pnum ) )
    return;

  if ( Ships[snum].warp > MAX_ORBIT_WARP )
    return;

  clbOrbit( snum, pnum );
  
  return;
}

void procRepair(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_REPAIR)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC REPAIR");
#endif

  if ( ! SCLOAKED(snum) )
    {
      SFSET(snum, SHIP_F_REPAIR);
      Ships[snum].dwarp = 0.0;
    }

  return;
}

void procCoup(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int i, pnum, now, entertime;
  real failprob;
  string nhp="We must be orbiting our home planet to attempt a coup.";
  char cbuf[128];

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_COUP)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC COUP");
#endif

  /* Check for allowability. */
  if ( oneplace( Ships[snum].kills ) < MIN_COUP_KILLS )
    {
      clbStoreMsgf(MSG_COMP, snum,
	      "Fleet orders require three kills before a coup can be attempted.",
	       MSG_FLAGS_FEEDBACK);
      return;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( Planets[i].team == Ships[snum].team && Planets[i].armies > 0 )
      {
	 clbStoreMsgf(MSG_COMP, snum,
		 "We don't need to coup, we still have armies left!",
		  MSG_FLAGS_FEEDBACK);
	return;
      }

  if ( Ships[snum].warp >= 0.0 )
    {
      clbStoreMsgf(MSG_COMP, snum, nhp, MSG_FLAGS_FEEDBACK );
      return;
    }

  pnum = -Ships[snum].lock;
  if ( pnum != Teams[Ships[snum].team].homeplanet )
    {
      clbStoreMsgf(MSG_COMP, snum, nhp, MSG_FLAGS_FEEDBACK );
      return;
    }
  if ( Planets[pnum].armies > MAX_COUP_ENEMY_ARMIES )
    {
      clbStoreMsgf(MSG_COMP, snum, 
	      "The enemy is still too strong to attempt a coup.",
	       MSG_FLAGS_FEEDBACK);
      return;
    }
  i = Planets[pnum].uninhabtime;
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
	     i );
      clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
      return;
    }
  
  /* Now our team can tell coup time for free. */
  Teams[Ships[snum].team].coupinfo = TRUE;
  
  i = Teams[Ships[snum].team].couptime;
  if ( i > 0 )
    {
      sprintf( cbuf, "Our forces need %d more minutes to organize.", i );
      clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
      return;
    }
  
  /* Now wait it out... */
  clbStoreMsgf( MSG_COMP, snum, "Attempting coup...", MSG_FLAGS_FEEDBACK );
  grand( &entertime );
  while ( dgrand( entertime, &now ) < COUP_GRAND )
    {
      /* See if we're still alive. */
      if ( ! clbStillAlive( Context.snum ) )
	return;
      
      /* Sleep */
      c_sleep( ITER_SECONDS );
    }
  
  PVLOCK(&ConqInfo->lockword);
  if ( Planets[pnum].team == Ships[snum].team )
    {
      PVUNLOCK(&ConqInfo->lockword);
      clbStoreMsgf( MSG_COMP, snum, 
	       "Sensors show hostile forces eliminated from the planet.",
		MSG_FLAGS_FEEDBACK);
      return;
    }
  
  failprob = Planets[pnum].armies / MAX_COUP_ENEMY_ARMIES * 0.5 + 0.5;

  if ( rnd() < failprob )
    {
      /* Failed; setup new reorganization time. */
      Teams[Ships[snum].team].couptime = rndint( 5, 10 );
      PVUNLOCK(&ConqInfo->lockword);
      clbStoreMsgf( MSG_COMP, snum, "Coup unsuccessful.", MSG_FLAGS_FEEDBACK );
      return;
    }
  
  clbTakePlanet( pnum, snum );
  Planets[pnum].armies = rndint( 10, 20 );	/* create token coup force */
  Users[Ships[snum].unum].stats[USTAT_COUPS] += 1;
  Teams[Ships[snum].team].stats[TSTAT_COUPS] += 1;
  PVUNLOCK(&ConqInfo->lockword);
  clbStoreMsgf( MSG_COMP, snum, "Coup successful!", MSG_FLAGS_FEEDBACK );

  /* force a team update for this ship */
  sendTeam(sInfo.sock, Ships[snum].team, TRUE);
  
  return;
}
  
void procFireTorps(Unsgn8 *buf)
{
  int snum = Context.snum;		/* we always use our own ship */
  cpFireTorps_t *cftorp = (cpFireTorps_t *)buf;
  real dir;
  int num;

  if (!validPkt(CP_FIRETORPS, cftorp))
    return;

  dir = (real)((real)ntohs(cftorp->dir) / 100.0);
  num = (int)cftorp->num;

  if (dir < 0)
    dir = 0.0;
  
  if (dir > 359.9)
    dir = 359.9;

#if defined(DEBUG_SERVERPROC)
  clog("PROC FIRETORPS: dir = %f, num = %d", dir, num);
#endif

  if ( SCLOAKED(snum) )
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "The cloaking device is using all available power.",
	       MSG_FLAGS_FEEDBACK);
      return;
    }

  if ( Ships[snum].wfuse > 0 )
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "Weapons are currently overloaded.", 
               MSG_FLAGS_FEEDBACK);
      return;
    }

  if ( Ships[snum].fuel < TORPEDO_FUEL )
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "Not enough fuel to launch a torpedo.", 
               MSG_FLAGS_FEEDBACK);
      return;
    }

  if ( ! clbLaunch( snum, dir, num, LAUNCH_NORMAL ) )
    {
      clbStoreMsgf(MSG_COMP, snum, ">TUBES EMPTY<", MSG_FLAGS_FEEDBACK);
    }
  
  return;
  
}

void procMessage(Unsgn8 *buf)
{
  int snum = Context.snum;		/* we always use our own ship */
  cpMessage_t *cmsg = (cpMessage_t *)buf;
  int to;
 
  if (sInfo.state != SVR_STATE_PLAY)
    return;
   
  if (!validPkt(CP_MESSAGE, cmsg))
    return;

  to = (Sgn16)ntohs(cmsg->to);
  cmsg->msg[MESSAGE_SIZE - 1] = 0;

#if defined(DEBUG_SERVERPROC)
  clog("PROC MESSAGE: to %d", to);
#endif

  clbStoreMsg(snum, to, cmsg->msg);
  checkOperExec(snum, to, cmsg->msg);

  return;
}

void procChangePassword(Unsgn8 *buf)
{
  char salt[3];
  int unum = Context.unum;
  cpAuthenticate_t *cauth = (cpAuthenticate_t *)buf;

  if (!validPkt(CP_AUTHENTICATE, buf))
    return;

  cauth->pw[MAXUSERNAME - 1] = 0;

  /* we are just interested in the pw */
 
#if defined(DEBUG_SERVERPROC)
  clog("PROC ChangePassword");
#endif
 
  salt[0] = (Users[unum].username[0] != EOS) ? Users[unum].username[0] :
    'J';
  salt[1] = (Users[unum].username[1] != EOS) ? Users[unum].username[1] :
    'T';
  salt[2] = EOS;

  strncpy(Users[unum].pw, (char *)crypt(cauth->pw, salt), MAXUSERNAME - 2);
  Users[unum].pw[MAXUSERNAME - 1] = EOS;

  return;
}


void procSetWar(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int unum = Context.unum;
  int dowait = FALSE, entertime, now, i;
  Unsgn8 war;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_SETWAR)
    return;

  war = (Unsgn8)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
  clog("PROC SETWAR war = 0x%02x", war);
#endif

  for (i=0; i<NUMPLAYERTEAMS; i++)
    {
      if (war & (1 << i))
	{
	  if (!Ships[Context.snum].war[i]) /* if not at war, we will delay */
	    dowait = TRUE;
	  
	  Ships[snum].war[i] = TRUE;
	}
      else
	{
	  Ships[snum].war[i] = FALSE;
	}
      
      Users[unum].war[i] = Ships[snum].war[i];
    } 
  
  /* now we 'sleep' for awhile, if dowait is set */
  /* any packets the client tries to send will have to wait ;-) */
  if (dowait && Ships[Context.snum].status != SS_RESERVED)
    {
      grand( &entertime );
      while ( dgrand( entertime, &now ) < REARM_GRAND )
	{
	  /* See if we're still alive. */
	  if ( ! clbStillAlive( Context.snum ) )
	    return;
	  
	  /* Sleep */
	  c_sleep( ITER_SECONDS );
	}
    }

  return;
}

void procRefit(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int entertime, now;
  int stype;
  int pnum;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_REFIT)
    return;

  stype = (int)ntohs(cmd->detail);

  if (stype < 0 || stype >= MAXNUMSHIPTYPES)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC REFIT: snum - %d, stype = %d", snum, stype);
#endif
  
  /* Check for allowability. */
  if ( oneplace( Ships[snum].kills ) < MIN_REFIT_KILLS )
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "You must have at least one kill to refit.",
               MSG_FLAGS_FEEDBACK);
      return;
    }

  pnum = -Ships[snum].lock;

  if (Planets[pnum].team != Ships[snum].team || Ships[snum].warp >= 0.0)
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "We must be orbiting a team owned planet to refit.",
               MSG_FLAGS_FEEDBACK);
      return;
    }

  if (Ships[snum].armies != 0)
    {
      clbStoreMsgf(MSG_COMP, snum, 
               "You cannot refit while carrying armies",
               MSG_FLAGS_FEEDBACK);
      return;
    }
  
  /* now we wait for a bit. */
  grand( &entertime );
  while ( dgrand( entertime, &now ) < REFIT_GRAND )
    {
      /* See if we're still alive. */
      if ( ! clbStillAlive( snum ) )
	return;
      
      /* Sleep */
      c_sleep( ITER_SECONDS );
    }
  
  /* set it */
  Ships[snum].shiptype = stype;

  return;
}

void procSetRate(cpCommand_t *cmd)
{
#if defined(DEBUG_SERVERPROC)
  int snum = Context.snum;
#endif
  int rate;

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_SETRATE)
    return;

  rate = (int)ntohs(cmd->detail);

  if (rate < 1 || rate > 10)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC SETRATE: snum = %d, rate = %d", snum, rate);
#endif

  Context.updsec = rate;

  return;
}

void procTow(cpCommand_t *cmd)
{
  int snum = Context.snum;
  int other;
  char cbuf[BUFFER_SIZE];

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_TOW)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC TOW: snum = %d", snum);
#endif

  other = (int)ntohs(cmd->detail);

  if ( Ships[snum].towedby != 0 )
    {
      c_strcpy( "But we are being towed by ", cbuf );
      appship( Ships[snum].towing, cbuf );
      appchr( '!', cbuf );
      clbStoreMsgf( MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK );
      return;
    }
  if ( Ships[snum].towing != 0 )
    {
      c_strcpy( "But we're already towing ", cbuf );
      appship( Ships[snum].towing, cbuf );
      appchr( '.', cbuf );
      clbStoreMsgf( MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK );
      return;
    }
  cbuf[0] = 0;
  PVLOCK(&ConqInfo->lockword);
  if ( other < 1 || other > MAXSHIPS )
    c_strcpy( "No such ship.", cbuf );
  else if ( Ships[other].status != SS_LIVE )
    c_strcpy( "Not found.", cbuf );
  else if ( other == snum )
    c_strcpy( "We can't tow ourselves!", cbuf );
  else if ( dist( Ships[snum].x, Ships[snum].y, Ships[other].x, Ships[other].y ) > TRACTOR_DIST )
    c_strcpy( "That ship is out of tractor range.", cbuf );
  else if ( Ships[other].warp < 0.0 )
    c_strcpy( "You can't tow a ship out of orbit.", cbuf );
  else if ( sqrt( pow(( (real) (Ships[snum].dx - Ships[other].dx) ), (real) 2.0) +
		  pow( (real) ( Ships[snum].dy - Ships[other].dy ), (real) 2.0 ) ) / 
	    ( MM_PER_SEC_PER_WARP * ITER_SECONDS ) > MAX_TRACTOR_WARP ) 
    sprintf( cbuf, "That ships relative velocity is higher than %2.1f.",
	     MAX_TRACTOR_WARP );
  else if ( Ships[other].towing != 0 || Ships[other].towedby != 0 )
    c_strcpy(
	     "There seems to be some interference with the tractor beams...",
	     cbuf );
  else
    {
      Ships[other].towedby = snum;
      Ships[snum].towing = other;
      c_strcpy("Tractor beams engaged.", cbuf);
    }
  PVUNLOCK(&ConqInfo->lockword);

  clbStoreMsgf( MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK );
  
  return;

}

void procUnTow(cpCommand_t *cmd)
{
  int snum = Context.snum;
  char cbuf[BUFFER_SIZE];
  int entertime, now;
  int warsome; 

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_UNTOW)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC UNTOW: snum = %d", snum);
#endif

  if ( Ships[snum].towedby != 0 )
    {
      /* If we're at war with him or he's at war with us, make it */
      /*  hard to break free. */
      warsome = ( satwar( snum, Ships[snum].towedby) );
      if ( warsome )
	{
	  grand( &entertime );
	  while ( dgrand( entertime, &now ) < BREAKAWAY_GRAND )
	    {
	      if ( ! clbStillAlive( Context.snum ) )
		return;
	      
	      c_sleep( ITER_SECONDS );
	      
	    }
	}
      if ( warsome && ( rnd() > BREAKAWAY_PROB ) )
	clbStoreMsgf(MSG_COMP, snum, "Attempt to break free failed.",
                 MSG_FLAGS_FEEDBACK);
      else
	{
	  c_strcpy( "Breaking free from ship ", cbuf );
	  appship( Ships[snum].towedby, cbuf );
	  PVLOCK(&ConqInfo->lockword);
	  if ( Ships[snum].towedby != 0 )
	    {
	      /* Coast to a stop. */
	      Ships[snum].head = Ships[Ships[snum].towedby].head;
	      Ships[snum].warp = Ships[Ships[snum].towedby].warp;
	      
	      /* Release the tow. */
	      if ( Ships[Ships[snum].towedby].towing != 0 )
		Ships[Ships[snum].towedby].towing = 0;
	      Ships[snum].towedby = 0;
	    }
	  PVUNLOCK(&ConqInfo->lockword);
	  appchr( '.', cbuf );
	  clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
	}
    }
  else if ( Ships[snum].towing != 0 )
    {
      c_strcpy( "Tow released from ship ", cbuf );
      appship( Ships[snum].towing, cbuf );
      PVLOCK(&ConqInfo->lockword);
      if ( Ships[snum].towing != 0 )
	{
	  /* Set other ship coasting. */
	  Ships[Ships[snum].towing].head = Ships[snum].head;

				
          if (!SysConf.AllowSlingShot)
            {               /* only set warp if valid JET - 9/15/97 */
              if (Ships[snum].warp >= 0.0)
                Ships[Ships[snum].towing].warp = Ships[snum].warp;
              else
                Ships[Ships[snum].towing].warp = 2;
            }
          else
            Ships[Ships[snum].towing].warp = Ships[snum].warp;
	  
	  /* Release the tow. */
	  if ( Ships[Ships[snum].towing].towedby != 0 )
	    Ships[Ships[snum].towing].towedby = 0;
	  Ships[snum].towing = 0;
	}
      PVUNLOCK(&ConqInfo->lockword);
      appchr( '.', cbuf );
      clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
    }
  else
    clbStoreMsgf(MSG_COMP, snum, "No tractor beam activity detected.",
            MSG_FLAGS_FEEDBACK);
  
  return;
  
}

void procBomb(cpCommand_t *cmd)
{
  int snum = Context.snum;
  int bomb;
  int pnum, now, entertime, total, ototal, oparmies;
  real x, killprob;
  int oldsshup;
  char buf[MSGMAXLINE];
  char cbuf[BUFFER_SIZE];
  string lastfew="The last few armies are eluding us.";


  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_BOMB)
    return;

  bomb = (int)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
  clog("PROC BOMB: snum = %d, start bombing = %d", snum, bomb);
#endif

  if (!bomb)			/* the bombing has stopped. yaay. */
    return;

  SFCLR(snum, SHIP_F_REPAIR);
  
  /* Check for allowability. */
  if ( Ships[snum].warp >= 0.0 )
    {
      clbStoreMsgf(MSG_COMP, snum, "We must be orbiting a planet to bombard it.",
	       MSG_FLAGS_FEEDBACK );
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  pnum = -Ships[snum].lock;
  if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON ||
      Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
    {
      clbStoreMsgf(MSG_COMP, snum, "There is no one there to bombard.",
	       MSG_FLAGS_FEEDBACK);
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  if ( Planets[pnum].team == Ships[snum].team )
    {
      clbStoreMsgf(MSG_COMP, snum, "We can't bomb our own armies!",
	      MSG_FLAGS_FEEDBACK);
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  if ( Planets[pnum].team != TEAM_SELFRULED && Planets[pnum].team != TEAM_GOD )
    if ( ! Ships[snum].war[Planets[pnum].team] )
      {
	clbStoreMsgf(MSG_COMP, snum, "But we are not at war with this planet!",
		 MSG_FLAGS_FEEDBACK);
	sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
	return;
      }
  
  /* Handle war logic. */
  Ships[snum].srpwar[pnum] = TRUE;
  if ( Planets[pnum].team >= 0 && Planets[pnum].team < NUMPLAYERTEAMS )
    {
      /* For a team planet make the war sticky and send an intruder alert. */
      Ships[snum].rwar[Planets[pnum].team] = TRUE;
      clbIntrude( snum, pnum );
    }
  /* Planets owned by GOD have a special defense system. */
  if ( Planets[pnum].team == TEAM_GOD )
    {
      sprintf( cbuf, "That was a bad idea, %s...", Ships[snum].alias );
      clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
      clbDamage( snum,  rnduni( 50.0, 100.0 ), KB_LIGHTNING );
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  
  /* Lower shields. */
  oldsshup = SSHUP(snum);
  SFCLR(snum, SHIP_F_SHUP);
  
  /* Bombard. */
  total = 0;
  ototal = -1;			/* force an update the first time */
  oparmies = -1;
  grand( &entertime );		/* get start time */
  while(TRUE)
    {
      if ( ! clbStillAlive( Context.snum ) )
	return;
      if ( isPacketWaiting(sInfo.sock) )
	break;
      
      /* See if it's time to bomb yet. */
      while ((int) fabs ((real)dgrand( (int)entertime, (int *)&now )) >= BOMBARD_GRAND )
	{
	  if ( Ships[snum].wfuse > 0 )
	    {
	      clbStoreMsgf(MSG_COMP, snum, "Weapons are currently overloaded.",
		       MSG_FLAGS_FEEDBACK);
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, 
		      NULL);
	      goto cbrk22; /* break 2;*/
	    }
	  x = BOMBARD_FUEL * (real)(BOMBARD_GRAND / 1000.0);
	  if ( ! clbUseFuel( snum, x, TRUE, TRUE ) )
	    {
	      clbStoreMsgf(MSG_COMP, snum, "Not enough fuel to bombard.",
		       MSG_FLAGS_FEEDBACK);
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, 
		      NULL);
	      goto cbrk22; /* break 2;*/
	    }
	  /*  entertime = mod( entertime + BOMBARD_GRAND, 24*60*60*1000 );*/
	  grand(&entertime);
	  killprob = (real)((BOMBARD_PROB *
			     ((real) weaeff( snum ) *
			      (real)((real)Planets[pnum].armies/100.0))) + 0.5);

	  if ( rnd() < killprob )
	    {
	      PVLOCK(&ConqInfo->lockword);
	      if ( Planets[pnum].armies <= MIN_BOMB_ARMIES )
		{
		  /* No more armies left to bomb. */
		  PVUNLOCK(&ConqInfo->lockword);
		  clbStoreMsgf(MSG_COMP, snum, lastfew, MSG_FLAGS_FEEDBACK);
		  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, 
			  NULL);
		  goto cbrk22; /* break 2;*/
		}
	      Planets[pnum].armies = Planets[pnum].armies - 1;
	      
	      Ships[snum].kills = Ships[snum].kills + BOMBARD_KILLS;
	      Users[Ships[snum].unum].stats[USTAT_ARMBOMB] += 1;
	      Teams[Ships[snum].team].stats[TSTAT_ARMBOMB] += 1;
	      PVUNLOCK(&ConqInfo->lockword);
	      total = total + 1;
	    }
	}
      
      if ( Planets[pnum].armies <= MIN_BOMB_ARMIES )
	{
	  /* No more armies left to bomb. */
	  clbStoreMsgf(MSG_COMP, snum, lastfew, MSG_FLAGS_FEEDBACK);
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
	  break;
	}
      
      if ( Planets[pnum].armies != oparmies || ototal != total )
	{
	  /* Either our bomb run total or the population changed. */
	  oparmies = Planets[pnum].armies;
	  if ( total == 1 )
	    c_strcpy( "y", buf );
	  else
	    c_strcpy( "ies", buf );
	  sprintf( cbuf, "Bombing %s, %d arm%s killed, %d left.",
		 Planets[pnum].name, total, buf, oparmies );
	  clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
	  
	  ototal = total;
	}
      
      c_sleep( ITER_SECONDS );
      
    }
 cbrk22:
  ;
  
  /* Restore shields. */
  if (oldsshup)
    SFSET(snum, SHIP_F_SHUP);

  return;
  
}


void procBeam(cpCommand_t *cmd)
{
  int snum = Context.snum;
  int beam, beamup;
  int pnum, total, num, upmax, downmax, capacity, beamax, i;
  int ototal, entertime, now;
  int oldsshup, dirup, zeroed, conqed;
  char cbuf[BUFFER_SIZE];
  real rkills;
  string lastfew="Fleet orders prohibit removing the last three armies.";

  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_BEAM)
    return;

  /* detail is (armies & 0x00ff), 0x8000 set if beaming down */

  beam = (int)(ntohs(cmd->detail) & 0x00ff);

  if (beam == 0)
    return;			/* stop beaming */

  if (ntohs(cmd->detail) & 0x8000)
    beamup = FALSE;		/* beaming down */
  else
    beamup = TRUE;		/* beaming up */

#if defined(DEBUG_SERVERPROC)
  clog("PROC BEAM: snum = %d, detail = 0x%04x, beamup = %d, beam = %d", 
       snum, cmd->detail, beamup, beam);
#endif

  SFCLR(snum, SHIP_F_REPAIR);
  
  /* Check for allowability. */
  if ( Ships[snum].warp >= 0.0 )
    {
      clbStoreMsgf(MSG_COMP, snum, 
	       "We must be orbiting a planet to use the transporter.",
	       MSG_FLAGS_FEEDBACK);
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  pnum = -Ships[snum].lock;
  if ( Ships[snum].armies > 0 )
    {
      if ( Planets[pnum].type == PLANET_SUN )
	{
	  clbStoreMsgf(MSG_COMP, snum,
		   "Idiot!  Our armies will fry down there!", 
		   MSG_FLAGS_FEEDBACK );
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
	  return;
	}
      else if ( Planets[pnum].type == PLANET_MOON )
	{
	  clbStoreMsgf(MSG_COMP, snum, 
		   "Fool!  Our armies will suffocate down there!",
		   MSG_FLAGS_FEEDBACK );
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
	  return;
	}
      else if ( Planets[pnum].team == TEAM_GOD )
	{
	  clbStoreMsgf(MSG_COMP, snum,
		   "GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
		   MSG_FLAGS_FEEDBACK );
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
	  return;
	}
    }
  
  i = Planets[pnum].uninhabtime;
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minute",
	     i );
      if ( i != 1 )
	appchr( 's', cbuf );
      appchr( '.', cbuf );
      clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK );
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  
  if ( Planets[pnum].team != Ships[snum].team &&
      Planets[pnum].team != TEAM_SELFRULED &&
      Planets[pnum].team != TEAM_NOTEAM )
    if ( ! Ships[snum].war[Planets[pnum].team] && Planets[pnum].armies != 0) /* can take empty planets */
      {
	clbStoreMsgf(MSG_COMP, snum, "But we are not at war with this planet!", 
		 MSG_FLAGS_FEEDBACK );
	sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
	return;
      }
  
  if ( Ships[snum].armies == 0 &&
      Planets[pnum].team == Ships[snum].team && Planets[pnum].armies <= MIN_BEAM_ARMIES )
    {
      clbStoreMsgf(MSG_COMP, snum, lastfew, MSG_FLAGS_FEEDBACK );
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  
  rkills = Ships[snum].kills;

  if ( rkills < (real)1.0 )
    {
      clbStoreMsgf(MSG_COMP, snum, 
	       "Fleet orders prohibit beaming armies until you have a kill.",
	       MSG_FLAGS_FEEDBACK );
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  
  /* Figure out what can be beamed. */
  downmax = Ships[snum].armies;
  if ( clbSPWar(snum,pnum) ||
      Planets[pnum].team == TEAM_SELFRULED ||
      Planets[pnum].team == TEAM_NOTEAM ||
      Planets[pnum].team == TEAM_GOD ||
      Planets[pnum].armies == 0 )
    {
      upmax = 0;
    }
  else
    {
      capacity = min( ifix( rkills ) * 2, ShipTypes[Ships[snum].shiptype].armylim );
      upmax = min( Planets[pnum].armies - MIN_BEAM_ARMIES, 
		   capacity - Ships[snum].armies );
    }

  /* If there are armies to beam but we're selfwar... */
  if ( upmax > 0 && selfwar(snum) && Ships[snum].team == Planets[pnum].team )
    {
      if ( downmax <= 0 )
	{
	  c_strcpy( "The arm", cbuf );
	  if ( upmax == 1 )
	    appstr( "y is", cbuf );
	  else
	    appstr( "ies are", cbuf );
	  appstr( " reluctant to beam aboard a pirate vessel.", cbuf );
	  clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK );
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
	  return;
	}
      upmax = 0;
    }
  
  /* Figure out which direction to beam. */
  if ( upmax <= 0 && downmax <= 0 )
    {
      clbStoreMsgf(MSG_COMP, snum, "There is no one to beam.", MSG_FLAGS_FEEDBACK );
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, NULL);
      return;
    }
  if ( upmax <= 0 )
    dirup = FALSE;
  else if ( downmax <= 0 )
    dirup = TRUE;
  else
    dirup = beamup;
  
  if ( dirup )
    beamax = upmax;
  else
    beamax = downmax;

  if (beam > beamax)
    beam = beamax;

  num = beam;

  /* Now we are ready! */
  if ( Planets[pnum].team >= NUMPLAYERTEAMS )
    {
      /* If the planet is not race owned, make it war with us. */
      Ships[snum].srpwar[pnum] = TRUE;
    }
  else if ( Planets[pnum].team != Ships[snum].team )
    {
      /* For a team planet make the war sticky and send an intruder alert. */
      Ships[snum].rwar[Planets[pnum].team] = TRUE;
      
      /* Chance to create a robot here. */
      clbIntrude( snum, pnum );
    }
  
  /* Lower shields. */
  oldsshup = SSHUP(snum);
  SFCLR(snum, SHIP_F_SHUP);
  
  /* Beam. */
  total = 0;
  ototal = -1;				/* force an update the first time */
  zeroed = FALSE;
  conqed = FALSE;
  
  grand( &entertime );
  while(TRUE)
    {
      if ( ! clbStillAlive( Context.snum ) )
	return;
      if ( isPacketWaiting(sInfo.sock) )
	break;
      
      /* See if it's time to beam again. */
      while ( dgrand( entertime, &now ) >= BEAM_GRAND )
	{
	  grand(&entertime);
	  PVLOCK(&ConqInfo->lockword);
	  if ( dirup )
	    {
	      /* Beam up. */
	      if ( Planets[pnum].armies <= MIN_BEAM_ARMIES )
		{
		  PVUNLOCK(&ConqInfo->lockword);
		  clbStoreMsgf(MSG_COMP, snum, lastfew, MSG_FLAGS_FEEDBACK );
		  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED, 
			  NULL);
		  break;
		}
	      Ships[snum].armies = Ships[snum].armies + 1;
	      Planets[pnum].armies = Planets[pnum].armies - 1;
	    }
	  else
	    {
	      /* Beam down. */
	      Ships[snum].armies = Ships[snum].armies - 1;
	      if ( Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
		{
		  clbTakePlanet( pnum, snum );
		  conqed = TRUE;
		}
	      else if ( Planets[pnum].team != Ships[snum].team )
		{
		  Planets[pnum].armies = Planets[pnum].armies - 1;
		  if ( Planets[pnum].armies == 0 )
		    {
		      clbZeroPlanet( pnum, snum );
		      zeroed = TRUE;
		    }
		}
	      else
		Planets[pnum].armies = Planets[pnum].armies + 1;
	    }
	  PVUNLOCK(&ConqInfo->lockword);
	  total = total + 1;
	  
	  if ( total >= num )
	    {
	      /* Done. */
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_DONE, 
		      NULL);
	      goto cbrk21; 
	    }
	}
      
      if ( ototal != total )
	{
	  c_strcpy( "Beaming ", cbuf );
	  if ( dirup )
	    appstr( "up from ", cbuf );
	  else
	    appstr( "down to ", cbuf );
	  appstr( Planets[pnum].name, cbuf );
	  appstr( ", ", cbuf );
	  if ( total == 0 )
	    appstr( "no", cbuf );
	  else
	    appint( total, cbuf );
	  appstr( " arm", cbuf );
	  if ( total == 1 )
	    {
	      appchr( 'y', cbuf );
	    }
	  else
	    {
	      appstr( "ies", cbuf );
	    }
	  appstr( " transported, ", cbuf );
	  appint( num - total, cbuf );
	  appstr( " to go.", cbuf );
	  clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
	  ototal = total;
	}
      
      if ( dirup && Planets[pnum].armies <= MIN_BEAM_ARMIES )
	{
	  clbStoreMsgf(MSG_COMP, snum, lastfew, MSG_FLAGS_FEEDBACK);
	  break;
	}
      
      c_sleep( ITER_SECONDS );
      
    }
 cbrk21:
  
  /* Restore shields. */
  if (oldsshup)
    SFSET(snum, SHIP_F_SHUP);
  
  if ( conqed )
    {
      sprintf( cbuf, "You have conquered %s.", Planets[pnum].name );
      clbStoreMsgf(MSG_COMP, snum, cbuf, MSG_FLAGS_FEEDBACK);
    }
  else if ( zeroed )
    clbStoreMsgf(MSG_COMP, snum, 
	     "Sensors show hostile forces eliminated from the planet.",
	     MSG_FLAGS_FEEDBACK);
  
  return;
  
}

void procDestruct(cpCommand_t *cmd)
{
  int snum = Context.snum;
  int entertime, now; 

  if (!validPkt(CP_COMMAND, cmd))
    return;
  
  if (cmd->cmd != CPCMD_DESTRUCT)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC DESTRUCT: snum = %d, detail = 0x%04x", 
       snum, cmd->detail);
#endif

  if (!ntohs(cmd->detail))
    return;			/* canceled a self destruct */

  /* time to breach the core */

  if ( SCLOAKED(snum) )
    {
      clbStoreMsgf(MSG_COMP, snum, 
	      "The cloaking device is using all available power.",
               MSG_FLAGS_FEEDBACK);
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED,
	      NULL);
      return;
    }

  /* Set up the destruct fuse. */
  Ships[Context.snum].sdfuse = SELFDESTRUCT_FUSE;
  
  gsecs( &entertime );
  

  Context.msgok = TRUE;			/* messages are ok in the beginning */
  while ( Ships[Context.snum].sdfuse > 0 )
    {
      Ships[Context.snum].sdfuse = SELFDESTRUCT_FUSE - dsecs ( entertime, &now );

      /* Display new messages until T-minus 3 seconds. */

      if ( Ships[Context.snum].sdfuse < 3 )
	Context.msgok = FALSE;

      if ( ! clbStillAlive( Context.snum ) )
	{
	  /* Died in the process. */
	  Ships[Context.snum].sdfuse = 0;
	  return;
	}

      if ( isPacketWaiting(sInfo.sock) )
	{
	  Ships[Context.snum].sdfuse = 0;
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED,
		  NULL);
	  return;
	}
      
      c_sleep( ITER_SECONDS );
      
    } /* end while */

  Context.msgok = FALSE;			/* turn off messages */
  
  if ( Doomsday->status == DS_LIVE )
    {
      if ( dist(Ships[Context.snum].x, Ships[Context.snum].y, 
		Doomsday->x, Doomsday->y) <= DOOMSDAY_KILL_DIST )
	{
	  Doomsday->status = DS_OFF;
	  clbStoreMsg( MSG_DOOM, MSG_ALL, "AIEEEEEEEE!" );
	  clbKillShip( Context.snum, KB_GOTDOOMSDAY );
	}
      else
	if (clbStillAlive(Context.snum))	/* if we're not dead yet... */
	  clbKillShip( Context.snum, KB_SELF );
    }
  else
    {
      if (clbStillAlive(Context.snum))	/* if we're not dead yet... */
	clbKillShip( Context.snum, KB_SELF );
    }
  
  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_DONE,
	  NULL);
  return;
  
}

void procAutoPilot(cpCommand_t *cmd)
{
  int snum = Context.snum;
  int laststat, now; 

  if (!validPkt(CP_COMMAND, cmd))
    return;
  
  if (cmd->cmd != CPCMD_AUTOPILOT)
    return;

#if defined(DEBUG_SERVERPROC)
  clog("PROC AUTOPILOT: snum = %d, detail = 0x%04x", 
       snum, ntohs(cmd->detail));
#endif

  if (!ntohs(cmd->detail))
    return;			/* cancelled autopilot */

  /* allowed? */
  if (!Users[Ships[snum].unum].ooptions[OOPT_AUTOPILOT])
    {
      clog("PROC AUTOPILOT: unum = %d, snum = %d: NOT ALLOWED", 
           Ships[snum].unum, snum);
      return;
    }

  clbStoreMsgf(MSG_COMP, snum, "Autopilot activated.", MSG_FLAGS_FEEDBACK);
  SFSET(snum, SHIP_F_ROBOT);

  gsecs( &laststat );			/* initialize stat timer */
  while ( clbStillAlive( Context.snum ) )
    {
      /* Make sure we still control our ship. */
      if ( Ships[snum].pid != Context.pid )
	break;
      
      /* See if it's time to update the statistics. */
      if ( dsecs( laststat, &now ) >= 15 )
	{
	  conqstats( Context.snum );
	  laststat = now;
	}

      if ( isPacketWaiting(sInfo.sock) ) 
        {
          sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_CANCELED,
                  NULL);
          break;
        }
      
      c_sleep(ITER_SECONDS);
    }

  SFCLR(snum, SHIP_F_ROBOT);
  Ships[snum].action = 0;
  
  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_DONE,
	  NULL);
  return;
  
}

void procReload(cpCommand_t *cmd)
{
  if (!validPkt(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_RELOAD)
    return;

  /* reset the packet history */
  spktInitPkt();

  return;
}


