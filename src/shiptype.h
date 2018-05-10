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


#ifndef SHIPTYPE_H_INCLUDED
#define SHIPTYPE_H_INCLUDED

#include "conqdef.h"

#define MAXSTNAME 32

#define MAXNUMSHIPTYPES 3

#define ST_SCOUT      0         /* Scout vessel (Ori) */
#define ST_DESTROYER  1         /* Destroyer (Fed/Kli) */
#define ST_CRUISER    2         /* Cruiser (Rom) */

typedef struct {

    char name[MAXSTNAME];       /* "Destroyer, Scout, Cruiser" */

    // max limits for this ship type
    uint8_t armyMax;            /* maximum armies this shiptype can carry */
    uint8_t warpMax;            /* maximum warp based on shiptype */
    uint8_t torpMax;            /* max number of torps */

    real shieldMax;             /* max shields value */
    real damageMax;             /* max damage value */
    real fuelMax;               /* max amount of fuel */

    real engfac;                /* shiptype engine constants */
    real accelfac;              /* shiptype acceleration constants */
    real weafac;                /* shiptype weapons constants */

    real torpwarp;              /* torp speed for shiptype */

    real size;                  /* in CU's */
} ShipType_t;

#endif /* SHIPTYPE_H_INCLUDED */
