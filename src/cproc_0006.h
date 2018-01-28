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


#ifndef CPROC_0006_H_INCLUDED
#define CPROC_0006_H_INCLUDED

/* packet proc routines for 0x0006 protocol */
int             proc_0006_User(char *buf);
int             proc_0006_Ship(char *buf);
int             proc_0006_ShipSml(char *buf);
int             proc_0006_ShipLoc(char *buf);
int             proc_0006_Planet(char *buf);
int             proc_0006_PlanetSml(char *buf);
int             proc_0006_PlanetLoc(char *buf);
int             proc_0006_PlanetLoc2(char *buf);
int             proc_0006_PlanetInfo(char *buf);
int             proc_0006_Torp(char *buf);
int             proc_0006_TorpLoc(char *buf);
int             proc_0006_TorpEvent(char *buf);
int             proc_0006_Team(char *buf);
int             proc_0006_Message(char *buf);
int             proc_0006_ServerStat(char *buf);
int             proc_0006_cbConqInfo(char *buf);
int             proc_0006_History(char *buf);
int             proc_0006_Doomsday(char *buf);
int             proc_0006_Ack(char *buf); /* handles ACK and ACKMSG */
int             proc_0006_ClientStat(char *buf);
int             proc_0006_Frame(char *buf);

#ifdef NOEXTERN_CPROC

/* version 0x0006 protocol */
static dispatchProc_t cprocDispatchTable_0006[] = {
    pktNotImpl,                   /* SP_NULL */
    pktNotImpl,                   /* SP_HELLO */
    proc_0006_Ack,                      /* SP_ACK */
    proc_0006_ServerStat,               /* SP_SERVERSTAT */
    proc_0006_ClientStat,               /* SP_CLIENTSTAT */
    proc_0006_Ship,                     /* SP_SHIP */
    proc_0006_ShipSml,                  /* SP_SHIPSML */
    proc_0006_ShipLoc,                  /* SP_SHIPLOC */
    proc_0006_Planet,                   /* SP_PLANET */
    proc_0006_PlanetSml,                /* SP_PLANETSML */
    proc_0006_PlanetLoc,                /* SP_PLANETLOC */
    proc_0006_Message,                  /* SP_MESSAGE */
    proc_0006_User,                     /* SP_USER */
    proc_0006_Torp,                     /* SP_TORP */
    proc_0006_Ack,                      /* SP_ACKMSG */
    proc_0006_Team,                     /* SP_TEAM */
    proc_0006_TorpLoc,                  /* SP_TORPLOC */
    proc_0006_cbConqInfo,                 /* SP_CONQINFO */
    proc_0006_Frame,                    /* SP_FRAME */
    proc_0006_History,                  /* SP_HISTORY */
    proc_0006_Doomsday,                 /* SP_DOOMSDAY */
    proc_0006_PlanetInfo,               /* SP_PLANETINFO */
    proc_0006_PlanetLoc2,               /* SP_PLANETLOC2 */
    proc_0006_TorpEvent,                /* SP_TORPEVENT */
    pktNotImpl                    /* SP_VARIABLE */
};

#define CPROCDISPATCHTABLENUM_0006                              \
    (sizeof(cprocDispatchTable_0006) / sizeof(dispatchProc_t))

#endif // NOEXTERN_CPROC


#endif /* CPROC_0006_H_INCLUDED */
