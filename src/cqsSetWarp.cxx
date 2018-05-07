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

#include <string>
#include "format.h"

#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "global.h"

#include "conf.h"
#include "conqinit.h"
#include "conqutil.h"

#include "cqsound.h"

// only defined if sound support is available, else the fake define
// in cqsound.h is used.
#ifndef CQS_NO_SOUND

#define WARP_UP     0
#define WARP_DOWN   1

void cqsSetWarp(real warp)
{
    static cqsHandle warpHandle = CQS_INVHANDLE;
    static cqsHandle engineHandle = CQS_INVHANDLE;
    static int warpufx = -1;
    static int warpdfx = -1;
    static int enginefx = -1;
    real dwarp = cbShips[Context.snum].dwarp;
    static int lastwarpdir = -1;
    int warpdir;
    static real lastwarp = 0;

    // this logic will have to change if we ever support team-specific
    // engine sounds... Right now, there is only one set of engine
    // sounds that all teams use.
    if (warpufx == -1)
    {
        std::string buf =
            fmt::format("ship{}-warp-up",
                        cbTeams[cbShips[Context.snum].team].teamchar);
        warpufx = cqsFindEffect(buf);
    }

    if (warpdfx == -1)
    {
        std::string buf =
            fmt::format("ship{}-warp-down",
                        cbTeams[cbShips[Context.snum].team].teamchar);
        warpdfx = cqsFindEffect(buf);
    }

    if (enginefx == -1)
    {
        enginefx = cqsFindEffect("engines");
    }

    /* first, the engine sounds */
    if (warp > 0)
    {
        if (engineHandle == CQS_INVHANDLE)
        {                       /* start it */
            cqsEffectPlay(enginefx, &engineHandle, 0.0, 0.0, 0.0);
        }
    }
    else
    {                           /* stop it */
        if (engineHandle != CQS_INVHANDLE)
        {
            cqsEffectStop(engineHandle, false);
            engineHandle = CQS_INVHANDLE;
        }
    }


    /* figure out where we are heading */
#if 0
    utLog ("warp = %f, dwarp %f lastwarp %f", warp, dwarp, lastwarp);
#endif

    if (warp == dwarp || warp <= 0.0 || dwarp < 0 ||
        warp == maxwarp(Context.snum))
    {                           /* we are where we want to be */
        if (warpHandle != CQS_INVHANDLE)
            cqsEffectStop(warpHandle, false);
        warpHandle = CQS_INVHANDLE;
        lastwarpdir = -1;
        lastwarp = warp;
        return;
    }

    if (warp < dwarp)
    {
        /* we need to do an extra check here for entering orbit */
        if (warp < lastwarp) /* we're really decelerating into orbit */
            warpdir = WARP_DOWN;
        else
            warpdir = WARP_UP;
    }
    else
        warpdir = WARP_DOWN;

    lastwarp = warp;

    if (warpHandle != CQS_INVHANDLE)
    {                           /* we are still playing one */
        /* we need to see if the direction has changed,
           if so, we need to stop the current effect and start the
           'other one' */

        if (warpdir != lastwarpdir)
        {
            cqsEffectStop(warpHandle, false);
            warpHandle = CQS_INVHANDLE;

            if (warpdir == WARP_UP)
                cqsEffectPlay(warpufx, &warpHandle, 0.0, 0.0, 0.0);
            else if (warpdir == WARP_DOWN)
                cqsEffectPlay(warpdfx, &warpHandle, 0.0, 0.0, 0.0);

            lastwarpdir = warpdir;
        }

        return;
    }
    else
    {                           /* we need to start one */
        if (warpdir == WARP_UP)
            cqsEffectPlay(warpufx, &warpHandle, 0.0, 0.0, 0.0);
        else if (warpdir == WARP_DOWN)
            cqsEffectPlay(warpdfx, &warpHandle, 0.0, 0.0, 0.0);

        lastwarpdir = warpdir;
    }

    return;
}

#endif /* !CQS_NO_SOUND */
