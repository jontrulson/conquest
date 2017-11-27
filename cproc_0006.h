/*
 * client packet proc routines (from server)
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

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
int             proc_0006_ConqInfo(char *buf);
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
    proc_0006_ConqInfo,                 /* SP_CONQINFO */
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
