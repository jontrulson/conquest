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
#include "conqutil.h"
#include "conf.h"
#include "server.h"
#include "serverpkt.h"
#include "context.h"
#include "record.h"
#include "conqlb.h"

#define MAXUDPERRS    (15)
static int sudperrs = 0;        /* keep track of udp write errors */

/* called when a write to a UDP socket fails. If the error threshold
   is exceeded, disable UDP to client and let them know */
static void handleUDPErr(void)
{
  sudperrs++;

  if (sudperrs > MAXUDPERRS)
    {
      sInfo.tryUDP = FALSE;
      sInfo.doUDP = FALSE;
      close(sInfo.usock);
      sInfo.usock = -1;
      pktSetSocketFds(PKT_SOCKFD_NOCHANGE, sInfo.usock);

      utLog("NET: too many UDP send errors to client, switching to TCP");
      clbStoreMsg(MSG_COMP, Context.snum, 
              "SERVER: too many UDP send errors. Switching to TCP");
    }
  return;
}

int sendClientStat(int sock, Unsgn8 flags, Unsgn8 snum, Unsgn8 team, 
		   Unsgn16 unum, Unsgn8 esystem)
{
  spClientStat_t scstat;

  memset((void *)&scstat, 0, sizeof(spClientStat_t));

#if defined(DEBUG_SERVERCLNTSTAT)
  utLog("sendClientStat: snum = %d, unum = %d, flags = 0x%x, team = %d",
       snum, unum, flags, team);
#endif

  scstat.type = SP_CLIENTSTAT;
  scstat.flags = flags;
  scstat.snum = snum;
  scstat.team = team;
  scstat.unum = htons(unum);
  scstat.esystem = esystem;

  if (pktWrite(PKT_SENDTCP, &scstat) <= 0)
    return FALSE;
  else
    return TRUE;
}

int sendUser(int sock, Unsgn16 unum)
{
  spUser_t *suser;

#if defined(DEBUG_SERVERSEND)
  utLog("sendUser: unum = %d",
       unum);
#endif

  /* not really any priv bits, but in case we add some later */
  if ((suser = spktUser(unum)))
    {
      if (pktWrite(PKT_SENDTCP, suser) <= 0)
        return FALSE;

      if (Context.recmode == RECMODE_ON)
        recWriteEvent(suser);
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
  utLog("sendShip: snum = %d",
       snum);
#endif

  /* SP_SHIP */
  if (Context.recmode == RECMODE_ON)
    {
      if ((sship = spktShip(snum, TRUE)))
        recWriteEvent(sship);
    }

  if ((sship = spktShip(snum, FALSE)))
    if (pktWrite(PKT_SENDTCP, sship) <= 0)
      return FALSE;

  /* SP_SHIPSML */
  if (Context.recmode == RECMODE_ON)
    {
      if ((sshipsml = spktShipSml(snum, TRUE)))
        recWriteEvent(sshipsml);
    }

  if ((sshipsml = spktShipSml(snum, FALSE)))
    if (pktWrite(PKT_SENDTCP, sshipsml) <= 0)
      return FALSE;

  /* SP_SHIPLOC */

  if (Context.recmode == RECMODE_ON)
    {
      if ((sshiploc = spktShipLoc(snum, TRUE)))
        recWriteEvent(sshiploc);
    }
  
  if ((sshiploc = spktShipLoc(snum, FALSE)))
    {
      if (pktWrite(PKT_SENDUDP, sshiploc) <= 0)
        {
          if (sInfo.doUDP)
            handleUDPErr();
          return FALSE;
        }
    }
  return TRUE;
}

/* we have the potential to send 3 packets here. */
int sendPlanet(int sock, Unsgn8 pnum, int force)
{
  spPlanet_t *splan;
  spPlanetSml_t *splansml;
  spPlanetLoc_t *splanloc;
  spPlanetLoc2_t *splanloc2;
  spPlanetInfo_t *splaninfo;

#if defined(DEBUG_SERVERSEND)
  utLog("sendPlanet: pnum = %d",
       pnum);
#endif

  /* SP_PLANET */
  if (Context.recmode == RECMODE_ON)
    {
      if ((splan = spktPlanet(pnum, TRUE)))
        recWriteEvent(splan);
    }

  if ((splan = spktPlanet(pnum, FALSE)))
    if (pktWrite(PKT_SENDTCP, splan) <= 0)
      return FALSE;

  /* SP_PLANETSML */
  if (Context.recmode == RECMODE_ON)
    {
      if ((splansml = spktPlanetSml(pnum, TRUE)))
        recWriteEvent(splansml);
    }

  if ((splansml = spktPlanetSml(pnum, FALSE)))
    if (pktWrite(PKT_SENDTCP, splansml) <= 0)
      return FALSE;

  /* SP_PLANETINFO */
  if (Context.recmode == RECMODE_ON)
    {
      if ((splaninfo = spktPlanetInfo(pnum, TRUE)))
        recWriteEvent(splaninfo);
    }

  if ((splaninfo = spktPlanetInfo(pnum, FALSE)))
    if (pktWrite(PKT_SENDTCP, splaninfo) <= 0)
      return FALSE;

  /* we will do loc packets for recording purposes only.  loc2 is sent
     to clients, but not recorded (server-side), since the clients
     compute their own planetary movement based on them. */

  /* SP_PLANETLOC */
  if (Context.recmode == RECMODE_ON)
    {
      if ((splanloc = spktPlanetLoc(pnum, TRUE, force)))
        recWriteEvent(splanloc);
    }

  /* SP_PLANETLOC2 */
  if ((splanloc2 = spktPlanetLoc2(pnum, FALSE, force)))
    if (pktWrite(PKT_SENDTCP, splanloc2) <= 0)
      return FALSE;

  return TRUE;
}

/* send a status packet */
int sendServerStat(int socktype)
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
  utLog("sendServerStats: ENTER");
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
  
  if (!pktWrite(socktype, &sStat))
    {
      utLog("sendServerStats: pktWrite failed\n");
      return FALSE;
    }

  return TRUE;
}

