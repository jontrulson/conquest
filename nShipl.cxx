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
static int shipOffset; /* offset into cbLimits.maxShips() for this page */
static int shipIdx;

static nodeStatus_t nShiplDisplay(dspConfig_t *);
static nodeStatus_t nShiplIdle(void);
static nodeStatus_t nShiplInput(int ch);

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

    shipOffset = 0;
    shipIdx = 0;

    if (setnode)
        setNode(&nShiplNode);

    return(&nShiplNode);
}


static nodeStatus_t nShiplDisplay(dspConfig_t *dsp)
{
    int snum = Context.snum;
    static char cbuf[BUFFER_SIZE_256];
    int i, unum, status, lin, col;
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

    lin++; // first line to use
    shipIdx = 0;
    if ( shipOffset < cbLimits.maxShips() )
    {
        while ((shipOffset + shipIdx) < cbLimits.maxShips())
        {
            i = shipOffset + shipIdx;
            shipIdx++;

            status = cbShips[i].status;

            if ( status == SS_LIVE )
            {
                sbuf[0] = 0;
                utAppendShip(sbuf , i) ;
                strcat(sbuf, " ") ;
                utAppendChar(sbuf, cbShipTypes[cbShips[i].shiptype].name[0]) ;

                unum = cbShips[i].unum;
                if ( unum >= 0 && unum < cbLimits.maxUsers() )
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

                if (snum >= 0 && snum < cbLimits.maxShips() )
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
            }
            lin++;

            if (lin == MSG_LIN1)
                break; // out of while

        } // while

        if ((shipOffset + shipIdx) >= cbLimits.maxShips())
            cprintf(MSG_LIN2, 0,  ALIGN_CENTER, "#%d#%s", NoColor, MTXT_DONE);
        else
            cprintf(MSG_LIN2, 0,  ALIGN_CENTER, "#%d#%s", NoColor, MTXT_MORE);
    } // if

    return NODE_OK;
}

static nodeStatus_t nShiplIdle(void)
{
    if (Context.recmode == RECMODE_PLAYING || Context.recmode == RECMODE_PAUSED)
        return NODE_OK;             /* no packet reading here */

    if (clientFlags & SPCLNTSTAT_FLAG_KILLED && retnode == DSP_NODE_CP)
    {
        /* time to die properly. */
        setONode(NULL);
        nDeadInit();
        return NODE_OK;
    }


    return NODE_OK;
}

static nodeStatus_t nShiplInput(int ch)
{
    ch = CQ_CHAR(ch);

    if (ch == ' ')
    {
        shipOffset += shipIdx;

        if (shipOffset < cbLimits.maxShips())
            return NODE_OK;
    }
    else if (ch == TERM_EXTRA) // go back to first page
    {
        shipOffset = 0;
        return NODE_OK;
    }

    /* go back */
    switch (retnode)
    {
    case DSP_NODE_CP:
        setONode(NULL);
        nCPInit(false);
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
