/* 
 * Torpedo structure for Ships[].torps
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef TORP_H_INCLUDED
#define TORP_H_INCLUDED

#include "conqdef.h"

typedef struct
{
    int status;			/* one of the TS_ values */
    int fuse;			/* seconds left */
    real x;			/* x and y coordinates of torps */
    real y;
    real dx;			/* x and y increments */
    real dy;			/*  in mega-meters per iter seconds */
    real mult;			/* multiplier when detonated */
    int war[NUMPLAYERTEAMS];	/* copy of war when fired */
} Torp_t;
#endif /* TORP_H_INCLUDED */
