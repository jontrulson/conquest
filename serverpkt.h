/* 
 * packet test/generation for the server
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _SERVERPKT_H
#define _SERVERPKT_H

#include "conqdef.h"
#include "conqnet.h"
#include "datatypes.h"
#include "protocol.h"
#include "packet.h"


void spktInit(void);
void spktInitPkt(void);
void spktInitRec(void);

spUser_t *spktUser(Unsgn16 unum);

spShip_t *spktShip(Unsgn8 snum, int rec);
spShipSml_t *spktShipSml(Unsgn8 snum, int rec);
spShipLoc_t *spktShipLoc(Unsgn8 snum, int rec);

spPlanet_t *spktPlanet(Unsgn8 pnum, int rec);
spPlanetSml_t *spktPlanetSml(Unsgn8 pnum, int rec);
spPlanetLoc_t *spktPlanetLoc(Unsgn8 pnum, int rec);

spTorp_t *spktTorp(Unsgn8 tsnum, Unsgn8 tnum, int rec);
spTorpLoc_t *spktTorpLoc(Unsgn8 tsnum, Unsgn8 tnum, int rec);

spTeam_t *spktTeam(Unsgn8 team, int force, int rec);
spConqInfo_t *spktConqInfo(int force);

spHistory_t *spktHistory(int hnum);
spDoomsday_t *spktDoomsday(int rec);
#endif /* _SERVERPKT_H */
