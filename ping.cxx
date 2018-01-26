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
#include "conf.h"
#include "packet.h"
#include "client.h"
#include "ping.h"

static bool pingPending = false;
static uint32_t pingSentTime = 0;

static bool pingEnabled = false;

void pingEnable(bool enable)
{
    pingEnabled = enable;
}

void pingSend(uint32_t iternow)
{
    if (pingEnabled && !pingPending && (iternow - pingSentTime) > pingTime)
    {
        pingPending = true;
        pingSentTime = iternow;
        sendCommand(CPCMD_PING, 0);
    }

    return;
}

void pingResponse(uint32_t iternow)
{
    if (!pingPending)
        return;

    pingPending = false;
    uint32_t totalTime = iternow - pingSentTime;
    totalTime /= 2; // divide by 2 to get one way time
    // avg one-way trip time
    pktStats.pingAvg = (pktStats.pingAvg + totalTime) / 2;

    return;
}
