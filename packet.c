#include "c_defs.h"

/************************************************************************
 *
 * packet - packet handling for conquest
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "datatypes.h"
#include "conf.h"
#include "protocol.h"
#include "conqutil.h"
#define NOEXTERN_PACKET
#include "packet.h"
#undef NOEXTERN_PACKET
#ifndef HAVE_SELECT
#error "The select() system call is required"
#endif

#include "global.h"
#include "rb.h"

/* our ringbuffers */
static ringBuffer_t  *RB_TCPIn = NULL;      /* input data TCP */
static ringBuffer_t  *RB_UDPIn = NULL;      /* input data UDP */
static ringBuffer_t  *RB_TCPOut = NULL;     /* output data TCP */
static ringBuffer_t  *RB_UDPOut = NULL;     /* output data UDP */

/* our tcp/udp sockets */
static int tcp_sock = -1;
static int udp_sock = -1;

/* default to 'server' mode.  If true, then we are being used in the
 * client
 */
static int isClient = FALSE;

/* these need to be kept in sync with the protocol numbers in
 *  protocol.h
 */
static struct _packetent clientPackets[] = {
  { CP_NULL,                    /* pktid */
    sizeof(cpNull_t),           /* size */
    "CP_NULL",                  /* name */
    pktNotImpl                  /* handler */
  },	/* never used */
  { CP_HELLO, 
    sizeof(cpHello_t), 
    "CP_HELLO", 
    pktNotImpl 
  },
  { CP_ACK, 
    sizeof(cpAck_t), 
    "CP_ACK", 
    pktNotImpl 
  },
  { CP_COMMAND, 
    sizeof(cpCommand_t), 
    "CP_COMMAND", 
    pktNotImpl 
  },
  { CP_FIRETORPS, 
    sizeof(cpFireTorps_t), 
    "CP_FIRETORPS", 
    pktNotImpl 
  },
  { CP_GETSINFO, 
    sizeof(cpGetSInfo_t), 
    "CP_GETSINFO", 
    pktNotImpl 
  },
  { CP_SENDMSG, 
    sizeof(cpSendMsg_t), 
    "CP_SENDMSG", 
    pktNotImpl 
  },
  { CP_SETNAME, 
    sizeof(cpSetName_t), 
    "CP_SETNAME", 
    pktNotImpl 
  },
  { CP_AUTHENTICATE, 
    sizeof(cpAuthenticate_t), 
    "CP_AUTHENTICATE", 
    pktNotImpl 
  },
  { CP_SETCOURSE,
    sizeof(cpSetCourse_t),
    "CP_SETCOURSE",
    pktNotImpl 
  },
  { CP_MESSAGE,
    sizeof(cpMessage_t),
    "CP_MESSAGE",
    pktNotImpl 
  },
  { CP_VARIABLE,
    0,                          /* these are special */
    "CP_VARIABLE",
    pktNotImpl 
  }
};

