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
 * global blinker animQue here (that the node renderer will iterate)
 * and make the blinkers globally available to conquest.
 */

#include "c_defs.h"
#include "conqdef.h"
#include "global.h"
#include "color.h"
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "textures.h"
#include "GL.h"
#include "anim.h"

#define NOEXTERN_BLINKER
#include "blinker.h"
#undef NOEXTERN_BLINKER

/* init the blinkers - needs to be done early before any rendering is done */
void blinkerInit(void)
{
    blinkerQue.clear();

    /* now setup the blinkers
     * these are toggle animations that never expire, so they
     *  only need to be done once at inittime.
     */

    if (!animInitState("onesec", &blinkerOneSec, NULL))
        utLog("%s: failed to init animstate for animation 'onesec'",
              __FUNCTION__);
    else
        blinkerQue.push_back(&blinkerOneSec);

    if (!animInitState("halfsec", &blinkerHalfSec, NULL))
        utLog("%s: failed to init animstate for animation 'halfsec'",
              __FUNCTION__);
    else
        blinkerQue.push_back(&blinkerHalfSec);

    if (!animInitState("qtrsec", &blinkerQtrSec, NULL))
        utLog("%s: failed to init animstate for animation 'qtrsec'",
              __FUNCTION__);
    else
        blinkerQue.push_back(&blinkerQtrSec);

    return;
}
