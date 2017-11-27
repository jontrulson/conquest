/*
 * Ship information, dependent on Type
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef SHIPTYPE_H_INCLUDED
#define SHIPTYPE_H_INCLUDED

#include "conqdef.h"

typedef struct {

    char name[MAXSTNAME];	/* "Destroyer, Scout, Cruiser" */
    unsigned int armylim; /* maximum armies this shiptype can carry */
    real warplim;		/* maximum warp based on shiptype */
    real engfac;		/* shiptype engine constants */
    real accelfac;		/* shiptype acceleration constants */
    real weafac;		/* shiptype weapons constants */
    real torpwarp;		/* torp speed for shiptype */
} ShipType_t;

#endif /* SHIPTYPE_H_INCLUDED */
