/* 
 * record.h - recording games in conquest
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _PLAYBACK_H
#define _PLAYBACK_H

#include "conqdef.h"
#include "conqcom.h"

void pbFileSeek(time_t newtime);
int pbProcessIter(void);
int pbProcessPackets(void);

#endif /* _PLAYBACK_H */
