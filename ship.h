/* 
 * Ship structure
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef SHIP_H_INCLUDED
#define SHIP_H_INCLUDED

#include "datatypes.h"
#include "conqdef.h"
#include "torp.h"

#define SHIP_F_NONE        0x0000
#define SHIP_F_CLOAKED     0x0001 /* true if cloaked */
#define SHIP_F_VACANT      0x0002 /* true if vacant */
#define SHIP_F_SHUP        0x0004 /* shields up */
#define SHIP_F_REPAIR      0x0008 /* true if repairing */
#define SHIP_F_TALERT      0x0010 /* torp alert! */
#define SHIP_F_ROBOT       0x0020 /* true if is a robot ship */
#define SHIP_F_MAP         0x0040 /* strategic map or not */
#define SHIP_F_SCANDIST    0x0080 /* ship dist < ACCINFO_DIST */

/* helpers */
#define SCLOAKED(x)  ( bitIsSet(Ships[x].flags, SHIP_F_CLOAKED) )
#define SVACANT(x)   ( bitIsSet(Ships[x].flags, SHIP_F_VACANT) )
#define SSHUP(x)     ( bitIsSet(Ships[x].flags, SHIP_F_SHUP) )
#define SREPAIR(x)   ( bitIsSet(Ships[x].flags, SHIP_F_REPAIR) )
#define STALERT(x)   ( bitIsSet(Ships[x].flags, SHIP_F_TALERT) )
#define SROBOT(x)    ( bitIsSet(Ships[x].flags, SHIP_F_ROBOT) )
#define SMAP(x)      ( bitIsSet(Ships[x].flags, SHIP_F_MAP) )
#define SSCANDIST(x) ( bitIsSet(Ships[x].flags, SHIP_F_SCANDIST) )

#define SFSET(x, b)  (bitSet(Ships[x].flags, b))
#define SFCLR(x, b)  (bitClear(Ships[x].flags, b))

typedef struct {
  int status;			/* one of the SS_ values */
  int killedby;			/* one of the KB_ values, or: */
				/*   >0 means ship number */
				/*   <0 means -(planet number) */
  int unum;			/* user number of the ship's captain */
  int team;			/* team of the ship */
  int pid;			/* process id of ships owner process */
  int shiptype;			/* one of the ST_* values  */
  real x;			/* x and y coordinates of ship */
  real y;
  real dx;			/* x and y increments */
  real dy;			/* in mega-meters per iter seconds */
  real head;			/* direction of motion - [0-359.9] */
  real dhead;			/* desired direction of motion */
  real warp;			/* warp factor: [0-variable] or */
				/*  <0 for orbiting */
  real dwarp;			/* desired warp factor: [0-variable] */
  int lock;			/* 0 or -planet locked on to; */
				/*   if swarp<0, orbiting */
  real shields;			/* shield percentage - [0-100] */
  real kills;			/* kills - [0-inf] */
  real damage;			/* damage percentage - [0-100] */
  real fuel;			/* fuel - [0-999] */
  real wtemp;			/* weapons temperature */
  real etemp;			/* engine temperature */
  int wfuse;			/* seconds until weapons work again */
  int efuse;			/* seconds until engines work again */
  int weapalloc;		/* weapons allocation - [30-70] */
  int engalloc;			/* engines allocation is 100-sweapons */
  int armies;			/* armies being carried */
  int war[NUMPLAYERTEAMS];	/* teams s/he is at war with - copy */
				/*  of war */
  int rwar[NUMPLAYERTEAMS];	/* teams s/he is at WAR with */
  int srpwar[NUMPLANETS + 1];	/* self-ruled planets s/he is at war */
  int sdfuse;			/* self-destruct fuse, 0 normally */
  int lastmsg;			/* last message seen */
  int alastmsg;			/* last message allowed to be seen */
  int towing;			/* ship we are towing, or 0 */
  int towedby;			/* ship towing us, or 0 */
  real lastblast;		/* direction of last weapons usage */
  real lastphase;		/* phaser direction, needed by display(] */
  int pfuse;			/* tenths until can phaser again */
  int scanned[NUMPLAYERTEAMS];	/* fuse for which ships have been */
				/*  scanned by which teams */
  int action;			/* current action token if a robot */
  int ctime;			/* cpu hundredths at last check */
  int etime;			/* elapsed thousands at last check */
  int cacc;			/* accumulated cpu time */
  int eacc;			/* accumulated elapsed time */
  real strkills;		/* 'extra' randomized kills used for robots */
  Torp_t torps[MAXTORPS];	/* Torpedos */
  Unsgn16 flags;		/* SHIP_F_ */
  Unsgn16 pad1;			/* PAD */
  char alias[MAXUSERPNAME];	/* user's pseudonym, copy of Users[].alias */
} Ship_t;

#endif /* SHIP_H_INCLUDED */
