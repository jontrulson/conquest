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
int             procShiptype(char *buf);

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
    procShiptype,                 /* SP_SHIPTYPE */
    procTorpEvent                 /* SP_TORPEVENT */
};

#define CPROCDISPATCHTABLENUM_0007                              \
    (sizeof(cprocDispatchTable_0007) / sizeof(dispatchProc_t))

#endif /* NOEXTERN_CPROC */



#endif /* CPROC_H_INCLUDED */
