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


#include "c_defs.h"
#include "conqdef.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "cb.h"
#include "conqlb.h"
#include "rndlb.h"
#include "conqutil.h"
#include "ibuf.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "textures.h"
#include "gldisplay.h"

#define NOEXTERN_ANIM
#include "anim.h"
#undef NOEXTERN_ANIM

#include "node.h"
#include "conf.h"
#include "record.h"

/* init an animstate.  can specify an optional istate drawing state */
int animInitState(const char *animname, animStatePtr_t astate,
                  animDrawStatePtr_t istate)
{
    int ndx;

    if (!animname || !astate)
        return false;

    /* first get the animdef index */
    if ((ndx = findGLAnimDef(animname)) < 0)
    {
        utLog("%s: could not find animdef for animation '%s'",
              __FUNCTION__, animname);
        return false;
    }

    memset((void *)astate, 0, sizeof(animStateRec_t));

    astate->adIndex = ndx;
    astate->flags = 0;          /* not currently used */

    if (istate)                 /* initial state override */
        astate->istate = *istate;
    else
    {                         /* if there are cqi istates, use them. */
        if (GLAnimDefs[ndx].istates & AD_ISTATE_TEX)
            astate->istate.id = GLAnimDefs[ndx].itexid;
        else
        {            /* if no texanim, and texid is valid, use it */
            if (GLAnimDefs[ndx].texid)
                astate->istate.id = GLAnimDefs[ndx].texid;
        }

        if (GLAnimDefs[ndx].istates & AD_ISTATE_COL)
            astate->istate.col = GLAnimDefs[ndx].icolor;

        if (GLAnimDefs[ndx].istates & AD_ISTATE_SZ)
            astate->istate.size = GLAnimDefs[ndx].isize;

        if (GLAnimDefs[ndx].istates & AD_ISTATE_ANG)
            astate->istate.angle = GLAnimDefs[ndx].iangle;
    }

    /* if no istate was specified, then we will build it up below as we
       handle each section in turn. */

    astate->anims = GLAnimDefs[ndx].anims;
    astate->expired = 0;        /* initialized in animResetState() */

    /* texanim */
    if (astate->anims & CQI_ANIMS_TEX)
    {
        if (!istate && !(GLAnimDefs[ndx].istates & AD_ISTATE_TEX))
        {
            astate->istate.id = GLAnimDefs[ndx].tex.tex[0].id;
            astate->istate.col = GLAnimDefs[ndx].tex.tex[0].col;
        }

        /* setup default s/t offsets */
        astate->istate.tc.s = 0.0;
        astate->istate.tc.t = 0.0;
    }

    /* colanim */
    if (astate->anims & CQI_ANIMS_COL)
    {
        if (!istate && !(astate->anims & CQI_ANIMS_TEX) &&
            !(GLAnimDefs[ndx].istates & AD_ISTATE_COL))
            astate->istate.col = GLAnimDefs[ndx].col.color;
    }

    /* the rest handled by animResetState(). */

    animResetState(astate, frameTime);

    return true;
}

/* reset's an anim state.  Hopefully you called animInitState() at least once
   on the state first, before calling this function.
   When complete, the state is suitable to be run. */
