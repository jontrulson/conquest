/*
 * Welcome to this node
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "cb.h"
#include "client.h"
#include "nWelcome.h"
#include "nMenu.h"
#include "gldisplay.h"
#include "node.h"
#include "conqutil.h"

#define S_DONE         0        /* nothing to display */
#define S_GREETINGS    1        /* GREETINGS - new user */
#define S_ERROR        2        /* some problem */

static int state;

static bool fatal = false;
static bool serror = false;
static bool newuser = false;
static time_t snooze = (time_t)0;          /* sleep time */

static const char *sorry1="I'm sorry, but the game is closed for repairs right now.";
static const char *sorry2="I'm sorry, but there is no room for a new player right now.";
static const char *sorryn="Please try again some other time.  Thank you.";
static const char *selected_str="You have been selected to command a";
static const char *starship_str=" starship.";
static const char *prepare_str="Prepare to be beamed aboard...";



static int nWelcomeDisplay(dspConfig_t *);

static scrNode_t nWelcomeNode = {
    nWelcomeDisplay,              /* display */
    NULL,                         /* idle */
    NULL,                         /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};

/*  gretds - block letter "greetings..." */
/*  SYNOPSIS */
/*    gretds */
static void gretds()
{
    int col,lin;
    static const char *g1=" GGG   RRRR   EEEEE  EEEEE  TTTTT   III   N   N   GGG    SSSS";
    static const char *g2="G   G  R   R  E      E        T      I    NN  N  G   G  S";
    static const char *g3="G      RRRR   EEE    EEE      T      I    N N N  G       SSS";
    static const char *g4="G  GG  R  R   E      E        T      I    N  NN  G  GG      S  ..  ..  ..";
    static const char *g5=" GGG   R   R  EEEEE  EEEEE    T     III   N   N   GGG   SSSS   ..  ..  ..";

    col = (int)(Context.maxcol - strlen(g5)) / (int)2;
    lin = 1;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g1);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g2);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g3);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g4);
    lin++;
    cprintf( lin,col,ALIGN_NONE,"#%d#%s", InfoColor, g5);

    return;

}

void nWelcomeInit(void)
{
    int pkttype;
    char buf[PKT_MAXSIZE];
    bool done = false;

    state = S_ERROR;
    setNode(&nWelcomeNode);

    /* now look for SP_CLIENTSTAT or SP_ACK */
    while (!done)
    {
        if ((pkttype =
             pktRead(buf, PKT_MAXSIZE, 60)) <= 0)
        {
            utLog("nWelcomeInit: read SP_CLIENTSTAT or SP_ACK failed: %d",
                  pkttype);
            fatal = true;
            return;
        }

        switch (pkttype)
        {
        case SP_CLIENTSTAT:
            if (PKT_PROCSP(buf))
            {
                done = true;
            }
            else
            {
                utLog("nWelcomeInit: invalid CLIENTSTAT");
                fatal = true;
                return;
            }
            break;
        case SP_ACK:
            PKT_PROCSP(buf);
            state = S_ERROR;
            serror = true;
            done = true;

            break;
        default:
            utLog("nWelcomeInit: got unexpected packet type %d. Ignoring.",
                  pkttype);
            done = false;

            break;
        }
    }

    if (pkttype == SP_CLIENTSTAT && (sClientStat.flags & SPCLNTSTAT_FLAG_NEW))
    {
        // new user
        state = S_GREETINGS;
        snooze = (time(0) + 3);
    }
    else
    {
        // not a new user
        if (!serror)
            state = S_DONE;           /* need to wait for user packet */
        else
            snooze = (time(0) + 4);
    }

    return;
}


static int nWelcomeDisplay(dspConfig_t *dsp)
{
    char buf[PKT_MAXSIZE];
    int team, col = 0;
    time_t t = time(0);

    if (fatal)
        return NODE_EXIT;           /* see ya! */

    if (snooze)
    {
        if (serror)                 /* an error */
        {
            if (t > snooze)     /* time to go */
            {
                snooze = 0;
                return NODE_EXIT;
            }
        }
        else
        {                       /* new user */
            if (t > snooze)
            {
                state = S_DONE;
                snooze = 0;
                return NODE_OK;
            }
        }
    }


    switch (state)
    {
    case S_GREETINGS:
        /* Must be a new player. */
        if ( cbConqInfo->closed )
        {
            /* Can't enroll if the game is closed. */
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
        }
        else
        {
            team = cbShips[Context.snum].team;
            gretds();                 /* 'GREETINGS' */

            if ( vowel( cbTeams[team].name[0] ) )
                cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s%c #%d#%s #%d#%s",
                        InfoColor,selected_str,'n',CQC_A_BOLD,cbTeams[team].name,
                        InfoColor,starship_str);
            else
                cprintf(MSG_LIN2/2,0,ALIGN_CENTER,"#%d#%s #%d#%s #%d#%s",
                        InfoColor,selected_str,CQC_A_BOLD,cbTeams[team].name,
                        InfoColor,starship_str);

            cprintf(MSG_LIN2/2+1,0,ALIGN_CENTER,"#%d#%s",
                    InfoColor, prepare_str );
        }

        return NODE_OK;
        break;

    case S_ERROR:
        switch (sAckMsg.code)
        {
        case PERR_CLOSED:
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry1 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
            break;

        case PERR_REGISTER:
            cprintf(MSG_LIN2/2,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorry2 );
            cprintf(MSG_LIN2/2+1,col,ALIGN_CENTER,"#%d#%s", InfoColor, sorryn );
            break;

        case PERR_NOSHIP:
            cprintf(MSG_LIN2/2, 0, ALIGN_CENTER,
                    "I'm sorry, but there are no ships available right now.");
            cprintf((MSG_LIN2/2)+1, 0, ALIGN_CENTER,
                    sorryn);
            break;

        default:
            utLog("nWelcomeDisplay: unexpected ACK code %d\n", sAckMsg.code);
            break;
        }

        return NODE_OK;
        break;

    case S_DONE:
        if (pktWaitForPacket(SP_USER, buf, PKT_MAXSIZE,
                             60, NULL) <= 0)
        {
            utLog("nWelcomeDisplay: waitforpacket SP_USER returned error");
            return NODE_EXIT;
        }
        else
            procUser(buf);

        nMenuInit();
        return NODE_OK;
        break;

    default:
        utLog("nWelcomeDisplay: unknown state %d", state);
        return NODE_EXIT;
        break;
    }

    return NODE_OK;
}
