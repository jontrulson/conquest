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
#include "GL.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "nMeta.h"
#include "nConsvr.h"
#include "gldisplay.h"
#include "glmisc.h"
#include "cqkeys.h"
#include "conqutil.h"

static const char *header = "Server List";
static const char *header2fmt = "(Page %d of %d)";
static char headerbuf[BUFFER_SIZE_256];
static char header2buf[BUFFER_SIZE_256];
static const char *eprompt = "Arrow keys to select, [TAB] or [ENTER] to accept, any other key to quit.";

static const int servers_per_page = 10;
static int flin, llin, clin, pages, curpage;

struct _srvvec {
    uint16_t vers;
    char hostname[MAXHOSTNAME + MAXPORTNAME];
};

static struct _srvvec servervec[META_MAXSERVERS] = {};

static nodeStatus_t nMetaDisplay(dspConfig_t *);
static nodeStatus_t nMetaInput(int ch);

static int numMetaServers;   /* number of servers in metaServerList */

static metaSRec_t *metaServerList;   /* list of servers */

static scrNode_t nMetaNode = {
    nMetaDisplay,                 /* display */
    NULL,                         /* idle */
    nMetaInput,                   /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */

};


static void dispServerInfo(dspConfig_t *dsp, metaSRec_t *metaServerList,
                           int num)
{
    static char buf1[BUFFER_SIZE_256];
    static char buf2[BUFFER_SIZE_256];
    static char buf3[BUFFER_SIZE_256];
    static char buf5[BUFFER_SIZE_256];
    static char buf6[BUFFER_SIZE_256];
    static char buf7[BUFFER_SIZE_256];
    static char pbuf1[BUFFER_SIZE_256];
    static char pbuf2[BUFFER_SIZE_256];
    static char pbuf3[BUFFER_SIZE_256];
    static char pbuf4[BUFFER_SIZE_256];
    static char pbuf5[BUFFER_SIZE_256];
    static char pbuf6[BUFFER_SIZE_256];
    static char pbuf7[BUFFER_SIZE_256];
    GLfloat x, y, w, h;
    static int inited = false;
    static const int hcol = 2, icol = 11;
    int tlin = 3;

    tlin = 3;
    x = dsp->ppCol;
    y = (dsp->ppRow * tlin);
    w = (dsp->wW - (dsp->ppCol * 3.0));
    h = (dsp->ppRow * 9.2);

    if (!inited)
    {
        inited = true;
        sprintf(pbuf1, "#%d#Server: ", MagentaColor);
        sprintf(buf1, "#%d#%%s", NoColor);

        sprintf(pbuf2, "#%d#Version: ", MagentaColor);
        sprintf(buf2, "#%d#%%s (Protocol: 0x%%04x)", NoColor);

        sprintf(pbuf3, "#%d#Status: ", MagentaColor);
        sprintf(buf3,
                "#%d#Ships #%d#%%d/%%d #%d#"
                "(#%d#%%d #%d#active, #%d#%%d #%d#vacant, "
                "#%d#%%d #%d#robot)",
                NoColor, CyanColor, NoColor,
                CyanColor, NoColor, CyanColor, NoColor,
                CyanColor, NoColor);

        sprintf(pbuf4, "#%d#Flags: ", MagentaColor);

        sprintf(pbuf5, "#%d#MOTD: ", MagentaColor);
        sprintf(buf5, "#%d#%%s", NoColor);

        sprintf(pbuf6, "#%d#Contact: ", MagentaColor);
        sprintf(buf6, "#%d#%%s", NoColor);

        sprintf(pbuf7, "#%d#Time: ", MagentaColor);
        sprintf(buf7, "#%d#%%s", NoColor);
    }

    cprintf(tlin, hcol, ALIGN_NONE, pbuf1);
    cprintf(tlin++, icol, ALIGN_NONE, buf1, metaServerList[num].servername);

    cprintf(tlin, hcol, ALIGN_NONE, pbuf2);
    cprintf(tlin++, icol, ALIGN_NONE, buf2, metaServerList[num].serverver,
        metaServerList[num].protovers);

    cprintf(tlin, hcol, ALIGN_NONE, pbuf3);
    cprintf(tlin++, icol, ALIGN_NONE, buf3,
            (metaServerList[num].numactive + metaServerList[num].numvacant +
             metaServerList[num].numrobot),
            metaServerList[num].numtotal,
            metaServerList[num].numactive,
            metaServerList[num].numvacant, metaServerList[num].numrobot);

    cprintf(tlin, hcol, ALIGN_NONE, pbuf4);

    tlin = clntPrintServerFlags(tlin, icol, metaServerList[num].flags, NoColor);

    tlin++;

    cprintf(tlin, hcol, ALIGN_NONE, pbuf5);
    cprintf(tlin++, icol, ALIGN_NONE, buf5, metaServerList[num].motd);

    cprintf(tlin, hcol, ALIGN_NONE, pbuf6);
    cprintf(tlin++, icol, ALIGN_NONE, buf6, metaServerList[num].contact);

    cprintf(tlin, hcol, ALIGN_NONE, pbuf7);
    cprintf(tlin++, icol, ALIGN_NONE, buf7, metaServerList[num].walltime);

    drawLineBox(x, y, 0.0, w, h, CyanColor, 2.0);

    return;
}


