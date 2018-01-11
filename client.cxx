#include "c_defs.h"

/************************************************************************
 *
 * client specific packet send routines (to server)
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#include "conf.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"

#define NOEXTERN_CLIENT
#include "client.h"
#undef NOEXTERN_CLIENT

#include "conqlb.h"
#include "conqutil.h"
#include "clientlb.h"
#include "cb.h"
#include "context.h"
#include "msg.h"
#include "record.h"
#include "udp.h"
#include "color.h"
#include "ui.h"

/* send an auth packet and wait for a response (if not CHPWD).
   returns the error code from the ack, or error from write/read */
int sendAuth(int sock, uint8_t flag, const char *login, const char *pw)
{
    char buf[PKT_MAXSIZE];
    int rv;
    cpAuthenticate_t cauth;

    memset((void *)&cauth, 0, sizeof(cauth));

    cauth.type = CP_AUTHENTICATE;
    cauth.flag = flag;

    if (login)
        utStrncpy((char *)cauth.login, (char *)login, MAXUSERNAME);
    if (pw)
        utStrncpy((char *)cauth.pw, (char *)pw, MAXUSERNAME);

#if defined(DEBUG_CLIENTSEND)
    utLog("sendAuth: LOGIN = '%s' PW = '%s'\n",
          login, pw);
#endif

    if ((rv = pktWrite(PKT_SENDTCP, &cauth)) <= 0)
    {
        utLog("sendAuth: pktWrite returned %d\n", rv);
        return rv;
    }

    /* now look for an SP_ACK, if we're not changeing pw's */

    if (flag == CPAUTH_CHGPWD)
        return PERR_OK;

    rv = pktWaitForPacket(SP_ACK, buf, PKT_MAXSIZE,
                          60, NULL);

    if (rv <= 0)			/* error or timeout (0) */
    {
        utLog("sendAuth: pktWaitForPacket = %d", rv);
        return -1;
    }

    PKT_PROCSP(buf);

    /* now we should have our ACK... */

    return sAckMsg.code;
}

int sendSetCourse(int sock, courseLock_t lock, uint16_t lockDetail, real head)
{
    cpSetCourse_t csc;

    memset((void *)&csc, 0, sizeof(cpSetCourse_t));

    csc.type = CP_SETCOURSE;
    csc.lock = (uint8_t)lock;
    csc.lockDetail = htons(lockDetail);
    csc.head = htons((uint16_t)(head * 100.0));

    if (pktWrite(PKT_SENDTCP, &csc) <= 0)
        return false;
    else
        return true;
}

/* send a command to server to change name */
int sendSetName(char *name)
{
    cpSetName_t sname;

    if (!name)
        return false;

    memset((void *)&sname, 0, sizeof(cpSetName_t));
    sname.type = CP_SETNAME;
    utStrncpy((char *)sname.alias, name, MAXUSERNAME);

    if (pktWrite(PKT_SENDTCP, &sname) <= 0)
        return false;
    else
        return true;
}

int sendCommand(uint8_t cmd, uint16_t detail)
{
    cpCommand_t ccmd;
    int socktype = PKT_SENDTCP;
    memset((void *)&ccmd, 0, sizeof(cpCommand_t));
    ccmd.type = CP_COMMAND;
    ccmd.cmd = cmd;
    ccmd.detail = htons(detail);

    /* send these via UDP if possible */
    if (cmd == CPCMD_KEEPALIVE && cInfo.usock != -1)
        socktype = PKT_SENDUDP;

    if (pktWrite(socktype, &ccmd) <= 0)
        return false;
    else
        return true;
}

int sendFireTorps(int num, real dir)
{
    cpFireTorps_t ftorps;

    memset((void *)&ftorps, 0, sizeof(cpFireTorps_t));

    ftorps.type = CP_FIRETORPS;
    ftorps.num = (uint8_t)num;
    ftorps.dir = htons((uint16_t)(dir * 100.0));

    if (pktWrite(PKT_SENDTCP, &ftorps) <= 0)
        return false;
    else
        return true;

}

/* send a message */
int sendMessage(msgTo_t to, uint16_t toDetail, char *msg)
{
    cpMessage_t cmsg;

    if (!msg)
        return false;

    memset((void *)&cmsg, 0, sizeof(cpMessage_t));
    cmsg.type = CP_MESSAGE;
    cmsg.to = (uint8_t)to;
    cmsg.toDetail = htons(toDetail);
    utStrncpy((char *)cmsg.msg, msg, MESSAGE_SIZE);

    if (pktWrite(PKT_SENDTCP, &cmsg) <= 0)
        return false;
    else
        return true;
}

