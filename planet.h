/*
 * Planet structure
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef PLANET_H_INCLUDED
#define PLANET_H_INCLUDED


#include "conqdef.h"

/* flags for planets.  Of the planets with HOMEPLANET set, the first
 * one will be the capital planet.  Only Fed, Kli, Ori, and Rom can
 * have homeplanets.
 */

#define PLAN_F_NONE        0x00000000
#define PLAN_F_VISIBLE     0x00000001 /* true if visible */
#define PLAN_F_HOMEPLANET  0x00000002 /* true if a team homeplanet */
#define PLAN_F_CORE        0x00000004 /* true if needed to take universe */

/* helpers */
#define PVISIBLE(x)        ( Planets[(x)].flags & PLAN_F_VISIBLE )
#define PHOMEPLANET(x)     ( Planets[(x)].flags & PLAN_F_HOMEPLANET )
#define PCORE(x)           ( Planets[(x)].flags & PLAN_F_CORE )

#define PFSET(x, b)  ( Planets[(x)].flags |=  (b) )
#define PFCLR(x, b)  ( Planets[(x)].flags &= ~(b) )

typedef struct {
    real x;			/* x coordinates of planet */
    real y;			/* y coordinates of planet */
    real orbrad;                  /* radius of the orbit */
    real orbvel;                  /* velocity of the orbit in deg/min */
    real orbang;                  /* current angle of the orbit */
    real size;                    /* size of planet in CU's */
    // FIXME - 32b too much?  16b better?
    uint32_t flags;                /* flags for this planet */
    int  primary;                 /* planet this planet orbits */
    int  type;                    /* type of planet - M, dead, sun */
    int  team;                    /* which team owns the planet */
    int  defendteam;              // for planets marked as homeplanet,
                                  // this member holds the team
                                  // responsible for defense (robots or
                                  // players)
    int  armies;                  /* how many armies are on the planet */
    int  uninhabtime;             /* minutes left till inhabitable */
    int  scanned[NUMPLAYERTEAMS]; /* which planets have been */
    /* scanned by which teams */
    char name[MAXPLANETNAME];     /* planet name */
} Planet_t;

#endif /* PLANET_H_INCLUDED */
