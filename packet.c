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

int pktIsConnDead(void)
{
  return connDead;
}

void pktNotImpl(void *nothing)
{
  utLog("packet: NULL/Not Implemented\n");
  return;
}

/* sends acks. to/from client/server. The server (TOCLIENT) can add a
   string message as well. */

int pktSendAck(int sock, int dir, Unsgn8 severity, Unsgn8 code, char *msg)
{
  cpAck_t cack;
  spAck_t sack;
  spAckMsg_t sackmsg;
  void *buf;

  switch (dir)
    {
    case PKT_TOSERVER:
      cack.type = CP_ACK;
      cack.severity = severity;
      cack.code = code;

      buf = &cack;

      break;
    case PKT_TOCLIENT:
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
      break;

    default:
#if defined(DEBUG_PKT)
      utLog("pktSendAck: invalid dir = %d\n", dir);
#endif
      return -1;
      break;
    }

  return(pktWrite(dir, sock, buf));
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

/* this just inverts the meaning of a packet direction - TOCLIENT becomes
   FROMCLIENT, etc... */
int pktInvertDirection(int dir)
{
  switch (dir)
    {
    case PKT_TOCLIENT:
      return PKT_FROMCLIENT;
      break;
   case PKT_TOSERVER:
      return PKT_FROMSERVER;
      break;
   case PKT_FROMCLIENT:
      return PKT_TOCLIENT;
      break;
   case PKT_FROMSERVER:
      return PKT_TOSERVER;
      break;
   default:
      return -1;
      break;
    }
  return -1;			/* NOTREACHED */
}

int pktWaitForPacket(int dir, int sockl[], int type, char *buf, int blen, 
		  int delay, char *nakmsg)
{
  int pkttype;

  while (TRUE)
    {
      errno = 0;		/* be afraid. */
      if ((pkttype = pktRead(dir, sockl, buf, blen, delay)) >= 0)
	{
	  if (pkttype == type || type == PKT_ANYPKT || pkttype == 0)
	    return pkttype;

	  if (pkttype != type && nakmsg) /* we need to use a msg nak */
	    pktSendAck(sockl[0], pktInvertDirection(dir), PSEV_ERROR, PERR_UNSPEC, 
                    nakmsg); 
	}

      if (pkttype < 0)
	{
	  if (errno != EINTR)
            {
#if defined(DEBUG_PKT)
              utLog("pktWaitForPacket(dir=%d): read error %s\n", dir, strerror(errno));
#endif
              return -1;
            }
	}
    }

  return -1;			/* NOTREACHED */

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
int pktIsPacketWaiting(int sock)
{
  struct timeval timeout;
  fd_set readfds;
  
  timeout.tv_sec = 0;		/* no wait */
  timeout.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);

  if (select(sock+1, &readfds, NULL, NULL, &timeout) > 0)
    return TRUE;
  else
    return FALSE;
}


/* sockl[0] is expected tcp socket, sockl[1] is for udp */
int pktRead(int direction, int sockl[], char *buf, int blen, 
	       unsigned int delay)
{
  Unsgn8 type;
  int len, rlen, left, rv;
  struct timeval timeout;
  fd_set readfds;
  int maxfd;
  int gotudp = FALSE;           
  int vartype;                  /* variable type (SP/CP) */

  if (connDead || direction == -1) 
    return -1;

  timeout.tv_sec = delay;		/* timeout for intial read */
  timeout.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(sockl[0], &readfds);
  maxfd = sockl[0];
  if (sockl[1] >= 0)
    {
      FD_SET(sockl[1], &readfds);
      maxfd = max(sockl[0], sockl[1]);
    }

  if ((rv=select(maxfd+1, &readfds, NULL, NULL, &timeout)) > 0)
    {				/* we have a byte */
      if (FD_ISSET(sockl[0], &readfds))
	{
	  if ((rv = read(sockl[0], &type, 1)) <= 0)
	    {
	      *buf = 0;
              utLog("ERROR: pktRead(): TCP read(header type): %s",
                   strerror(errno));
	      return -1;
	    }
	}
      else if (sockl[1] >= 0 && FD_ISSET(sockl[1], &readfds))
        {                     /* got a udp, read the whole thing */
          if ((rv = read(sockl[1], buf, blen)) <= 0)
            {
              *buf = 0;
              utLog("ERROR: pktRead(): UDP read(header type): %s",
                   strerror(errno));
              return -1;
            }
          else
            {
              gotudp = TRUE;
              type = buf[0];
            }
        }
      else
        {
          utLog("pktRead: select returned >0, but !FD_ISSET");
          return 0;
        }
    }
  else if (rv == 0)
    {				/* timed out */
      return 0;
    }
  else if (rv < 0)		/* error */
    {
      utLog("ERROR: pktRead(): select(): %s",
           strerror(errno));
      return -1;
    }

  switch(direction)
    {
    case PKT_FROMSERVER:
      len = pktServerPacketSize(type);
      vartype = SP_VARIABLE;    /* possible variable */
      break;
    case PKT_FROMCLIENT:
      len = pktClientPacketSize(type);
      vartype = CP_VARIABLE;      /* possible variable */
      break;
    default:
      utLog("pktRead: Invalid dir code %s", direction);
      return -1;
      break;
    }

  pktRXBytes += len;

  if (gotudp)
    {                           /* then we already got the whole packet */
      if (rv != len)
        {
          utLog("gotudp: rv != len: %d %d", rv, len);
          *buf = 0;
          type = 0;

          return 0;
        }

      if (type == vartype)
        {                       /* if encap packet, do the right thing */
          memmove(buf, buf + sizeof(struct _generic_var), 
                 rv - sizeof(struct _generic_var));
          type = buf[0];
        }
 
      return type;
    }
        

  if (len)
    {
      if (len >= blen)		/* buf too small */
	{
	  utLog("pktRead: buffer too small");
	  return -1;
	}
      len = len - sizeof(Unsgn8);
      left = len;

      while (left > 0)
	{
	  timeout.tv_sec = SOCK_TMOUT; /* longest we will wait */
	  timeout.tv_usec = 0;
      
	  FD_ZERO(&readfds);
	  FD_SET(sockl[0], &readfds);
	  
	  if ((rv=select(sockl[0]+1, &readfds, NULL, NULL, &timeout)) > 0)
	    {			/* some data avail */
	      if ((rlen = read(sockl[0], ((buf + 1) + (len - left)), left)) > 0)
		{
		  /* do we have enough? */
		  if ((left - rlen) > 0 /*len != rlen*/)
		    {
#if defined(DEBUG_PKT)
		      utLog("pktRead: short packet: type(%d) len = %d, "
                           "rlen = %d left = %d",
			   type, len, rlen, left - rlen);
#endif
		      left -= rlen;
		      continue;	/* get rest of packet */
		    }

                                /* we're done... maybe */
                  if (type == vartype)
                    {         /* if encap packet, do the right thing */
                      pktVariable_t *vpkt = (pktVariable_t *)buf;

                      /* read the first byte (type) of new pkt */
                      if ((rv = read(sockl[0], &type, 1)) <= 0)
                        {
                          *buf = 0;
                          utLog("ERROR: pktRead(): VARTYPE read(header type): %s",
                               strerror(errno));
                          return -1;
                        }

                      /* now reset the bytes left so the encapsulated pkt
                         can be read */

                      len = left = (vpkt->len - 1);
                      continue;
                    }

                  /* really done */
		  buf[0] = type;
		  return type;
		}
	      else
		{
		  if (rlen == 0)
		    {
		      utLog("pktRead: ERROR: read returned 0");
		      return -1;
		    }

		  if (errno == EINTR)
		    continue;
		  else
		    {
		      utLog("pktRead: read returned %d: %s", rlen,
			   strerror(errno));
		      return -1;
		    }
		}
	    }

	  if (rv == 0)		/* timeout */
	    {
	      utLog("pktRead: timed out - connDead");
	      connDead = 1;
	      return -1;
	    }
	  else if (rv < 0)
	    {
	      if (errno == EINTR)
		continue;
	      utLog("pktRead: select error: %s", strerror(errno));
	      return -1;
	    }
	}

    }
  else
    utLog("pktRead: invalid packet type read %d\n",
	 type);

  return -1;
}


int pktWrite(int direction, int sock, void *data)
{
  int len, wlen, left;
  Unsgn8 type;
  char *packet = (char *)data;

  type = (Unsgn8)*packet;	/* first byte is ALWAYS pkt type */

  if (connDead) 
    return -1;

  switch(direction)
    {
    case PKT_TOSERVER:
      len = pktClientPacketSize(type);
      break;
    case PKT_TOCLIENT:
      len = pktServerPacketSize(type);
      break;
    default:
#if defined(DEBUG_PKT)
      utLog("pktWrite: Invalid dir code %s\n", direction);
#endif
      return -1;
      break;
    }

  if (len)
    {
      left = len;

      while (left > 0)
	{
	  if ((wlen = write(sock, packet, left)) > 0)
	    {
	      if ((left - wlen) > 0)
		{
#if defined(DEBUG_PKT)
		  utLog("pktWrite: wrote short packet: left = %d, "
                       "wlen = %d, len = %d",
		       left, wlen, len);
#endif
		  left -= wlen;
		  continue;
		}
	      return(TRUE);
	    }
	  else
	    {
	      if (wlen < 0 && errno == EINTR)
		{
		  utLog("pktWrite: write: Interrupted");
		  continue;
		}

	      if (wlen == 0)
		{
		  utLog("pktWrite: wrote 0: %s", strerror(errno));
		  continue;
		}

	      utLog("pktWrite: write (wlen=%d): %s", wlen, strerror(errno));
	      return FALSE;
	    }
	}
    }
  else
    {
      utLog("pktWrite: invalid packet type %d\n", type);
/*       abort(); */
    }

  return(FALSE);
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

void pktSetNodelay(int sock)
{
  /* turn off TCP delay. */
  int on = 1;
  struct protoent *p = getprotobyname("tcp");

  if (!p)
    {
      utLog("INFO: getprotobyname(tcp) == NULL");
      return;
    }

  if (setsockopt(sock, 
                 p->p_proto, TCP_NODELAY, (void *)&on, sizeof(on)) <  0) 
    {
      utLog("INFO: setsockopt(TCP_NODELAY) failed: %s",
           strerror(errno));
    }
  
  return;
}

