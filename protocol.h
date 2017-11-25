/*
 * The protocol definition
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include "conqdef.h"
#include "conqcom.h"
// version 0006 protocol defs
#include "proto_0006.h"

// 11/8/17 - PROTOCOL_VERSION  0x0006
#define PROTOCOL_VERSION  0x0007


/* server -> client packet types */

#define SP_NULL           0	/* never used */
#define SP_HELLO          1
#define SP_ACK            2
#define SP_SERVERSTAT     3
#define SP_CLIENTSTAT     4
#define SP_SHIP           5
#define SP_SHIPSML        6
#define SP_SHIPLOC        7
#define SP_PLANET         8
#define SP_PLANETSML      9
#define SP_PLANETLOC      10
#define SP_MESSAGE        11
#define SP_USER           12
#define SP_TORP           13
#define SP_ACKMSG         14
#define SP_TEAM           15
#define SP_TORPLOC        16
#define SP_CONQINFO       17
#define SP_FRAME          18    /* used in recording only */
#define SP_HISTORY        19
#define SP_DOOMSDAY       20

#define SP_PLANETINFO     21    /* planet's orbital characteristics */

#define SP_PLANETLOC2     22    /* new planloc packet */

#define SP_TORPEVENT      23    /* torp launch/status */

#define SP_VARIABLE       24    /* variable length packet */

/* client -> server packet types */

#define CP_NULL           0	/* never used */
#define CP_HELLO          1
#define CP_ACK            2
#define CP_COMMAND        3	/* most commands handled by this */
#define CP_FIRETORPS      4
#define CP_GETSINFO       5	/* get special ship info ('ne', 'ns', etc)*/
#define CP_SENDMSG        6
#define CP_SETNAME        7
#define CP_AUTHENTICATE   8
#define CP_SETCOURSE      9
#define CP_MESSAGE        10

#define CP_VARIABLE       11    /* variable length packet */

#define PKT_ANYPKT        ~0


#pragma pack(1)                 /* show me a better way... */

/* a variable length packet for future use, and to help
   with protocol compatibility in the future. Both server and
   client use the same format */

typedef struct {
    uint8_t type;                  /* SP_VARIABLE | CP_VARIABLE */
    uint8_t len;
    uint16_t pad1;

    /* the actual data would be written following this */
} pktVariable_t;

/* server -> client packets */
typedef struct {
    uint8_t type;			/* SP_NULL */
    uint8_t pad1;
    uint8_t pad2;
    uint8_t pad3;
} spNull_t;

#define SPHELLO_FLAGS_NONE         0x00
#define SPHELLO_FLAGS_CLOSED       0x01	/* game closed */
typedef struct {
    uint8_t  type;			/* SP_HELLO */
    uint8_t  flags;                /* closed, etc... */
    uint16_t protover;		/* protocol rev */

    uint32_t cmnrev;		/* common block version */

    uint32_t pad1;

    /* see conf.h for SERVER_*_SZ */
    uint8_t  servername[CONF_SERVER_NAME_SZ]; /* long server name */

    uint8_t  serverver[CONF_SERVER_NAME_SZ]; /* server's version */

    uint8_t  motd[CONF_SERVER_MOTD_SZ];	/* MOTD... */
} spHello_t;

/* ACK without message text */
typedef struct {
    uint8_t type;			/* SP_ACK */
    uint8_t severity;		/* none, info, warning, etc */
    uint8_t code;			/* ack code */
    uint8_t pad1;

} spAck_t;

/* ACK with message text */
typedef struct {
    uint8_t type;			/* SP_ACKMSG */
    uint8_t severity;		/* none, info, warning, etc */
    uint8_t code;			/* ack code */
    uint8_t pad1;

    uint8_t txt[MESSAGE_SIZE];	/* text */
} spAckMsg_t;

#define SPSSTAT_FLAGS_NONE        0x00000000
#define SPSSTAT_FLAGS_REFIT       0x00000001 /* allow refits? */
#define SPSSTAT_FLAGS_VACANT      0x00000002 /* allow vacant ships? */
#define SPSSTAT_FLAGS_SLINGSHOT   0x00000004 /* 'slingshot' bug enabled? */
#define SPSSTAT_FLAGS_NODOOMSDAY  0x00000008 /* no doomsday allowed? */
#define SPSSTAT_FLAGS_KILLBOTS    0x00000010 /* robots have randomized kills? */
#define SPSSTAT_FLAGS_SWITCHTEAM  0x00000020 /* allowed to switch teams? */

