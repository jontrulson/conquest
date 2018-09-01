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

/* blinkers - blinkers are animdefs that simply toggle their armed
 * state on and off at given time intervals.  They can be used
 * anywhere within conquest(gl only of course :).  Hence we create a
 * global blinker animVec here (that the node renderer will iterate)
 * and make the blinkers globally available to conquest.
 */

#ifndef _BLINKER_H
#define _BLINKER_H

#if defined(NOEXTERN_BLINKER)
animStateRec_t        blinkerOneSec = {};
animStateRec_t        blinkerHalfSec = {};
animStateRec_t        blinkerQtrSec = {};

animVec_t             blinkerQue = {};

#else

extern animStateRec_t blinkerOneSec;
extern animStateRec_t blinkerHalfSec;
extern animStateRec_t blinkerQtrSec;

extern animVec_t      blinkerQue;

#endif

/* easier macros */
#define BLINK_ONESEC      (blinkerOneSec.state.armed)
#define BLINK_HALFSEC     (blinkerHalfSec.state.armed)
#define BLINK_QTRSEC      (blinkerQtrSec.state.armed)

void blinkerInit(void);

#endif  /* _BLINKER_H */
