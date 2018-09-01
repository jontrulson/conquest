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

#include "context.h"
#include "global.h"
#include "cb.h"
#include "conqunix.h"

#include "color.h"
#include "conf.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "conqlb.h"
#include "prm.h"
#include "glmisc.h"

#include "nMenu.h"
#include "nShipl.h"
#include "nPlanetl.h"
#include "nUserl.h"
#include "nHistl.h"
#include "nPlay.h"
#include "nTeaml.h"
#include "nOptions.h"
#include "cqkeys.h"
#include "cqsound.h"
#include "conqutil.h"
#include "cprintf.h"
#include "ping.h"

#include <vector>
using namespace std;

static const char *if1="Suddenly  a  sinister,  wraithlike  figure appears before you";
static const char *if2="seeming to float in the air.  In a low,  sorrowful  voice  he";
static const char *if3="says, \"Alas, the very nature of the universe has changed, and";
static const char *if4="your ship cannot be found.  All must now pass away.\"  Raising";
static const char *if5="his  oaken  staff  in  farewell,  he fades into the spreading";
static const char *if6="darkness.  In his place appears a  tastefully  lettered  sign";
static const char *if7="reading:";
static const char *if8="INITIALIZATION FAILURE";
static const char *if9="The darkness becomes all encompassing, and your vision fails.";

// idle timeout in menu()
static const char *itimeout1 =
    "Menu idle timeout after 10 minutes of inactivity";
static const char *itimeout2 =
    "Server closed connection";

#define S_NONE          0
#define S_WAR           1
#define S_PSEUDO        2
#define S_RESIGN        3
#define S_LOSE          4
#define S_TIMEOUT       5
static int state;

static prm_t prm;
static bool prompting;

/* init vars */
static bool lose = false;
static bool timedOut = false;
static bool fatal = false;

/* war vars */
static int twar[NUMPLAYERTEAMS];

static nodeStatus_t nMenuDisplay(dspConfig_t *);
static nodeStatus_t nMenuIdle(void);
static nodeStatus_t nMenuInput(int ch);

static scrNode_t nMenuNode = {
    nMenuDisplay,               /* display */
    nMenuIdle,                  /* idle */
    nMenuInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animVec */
};

static void _conqds(dspConfig_t *dsp)
{
    int i, col, lin;
    extern char *ConquestVersion;
    extern char *ConquestDate;
    static bool FirstTime = true;
    static char sfmt[MSGMAXLINE * 2];

    if (FirstTime)
    {
        FirstTime = false;
        sprintf(sfmt,
                "#%d#(#%d#%%c#%d#)#%d# - %%s",
                LabelColor,
                InfoColor,
                LabelColor,
                NoColor);
    }

    /* Display the logo. */
    mglConqLogo(dsp, true);

    lin = 7;

    cprintf( lin,0,ALIGN_CENTER,"#%d#%s (%s)",YellowLevelColor,
             ConquestVersion, ConquestDate);

    if ( sStat.serverFlags & SERVER_F_CLOSED )
        cprintf(++lin,0,ALIGN_CENTER,"#%d#%s",
                RedLevelColor,"The game is closed.");
    else
        lin++;

    lin++;
    cprintf(lin,0,ALIGN_CENTER,"#%d#%s", MagentaColor, CONQ_HTTP);
    lin++;
    lin++;
    lin++;
    cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Options:");

    col = 8;
    lin+=2;
    i = lin;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'e', "enter the game");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'S', "more user statistics");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'T', "team statistics");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'U', "user statistics");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'N', "change your name");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'W', "set war or peace");

    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'O', "options menu");

    col = 48;
    lin = i;

    cprintf(lin,col,ALIGN_NONE,sfmt, 'r', "resign your commission");
    lin++;

    if ( sStat.serverFlags & SERVER_F_SWITCHTEAM )
    {

        cprintf(lin,col,ALIGN_NONE,sfmt, 's', "switch teams");
        lin++;
    }

    cprintf(lin,col,ALIGN_NONE,sfmt, 'H', "user history");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, '/', "player list");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, '?', "planet list");
    lin++;
    cprintf(lin,col,ALIGN_NONE,sfmt, 'q', "quit");

    if (state == S_WAR)
        cprintf(prm.index, 0, ALIGN_NONE, "%s", prm.pbuf.c_str());

    return;

}