void animResetState(animStatePtr_t astate, uint32_t thetime)
{
    int ndx;

    if (!astate)
        return;

    ndx = astate->adIndex;        /* hope it's correct... */

    /* set the starting time in case there is a time limit */
    astate->starttime = thetime;

    /* first reset the draw state from initial draw state - we only copy the
       non private items - explicitly */
    astate->state.id    = astate->istate.id;
    astate->state.tc.s  = astate->istate.tc.s;
    astate->state.tc.t  = astate->istate.tc.t;
    astate->state.col   = astate->istate.col;
    astate->state.x     = astate->istate.x;
    astate->state.y     = astate->istate.y;
    astate->state.z     = astate->istate.z;
    astate->state.angle = astate->istate.angle;
    astate->state.size  = astate->istate.size;

    /* need to check for a negative angle.  If so, gen a
       random one. */
    if (astate->state.angle < 0.0)
        astate->state.angle = rnduni( 0.0, 360.0 );

    /* setup exired so that non-existant anim types are already 'pre-expired' */
    astate->expired = (~(astate->anims & CQI_ANIMS_MASK) & CQI_ANIMS_MASK);

    /* reset the counters, loops, timers, etc */

    /* texanim */
    if (astate->anims & CQI_ANIMS_TEX)
    {
        astate->tex.flags = 0; /* not currently used */
        astate->tex.lasttime = thetime;
        astate->tex.curstage = 0;
        astate->tex.curloop = GLAnimDefs[ndx].tex.loops;
    }

    /* colanim */
    if (astate->anims & CQI_ANIMS_COL)
    {
        astate->col.flags = 0;
        astate->col.lasttime = thetime;
        astate->col.curstage = 0;
        astate->col.curloop = GLAnimDefs[ndx].col.loops;
    }

    /* geoanim */
    if (astate->anims & CQI_ANIMS_GEO)
    {
        astate->geo.flags = 0;
        astate->geo.lasttime = thetime;
        astate->geo.curstage = 0;
        astate->geo.curloop = GLAnimDefs[ndx].geo.loops;
    }

    /* toganim */
    if (astate->anims & CQI_ANIMS_TOG)
    {
        astate->tog.flags = 0;
        astate->tog.lasttime = thetime;
    }

    return;
}

/* the guts really.  iterate a state according to it's animdef, possibly
   expiring it */