typedef struct {
    uint8_t  type;                 /* SP_SERVSTAT */
    uint8_t  numactive;		/* active ships */
    uint8_t  numvacant;		/* vacant ships */
    uint8_t  numrobot;		/* robots active */

    uint8_t  numtotal;		/* total number of ships */
    uint8_t  pad1;
    uint16_t numusers;		/* total number of users */

    uint32_t flags;		/* certain operating modes/options */

    uint32_t servertime;		/* the server's time */
} spServerStat_t;

#define SPCLNTSTAT_FLAG_NONE    0x00
#define SPCLNTSTAT_FLAG_NEW     0x01
#define SPCLNTSTAT_FLAG_CONQUER 0x02 /* you conquered the universe. send a
					message (lastwords) */
#define SPCLNTSTAT_FLAG_KILLED  0x04 /* you were killed by something */

typedef struct {
    uint8_t  type;			/* SP_CLIENTSTAT */
    uint8_t  flags;                /* LR/SR, live, dead, etc. */
    uint8_t  snum;			/* your ship number */
    uint8_t  team;			/* your team number */

    uint16_t unum;			/* your user number */
    uint8_t  esystem;		/* team bitmask of systems
				   you can enter from when entering
				   the game. 0 if only one available */
    uint8_t  pad2;
} spClientStat_t;

typedef struct {	/* 'large' ship data */
    uint8_t  type;			/* SP_SHIP */
    uint8_t  status;		/* one of the SS_ values */
    uint8_t  snum;			/* ship num */
    uint8_t  team;

    uint16_t unum;			/* user num */
    uint8_t  shiptype;
    uint8_t  towing;

    uint8_t  towedby;
    uint8_t  war;
    uint8_t  rwar;
    uint8_t  killedBy;		/* depends on status - keep together
                                 * in pkt, along with killedByDetail */

    uint8_t  pad1;
    uint8_t  pad2;
    uint16_t killedByDetail;

    uint32_t kills;		/* x10 */

    uint8_t  srpwar[MAXPLANETS];

    uint8_t  scanned[NUMPLAYERTEAMS];

    uint8_t  alias[MAXUSERNAME];
} spShip_t;

typedef struct {	/* 'short' ship packets */
    uint8_t  type;			/* SP_SHIPSML */
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

    uint8_t   lock;
    uint8_t   pad1;
    uint8_t   pad2;
    uint8_t   pad3;

    uint16_t  lockDetail;
    int16_t   flags;                /* SHIP_F_* */

    uint16_t lastphase;
    uint16_t lastblast;
} spShipSml_t;

typedef struct {	/* ship location packets */
    uint8_t  type;			/* SP_SHIPLOC */
    uint8_t  snum;			/* ship num */
    uint16_t head;			/* x10 */

    int8_t   warp;			/* x10 */
    uint8_t pad1;
    uint8_t pad2;
    uint8_t pad3;

    int32_t  x;			/* x1000 */

    int32_t  y;			/* x1000 */
} spShipLoc_t;

typedef struct {
    uint8_t  type;			/* SP_PLANET */
    uint8_t  pnum;			/* planet number */
    uint8_t  ptype;                /* planet type */
    uint8_t  team;			/* team */

    uint8_t  defendteam;        /* original home planet team owner*/
    uint8_t  pad[3];

    uint8_t  name[MAXPLANETNAME];
} spPlanet_t;

typedef struct {
    uint8_t  type;			/* SP_PLANETSML */
    uint8_t  pnum;			/* planet number */
    uint8_t  scanned;		/* bitmask of teams that have scanned */
    uint8_t  uninhabtime;
} spPlanetSml_t;

typedef struct {
    uint8_t  type;                 /* SP_PLANETLOC */
    uint8_t  pnum;
    int16_t   armies;

    int32_t   x;			/* x1000 */

    int32_t   y;			/* x1000 */
} spPlanetLoc_t;

