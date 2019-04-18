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


#ifndef PROTO_0006_H_INCLUDED
#define PROTO_0006_H_INCLUDED

#include "conqdef.h"


/* Protocol version 0006 compatibility (cqr only).  We do this to
 * allow the playback of older .cqr game recordings using older
 * protocols.  For this reason, we hardcode any array sizes to the
 * value it was at when the protocol was changed.  We also discard any
 * client (cp/CP) packet information since those are never recorded.
 */

/* server -> client packet types */

#define SP_0006_NULL           0	/* never used */
#define SP_0006_HELLO          1
#define SP_0006_ACK            2
#define SP_0006_SERVERSTAT     3
#define SP_0006_CLIENTSTAT     4
#define SP_0006_SHIP           5
#define SP_0006_SHIPSML        6
#define SP_0006_SHIPLOC        7
#define SP_0006_PLANET         8
#define SP_0006_PLANETSML      9
#define SP_0006_PLANETLOC      10
#define SP_0006_MESSAGE        11
#define SP_0006_USER           12
#define SP_0006_TORP           13
#define SP_0006_ACKMSG         14
#define SP_0006_TEAM           15
#define SP_0006_TORPLOC        16
#define SP_0006_CONQINFO       17
#define SP_0006_FRAME          18    /* used in recording only */
#define SP_0006_HISTORY        19
#define SP_0006_DOOMSDAY       20

#define SP_0006_PLANETINFO     21    /* planet's orbital characteristics */

#define SP_0006_PLANETLOC2     22    /* new planloc packet */

#define SP_0006_TORPEVENT      23    /* torp launch/status */

#define SP_0006_VARIABLE       24    /* variable length packet */

/* client -> server packet types removed */

#pragma pack(1)                 /* show me a better way... */

/* server -> client packets */
typedef struct {
    uint8_t type;			/* SP_0006_NULL */
    uint8_t pad1;
    uint8_t pad2;
    uint8_t pad3;
} sp_0006_Null_t;

#define SP_0006_HELLO_FLAGS_NONE         0x00
#define SP_0006_HELLO_FLAGS_CLOSED       0x01	/* game closed */
typedef struct {
    uint8_t  type;			/* SP_0006_HELLO */
    uint8_t  flags;                /* closed, etc... */
    uint16_t protover;		/* protocol rev */

    uint32_t cmnrev;		/* common block version */

    uint32_t pad1;

    /* see conf.h for SERVER_*_SZ */
    uint8_t  servername[70 /*CONF_SERVER_NAME_SZ*/]; /* long server name */

    uint8_t  serverver[70 /*CONF_SERVER_NAME_SZ*/]; /* server's version */

    uint8_t  motd[70 /*CONF_SERVER_MOTD_SZ*/];	/* MOTD... */
} sp_0006_Hello_t;

/* ACK without message text */
typedef struct {
    uint8_t type;			/* SP_0006_ACK */
    uint8_t severity;		/* none, info, warning, etc */
    uint8_t code;			/* ack code */
    uint8_t pad1;

} sp_0006_Ack_t;

/* ACK with message text */
typedef struct {
    uint8_t type;			/* SP_0006_ACKMSG */
    uint8_t severity;		/* none, info, warning, etc */
    uint8_t code;			/* ack code */
    uint8_t pad1;

    uint8_t txt[70 /*MAX_MSGSIZE*/];	/* text */
} sp_0006_AckMsg_t;

#define SP_0006_SSTAT_FLAGS_NONE        0x00000000
#define SP_0006_SSTAT_FLAGS_REFIT       0x00000001 /* allow refits? */
#define SP_0006_SSTAT_FLAGS_VACANT      0x00000002 /* allow vacant ships? */
#define SP_0006_SSTAT_FLAGS_SLINGSHOT   0x00000004 /* 'slingshot' bug enabled? */
#define SP_0006_SSTAT_FLAGS_NODOOMSDAY  0x00000008 /* no doomsday allowed? */
#define SP_0006_SSTAT_FLAGS_KILLBOTS    0x00000010 /* robots have randomized kills? */
#define SP_0006_SSTAT_FLAGS_SWITCHTEAM  0x00000020 /* allowed to switch teams? */

typedef struct {
    uint8_t  type;                 /* SP_0006_SERVSTAT */
    uint8_t  numactive;		/* active ships */
    uint8_t  numvacant;		/* vacant ships */
    uint8_t  numrobot;		/* robots active */

    uint8_t  numtotal;		/* total number of ships */
    uint8_t  pad1;
    uint16_t numusers;		/* total number of users */

    uint32_t flags;		/* certain operating modes/options */

    uint32_t servertime;		/* the server's time */
} sp_0006_ServerStat_t;

