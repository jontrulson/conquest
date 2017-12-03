#include "c_defs.h"

/************************************************************************
 *
 * packet - packet handling for conquest
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/


#include "conf.h"
#include "protocol.h"
#include "conqutil.h"
#define NOEXTERN_PACKET
#include "packet.h"
#undef NOEXTERN_PACKET
#if !defined(MINGW) && !defined(HAVE_SELECT)
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

/* non-blocking i/o in effect flag */
static int nonBlocking = FALSE;

/* default protocol version that a client will be expecting.  This can
 * be changed, for example, to connect to an older (but supported
 * server) or to playback a cqr file recorded in an older (but
 * supported) protocol.
 */

static uint16_t clientProtoVers = PROTOCOL_VERSION;

/* these need to be kept in sync with the protocol numbers in
 *  protocol.h
 */
static packetEnt_t clientPackets[] = {
    { CP_NULL,                    /* pktid */
      sizeof(cpNull_t),           /* size */
      "CP_NULL",                  /* name */
      pktNotImpl                  /* handler, initialized by
                                   * procDispatchInit() */
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

#define CLIENTPKTMAX (sizeof(clientPackets) / sizeof(packetEnt_t))

static packetEnt_t serverPackets_0006[] = {
    { SP_0006_NULL,        /* pktid */
      sizeof(sp_0006_Null_t),    /* size */
      "SP_0006_NULL",      /* name */
      pktNotImpl           /* handler, init'd by procDispatchInit() */
    },	/* never used */
    { SP_0006_HELLO,
      sizeof(sp_0006_Hello_t),
      "SP_0006_HELLO",
      pktNotImpl
    },
    { SP_0006_ACK,
      sizeof(sp_0006_Ack_t),
      "SP_0006_ACK",
      pktNotImpl
    },
    { SP_0006_SERVERSTAT,
      sizeof(sp_0006_ServerStat_t),
      "SP_0006_SERVERSTAT",
      pktNotImpl
    },
    { SP_0006_CLIENTSTAT,
      sizeof(sp_0006_ClientStat_t),
      "SP_0006_CLIENTSTAT",
      pktNotImpl
    },
    { SP_0006_SHIP,
      sizeof(sp_0006_Ship_t),
      "SP_0006_SHIP",
      pktNotImpl
    },
    { SP_0006_SHIPSML,
      sizeof(sp_0006_ShipSml_t),
      "SP_0006_SHIPSML",
      pktNotImpl
    },
    { SP_0006_SHIPLOC,
      sizeof(sp_0006_ShipLoc_t),
      "SP_0006_SHIPLOC",
      pktNotImpl
    },
    { SP_0006_PLANET,
      sizeof(sp_0006_Planet_t),
      "SP_0006_PLANET",
      pktNotImpl
    },
    { SP_0006_PLANETSML,
      sizeof(sp_0006_PlanetSml_t),
      "SP_0006_PLANETSML",
      pktNotImpl
    },
    { SP_0006_PLANETLOC,
      sizeof(sp_0006_PlanetLoc_t),
      "SP_0006_PLANETLOC",
      pktNotImpl
    },
    { SP_0006_MESSAGE,
      sizeof(sp_0006_Message_t),
      "SP_0006_MESSAGE",
      pktNotImpl
    },
    { SP_0006_USER,
      sizeof(sp_0006_User_t),
      "SP_0006_USER",
      pktNotImpl
    },
    { SP_0006_TORP,
      sizeof(sp_0006_Torp_t),
      "SP_0006_TORP",
      pktNotImpl
    },
    { SP_0006_ACKMSG,
      sizeof(sp_0006_AckMsg_t),
      "SP_0006_ACKMSG",
      pktNotImpl
    },
    { SP_0006_TEAM,
      sizeof(sp_0006_Team_t),
      "SP_0006_TEAM",
      pktNotImpl
    },
    { SP_0006_TORPLOC,
      sizeof(sp_0006_TorpLoc_t),
      "SP_0006_TORPLOC",
      pktNotImpl
    },
    { SP_0006_CONQINFO,
      sizeof(sp_0006_cbConqInfo_t),
      "SP_0006_CONQINFO",
      pktNotImpl
    },
    { SP_0006_FRAME,
      sizeof(sp_0006_Frame_t),
      "SP_0006_FRAME",
      pktNotImpl
    },
    { SP_0006_HISTORY,
      sizeof(sp_0006_History_t),
      "SP_0006_HISTORY",
      pktNotImpl
    },
    { SP_0006_DOOMSDAY,
      sizeof(sp_0006_Doomsday_t),
      "SP_0006_DOOMSDAY",
      pktNotImpl
    },
    { SP_0006_PLANETINFO,
      sizeof(sp_0006_PlanetInfo_t),
      "SP_0006_PLANETINFO",
      pktNotImpl
    },
    { SP_0006_PLANETLOC2,
      sizeof(sp_0006_PlanetLoc2_t),
      "SP_0006_PLANETLOC2",
      pktNotImpl
    },
    { SP_0006_TORPEVENT,
      sizeof(sp_0006_TorpEvent_t),
      "SP_0006_TORPEVENT",
      pktNotImpl
    },
    { SP_0006_VARIABLE,
      0,                          /* these are special */
      "SP_0006_VARIABLE",
      pktNotImpl
    }
};

#define SERVERPKTMAX_0006 (sizeof(serverPackets_0006) / sizeof(packetEnt_t))

// The current protocol version
static packetEnt_t serverPackets_0007[] = {
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
      pktNotImpl
    },
    { SP_PLANETSML,
      sizeof(spPlanetSml_t),
      "SP_PLANETSML",
      pktNotImpl
    },
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
      sizeof(spcbConqInfo_t),
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

#define SERVERPKTMAX_0007 (sizeof(serverPackets_0007) / sizeof(packetEnt_t))

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

uint16_t pktGetClientProtocolVersion()
{
    return clientProtoVers;
}

/* set the desired protocol version compatibility for the client */
int pktSetClientProtocolVersion(uint16_t vers)
{
    switch (vers)
    {
    case 0x0006:
        serverPackets = serverPackets_0006;
        serverPktMax = SERVERPKTMAX_0006;
        break;

    case PROTOCOL_VERSION:      /* current protocol (0x0007) */
        serverPackets = serverPackets_0007;
        serverPktMax = SERVERPKTMAX_0007;
        break;

    default:
        utLog("%s: Unsupported protocol version: %h", __FUNCTION__, vers);
        return FALSE;
    }

    clientProtoVers = vers;

    if (cqDebug)
        utLog("%s: setting version %h", __FUNCTION__, vers);

    return procDispatchInit(vers, serverPackets, serverPktMax);
}


int pktIsConnDead(void)
{
    return connDead;
}

/* should never actually be called in normal operation */
int pktNotImpl(char *nothing)
{
    utLog("%s: ERROR: Packet Not Implemented", __FUNCTION__);
    return FALSE;
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

int pktSendAck(uint8_t severity, uint8_t code, char *msg)
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

int pktServerPacketSize(int type)
{
    if (type <= 0 || type >= serverPktMax)
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
   is data ready on the socket to be read. */
int pktSocketHasData(int sock)
{
    struct timeval timeout;
    fd_set readfds;

    if (sock < 0)
        return FALSE;

    timeout.tv_sec = 0;		/* no wait */
    timeout.tv_usec = 0;

    FD_ZERO(&readfds);

    FD_SET(sock, &readfds);

    if (select(sock + 1, &readfds, NULL, NULL, &timeout) > 0)
        return TRUE;
    else
        return FALSE;

}

/* get a packet from a ringbuffer.  If there's no data, or not
 * enough data, return 0, else return the packet type (and data)
 * this function is only suitable when reading data from a remote host.
 *
 * If update is FALSE, then we do not actually remove the data from
 * the RB.
 */
static uint8_t _pktReadGetRB(ringBuffer_t *RB, void *buf, int blen, int update)
{
    int bu, len;
    uint8_t type = 0;
    char tmppacket[PKT_MAXSIZE];
    char *packet;

    if (!blen)
        return 0;

    if (buf)
        packet = (char *)buf;
    else
        packet = tmppacket;

    if ((bu = rbBytesUsed(RB)))
    {
        /* get the first character */
        rbGet(RB, &type, 1, FALSE);

        /* clients always read server pkts, and vice versa */
        if (isClient)
            len = pktServerPacketSize(type);
        else
            len = pktClientPacketSize(type);

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
                utLog("ERROR: %s: return buffer too small, need %d, only have %d\n",
                      __FUNCTION__, len, blen);
                return 0;
            }

            /* now get the whole packet, removing it from the RB if
             *  update == TRUE
             */
            rbGet(RB, (uint8_t *)packet, len, update);

            /* we check for CP_COMMAND->CPCMD_KEEPALIVE (server-side
             * only) packets here.  If found, we remove it and just
             * return 0, indicating that no valid packet (that we
             * actually care about) is available.  Make no mistake, this
             * is a hack that should be removed at the next proto rev.
             */
            // FIXME - we need to deal with CPCMD_PING too
            if (!isClient)
            {                   /* server-side */
                if ( (type == CP_COMMAND) &&
                     (CPCMD_KEEPALIVE == (((cpCommand_t *)packet)->cmd)) )
                {
                    /* (FIXME - this sucks.  next proto rev )
                     */

                    /* if it had not already been removed from the RB
                     * above, then do it now.
                     */
                    if (!update)
                        rbGet(RB, NULL, len, TRUE);
                    return 0;
                }
            }

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
    char tmppacket[PKT_MAXSIZE];
    char *packet;

    /* if buf is NULL, we are not interested in returning the packet,
     *  just put it in the RB
     */
    if (buf)
        packet = (char *)buf;
    else
        packet = tmppacket;

    rlen = min(blen, rbBytesFree(RB));

    if ((rv = recv(sock, packet, rlen, 0)) < 0)
    {
        *packet = 0;
        utLog("%s: recv(%d): %s", __FUNCTION__, rlen, strerror(errno));
        return -1;
    }

    if (rv)
    {
        pktStats.rxBytes += rv;         /* update the counter */

        /* if buf was NULL, then we just want to put whatever we read
         * into the RB and leave.  Otherwise, we optimize by putting the
         * appropriate data in the caller's read buffer directly and
         * whatever is left over in the RB.
         */

        /* if we read enough to return a packet, and there's
         * nothing in the rb, extract it and copy any leftovers into
         * the RB.
         */
        if (buf && !rbBytesUsed(RB))
        {
            type = (uint8_t)packet[0];

            if (isClient)
                len = pktServerPacketSize(type);
            else
                len = pktClientPacketSize(type);

            if (len && rv >= len)
            {
                /* then we got enough to satify the request.
                 * convieniently, the packet is already in buf, so
                 * just put the rest in the RB :)
                 */
                if ((rv - len) > 0)
                    rbPut(RB, (uint8_t *)&packet[len], rv - len);

                return type;
            }
        }

        /* otherwise just put what we got into the RB */
        rbPut(RB, (uint8_t *)packet, rv);
    }

    return 0;
}

/* check to see if there is a packet ready.  Do a read from the
 * socket(s) first, but do not remove any packets from the RB(s)
 * (assuming one is actually there).
 *
 * FIXME Right now, this function is used only in the server to detect
 * when certain operations need to be cancelled, like destruct, bomb,
 * beam, and autopilot.  As of the current rev (808) client UDP
 * KEEPALIVE's are improperly terminating these operations, though we
 * can't simply disable KEEPALIVE's during these operations since some
 * firewalls require periodic outbound activity on UDP connections in
 * order to keep the link active.  So, we need a function like this to
 * prevent that from happening, until the next protocol revison deals
 * with these cases in a better way..
 *
 */
int pktReadPacketReady(void)
{
    int type;

    /* KEEPALIVE packets (server-side) will be filtered out by
     * _pktReadGetRB(), but we will still need check the UDP RB here so that
     * it gets the chance.
     *
     * FIXME.  This whole KEEPALIVE/PING stuff is a kludge.  Needs to be
     * reworked in the next proto rev so that this kind of 'special'
     * casing isn't required.
     */

    /* sanity check */
    if (tcp_sock < 0 && udp_sock < 0)
        return 0;

    /* do socket read(s) if possible */
    if (pktSocketHasData(tcp_sock))
        _pktReadSocket(tcp_sock, RB_TCPIn, NULL, PKT_MAXSIZE);

    if (pktSocketHasData(udp_sock))
        _pktReadSocket(udp_sock, RB_UDPIn, NULL, PKT_MAXSIZE);

    /* Check the ringbuffers, TCP first.  If there's a packet waiting
     * return it's type (>0).
     */

    if ((type = _pktReadGetRB(RB_TCPIn, NULL, PKT_MAXSIZE, FALSE)))
        return type;

    if ((type = _pktReadGetRB(RB_UDPIn, NULL, PKT_MAXSIZE, FALSE)))
        return type;

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

    if ((type = _pktReadGetRB(RB_TCPIn, buf, blen, TRUE)))
        return type;

    if ((type = _pktReadGetRB(RB_UDPIn, buf, blen, TRUE)))
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
        FD_SET(tcp_sock, &readfds);
    if (udp_sock >= 0)
        FD_SET(udp_sock, &readfds);

    maxfd = max(tcp_sock, udp_sock);

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
        {
            if ((type = _pktReadSocket(udp_sock, RB_UDPIn, buf, blen)) < 0)
            {
                /* an error */
                *buf = 0;
                utLog("%s: pktReadSocket UDP: failed", __FUNCTION__);
                return type;
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
        utLog("ERROR: %s: select(): %s", __FUNCTION__, strerror(errno));
        return rv;
    }

    /* if we're here, we try one more time on the RB's, in case some data
     * was recently read
     */
    if ((type = _pktReadGetRB(RB_TCPIn, buf, blen, TRUE)))
        return type;

    if ((type = _pktReadGetRB(RB_UDPIn, buf, blen, TRUE)))
        return type;

    return 0;
}

/* write to actual sockets */
static int _pktWriteSocket(int sock, void *data, int len)
{
    int rv;

    if (!data || !len)
        return 0;

    if ((rv = send(sock, data, len, 0)) > 0)
        pktStats.txBytes += rv;       /* update the counter */

    return rv;
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
    if ((len = rbGet(RB, (uint8_t *)buf, PKT_MAXSIZE, FALSE)))
    {
        if ((wlen = _pktWriteSocket(sock, buf, len)) <= 0)
        {
            /* log errors */
            if (wlen < 0)
                utLog("ERROR: %s: _pktWriteSocket failed: %s\n",
                      __FUNCTION__, strerror(errno));

            return wlen;
        }

        /* remove the data for good, but don't copy it again */
        rbGet(RB, NULL, wlen, TRUE);
    }

    return wlen;
}

int pktWrite(int socktype, void *data)
{
    int len;
    uint8_t type;
    int rv;
    char *packet = (char *)data;
    char *ptr;
    int sock;
    ringBuffer_t *RB;

    type = (uint8_t)*packet;	/* first byte is ALWAYS pkt type */

    if (connDead)
        return -1;

    if (socktype == PKT_SENDUDP && udp_sock >= 0)
    {
        RB   = RB_UDPOut;
        sock = udp_sock;
    }
    else
    {
        RB   = RB_TCPOut;
        sock = tcp_sock;
    }

    if (isClient)
        len = pktClientPacketSize(type);
    else
        len = pktServerPacketSize(type);

    /* try to drain the RB if needed */
    if ((rv = _pktDrainRB(sock, RB)) < 0)
        return rv;

    /* if there is already data in the RB, just add the data to it. */
    if (rbBytesUsed(RB))
    {
        if (rbBytesFree(RB) < len)
        {
            utLog("FATAL: %s@%d: output ringbuffer (%d) full, can't add %d bytes\n",
                  __FUNCTION__, __LINE__, socktype, len);
            return -1;
        }

        rbPut(RB, (uint8_t *)data, len);
    }
    else
    {
        /* try to write the data directly, and put any leftovers into the RB */
        if ((rv = _pktWriteSocket(sock, data, len)) < 0)
            return rv;

        if (len != rv)
        {
            /* there's some (or all) data left over */
            ptr = &packet[rv];

            if (rbPut(RB, (uint8_t *)ptr, len - rv) != (len - rv))
            {
                utLog("FATAL: %s@%d: output ringbuffer (%d) full, can't add %d bytes\n",
                      __FUNCTION__, __LINE__, socktype, len);
                return -1;
            }
        }
    }

    /* one last drain for old time's sake */
    if ((rv = _pktDrainRB(sock, RB)) < 0)
        return rv;

    return len;
}

/* Simply check pkt for non-NULL, and compare pkttype with packet's type */
int pktIsValid(int pkttype, void *pkt)
{
    uint8_t *p = (uint8_t *)pkt;

    if (!p)
        return FALSE;

    if (((uint8_t) *p) != pkttype)
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

void pktSetNonBlocking(int enable)
{
    nonBlocking = enable;

    return;
}