int sendTorp(int sock, Unsgn8 tsnum, Unsgn8 tnum)
{
  spTorp_t *storp;
  spTorpLoc_t *storploc;
  spTorpEvent_t *storpev;

  /* no point in sending torp data if we're not playing */
  if (sInfo.state != SVR_STATE_PLAY)
    return TRUE;

  if (tsnum <= 0 || tsnum > MAXSHIPS)
    return FALSE;
  
  if (tnum >= MAXTORPS)
    return FALSE;

#if defined(DEBUG_SERVERSEND)
  utLog("sendTorp: %d %d", tsnum, tnum);
#endif


  /* SP_TORPEVENT */
  if (Context.recmode == RECMODE_ON)
    {
      if ((storpev = spktTorpEvent(tsnum, tnum, TRUE)))
        {
          recWriteEvent(storpev);
        }
    }

  if ((storpev = spktTorpEvent(tsnum, tnum, FALSE)))
    {
      if (pktWrite(PKT_SENDTCP, storpev) <= 0)
        return FALSE;
    }

  /* SP_TORP */
  /* we only record these */
  if (Context.recmode == RECMODE_ON)
    {
      if ((storp = spktTorp(tsnum, tnum, TRUE)))
        recWriteEvent(storp);
    }

  /* SP_TORPLOC */

  /* we only record these */
  if (Context.recmode == RECMODE_ON)
    {
      if ((storploc = spktTorpLoc(tsnum, tnum, TRUE)))
        recWriteEvent(storploc);
    }

  return TRUE;
}

/* send a Feedback (MSG_FLAGS_FEEDBACK) message to the client.
 * For MSG_FLAGS_FEEDBACK msgs, we do not need to deposit into                
 * the CB ringbuffer and deal with the associated 'lock-hogging' for a        
 * busy server.  We will send these immediately and return.                   
 */
void sendFeedback(char *msg)
{
  char buf[MESSAGE_SIZE + 64];
  Msg_t themsg;

  memset((void *)&themsg, 0, sizeof(Msg_t));
  themsg.msgto = Context.snum;
  themsg.msgfrom = MSG_COMP;
  themsg.flags = MSG_FLAGS_FEEDBACK;
  strncpy(themsg.msgbuf, msg, MESSAGE_SIZE - 1);

  sendMessage(&themsg);

  if (SysConf.LogMessages == TRUE)
    {
      clbFmtMsg(themsg.msgto, themsg.msgfrom, buf);
      utLog("MSG:FEEDBACK: %s: %s",
           buf, themsg.msgbuf);
    }

  return;
}

/* send a msg to the client */
int sendMessage(Msg_t *msg)
{
  spMessage_t smsg;

  if (sInfo.state != SVR_STATE_PLAY)
    return TRUE;

  if (!msg)
    return TRUE;

  memset((void *)&smsg, 0, sizeof(spMessage_t));

#if defined(DEBUG_SERVERSEND) 
  utLog("sendMessage: to = %d, from = %d, flags = 0x%02x, msg = '%s'",
       msg->msgto, msg->msgfrom, msg->flags, msg->msgbuf);
#endif

  smsg.type = SP_MESSAGE;
  
  smsg.from = (Sgn16)htons(msg->msgfrom);
  smsg.to = (Sgn16)htons(msg->msgto);
  smsg.flags = msg->flags;
  
  strncpy((char *)smsg.msg, msg->msgbuf, MESSAGE_SIZE - 1);

  /* don't record feeback or tersable msgs */
  if (Context.recmode == RECMODE_ON)
    if (!(smsg.flags & (MSG_FLAGS_FEEDBACK | MSG_FLAGS_TERSABLE)))
      recWriteEvent(&smsg);

  if (!pktWrite(PKT_SENDTCP, &smsg))
    {
      utLog("sendMessage: pktWrite failed\n");
      return FALSE;
    }

  return TRUE;
}

