/* blinkers - blinkers are animdefs that simply toggle their armed
 * state on and off at given time intervals.  They can be used
 * anywhere within conquest(gl only of course :).  Hence we create a
 * global blinker animQue here (that the node renderer will iterate)
 * and make the blinkers globally available to conquest.
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _BLINKER_H
#define _BLINKER_H

#if defined(NOEXTERN_BLINKER)
animStateRec_t        blinkerOneSec = {};
animStateRec_t        blinkerHalfSec = {};
animStateRec_t        blinkerQtrSec = {};

animQue_t             blinkerQue = {};

#else

extern animStateRec_t blinkerOneSec;
extern animStateRec_t blinkerHalfSec;
extern animStateRec_t blinkerQtrSec;

extern animQue_t      blinkerQue;

#endif

/* easier macros */
#define BLINK_ONESEC      (blinkerOneSec.state.armed)
#define BLINK_HALFSEC     (blinkerHalfSec.state.armed)
#define BLINK_QTRSEC      (blinkerQtrSec.state.armed)

void blinkerInit(void);

#endif  /* _BLINKER_H */
