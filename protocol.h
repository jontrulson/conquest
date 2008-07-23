/* 
 * The protocol definition
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef PROTOCOL_H_INCLUDED
#define PROTOCOL_H_INCLUDED

#include "conqdef.h"
#include "datatypes.h"

#define PROTOCOL_VERMAJ 0
#define PROTOCOL_VERMIN 6
#define PROTOCOL_VERSION  (Unsgn16)((PROTOCOL_VERMAJ << 8) | PROTOCOL_VERMIN)


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

struct _generic_var {
  Unsgn8 type;                  /* SP_VARIABLE | CP_VARIABLE */
  Unsgn8 len;
  Unsgn16 pad1;

  /* the actual data would be written following this */
};

typedef struct _generic_var pktVariable_t;

/* server -> client packets */
typedef struct _sp_null {
  Unsgn8 type;			/* SP_NULL */
  Unsgn8 pad1;
  Unsgn8 pad2;
  Unsgn8 pad3;
} spNull_t;

#define SPHELLO_FLAGS_NONE         0x00
#define SPHELLO_FLAGS_CLOSED       0x01	/* game closed */
typedef struct _sp_hello {
  Unsgn8  type;			/* SP_HELLO */
  Unsgn8  flags;                /* closed, etc... */
  Unsgn16 protover;		/* protocol rev */

  Unsgn32 cmnrev;		/* common block version */

  Unsgn32 pad1;

  /* see conf.h for SERVER_*_SZ */
  Unsgn8  servername[CONF_SERVER_NAME_SZ]; /* long server name */

  Unsgn8  serverver[CONF_SERVER_NAME_SZ]; /* server's version */

  Unsgn8  motd[CONF_SERVER_MOTD_SZ];	/* MOTD... */
} spHello_t;

/* ACK without message text */
typedef struct _sp_ack {
  Unsgn8 type;			/* SP_ACK */
  Unsgn8 severity;		/* none, info, warning, etc */
  Unsgn8 code;			/* ack code */
  Unsgn8 pad1;

} spAck_t;

/* ACK with message text */
typedef struct _sp_ackmsg {
  Unsgn8 type;			/* SP_ACKMSG */
  Unsgn8 severity;		/* none, info, warning, etc */
  Unsgn8 code;			/* ack code */
  Unsgn8 pad1;

  Unsgn8 txt[MESSAGE_SIZE];	/* text */
} spAckMsg_t;

#define SPSSTAT_FLAGS_NONE        0x00000000
#define SPSSTAT_FLAGS_REFIT       0x00000001 /* allow refits? */
#define SPSSTAT_FLAGS_VACANT      0x00000002 /* allow vacant ships? */
#define SPSSTAT_FLAGS_SLINGSHOT   0x00000004 /* 'slingshot' bug enabled? */
#define SPSSTAT_FLAGS_NODOOMSDAY  0x00000008 /* no doomsday allowed? */
#define SPSSTAT_FLAGS_KILLBOTS    0x00000010 /* robots have randomized kills? */
#define SPSSTAT_FLAGS_SWITCHTEAM  0x00000020 /* allowed to switch teams? */

typedef struct _sp_serverstat {
  Unsgn8  type;                 /* SP_SERVSTAT */
  Unsgn8  numactive;		/* active ships */
  Unsgn8  numvacant;		/* vacant ships */
  Unsgn8  numrobot;		/* robots active */

  Unsgn8  numtotal;		/* total number of ships */
  Unsgn8  pad1;
  Unsgn16 numusers;		/* total number of users */

  Unsgn32 flags;		/* certain operating modes/options */
  
  Unsgn32 servertime;		/* the server's time */
} spServerStat_t;

#define SPCLNTSTAT_FLAG_NONE    0x00
#define SPCLNTSTAT_FLAG_NEW     0x01
#define SPCLNTSTAT_FLAG_CONQUER 0x02 /* you conquered the universe. send a
					message (lastwords) */
#define SPCLNTSTAT_FLAG_KILLED  0x04 /* you were killed by something */