int sendTeam(int sock, Unsgn8 team, int force)
{
  spTeam_t *steam;

#if defined(DEBUG_SERVERSEND)
  utLog("sendTeam: team = %d, f = %d", team, force);
#endif

  if (Context.recmode == RECMODE_ON)
    {
      if ((steam = spktTeam(team, force, TRUE)))
        recWriteEvent(steam);
    }

  if ((steam = spktTeam(team, force, FALSE)))
    if (pktWrite(PKT_SENDTCP, steam) <= 0)
      return FALSE;

  return TRUE;
}

int sendConqInfo(int sock, int force)
{
  spConqInfo_t *spci;

#if defined(DEBUG_SERVERSEND)
  utLog("sendConqInfo: f = %d", force);
#endif

  if ((spci = spktConqInfo(force)))
    if (pktWrite(PKT_SENDTCP, spci) <= 0)
      return FALSE;

  return TRUE;
}

int sendHistory(int sock, int hnum)
{
  spHistory_t *shist;

  if (hnum < 0 || hnum > MAXHISTLOG)
    return FALSE;

#if defined(DEBUG_SERVERSEND)
  utLog("sendHistory: hnum = %d", hnum);
#endif

  if ((shist = spktHistory(hnum)))
    if (pktWrite(PKT_SENDTCP, shist) <= 0)
      return FALSE;

  return TRUE;
}

int sendDoomsday(int sock)
{
  spDoomsday_t *dd;

#if defined(DEBUG_SERVERSEND)
  utLog("sendDoomsday");
#endif

  if (Context.recmode == RECMODE_ON)
    {
      if ((dd = spktDoomsday(TRUE)))
        recWriteEvent(dd);
    }

  if ((dd = spktDoomsday(FALSE)))
    if (pktWrite(PKT_SENDTCP, dd) <= 0)
      return FALSE;

  return TRUE;
}
  

void procSetName(char *buf)
{
  cpSetName_t *cpsetn = (cpSetName_t *)buf;

  if (!pktIsValid(CP_SETNAME, cpsetn))
    return;

  cpsetn->alias[MAXUSERPNAME - 1] = 0;

  strncpy(Users[Context.unum].alias, (char *)cpsetn->alias, MAXUSERPNAME);

  if (Context.snum > 0 && Context.snum <= MAXSHIPS)
    strncpy(Ships[Context.snum].alias, (char *)cpsetn->alias, MAXUSERPNAME);

  return;
}

void procSetCourse(char *buf)
{
  cpSetCourse_t *csc = (cpSetCourse_t *)buf;
  int lock;
  real dir;

  if (!pktIsValid(CP_SETCOURSE, csc))
    return;

  lock = (int)csc->lock;
  dir = (real)((real)ntohs(csc->head) / 100.0);

#if defined(DEBUG_SERVERPROC)
  utLog("PROC SETCOURSE: lock = %d, head = %f",
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

  if (!pktIsValid(CP_COMMAND, swarp))
    return;

  if (swarp->cmd != CPCMD_SETWARP)
    return;

  warp = (real)ntohs(swarp->detail);

#if defined(DEBUG_SERVERPROC)
  utLog("PROC SETWARP: warp = %f", warp);
#endif

  if ( Ships[snum].dwarp == 0.0 && warp != 0.0 )
    {
      /* See if engines are working. */
      if ( Ships[snum].efuse > 0 )
	{
	  sendFeedback("Engines are currently overloaded.");
	  return;
	}
            /* No charge if already warp 0. */
      if ( clbUseFuel( snum, ENGINES_ON_FUEL, FALSE, TRUE ) == FALSE)
	{
	  sendFeedback("We don't have enough fuel.");
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
      sendFeedback(cbuf);
    }

  return;
}

void procSetShields(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int shup;

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_SETSHIELDS)
    return;

  shup = (((int)ntohs(cmd->detail)) ? TRUE : FALSE);

#if defined(DEBUG_SERVERPROC)
  utLog("PROC SETSHIELDS: sh = %d", shup);
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

  if (!pktIsValid(CP_COMMAND, cmd))
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
  utLog("PROC ALLOC: (w)alloc = %d", alloc);
#endif

  Ships[snum].weapalloc = alloc;
  Ships[snum].engalloc = (100 - alloc);

  return;
}

 
void procCloak(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  string nofuel="Not enough fuel to engage cloaking device.";

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_CLOAK)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC CLOAK");
#endif


  if ( SCLOAKED(snum) )
    {
      SFCLR(snum, SHIP_F_CLOAKED);
      sendFeedback("Cloaking device disengaged.");
      return;
    }
  if ( Ships[snum].efuse > 0 )
    {
      sendFeedback("Engines are currently overloaded.");
      return;
    }
  if ( Ships[snum].fuel < CLOAK_ON_FUEL )
    {
      sendFeedback(nofuel);
      return;
    }

  SFCLR(snum, SHIP_F_REPAIR);
  if ( ! clbUseFuel( snum, CLOAK_ON_FUEL, FALSE, TRUE ) )
    {
      sendFeedback(nofuel);
      return;
    }

  SFSET(snum, SHIP_F_CLOAKED);
  sendFeedback("Cloaking device engaged.");

  return;
  
}

