/* 
 * server connect node
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"
#include "datatypes.h"
#include "color.h"
#include "conf.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "nConsvr.h"
#include "nAuth.h"

#include "glmisc.h"

#define CLIENTNAME "ConquestGL"

static char *rhost = NULL;
static Unsgn16 rport;

static int err = FALSE;

#define ERR_BUFSZ 128
static char errbuf1[ERR_BUFSZ];
static char errbuf2[ERR_BUFSZ];

static int nConsvrDisplay(dspConfig_t *);
static int nConsvrIdle(void);
static int nConsvrInput(int ch);

static scrNode_t nConsvrNode = {
  nConsvrDisplay,               /* display */
  nConsvrIdle,                  /* idle */
  nConsvrInput                  /* input */
};


void nConsvrInit(char *remotehost, Unsgn16 remoteport)
{
  rhost = remotehost;
  rport = remoteport;
  errbuf1[0] = 0;
  errbuf2[0] = 0;
  err = FALSE;

  setNode(&nConsvrNode);

  return;
}

static int hello(void)
{
  cpHello_t chello;
  spAckMsg_t *sackmsg;
  Unsgn8 buf[PKT_MAXSIZE];
  int pkttype;
  extern char *ConquestVersion, *ConquestDate;


  /* there should be a server hello waiting for us */
  if ((pkttype = readPacket(PKT_FROMSERVER, cInfo.sock, 
			    buf, PKT_MAXSIZE, 10)) < 0)
  {
    clog("HELLO: read server hello failed\n");
    return FALSE;
  }

  if (pkttype == 0)
  {
    clog("HELLO: read server hello: timeout.\n");
    return FALSE;
  }

  if (pkttype != SP_HELLO)
  {
    clog("HELLO: read server hello: wrong packet type %d\n", pkttype);
    return FALSE;
  }

  sHello = *(spHello_t *)buf;

  /* fix up byte ordering */
  sHello.protover = (Unsgn16)ntohs(sHello.protover);
  sHello.cmnrev = (Unsgn32)ntohl(sHello.cmnrev);

  sHello.servername[CONF_SERVER_NAME_SZ - 1] = 0;
  sHello.serverver[CONF_SERVER_NAME_SZ - 1] = 0;
  sHello.motd[CONF_SERVER_MOTD_SZ - 1] = 0;

  clog("HELLO: CLNT: sname = '%s'\n"
       "             sver = '%s'\n"
       "             protv = 0x%04hx, cmnr = %d, flags: 0x%02x\n"
       "             motd = '%s",
       sHello.servername,
       sHello.serverver,
       sHello.protover,
       sHello.cmnrev,
       sHello.flags,
       sHello.motd);

  /* now send a client hello */
  chello.type = CP_HELLO;
  chello.updates = Context.updsec;
  chello.protover = htons(PROTOCOL_VERSION);
  chello.cmnrev = htonl(COMMONSTAMP);

  strncpy(chello.clientname, CLIENTNAME, CONF_SERVER_NAME_SZ);
  strncpy(chello.clientver, ConquestVersion, CONF_SERVER_NAME_SZ);

  strcat(chello.clientver, " ");
  strncat(chello.clientver, ConquestDate, 
	  (CONF_SERVER_NAME_SZ - strlen(ConquestVersion)) - 2);

  if (!writePacket(PKT_TOSERVER, cInfo.sock, (Unsgn8 *)&chello))
    {
      clog("HELLO: write client hello failed\n");
      return FALSE;
    }

  clog("HELLO: sent client hello to server");

  /* now we need a server stat or a Nak */

  if ((pkttype = readPacket(PKT_FROMSERVER, cInfo.sock, 
			    buf, PKT_MAXSIZE, 5)) < 0)
  {
    clog("HELLO: read server SP_ACKMSG or SP_ACK failed\n");
    return FALSE;
  }

  if (pkttype == SP_ACKMSG || pkttype == SP_ACK)/* we only get this if problem */
    {
      if (pkttype == SP_ACKMSG)
	{
	  sackmsg = (spAckMsg_t *)buf;
	  if (sackmsg->txt)
	    {
	      clog("conquest:hello:NAK:%s '%s'\n", 
		   psev2String(sackmsg->severity), 
		   sackmsg->txt);
	      printf("conquest:hello:NAK:%s '%s'\n", 
		   psev2String(sackmsg->severity), 
		   sackmsg->txt);

	    }
	}
      return FALSE;
    }

  if (pkttype == SP_SERVERSTAT)
    {
      procServerStat(buf);
# if defined(DEBUG_CLIENTPROC)
      clog("HELLO: recv SP_SERVERSTAT: ships = %d, na = %d, nv = %d, nr = %d\n"
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
      clog("HELLO: pkttype = %d, was waiting for SP_SERVERSTAT", pkttype);
      return FALSE;
    }

  sendAck(cInfo.sock, PKT_TOSERVER, PSEV_INFO, PERR_OK, NULL);

  return TRUE;
}

static int nConsvrDisplay(dspConfig_t *dsp)
{
  int lin;

  if (!rhost)
    return NODE_EXIT;

  /* display the logo */
  mglConqLogo(dsp);

  lin = 12;

  cprintf(lin++, 0, ALIGN_CENTER, "Connecting to %s:%d",
          rhost, rport);

  
  if (err)
    {
      if (strlen(errbuf1))
        cprintf(lin++, 0, ALIGN_CENTER, "%s", errbuf1);
      
      if (strlen(errbuf2))
        cprintf(lin++, 0, ALIGN_CENTER, "%s", errbuf2);

      cprintf( MSG_LIN2, 0, ALIGN_CENTER,"#%d#%s", InfoColor, MTXT_DONE);
 
    }

  return NODE_OK;
}  
  
static int nConsvrIdle(void)
{
  int s;
  struct sockaddr_in sa;
  struct hostent *hp;

  /* no point in trying again... */
  if (err)
    return NODE_ERR;

  if (!rhost)
    return NODE_EXIT;

  /* should not happen - debugging */
  if (!cInfo.serverDead)
    {
      clog("ALREADY CONNECTED! I AM READY TO TRANSFER TO AUTH NODE!!!");      
      return NODE_OK;
    }

  if ((hp = gethostbyname(rhost)) == NULL) 
    {
      clog("conquest: %s: no such host\n", rhost);

      snprintf(errbuf1, sizeof(errbuf1) - 1, "%s: no such host", 
              rhost);
      err = TRUE;

      return NODE_ERR;
    }

  /* put host's address and address type into socket structure */
  memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);
  sa.sin_family = hp->h_addrtype;

  sa.sin_port = htons(rport);

  if ((s = socket(AF_INET, SOCK_STREAM, 0 )) < 0) 
    {
      clog("socket: %s", strerror(errno));
      snprintf(errbuf1, sizeof(errbuf1) - 1, "socket: %s", rhost);
      err = TRUE;

      return NODE_ERR;
    }

  clog("Connecting to host: %s, port %d\n",
       rhost, rport);

  /* connect to the remote server */
  if (connect(s, (const struct sockaddr *)&sa, sizeof(sa)) < 0) 
    {
      clog("connect %s:%d: %s", 
           rhost, rport, strerror(errno));

      snprintf(errbuf1, sizeof(errbuf1) - 1, "connect %s:%d: %s",
               rhost, rport, strerror(errno));
      snprintf(errbuf2, sizeof(errbuf2) - 1, 
               "Is there a conquestd server running there?");

      err = TRUE;

      return NODE_ERR;
    }

  cInfo.serverDead = FALSE;
  cInfo.sock = s;

  if (!hello())
    {
      clog("conquest: hello() failed\n");
      printf("conquest: hello() failed, check log\n");

      cInfo.serverDead = TRUE;
      return NODE_EXIT;
    }

  nAuthInit();                  /* transfer to Auth */

  return TRUE;
}

static int nConsvrInput(int ch)
{
  if (err)                      /* then we just exit */
    return NODE_EXIT;

  return NODE_OK;
}

