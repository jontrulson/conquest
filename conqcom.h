
/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                              C O N Q C O M */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

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