#define SP_0006_CLNTSTAT_FLAG_NONE    0x00
#define SP_0006_CLNTSTAT_FLAG_NEW     0x01
#define SP_0006_CLNTSTAT_FLAG_CONQUER 0x02 /* you conquered the universe. send a
					message (lastwords) */
#define SP_0006_CLNTSTAT_FLAG_KILLED  0x04 /* you were killed by something */

typedef struct {
    uint8_t  type;			/* SP_0006_CLIENTSTAT */
    uint8_t  flags;                /* LR/SR, live, dead, etc. */
    uint8_t  snum;			/* your ship number */
    uint8_t  team;			/* your team number */

    uint16_t unum;			/* your user number */
    uint8_t  esystem;		/* team bitmask of systems
				   you can enter from when entering
				   the game. 0 if only one available */
    uint8_t  pad2;
} sp_0006_ClientStat_t;

typedef struct {	/* 'large' ship data */
    uint8_t  type;			/* SP_0006_SHIP */
    uint8_t  status;		/* one of the SS_ values */
    uint8_t  snum;			/* ship num */
    uint8_t  team;

    uint16_t unum;			/* user num */
    uint8_t  shiptype;
    uint8_t  towing;

    uint8_t  towedby;
    uint8_t  war;
    uint8_t  rwar;
    uint8_t  pad1;

    int16_t   killedby;		/* depends on status - keep together in pkt */
    int16_t   pad2;

    uint32_t kills;		/* x10 */

    uint8_t  srpwar[40 + 20 /*MAXPLANETS*/ + 1];

    uint8_t  scanned[4 /*NUM_PLAYERTEAMS*/];

    uint8_t  alias[24 /*MAXUSERALIAS*/];
} sp_0006_Ship_t;

typedef struct {	/* 'short' ship packets */
    uint8_t  type;			/* SP_0006_SHIPSML */
    uint8_t  snum;			/* ship num */
    uint8_t  action;               /* ROB_* for robot/autopilot */
    uint8_t  shields;		/* 0-100 */

    uint8_t  damage;		/* 0-100 */
    uint8_t  armies;
    uint8_t  walloc;
    int8_t    pfuse;

    uint8_t  etemp;
    uint8_t  wtemp;
    uint16_t fuel;

    int16_t   sdfuse;
    int8_t    wfuse;
    int8_t    efuse;

    int16_t   lock;
    int16_t   flags;                /* SHIP_F_* */

    uint16_t lastphase;
    uint16_t lastblast;
} sp_0006_ShipSml_t;

typedef struct {	/* ship location packets */
    uint8_t  type;			/* SP_0006_SHIPLOC */
    uint8_t  snum;			/* ship num */
    uint16_t head;			/* x10 */

    int8_t   warp;			/* x10 */
    uint8_t pad1;
    uint8_t pad2;
    uint8_t pad3;

    int32_t  x;			/* x1000 */

    int32_t  y;			/* x1000 */
} sp_0006_ShipLoc_t;

typedef struct {
    uint8_t  type;			/* SP_0006_PLANET */
    uint8_t  pnum;			/* planet number */
    uint8_t  ptype;                /* planet type */
    uint8_t  team;			/* team */

    uint8_t  name[12 /*MAX_PLANETNAME*/];
} sp_0006_Planet_t;

typedef struct {
    uint8_t  type;			/* SP_0006_PLANETSML */
    uint8_t  pnum;			/* planet number */
    uint8_t  scanned;		/* bitmask of teams that have scanned */
    uint8_t  uninhabtime;
} sp_0006_PlanetSml_t;

typedef struct {
    uint8_t  type;                 /* SP_0006_PLANETLOC */
    uint8_t  pnum;
    int16_t   armies;

    int32_t   x;			/* x1000 */

    int32_t   y;			/* x1000 */
} sp_0006_PlanetLoc_t;

typedef struct {
    uint8_t  type;                  /* SP_0006_PLANETLOC2 */
    uint8_t  pnum;
    int16_t   armies;

    int32_t   x;			/* x1000 */

    int32_t   y;			/* x1000 */

    uint16_t orbang;               /* x100 */

} sp_0006_PlanetLoc2_t;


#define SP_0006_PLANETINFO_FLAGS_NONE       0x00
#define SP_0006_PLANETINFO_FLAGS_FVALID     0x01 /* flags are valid */
#define SP_0006_PLANETINFO_FLAGS_REAL       0x02 /* planet is real (visible) */

typedef struct {
    uint8_t  type;			/* SP_0006_PLANETINFO */
    uint8_t  pnum;			/* planet number */
    uint8_t  primary;
    uint8_t  flags;

    uint32_t orbrad;               /* x10 */

    int32_t   orbvel;               /* x100 */
} sp_0006_PlanetInfo_t;