void nMenuInit(void)
{
    static int inited = false;

    // we come back here from renderNode() if the network has
    // disappeared.
    if (pktNoNetwork())
    {
        // if we came here from nCP or it's decedents, we did not time
        // out
        if (cInfo.state == CLIENT_STATE_PLAY)
            timedOut = false;
        else
            timedOut = true;

        state = S_TIMEOUT;
        mglBeep(MGL_BEEP_ERR);
    }
    else
        state = S_NONE;

    prompting = false;
    cInfo.state = CLIENT_STATE_MENU;

    if (!inited)
    {
        inited = true;
        /* Initialize statistics. */
        initstats( &cbShips[Context.snum].ctime, &cbShips[Context.snum].etime );


        /* now look for our ship packet before we get started.  It should be a
           full SP_SHIP packet for this first time */
        static const unsigned int waitTime = 30000; // 30 secs
        unsigned int startTime = cInfo.nodeMillis;
        bool gotShip = false;
        while ((cInfo.nodeMillis - startTime) < waitTime)
        {
            int rv;
            char buf[PKT_MAXSIZE];
            if ((rv = pktRead(buf, PKT_MAXSIZE, 1)) < 0)
            {
                utLog("nMenuInit: pktRead failed: %d", rv);
                fatal = true;
                return;
            }

            if (rv == 0)
                continue;

            // always process the packets
            PKT_PROCSP(buf);
            if (rv == SP_SHIP)
            {
                // if it was an SP_SHIP, we are good to go
                gotShip = true;
                break;
            }
        }

        if (!gotShip)
        {
            utLog("nMenuInit: Never received initial SP_SHIP packet");
            fatal = true;
            return;
        }

        lose = false;
    }

    clientLastServerAckCode = 0;
    setNode(&nMenuNode);

    if (cInfo.tryUDP && !cInfo.doUDP)
    {
        utLog("%s: requesting a UDP connection from server", __FUNCTION__);
        sendCommand(CPCMD_UDP, 1);
    }

    // enable pinging now
    pingEnable(true);

    return;
}


static nodeStatus_t nMenuDisplay(dspConfig_t *dsp)
{
    int lin, col;
    std::string cbuf;

    if (fatal)
        return NODE_EXIT;

    if (state == S_NONE || state == S_WAR)
    {
        _conqds(dsp);
        clbUserline( -1, -1, cbuf, false, true );
        cprintf(MSG_LIN1, 1, ALIGN_LEFT, "#%d#%s",
                LabelColor,
                cbuf.c_str());
        clbUserline( Context.unum, -1, cbuf, false, true );
        cprintf(MSG_LIN2, 1, ALIGN_LEFT, "#%d#%s",
                NoColor,
                cbuf.c_str());
    }

    if (state == S_PSEUDO)
    {
        cprintf(prm.index, 0, ALIGN_NONE, "#%d#Old pseudonym: %s",
                NoColor, cbUsers[Context.unum].alias);

        cprintf(prm.index + 1, 0, ALIGN_NONE, "#%d#Enter a new pseudonym: %s",
                NoColor, prm.buf.c_str());
    }

    if (state == S_RESIGN)
    {
        cprintf(prm.index, 0, ALIGN_CENTER,
                "#%d#Press [TAB] to confirm your resignation: ",
                NoColor);
    }

    if (state == S_LOSE)
    {
        /* We reincarnated or else something bad happened. */
        lin = 7;
        col = 11;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if1);
        lin++;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if2);
        lin++;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if3);
        lin++;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if4);
        lin++;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if5);
        lin++;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if6);
        lin++;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if7);
        lin+=2;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor | CQC_A_BOLD, if8);
        lin+=2;
        cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedLevelColor, if9);

        cprintf(20, 0, ALIGN_CENTER, MTXT_DONE);

        // cancel any overlay nodes
        setONode(NULL);

        return NODE_OK;
    }

    if (state == S_TIMEOUT)
    {
        lin = 7;
        if (timedOut)
            cprintf( lin++, 0, ALIGN_CENTER, "#%d#%s", NoColor, itimeout1);
        cprintf( lin, 0, ALIGN_CENTER, "#%d#%s", RedLevelColor, itimeout2);

        cprintf(20, 0, ALIGN_CENTER, MTXT_DONE);

        // cancel any overlay nodes
        setONode(NULL);

        return NODE_OK;
    }

    return NODE_OK;
}

