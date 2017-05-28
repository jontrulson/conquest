/*
 * Planet listing
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "conqcom.h"
#include "conqlb.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"
#include "conqutil.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nPlanetl.h"
#include "cqkeys.h"

static int team, snum;

static int PlanetOffset;    /* offset into NUMPLANETS for this page */
static int PlanetIdx;
static char *hd0="P L A N E T   L I S T   ";
static char *hd1="' = must take to conquer the Universe)";
static char *hd2="planet      type team armies          planet      type team armies";
static char hd3[BUFFER_SIZE];

static int nPlanetlDisplay(dspConfig_t *);
static int nPlanetlIdle(void);
static int nPlanetlInput(int ch);

static scrNode_t nPlanetlNode = {
    nPlanetlDisplay,               /* display */
    nPlanetlIdle,                  /* idle */
    nPlanetlInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

static int retnode;             /* the node to return to */

scrNode_t *nPlanetlInit(int nodeid, int setnode, int sn, int tn)
{
    retnode = nodeid;

    PlanetOffset = 1;
    PlanetIdx = 0;
    snum = sn;
    team = tn;

    if (setnode)
        setNode(&nPlanetlNode);

    return(&nPlanetlNode);
}


static int nPlanetlDisplay(dspConfig_t *dsp)
{
    int i, lin, col, olin, pnum;
    static int sv[NUMPLANETS + 1];
    char ch, junk[10], coreflag;
    int outattr;
    int col2;
    int column_h = 7;
    int column_1 = 5;
    int column_2 = 43;
    char xbuf[BUFFER_SIZE];
    static char pd0[MID_BUFFER_SIZE];
    static int FirstTime = TRUE;

    if (FirstTime == TRUE)
    {
        FirstTime = FALSE;
        /* build header fmt string */
        sprintf(pd0,
                "#%d#%s#%d#%s#%d#%s#%d#%s" ,
                LabelColor,
                hd0,
                InfoColor,
                "('",
                SpecialColor,
                "+",
                InfoColor,
                hd1);

        /* sort the planets */
        for ( i = 1; i <= NUMPLANETS; i++ )
            sv[i] = i;
        clbSortPlanets( sv );

        strcpy( hd3, hd2 );

        for ( i = 0; hd3[i] != 0; i++ )
            if ( hd3[i] != ' ' )
                hd3[i] = '-';
    }

    lin = 1;
    col = column_h;

    cprintf(lin, column_h, ALIGN_NONE, pd0);

    /* display column headings */
    lin += 2;
    cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", LabelColor, hd2);
    lin++;
    cprintf(lin, 0,  ALIGN_CENTER, "#%d#%s", LabelColor, hd3);
    lin++;
    olin = lin;
    col = column_1;
    col2 = FALSE;

    PlanetIdx = 0;
    if (PlanetOffset <= NUMPLANETS)
    {
        while ((PlanetOffset + PlanetIdx) <= NUMPLANETS)
        {
            i = PlanetOffset + PlanetIdx;
            PlanetIdx++;
            pnum = sv[i];

            /* colorize - dwp */
            if ( snum > 0 && snum <= MAXSHIPS)
            {	/* if user has a valid ship */
                if ( Planets[pnum].team == Ships[snum].team && !selfwar(snum) )
                    outattr = GreenLevelColor;
                else if ( (clbSPWar(snum,pnum) && Planets[pnum].scanned[Ships[snum].team] ) ||
                          Planets[pnum].type == PLANET_SUN )
                    outattr = RedLevelColor;
                else
                    outattr = YellowLevelColor;
            }
            else
            {			/* else, user doesn't have a ship yet */
                if (team == TEAM_NOTEAM)
                {			/* via conqoper */
                    switch(Planets[pnum].type)
                    {
                    case PLANET_SUN:
                        outattr = RedLevelColor;
                        break;
                    case PLANET_CLASSM:
                        outattr = GreenLevelColor;
                        break;
                    case PLANET_DEAD:
                        outattr = YellowLevelColor;
                        break;
                    case PLANET_CLASSA:
                    case PLANET_CLASSO:
                    case PLANET_CLASSZ:
                        outattr = CQC_A_BOLD;
                        break;
                    case PLANET_GHOST:
                        outattr = NoColor;
                        break;
                    default:
                        outattr = SpecialColor;
                        break;
                    }
                }
                else
                {			/* via menu() */
                    if ( Planets[pnum].team == Users[Context.unum].team &&
                         !(Users[Context.unum].war[Users[Context.unum].team]))
                    {
                        outattr = GreenLevelColor;
                    }
                    else if ( Planets[pnum].type == PLANET_SUN ||
                              (Planets[pnum].team < NUMPLAYERTEAMS &&
                               Users[Context.unum].war[Planets[pnum].team] &&
                               Planets[pnum].scanned[Users[Context.unum].team]) )
                    {
                        outattr = RedLevelColor;
                    }
                    else
                    {
                        outattr = YellowLevelColor;
                    }
                }
            }

            /* Don't display unless it's real. */
            if ( ! PVISIBLE(pnum) )
                continue;

            /* I want everything if it's real */

            /* Figure out who owns it and count armies. */
            ch =  Teams[Planets[pnum].team].teamchar;
            sprintf( junk, "%d", Planets[pnum].armies );

            /* Then modify based on scan information. */

            if ( team != TEAM_NOTEAM )
                if ( ! Planets[pnum].scanned[team] )
                {
                    ch = '?';
                    c_strcpy( "?", junk );
                }

            /* Suns and moons are displayed as unowned. */
            if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON )
                ch = ' ';

            /* Don't display armies for suns unless we're special. */
            if ( Planets[pnum].type == PLANET_SUN )
                if ( team != TEAM_NOTEAM )
                    junk[0] = 0;

            /* Moons aren't supposed to have armies. */
            if ( Planets[pnum].type == PLANET_MOON )
            {
                if ( team != TEAM_NOTEAM )
                    junk[0] = 0;
                else if ( Planets[pnum].armies == 0 )
                    junk[0] = 0;
            }

            coreflag = ' ';

            /* flag planets that are required for a conq */
            if (Planets[pnum].type == PLANET_CLASSM || Planets[pnum].type == PLANET_DEAD)
            {
                if (pnum > NUMCONPLANETS)
                    coreflag = ' ';
                else
                    coreflag = '+';
            }

            cprintf(lin, col,  ALIGN_NONE, "#%d#%c", SpecialColor,
                    coreflag);
            col += 2;

            sprintf(xbuf,"%-11s ",Planets[pnum].name);  /* Planets[pnum].name */
            cprintf(lin, col,  ALIGN_NONE, "#%d#%s", outattr, xbuf);

            col+=(strlen(xbuf));
            sprintf( xbuf, "%-4c %-3c  ",
                     ConqInfo->chrplanets[Planets[pnum].type], ch);
            cprintf(lin, col,  ALIGN_NONE, "#%d#%s", outattr, xbuf);

            col+=(strlen(xbuf));
            sprintf(xbuf,"%4s",junk);
            cprintf(lin, col,  ALIGN_NONE, "#%d#%s",
                    (junk[0] == '?') ? YellowLevelColor : outattr,
                    xbuf);

            lin++;;
            if ( lin == MSG_LIN1 )
            {
                if (col2)	/* need a new page... */
                {
                    break; /* out of while */
                }
                else
                {
                    lin = olin;
                    col2 = TRUE;
                }
            }

            if (!col2)
                col = column_1;
            else
                col = column_2;

        } /* while */

        if ((PlanetOffset + PlanetIdx) > NUMPLANETS)
            cprintf(MSG_LIN2, 0,  ALIGN_CENTER, "#%d#%s", NoColor, MTXT_DONE);
        else
            cprintf(MSG_LIN2, 0,  ALIGN_CENTER, "#%d#%s", NoColor, MTXT_MORE);

    } /* if PlanetOffset <= NUMPLANETS */

    return NODE_OK;
}

static int nPlanetlIdle(void)
{
    int pkttype;
    char buf[PKT_MAXSIZE];

    while ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                       buf, PKT_MAXSIZE, 0, NULL)) > 0)
        processPacket(buf);

    if (pkttype < 0)          /* some error */
    {
        utLog("nPlanetlIdle: waiForPacket returned %d", pkttype);
        Ships[Context.snum].status = SS_OFF;
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

static int nPlanetlInput(int ch)
{
    ch = CQ_CHAR(ch);

    if (ch == ' ')
    {
        PlanetOffset += PlanetIdx;

        if (PlanetOffset <= NUMPLANETS)
            return NODE_OK;
    }

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

    default:
        utLog("nPlanetlInput: invalid return node: %d, going to DSP_NODE_MENU",
              retnode);
        setONode(NULL);
        nMenuInit();
        break;
    }

    /* NOTREACHED */
    return NODE_OK;
}
