/* 
 * Driver structure
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef DRIVER_H_INCLUDED
#define DRIVER_H_INCLUDED

#include "conqdef.h"

typedef struct {
  int drivstat;			/* driver status; one of the DRS_ values */
  int drivpid;			/* pid of the driver process */
  int drivcnt;			/* number of driver timeouts (modp1 1000) */
  int drivsecs;			/* phase of the universe in seconds */
				/*  (modp1 five minutes) */
  char drivowner[SIZEUSERNAME]; /* username of the driver's owner */
  				/* The following are seconds since midnight. */
  int drivtime;			/* last time the driver looped */
  int playtime;			/* last time a player looped */
  
} Driver_t;

#endif /* DRIVER_H_INCLUDED */
