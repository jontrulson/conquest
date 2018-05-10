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

#include "conqdef.h"
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"
#include "conf.h"
#include "serverpkt.h"
#include "context.h"
#include "record.h"
#include "conqlb.h"
#include "udp.h"

#define NOEXTERN_SERVER
#include "server.h"
#undef NOEXTERN_SERVER

int sendClientStat(int sock, uint8_t flags, int16_t snum, uint8_t team,
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
    scstat.team = team;
    scstat.unum = htons(unum);
    scstat.snum = htons(snum);
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
            // If we get a UDP while writing, we'll disable UDP, and
            // tell the client.  TCP will be used from then on.
            if (sInfo.doUDP)
            {
                utLog("%s: UDP send error, turning off UDP", __FUNCTION__);
                pktSendAckUDP(PKT_SENDTCP, PKTUDP_STATE_SERVER_ERR, 0);
                sInfo.doUDP = false;
                if (sInfo.usock >= 0)
                    close(sInfo.usock);
                sInfo.usock = -1;
                pktSetSocketFds(PKT_SOCKFD_NOCHANGE, sInfo.usock);
            }
            else
            {
                return false;
            }
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

    /* SP_PLANETLOC */
    if (Context.recmode == RECMODE_ON)
    {
        if ((splanloc = spktPlanetLoc(pnum, true, force)))
            recWriteEvent(splanloc);
    }

    if ((splanloc = spktPlanetLoc(pnum, false, force)))
        if (pktWrite(PKT_SENDTCP, splanloc) <= 0)
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

    if (tsnum >= cbLimits.maxShips())
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
        std::string fbuf;
        clbFmtMsg(themsg.from, themsg.fromDetail,
                  themsg.to, themsg.toDetail, fbuf);
        utLog("MSG:FEEDBACK: %s: %s",
              fbuf.c_str(), themsg.msgbuf);
    }

    return;
}

/* send a msg to the client */
void sendMessage(Msg_t *msg)
{
    spMessage_t smsg;

    if (sInfo.state != SVR_STATE_PLAY)
        return;

    if (!msg)
        return;

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
        utLog("sendMessage: pktWrite failed\n");

    return;
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
    spConqInfo_t *spci;

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

// begin the process of negotiating a UDP connect to the client.
void serverStartUDP(pid_t pid, const cpCommand_t *ccmd)
{
    // Note the server is essentially paused while this function is
    // running...

    const unsigned int maxWait = 5000;   // 5 seconds

    uint16_t detail = ccmd->detail;

    // look at the command packet and see what to do
    if (!detail)
    {
        utLog("%s: NET: Stopping UDP per client request...",
              __FUNCTION__);
        // shutting down udp...
        sInfo.doUDP = false;
        if (sInfo.usock >= 0)
            close(sInfo.usock);
        sInfo.usock = -1;
        pktSetSocketFds(PKT_SOCKFD_NOCHANGE, sInfo.usock);
        // make sure the client knows...
        pktSendAckUDP(PKT_SENDTCP, PKTUDP_STATE_SERVER_ERR, 0);
        return;
    }

    // No need to start UDP if we are already doing it
    if (sInfo.doUDP)
    {
        utLog("%s: NET: UDP is already enabled...", __FUNCTION__);
        return;
    }

    utLog("%s: NET: Starting UDP negotiation...", __FUNCTION__);

    // otherwise, we will open a connection, send a message to the
    // server, and wait for a response from the client.
    if ((sInfo.usock = udpOpen(sInfo.listenPort,
                               &sInfo.clntaddr)) < 0)
    {
        utLog("%s: NET:  udpOpen() failed: %s", __FUNCTION__,
              strerror(errno));
        sInfo.usock = -1;
        pktSendAckUDP(PKT_SENDTCP, PKTUDP_STATE_SERVER_ERR, 0);
        return;
    }

    // now setup to wait for a packet and wait.
    struct timeval tv;
    fd_set readfds;
    int rv;

    tv.tv_sec = maxWait / 1000;
    tv.tv_usec = (maxWait % 1000) * 1000;
    FD_ZERO(&readfds);
    FD_SET(sInfo.usock, &readfds);

    // Let the client know we are ready to start...
    pktSendAckUDP(PKT_SENDTCP, PKTUDP_STATE_SERVER_READY, (int)pid);

    if ((rv = select(sInfo.usock+1, &readfds, NULL, NULL, &tv)) <= 0)
    {
        if (rv == 0)
            utLog("%s: NET: udp select timed out. No UDP", __FUNCTION__);
        else
            utLog("%s NET: udp select failed: %s", __FUNCTION__,
                  strerror(errno));

        if (sInfo.usock >= 0)
            close(sInfo.usock);
        sInfo.usock = -1;
        pktSendAckUDP(PKT_SENDTCP, PKTUDP_STATE_SERVER_ERR, 0);
        return;
    }
    else
    {
        if (FD_ISSET(sInfo.usock, &readfds))
        {            /* get the packet, almost done negotiating udp */
            char buf[PKT_MAXSIZE];
            rv = udpRecvFrom(sInfo.usock, buf, PKT_MAXSIZE, &sInfo.clntaddr);

            utLog("%s: NET: got %d UDP bytes from client port %d",
                  __FUNCTION__, rv,
                  (int)ntohs(sInfo.clntaddr.sin_port));

            // problem here is buf has a 4 digit seq number in front
            // of it. So make sure the packet is the right size, and
            // skip past the sequence number
            if (rv == (sizeof(cpAckUDP_t) + sizeof(uint32_t)))
                procAckUDP(buf + 4); // ignore sequence number
            else
            {
                // wrong size.  Hmmm.
                utLog("%s: NET: ERROR: got %d UDP bytes from client, "
                      "expected %lu",
                      __FUNCTION__, rv,
                      (sizeof(cpAckUDP_t) + sizeof(uint32_t)));
                pktSendAckUDP(PKT_SENDTCP, PKTUDP_STATE_SERVER_ERR, 0);
                if (sInfo.usock >= 0)
                    close(sInfo.usock);
                sInfo.usock = -1;
                return;
            }

            // the rest of the connection setup (conect(), payload
            // test, etc) should have been handled by procAckUDP()
        }
        else
        {
            // should never happen in theory, but...
            utLog("%s: NET: UDP FD_ISSET() returned 0.",
                  __FUNCTION__);
            pktSendAckUDP(PKT_SENDTCP, PKTUDP_STATE_SERVER_ERR, 0);
            if (sInfo.usock >= 0)
                close(sInfo.usock);
            sInfo.usock = -1;
        }
    }

    return;
}

bool sendShiptype(int index)
{
    spShiptype_t *stype;

#if defined(DEBUG_SERVERSEND)
    utLog("sendShipType: index = %d", index);
#endif

    if ((stype = spktShiptype(index)))
    {
        if (pktWrite(PKT_SENDTCP, stype) <= 0)
            return false;

        if (Context.recmode == RECMODE_ON)
            recWriteEvent(stype);

        return true;
    }

    return false;
}