typedef struct _sp_clientstat {
  Unsgn8  type;			/* SP_CLIENTSTAT */
  Unsgn8  flags;			/* LR/SR, live, dead, etc. */
  Unsgn8  snum;			/* your ship number */
  Unsgn8  team;			/* your team number */

  Unsgn16 unum;			/* your user number */
  Unsgn8  esystem;		/* team bitmask of systems
				   you can enter from when entering
				   the game. 0 if only one available */
  Unsgn8  pad2;
} spClientStat_t;

typedef struct _sp_ship {	/* 'large' ship data */
  Unsgn8  type;			/* SP_SHIP */
  Unsgn8  status;		/* one of the SS_ values */
  Unsgn8  snum;			/* ship num */
  Unsgn8  team;

  Unsgn16 unum;			/* user num */
  Unsgn8  shiptype;
  Unsgn8  towing;

  Unsgn8  towedby;
  Unsgn8  war;
  Unsgn8  rwar;
  Unsgn8  pad1;

  Sgn16   killedby;		/* depends on status - keep together in pkt */
  Sgn16   pad2;

  Unsgn32 kills;		/* x10 */

  Unsgn8  srpwar[NUMPLANETS + 1];

  Unsgn8  scanned[NUMPLAYERTEAMS];

  Unsgn8  alias[MAXUSERPNAME];
} spShip_t;

typedef struct _sp_shipsml {	/* 'short' ship packets */
  Unsgn8  type;			/* SP_SHIPSML */
  Unsgn8  snum;			/* ship num */
  Unsgn8  action;               /* ROB_* for robot/autopilot */
  Unsgn8  shields;		/* 0-100 */

  Unsgn8  damage;		/* 0-100 */
  Unsgn8  armies;
  Unsgn8  walloc;
  Sgn8    pfuse;

  Unsgn8  etemp;
  Unsgn8  wtemp;
  Unsgn16 fuel;

  Sgn16   sdfuse;
  Sgn8    wfuse;
  Sgn8    efuse;

  Sgn16   lock;
  Sgn16   flags;                /* SHIP_F_* */

  Unsgn16 lastphase;
  Unsgn16 lastblast;
} spShipSml_t;

typedef struct _sp_shiploc {	/* ship location packets */
  Unsgn8  type;			/* SP_SHIPLOC */
  Unsgn8  snum;			/* ship num */
  Unsgn16 head;			/* x10 */

  Sgn8   warp;			/* x10 */
  Unsgn8 pad1;
  Unsgn8 pad2;
  Unsgn8 pad3;

  Sgn32  x;			/* x1000 */

  Sgn32  y;			/* x1000 */
} spShipLoc_t;

typedef struct _sp_planet {
  Unsgn8  type;			/* SP_PLANET */
  Unsgn8  pnum;			/* planet number */
  Unsgn8  ptype;                /* planet type */
  Unsgn8  team;			/* team */

  Unsgn8  name[MAXPLANETNAME];
} spPlanet_t;

typedef struct _sp_planetsml {
  Unsgn8  type;			/* SP_PLANETSML */
  Unsgn8  pnum;			/* planet number */
  Unsgn8  scanned;		/* bitmask of teams that have scanned */
  Unsgn8  uninhabtime;
} spPlanetSml_t;

typedef struct _sp_planetloc {
  Unsgn8  type;                 /* SP_PLANETLOC */
  Unsgn8  pnum;
  Sgn16   armies;

  Sgn32   x;			/* x1000 */

  Sgn32   y;			/* x1000 */
} spPlanetLoc_t;

typedef struct _sp_planetloc2 {
  Unsgn8  type;                  /* SP_PLANETLOC2 */
  Unsgn8  pnum;
  Sgn16   armies;

  Sgn32   x;			/* x1000 */

  Sgn32   y;			/* x1000 */

  Unsgn16 orbang;               /* x100 */

} spPlanetLoc2_t;


#define SPPLANETINFO_FLAGS_NONE       0x00
#define SPPLANETINFO_FLAGS_FVALID     0x01 /* flags are valid */
#define SPPLANETINFO_FLAGS_REAL       0x02 /* planet is real (visible) */

