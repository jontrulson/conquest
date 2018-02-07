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
#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "nConsvr.h"
#include "nAuth.h"
#include "udp.h"
#include "glmisc.h"
#include "conqutil.h"
#include "conquest.h"

static char *rhost = NULL;
static uint16_t rport;

static int err = false;

#define ERR_BUFSZ 128
static char errbuf1[ERR_BUFSZ] = {};
static char errbuf2[ERR_BUFSZ] = {};

static bool serverDead = true;

static int s = -1; // socket
static struct sockaddr_in sa;
static struct hostent *hp = NULL;

static bool isConnecting = false;
static const char *abortStr = "--- press any key to abort ---";
static nodeStatus_t nConsvrDisplay(dspConfig_t *);
static nodeStatus_t nConsvrIdle(void);
static nodeStatus_t nConsvrInput(int ch);

static scrNode_t nConsvrNode = {
    nConsvrDisplay,               /* display */
    nConsvrIdle,                  /* idle */
    nConsvrInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};


void nConsvrInit(char *remotehost, uint16_t remoteport)
{
    rhost = remotehost;
    rport = remoteport;
    errbuf1[0] = 0;
    errbuf2[0] = 0;
    err = false;
    serverDead = true;

    // start the ball rolling...
    if ((hp = gethostbyname(rhost)) == NULL)
    {
        utLog("conquest: %s: no such host\n", rhost);

        snprintf(errbuf1, ERR_BUFSZ, "%s: no such host",
                 rhost);
        err = true;

        return;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons(rport);

    if ((s = socket(AF_INET, SOCK_STREAM, 0 )) < 0)
    {
        utLog("socket: %s", strerror(errno));
        snprintf(errbuf1, ERR_BUFSZ, "socket: %s", rhost);
        err = true;

        return;
    }

    utLog("Connecting to host: %s, port %d\n", rhost, rport);

    pktSetNonBlocking(s, true);

    // let the good times roll...
    setNode(&nConsvrNode);

    return;
}

static nodeStatus_t nConsvrDisplay(dspConfig_t *dsp)
{
    int lin;

    if (!rhost)
        return NODE_EXIT;

    /* display the logo */
    mglConqLogo(dsp, false);

    lin = 10;

    cprintf(lin++, 0, ALIGN_CENTER, "Connecting to %s:%d",
            rhost, rport);
    lin++;

    if (isConnecting && !err)
    {
        cprintf( MSG_LIN2, 0, ALIGN_CENTER,"#%d#%s", InfoColor, abortStr);
    }

    if (err)
    {
        if (strlen(errbuf1))
            cprintf(lin++, 0, ALIGN_CENTER, "%s", errbuf1);

        if (strlen(errbuf2))
            cprintf(lin++, 0, ALIGN_CENTER, "%s", errbuf2);

        cprintf( MSG_LIN2, 0, ALIGN_CENTER,"#%d#%s", InfoColor, MTXT_DONE);

    }

    return NODE_OK_NO_PKTPROC;
}

static nodeStatus_t nConsvrIdle(void)
{
    if (serverDead && !err)
    {
        isConnecting = true;

        /* connect to the remote server */
        if (connect(s, (const struct sockaddr *)&sa, sizeof(sa)) < 0)
        {
            if (!(errno == EWOULDBLOCK || errno == EAGAIN
                  || errno == EINPROGRESS || errno == EALREADY))
            {

                utLog("connect %s:%d: %s",
                      rhost, rport, strerror(errno));

                snprintf(errbuf1, ERR_BUFSZ, "connect %s:%d: (%d)%s",
                         rhost, rport, errno, strerror(errno));
                snprintf(errbuf2, ERR_BUFSZ,
                         "Is there a conquestd server running there?");

                err = true;

                return NODE_OK;
            }
        }
        else
        {
            // connected...

            isConnecting = false;

            pktSetNonBlocking(s, false);

            serverDead = false;
            cInfo.sock = s;
            cInfo.servaddr = sa;

            pktSetSocketFds(cInfo.sock, PKT_SOCKFD_NOCHANGE);
            pktSetNodelay();
            if (!clientHello(CONQUEST_NAME))
            {
                utLog("%s: clientHello() failed", __FUNCTION__);
                snprintf(errbuf1, ERR_BUFSZ,
                         "Negotiation with server failed (clientHello())");
                snprintf(errbuf2, ERR_BUFSZ,
                         "Is there a Conquest server running there?");
                serverDead = true;
                err = true;
                return NODE_OK;
            }

            nAuthInit();                  /* transfer to Auth */

            return NODE_OK;
        }
    }

    return NODE_OK;
}

static nodeStatus_t nConsvrInput(int ch)
{
    if (isConnecting)
    {
        // clean up
        serverDead = true;
        if (s >= 0)
            close(s);
        s = -1;

        return NODE_EXIT;
    }

    if (err)                      /* then we just exit */
        return NODE_EXIT;

    return NODE_OK;
}
