#include "c_defs.h"

/************************************************************************
 *
 * server specific stuff
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "cb.h"
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
        sInfo.tryUDP = false;
        sInfo.doUDP = false;
        close(sInfo.usock);
        sInfo.usock = -1;
        pktSetSocketFds(PKT_SOCKFD_NOCHANGE, sInfo.usock);

        utLog("NET: too many UDP send errors to client, switching to TCP");
        clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_SHIP, Context.snum,
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
        return false;
    else
        return true;
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
            return false;

        if (Context.recmode == RECMODE_ON)
            recWriteEvent(suser);
    }

    return true;
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
        if ((sship = spktShip(snum, true)))
            recWriteEvent(sship);
    }

    if ((sship = spktShip(snum, false)))
        if (pktWrite(PKT_SENDTCP, sship) <= 0)
            return false;

    /* SP_SHIPSML */
    if (Context.recmode == RECMODE_ON)
    {
        if ((sshipsml = spktShipSml(snum, true)))
            recWriteEvent(sshipsml);
    }

    if ((sshipsml = spktShipSml(snum, false)))
        if (pktWrite(PKT_SENDTCP, sshipsml) <= 0)
            return false;

    /* SP_SHIPLOC */

    if (Context.recmode == RECMODE_ON)
    {
        if ((sshiploc = spktShipLoc(snum, true)))
            recWriteEvent(sshiploc);
    }

    if ((sshiploc = spktShipLoc(snum, false)))
    {
        if (pktWrite(PKT_SENDUDP, sshiploc) <= 0)
        {
            if (sInfo.doUDP)
                handleUDPErr();
            return false;
        }
    }
    return true;
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
        if ((splan = spktPlanet(pnum, true)))
            recWriteEvent(splan);
    }

    if ((splan = spktPlanet(pnum, false)))
        if (pktWrite(PKT_SENDTCP, splan) <= 0)
            return false;

    /* SP_PLANETSML */
    if (Context.recmode == RECMODE_ON)
    {
        if ((splansml = spktPlanetSml(pnum, true)))
            recWriteEvent(splansml);
    }

    if ((splansml = spktPlanetSml(pnum, false)))
        if (pktWrite(PKT_SENDTCP, splansml) <= 0)
            return false;

    /* SP_PLANETINFO */
    if (Context.recmode == RECMODE_ON)
    {
        if ((splaninfo = spktPlanetInfo(pnum, true)))
            recWriteEvent(splaninfo);
    }

    if ((splaninfo = spktPlanetInfo(pnum, false)))
        if (pktWrite(PKT_SENDTCP, splaninfo) <= 0)
            return false;

    /* we will do loc packets for recording purposes only.  loc2 is sent
       to clients, but not recorded (server-side), since the clients
       compute their own planetary movement based on them. */

    /* SP_PLANETLOC */
    if (Context.recmode == RECMODE_ON)
    {
        if ((splanloc = spktPlanetLoc(pnum, true, force)))
            recWriteEvent(splanloc);
    }

    /* SP_PLANETLOC2 */
    if ((splanloc2 = spktPlanetLoc2(pnum, false, force)))
        if (pktWrite(PKT_SENDTCP, splanloc2) <= 0)
            return false;

    return true;
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
    for ( i = 0; i < cbLimits.maxUsers(); i++)
        if ( ULIVE(i) )
            numusers++;

    /* count ships */
    for ( i = 0; i < cbLimits.maxShips(); i++ )
    {
        if ( cbShips[i].status == SS_LIVE )
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
    sStat.serverFlags = htonl(getServerFlags());

    sStat.servertime = (uint32_t)htonl(time(0));

    if (pktWrite(socktype, &sStat) <= 0)
    {
        utLog("sendServerStats: pktWrite failed\n");
        return false;
    }

    return true;
}

int sendTorp(int sock, uint8_t tsnum, uint8_t tnum)
{
    spTorp_t *storp;
    spTorpLoc_t *storploc;
    spTorpEvent_t *storpev;

    /* no point in sending torp data if we're not playing */
    if (sInfo.state != SVR_STATE_PLAY)
        return true;

    if (tsnum < 0 || tsnum >= cbLimits.maxShips())
        return false;

    if (tnum >= cbLimits.maxTorps())
        return false;

#if defined(DEBUG_SERVERSEND)
    utLog("sendTorp: %d %d", tsnum, tnum);
#endif


    /* SP_TORPEVENT */
    if (Context.recmode == RECMODE_ON)
    {
        if ((storpev = spktTorpEvent(tsnum, tnum, true)))
        {
            recWriteEvent(storpev);
        }
    }

    if ((storpev = spktTorpEvent(tsnum, tnum, false)))
    {
        if (pktWrite(PKT_SENDTCP, storpev) <= 0)
            return false;
    }

    /* SP_TORP */
    /* we only record these */
    if (Context.recmode == RECMODE_ON)
    {
        if ((storp = spktTorp(tsnum, tnum, true)))
            recWriteEvent(storp);
    }

    /* SP_TORPLOC */

    /* we only record these */
    if (Context.recmode == RECMODE_ON)
    {
        if ((storploc = spktTorpLoc(tsnum, tnum, true)))
            recWriteEvent(storploc);
    }

    return true;
}

