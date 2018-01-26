//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


#ifndef _SERVERPKT_H
#define _SERVERPKT_H

#include "conqdef.h"
#include "conqnet.h"

#include "protocol.h"
#include "packet.h"

// false == success, true == failure
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

spTorp_t       *spktTorp(uint8_t tsnum, uint8_t tnum, int rec);
spTorpLoc_t    *spktTorpLoc(uint8_t tsnum, uint8_t tnum, int rec);
spTorpEvent_t  *spktTorpEvent(uint8_t tsnum, uint8_t tnum, int rec);

spTeam_t       *spktTeam(uint8_t team, int force, int rec);
spcbConqInfo_t   *spktcbConqInfo(int force);

spHistory_t    *spktHistory(int hnum);
spDoomsday_t   *spktDoomsday(int rec);

spPlanetInfo_t *spktPlanetInfo(uint8_t pnum, int rec);

#endif /* _SERVERPKT_H */