void procDetSelf(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int j;

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_DETSELF)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC DETSELF");
#endif

  for ( j = 0; j < MAXTORPS; j++ )
    clbDetonate( snum, j );

  return;
}

void procDetEnemy(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_DETENEMY)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC DETENEMY");
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

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_DISTRESS)
    return;

  tofriendly = (int)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
  utLog("PROC DISTRESS: tofriendly = %d", tofriendly);
#endif

  sprintf( cbuf,
	   "sh=%d %c, dam=%d, fuel=%d, temp=",
	   round(Ships[snum].shields),
	   (SSHUP(snum)) ? 'U' : 'D',
	   round(Ships[snum].damage),
	   round(Ships[snum].fuel) );

  i = round(Ships[snum].wtemp);
  if ( i < 100 )
    utAppendInt( i, cbuf );
  else
    appstr( "**", cbuf );
  appchr( '/', cbuf );
  i = round(Ships[snum].etemp);
  if ( i < 100 )
    utAppendInt( i, cbuf );
  else
    appstr( "**", cbuf );
  i = Ships[snum].armies;

  if ( i > 0 )
    {
      appstr( ", arm=", cbuf );
      utAppendInt( i, cbuf );
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

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_FIREPHASER)
    return;

  dir = (real)ntohs(cmd->detail) / 100.0;

  if (dir < 0)
    dir = 0.0;

  if (dir > 359.9)
    dir = 359.9;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC FIREPHASER: dir = %f", dir);
#endif

  if ( SCLOAKED(snum) )
    {
      sendFeedback("The cloaking device is using all available power.");
      return;
    }

  if ( Ships[snum].wfuse > 0 )
    {
      sendFeedback("Weapons are currently overloaded.");
      return;
    }

  if ( Ships[snum].fuel < PHASER_FUEL )
    {
      sendFeedback("Not enough fuel to fire phasers.");
      return;
    }
  
  if ( !clbPhaser( snum, dir ) )
    sendFeedback(">PHASERS DRAINED<");

  return;
}

void procOrbit(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int pnum;

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_ORBIT)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC ORBIT");
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

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_REPAIR)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC REPAIR");
#endif

  if ( ! SCLOAKED(snum) )
    {
      SFSET(snum, SHIP_F_REPAIR);
      Ships[snum].dwarp = 0.0;
    }
  else
    sendFeedback("The cloaking device is using all available power.");

  return;
}

void procCoup(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int i, pnum, now, entertime;
  real failprob;
  string nhp="We must be orbiting our home planet to attempt a coup.";
  char cbuf[128];
  int rv = -1;

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_COUP)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC COUP");
#endif

  /* Check for allowability. */
  if ( oneplace( Ships[snum].kills ) < MIN_COUP_KILLS )
    {
      sendFeedback("Fleet orders require three kills before a coup can be attempted.");
      return;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( Planets[i].real && (Planets[i].team == Ships[snum].team) && 
         (Planets[i].armies > 0) )
      {
	 sendFeedback("We don't need to coup, we still have armies left!");
	return;
      }

  if ( Ships[snum].warp >= 0.0 )
    {
      sendFeedback(nhp);
      return;
    }

  pnum = -Ships[snum].lock;
  if ( pnum != Teams[Ships[snum].team].homeplanet )
    {
      sendFeedback(nhp);
      return;
    }
  if ( Planets[pnum].armies > MAX_COUP_ENEMY_ARMIES )
    {
      sendFeedback("The enemy is still too strong to attempt a coup.");
      return;
    }
  i = Planets[pnum].uninhabtime;
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
	     i );
      sendFeedback(cbuf);
      return;
    }
  
  /* Now our team can tell coup time for free. */
  Teams[Ships[snum].team].coupinfo = TRUE;
  
  i = Teams[Ships[snum].team].couptime;
  if ( i > 0 )
    {
      sprintf( cbuf, "Our forces need %d more minutes to organize.", i );
      sendFeedback(cbuf);
      return;
    }
  
  /* Now wait it out... */
  sendFeedback("Attempting coup...");
  utGrand( &entertime );
  while ( utDeltaGrand( entertime, &now ) < COUP_GRAND )
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
      sendFeedback("Sensors show hostile forces eliminated from the planet.");
      return;
    }
  
  failprob = Planets[pnum].armies / MAX_COUP_ENEMY_ARMIES * 0.5 + 0.5;

  if ( rnd() < failprob )
    {
      /* Failed; setup new reorganization time. */
      Teams[Ships[snum].team].couptime = rndint( 5, 10 );
      PVUNLOCK(&ConqInfo->lockword);
      sendFeedback("Coup unsuccessful.");
      return;
    }
  
  rv = clbTakePlanet( pnum, snum );

  /* Make the planet not scanned. */
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    Planets[pnum].scanned[i] = FALSE;

  /* ...except by us */
  Planets[pnum].scanned[Ships[snum].team] = TRUE;
  
  Planets[pnum].armies = rndint( 10, 20 );	/* create token coup force */
  Users[Ships[snum].unum].stats[USTAT_COUPS] += 1;
  Teams[Ships[snum].team].stats[TSTAT_COUPS] += 1;
  PVUNLOCK(&ConqInfo->lockword);

  sendFeedback("Coup successful!");

  /* force a team update for this ship */
  sendTeam(sInfo.sock, Ships[snum].team, TRUE);
  
  return;
}
  
