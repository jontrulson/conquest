/*
 * Doomsday structure
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef DOOMSDAY_H_INCLUDED
#define DOOMSDAY_H_INCLUDED

#include "conqdef.h"

// global limits
#define DOOMSDAY_START_DIST 20000.0 /* distance from the center that it starts at */
#define DOOMSDAY_WARP 2.0 	/* warp factor of the planet eater */
#define DOOMSDAY_DIST 1500.0 	/* distance it starts shooting */
#define DOOMSDAY_KILL_DIST 300.0 /* minimum distance for killing it */
#define DOOMSDAY_HIT 7.0 	/* blast per second */
#define DOOMSDAY_PROB 0.0001 	/* probability per minute that it appears */

#define DOOMSIZE 800            /* base size of doomsday in CU's */

// flags
#define DOOM_F_NONE        0x00
#define DOOM_F_LIVE        0x01
#define DOOM_F_ATTACKING   0x02

#define DOOM_LIVE()        ( cbDoomsday->flags & DOOM_F_LIVE )
#define DOOM_ATTACKING()   ( cbDoomsday->flags & DOOM_F_ATTACKING )

#define DOOMSET(b)         ( cbDoomsday->flags |= (b) )
#define DOOMCLR(b)         ( cbDoomsday->flags &= ~(b) )

typedef enum {
    EATER_DOOMSDAY           = 0, // default
    EATER_AMOEBA             = 1,
    EATER_NOMAD              = 2,

    EATER_MAX                // must be last
} eaterType_t;

typedef struct {
    uint8_t     flags;      // Just the flags ma'am.
    eaterType_t eaterType;  // type of eater, always doomsday for now
    real x;                 /* x and y coordinates of doomsday */
    real y;
    real dx;                /* x and y increments in mega-meters */
    real dy;
    real heading;           /* heading */
    courseLock_t lock;      /* one of the courseLock_t values */
    uint16_t lockDetail;    /* ship or planet number locked onto */
    char name[MAXUSERNAME];	/* name of doomsday machine */
} Doomsday_t;

#endif /* DOOMSDAY_H_INCLUDED */
