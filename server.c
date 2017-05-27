#include "c_defs.h"

/************************************************************************
 *
 * server specific stuff
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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

int sendClientStat(int sock, uint8_t flags, uint8_t snum, uint8_t team, 
		   uint16_t unum, uint8_t esystem)
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

int sendUser(int sock, uint16_t unum)
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
int sendShip(int sock, uint8_t snum)
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
int sendPlanet(int sock, uint8_t pnum, int force)
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

  sStat.flags = (uint32_t)htonl(sStat.flags);

  sStat.servertime = (uint32_t)htonl(getnow(NULL, 0));
  
  if (pktWrite(socktype, &sStat) <= 0)
    {
      utLog("sendServerStats: pktWrite failed\n");
      return FALSE;
    }

  return TRUE;
}

int sendTorp(int sock, uint8_t tsnum, uint8_t tnum)
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
  
  smsg.from = (int16_t)htons(msg->msgfrom);
  smsg.to = (int16_t)htons(msg->msgto);
  smsg.flags = msg->flags;
  
  strncpy((char *)smsg.msg, msg->msgbuf, MESSAGE_SIZE - 1);

  /* don't record feedback or tersable msgs */
  if (Context.recmode == RECMODE_ON)
    if (!(smsg.flags & (MSG_FLAGS_FEEDBACK | MSG_FLAGS_TERSABLE)))
      recWriteEvent(&smsg);

  if (pktWrite(PKT_SENDTCP, &smsg) <= 0)
    {
      utLog("sendMessage: pktWrite failed\n");
      return FALSE;
    }

  return TRUE;
}

int sendTeam(int sock, uint8_t team, int force)
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
  