void procFireTorps(char *buf)
{
  int snum = Context.snum;		/* we always use our own ship */
  cpFireTorps_t *cftorp = (cpFireTorps_t *)buf;
  real dir;
  int num;

  if (!pktIsValid(CP_FIRETORPS, cftorp))
    return;

  dir = (real)((real)ntohs(cftorp->dir) / 100.0);
  num = (int)cftorp->num;

  if (dir < 0)
    dir = 0.0;
  
  if (dir > 359.9)
    dir = 359.9;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC FIRETORPS: dir = %f, num = %d", dir, num);
#endif

  if ( SCLOAKED(snum) )
    {
      sendFeedback("The cloaking device is using all available power.");
      return;
    }

  if ( Ships[snum].wfuse > 0 )
    {
      sendFeedback("Weapons are currently overloaded.");
      return;
    }

  if ( Ships[snum].fuel < TORPEDO_FUEL )
    {
      sendFeedback("Not enough fuel to launch a torpedo.");
      return;
    }

  if ( ! clbLaunch( snum, dir, num, LAUNCH_NORMAL ) )
    {
      sendFeedback(">TUBES EMPTY<");
    }
  
  return;
  
}

void procMessage(char *buf)
{
  int snum = Context.snum;		/* we always use our own ship */
  cpMessage_t *cmsg = (cpMessage_t *)buf;
  int to;
 
  if (sInfo.state != SVR_STATE_PLAY)
    return;
   
  if (!pktIsValid(CP_MESSAGE, cmsg))
    return;

  to = (Sgn16)ntohs(cmsg->to);
  cmsg->msg[MESSAGE_SIZE - 1] = 0;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC MESSAGE: to %d", to);
#endif

  clbStoreMsg(snum, to, (char *)cmsg->msg);
  checkOperExec(snum, to, (char *)cmsg->msg);

  return;
}

void procChangePassword(char *buf)
{
  char salt[3];
  int unum = Context.unum;
  cpAuthenticate_t *cauth = (cpAuthenticate_t *)buf;

  if (!pktIsValid(CP_AUTHENTICATE, buf))
    return;

  cauth->pw[MAXUSERNAME - 1] = 0;

  /* we are just interested in the pw */
 
#if defined(DEBUG_SERVERPROC)
  utLog("PROC ChangePassword");
#endif
 
  salt[0] = (Users[unum].username[0] != EOS) ? Users[unum].username[0] :
    'J';
  salt[1] = (Users[unum].username[1] != EOS) ? Users[unum].username[1] :
    'T';
  salt[2] = EOS;

  strncpy(Users[unum].pw, (char *)crypt((char *)cauth->pw, salt), 
          MAXUSERNAME - 2);
  Users[unum].pw[MAXUSERNAME - 1] = EOS;

  return;
}


void procSetWar(cpCommand_t *cmd)
{
  int snum = Context.snum;		/* we always use our own ship */
  int unum = Context.unum;
  int dowait = FALSE, entertime, now, i;
  Unsgn8 war;

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_SETWAR)
    return;

  war = (Unsgn8)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
  utLog("PROC SETWAR war = 0x%02x", war);
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
      utGrand( &entertime );
      while ( utDeltaGrand( entertime, &now ) < REARM_GRAND )
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

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_REFIT)
    return;

  stype = (int)ntohs(cmd->detail);

  if (stype < 0 || stype >= MAXNUMSHIPTYPES)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC REFIT: snum - %d, stype = %d", snum, stype);
#endif
  
  /* Check for allowability. */
  if ( oneplace( Ships[snum].kills ) < MIN_REFIT_KILLS )
    {
      sendFeedback("You must have at least one kill to refit.");
      return;
    }

  pnum = -Ships[snum].lock;

  if (Planets[pnum].team != Ships[snum].team || Ships[snum].warp >= 0.0)
    {
      sendFeedback("We must be orbiting a team owned planet to refit.");
      return;
    }

  if (Ships[snum].armies != 0)
    {
      sendFeedback("You cannot refit while carrying armies");
      return;
    }
  
  /* now we wait for a bit. */
  utGrand( &entertime );
  while ( utDeltaGrand( entertime, &now ) < REFIT_GRAND )
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

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_SETRATE)
    return;

  rate = (int)ntohs(cmd->detail);

  if (rate < 1 || rate > 10)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC SETRATE: snum = %d, rate = %d", snum, rate);