typedef struct {
    uint8_t  type;                  /* SP_PLANETLOC2 */
    uint8_t  pnum;
    int16_t   armies;

    int32_t   x;			/* x1000 */

    int32_t   y;			/* x1000 */

    uint16_t orbang;               /* x100 */

} spPlanetLoc2_t;

typedef struct {
    uint8_t  type;			/* SP_PLANETINFO */
    uint8_t  pnum;			/* planet number */
    uint8_t  primary;
    uint8_t  pad;

    uint32_t flags;
    uint32_t orbrad;               /* x10 */

    int32_t   orbvel;               /* x100 */
} spPlanetInfo_t;

typedef struct {
    uint8_t  type;                 /* SP_MESSAGE */
    uint8_t  flags;                /* see msg.h */
    uint8_t  from;
    uint8_t  to;

    uint16_t fromDetail;        /* ship/planet/team number, etc */
    uint16_t toDetail;

    uint8_t  msg[MESSAGE_SIZE];
} spMessage_t;

typedef struct {
    uint8_t  type;			/* SP_USER */
    uint8_t  team;
    uint16_t unum;			/* user num */

    uint16_t flags;                /* user flags */
    int16_t  rating;               /* x10 */

    uint8_t  userType;          // USERTYPE_* */
    uint8_t  war;			/* a bit mask (1 << TEAM_*) */
    uint16_t opFlags;                   /* operator set flags */

    uint32_t lastentry;

    int32_t  stats[USTAT_TOTALSTATS];

    uint8_t  username[MAXUSERNAME];

    uint8_t  alias[MAXUSERNAME];
} spUser_t;

typedef struct {
    uint8_t  type;			/* SP_TORP */
    uint8_t  snum;			/* ship num */
    uint8_t  tnum;			/* torp num */
    uint8_t  status;		/* one of the TS_* values */
} spTorp_t;

typedef struct {
    uint8_t  type;                 /* SP_TORPLOC */
    uint8_t  snum;                 /* ship num */
    uint8_t  tnum;                 /* torp num */
    uint8_t  war;                  /* war. what is it good for. */

    int32_t   x;			/* x1000 */

    int32_t   y;			/* x1000 */
} spTorpLoc_t;

typedef struct {
    uint8_t  type;                 /* SP_TORPEVENT */
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

} spTorpEvent_t;

#define SPTEAM_FLAGS_NONE         0x00
#define SPTEAM_FLAGS_COUPINFO     0x01
typedef struct {
    uint8_t  type;                 /* SP_TEAM */
    uint8_t  team;
    uint8_t  couptime;		/* minutes until coup can work */
    uint8_t  flags;                /* coupinfo, etc */

    uint8_t  homeplanet;
    uint8_t  pad[3];

    uint32_t stats[MAXTSTATS];

    uint8_t  name[MAXTEAMNAME];
} spTeam_t;

typedef struct {
    uint8_t  type;                 /* SP_CONQINFO */
    uint8_t  pad1;
    uint8_t  pad2;
    uint8_t  pad3;

    uint8_t  conqueror[MAXUSERNAME]; /* user who last conquered */
    uint8_t  conqteam[MAXTEAMNAME];   /* team s/he was on */
    uint8_t  conqtime[DATESIZE];      /* time of last conquering */
    uint8_t  lastwords[MAXLASTWORDS]; /* remarks upon conquering */
} spConqInfo_t;

/* only stored in recordings (.cqr) files */
typedef struct {
    uint8_t  type;			/* SP_FRAME */
    uint8_t  pad1;
    uint8_t  pad2;
    uint8_t  pad3;

    uint32_t frame;                /* frame number */

    uint32_t time;                 /* timestamp */

} spFrame_t;

typedef struct {
    uint8_t  type;			/* SP_HISTORY */
    uint8_t  histptr;              /* current hist ptr */
    uint8_t  hnum;
    uint8_t  pad1;

    uint16_t unum;
    uint16_t pad2;

    uint32_t elapsed;

    uint32_t enterTime;

    uint8_t  username[MAXUSERNAME];
} spHistory_t;

typedef struct {
    uint8_t  type;			/* SP_DOOMSDAY */
    uint8_t  status;               /* DS_* */
    uint16_t heading;              /* x10 */

    int32_t   x;			/* x1000 */
    int32_t   y;			/* x1000 */
} spDoomsday_t;


