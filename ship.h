/* 
 * Ship structure
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef SHIP_H_INCLUDED
#define SHIP_H_INCLUDED

#include "conqdef.h"
#include "torp.h"

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
  int shup;			/* shields up or down */
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
  int rmode;			/* in repair mode or not */
  int cloaked;			/* cloaking device on or not */
  int options[MAXOPTIONS];	/* user options - copy of options */
  int war[NUMPLAYERTEAMS];	/* teams s/he is at war with - copy */
				/*  of war */
  int rwar[NUMPLAYERTEAMS];	/* teams s/he is at WAR with */
  int srpwar[NUMPLANETS + 1];	/* self-ruled planets s/he is at war */
  int sdfuse;			/* self-destruct fuse, 0 normally */
  int lastmsg;			/* last message seen */
  int alastmsg;			/* last message allowed to be seen */
  int map;			/* strategic map or not */
  int towing;			/* ship we are towing, or 0 */
  int towedby;			/* ship towing us, or 0 */
  real lastblast;		/* direction of last weapons usage */
  real lastphase;		/* phaser direction, needed by display(] */
  int pfuse;			/* tenths until can phaser again */
  int scanned[NUMPLAYERTEAMS];	/* fuse for which ships have been */
				/*  scanned by which teams */
  int talert;			/* torp alert! */
  int robot;			/* true if is a robot ship */
  int action;			/* current action token if a robot */
  int ctime;			/* cpu hundredths at last check */
  int etime;			/* elapsed thousands at last check */
  int cacc;			/* accumulated cpu time */
  int eacc;			/* accumulated elapsed time */
  real strkills;		/* 'extra' randomized kills used for robots */
  Torp_t torps[MAXTORPS];	/* Torpedos */
  char alias[SIZEUSERPNAME];	/* user's pseudonym, copy of Users[].alias */
} Ship_t;

#endif /* SHIP_H_INCLUDED */
