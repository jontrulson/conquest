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

// A Unix implementation of a Fortran common block, mostly.

#ifndef CB_H
#define CB_H

#ifdef NOEXTERN_CB
#define CEXTERN
#else
#define CEXTERN extern
#endif

#include "user.h"
#include "shiptype.h"
#include "ship.h"
#include "planet.h"
#include "team.h"
#include "msg.h"
#include "doomsday.h"
#include "driver.h"
#include "conqinfo.h"
#include "history.h"
#include "robot.h"
#include "cbMaxLimits.h"

#define CMN_MODE 0660		/* mode of a new common block */

// map and unmap a file backed cCB (servers)
void cbMap(void);
void cbUnmap();

// map and unmap a heap based common block
void cbMapLocal(void);
void cbUnmapLocal();

// flush a common block to disk (servers)
void cbFlush(void);
// Zero out a common block
void cbZero(void);

// lock and unlock (via semaphore) access to the common block (servers
// only)
void cbLock(int *);
void cbUnlock(int *);

// is the common block mapped?
bool cbIsMapped();

// do a "pretend" map of the CB variables and return the total size
// in bytes that needs to be available to do a real mmap() or
// malloc().
unsigned int cbGetSize();

// Common block dynamic limits
CEXTERN cbMaxLimits cbLimits;

/* The Common Block in all it's majesty */

// This must be the first var
CEXTERN unsigned int *cbRevision; /* common block rev number */

CEXTERN cbConqInfo_t *cbConqInfo; /* misc game info */

CEXTERN User_t *cbUsers;        /* User statistics. */

CEXTERN Robot_t *cbRobot;       /* Robots. */

CEXTERN Planet_t *cbPlanets;	/* Planets. */

CEXTERN Team_t *cbTeams;        /* Teams. */

CEXTERN Doomsday_t *cbDoomsday;	/* Doomsday machine. */

CEXTERN History_t *cbHistory;	/* History */

CEXTERN Driver_t *cbDriver;	/* Driver. */

CEXTERN Ship_t *cbShips;		/* Ships. */

CEXTERN ShipType_t *cbShipTypes;  /* Ship types. */

CEXTERN Msg_t *cbMsgs;               /* Messages. */

#undef CEXTERN

#endif /* CB_H */