static struct _packetent serverPackets[] = {
  { SP_NULL,                    /* pktid */
    sizeof(spNull_t),           /* size */
    "SP_NULL",                  /* name */
    pktNotImpl                  /* handler */
  },	/* never used */
  { SP_HELLO, 
    sizeof(spHello_t), 
    "SP_HELLO", 
    pktNotImpl 
  },
  { SP_ACK, 
    sizeof(spAck_t), 
    "SP_ACK", 
    pktNotImpl 
  },
  { SP_SERVERSTAT, 
    sizeof(spServerStat_t), 
    "SP_SERVERSTAT", 
    pktNotImpl 
  },
  { SP_CLIENTSTAT, 
    sizeof(spClientStat_t), 
    "SP_CLIENTSTAT", 
    pktNotImpl 
  },
  { SP_SHIP, 
    sizeof(spShip_t), 
    "SP_SHIP", 
    pktNotImpl 
  },
  { SP_SHIPSML, 
    sizeof(spShipSml_t), 
    "SP_SHIPSML", 
    pktNotImpl 
  },
  { SP_SHIPLOC, 
    sizeof(spShipLoc_t), 
    "SP_SHIPLOC", 
    pktNotImpl 
  },
  { SP_PLANET, 
    sizeof(spPlanet_t), 
    "SP_PLANET", 
    pktNotImpl },
  { SP_PLANETSML, 
    sizeof(spPlanetSml_t), 
    "SP_PLANETSML", 
    pktNotImpl },
  { SP_PLANETLOC, 
    sizeof(spPlanetLoc_t), 
    "SP_PLANETLOC", 
    pktNotImpl 
  },
  { SP_MESSAGE, 
    sizeof(spMessage_t), 
    "SP_MESSAGE", 
    pktNotImpl 
  },
  { SP_USER, 
    sizeof(spUser_t), 
    "SP_USER", 
    pktNotImpl 
  },
  { SP_TORP, 
    sizeof(spTorp_t), 
    "SP_TORP", 
    pktNotImpl 
  },
  { SP_ACKMSG, 
    sizeof(spAckMsg_t), 
    "SP_ACKMSG", 
    pktNotImpl 
  },
  { SP_TEAM, 
    sizeof(spTeam_t), 
    "SP_TEAM", 
    pktNotImpl 
  },
  { SP_TORPLOC, 
    sizeof(spTorpLoc_t), 
    "SP_TORPLOC", 
    pktNotImpl 
  },
  { SP_CONQINFO, 
    sizeof(spConqInfo_t), 
    "SP_CONQINFO", 
    pktNotImpl 
  },
  { SP_FRAME, 
    sizeof(spFrame_t), 
    "SP_FRAME", 
    pktNotImpl 
  },
  { SP_HISTORY, 
    sizeof(spHistory_t), 
    "SP_HISTORY", 
    pktNotImpl 
  },
  { SP_DOOMSDAY, 
    sizeof(spDoomsday_t), 
    "SP_DOOMSDAY", 
    pktNotImpl 
  },
  { SP_PLANETINFO, 
    sizeof(spPlanetInfo_t), 
    "SP_PLANETINFO", 
    pktNotImpl 
  },
  { SP_PLANETLOC2, 
    sizeof(spPlanetLoc2_t), 
    "SP_PLANETLOC2", 
    pktNotImpl 
  },
  { SP_TORPEVENT, 
    sizeof(spTorpEvent_t), 
    "SP_TORPEVENT", 
    pktNotImpl 
  },
  { SP_VARIABLE, 
    0,                          /* these are special */
    "SP_VARIABLE", 
    pktNotImpl 
  }
};

#define CLIENTPKTMAX (sizeof(clientPackets) / sizeof(struct _packetent))
#define SERVERPKTMAX (sizeof(serverPackets) / sizeof(struct _packetent))


#define SOCK_TMOUT 15		/* 15 seconds to complete packet */

static int connDead = 0;	/* if we die for some reason */

/* initialize the packet stuff, mainly the RB's */
int pktInit(void)
{
  /* if RB's have already been created, destroy them */
  if (RB_UDPIn)
    {
      rbDestroy(RB_UDPIn);
      RB_UDPIn = NULL;
    }
  if (RB_TCPIn)
    {
      rbDestroy(RB_TCPIn);
      RB_TCPIn = NULL;
    }
  if (RB_TCPOut)
    {
      rbDestroy(RB_TCPOut);
      RB_TCPOut = NULL;
    }
  if (RB_UDPOut)
    {
      rbDestroy(RB_UDPOut);
      RB_UDPOut = NULL;
    }

  if (!(RB_UDPIn = rbCreate(PKT_UDP_RB_MAX)))
    {
      utLog("%s: rbCreate(PKT_UDP_RB_MAX=%d) failed.", 
            __FUNCTION__, PKT_UDP_RB_MAX);
      return FALSE;             /* this is fatal */
    }

  if (!(RB_TCPIn = rbCreate(PKT_TCP_RB_MAX)))
    {
      utLog("%s: rbCreate(PKT_TCP_RB_MAX=%d) failed.", 
            __FUNCTION__, PKT_TCP_RB_MAX);
      return FALSE;             /* this is fatal */
    }

  if (!(RB_UDPOut = rbCreate(PKT_UDP_RB_MAX)))
    {
      utLog("%s: rbCreate(PKT_UDP_RB_MAX=%d) failed.", 
            __FUNCTION__, PKT_UDP_RB_MAX);
      return FALSE;             /* this is fatal */
    }

  if (!(RB_TCPOut = rbCreate(PKT_TCP_RB_MAX)))
    {
      utLog("%s: rbCreate(PKT_TCP_RB_MAX=%d) failed.", 
            __FUNCTION__, PKT_TCP_RB_MAX);
      return FALSE;             /* this is fatal */
    }

  if (cqDebug)
    utLog("%s: initialized packet ring buffers", __FUNCTION__);

  return TRUE;
}


