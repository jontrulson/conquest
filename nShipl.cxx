/*
 * Ship list
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"
#include "record.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nPlayB.h"
#include "nPlayBMenu.h"
#include "nShipl.h"

static const char *hd2="ship  name          pseudonym              kills     type";
static int fship;

static int nShiplDisplay(dspConfig_t *);
static int nShiplIdle(void);
static int nShiplInput(int ch);

static scrNode_t nShiplNode = {
    nShiplDisplay,               /* display */
    nShiplIdle,                  /* idle */
    nShiplInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

static int retnode;             /* the node to return to */

scrNode_t *nShiplInit(int nodeid, int setnode)
{
    retnode = nodeid;

    if (setnode)
        setNode(&nShiplNode);

    return(&nShiplNode);
}


static int nShiplDisplay(dspConfig_t *dsp)
{
    int snum = Context.snum;
    static const int doall = FALSE; /* for now... */
    static char cbuf[BUFFER_SIZE_256];
    int i, unum, status, lin, col;
    killedBy_t kb;
    uint16_t detail;
    int fline, lline;
    char sbuf[20];
    char kbuf[20];
    char pidbuf[20];
    char ubuf[MAXUSERNAME + 2];
    int color;

    strcpy(cbuf , hd2) ;

    col = (int)(Context.maxcol - strlen( cbuf )) / (int)2;
    lin = 2;
    cprintf(lin, col, ALIGN_NONE, "#%d#%s", LabelColor, cbuf);

    for ( i = 0; cbuf[i] != 0; i = i + 1 )
        if ( cbuf[i] != ' ' )
            cbuf[i] = '-';
    lin = lin + 1;
    cprintf(lin, col, ALIGN_NONE, "#%d#%s", LabelColor, cbuf);

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */

    i = fship;

    lin = fline;
    while ( i < MAXSHIPS && lin <= lline )
    {
        status = cbShips[i].status;

        kb = cbShips[i].killedBy;
        detail = cbShips[i].killedByDetail;
        if ( status == SS_LIVE ||
             ( doall && ( status != SS_OFF || kb != KB_NONE ) ) )
        {
            sbuf[0] = 0;
            utAppendShip(sbuf , i) ;
            strcat(sbuf, " ") ;
            utAppendChar(sbuf, cbShipTypes[cbShips[i].shiptype].name[0]) ;

            unum = cbShips[i].unum;
            if ( unum >= 0 && unum < MAXUSERS )
            {
                if (SROBOT(i)) /* robot */
                    strcpy(pidbuf, " ROBOT");
                else if (SVACANT(i))
                    strcpy(pidbuf, "VACANT");
                else
                    strcpy(pidbuf, "  LIVE");

                strcpy(ubuf, cbUsers[unum].username);

                sprintf(kbuf, "%6.1f", (cbShips[i].kills + cbShips[i].strkills));
                sprintf( cbuf, "%-5s %-13.13s %-21.21s %-8s %6s",
                         sbuf, ubuf, cbShips[i].alias,
                         kbuf, pidbuf );
            }
            else
                sprintf( cbuf, "%-5s %13s %21s %8s %6s", sbuf,
                         " ", " ", " ", " " );
            if ( doall && kb != KB_NONE )
            {
                strcat(cbuf, "  ") ;
                utAppendKilledBy(cbuf, kb, detail) ;
            }

            if (snum >= 0 && snum < MAXSHIPS )
            {		/* a normal ship view */
                if ( i == snum )    /* it's ours */
                    color = NoColor | CQC_A_BOLD;
                else if (satwar(i, snum)) /* we're at war with it */
                    color = RedLevelColor;
                else if (cbShips[i].team == cbShips[snum].team && !selfwar(snum))
                    color = GreenLevelColor; /* it's a team ship */
                else
                    color = YellowLevelColor;
            }
            else
            { /* not conqoper, and not a valid ship (main menu) */
                if (cbUsers[Context.unum].war[cbShips[i].team])  /* we're at war with ships's
                                                                team */
                    color = RedLevelColor;
                else if (cbUsers[Context.unum].team == cbShips[i].team)
                    color = GreenLevelColor; /* it's a team ship */
                else
                    color = YellowLevelColor;
            }

            cprintf(lin, col, ALIGN_NONE, "#%d#%s", color, cbuf);

            if ( doall && status != SS_LIVE )
            {
                cbuf[0] = 0;
                utAppendShipStatus(cbuf , status) ;

                cprintf(lin, col - 2 - strlen( cbuf ),
                        ALIGN_NONE, "#%d#%s", YellowLevelColor, cbuf);
            }
        }
        i = i + 1;
        lin = lin + 1;
    }

    cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);

    return NODE_OK;
}

static int nShiplIdle(void)
{
    int pkttype;
    char buf[PKT_MAXSIZE];

    if (Context.recmode == RECMODE_PLAYING || Context.recmode == RECMODE_PAUSED)
        return NODE_OK;             /* no packet reading here */

    while ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                       buf, PKT_MAXSIZE, 0, NULL)) > 0)
        processPacket(buf);

    if (pkttype < 0)          /* some error */
    {
        utLog("nShiplIdle: waiForPacket returned %d", pkttype);
        cbShips[Context.snum].status = SS_OFF;
        return NODE_EXIT;
    }

    if (clientFlags & SPCLNTSTAT_FLAG_KILLED && retnode == DSP_NODE_CP)
    {
        /* time to die properly. */
        setONode(NULL);
        nDeadInit();
        return NODE_OK;
    }


    return NODE_OK;
}

static int nShiplInput(int ch)
{
    /* go back */
    switch (retnode)
    {
    case DSP_NODE_CP:
        setONode(NULL);
        nCPInit(FALSE);
        break;
    case DSP_NODE_MENU:
        setONode(NULL);
        nMenuInit();
        break;

    case DSP_NODE_PLAYBMENU:
        setONode(NULL);
        nPlayBMenuInit();
        break;

    case DSP_NODE_PLAYB:
        setONode(NULL);
        nPlayBInit();
        break;

    default:
        utLog("nShiplInput: invalid return node: %d, going to DSP_NODE_MENU",
              retnode);
        setONode(NULL);
        nMenuInit();
        break;
    }

    /* NOTREACHED */
    return NODE_OK;
}
