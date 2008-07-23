/* 
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _PLAYBACK_H
#define _PLAYBACK_H

#if defined(NOEXTERN_PLAYBACK)
int        pbSpeed = 1;         /* 1x */
#else
extern int pbSpeed;
#endif /* NOEXTERN_PLAYBACK */

#include "conqdef.h"
#include "conqcom.h"

/* minimum/maximum playback speed (-PB_SPEED_MAX_TIMESx to
 *  PB_SPEED_MAX_TIMESx). 
 */ 
#define PB_SPEED_MAX_TIMES 10   
/* the fastest you can go - recFrameDelay set to 0.0 */
#define PB_SPEED_INFINITE  (PB_SPEED_MAX_TIMES + 1)   

void pbFileSeek(time_t newtime);
int  pbProcessIter(void);
int  pbProcessPackets(void);
void pbSetPlaybackSpeed(int speed, int samplerate);

#endif /* _PLAYBACK_H */