void nMetaInit(void)
{
    int i;

    /* get the server list */
    utLog("nMetaInit: Querying metaserver at %s", cInfo.metaServer);
    numMetaServers = metaGetServerList(cInfo.metaServer,
                                       &metaServerList);

    if (numMetaServers < 0)
    {
        utLog("nMetaInit: metaGetServerList() failed");
        return;
    }

    if (numMetaServers == 0)
    {
        utLog("nMetaInit: metaGetServerList() reported 0 servers online");
        return;
    }

    utLog("nMetaInit: Found %d server(s)", numMetaServers);

    /* this is the number of required pages,
       though page accesses start at 0 */

    if (numMetaServers >= servers_per_page)
    {
        pages = numMetaServers / servers_per_page;
        if ((numMetaServers % servers_per_page) != 0)
            pages++;                /* for runoff */
    }
    else
        pages = 1;

    /* init the servervec array */
    for (i=0; i < numMetaServers; i++)
    {
        if (metaServerList[i].version >= 2) /* valid for newer meta protocols */
            servervec[i].vers = metaServerList[i].protovers;
        else
            servervec[i].vers = 0; /* always 'incompatible' */

        snprintf(servervec[i].hostname, (MAXHOSTNAME + MAXPORTNAME), "%s:%hu",
                 metaServerList[i].altaddr,
                 metaServerList[i].port);
    }

    curpage = 0;

    flin = 12;			/* first server line */
    llin = 0;			/* last server line on this page */
    clin = 0;			/* current server line */

    setNode(&nMetaNode);

    return;
}

static nodeStatus_t nMetaDisplay(dspConfig_t *dsp)
{
    int i, k;
    char *dispmac;
    int lin;
    int col;

    sprintf(header2buf, header2fmt, curpage + 1, pages);
    sprintf(headerbuf, "%s %s", header, header2buf);

    lin = 1;

    cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", NoColor, headerbuf);

    lin = flin;
    col = 1;

				/* figure out the last editable line on
				   this page */

    if (curpage == (pages - 1)) /* last page - might be less than full */
        llin = (numMetaServers % servers_per_page); /* ..or more than empty? ;-) */
    else
        llin = servers_per_page;

    i = 0;                      /* start at index 0 */

    while (i < llin)
    {			/* display this page */
        /* get the server number for this line */
        k = (curpage * servers_per_page) + i;

        dispmac = servervec[k].hostname;

        /* highlight the currently selected line */
        cqColor servColor;
        cqColor dataColor;
        if (i == clin)
        {
            servColor = RedLevelColor;
            dataColor = BlueColor | CQC_A_BOLD;
        }
        else
        {
            servColor = InfoColor;
            dataColor = BlueColor;
        }

        if (servervec[k].vers == PROTOCOL_VERSION)
            cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d#",
                    servColor, dispmac, NoColor);
        else
            cprintf(lin, col, ALIGN_NONE,
                    "#%d#%s#%d# (incompatible protocol version 0x%04x)",
                    dataColor, dispmac, NoColor,
                    servervec[k].vers);

        lin++;
        i++;
    }

    cprintf(MSG_LIN1, 1, ALIGN_NONE, eprompt);

    if (clin >= llin)
        clin = llin - 1;

    dispServerInfo(dsp, metaServerList, clin);

    // We are not connected to a server, so don't let the nod renderer
    // try to read/process packets.
    return NODE_OK_NO_PKTPROC;
}



static nodeStatus_t nMetaInput(int ch)
{
    int i;

    ch = CQ_CHAR(ch) | CQ_FKEY(ch);

    switch(ch)
    {
    case CQ_KEY_UP:		/* up */
    case CQ_KEY_LEFT:
    case 'w':
    case 'k':
        clin--;
        if (clin < 0)
        {
            if (pages != 1)
            {
                curpage--;
                if (curpage < 0)
                {
                    curpage = pages - 1;
                }
            }

            /* setup llin  for current page */
            if (curpage == (pages - 1))
                llin = (numMetaServers % servers_per_page);
            else
                llin = servers_per_page;

            clin = llin - 1;
        }
        break;

    case CQ_KEY_DOWN:		/* down */
    case CQ_KEY_RIGHT:
    case 'x':
    case 'j':
        clin++;
        if (clin >= llin)
        {
            if (pages != 1)
            {
                curpage++;
                if (curpage >= pages)
                {
                    curpage = 0;
                }
            }

            clin = 0;
        }
        break;

    case CQ_KEY_PAGE_UP:		/* prev page */
        if (pages != 1)
        {
            curpage--;
            if (curpage < 0)
            {
                curpage = pages - 1;
            }
        }

        break;

    case CQ_KEY_PAGE_DOWN:		/* next page */
        if (pages != 1)
        {
            curpage++;
            if (curpage >= pages)
            {
                curpage = 0;
            }
        }

        break;

    case TERM_NORMAL:	/* selected one */
    case TERM_EXTRA:

        if (cInfo.remotehost)
        {
            free(cInfo.remotehost);
            cInfo.remotehost = NULL;
        }

        i = (curpage * servers_per_page) + clin;

        if ((metaServerList[i].protovers == PROTOCOL_VERSION) ||
            metaServerList[i].version < 2) /* too old to know for sure */
        {
            if ((cInfo.remotehost = strdup(metaServerList[i].altaddr)) == NULL)
            {
                utLog("strdup(metaServerList[i]) failed");
                return NODE_EXIT;
            }
            cInfo.remoteport = metaServerList[i].port;

            /* transfer to the Consvr node */
            nConsvrInit(cInfo.remotehost, cInfo.remoteport);
        }
        else
            mglBeep(MGL_BEEP_ERR);

        break;

    default:		/* everything else */
        return NODE_EXIT;
        break;
    }

    return NODE_OK;
}