/* client -> server packets */
typedef struct {	/* never used */
    uint8_t  type;			/* CP_NULL */
    uint8_t  pad1;
    uint8_t  pad2;
    uint8_t  pad3;
} cpNull_t;

typedef struct {
    uint8_t  type;			/* CP_HELLO */
    uint8_t  updates;		/* desired updates per sec */
    uint16_t protover;		/* protocol rev */

    uint32_t cmnrev;		/* common block version */

    uint8_t  clientname[CONF_SERVER_NAME_SZ]; /* "ConquestGL", etc */

    uint8_t  clientver[CONF_SERVER_NAME_SZ];	/* version, misc info */
} cpHello_t;

typedef struct {
    uint8_t  type;			/* CP_ACK */
    uint8_t  severity;		/* none, info, warning, etc */
    uint8_t  code;			/* ack code */
    uint8_t  pad1;
} cpAck_t;

/* cmd values */
#define CPCMD_NULL         0	/* never used */
#define CPCMD_SETRATE      1
#define CPCMD_SETWARP      2
#define CPCMD_ALLOC        3
#define CPCMD_BEAM         4
#define CPCMD_CLOAK        5
#define CPCMD_DETENEMY     6
#define CPCMD_DETSELF      7
#define CPCMD_DISTRESS     8
#define CPCMD_FIREPHASER   9
#define CPCMD_RESIGN       10
#define CPCMD_BOMB         11
#define CPCMD_ORBIT        12
#define CPCMD_DESTRUCT     13
#define CPCMD_REFIT        14
#define CPCMD_REPAIR       15
#define CPCMD_TRACTOR      16
#define CPCMD_UNTRACTOR    17
#define CPCMD_SETWAR       18
#define CPCMD_SETSHIELDS   19
#define CPCMD_ENTER        20
#define CPCMD_SWITCHTEAM   21
#define CPCMD_DISCONNECT   22
#define CPCMD_TOW          23
#define CPCMD_UNTOW        24
#define CPCMD_COUP         25
#define CPCMD_AUTOPILOT    26
#define CPCMD_RELOAD       27
#define CPCMD_PING         28
#define CPCMD_KEEPALIVE    29   /* udp keepalive */

#define CPCMD_LAST         29

typedef struct {
    uint8_t  type;			/* CP_COMMAND */
    uint8_t  cmd;			/* one of CPCMD_* */
    uint16_t detail;		/* command specific */
} cpCommand_t;

typedef struct {
    uint8_t  type;			/* CP_FIRETORPS */
    uint8_t  num;			/* number to fire */
    uint16_t dir;			/* x100 */
} cpFireTorps_t;

#define CP_SINFOMAX  15
typedef struct {
    uint8_t  type;			/* CP_GETSINFO */
    uint8_t  info[CP_SINFOMAX];
} cpGetSInfo_t;

typedef struct {
    uint8_t  type;			/* CP_SENDMSG */
    uint8_t  pad1;
    int16_t   to;

    uint8_t  msg[MESSAGE_SIZE];
} cpSendMsg_t;

typedef struct {
    uint8_t  type;			/* CP_SETNAME */
    uint8_t  alias[MAXUSERNAME];
} cpSetName_t;

/* authenticate flags */
#define CPAUTH_CHECKUSER  0
#define CPAUTH_LOGIN      1
#define CPAUTH_CHGPWD     2
typedef struct {
    uint8_t  type;			/* CP_AUTHENTICATE */
    uint8_t  flag;			/* CPAUTH_* */
    uint8_t  pad2;
    uint8_t  pad3;

    uint8_t  login[MAXUSERNAME];
    uint8_t  pw[MAXUSERNAME];
} cpAuthenticate_t;

typedef struct {
    uint8_t  type;			/* CP_SETCOURSE */
    uint8_t  lock;			/* lockon */
    uint16_t lockDetail;

    uint16_t head; 		/* x100 */
} cpSetCourse_t;

typedef struct {
    uint8_t  type;                 /* CP_MESSAGE */
    uint8_t  to;
    int16_t  toDetail;

    uint8_t  msg[MESSAGE_SIZE];
} cpMessage_t;

#pragma pack()                  /* restore dflt struct alignment */

#endif /* PROTOCOL_H_INCLUDED */