int clientHello(const char *clientname)
{
    cpHello_t chello;
    char buf[PKT_MAXSIZE];
    int pkttype;
    extern char *ConquestVersion, *ConquestDate;
    int rv;
    struct timeval tv;
    fd_set readfds;

    /* there should be a server hello waiting for us */
    if ((pkttype = pktRead(buf, PKT_MAXSIZE, 60)) < 0)
    {
        utLog("clientHello: read server hello failed\n");
        return false;
    }

    if (pkttype == 0)
    {
        utLog("clientHello: read server hello: timeout.\n");
        return false;
    }

    if (pkttype != SP_HELLO)
    {
        utLog("clientHello: read server hello: wrong packet type %d\n",
              pkttype);
        return false;
    }

    sHello = *(spHello_t *)buf;

    // check for access troubles first
    if (sHello.flags & SPHELLO_FLAGS_ACCESS_DENIED)
    {
        utLog("%s: Server denied access (SPHELLO_FLAGS_ACCESS_DENIED)",
              __FUNCTION__);
        return false;
    }

    /* now fix up byte ordering */
    sHello.protover = (uint16_t)ntohs(sHello.protover);
    sHello.cmnrev = (uint32_t)ntohl(sHello.cmnrev);

    // server cbLimits
    sHello.maxplanets = ntohl(sHello.maxplanets);
    sHello.maxships = ntohl(sHello.maxships);
    sHello.maxusers = ntohl(sHello.maxusers);
    sHello.maxhist = ntohl(sHello.maxhist);
    sHello.maxmsgs = ntohl(sHello.maxmsgs);
    sHello.maxtorps = ntohl(sHello.maxtorps);

    // enforce 0 termination in case of hostile server
    sHello.servername[CONF_SERVER_NAME_SZ - 1] = 0;
    sHello.serverver[CONF_SERVER_NAME_SZ - 1] = 0;
    sHello.motd[CONF_SERVER_MOTD_SZ - 1] = 0;

    utLog("SERVER ID: %s:%s:0x%04hx:%d:0x%02x",
          sHello.servername,
          sHello.serverver,
          sHello.protover,
          sHello.cmnrev,
          sHello.flags);
    utLog("           MOTD: %s", sHello.motd);
    utLog("SERVER LIMITS: planets: %u ships: %u users: %u hist: %u msgs: %u"
          " torps: %u\n",
          sHello.maxplanets,
          sHello.maxships,
          sHello.maxusers,
          sHello.maxhist,
          sHello.maxmsgs,
          sHello.maxtorps);

    // At this point, we need to unmap the current CB if mapped, setup
    // our cbLimits based on what the server told us, and then remap
    // the new universe.

    if (cbIsMapped())
        cbUnmapLocal();

    cbLimits.setMaxPlanets(sHello.maxplanets);
    cbLimits.setMaxShips(sHello.maxships);
    cbLimits.setMaxUsers(sHello.maxusers);
    cbLimits.setMaxHist(sHello.maxhist);
    cbLimits.setMaxMsgs(sHello.maxmsgs);
    cbLimits.setMaxTorps(sHello.maxtorps);

    // Now remap...
    cbMapLocal();

    // and we are ready to continue...

    if (cInfo.tryUDP)
    {
        if (connect(cInfo.usock, (const struct sockaddr *)&cInfo.servaddr,
                    sizeof(cInfo.servaddr)) < 0)
        {
            utLog("NET: clientHello: udp connect() failed: %s", strerror(errno));
            cInfo.tryUDP = false;
        }
        else
        {
            /* see if this will succeed in setting up a NAT tunnel
               to the server */
            utLog("NET: clientHello: send udp to server.");
            send(cInfo.usock, "Open Me", 7, 0);
        }
    }

    /* now send a client hello */
    chello.type = CP_HELLO;
    chello.updates = Context.updsec;
    chello.protover = htons(PROTOCOL_VERSION);
    chello.cmnrev = htonl(COMMONSTAMP);

    utStrncpy((char *)chello.clientname, clientname, CONF_SERVER_NAME_SZ);

    utStrncpy((char *)chello.clientver, ConquestVersion, CONF_SERVER_NAME_SZ);
    utStrncat((char *)chello.clientver, " ", CONF_SERVER_NAME_SZ);
    utStrncat((char *)chello.clientver, ConquestDate,
              CONF_SERVER_NAME_SZ);

    if (pktWrite(PKT_SENDTCP, &chello) <= 0)
    {
        utLog("clientHello: write client hello failed\n");
        return false;
    }

    utLog("clientHello: sent hello to server");


    if (cInfo.tryUDP)
    {
        /* see if we get an ack back from the server via udp */
        /* this is kind of weak and probably needs more work.  As it
           is, the client needs to receive a udp from the server
           within 5 seconds, or UDP will not be used.  If the inbound
           packet gets lost.... Oh well. */
        tv.tv_sec = 5;            /* 5 secs */
        tv.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(cInfo.usock, &readfds);
        if ((rv = select(cInfo.usock+1, &readfds, NULL, NULL, &tv)) <= 0)
        {
            utLog("CLIENT: hello: select udp failed: %s", strerror(errno));
            cInfo.tryUDP = false;
        }
        else
        {
            if (rv > 0 && FD_ISSET(cInfo.usock, &readfds))
            {
                if ((rv = udpRecvFrom(cInfo.usock, buf, PKT_MAXSIZE, &cInfo.servaddr)) >= 0 )
                {
                    utLog("NET: got (%d) UDP bytes from server, will ACK for server UDP", rv);
                    cInfo.doUDP = true;
                    pktSetSocketFds(PKT_SOCKFD_NOCHANGE, cInfo.usock);
                }
            }
        }
    }

    /* now we need a server stat or a Nak */

    if ((pkttype = pktRead(buf, PKT_MAXSIZE, 60)) < 0)
    {
        utLog("clientHello: read of SP_ACK or SP_SERVERSTAT failed\n");
        return false;
    }

    if (pkttype == SP_ACKMSG || pkttype == SP_ACK)/* we only get this if problem */
    {
        if (PKT_PROCSP(buf))
	{
            utLog("clientHello: %s '%s'\n",
                  pktSeverity2String(sAckMsg.severity),
                  sAckMsg.txt);
            printf("clientHello: %s '%s'\n",
                   pktSeverity2String(sAckMsg.severity),
                   sAckMsg.txt);

	}
        return false;
    }

    if (pkttype == SP_SERVERSTAT)
    {
        procServerStat(buf);
# if defined(DEBUG_CLIENTPROC)
        utLog("clientHello: recv SP_SERVERSTAT: ships = %d, na = %d, nv = %d, nr = %d\n"
              " nu = %d flags = 0x%08x",
              sStat.numtotal,
              sStat.numactive,
              sStat.numvacant,
              sStat.numrobot,
              sStat.numusers,
              sStat.flags);
#endif
    }
    else
    {
        utLog("clientHello: pkttype = %d, was waiting for SP_SERVERSTAT", pkttype);
        return false;
    }

    if (cInfo.doUDP)
        pktSendAck(PSEV_INFO, PERR_DOUDP, NULL);
    else
        pktSendAck(PSEV_INFO, PERR_OK, NULL);

    return true;
}

