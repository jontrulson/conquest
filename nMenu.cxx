/*
 * menu node
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#include "c_defs.h"
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

static const char *if1="Suddenly  a  sinister,  wraithlike  figure appears before you";
static const char *if2="seeming to float in the air.  In a low,  sorrowful  voice  he";
static const char *if3="says, \"Alas, the very nature of the universe has changed, and";
static const char *if4="your ship cannot be found.  All must now pass away.\"  Raising";
static const char *if5="his  oaken  staff  in  farewell,  he fades into the spreading";
static const char *if6="darkness.  In his place appears a  tastefully  lettered  sign";
static const char *if7="reading:";
static const char *if8="INITIALIZATION FAILURE";
static const char *if9="The darkness becomes all encompassing, and your vision fails.";


#define S_NONE          0
#define S_WAR           1
#define S_PSEUDO        2
#define S_RESIGN        3
#define S_LOSE          4
static int state;

static prm_t prm;
static int prompting;

static char cbuf[BUFFER_SIZE_256];

/* init vars */
static int lose, oclosed;
static int fatal = FALSE;

/* war vars */
static int twar[NUMPLAYERTEAMS];

static int nMenuDisplay(dspConfig_t *);
static int nMenuIdle(void);
static int nMenuInput(int ch);

static scrNode_t nMenuNode = {
    nMenuDisplay,               /* display */
    nMenuIdle,                  /* idle */
    nMenuInput,                  /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

static void _conqds(dspConfig_t *dsp)
{
    int i, col, lin;
    extern char *ConquestVersion;
    extern char *ConquestDate;
    static int FirstTime = TRUE;
    static char sfmt[MSGMAXLINE * 2];

    if (FirstTime == TRUE)
    {
        FirstTime = FALSE;
        sprintf(sfmt,
                "#%d#(#%d#%%c#%d#) - %%s",
                LabelColor,
                InfoColor,
                LabelColor);
    }

    /* Display the logo. */
    mglConqLogo(dsp, TRUE);

    lin = 7;

    if ( cbConqInfo->closed )
        cprintf(lin,0,ALIGN_CENTER,"#%d#%s",RedLevelColor,"The game is closed.");
    else
        cprintf( lin,0,ALIGN_CENTER,"#%d#%s (%s)",YellowLevelColor,
                 ConquestVersion, ConquestDate);

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

    if ( sStat.flags & SPSSTAT_FLAGS_SWITCHTEAM )
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
    cprintf(lin,col,ALIGN_NONE,sfmt, 'q', "exit the program");

    if (state == S_WAR)
        cprintf(prm.index, 0, ALIGN_NONE, prm.pbuf);

    return;

}


void nMenuInit(void)
{
    char buf[PKT_MAXSIZE];
    static int inited = FALSE;

    state = S_NONE;
    prompting = FALSE;

    if (!inited)
    {
        inited = TRUE;
        /* Initialize statistics. */
        initstats( &cbShips[Context.snum].ctime, &cbShips[Context.snum].etime );

        /* Set up some things for the menu display. */
        oclosed = cbConqInfo->closed;
        Context.leave = FALSE;

        /* now look for our ship packet before we get started.  It should be a
           full SP_SHIP packet for this first time */
        if (pktWaitForPacket(SP_SHIP, buf, PKT_MAXSIZE,
                             60, NULL) <= 0)
        {
            utLog("nMenuInit: didn't get initial SP_SHIP");
            fatal = TRUE;
            return;
        }
        else
            procShip(buf);

        /* Some simple housekeeping. */
        if ( oclosed != cbConqInfo->closed )
            oclosed = ! oclosed;

        lose = FALSE;
    }

    setNode(&nMenuNode);

    return;
}


static int nMenuDisplay(dspConfig_t *dsp)
{
    int lin, col;
    char cbuf[BUFFER_SIZE_256];

    if (fatal)
        return NODE_EXIT;

    _conqds(dsp);
    clbUserline( -1, -1, cbuf, FALSE, TRUE );
    cprintf(MSG_LIN1, 1, ALIGN_LEFT, "#%d#%s",
            LabelColor,
            cbuf);
    clbUserline( Context.unum, -1, cbuf, FALSE, TRUE );
    cprintf(MSG_LIN2, 1, ALIGN_LEFT, "#%d#%s",
            NoColor,
            cbuf);

    if (state == S_PSEUDO)
    {
        cprintf(prm.index, 0, ALIGN_NONE, "#%d#Old pseudonym: %s",
                NoColor, cbUsers[Context.unum].alias);

        cprintf(prm.index + 1, 0, ALIGN_NONE, "#%d#Enter a new pseudonym: %s",
                NoColor, prm.buf);
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

        return NODE_OK;
    }



    return NODE_OK;
}


static int nMenuIdle(void)
{
    int pkttype;
    char buf[PKT_MAXSIZE];

    while ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                       buf, PKT_MAXSIZE, 0, NULL)) > 0)
    {                       /* proc packets while we get them */
        switch (pkttype)
        {
        case SP_ACK:
            PKT_PROCSP(buf);
            if (sAckMsg.code == PERR_LOSE)
            {
                lose = TRUE;
                state = S_LOSE;
                return NODE_OK;   /* but not for long... */
            }
            else
                utLog("nMenuIdle: got unexp ack code %d", sAckMsg.code);

            break;

        default:
            processPacket(buf);
            break;
        }
    }

    if (pkttype < 0)          /* some error */
    {
        utLog("nMenuIdle: waitForPacket returned %d", pkttype);
        cbShips[Context.snum].status = SS_OFF;
        return NODE_EXIT;
    }

    return NODE_OK;
}