#endif

  Context.updsec = rate;

  return;
}

void procTow(cpCommand_t *cmd)
{
  int snum = Context.snum;
  int other;
  char cbuf[BUFFER_SIZE];

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_TOW)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC TOW: snum = %d", snum);
#endif

  other = (int)ntohs(cmd->detail);

  if ( Ships[snum].towedby != 0 )
    {
      c_strcpy( "But we are being towed by ", cbuf );
      utAppendShip( Ships[snum].towedby, cbuf );
      appchr( '!', cbuf );
      sendFeedback(cbuf);
      return;
    }
  if ( Ships[snum].towing != 0 )
    {
      c_strcpy( "But we're already towing ", cbuf );
      utAppendShip( Ships[snum].towing, cbuf );
      appchr( '.', cbuf );
      sendFeedback(cbuf);
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

  sendFeedback(cbuf);
  
  return;

}

void procUnTow(cpCommand_t *cmd)
{
  int snum = Context.snum;
  char cbuf[BUFFER_SIZE];
  int entertime, now;
  int warsome; 

  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_UNTOW)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC UNTOW: snum = %d", snum);
#endif

  if ( Ships[snum].towedby != 0 )
    {
      /* If we're at war with him or he's at war with us, make it */
      /*  hard to break free. */
      warsome = ( satwar( snum, Ships[snum].towedby) );
      if ( warsome )
	{
	  utGrand( &entertime );
	  while ( utDeltaGrand( entertime, &now ) < BREAKAWAY_GRAND )
	    {
	      if ( ! clbStillAlive( Context.snum ) )
		return;
	      
	      c_sleep( ITER_SECONDS );
	      
	    }
	}
      if ( warsome && ( rnd() > BREAKAWAY_PROB ) )
	sendFeedback("Attempt to break free failed.");
      else
	{
	  c_strcpy( "Breaking free from ship ", cbuf );
	  utAppendShip( Ships[snum].towedby, cbuf );
	  PVLOCK(&ConqInfo->lockword);
	  if ( Ships[snum].towedby != 0 )
	    {
	      /* Coast to a stop. */
	      Ships[snum].head = Ships[Ships[snum].towedby].head;
              
              if (!SysConf.AllowSlingShot)
                {               /* only set warp if valid JET - 9/15/97 */
                  if (Ships[Ships[snum].towedby].warp >= 0.0)
                    Ships[snum].warp = Ships[Ships[snum].towedby].warp;
                  else
                    Ships[snum].warp = 2.0;
                }
              else
                Ships[snum].warp = Ships[Ships[snum].towedby].warp;
	      
	      /* Release the tow. */
	      if ( Ships[Ships[snum].towedby].towing != 0 )
		Ships[Ships[snum].towedby].towing = 0;
	      Ships[snum].towedby = 0;
	    }
	  PVUNLOCK(&ConqInfo->lockword);
	  appchr( '.', cbuf );
	  sendFeedback(cbuf);
	}
    }
  else if ( Ships[snum].towing != 0 )
    {
      c_strcpy( "Tow released from ship ", cbuf );
      utAppendShip( Ships[snum].towing, cbuf );
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
                Ships[Ships[snum].towing].warp = 2.0;
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
      sendFeedback(cbuf);
    }
  else
    sendFeedback("No tractor beam activity detected.");
  
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


  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_BOMB)
    return;

  bomb = (int)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
  utLog("PROC BOMB: snum = %d, start bombing = %d", snum, bomb);
