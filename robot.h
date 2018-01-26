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

#ifndef ROBOT_H_INCLUDED
#define ROBOT_H_INCLUDED

#include "conqdef.h"

/* Robot variables. */
#define VAR_FUEL 0
#define VAR_SHIELDS 1
#define VAR_NUMTORPS 2
#define VAR_DAMAGE 3
#define VAR_ETEMP 4
#define VAR_WTEMP 5
#define VAR_WARP 6
#define VAR_SHUP 7
#define VAR_WALLOC 8
#define VAR_ORBITING 9
#define VAR_REPAIRING 10
#define VAR_CLOAKED 11
#define VAR_CANREAD 12
#define VAR_RANDOM 13
#define VAR_DNE 14
#define VAR_ENEMYCLOAKED 15
#define VAR_ENEMYDAMAGE 16
#define VAR_PHASERDAM 17
#define VAR_TORPDAM 18
#define VAR_INCOMING 19

#define MAX_VAR 20
#define REAL_MAX_VAR 32 /* common block max (leaves room for expansion) */

/* Robot action tokens. */
#define ROB_NOOP 1 		/* do no-thing */
#define ROB_GOHOME 2 		/* head for our home planet */
#define ROB_GOREPAIR 3 		/* head for any planet */
#define ROB_ALLOCATE 4 		/* change engines/weapons allocation */
#define ROB_DETONATE 5 		/* detonate enemy torpedos */
#define ROB_MYDETONATE 6 	/* detonate my torpedos */
#define ROB_PHASER 7 		/* fire phasers */
#define ROB_TORPEDO 8 		/* fire a torpedo */
#define ROB_BURST 9 		/* fire a torpedo burst */
#define ROB_SHIELD 10 		/* raise or lower shields */
#define ROB_UNTRACTOR 11 	/* attempt to untractor */
#define ROB_WARP_0 12 		/* change speed */
#define ROB_WARP_2 13 		/* change speed */
#define ROB_WARP_5 14 		/* change speed */
#define ROB_WARP_8 15 		/* change speed */
#define ROB_TRACK 16 		/* go after an enemy */
#define ROB_SILENT 17 		/* slient running */
#define ROB_MESSAGE 18 		/* send a message */
#define ROB_TAKEDRUGS 19 	/* space out */
#define ROB_REPAIR 20 		/* enter repair mode */
#define ROB_READMSG 21 		/* read (and possibly reply to) a message */
#define ROB_INSULT 22 		/* send a gratuitous insult */
#define ROB_GOFUEL 23 		/* head for any class M planet */
#define ROB_RUNAWAY 24 		/* go away from the nearest enemy */


typedef struct {
    int rstrat[REAL_MAX_VAR][10];	/* robot strategy table */
    int rvec[32];			/* robot rule to action token */
} Robot_t;

#endif /* ROBOT_H_INCLUDED */