static nodeStatus_t nMenuIdle(void)
{
    if (clientLastServerAckCode == PERR_LOSE)
    {
        lose = true;
        state = S_LOSE;
        return NODE_OK;   /* but not for long... */
    }

    if (clientLastServerAckCode == PERR_IDLETIMEOUT)
    {
        state = S_TIMEOUT;
        timedOut = true;
        // turn off any overlays
        setONode(NULL);
        return NODE_OK;   /* but not for long... */
    }

    return NODE_OK;
}

static nodeStatus_t nMenuInput(int ch)
{
    int i, irv;
    uint16_t cwar;

    ch = CQ_CHAR(ch);

    if ((state == S_LOSE || state == S_TIMEOUT) && ch != 0)
        return NODE_EXIT;           /* you lose */

    if (prompting)
    {
        irv = prmProcInput(prm, ch);

        switch (state)
        {
        case S_PSEUDO:
            if (irv > 0)
            {
                if (ch != TERM_ABORT && !prm.buf.empty())
                    sendSetName(prm.buf.c_str());
                prompting = false;
                state = S_NONE;
            }

            break;

        case S_RESIGN:
            if (irv > 0 && ch == TERM_EXTRA)
            {                   /* buhbye kapitan */
                sendCommand(CPCMD_RESIGN, 0);
                return NODE_EXIT;
            }
            state = S_NONE;
            prompting = false;

            break;

        case S_WAR:
            if (irv > 0)
            {
                if (ch == TERM_ABORT || ch == TERM_NORMAL)
                {
                    state = S_NONE;
                    prompting = false;
                    return NODE_OK;
                }

                if (ch == TERM_EXTRA) /* accepted */
                {
                    state = S_NONE;
                    prompting = false;
                    cwar = 0;
                    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                    {
                        if (twar[i])
                            cwar |= (1 << i);

                        /* we'll let it happen locally as well... */
                        cbUsers[cbShips[Context.snum].unum].war[i] = twar[i];
                        cbShips[Context.snum].war[i] = twar[i];
                    }

                    sendCommand(CPCMD_SETWAR, (uint16_t)cwar);
                    return NODE_OK;
                }

            }
            else
            {
                prm.buf.clear();
                for (i=0; i < NUMPLAYERTEAMS; i++)
                {
                    if ( (sStat.serverFlags & SERVER_F_NOTEAMWAR)
                         && i == cbShips[Context.snum].team
                         && ch == (char)tolower(cbTeams[i].teamchar))
                    {
                        // not allowed, make sure war status is
                        // cleared, then beep
                        cbShips[Context.snum].war[cbShips[Context.snum].team] = false;
                        mglBeep(MGL_BEEP_ERR);
                    }
                    else
                    {
                        if ( ch == (char)tolower( cbTeams[i].teamchar ) )
                        {
                            if ( ! twar[i] || ! cbShips[Context.snum].rwar[i] )
                                twar[i] = ! twar[i];
                            prm.pbuf = clbWarPrompt(Context.snum, twar);
                        }
                    }
                }
            }

        default:
            break;
        }

        return NODE_OK;
    }


    switch (ch)
    {
        case 'e':                   /* enter the game */
            nPlayInit();
            return NODE_OK;
            break;

        case 'H':
            nHistlInit(DSP_NODE_MENU, true);
            break;

        case 'N':
            state = S_PSEUDO;
            prm.preinit = false;
            prm.buf.clear();
            prm.buflen = MAXUSERNAME;
            prm.index = DISPLAY_LINS;
            prompting = true;

            break;

        case 'r':
            for ( i = 0; i < cbLimits.maxShips(); i++ )
                if ( cbShips[i].status == SS_LIVE ||
                     cbShips[i].status == SS_ENTERING )
                    if ( cbShips[i].unum == Context.unum )
                        break;

            if ( i < cbLimits.maxShips() )
                mglBeep(MGL_BEEP_ERR);
            else
            {
                state = S_RESIGN;
                prm.preinit = false;
                prm.buf.clear();
                prm.buflen = MAXUSERNAME;
                prm.index = DISPLAY_LINS;
                prompting = true;
            }
            break;
        case 's':
            if ( ! (sStat.serverFlags & SERVER_F_SWITCHTEAM) )
                mglBeep(MGL_BEEP_ERR);
            else
            {
                vector<int> enabledTeams = clbGetEnabledTeams();
                /* we'll update local data here anyway, even though it will be
                   overwritten on the next ship update.  Improves perceived
                   response time. */
                if (!enabledTeams.size())
                {
                    // choose among all of them
                    cbShips[Context.snum].team =
                        mod( cbShips[Context.snum].team + 1, NUMPLAYERTEAMS );
                }
                else
                {
                    // find our current team in enabledTeams
                    int idx = -1;
                    for (int i=0; i<enabledTeams.size(); i++)
                    {
                        if (cbShips[Context.snum].team == enabledTeams[i])
                        {
                            idx = i;
                            break;
                        }
                    }

                    if (idx == -1)
                    {
                        // shouldn't happen - we can't select another one
                        mglBeep(MGL_BEEP_ERR);
                        break; // case
                    }

                    int team = (idx + 1) % enabledTeams.size();
                    cbShips[Context.snum].team = enabledTeams[team];
                }
                cbShips[Context.snum].shiptype =
                    cbTeams[cbShips[Context.snum].team].shiptype;
                cbUsers[Context.unum].team = cbShips[Context.snum].team;
                cbShips[Context.snum].war[cbShips[Context.snum].team] = false;
                cbUsers[Context.unum].war[cbUsers[Context.unum].team] = false;

                sendCommand(CPCMD_SWITCHTEAM,
                            (uint16_t)cbShips[Context.snum].team);
            }
            break;

        case 'O':
            nOptionsInit(NOPT_USER, true, DSP_NODE_MENU);
            break;

        case 'S':
            nUserlInit(DSP_NODE_MENU, true, Context.snum, false, true);
            break;

        case 'T':
            nTeamlInit(DSP_NODE_MENU, true, cbShips[Context.snum].team);
            break;

        case 'U':
            nUserlInit(DSP_NODE_MENU, true, Context.snum, false, false);
            break;

        case 'W':
            for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                twar[i] = cbShips[Context.snum].war[i];

            state = S_WAR;
            prompting = true;
            prm.preinit = false;
            prm.buf.clear();
            prm.buflen = 5;
            prm.pbuf = clbWarPrompt(Context.snum, twar);
            prm.index = 22;

            break;
        case '/':
            nShiplInit(DSP_NODE_MENU, true);
            break;

        case '?':
            if (Context.snum >= 0 && Context.snum < cbLimits.maxShips())
                nPlanetlInit(DSP_NODE_MENU, true, Context.snum, cbShips[Context.snum].team);
            else          /* then use user team if user doen't have a ship yet */
                nPlanetlInit(DSP_NODE_MENU, true, Context.snum, cbUsers[Context.unum].team);
            break;

        case 'q':
            /* first stop all music and effects */
            cqsEffectStop(CQS_INVHANDLE, true);
            cqsMusicStop(true);

            return NODE_EXIT;

            break;                    /* NOTREACHED */

        default:
            mglBeep(MGL_BEEP_ERR);
            return NODE_OK;
    }

    return NODE_OK;
}
