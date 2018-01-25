/*
 * client packet proc routines (from server)
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef CPROC_H_INCLUDED
#define CPROC_H_INCLUDED

#include "conqdef.h"

#include "conqnet.h"

#include "protocol.h"

#include "packet.h"

// Get definitions for 0006 protocol
#include "cproc_0006.h"

/* packet proc routines for current protocol */
int             procUser(char *buf);
int             procShip(char *buf);
int             procShipSml(char *buf);
int             procShipLoc(char *buf);
int             procPlanet(char *buf);
int             procPlanetSml(char *buf);
int             procPlanetLoc(char *buf);
int             procPlanetLoc2(char *buf);
int             procPlanetInfo(char *buf);
int             procTorp(char *buf);
int             procTorpLoc(char *buf);
int             procTorpEvent(char *buf);
int             procTeam(char *buf);
int             procMessage(char *buf);
int             procServerStat(char *buf);
int             procConqInfo(char *buf);
int             procHistory(char *buf);
int             procDoomsday(char *buf);
int             procAck(char *buf); /* handles ACK and ACKMSG */
int             procClientStat(char *buf);
int             procFrame(char *buf);
int             procAckUDP(char *buf);

void            processPacket(char *buf);

#ifdef NOEXTERN_CPROC
/* current version 0x0007 protocol */
static dispatchProc_t cprocDispatchTable_0007[] = {
    pktNotImpl,                   /* SP_NULL */
    pktNotImpl,                   /* SP_HELLO */
    procAck,                      /* SP_ACK */
    procServerStat,               /* SP_SERVERSTAT */
    procClientStat,               /* SP_CLIENTSTAT */
    procShip,                     /* SP_SHIP */
    procShipSml,                  /* SP_SHIPSML */
    procShipLoc,                  /* SP_SHIPLOC */
    procPlanet,                   /* SP_PLANET */
    procPlanetSml,                /* SP_PLANETSML */
    procPlanetLoc,                /* SP_PLANETLOC */
    procMessage,                  /* SP_MESSAGE */
    procUser,                     /* SP_USER */
    procTorp,                     /* SP_TORP */
    procAckUDP,                   /* SP_ACKUDP */
    procTeam,                     /* SP_TEAM */
    procTorpLoc,                  /* SP_TORPLOC */
    procConqInfo,                 /* SP_CONQINFO */
    procFrame,                    /* SP_FRAME */
    procHistory,                  /* SP_HISTORY */
    procDoomsday,                 /* SP_DOOMSDAY */
    procPlanetInfo,               /* SP_PLANETINFO */
    procPlanetLoc2,               /* SP_PLANETLOC2 */
    procTorpEvent                 /* SP_TORPEVENT */
};

#define CPROCDISPATCHTABLENUM_0007                              \
    (sizeof(cprocDispatchTable_0007) / sizeof(dispatchProc_t))

#endif /* NOEXTERN_CPROC */



#endif /* CPROC_H_INCLUDED */
