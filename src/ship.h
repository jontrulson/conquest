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

#ifndef SHIP_H_INCLUDED
#define SHIP_H_INCLUDED


#include "conqdef.h"
#include "torp.h"

#define SHIP_F_NONE        0x00000000
#define SHIP_F_CLOAKED     0x00000001 /* true if cloaked */
#define SHIP_F_VACANT      0x00000002 /* true if vacant */
#define SHIP_F_SHUP        0x00000004 /* shields up */
#define SHIP_F_REPAIR      0x00000008 /* true if repairing */
#define SHIP_F_TALERT      0x00000010 /* torp alert! */
#define SHIP_F_ROBOT       0x00000020 /* true if is a robot ship */
#define SHIP_F_MAP         0x00000040 /* strategic map or not */
#define SHIP_F_SCANDIST    0x00000080 /* ship dist < ACCINFO_DIST */
#define SHIP_F_BOMBING     0x00000100 /* ship is currently bombing */
#define SHIP_F_TOWEDBY     0x00000200 /* ship is being towed (see towedby) */
#define SHIP_F_TOWING      0x00000400 /* ship is towing someone (see towing) */

/* helpers */
#define SCLOAKED(x)  ( cbShips[(x)].flags & SHIP_F_CLOAKED )
#define SVACANT(x)   ( cbShips[(x)].flags & SHIP_F_VACANT )
#define SSHUP(x)     ( cbShips[(x)].flags & SHIP_F_SHUP )
#define SREPAIR(x)   ( cbShips[(x)].flags & SHIP_F_REPAIR )
#define STALERT(x)   ( cbShips[(x)].flags & SHIP_F_TALERT )
#define SROBOT(x)    ( cbShips[(x)].flags & SHIP_F_ROBOT )
#define SMAP(x)      ( cbShips[(x)].flags & SHIP_F_MAP )
#define SSCANDIST(x) ( cbShips[(x)].flags & SHIP_F_SCANDIST )
#define SBOMBING(x)  ( cbShips[(x)].flags & SHIP_F_BOMBING )
#define STOWEDBY(x)  ( cbShips[(x)].flags & SHIP_F_TOWEDBY )
#define STOWING(x)   ( cbShips[(x)].flags & SHIP_F_TOWING )

/* lookup the proper [SCALE|MAP]_FAC depending on LR/SR status */
#define SFAC(x)      (SMAP(x) ? MAP_LR_FAC : MAP_SR_FAC)

#define SFSET(x, b)  ( cbShips[(x)].flags |=  (b) )
#define SFCLR(x, b)  ( cbShips[(x)].flags &= ~(b) )

#define SHIP_IPADDR_LEN 16

typedef struct {
    int status;                 /* one of the SS_ values */

    killedBy_t killedBy;        /* one of the KB_ values.  Do not
                                 * exceed 255 for this value (8 bits)
                                 * since only a uint8_t is sent over
                                 * the protocol for this member. */
    uint16_t killedByDetail;    /* if killed by ship or planet, this
                                 * will hold the ship or planet
                                 * number.*/

    int unum;                   /* user number of the ship's captain */
    int team;                   /* team of the ship */
    int pid;                    /* process id of ships owner process */
    int shiptype;               /* one of the ST_* values  */
    real x;                     /* x and y coordinates of ship */
    real y;
    real dx;                    /* x and y increments */
    real dy;                    /* in mega-meters per iter seconds */
    real head;                  /* direction of motion - [0-359.9] */
    real dhead;                 /* desired direction of motion */
    real warp;                  /* warp factor: [0-variable] or */
                                /*  <0 for orbiting */
    real dwarp;                 /* desired warp factor: [0-variable] */
    courseLock_t lock;          /* one of the courseLock_t values */
    uint16_t lockDetail;        /* ship or planet number locked onto */
    real shields;               /* shield percentage - [0-100] */
    real kills;                 /* kills - [0-inf] */
    real damage;                /* damage percentage - [0-100] */
    real fuel;                  /* fuel - [0-999] */
    real wtemp;                 /* weapons temperature */
    real etemp;                 /* engine temperature */
    int wfuse;                  /* seconds until weapons work again */
    int efuse;                  /* seconds until engines work again */
    int weapalloc;              /* weapons allocation - [30-70] */
    int engalloc;               /* engines allocation is 100-sweapons */
    int armies;                 /* armies being carried */
    bool war[NUMPLAYERTEAMS];   /* teams s/he is at war with - copy */
                                /*  of war */
    bool rwar[NUMPLAYERTEAMS];  /* teams s/he is at WAR with */
    bool srpwar[ABS_MAXPLANETS];        /* self-ruled planets s/he is at war */
    int sdfuse;                 /* self-destruct fuse, 0 normally */
    int lastmsg;                        /* last message seen */
    int alastmsg;                       /* last message allowed to be seen */
    int towing;                 /* ship we are towing, or 0 */
    int towedby;                        /* ship towing us, or 0 */
    real lastblast;             /* direction of last weapons usage */
    real lastphase;             /* phaser direction, needed by display(] */
    int pfuse;                  /* tenths until can phaser again */
    int scanned[NUMPLAYERTEAMS]; /* fuse for which ships have been */
                                /*  scanned by which teams */
    int action;                 /* current action token if a robot */
    unsigned int ctime;         /* cpu hundredths at last check */
    uint64_t etime;             /* elapsed thousands at last check */
    unsigned int cacc;          /* accumulated cpu time */
    unsigned int eacc;          /* accumulated elapsed time */
    real strkills;              /* 'extra' randomized kills used for robots */
    Torp_t torps[ABS_MAXTORPS]; /* Torpedos */
    uint32_t flags;             /* SHIP_F_ */
    // Ship's IP address.  For IPV4 (only for now) the address is
    // stored at locations 12-15.
    uint8_t ipaddr[SHIP_IPADDR_LEN];
    uint32_t pad[5];            /* PAD */
    char alias[MAXUSERNAME];    /* copy of cbUsers[].alias */
} Ship_t;

#endif /* SHIP_H_INCLUDED */
