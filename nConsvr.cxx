/*
 * server connect node
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

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

    lin = 12;

    cprintf(lin++, 0, ALIGN_CENTER, "Connecting to %s:%d",
            rhost, rport);


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
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;

    /* no point in trying again... */
    if (err)
        return NODE_ERR;

    if (!rhost)
        return NODE_EXIT;

    /* should not happen - debugging */
    if (!serverDead)
    {
        utLog("nConsvrIdle: Already connected.  Ready to transfer to Auth node.");
        return NODE_OK;
    }

    if ((hp = gethostbyname(rhost)) == NULL)
    {
        utLog("conquest: %s: no such host\n", rhost);

        snprintf(errbuf1, ERR_BUFSZ, "%s: no such host",
                 rhost);
        err = true;

        return NODE_ERR;
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

        return NODE_ERR;
    }

    utLog("Connecting to host: %s, port %d\n",
          rhost, rport);

    /* connect to the remote server */
    if (connect(s, (const struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        utLog("connect %s:%d: %s",
              rhost, rport, strerror(errno));

        snprintf(errbuf1, ERR_BUFSZ, "connect %s:%d: %s",
                 rhost, rport, strerror(errno));
        snprintf(errbuf2, ERR_BUFSZ,
                 "Is there a conquestd server running there?");

        err = true;

        return NODE_ERR;
    }

    serverDead = false;
    cInfo.sock = s;
    cInfo.servaddr = sa;

    pktSetSocketFds(cInfo.sock, PKT_SOCKFD_NOCHANGE);
    pktSetNodelay();

    if (!clientHello(CONQUEST_NAME))
    {
        utLog("%s: hello() failed", __FUNCTION__);
        printf("%s: hello() failed, check log\n", __FUNCTION__);

        serverDead = true;
        return NODE_EXIT;
    }

    nAuthInit();                  /* transfer to Auth */

    return NODE_OK;
}

static nodeStatus_t nConsvrInput(int ch)
{
    if (err)                      /* then we just exit */
        return NODE_EXIT;

    return NODE_OK;
}