typedef struct {
    uint8_t  type;                 /* SP_0006_MESSAGE */
    uint8_t  flags;                /* see msg.h */
    uint8_t  pad2;
    uint8_t  pad3;

    int16_t   from;
    int16_t   to;

    uint8_t  msg[70 /*MAX_MSGSIZE*/];
} sp_0006_Message_t;

#define SP_0006_USER_FLAGS_NONE     0x00
#define SP_0006_USER_FLAGS_LIVE     0x01
typedef struct {
    uint8_t  type;			/* SP_0006_USER */
    uint8_t  team;
    uint16_t unum;			/* user num */

    uint8_t  flags;                /* hostile, live, etc */
    uint8_t  war;			/* a bit mask (1 << TEAM_*) */
    int16_t   rating;               /* x100 */

    uint32_t lastentry;

    int32_t   ooptions[7 /*OOPT_TOTALOOPTION*/];

    int32_t   stats[14 /*USTAT_TOTALSTATS*/];

    uint8_t  username[32 /*MAX_USERNAME*/];

    uint8_t  alias[24 /*MAXUSERALIAS*/];
} sp_0006_User_t;

typedef struct {
    uint8_t  type;			/* SP_0006_TORP */
    uint8_t  snum;			/* ship num */
    uint8_t  tnum;			/* torp num */
    uint8_t  status;		/* one of the TS_* values */
} sp_0006_Torp_t;

typedef struct {
    uint8_t  type;                 /* SP_0006_TORPLOC */
    uint8_t  snum;                 /* ship num */
    uint8_t  tnum;                 /* torp num */
    uint8_t  war;                  /* war. what is it good for. */

    int32_t   x;			/* x1000 */

    int32_t   y;			/* x1000 */
} sp_0006_TorpLoc_t;

typedef struct {
    uint8_t  type;                 /* SP_0006_TORPEVENT */
    uint8_t  snum;                 /* ship num */
    uint8_t  tnum;                 /* torp num */
    uint8_t  war;                  /* war. what is it good for. */

    int32_t   x;			/* x1000 */

    int32_t   y;			/* x1000 */

    int32_t   dx;			/* x1000 */

    int32_t   dy;			/* x1000 */


    uint8_t  status;		/* one of the TS_* values */
    uint8_t  pad1;
    uint8_t  pad2;
    uint8_t  pad3;

} sp_0006_TorpEvent_t;

#define SP_0006_TEAM_FLAGS_NONE         0x00
#define SP_0006_TEAM_FLAGS_COUPINFO     0x01
typedef struct {
    uint8_t  type;                 /* SP_0006_TEAM */
    uint8_t  team;
    uint8_t  homesun;
    uint8_t  flags;                /* coupinfo, etc */

    uint8_t  couptime;		/* minutes until coup can work */
    uint8_t  teamhplanets[3];

    uint8_t  homeplanet;
    uint8_t  pad1;
    uint16_t pad2;

    uint32_t stats[20 /*MAX_TEAM_STATS*/];

    uint8_t  name[12 /*MAX_TEAMNAME*/];
} sp_0006_Team_t;

typedef struct {
    uint8_t  type;                 /* SP_0006_CONQINFO */
    uint8_t  pad1;
    uint8_t  pad2;
    uint8_t  pad3;

    uint8_t  conqueror[24 /*MAXUSERALIAS*/]; /* user who last conquered */
    uint8_t  conqteam[12 /*MAX_TEAMNAME*/];   /* team s/he was on */
    uint8_t  conqtime[20 /*MAX_DATESIZE*/];      /* time of last conquering */
    uint8_t  lastwords[60 /*MAX_LASTWORDS*/]; /* remarks upon conquering */
} sp_0006_cbConqInfo_t;

/* only stored in recordings (.cqr) files */
typedef struct {
    uint8_t  type;			/* SP_0006_FRAME */
    uint8_t  pad1;
    uint8_t  pad2;
    uint8_t  pad3;

    uint32_t frame;                /* frame number */

    uint32_t time;                 /* timestamp */

} sp_0006_Frame_t;

typedef struct {
    uint8_t  type;			/* SP_0006_HISTORY */
    uint8_t  histptr;              /* current hist ptr */
    uint8_t  hnum;
    uint8_t  pad1;

    uint16_t unum;
    uint16_t pad2;

    uint32_t elapsed;

    uint32_t histlog;
} sp_0006_History_t;

typedef struct {
    uint8_t  type;			/* SP_0006_DOOMSDAY */
    uint8_t  status;               /* DS_* */
    uint16_t heading;              /* x10 */

    int32_t   x;			/* x1000 */
    int32_t   y;			/* x1000 */
} sp_0006_Doomsday_t;


/* client -> server packets (deleted) */

#pragma pack()                  /* restore dflt struct alignment */

#endif /* PROTO_0006_H_INCLUDED */