#endif

  if (!bomb)			/* the bombing has stopped. yaay. */
    return;

  SFCLR(snum, SHIP_F_REPAIR);
  
  /* Check for allowability. */
  if ( Ships[snum].warp >= 0.0 )
    {
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback("We must be orbiting a planet to bombard it.");
      return;
    }
  pnum = -Ships[snum].lock;
  if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON ||
      Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
    {
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback("There is no one there to bombard.");
      return;
    }
  if ( Planets[pnum].team == Ships[snum].team )
    {
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback("We can't bomb our own armies!");
      return;
    }
  if ( Planets[pnum].team != TEAM_SELFRULED && Planets[pnum].team != TEAM_GOD )
    if ( ! Ships[snum].war[Planets[pnum].team] )
      {
	pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
	sendFeedback("But we are not at war with this planet!");
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
      clbDamage( snum,  rnduni( 50.0, 100.0 ), KB_LIGHTNING );
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback(cbuf);
      return;
    }
  
  /* Lower shields. */
  oldsshup = SSHUP(snum);
  SFCLR(snum, SHIP_F_SHUP);
  
  /* Bombard. */
  total = 0;
  ototal = -1;			/* force an update the first time */
  oparmies = -1;
  utGrand( &entertime );		/* get start time */
  SFSET(snum, SHIP_F_BOMBING);
  while(TRUE)
    {
      if ( ! clbStillAlive( Context.snum ) )
        {
          SFCLR(snum, SHIP_F_BOMBING);
          return;
        }
      if ( pktIsWaiting() )
        {
          SFCLR(snum, SHIP_F_BOMBING);
          break;
        }
      
      /* See if it's time to bomb yet. */
      while ((int) fabs ((real)utDeltaGrand( (int)entertime, (int *)&now )) >= BOMBARD_GRAND )
	{
	  if ( Ships[snum].wfuse > 0 )
	    {
	      pktSendAck(PSEV_INFO, PERR_CANCELED, 
		      NULL);
	      sendFeedback("Weapons are currently overloaded.");
	      goto cbrk22; /* break 2;*/
	    }
	  x = BOMBARD_FUEL * (real)(BOMBARD_GRAND / 1000.0);
	  if ( ! clbUseFuel( snum, x, TRUE, TRUE ) )
	    {
	      pktSendAck(PSEV_INFO, PERR_CANCELED, 
		      NULL);
	      sendFeedback("Not enough fuel to bombard.");
	      goto cbrk22; /* break 2;*/
	    }
	  /*  entertime = mod( entertime + BOMBARD_GRAND, 24*60*60*1000 );*/
	  utGrand(&entertime);
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
		  pktSendAck(PSEV_INFO, PERR_CANCELED, 
			  NULL);
		  sendFeedback(lastfew);
		  goto cbrk22; /* break 2;*/
		}
	      Planets[pnum].armies = Planets[pnum].armies - 1;
	      
	      Ships[snum].kills = Ships[snum].kills + BOMBARD_KILLS;
	      Users[Ships[snum].unum].stats[USTAT_ARMBOMB] += 1;
	      Teams[Ships[snum].team].stats[TSTAT_ARMBOMB] += 1;
	      PVUNLOCK(&ConqInfo->lockword);
	      total = total + 1;
	    }
	} /* while */
      
      if ( Planets[pnum].armies <= MIN_BOMB_ARMIES )
	{
	  /* No more armies left to bomb. */
	  pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
	  sendFeedback(lastfew);
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
	  sendFeedback(cbuf);
	  
	  ototal = total;
	}
      
      c_sleep( ITER_SECONDS );
      
    } /* while */
 cbrk22:
  ;
  
  /* no longer bombing */
  SFCLR(snum, SHIP_F_BOMBING);

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
  int rv = -1;

  if (!pktIsValid(CP_COMMAND, cmd))
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
  utLog("PROC BEAM: snum = %d, detail = 0x%04x, beamup = %d, beam = %d", 
       snum, cmd->detail, beamup, beam);