/* clients call this so these routines know how to validate packets */
void pktSetClientMode(int isclient)
{
  isClient = isclient;

  return;
}

int pktIsConnDead(void)
{
  return connDead;
}

void pktNotImpl(void *nothing)
{
  utLog("packet: NULL/Not Implemented\n");
  return;
}

/* initialize the tcp and udp sockets we'll be using in the packet routines */
void pktSetSocketFds(int tcpsock, int udpsock)
{
  if (tcpsock != PKT_SOCKFD_NOCHANGE)
    tcp_sock = tcpsock;
  if (udpsock != PKT_SOCKFD_NOCHANGE)
    udp_sock = udpsock;

  return;
}


/* sends acks. to/from client/server. The server can add a
 * string message as well. We always send acks via TCP.
 */

int pktSendAck(Unsgn8 severity, Unsgn8 code, char *msg)
{
  cpAck_t cack;
  spAck_t sack;
  spAckMsg_t sackmsg;
  void *buf;

  if (tcp_sock < 0)
    return -1;

  if (isClient)
    {
      cack.type = CP_ACK;
      cack.severity = severity;
      cack.code = code;
      
      buf = &cack;
    }
  else
    {                           /* server */
      if (msg)
	{
	  sackmsg.type = SP_ACKMSG;
	  memset(sackmsg.txt, 0, MESSAGE_SIZE);
	  strncpy((char *)sackmsg.txt, msg, MESSAGE_SIZE - 1);
	  sackmsg.severity = severity;
	  sackmsg.code = code;

	  buf = &sackmsg;
	}
      else
	{
	  sack.type = SP_ACK;
	  sack.severity = severity;
	  sack.code = code;
	  sack.severity = severity;
	  sack.code = code;

	  buf = &sack;
	}
    }    

  return(pktWrite(PKT_SENDTCP, buf));
}

char *pktSeverity2String(int psev)
{
  switch (psev)
    {
    case PSEV_INFO:
      return "INFO";
      break;

    case PSEV_WARN:
      return "WARN";
      break;

    case PSEV_ERROR:
      return "ERROR";
      break;

    case PSEV_FATAL:
      return "FATAL";
      break;

    default:
      return "";
      break;
    }

  return ("");			/* NOTREACHED */
}

int pktWaitForPacket(int type, char *buf, int blen, 
                     int delay, char *nakmsg)
{
  int pkttype;

  while (TRUE)
    {
      errno = 0;		/* be afraid. */
      if ((pkttype = pktRead(buf, blen, delay)) >= 0)
	{
	  if (pkttype == type || type == PKT_ANYPKT || pkttype == 0)
	    return pkttype;

	  if (pkttype != type && nakmsg) /* we need to use a msg nak */
	    pktSendAck(PSEV_ERROR, PERR_UNSPEC, nakmsg); 
	}

      if (pkttype < 0)
	{
	  if (errno != EINTR)
            {
#if defined(DEBUG_PKT)
              utLog("pktWaitForPacket: read error %s\n", strerror(errno));
#endif
              return -1;
            }
	}
    }

  return -1;			/* NOTREACHED */

}


int pktPacketSize(int type)
{
  /* clients always get server pkts, servers always get client pkts */
  if (isClient)
    return pktServerPacketSize(type);
  else
    return pktClientPacketSize(type);
}


int pktServerPacketSize(int type)
{
  if (type <= 0 || type >= SERVERPKTMAX)
    {
      utLog("pktServerPacketSize: invalid packet type %d\n",
	   type);

      /* abort();*/

      return 0;
    }

  return serverPackets[type].size;
}
      
int pktClientPacketSize(int type)
{

  if (type <= 0 || type >= CLIENTPKTMAX)
    {
      utLog("pktClientPacketSize: invalid packet type %d\n",
	   type);
      return 0;
    }

  return clientPackets[type].size;
}

/* like iochav(), but for the network connection.  return true if there
   is packet data ready */
int pktIsWaiting(void)
{
  struct timeval timeout;
  fd_set readfds;
  int maxfd;
  
  /* sanity check */
  if (tcp_sock < 0 && udp_sock < 0)
    return FALSE;

  timeout.tv_sec = 0;		/* no wait */
  timeout.tv_usec = 0;

  FD_ZERO(&readfds);

  if (tcp_sock >= 0)
    FD_SET(tcp_sock, &readfds);

  if (udp_sock >= 0)
    FD_SET(udp_sock, &readfds);

  maxfd = max(tcp_sock, udp_sock);
  if (select(maxfd + 1, &readfds, NULL, NULL, &timeout) > 0)
    return TRUE;
  else
    return FALSE;

}

