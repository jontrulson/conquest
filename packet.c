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
#include "protocol.h"
#define NOEXTERN
#include "packet.h"
#undef NOEXTERN

#ifndef HAVE_SELECT
#error "The select() system call is required"
#endif

#define SOCK_TMOUT 15		/* 15 seconds to complete packet */

static int connDead = 0;	/* if we die for some reason */

int isConnDead(void)
{
  return connDead;
}

void pktNotImpl(void *nothing)
{
  clog("packet: NULL/Not Implemented\n");
  return;
}

/* sends acks. to/from client/server. The server (TOCLIENT) can add a
   string message as well. */

int sendAck(int sock, int dir, Unsgn8 severity, Unsgn8 code, Unsgn8 *msg)
{
  cpAck_t cack;
  spAck_t sack;
  spAckMsg_t sackmsg;
  Unsgn8 *buf;

  switch (dir)
    {
    case PKT_TOSERVER:
      cack.type = CP_ACK;
      cack.severity = severity;
      cack.code = code;

      buf = (Unsgn8 *)&cack;

      break;
    case PKT_TOCLIENT:
      if (msg)
	{
	  sackmsg.type = SP_ACKMSG;
	  memset(sackmsg.txt, 0, MESSAGE_SIZE);
	  strncpy(sackmsg.txt, msg, MESSAGE_SIZE - 1);
	  sackmsg.severity = severity;
	  sackmsg.code = code;

	  buf = (Unsgn8 *)&sackmsg;
	}
      else
	{
	  sack.type = SP_ACK;
	  sack.severity = severity;
	  sack.code = code;
	  sack.severity = severity;
	  sack.code = code;
	  buf = (Unsgn8 *)&sack;
	}
      break;

    default:
      clog("sendAck: invalid dir = %d\n", dir);
      return -1;
      break;
    }

  return(writePacket(dir, sock, buf));
}

char *psev2String(int psev)
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
int invertDir(int dir)
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

int waitForPacket(int dir, int sock, int type, Unsgn8 *buf, int blen, 
		  int delay, char *nakmsg)
{
  int pkttype;

  while (TRUE)
    {
      errno = 0;		/* be afraid. */
      if ((pkttype = readPacket(dir, sock, buf, blen, delay)) >= 0)
	{
	  if (pkttype == type || type == PKT_ANYPKT || pkttype == 0)
	    return pkttype;

	  if (pkttype != type && nakmsg) /* we need to use a msg nak */
	    sendAck(sock, invertDir(dir), PSEV_ERROR, PERR_UNSPEC, nakmsg); 
	}

      if (pkttype < 0)
	{
	  if (errno != EINTR)
	    clog("waitForPacket(dir=%d): read error %s\n", dir, strerror(errno));
	  return -1;
	}
    }

  return -1;			/* NOTREACHED */

}



int serverPktSize(int type)
{
  if (type <= 0 || type >= SERVERPKTMAX)
    {
      clog("serverPktSize: invalid packet type %d\n",
	   type);
      /*      abort();*/
      return 0;
    }

  return serverPackets[type].size;
}
      
int clientPktSize(int type)
{

  if (type <= 0 || type >= CLIENTPKTMAX)
    {
      clog("clientPktSize: invalid packet type %d\n",
	   type);
      return 0;
    }

  return clientPackets[type].size;
}

/* like iochav(), but for the network connection.  return true if there
   is packet data ready */
int isPacketWaiting(int sock)
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


