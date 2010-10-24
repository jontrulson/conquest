/* 
 * client packet proc routines (from server)
 *
 * $Id$
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

void            processPacket(char *buf);

#endif /* CPROC_H_INCLUDED */
