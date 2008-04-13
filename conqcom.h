
/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef CONQCOM_H
#define CONQCOM_H

#ifdef NOCOMEXTERN
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

#define CMN_MODE 0660		/* mode of a new common block */


/* conqcm.c */
char *mymalloc(int size);
void map_common(void);
void fake_common(void);		/* malloc cmn rather than map */
void lock_common(void);
void flush_common(void);
int check_cblock(char *fname, int fmode, int sizeofcb);

void PVLOCK(int *);
void PVUNLOCK(int *);

void zero_common(void);
void fake_common(void);
void map_lcommon(void);
				/* The Common Block in all it's majesty */

/* Special data items - these must be at the very beginning. */
				/* This must be the first var */
CEXTERN int *CBlockRevision;	/* common block rev number */

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

CEXTERN int *EndOfCBlock;	/* end of the common block */

#undef CEXTERN
#endif /* CONQCOM_H */
