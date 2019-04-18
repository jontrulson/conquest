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


#ifndef PLANET_H_INCLUDED
#define PLANET_H_INCLUDED


#include "conqdef.h"
#include "team.h"

// Some maximums...
#define MAXPLANETTYPES 8  /* number of different planet types */
#define MAXPTYPENAME   20 /* maximum size of a planet type's name */
#define MAX_PLANETNAME  12 /* maximum size of a planet's name */

// Planet types - we specify an int as the size to maintain CB
// compatibility

enum PlanetType : int {
    PLANET_CLASSZ = 0,    // ???
    PLANET_CLASSM = 1,    // "Sensors indicate a Class M planet..." */
    PLANET_DEAD = 2,
    PLANET_SUN = 3,       // a bit of a contradiction...
    PLANET_MOON = 4,      // a bit of fun
    PLANET_GHOST = 5,     // invisible anchor for binary systems
    PLANET_CLASSA = 6,    // ???
    PLANET_CLASSO = 7,    // ???
};

/* flags for planets.  Of the planets with HOMEPLANET set, the first
 * one will be the capital planet.  Only Fed, Kli, Ori, and Rom can
 * have homeplanets.
 */

#define PLAN_F_NONE        0x00000000
#define PLAN_F_VISIBLE     0x00000001 /* true if visible */
#define PLAN_F_HOMEPLANET  0x00000002 /* true if a team homeplanet */
#define PLAN_F_CORE        0x00000004 /* true if needed to take universe */

/* helpers */
#define PVISIBLE(x)        ( cbPlanets[(x)].flags & PLAN_F_VISIBLE )
#define PHOMEPLANET(x)     ( cbPlanets[(x)].flags & PLAN_F_HOMEPLANET )
#define PCORE(x)           ( cbPlanets[(x)].flags & PLAN_F_CORE )

#define PFSET(x, b)  ( cbPlanets[(x)].flags |=  (b) )
#define PFCLR(x, b)  ( cbPlanets[(x)].flags &= ~(b) )

typedef struct {
    real x;                       /* x coordinates of planet */
    real y;                       /* y coordinates of planet */
    real orbrad;                  /* radius of the orbit */
    real orbvel;                  /* velocity of the orbit in deg/min */
    real orbang;                  /* current angle of the orbit */
    uint16_t size;                /* size of planet in CU's */
    uint32_t flags;               /* flags for this planet */
    int  primary;                 /* planet this planet orbits */
    PlanetType type;              /* type of planet - M, dead, sun */
    int  team;                    /* which team owns the planet */
    int  defendteam;              // for planets marked as homeplanet,
                                  // this member holds the team
                                  // responsible for defense (robots or
                                  // players)
    int  armies;                  /* how many armies are on the planet */
    int  uninhabtime;             /* minutes left till inhabitable */
    bool scanned[NUM_PLAYERTEAMS]; /* which planets have been */
                                  /* scanned by which teams */
    char name[MAX_PLANETNAME];     /* planet name */
} Planet_t;

#endif /* PLANET_H_INCLUDED */