static int nMenuInput(int ch)
{
    int i, irv;
    uint16_t cwar;

    ch = CQ_CHAR(ch);

    if ((state == S_LOSE) && ch != 0)
        return NODE_EXIT;           /* you lose */

    if (prompting)
    {
        irv = prmProcInput(&prm, ch);

        switch (state)
        {
        case S_PSEUDO:
            if (irv > 0)
            {
                if (ch != TERM_ABORT && prm.buf[0] != 0)
                    sendSetName(prm.buf);
                prompting = FALSE;
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
            prompting = FALSE;

            break;

        case S_WAR:
            if (irv > 0)
            {
                if (ch == TERM_ABORT || ch == TERM_NORMAL)
                {
                    state = S_NONE;
                    prompting = FALSE;
                    return NODE_OK;
                }

                if (ch == TERM_EXTRA) /* accepted */
                {
                    state = S_NONE;
                    prompting = FALSE;
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
                prm.buf[0] = 0;
                for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                    if ( ch == (char)tolower( cbTeams[i].teamchar ) )
                    {
                        if ( ! twar[i] || ! cbShips[Context.snum].rwar[i] )
                            twar[i] = ! twar[i];
                        prm.pbuf = clbWarPrompt(Context.snum, twar);
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
        nHistlInit(DSP_NODE_MENU, TRUE);
        break;

    case 'N':
        state = S_PSEUDO;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = MAXUSERNAME;
        prm.terms = TERMS;
        prm.index = 20;
        prm.buf[0] = 0;
        prompting = TRUE;

        break;

    case 'r':
        for ( i = 0; i < MAXSHIPS; i++ )
            if ( cbShips[i].status == SS_LIVE ||
                 cbShips[i].status == SS_ENTERING )
                if ( cbShips[i].unum == Context.unum )
                    break;

        if ( i < MAXSHIPS )
            mglBeep(MGL_BEEP_ERR);
        else
        {
            state = S_RESIGN;
            prm.preinit = FALSE;
            prm.buf = cbuf;
            prm.buflen = MAXUSERNAME;
            prm.terms = TERMS;
            prm.index = 20;
            prm.buf[0] = 0;
            prompting = TRUE;
        }
        break;
    case 's':
        if ( ! (sStat.flags & SPSSTAT_FLAGS_SWITCHTEAM) )
            mglBeep(MGL_BEEP_ERR);
        else
        {
            /* we'll update local data here anyway, even though it will be
               overwritten on the next ship update.  Improves perceived
               response time. */
            cbShips[Context.snum].team =
                utModPlusOne( cbShips[Context.snum].team+1, NUMPLAYERTEAMS );
            cbShips[Context.snum].shiptype =
                cbTeams[cbShips[Context.snum].team].shiptype;
            cbUsers[Context.unum].team = cbShips[Context.snum].team;
            cbShips[Context.snum].war[cbShips[Context.snum].team] = FALSE;
            cbUsers[Context.unum].war[cbUsers[Context.unum].team] = FALSE;

            sendCommand(CPCMD_SWITCHTEAM, (uint16_t)cbShips[Context.snum].team);
        }
        break;

    case 'O':
        nOptionsInit(NOPT_USER, TRUE, DSP_NODE_MENU);
        break;

    case 'S':
        nUserlInit(DSP_NODE_MENU, TRUE, Context.snum, FALSE, TRUE);
        break;

    case 'T':
        nTeamlInit(DSP_NODE_MENU, TRUE, cbShips[Context.snum].team);
        break;

    case 'U':
        nUserlInit(DSP_NODE_MENU, TRUE, Context.snum, FALSE, FALSE);
        break;

    case 'W':
        for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
            twar[i] = cbShips[Context.snum].war[i];

        state = S_WAR;
        prompting = TRUE;
        prm.preinit = FALSE;
        prm.buf = cbuf;
        prm.buflen = 5;
        prm.pbuf = clbWarPrompt(Context.snum, twar);
        prm.terms = TERMS;
        prm.index = 20;
        prm.buf[0] = 0;

        break;
    case '/':
        nShiplInit(DSP_NODE_MENU, TRUE);
        break;

    case '?':
        if (Context.snum >= 0 && Context.snum < MAXSHIPS)
            nPlanetlInit(DSP_NODE_MENU, TRUE, Context.snum, cbShips[Context.snum].team);
        else          /* then use user team if user doen't have a ship yet */
            nPlanetlInit(DSP_NODE_MENU, TRUE, Context.snum, cbUsers[Context.unum].team);
        break;

    case 'q':
        /* first stop all music and effects */
        cqsEffectStop(CQS_INVHANDLE, TRUE);
        cqsMusicStop(TRUE);

        return NODE_EXIT;

        break;                    /* NOTREACHED */

    }

    return NODE_OK;
}
