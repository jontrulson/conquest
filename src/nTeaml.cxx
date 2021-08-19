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
#include "cb.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "packet.h"
#include "conqutil.h"
#include "cprintf.h"

#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nTeaml.h"

static int team;

static nodeStatus_t nTeamlDisplay(dspConfig_t *);
static nodeStatus_t nTeamlIdle(void);
static nodeStatus_t nTeamlInput(int ch);

static scrNode_t nTeamlNode = {
    nTeamlDisplay,               /* display */
    nTeamlIdle,                  /* idle */
    nTeamlInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animVec */
};

static int retnode;             /* the node to return to */

scrNode_t *nTeamlInit(int nodeid, int setnode, int tn)
{
    team = tn;
    retnode = nodeid;

    if (setnode)
        setNode(&nTeamlNode);

    return(&nTeamlNode);
}


static nodeStatus_t nTeamlDisplay(dspConfig_t *dsp)
{
    int i, j, lin, col, ctime, etime;
    int godlike;
    char buf[MAX_MSGLINE_LENGTH], timbuf[5][MAX_DATESIZE];
    real x[5];
    static const char *sfmt="%15s %11s %11s %11s %11s %11s";
    static const char *stats="Statistics since: ";
    static const char *last_conquered="Universe last conquered at: ";

    char tmpfmt[MAX_MSGLINE_LENGTH * 2];
    static char sfmt2[MAX_MSGLINE_LENGTH * 2];
    static char sfmt3[MAX_MSGLINE_LENGTH * 2];
    static char dfmt2[MAX_MSGLINE_LENGTH * 2];
    static char pfmt2[MAX_MSGLINE_LENGTH * 2];
    static bool FirstTime = true;	/* Only necc if the colors aren't
                                           going to change at runtime */

    if (FirstTime)
    {
        FirstTime = false;
        sprintf(sfmt2,
                "#%d#%%16s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s",
                LabelColor,
                GreenLevelColor,
                YellowLevelColor,
                RedLevelColor,
                SpecialColor,
                InfoColor);

        sprintf(sfmt3,
                "#%d#%%15s #%d#%%12s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s",
                LabelColor,
                GreenLevelColor,
                YellowLevelColor,
                RedLevelColor,
                SpecialColor,
                InfoColor);

        sprintf(dfmt2,
                "#%d#%%15s #%d#%%12d #%d#%%11d #%d#%%11d #%d#%%11d #%d#%%11d",
                LabelColor,
                GreenLevelColor,
                YellowLevelColor,
                RedLevelColor,
                SpecialColor,
                InfoColor);

        sprintf(pfmt2,
                "#%d#%%15s #%d#%%11.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%%",
                LabelColor,
                GreenLevelColor,
                YellowLevelColor,
                RedLevelColor,
                SpecialColor,
                InfoColor);

    } /* FIRST_TIME */

    godlike = ( team < 0 || team >= NUM_PLAYERTEAMS );
    col = 0; /*1*/

    lin = 1;
    /* team stats and last date conquered */
    sprintf(tmpfmt, "#%d#%%s#%d#%%s", LabelColor, InfoColor);
    cprintf(lin, col, ALIGN_CENTER, tmpfmt, stats, cbConqInfo->inittime);
    lin++;

    /* last conquered */
    cprintf(lin, col, ALIGN_CENTER, tmpfmt, last_conquered,
            cbConqInfo->conqtime);
    lin++;

    /* last conqueror and conqteam */
    sprintf(tmpfmt, "#%d#by #%d#%%s #%d#for the #%d#%%s #%d#team",
            LabelColor, (int)CQC_A_BOLD, LabelColor, (int)CQC_A_BOLD, LabelColor);
    cprintf(lin,col,ALIGN_CENTER, tmpfmt, cbConqInfo->conqueror,
            cbConqInfo->conqteam);

    lin = lin + 1;

    if ( cbConqInfo->lastwords[0] != 0 )
    {
        sprintf(tmpfmt, "#%d#%%c%%s%%c", YellowLevelColor);
        cprintf(lin, col, ALIGN_CENTER, tmpfmt,
                '"', cbConqInfo->lastwords, '"' );
    }

    lin+=2;
    sprintf( buf, sfmt, " ",
             cbTeams[0].name, cbTeams[1].name, cbTeams[2].name, cbTeams[3].name, "Totals" );
    cprintf(lin, col, ALIGN_NONE, sfmt2, " ",
            cbTeams[0].name, cbTeams[1].name, cbTeams[2].name, cbTeams[3].name, "Totals" );

    lin++;
    for ( i = 0; buf[i] != 0; i++ )
        if ( buf[i] != ' ' )
            buf[i] = '-';

    cprintf(lin, col, ALIGN_NONE, "#%d#%s", LabelColor, buf);

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Conquers",
            cbTeams[0].stats[TeamStats::Conquers], cbTeams[1].stats[TeamStats::Conquers],
            cbTeams[2].stats[TeamStats::Conquers], cbTeams[3].stats[TeamStats::Conquers],
            cbTeams[0].stats[TeamStats::Conquers] + cbTeams[1].stats[TeamStats::Conquers] +
            cbTeams[2].stats[TeamStats::Conquers] + cbTeams[3].stats[TeamStats::Conquers] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Wins",
            cbTeams[0].stats[TeamStats::Wins], cbTeams[1].stats[TeamStats::Wins],
            cbTeams[2].stats[TeamStats::Wins], cbTeams[3].stats[TeamStats::Wins],
            cbTeams[0].stats[TeamStats::Wins] + cbTeams[1].stats[TeamStats::Wins] +
            cbTeams[2].stats[TeamStats::Wins] + cbTeams[3].stats[TeamStats::Wins] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Losses",
            cbTeams[0].stats[TeamStats::Losses], cbTeams[1].stats[TeamStats::Losses],
            cbTeams[2].stats[TeamStats::Losses], cbTeams[3].stats[TeamStats::Losses],
            cbTeams[0].stats[TeamStats::Losses] + cbTeams[1].stats[TeamStats::Losses] +
            cbTeams[2].stats[TeamStats::Losses] + cbTeams[3].stats[TeamStats::Losses] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Ships",
            cbTeams[0].stats[TeamStats::Entries], cbTeams[1].stats[TeamStats::Entries],
            cbTeams[2].stats[TeamStats::Entries], cbTeams[3].stats[TeamStats::Entries],
            cbTeams[0].stats[TeamStats::Entries] + cbTeams[1].stats[TeamStats::Entries] +
            cbTeams[2].stats[TeamStats::Entries] + cbTeams[3].stats[TeamStats::Entries] );

    lin++;
    etime = cbTeams[0].stats[TeamStats::Seconds] + cbTeams[1].stats[TeamStats::Seconds] +
        cbTeams[2].stats[TeamStats::Seconds] + cbTeams[3].stats[TeamStats::Seconds];
    utFormatSeconds( cbTeams[0].stats[TeamStats::Seconds], timbuf[0] );
    utFormatSeconds( cbTeams[1].stats[TeamStats::Seconds], timbuf[1] );
    utFormatSeconds( cbTeams[2].stats[TeamStats::Seconds], timbuf[2] );
    utFormatSeconds( cbTeams[3].stats[TeamStats::Seconds], timbuf[3] );
    utFormatSeconds( etime, timbuf[4] );
    cprintf(lin, col, ALIGN_NONE, sfmt3, "Time",
            timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

    lin++;
    ctime = cbTeams[0].stats[TeamStats::CpuSeconds] + cbTeams[1].stats[TeamStats::CpuSeconds] +
        cbTeams[2].stats[TeamStats::CpuSeconds] + cbTeams[3].stats[TeamStats::CpuSeconds];
    utFormatSeconds( cbTeams[0].stats[TeamStats::CpuSeconds], timbuf[0] );
    utFormatSeconds( cbTeams[1].stats[TeamStats::CpuSeconds], timbuf[1] );
    utFormatSeconds( cbTeams[2].stats[TeamStats::CpuSeconds], timbuf[2] );
    utFormatSeconds( cbTeams[3].stats[TeamStats::CpuSeconds], timbuf[3] );
    utFormatSeconds( ctime, timbuf[4] );
    cprintf(lin, col, ALIGN_NONE, sfmt3, "Cpu time",
             timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

    lin++;
    for ( i = 0; i < 4; i++ )
    {
        j = cbTeams[i].stats[TeamStats::Seconds];
        if ( j <= 0 )
            x[i] = 0.0;
        else
            x[i] = 100.0 * ((real) cbTeams[i].stats[TeamStats::CpuSeconds] / (real) j);
    }
    if ( etime <= 0 )
        x[4] = 0.0;
    else
        x[4] = 100.0 * (real) ctime / (real)etime;
    cprintf(lin, col, ALIGN_NONE, pfmt2, "Cpu usage",
            x[0], x[1], x[2], x[3], x[4] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Phaser shots",
            cbTeams[0].stats[TeamStats::Phasers], cbTeams[1].stats[TeamStats::Phasers],
            cbTeams[2].stats[TeamStats::Phasers], cbTeams[3].stats[TeamStats::Phasers],
            cbTeams[0].stats[TeamStats::Phasers] + cbTeams[1].stats[TeamStats::Phasers] +
            cbTeams[2].stats[TeamStats::Phasers] + cbTeams[3].stats[TeamStats::Phasers] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Torps fired",
            cbTeams[0].stats[TeamStats::Torps], cbTeams[1].stats[TeamStats::Torps],
            cbTeams[2].stats[TeamStats::Torps], cbTeams[3].stats[TeamStats::Torps],
            cbTeams[0].stats[TeamStats::Torps] + cbTeams[1].stats[TeamStats::Torps] +
            cbTeams[2].stats[TeamStats::Torps] + cbTeams[3].stats[TeamStats::Torps] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Armies bombed",
            cbTeams[0].stats[TeamStats::ArmiesBombed], cbTeams[1].stats[TeamStats::ArmiesBombed],
            cbTeams[2].stats[TeamStats::ArmiesBombed], cbTeams[3].stats[TeamStats::ArmiesBombed],
            cbTeams[0].stats[TeamStats::ArmiesBombed] + cbTeams[1].stats[TeamStats::ArmiesBombed] +
            cbTeams[2].stats[TeamStats::ArmiesBombed] + cbTeams[3].stats[TeamStats::ArmiesBombed] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Armies captured",
            cbTeams[0].stats[TeamStats::ArmiesSpaced], cbTeams[1].stats[TeamStats::ArmiesSpaced],
            cbTeams[2].stats[TeamStats::ArmiesSpaced], cbTeams[3].stats[TeamStats::ArmiesSpaced],
            cbTeams[0].stats[TeamStats::ArmiesSpaced] + cbTeams[1].stats[TeamStats::ArmiesSpaced] +
            cbTeams[2].stats[TeamStats::ArmiesSpaced] + cbTeams[3].stats[TeamStats::ArmiesSpaced] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Planets taken",
            cbTeams[0].stats[TeamStats::PlanetsConquered], cbTeams[1].stats[TeamStats::PlanetsConquered],
            cbTeams[2].stats[TeamStats::PlanetsConquered], cbTeams[3].stats[TeamStats::PlanetsConquered],
            cbTeams[0].stats[TeamStats::PlanetsConquered] + cbTeams[1].stats[TeamStats::PlanetsConquered] +
            cbTeams[2].stats[TeamStats::PlanetsConquered] + cbTeams[3].stats[TeamStats::PlanetsConquered] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Coups",
            cbTeams[0].stats[TeamStats::Coups], cbTeams[1].stats[TeamStats::Coups],
            cbTeams[2].stats[TeamStats::Coups], cbTeams[3].stats[TeamStats::Coups],
            cbTeams[0].stats[TeamStats::Coups] + cbTeams[1].stats[TeamStats::Coups] +
            cbTeams[2].stats[TeamStats::Coups] + cbTeams[3].stats[TeamStats::Coups] );

    lin++;
    cprintf(lin, col, ALIGN_NONE, dfmt2, "Genocides",
            cbTeams[0].stats[TeamStats::Genocide], cbTeams[1].stats[TeamStats::Genocide],
            cbTeams[2].stats[TeamStats::Genocide], cbTeams[3].stats[TeamStats::Genocide],
            cbTeams[0].stats[TeamStats::Genocide] + cbTeams[1].stats[TeamStats::Genocide] +
            cbTeams[2].stats[TeamStats::Genocide] + cbTeams[3].stats[TeamStats::Genocide] );

    for ( i = 0; i < 4; i++ )
        if ( cbTeams[i].couptime == 0 )
            timbuf[i][0] = 0;
        else
            sprintf( timbuf[i], "%d", cbTeams[i].couptime );

    if ( ! godlike )
    {
        for ( i = 0; i < 4; i++ )
            if ( team != i )
                strcpy(timbuf[i] , "-") ;
            else if ( ! TEAM_COUPINFO(i) && timbuf[i][0] != 0 )
                strcpy(timbuf[i] , "?") ;
    }

    timbuf[4][0] = 0;

    lin++;
    cprintf(lin, col, ALIGN_NONE, sfmt3, "Coup time",
            timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

    cprintf( MSG_LIN2, 0, ALIGN_CENTER, "#%d#%s", NoColor, MTXT_DONE);

    return NODE_OK;
}

static nodeStatus_t nTeamlIdle(void)
{
    if (clientStatLastFlags & SPCLNTSTAT_FLAG_KILLED && retnode == DSP_NODE_CP)
    {
        /* time to die properly. */
        setONode(NULL);
        nDeadInit();
        return NODE_OK;
    }

    return NODE_OK;
}

static nodeStatus_t nTeamlInput(int ch)
{
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

    default:
        utLog("nTeamlInput: invalid return node: %d, going to DSP_NODE_MENU",
              retnode);
        setONode(NULL);
        nMenuInit();
        break;
    }

    /* NOTREACHED */
    return NODE_OK;
}