typedef struct _sp_planetinfo {
  Unsgn8  type;			/* SP_PLANETINFO */
  Unsgn8  pnum;			/* planet number */
  Unsgn8  primary;
  Unsgn8  flags;

  Unsgn32 orbrad;               /* x10 */

  Sgn32   orbvel;               /* x100 */
} spPlanetInfo_t;

typedef struct _sp_message {
  Unsgn8  type;                 /* SP_MESSAGE */
  Unsgn8  flags;                /* see msg.h */
  Unsgn8  pad2;
  Unsgn8  pad3;

  Sgn16   from;
  Sgn16   to;
  
  Unsgn8  msg[MESSAGE_SIZE];
} spMessage_t;

#define SPUSER_FLAGS_NONE     0x00
#define SPUSER_FLAGS_LIVE     0x01
typedef struct _sp_user {
  Unsgn8  type;			/* SP_USER */
  Unsgn8  team;
  Unsgn16 unum;			/* user num */

  Unsgn8  flags;                /* hostile, live, etc */
  Unsgn8  war;			/* a bit mask (1 << TEAM_*) */
  Sgn16   rating;               /* x100 */

  Unsgn32 lastentry;

  Sgn32   ooptions[OOPT_TOTALOOPTION];

  Sgn32   stats[USTAT_TOTALSTATS];

  Unsgn8  username[MAXUSERNAME];

  Unsgn8  alias[MAXUSERPNAME];
} spUser_t;

typedef struct _sp_torp {
  Unsgn8  type;			/* SP_TORP */
  Unsgn8  snum;			/* ship num */
  Unsgn8  tnum;			/* torp num */
  Unsgn8  status;		/* one of the TS_* values */
} spTorp_t;

typedef struct _sp_torploc {
  Unsgn8  type;                 /* SP_TORPLOC */
  Unsgn8  snum;                 /* ship num */
  Unsgn8  tnum;                 /* torp num */
  Unsgn8  war;                  /* war. what is it good for. */

  Sgn32   x;			/* x1000 */

  Sgn32   y;			/* x1000 */
} spTorpLoc_t;

typedef struct _sp_torpevent {
  Unsgn8  type;                 /* SP_TORPEVENT */
  Unsgn8  snum;                 /* ship num */
  Unsgn8  tnum;                 /* torp num */
  Unsgn8  war;                  /* war. what is it good for. */

  Sgn32   x;			/* x1000 */

  Sgn32   y;			/* x1000 */

  Sgn32   dx;			/* x1000 */

  Sgn32   dy;			/* x1000 */

  
  Unsgn8  status;		/* one of the TS_* values */
  Unsgn8  pad1;
  Unsgn8  pad2;
  Unsgn8  pad3;

} spTorpEvent_t;

#define SPTEAM_FLAGS_NONE         0x00
#define SPTEAM_FLAGS_COUPINFO     0x01
typedef struct _sp_team {
  Unsgn8  type;                 /* SP_TEAM */
  Unsgn8  team;
  Unsgn8  homesun;
  Unsgn8  flags;                /* coupinfo, etc */

  Unsgn8  couptime;		/* minutes until coup can work */
  Unsgn8  teamhplanets[3];

  Unsgn8  homeplanet;  
  Unsgn8  pad1;
  Unsgn16 pad2;

  Unsgn32 stats[MAXTSTATS];

  Unsgn8  name[MAXTEAMNAME]; 
} spTeam_t;

typedef struct _sp_conqinfo {
  Unsgn8  type;                 /* SP_CONQINFO */
  Unsgn8  pad1;
  Unsgn8  pad2;
  Unsgn8  pad3;

  Unsgn8  conqueror[MAXUSERPNAME]; /* user who last conquered */
  Unsgn8  conqteam[MAXTEAMNAME];   /* team s/he was on */
  Unsgn8  conqtime[DATESIZE];      /* time of last conquering */
  Unsgn8  lastwords[MAXLASTWORDS]; /* remarks upon conquering */
} spConqInfo_t;