#endif

  SFCLR(snum, SHIP_F_REPAIR);
  
  /* Check for allowability. */
  if ( Ships[snum].warp >= 0.0 )
    {
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback("We must be orbiting a planet to use the transporter.");
      return;
    }
  pnum = -Ships[snum].lock;
  if ( Ships[snum].armies > 0 )
    {
      if ( Planets[pnum].type == PLANET_SUN )
	{
	  pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
	  sendFeedback("Idiot!  Our armies will fry down there!");
	  return;
	}
      else if ( Planets[pnum].type == PLANET_MOON )
	{
	  pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
	  sendFeedback("Fool!  Our armies will suffocate down there!");
	  return;
	}
      else if ( Planets[pnum].team == TEAM_GOD )
	{
	  pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
	  sendFeedback("GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.");
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
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback(cbuf);
      return;
    }
  
  if ( Planets[pnum].team != Ships[snum].team &&
      Planets[pnum].team != TEAM_SELFRULED &&
      Planets[pnum].team != TEAM_NOTEAM )
    if ( ! Ships[snum].war[Planets[pnum].team] && Planets[pnum].armies != 0) /* can take empty planets */
      {
	pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
	sendFeedback("But we are not at war with this planet!");
	return;
      }
  
  if ( Ships[snum].armies == 0 &&
      Planets[pnum].team == Ships[snum].team && Planets[pnum].armies <= MIN_BEAM_ARMIES )
    {
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback(lastfew);
      return;
    }
  
  rkills = Ships[snum].kills;

  if ( rkills < (real)1.0 )
    {
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback("Fleet orders prohibit beaming armies until you have a kill.");
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
	  pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
	  sendFeedback(cbuf);
	  return;
	}
      upmax = 0;
    }
  
  /* Figure out which direction to beam. */
  if ( upmax <= 0 && downmax <= 0 )
    {
      pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
      sendFeedback("There is no one to beam.");
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
  
  utGrand( &entertime );
  while(TRUE)
    {
      if ( ! clbStillAlive( Context.snum ) )
	return;
      if ( pktIsWaiting() )
	break;
      
      /* See if it's time to beam again. */
      while ( utDeltaGrand( entertime, &now ) >= BEAM_GRAND )
	{
	  utGrand(&entertime);
	  PVLOCK(&ConqInfo->lockword);
	  if ( dirup )
	    {
	      /* Beam up. */
	      if ( Planets[pnum].armies <= MIN_BEAM_ARMIES )
		{
		  PVUNLOCK(&ConqInfo->lockword);
		  pktSendAck(PSEV_INFO, PERR_CANCELED, 
			  NULL);
		  sendFeedback(lastfew);
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
		  rv = clbTakePlanet( pnum, snum );
		  conqed = TRUE;
		}
	      else if ( Planets[pnum].team != Ships[snum].team )
		{
		  Planets[pnum].armies = Planets[pnum].armies - 1;
		  if ( Planets[pnum].armies == 0 )
		    {
		      rv = clbZeroPlanet( pnum, snum );
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
	      pktSendAck(PSEV_INFO, PERR_DONE, 
		      NULL);
              sendFeedback("");
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
	    utAppendInt( total, cbuf );
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
	  utAppendInt( num - total, cbuf );
	  appstr( " to go.", cbuf );
	  sendFeedback(cbuf);
	  ototal = total;
	}
      
      if ( dirup && Planets[pnum].armies <= MIN_BEAM_ARMIES )
	{
	  sendFeedback(lastfew);
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
      sendFeedback(cbuf);
    }
  else if ( zeroed )
    sendFeedback("Sensors show hostile forces eliminated from the planet.");
  
  return;
  
}

void procDestruct(cpCommand_t *cmd)
{
  int snum = Context.snum;
  int entertime, now; 

  if (!pktIsValid(CP_COMMAND, cmd))
    return;
  
  if (cmd->cmd != CPCMD_DESTRUCT)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC DESTRUCT: snum = %d, detail = 0x%04x", 
       snum, cmd->detail);
#endif

  if (!ntohs(cmd->detail))
    return;			/* canceled a self destruct */

  /* time to breach the core */

  if ( SCLOAKED(snum) )
    {
      pktSendAck(PSEV_INFO, PERR_CANCELED,
	      NULL);
      sendFeedback("The cloaking device is using all available power.");
      return;
    }

  /* Set up the destruct fuse. */
  Ships[Context.snum].sdfuse = SELFDESTRUCT_FUSE;
  
  utGetSecs( &entertime );
  

  Context.msgok = TRUE;			/* messages are ok in the beginning */
  while ( Ships[Context.snum].sdfuse > 0 )
    {
      Ships[Context.snum].sdfuse = SELFDESTRUCT_FUSE - utDeltaSecs ( entertime, &now );

      /* Display new messages until T-minus 3 seconds. */

      if ( Ships[Context.snum].sdfuse < 3 )
	Context.msgok = FALSE;

      if ( ! clbStillAlive( Context.snum ) )
	{
	  /* Died in the process. */
	  Ships[Context.snum].sdfuse = 0;
	  return;
	}

      if ( pktIsWaiting() )
	{
	  Ships[Context.snum].sdfuse = 0;
	  pktSendAck(PSEV_INFO, PERR_CANCELED,
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
  
  pktSendAck(PSEV_INFO, PERR_DONE,
	  NULL);
  return;
  
}

void procAutoPilot(cpCommand_t *cmd)
{
  int snum = Context.snum;
  int laststat, now; 

  if (!pktIsValid(CP_COMMAND, cmd))
    return;
  
  if (cmd->cmd != CPCMD_AUTOPILOT)
    return;

#if defined(DEBUG_SERVERPROC)
  utLog("PROC AUTOPILOT: snum = %d, detail = 0x%04x", 
       snum, ntohs(cmd->detail));
#endif

  if (!ntohs(cmd->detail))
    return;			/* cancelled autopilot */

  /* allowed? */
  if (!Users[Ships[snum].unum].ooptions[OOPT_AUTOPILOT])
    {
      utLog("PROC AUTOPILOT: unum = %d, snum = %d: NOT ALLOWED", 
           Ships[snum].unum, snum);
      return;
    }

 sendFeedback("Autopilot activated.");
 SFSET(snum, SHIP_F_ROBOT);

  utGetSecs( &laststat );			/* initialize stat timer */
  while ( clbStillAlive( Context.snum ) )
    {
      /* Make sure we still control our ship. */
      if ( Ships[snum].pid != Context.pid )
	break;
      
      /* See if it's time to update the statistics. */
      if ( utDeltaSecs( laststat, &now ) >= 15 )
	{
	  conqstats( Context.snum );
	  laststat = now;
	}

      if ( pktIsWaiting() ) 
        {
          pktSendAck(PSEV_INFO, PERR_CANCELED,
                  NULL);
          break;
        }
      
      c_sleep(ITER_SECONDS);
    }

  SFCLR(snum, SHIP_F_ROBOT);
  Ships[snum].action = 0;
  
  pktSendAck(PSEV_INFO, PERR_DONE,
	  NULL);

  return;
}

void procReload(cpCommand_t *cmd)
{
  if (!pktIsValid(CP_COMMAND, cmd))
    return;

  if (cmd->cmd != CPCMD_RELOAD)
    return;

  /* reset the packet history */
  spktInitPkt();

  return;
}