/* get a packet from a ringbuffer.  If there's no data, or not
 * enough data, return 0, else return the packet type (and data)  
 */
static Unsgn8 _pktGetRB(ringBuffer_t *RB, void *buf, int blen)
{
  int bu, len;
  Unsgn8 type = 0;

  if (!buf || !blen)
    return 0;

  if ((bu = rbBytesUsed(RB)))
    {
      /* get the first character */
      rbGet(RB, &type, 1, FALSE);
      
      len = pktPacketSize(type);

      if (!len)
        {
          /* it's invalid, we are probably screwed, but dump it anyway */
          rbGet(RB, NULL, 1, TRUE);
          utLog("%s: invalid packet type read %d, dumping\n",
                __FUNCTION__, (int)type);
          return 0;
        }

      /* if there's enough data, we're done */
      if (bu >= len)
        {
          /* last sanity check */
          if (len > blen)
            {
              utLog("%s: return buffer too small, need %d, only have %d\n",
                    __FUNCTION__, len, blen);
              return 0;
            }

          /* now get it for real */
          rbGet(RB, buf, len, TRUE);
          return type;
        }
    }

  return 0;
}

/* read data into the buffer, possibly depositing it into an RB */
static int _pktReadSocket(int sock, ringBuffer_t *RB, void *buf, int blen)
{
  int rv, len, rlen;
  int type;
  char *packet = (char *)buf;

  rlen = min(blen, rbBytesFree(RB));
  
  if ((rv = read(sock, buf, rlen)) < 0)
    {
      *packet = 0;
      utLog("%s: read(%d): %s", __FUNCTION__, rlen, strerror(errno));
      return -1;
    }
  
  if (rv)
    {
      pktRXBytes += rv;         /* update the counter */

      /* if we read enough to return a packet, and there's
       * nothing in the rb, extract it and copy any leftovers into
       * the RB.
       */
      if (!rbBytesUsed(RB))
        {
          type = (Unsgn8)packet[0];
          
          len = pktPacketSize(type);
          
          if (len && rv >= len)
            {
              /* then we got enough to satify the request.
               * convieniently, the packet is already in buf, so
               * just put the rest in the RB :)
               */
              if ((rv - len) > 0)
                rbPut(RB, (void *)&packet[len], rv - len);
              
              return type;
            }
        }
      else
        {
          /* just put it all into the RB */
          rbPut(RB, buf, rv);
        }
    }

  return 0;
}

/* look for packets to make us go. */
int pktRead(char *buf, int blen, unsigned int delay)
{
  int type;
  int rv;
  struct timeval timeout;
  fd_set readfds;
  int maxfd = 0;

  if (connDead) 
    return -1;

  /* sanity check */
  if (tcp_sock < 0 && udp_sock < 0)
    return -1;

  /* First check the ringbuffers, TCP first.  If there's a packet waiting
   * get it and return it
   */

  if ((type = _pktGetRB(RB_TCPIn, buf, blen)))
    return type;

  if ((type = _pktGetRB(RB_UDPIn, buf, blen)))
    return type;

  /* if we're here, then either there was no RB data, or there wasn't
   * enough to satisfy the request.  So now is the time in Sprockets
   * when we read.
   */
  
  /* timeout for read */
  timeout.tv_sec = delay;
  timeout.tv_usec = 0;

  FD_ZERO(&readfds);
  if (tcp_sock >= 0)
    {
      FD_SET(tcp_sock, &readfds);
      maxfd = tcp_sock;
    }
  if (udp_sock >= 0)
    {
      FD_SET(udp_sock, &readfds);
      maxfd = max(tcp_sock, udp_sock);
    }

  if ((rv=select(maxfd+1, &readfds, NULL, NULL, &timeout)) > 0)
    {
      /* we have TCP data */
      if (FD_ISSET(tcp_sock, &readfds))
	{
          if ((type = _pktReadSocket(tcp_sock, RB_TCPIn, buf, blen)) < 0)
            {
              /* an error */
	      *buf = 0;
              utLog("%s: pktReadSocket TCP: failed", __FUNCTION__);
	      return -1;
            }              

          if (type)
            {
              /* data is already in buf */
              return type;
            }
	}

      /* now try for any UDP */
      if (udp_sock >= 0 && FD_ISSET(udp_sock, &readfds))
        {                     /* got a udp, read the whole thing */
          if ((type = _pktReadSocket(udp_sock, RB_UDPIn, buf, blen)) < 0)
            {
              /* an error */
	      *buf = 0;
              utLog("%s: pktReadSocket UDP: failed", __FUNCTION__);
	      return -1;
            }              

          if (type)
            {
              /* data is already in buf */
              return type;
            }
        }
    }
  else if (rv == 0)
    {				/* timed out */
      return 0;
    }
  else if (rv < 0)		/* error */
    {
      utLog("ERROR: pktRead(): select(): %s", strerror(errno));
      return -1;
    }

  /* if we're here, we try one more time on the RB's, in case some data
   * was recently read 
   */
  if ((type = _pktGetRB(RB_TCPIn, buf, blen)))
    return type;

  if ((type = _pktGetRB(RB_UDPIn, buf, blen)))
    return type;

  return 0;
}