/* only stored in recordings (.cqr) files */
typedef struct _sp_frame {
  Unsgn8  type;			/* SP_FRAME */
  Unsgn8  pad1;
  Unsgn8  pad2;
  Unsgn8  pad3;

  Unsgn32 frame;                /* frame number */

  Unsgn32 time;                 /* timestamp */

} spFrame_t;

typedef struct _sp_history {
  Unsgn8  type;			/* SP_HISTORY */
  Unsgn8  histptr;              /* current hist ptr */
  Unsgn8  hnum;
  Unsgn8  pad1;

  Unsgn16 unum;
  Unsgn16 pad2;

  Unsgn32 elapsed;
 
  Unsgn32 histlog;
} spHistory_t;

typedef struct _sp_doomsday {
  Unsgn8  type;			/* SP_DOOMSDAY */
  Unsgn8  status;               /* DS_* */
  Unsgn16 heading;              /* x10 */

  Sgn32   x;			/* x1000 */
  Sgn32   y;			/* x1000 */
} spDoomsday_t;


/* client -> server packets */
typedef struct _cp_null {	/* never used */
  Unsgn8  type;			/* CP_NULL */
  Unsgn8  pad1;
  Unsgn8  pad2;
  Unsgn8  pad3;
} cpNull_t;

typedef struct _cp_hello {
  Unsgn8  type;			/* CP_HELLO */
  Unsgn8  updates;		/* desired updates per sec */
  Unsgn16 protover;		/* protocol rev */

  Unsgn32 cmnrev;		/* common block version */

  Unsgn8  clientname[CONF_SERVER_NAME_SZ]; /* "ConquestGL", etc */

  Unsgn8  clientver[CONF_SERVER_NAME_SZ];	/* version, misc info */
} cpHello_t;

typedef struct _cp_ack {
  Unsgn8  type;			/* CP_ACK */
  Unsgn8  severity;		/* none, info, warning, etc */
  Unsgn8  code;			/* ack code */
  Unsgn8  pad1;
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

typedef struct _cp_command {
  Unsgn8  type;			/* CP_COMMAND */
  Unsgn8  cmd;			/* one of CPCMD_* */
  Unsgn16 detail;		/* command specific */
} cpCommand_t;

typedef struct _cp_firetorps {
  Unsgn8  type;			/* CP_FIRETORPS */
  Unsgn8  num;			/* number to fire */
  Unsgn16 dir;			/* x100 */
} cpFireTorps_t;

#define CP_SINFOMAX  15
typedef struct _cp_getsinfo {
  Unsgn8  type;			/* CP_GETSINFO */
  Unsgn8  info[CP_SINFOMAX];
} cpGetSInfo_t;

typedef struct _cp_sendmsg {
  Unsgn8  type;			/* CP_SENDMSG */
  Unsgn8  pad1;
  Sgn16   to;

  Unsgn8  msg[MESSAGE_SIZE];
} cpSendMsg_t;

typedef struct _cp_setname {
  Unsgn8  type;			/* CP_SETNAME */
  Unsgn8  alias[MAXUSERPNAME];
} cpSetName_t;

/* authenticate flags */
#define CPAUTH_CHECKUSER  0
#define CPAUTH_LOGIN      1
#define CPAUTH_CHGPWD     2
typedef struct _cp_authenticate {
  Unsgn8  type;			/* CP_AUTHENTICATE */
  Unsgn8  flag;			/* CPAUTH_* */
  Unsgn8  pad2;
  Unsgn8  pad3;

  Unsgn8  login[MAXUSERNAME];
  Unsgn8  pw[MAXUSERNAME];
} cpAuthenticate_t;

typedef struct _cp_setcourse{
  Unsgn8  type;			/* CP_SETCOURSE */
  Sgn8    lock;			/* lockon */
  Unsgn16 head;			/* x100 */
} cpSetCourse_t;

typedef struct _cp_message {
  Unsgn8  type;                 /* CP_MESSAGE */
  Unsgn8  pad1;
  Sgn16   to;
  
  Unsgn8  msg[MESSAGE_SIZE];
} cpMessage_t;

#pragma pack()                  /* restore dflt struct alignment */

#endif /* PROTOCOL_H_INCLUDED */
