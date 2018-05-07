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


#ifndef _PLAYBACK_H
#define _PLAYBACK_H

#if defined(NOEXTERN_PLAYBACK)
int        pbSpeed = 1;         /* 1x */
#else
extern int pbSpeed;
#endif /* NOEXTERN_PLAYBACK */

#include "conqdef.h"
#include "cb.h"

/* First 2 bytes of a gzipped archive.  We use this to detect when an
 * attempt is made to playback a compressed cqr file, but gzip support
 * is not compiled in */
#define GZIPPED_BYTE_1     0x1f
#define GZIPPED_BYTE_2     0x8b

/* minimum/maximum playback speed (-PB_SPEED_MAX_TIMESx to
 *  PB_SPEED_MAX_TIMESx).
 */
#define PB_SPEED_MAX_TIMES 10
/* the fastest you can go - recFrameDelay set to 0.0 */
#define PB_SPEED_INFINITE  (PB_SPEED_MAX_TIMES + 1)

int  pbInitReplay(char *fname, time_t *elapsed);
void pbFileSeek(time_t newtime);
int  pbProcessIter(void);
int  pbProcessPackets(void);
void pbSetPlaybackSpeed(int speed, int samplerate);

void pbDisplayReplayMenu();
void pbDisplayReplayHelp();
#endif /* _PLAYBACK_H */
