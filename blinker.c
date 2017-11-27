/* blinkers - blinkers are animdefs that simply toggle their armed
 * state on and off at given time intervals.  They can be used
 * anywhere within conquest(gl only of course :).  Hence we create a
 * global blinker animQue here (that the node renderer will iterate)
 * and make the blinkers globally available to conquest.
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#include "c_defs.h"
#include "conqdef.h"
#include "global.h"
#include "color.h"
#include "conqcom.h"
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
    animQueInit(&blinkerQue);

    /* now setup the blinkers
     * these are toggle animations that never expire, so they
     *  only need to be done once at inittime.
     */

    if (!animInitState("onesec", &blinkerOneSec, NULL))
        utLog("%s: failed to init animstate for animation 'onesec'",
              __FUNCTION__);
    else
        animQueAdd(&blinkerQue, &blinkerOneSec);

    if (!animInitState("halfsec", &blinkerHalfSec, NULL))
        utLog("%s: failed to init animstate for animation 'halfsec'",
              __FUNCTION__);
    else
        animQueAdd(&blinkerQue, &blinkerHalfSec);

    if (!animInitState("qtrsec", &blinkerQtrSec, NULL))
        utLog("%s: failed to init animstate for animation 'qtrsec'",
              __FUNCTION__);
    else
        animQueAdd(&blinkerQue, &blinkerQtrSec);

    return;
}