/* send a Feedback (MSG_FLAGS_FEEDBACK) message to the client.
 * For MSG_FLAGS_FEEDBACK msgs, we do not need to deposit into
 * the CB ringbuffer and deal with the associated 'lock-hogging' for a
 * busy server.  We will send these immediately and return.
 */
void sendFeedback(const char *msg)
{
    char buf[MESSAGE_SIZE + 64];
    Msg_t themsg;

    memset((void *)&themsg, 0, sizeof(Msg_t));
    themsg.to = MSG_TO_SHIP;
    themsg.toDetail = Context.snum;
    themsg.from = MSG_FROM_COMP;
    themsg.fromDetail = 0;
    themsg.flags = MSG_FLAGS_FEEDBACK;
    utStrncpy(themsg.msgbuf, msg, MESSAGE_SIZE);

    sendMessage(&themsg);

    if (SysConf.LogMessages == true)
    {
        clbFmtMsg(themsg.from, themsg.fromDetail,
                  themsg.to, themsg.toDetail, buf);
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
        return true;

    if (!msg)
        return true;

#if defined(DEBUG_SERVERSEND)
    utLog("sendMessage: to = %d(%d), from = %d(%d), flags = 0x%02x, msg = '%s'",
          msg->to, msg->toDetail,
          msg->from, msg->fromDetail,
          msg->flags, msg->msgbuf);
#endif

    memset((void *)&smsg, 0, sizeof(spMessage_t));

    smsg.type = SP_MESSAGE;

    smsg.from = msg->from;
    smsg.fromDetail = htons(msg->fromDetail);
    smsg.to = msg->to;
    smsg.toDetail = htons(msg->toDetail);
    smsg.flags = msg->flags;

    utStrncpy((char *)smsg.msg, msg->msgbuf, MESSAGE_SIZE);

    /* don't record feedback or tersable msgs */
    if (Context.recmode == RECMODE_ON)
        if (!(smsg.flags & (MSG_FLAGS_FEEDBACK | MSG_FLAGS_TERSABLE)))
            recWriteEvent(&smsg);

    if (pktWrite(PKT_SENDTCP, &smsg) <= 0)
    {
        utLog("sendMessage: pktWrite failed\n");
        return false;
    }

    return true;
}

int sendTeam(int sock, uint8_t team, int force)
{
    spTeam_t *steam;

#if defined(DEBUG_SERVERSEND)
    utLog("sendTeam: team = %d, f = %d", team, force);
#endif

    if (Context.recmode == RECMODE_ON)
    {
        if ((steam = spktTeam(team, force, true)))
            recWriteEvent(steam);
    }

    if ((steam = spktTeam(team, force, false)))
        if (pktWrite(PKT_SENDTCP, steam) <= 0)
            return false;

    return true;
}

int sendcbConqInfo(int sock, int force)
{
    spcbConqInfo_t *spci;

#if defined(DEBUG_SERVERSEND)
    utLog("sendcbConqInfo: f = %d", force);
#endif

    if ((spci = spktcbConqInfo(force)))
        if (pktWrite(PKT_SENDTCP, spci) <= 0)
            return false;

    return true;
}

int sendHistory(int sock, int hnum)
{
    spHistory_t *shist;

    if (hnum < 0 || hnum >= cbLimits.maxHist())
        return false;

#if defined(DEBUG_SERVERSEND)
    utLog("sendHistory: hnum = %d", hnum);
#endif

    if ((shist = spktHistory(hnum)))
        if (pktWrite(PKT_SENDTCP, shist) <= 0)
            return false;

    return true;
}

int sendDoomsday(int sock)
{
    spDoomsday_t *dd;

#if defined(DEBUG_SERVERSEND)
    utLog("sendDoomsday");
#endif

    if (Context.recmode == RECMODE_ON)
    {
        if ((dd = spktDoomsday(true)))
            recWriteEvent(dd);
    }

    if ((dd = spktDoomsday(false)))
        if (pktWrite(PKT_SENDTCP, dd) <= 0)
            return false;

    return true;
}
