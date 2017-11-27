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

void cbMap(void);
void cbFlush(void);

void cbLock(int *);
void cbUnlock(int *);

void cbZero(void);
void cbMapLocal(void);

/* The Common Block in all it's majesty */

// This must be the first var
CEXTERN unsigned int *CBlockRevision;	/* common block rev number */
// This must be the second var
CEXTERN CBGlobal_t *CBGlobal;   /* global limits from CQI */

CEXTERN ConqInfo_t *ConqInfo;	/* misc game info */

CEXTERN User_t *Users;		/* User statistics. */

CEXTERN Robot_t *Robot;		/* Robots. */

CEXTERN Planet_t *Planets;	/* Planets. */

CEXTERN Team_t *Teams;		/* Teams. */

CEXTERN Doomsday_t *Doomsday;	/* Doomsday machine. */

CEXTERN History_t *History;	/* History */

CEXTERN Driver_t *Driver;	/* Driver. */

CEXTERN Ship_t *Ships;		/* Ships. */

CEXTERN ShipType_t *ShipTypes;		/* Ship types. */

CEXTERN Msg_t *Msgs;		/* Messages. */
// This must be the last var
CEXTERN unsigned int *EndOfCBlock;	/* end of the common block */

#undef CEXTERN
#endif /* CB_H */
