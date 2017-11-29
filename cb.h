// A Unix implementation of a Fortran common block, mostly.

/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

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
#include "cbglobal.h"

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


// do a "pretend" map of the CB variables and return the total size
// in bytes that needs to be available to do a real mmap() or
// malloc().
unsigned int cbGetSize();

/* The Common Block in all it's majesty */

// This must be the first var
CEXTERN unsigned int *cbRevision; /* common block rev number */
// This must be the second var
CEXTERN cbGlobalLimits_t *cbGlobalLimits;   /* global limits from CQI */

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
// This must be the last var
CEXTERN unsigned int *EndOfCBlock; /* end of the common block */

#undef CEXTERN
#endif /* CB_H */