/* Some routers/firewalls seem to disconnect a UDP conncetion if there haven't
 * been any outgoing (client->server) packets for awhile.  Normally, conquest
 * never sends UDP packets except during UDP negotiation at the start of a
 * game.  This routine will send a CPCMD_KEEPALIVE packet every 30 seconds
 * if a UDP connection is enabled.  Seems to solve clute's problem.
 */
void sendUDPKeepAlive(uint32_t timebase)
{
    static uint32_t katime = 0;     /* UDP keepalive packets */
    static const uint32_t kawait = 30000;  /* ms (30 seconds) */
    uint32_t iternow;

    if (!cInfo.doUDP)
        return;                     /* no point */

    if (timebase)                 /* don't query the clock */
        iternow = timebase;
    else
        iternow = clbGetMillis();

    /* send a UDP keepalive packet if it's time */
    if (((iternow - katime) > kawait) && cInfo.usock != -1)
    {
#if 0
        utLog("%s: Sending CPCMD_KEEPALIVE\n", __FUNCTION__);
#endif
        sendCommand(CPCMD_KEEPALIVE, 0);
        katime = iternow;
    }

    return;
}

/* this function accepts a character buffer representing a clientstat packet
   and validates it.  It returns a pointer to a static spClientStat_t
   packet if everything is in order, NULL otherwise. */
spClientStat_t *chkClientStat(char *buf)
{
    static spClientStat_t scstat;

    if (!buf)
        return NULL;

    scstat = *(spClientStat_t *)buf;

    scstat.unum = (uint16_t)ntohs(scstat.unum);

    if (scstat.unum >= cbLimits.maxUsers())
    {
#if defined(DEBUG_PKT)
        utLog("%s: unum not in valid range", __FUNCTION__);
#endif
        return NULL;
    }

    if (scstat.snum < 0 || scstat.snum >= cbLimits.maxShips())
    {
#if defined(DEBUG_PKT)
        utLog("%s: snum not in valid range", __FUNCTION__);
#endif
        return NULL;
    }

    if (scstat.team >= NUMALLTEAMS)
    {
#if defined(DEBUG_PKT)
        utLog("%s: team not in valid range", __FUNCTION__);
#endif
        return NULL;
    }

    return &scstat;
}
