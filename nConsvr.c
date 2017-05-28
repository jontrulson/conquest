/*
 * server connect node
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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

static int err = FALSE;

#define ERR_BUFSZ 128
static char errbuf1[ERR_BUFSZ];
static char errbuf2[ERR_BUFSZ];

static int nConsvrDisplay(dspConfig_t *);
static int nConsvrIdle(void);
static int nConsvrInput(int ch);

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
    err = FALSE;

    setNode(&nConsvrNode);

    return;
}

static int nConsvrDisplay(dspConfig_t *dsp)
{
    int lin;

    if (!rhost)
        return NODE_EXIT;

    /* display the logo */
    mglConqLogo(dsp, FALSE);

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

    return NODE_OK;
}

static int nConsvrIdle(void)
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
    if (!cInfo.serverDead)
    {
        utLog("nConsvrIdle: Already connected.  Ready to transfer to Auth node.");
        return NODE_OK;
    }

    if ((hp = gethostbyname(rhost)) == NULL)
    {
        utLog("conquest: %s: no such host\n", rhost);

        snprintf(errbuf1, sizeof(errbuf1) - 1, "%s: no such host",
                 rhost);
        err = TRUE;

        return NODE_ERR;
    }

    /* put host's address and address type into socket structure */
    memcpy((char *)&sa.sin_addr, (char *)hp->h_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;

    sa.sin_port = htons(rport);

    if ((s = socket(AF_INET, SOCK_STREAM, 0 )) < 0)
    {
        utLog("socket: %s", strerror(errno));
        snprintf(errbuf1, sizeof(errbuf1) - 1, "socket: %s", rhost);
        err = TRUE;

        return NODE_ERR;
    }

    if (cInfo.tryUDP)
    {
        utLog("NET: Opening UDP...");
        if ((cInfo.usock = udpOpen(0, &cInfo.servaddr)) < 0)
        {
            utLog("NET: udpOpen: %s", strerror(errno));
            cInfo.tryUDP = FALSE;
        }
    }

    utLog("Connecting to host: %s, port %d\n",
          rhost, rport);

    /* connect to the remote server */
    if (connect(s, (const struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        utLog("connect %s:%d: %s",
              rhost, rport, strerror(errno));

        snprintf(errbuf1, sizeof(errbuf1) - 1, "connect %s:%d: %s",
                 rhost, rport, strerror(errno));
        snprintf(errbuf2, sizeof(errbuf2) - 1,
                 "Is there a conquestd server running there?");

        err = TRUE;

        return NODE_ERR;
    }

    cInfo.serverDead = FALSE;
    cInfo.sock = s;
    cInfo.servaddr = sa;

    pktSetSocketFds(cInfo.sock, PKT_SOCKFD_NOCHANGE);
    pktSetNodelay();

    if (!clientHello(CONQUESTGL_NAME))
    {
        utLog("conquestgl: hello() failed\n");
        printf("conquestgl: hello() failed, check log\n");

        cInfo.serverDead = TRUE;
        return NODE_EXIT;
    }

    nAuthInit();                  /* transfer to Auth */

    return TRUE;
}

static int nConsvrInput(int ch)
{
    if (err)                      /* then we just exit */
        return NODE_EXIT;

    return NODE_OK;
}