int readPacket(int direction, int sock, Unsgn8 *buf, int blen, 
	       unsigned int delay)
{
  Unsgn8 type;
  int len, rlen, left, rv;
  struct timeval timeout;
  fd_set readfds;
  
  if (connDead || direction == -1) 
    return -1;

  timeout.tv_sec = delay;		/* timeout for intial read */
  timeout.tv_usec = 0;

  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);

  if ((rv=select(sock+1, &readfds, NULL, NULL, &timeout)) > 0)
    {				/* we have a byte */
      if (FD_ISSET(sock, &readfds))
	{
	  if ((rv = read(sock, &type, 1)) <= 0)
	    {
	      *buf = 0;
	      return -1;
	    }
	}
      else
	{
	  clog("readPacket: select returned >0, but !FD_ISSET");
	  return 0;
	}

    }
  else if (rv == 0)
    {				/* timed out */
      return 0;
    }
  else if (rv < 0)		/* error */
    {
      return -1;
    }

  switch(direction)
    {
    case PKT_FROMSERVER:
      len = serverPktSize(type);
      break;
    case PKT_FROMCLIENT:
      len = clientPktSize(type);
      break;
    default:
      clog("readPacket: Invalid dir code %s\n", direction);
      return -1;
      break;
    }

  if (len)
    {
      if (len >= blen)		/* buf too small */
	{
	  clog("readPacket: buffer too small\n");
	  return -1;
	}
      len = len - sizeof(Unsgn8);
      left = len;

      while (left > 0)
	{
	  timeout.tv_sec = SOCK_TMOUT; /* longest we will wait */
	  timeout.tv_usec = 0;
      
	  FD_ZERO(&readfds);
	  FD_SET(sock, &readfds);
	  
	  if ((rv=select(sock+1, &readfds, NULL, NULL, &timeout)) > 0)
	    {			/* some data avail */
	      if ((rlen = read(sock, ((buf + 1) + (len - left)), left)) > 0)
		{
		  /* do we have enough? */
		  if ((left - rlen) > 0 /*len != rlen*/)
		    {
		      clog("readPacket: short packet: type(%d) len = %d, rlen = %d left = %d\n",
			   type, len, rlen, left - rlen);
		      left -= rlen;
		      continue;	/* get rest of packet */
		    }

		  buf[0] = type;
		  return type;
		}
	      else
		{
		  if (rlen == 0)
		    {
		      clog("readPacket: read returned 0");
		      return -1;
		    }

		  if (errno == EINTR)
		    continue;
		  else
		    {
		      clog("readPacket: read returned %d: %s", rlen,
			   strerror(errno));
		      return -1;
		    }
		}
	    }

	  if (rv == 0)		/* timeout */
	    {
	      clog("readPacket: timed out - connDead\n");
	      connDead = 1;
	      return -1;
	    }
	  else if (rv < 0)
	    {
	      if (errno == EINTR)
		continue;
	      clog("readPacket: select error: %s\n", strerror(errno));
	      return -1;
	    }
	}

    }
  else
    clog("readPacket: invalid packet type read %d\n",
	 type);

  return -1;
}


int writePacket(int direction, int sock, Unsgn8 *packet)
{
  int len, wlen, left;
  Unsgn8 type = *packet;	/* first byte is ALWAYS pkt type */

  if (connDead) 
    return -1;

  switch(direction)
    {
    case PKT_TOSERVER:
      len = clientPktSize(type);
      break;
    case PKT_TOCLIENT:
      len = serverPktSize(type);
      break;
    default:
      clog("writePacket: Invalid dir code %s\n", direction);
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
		  clog("writePacket: wrote short packet: left = %d, wlen = %d, len = %d",
		       left, wlen, len);
		  left -= wlen;
		  continue;
		}
	      return(TRUE);
	    }
	  else
	    {
	      if (wlen < 0 && errno == EINTR)
		{
		  clog("writePacket: write: EINTR");
		  continue;
		}

	      if (wlen == 0)
		{
		  clog("writePacket: wrote 0: %s", strerror(errno));
		  continue;
		}

	      clog("writePacket: write (wlen=%d): %s", wlen, strerror(errno));
	      return FALSE;
	    }
	}
    }
  else
    clog("writePacket: invalid packet type %d\n", type);

  return(FALSE);
}  


/* Simply check pkt for non-NULL, and compare pkttype with packet's type */
int validPkt(int pkttype, void *pkt)
{
  Unsgn8 *p = (Unsgn8 *)pkt;

  if (!p)
    return FALSE;

  if (((Unsgn8) *p) != pkttype)
    return FALSE;

  return TRUE;
}
