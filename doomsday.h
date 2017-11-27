/*
 * Doomsday structure
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef DOOMSDAY_H_INCLUDED
#define DOOMSDAY_H_INCLUDED

#include "conqdef.h"

typedef struct {
    int  status;          /* doomsday status; one of the DS_ values */
    real x;			/* x and y coordinates of doomsday */
    real y;
    real dx;			/* x and y increments in mega-meters */
    real dy;
    real heading;			/* heading */
    courseLock_t lock;      /* one of the courseLock_t values */
    uint16_t lockDetail;    /* ship or planet number locked onto */
    char name[MAXUSERNAME];	/* name of doomsday machine */
} Doomsday_t;

#endif /* DOOMSDAY_H_INCLUDED */
