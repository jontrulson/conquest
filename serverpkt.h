/*
 * packet test/generation for the server
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _SERVERPKT_H
#define _SERVERPKT_H

#include "conqdef.h"
#include "conqnet.h"

#include "protocol.h"
#include "packet.h"


void           spktInit(void);
void           spktInitPkt(void);
void           spktInitRec(void);

spUser_t       *spktUser(uint16_t unum);

spShip_t       *spktShip(uint8_t snum, int rec);
spShipSml_t    *spktShipSml(uint8_t snum, int rec);
spShipLoc_t    *spktShipLoc(uint8_t snum, int rec);

spPlanet_t     *spktPlanet(uint8_t pnum, int rec);
spPlanetSml_t  *spktPlanetSml(uint8_t pnum, int rec);
spPlanetLoc_t  *spktPlanetLoc(uint8_t pnum, int rec, int force);
spPlanetLoc2_t *spktPlanetLoc2(uint8_t pnum, int rec, int force);

spTorp_t       *spktTorp(uint8_t tsnum, uint8_t tnum, int rec);
spTorpLoc_t    *spktTorpLoc(uint8_t tsnum, uint8_t tnum, int rec);
spTorpEvent_t  *spktTorpEvent(uint8_t tsnum, uint8_t tnum, int rec);

spTeam_t       *spktTeam(uint8_t team, int force, int rec);
spConqInfo_t   *spktConqInfo(int force);

spHistory_t    *spktHistory(int hnum);
spDoomsday_t   *spktDoomsday(int rec);

spPlanetInfo_t *spktPlanetInfo(uint8_t pnum, int rec);

#endif /* _SERVERPKT_H */
