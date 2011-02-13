#include "c_defs.h"

/************************************************************************
 *
 * client specific packet send routines (to server)
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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
#include "conqcom.h"
#include "context.h"
#include "msg.h"
#include "record.h"
#include "udp.h"
#include "color.h"
#include "ui.h"

/* send an auth packet and wait for a response (if not CHPWD).
   returns the error code from the ack, or error from write/read */
int sendAuth(int sock, Unsgn8 flag, char *login, char *pw)
{
  char buf[PKT_MAXSIZE];
  int rv;
  cpAuthenticate_t cauth;

  memset((void *)&cauth, 0, sizeof(cauth));

  cauth.type = CP_AUTHENTICATE;
  cauth.flag = flag;

  if (login)
    strncpy((char *)cauth.login, (char *)login, MAXUSERNAME - 1);
  if (pw)
    strncpy((char *)cauth.pw, (char *)pw, MAXUSERNAME - 1);

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

int sendSetCourse(int sock, Sgn8 lock, real head)
{
  cpSetCourse_t csc;

  memset((void *)&csc, 0, sizeof(cpSetCourse_t));

  csc.type = CP_SETCOURSE;
  csc.lock = lock;
  csc.head = (Unsgn16)htons((Unsgn16)(head * 100.0));

  if (pktWrite(PKT_SENDTCP, &csc) <= 0)
    return FALSE;
  else
    return TRUE;
}

/* send a command to server to change name */
int sendSetName(char *name)
{
  cpSetName_t sname;

  if (!name)
    return FALSE;

  memset((void *)&sname, 0, sizeof(cpSetName_t));
  sname.type = CP_SETNAME;
  strncpy((char *)sname.alias, name, MAXUSERPNAME - 1);

  if (pktWrite(PKT_SENDTCP, &sname) <= 0)
    return FALSE;
  else 
    return TRUE;
}

int sendCommand(Unsgn8 cmd, Unsgn16 detail)
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
    return FALSE;
  else 
    return TRUE;
}

int sendFireTorps(int num, real dir)
{
  cpFireTorps_t ftorps;

  memset((void *)&ftorps, 0, sizeof(cpFireTorps_t));

  ftorps.type = CP_FIRETORPS;
  ftorps.num = (Unsgn8)num;
  ftorps.dir = htons((Unsgn16)(dir * 100.0));

  if (pktWrite(PKT_SENDTCP, &ftorps) <= 0)
    return FALSE;
  else 
    return TRUE;

}

/* send a message */
int sendMessage(int to, char *msg)
{
  cpMessage_t cmsg;

  if (!msg)
    return FALSE;

  memset((void *)&cmsg, 0, sizeof(cpMessage_t));
  cmsg.type = CP_MESSAGE;
  cmsg.to = (Sgn16)htons(to);
  strncpy((char *)cmsg.msg, msg, MESSAGE_SIZE - 1);

  if (pktWrite(PKT_SENDTCP, &cmsg) <= 0)
    return FALSE;
  else 
    return TRUE;
}