int animIterState(animStatePtr_t astate)
{
    static real lastfdelay = -1.0; /* last frame delay value (REC/PLYBK) */
    static real timescale = 1.0;
    GLAnimDef_t *glad;

    if (!astate)                  /* shouldn't happen */
        return false;

    glad = &GLAnimDefs[astate->adIndex];

    /* fix up our recFrameDelay in case we are playing back a recording */
    if (Context.recmode == RECMODE_PLAYING || Context.recmode == RECMODE_PAUSED)
    {              /* it changed, re-adjust our timing for playback */
        if (lastfdelay != recFrameDelay)
        {
            lastfdelay = recFrameDelay;
            timescale = (recFrameDelay * 10.0); /* 100ms = 1X */
#if 0
            utLog("SETTING TIMESCALE: recFrameDelay = %f, timescale = %f",
                  recFrameDelay, timescale);
#endif
        }
    }
    else
        timescale = 1.0;

    /* check if we have a time limit, if so, and we've exceeded it,
       expire */

    if (glad->timelimit)
        if ((frameTime - astate->starttime) > (glad->timelimit * timescale))
            astate->expired = CQI_ANIMS_MASK; /* expire them all */

    /* texanim */
    if (!(astate->expired & CQI_ANIMS_TEX))
    {
        /* check the time */
        if (!glad->tex.delayms ||
            (frameTime - astate->tex.lasttime) > (int)((real)glad->tex.delayms * timescale))
        {
            astate->tex.lasttime = frameTime;

            if (astate->tex.curstage + 1 >= glad->tex.stages)
            {
                if (glad->tex.loops)
                {
                    if (astate->tex.curloop - 1 <= 0)
                    {               /* we can do no more */
                        astate->expired |= CQI_ANIMS_TEX;
                    }
                    else
                    {
                        astate->tex.curloop--;
                        astate->tex.curstage = 0; /* reset it */
                    }
                }
                else              /* no loops, just reset stage */
                    astate->tex.curstage = 0; /* reset it */
            }
            else
                astate->tex.curstage++;

            /* now setup the state */
            if (!(astate->expired & CQI_ANIMS_TEX))
            {
                astate->state.id = glad->tex.tex[astate->tex.curstage].id;
                astate->state.col = glad->tex.tex[astate->tex.curstage].col;

                /* iter tc's */
                astate->state.tc.s += glad->tex.deltas;
                astate->state.tc.t += glad->tex.deltat;
            }
        }
    }

    /* colanim - this can only run if no texanim is running */
    if (!(astate->expired & CQI_ANIMS_COL) &&
        (astate->expired & CQI_ANIMS_TEX))
    {
        /* check the time */
        if (!glad->col.delayms ||
            (frameTime - astate->col.lasttime) > (int)((real)glad->col.delayms * timescale))
        {
            astate->col.lasttime = frameTime;

            if (glad->col.stages &&
                (astate->col.curstage + 1 >= glad->col.stages))
            {
                if (glad->col.loops)
                {
                    if (astate->col.curloop - 1 <= 0)
                    {               /* we can do no more */
                        astate->expired |= CQI_ANIMS_COL;
                    }
                    else
                    {
                        astate->col.curloop--;
                        astate->col.curstage = 0; /* reset it */
                    }
                }
            }
            else
                astate->col.curstage++;

            /* now setup the state */
            if (!(astate->expired & CQI_ANIMS_COL))
            {
                astate->state.col.a =
                    CLAMP(0.0, 1.0, astate->state.col.a + glad->col.deltaa);
                astate->state.col.r =
                    CLAMP(0.0, 1.0, astate->state.col.r + glad->col.deltar);
                astate->state.col.g =
                    CLAMP(0.0, 1.0, astate->state.col.g + glad->col.deltag);
                astate->state.col.b =
                    CLAMP(0.0, 1.0, astate->state.col.b + glad->col.deltab);
            }
        }
    }

    /* geoanim - this can only run if no texanim is running */
    if (!(astate->expired & CQI_ANIMS_GEO) &&
        (astate->expired & CQI_ANIMS_TEX))
    {
        /* check the time */
        if (!glad->geo.delayms ||
            (frameTime - astate->geo.lasttime) > (int)((real)glad->geo.delayms * timescale))
        {
            astate->geo.lasttime = frameTime;

            if (glad->geo.stages &&
                (astate->geo.curstage + 1 >= glad->geo.stages))
            {
                if (glad->geo.loops)
                {
                    if (astate->geo.curloop - 1 <= 0)
                    {               /* we can do no more */
                        astate->expired |= CQI_ANIMS_GEO;
                    }
                    else
                    {
                        astate->geo.curloop--;
                        astate->geo.curstage = 0; /* reset it */
                    }
                }
            }
            else
                astate->geo.curstage++;

            /* now setup the state */
            if (!(astate->expired & CQI_ANIMS_GEO))
            {

                astate->state.x += glad->geo.deltax;
                astate->state.y += glad->geo.deltay;
                astate->state.z += glad->geo.deltaz;
                astate->state.size += glad->geo.deltas;
                if (astate->state.size < 0.0)
                    astate->state.size = 0.0;

                /* angle is special */
                if (astate->istate.angle < 0.0)
                    astate->state.angle = rnduni( 0.0, 360.0 );
                else
                    astate->state.angle =
                        utMod360(astate->state.angle + glad->geo.deltar);
            }
        }
    }

    /* toganim */
    if (!(astate->expired & CQI_ANIMS_TOG))
    {
        /* check the time */
        /* when playing back at hi speed, it's annoying to have the blinker
           anims accelerated according to timescale.  So, if we are the only
           animtype in this animdef (standard blinker anim) then we do not
           rescale the time according to recFrameDelay.  */
        if (!glad->tog.delayms ||
            (frameTime - astate->tog.lasttime) >
            (int)((real)glad->tog.delayms *
                  ( ((astate->anims & CQI_ANIMS_MASK) == CQI_ANIMS_TOG) ?
                    1.0 : timescale) ))
        {
#if 0
            if (glad->tog.delayms == 500)
                utLog("%s: TOG RUN timescale = %f scaledelay = %d",
                      __FUNCTION__, timescale,
                      (int)((real)glad->tog.delayms * timescale));
#endif
            astate->tog.lasttime = frameTime;

            /* just set the state */
            astate->state.armed = !astate->state.armed;
        }
    }

    /* indicate whether we expired */
    return (ANIM_EXPIRED(astate) ? true : false);
}

/* run the list of que entries, removing thos that expire */
void animVecRun(animVec_t& avec)
{
    for (auto astate: avec)
        if (animIterState(astate))
        {
            // it expired, remove it
            avec.erase(std::remove(avec.begin(), avec.end(), astate),
                       avec.end());
        }

    return;
}