/* write to actual sockets */
static int _pktWriteSocket(int sock, void *data, int len)
{
  return write(sock, data, len);
  
}

/* try to write out data in a ringbuffer */
static int _pktDrainRB(int sock, ringBuffer_t *RB)
{
  char buf[PKT_MAXSIZE];
  int wlen = 0;
  int len;

  /* no data, no point */
  if (!rbBytesUsed(RB))
    return 0;
  
  /* try to get as much data as possible and write it. */
  if ((len = rbGet(RB, (void *)buf, PKT_MAXSIZE, FALSE)))
    {
      if ((wlen = _pktWriteSocket(sock, buf, len)) <= 0)
        return wlen;

      /* remove the data for good, but don't copy it again */
      rbGet(RB, NULL, wlen, TRUE);
    }

  return wlen;
}

int pktWrite(int socktype, void *data)
{
  int len;
  Unsgn8 type;
  int rv;
  char *packet = (char *)data;
  char *ptr;
  int sock;
  ringBuffer_t *RB;

  type = (Unsgn8)*packet;	/* first byte is ALWAYS pkt type */

  if (connDead) 
    return -1;
  
  if (socktype == PKT_SENDUDP && udp_sock >= 0)
    {
      RB = RB_UDPOut;
      sock = udp_sock;
    }
  else
    {
      RB = RB_TCPOut;
      sock = tcp_sock;
    }

  if (isClient)
    len = pktClientPacketSize(type);
  else
    len = pktServerPacketSize(type);
    
  /* try to drain the RB if needed */
  if ((rv = _pktDrainRB(sock, RB)) < 0)
    return FALSE;

  /* if there is already data in the RB, just add the data to it. */
  if (rbBytesUsed(RB))
    {
      if (rbBytesFree(RB) < len)
        {
          utLog("%s: FATAL: output ringbuffer full.\n");
          return FALSE;
        }
      
      rbPut(RB, data, len);
    }
  else
    {
      /* try to write the data directly, and put any leftovers into the RB */
      if ((rv = _pktWriteSocket(sock, data, len)) < 0)
        return FALSE;

      if (len != rv)
        {
          /* there's some data left over */
          ptr = &packet[rv];
          
          if (rbPut(RB, (void *)ptr, len - rv) != (len - rv))
            {
              utLog("%s: FATAL: output ringbuffer full.\n");
              return FALSE;
            }
        }
    }            

  /* one last drain for old times sake */
  if ((rv = _pktDrainRB(sock, RB)) < 0)
    return FALSE;

  return TRUE;
}  


/* Simply check pkt for non-NULL, and compare pkttype with packet's type */
int pktIsValid(int pkttype, void *pkt)
{
  Unsgn8 *p = (Unsgn8 *)pkt;

  if (!p)
    return FALSE;

  if (((Unsgn8) *p) != pkttype)
    return FALSE;

  return TRUE;
}

void pktSetNodelay(void)
{
  /* turn off TCP delay. */
  int on = 1;
  struct protoent *p = getprotobyname("tcp");

  if (tcp_sock < 0)
    {
      utLog("INFO: pktSetNodelay: tcp_sock is invalid (%d)", tcp_sock);
      return;
    }

  if (!p)
    {
      utLog("INFO: pktSetNodelay: getprotobyname(tcp) == NULL");
      return;
    }

  if (setsockopt(tcp_sock, 
                 p->p_proto, TCP_NODELAY, (void *)&on, sizeof(on)) <  0) 
    {
      utLog("INFO: pktSetNodelay: setsockopt(TCP_NODELAY) failed: %s",
           strerror(errno));
    }
  
  return;
}