int clientHello(char *clientname)
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
    return FALSE;
  }

  if (pkttype == 0)
  {
    utLog("clientHello: read server hello: timeout.\n");
    return FALSE;
  }

  /* we only get this if there's problem (server denied access, usually) */
  if (pkttype == SP_ACKMSG || pkttype == SP_ACK)
    {
      if (PKT_PROCSP(buf))
        utLog("clientHello: %s '%s'",
              pktSeverity2String(sAckMsg.severity),
              sAckMsg.txt);

      return FALSE;
    }

  if (pkttype != SP_HELLO)
  {
    utLog("clientHello: read server hello: wrong packet type %d\n", pkttype);
    return FALSE;
  }

  sHello = *(spHello_t *)buf;

  /* fix up byte ordering */
  sHello.protover = (Unsgn16)ntohs(sHello.protover);
  sHello.cmnrev = (Unsgn32)ntohl(sHello.cmnrev);

  sHello.servername[CONF_SERVER_NAME_SZ - 1] = 0;
  sHello.serverver[CONF_SERVER_NAME_SZ - 1] = 0;
  sHello.motd[CONF_SERVER_MOTD_SZ - 1] = 0;

  utLog("SERVERID:%s:%s:0x%04hx:%d:0x%02x:%s",
       sHello.servername,
       sHello.serverver,
       sHello.protover,
       sHello.cmnrev,
       sHello.flags,
       sHello.motd);

  if (cInfo.tryUDP)
    {
      if (connect(cInfo.usock, (const struct sockaddr *)&cInfo.servaddr, 
                  sizeof(cInfo.servaddr)) < 0)
        {
          utLog("NET: clientHello: udp connect() failed: %s", strerror(errno));
          cInfo.tryUDP = FALSE;
        }
      else
        {
          /* see if this will succeed in setting up a NAT tunnel
             to the server */
          utLog("NET: clientHello: send udp to server.");
#if defined(MINGW)
          send(cInfo.usock, "Open Me", 7, 0);
#else
          write(cInfo.usock, "Open Me", 7);
#endif
        }
    }

  /* now send a client hello */
  chello.type = CP_HELLO;
  chello.updates = Context.updsec;
  chello.protover = htons(PROTOCOL_VERSION);
  chello.cmnrev = htonl(COMMONSTAMP);

  strncpy((char *)chello.clientname, clientname, CONF_SERVER_NAME_SZ);
  strncpy((char *)chello.clientver, ConquestVersion, CONF_SERVER_NAME_SZ);

  strcat((char *)chello.clientver, " ");
  strncat((char *)chello.clientver, ConquestDate, 
	  (CONF_SERVER_NAME_SZ - strlen(ConquestVersion)) - 2);

  if (pktWrite(PKT_SENDTCP, &chello) <= 0)
    {
      utLog("clientHello: write client hello failed\n");
      return FALSE;
    }

  utLog("clientHello: sent hello to server");

  if (cInfo.tryUDP)
    {
      /* see if we get an ack back from the server via udp */
      /* this is kind of weak and probably needs more work.  As it is, the client needs
         to receive a udp from the server within 5 seconds, or UDP will not be used.  If the
         inbound packet gets lost.... Oh well. */
      tv.tv_sec = 5;            /* 5 secs */
      tv.tv_usec = 0;
      FD_ZERO(&readfds);
      FD_SET(cInfo.usock, &readfds);
      if ((rv = select(cInfo.usock+1, &readfds, NULL, NULL, &tv)) <= 0)
        {
          utLog("CLIENT: hello: select udp failed: %s", strerror(errno));
          cInfo.tryUDP = FALSE;
        }
      else
        {
          if (rv > 0 && FD_ISSET(cInfo.usock, &readfds))
            {
              if ((rv = udpRecv(cInfo.usock, buf, PKT_MAXSIZE, &cInfo.servaddr)) >= 0 )
                {
                  utLog("NET: got (%d) UDP bytes from server, will ACK for server UDP", rv);
                  cInfo.doUDP = TRUE;
                  pktSetSocketFds(PKT_SOCKFD_NOCHANGE, cInfo.usock);
                }
            }
        }
    }

  /* now we need a server stat or a Nak */

  if ((pkttype = pktRead(buf, PKT_MAXSIZE, 60)) < 0)
  {
    utLog("clientHello: read of SP_ACK or SP_SERVERSTAT failed\n");
    return FALSE;
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
      return FALSE;
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
      return FALSE;
    }

  if (cInfo.doUDP)
    pktSendAck(PSEV_INFO, PERR_DOUDP, NULL);
  else
    pktSendAck(PSEV_INFO, PERR_OK, NULL);

  return TRUE;
}

/* Some routers/firewalls seem to disconnect a UDP conncetion if there haven't
 * been any outgoing (client->server) packets for awhile.  Normally, conquest
 * never sends UDP packets except during UDP negotiation at the start of a
 * game.  This routine will send a CPCMD_KEEPALIVE packet every 60 seconds
 * if a UDP connection is enabled.  Seems to solve clute's problem.
 */
void sendUDPKeepAlive(Unsgn32 timebase)
{
  static Unsgn32 katime = 0;     /* UDP keepalive packets */
  static const Unsgn32 kawait = 30000;  /* ms (30 seconds) */
  Unsgn32 iternow;

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
   and validates it.  It return a pointer to a static spClientStat_t
   packet if everything is in order, NULL otherwise. */
spClientStat_t *chkClientStat(char *buf)
{
  static spClientStat_t scstat;

  if (!buf)
    return NULL;

  scstat = *(spClientStat_t *)buf;
  
  scstat.unum = (Unsgn16)ntohs(scstat.unum);

  if (scstat.unum >= MAXUSERS)
    {
#if defined(DEBUG_PKT)
      utLog("%s: unum not in valid range", __FUNCTION__);
#endif
      return NULL;
    }

  if (scstat.snum < 1 || scstat.snum > MAXSHIPS)
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
  
