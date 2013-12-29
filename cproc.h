/* 
 * client packet proc routines (from server)
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef CPROC_H_INCLUDED
#define CPROC_H_INCLUDED

#include "conqdef.h"
#include "datatypes.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"

/* packet proc routines */
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

void            processPacket(char *buf);

#ifdef NOEXTERN_CPROC
/* version 0x0006 protocol */
static dispatchProc_t cprocDispatchTable_0006[] = {
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
  procAck,                      /* SP_ACKMSG */
  procTeam,                     /* SP_TEAM */
  procTorpLoc,                  /* SP_TORPLOC */
  procConqInfo,                 /* SP_CONQINFO */
  procFrame,                    /* SP_FRAME */
  procHistory,                  /* SP_HISTORY */
  procDoomsday,                 /* SP_DOOMSDAY */
  procPlanetInfo,               /* SP_PLANETINFO */
  procPlanetLoc2,               /* SP_PLANETLOC2 */
  procTorpEvent,                /* SP_TORPEVENT */
  pktNotImpl                    /* SP_VARIABLE */
};

#define CPROCDISPATCHTABLENUM_0006 \
  (sizeof(cprocDispatchTable_0006) / sizeof(dispatchProc_t))

#endif /* NOEXTERN_CPROC */



#endif /* CPROC_H_INCLUDED */
