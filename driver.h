/*
 * Driver structure
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef DRIVER_H_INCLUDED
#define DRIVER_H_INCLUDED

#include "conqdef.h"

typedef struct {
    int  drivstat; /* driver status; one of the DRS_ values */
    int  drivpid;  /* pid of the driver process */
    int  drivcnt;  /* number of driver timeouts (utModPlusOne 1000) */
    int  drivsecs; /* phase of the universe in seconds */
                                /*  (utModPlusOne five minutes) */
    char drivowner[MAXUSERNAME]; /* username of the driver's owner */

    // The following are seconds since midnight.
    unsigned int  drivtime;     /* last time the driver looped */
    unsigned int  playtime;     /* last time a player looped */
} Driver_t;

#endif /* DRIVER_H_INCLUDED */
