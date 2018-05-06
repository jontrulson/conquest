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

#include <vector>
#include <string>

#include "context.h"
#include "global.h"

#include "color.h"
#include "conf.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "conqlb.h"
#include "nMeta.h"
#include "nWelcome.h"
#include "userauth.h"
#include "gldisplay.h"
#include "cqkeys.h"
#include "prm.h"
#include "glmisc.h"
#include "conqutil.h"

#include "GL.h"
#include "blinker.h"

static const char *unamep = "Username:";
static const char *pwp = "Password:";
static const char *rpwp = "Retype Password:";
static const char *newuserp = "User doesn't exist. Is this a new user?";
static const char *pwerr = "Invalid Password.";
static const char *uhelper = "You can use A-Z, a-z, and 0-9";
static const char *phelper = "Use any printable characters.";

static char cursor = ' ';       /* the cursor */

/* the current prompt */
static prm_t prm;

// storage for name and pw
static std::string nm, pw;

static const char *statlin = NULL;
static const char *errlin = NULL;
static int  newuser = false;    /* a new user? */

/* the current state */
#define S_DONE         0
#define S_GETUNAME     1
#define S_GETPW        2
#define S_GETRPW       3
#define S_CONFIRMNEW   4

static int state;

static nodeStatus_t nAuthDisplay(dspConfig_t *dsp);
static nodeStatus_t nAuthInput(int ch);
static nodeStatus_t nAuthIdle(void);

static scrNode_t nAuthNode = {
    nAuthDisplay,                 /* display */
    nAuthIdle,                    /* idle */
    nAuthInput,                    /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */
};


/* basically a copy from nMeta, with extra gleen */
static void dispServerInfo(int tlin)
{
    static char buf1[BUFFER_SIZE_256];
    static char buf2[BUFFER_SIZE_256];
    static char buf3[BUFFER_SIZE_256];
    static char buf5[BUFFER_SIZE_256];
    static char buf6[BUFFER_SIZE_256];
    static char pbuf1[BUFFER_SIZE_256];
    static char pbuf2[BUFFER_SIZE_256];
    static char pbuf3[BUFFER_SIZE_256];
    static char pbuf4[BUFFER_SIZE_256];
    static char pbuf5[BUFFER_SIZE_256];
    static char pbuf6[BUFFER_SIZE_256];
    static int inited = false;
    static const int hcol = 1, icol = 11;
    static char timebuf[BUFFER_SIZE_256];
    time_t servtm;

    if (!inited)
    {
        inited = true;
        sprintf(pbuf1, "#%d#Server: ", MagentaColor);
        sprintf(buf1, "#%d#%%s", NoColor);

        sprintf(pbuf2, "#%d#Version: ", MagentaColor);
        sprintf(buf2, "#%d#%%s", NoColor);

        sprintf(pbuf6, "#%d#Time: ", MagentaColor);
        sprintf(buf6, "#%d#%%s", NoColor);

        sprintf(pbuf3, "#%d#Status: ", MagentaColor);
        sprintf(buf3,
                "#%d#Users #%d#%%d#%d#,"
                "#%d#Ships #%d#%%d/%%d #%d#"
                "(#%d#%%d #%d#active, #%d#%%d #%d#vacant, "
                "#%d#%%d #%d#robot)",
                NoColor, CyanColor, NoColor,
                NoColor, CyanColor, NoColor,
                CyanColor, NoColor, CyanColor, NoColor,
                CyanColor, NoColor);

        sprintf(pbuf4, "#%d#Flags: ", MagentaColor);

        sprintf(pbuf5, "#%d#MOTD: ", MagentaColor);
        sprintf(buf5, "#%d#%%s", NoColor);

        servtm = sStat.servertime; /* fix alignment */
        utStrncpy(timebuf, ctime(&servtm), BUFFER_SIZE_256);
        timebuf[strlen(timebuf) - 1] = 0; /* remove the NL */

    }

    cprintf(tlin, hcol, ALIGN_NONE, "%s", pbuf1);
    cprintf(tlin++, icol, ALIGN_NONE, buf1, sHello.servername);

    cprintf(tlin, hcol, ALIGN_NONE, "%s", pbuf2);
    cprintf(tlin++, icol, ALIGN_NONE, buf2, sHello.serverver);

    cprintf(tlin, hcol, ALIGN_NONE, "%s", pbuf6);
    cprintf(tlin++, icol, ALIGN_NONE, buf6, timebuf);

    cprintf(tlin, hcol, ALIGN_NONE, "%s", pbuf3);
    cprintf(tlin++, icol, ALIGN_NONE, buf3,
            sStat.numusers, sStat.numtotal, cbLimits.maxShips(),
            sStat.numactive,
            sStat.numvacant, sStat.numrobot);

    cprintf(tlin, hcol, ALIGN_NONE, "%s", pbuf4);

    tlin = clntPrintServerFlags(tlin, icol, sStat.serverFlags, NoColor);

    tlin++;
    cprintf(tlin, hcol, ALIGN_NONE, "%s", pbuf5);
    cprintf(tlin++, icol, ALIGN_NONE, buf5, sHello.motd);

    return;
}


void nAuthInit(void)
{
    pw.clear();
    nm.clear();

    state = S_GETUNAME;           /* initial state */
    prm.preinit = false;
    prm.buf.clear();
    prm.buflen = MAXUSERNAME;
    prm.pbuf = unamep;

    statlin = uhelper;

    setNode(&nAuthNode);

    return;
}

/* all we do here is 'blink' the cursor ;-) */
static nodeStatus_t nAuthIdle(void)
{
    cursor = (BLINK_QTRSEC) ? '_' : ' ';

    return NODE_OK;
}



static nodeStatus_t nAuthDisplay(dspConfig_t *dsp)
{
    int lin;
    int statline;
    int slin;
    int tmpcolor;
    extern char *ConquestVersion;
    extern char *ConquestDate;

    /* display the logo */
    mglConqLogo(dsp, false);

    lin = 7;

    cprintf( lin, 1, ALIGN_CENTER, "#%d#Welcome to #%d#Conquest#%d# %s (%s)",
             YellowLevelColor,
             RedLevelColor,
             YellowLevelColor,
             ConquestVersion, ConquestDate);

    lin += 2;

    statline = lin;

    lin += 9;
    slin = lin;

    dispServerInfo(statline);

    /* Username */
    if (state == S_GETUNAME && prm.preinit)
        tmpcolor = MagentaColor;
    else
        tmpcolor = NoColor;

    cprintf( slin - 1, 1, ALIGN_LEFT,
             "#%d#Please login. Press [ENTER] to exit. "
             "(New Users: Just pick a username)",
             SpecialColor);

    cprintf(slin, 1, ALIGN_LEFT, "#%d#%s",
            CyanColor,
            unamep); 

    cprintf(slin, 11, ALIGN_LEFT, "#%d#%s%c",
            tmpcolor,
            (state == S_GETUNAME)? prm.buf.c_str() : nm.c_str(),
            (state == S_GETUNAME) ? cursor: ' ');

    slin++;

    /* password(s) */
    if (state == S_GETPW || state == S_GETRPW)
    {
        cprintf(slin, 1, ALIGN_LEFT, "#%d#%s%c",
                CyanColor,
                pwp,
                (state == S_GETPW) ? cursor: ' ');

        if (state == S_GETRPW)
        {
            slin++;
            cprintf(slin, 1, ALIGN_LEFT, "#%d#%s%c",
                    CyanColor,
                    rpwp,
                    cursor);
        }
    }

    slin++;
    /* new user confirm */
    if (state == S_CONFIRMNEW)
        cprintf(slin, 1, ALIGN_LEFT, "#%d#%s#%d# %s%c",
                CyanColor,
                newuserp,
                NoColor,
                prm.buf.c_str(),
                cursor);

    /* status line */
    if (statlin)
        cprintf(MSG_LIN1, 0, ALIGN_LEFT, "#%d#%s",
                CyanColor,
                statlin);

    /* err line */
    if (errlin)
        cprintf(MSG_LIN2, 0, ALIGN_LEFT, "#%d#%s",
                RedColor,
                errlin);

    return NODE_OK_NO_PKTPROC;
}


static nodeStatus_t nAuthInput(int ch)
{
    int rv, irv;

    ch = CQ_CHAR(ch);
    irv = prmProcInput(prm, ch);
    switch (state)
    {
    case S_GETUNAME:
        if (irv > 0)
        {                       /* a terminator */
            /* we wish to leave. chicken... */
            if (irv == TERM_ABORT || prm.buf[0] == 0)
                return NODE_EXIT;

            /* check validity */
            if (!uaValidateUsername(prm.buf))
            {                   /* invalid username */
                mglBeep(MGL_BEEP_ERR);
                errlin = "Invalid character in username.";
                prm.buf.clear();
                return NODE_OK;
            }

            /* check if new user */
            if ((rv = sendAuth(cInfo.sock, CPAUTH_CHECKUSER,
                               prm.buf.c_str(), "")) < 0)
                return NODE_EXIT;       /* pkt error */

            // save the name
            nm = prm.buf;

            if (rv == PERR_NOUSER)
            {                   /* new */
                /* for a new user, we will want confirmation */
                state = S_CONFIRMNEW;

                prm.preinit = false;
                prm.buf.clear();
                prm.buflen = MAXUSERNAME;
                prm.pbuf = newuserp;

                errlin = NULL;
                return NODE_OK;
            }
            else
            {                   /* old user */
                state = S_GETPW;

                /* setup for the new prompt */
                prm.preinit = false;
                prm.buf.clear();
                prm.buflen = MAXUSERNAME;
                prm.pbuf = pwp;

                statlin = phelper;
                errlin = NULL;
                return NODE_OK;
            }
        }

        return NODE_OK;
        break;                    /* S_GETUNAME */

    case S_CONFIRMNEW:
        if (irv > 0)
        {                       /* a terminator */
            if (irv == TERM_NORMAL
                && prm.buf.size()
                && (prm.buf[0] == 'y' || prm.buf[0] == 'Y'))
            {                   /* confirming new user */
                state = S_GETPW;

                /* setup for the new prompt */
                prm.preinit = false;
                prm.buf.clear();
                prm.buflen = MAXUSERNAME;
                prm.pbuf = pwp;

                statlin = phelper;
                errlin = NULL;
                newuser = true;
            }
            else
            {                   /* oops */
                state = S_GETUNAME;

                prm.preinit = true;
                prm.buf = nm;
                prm.buflen = MAXUSERNAME;
                prm.pbuf = unamep;

                statlin = uhelper;
                errlin = NULL;
                newuser = false;
            }
        }

        return NODE_OK;
        break;                    /* S_CONFIRMNEW */

    case S_GETPW:
        if (irv > 0)
        {                       /* a terminator */
            // we have a password save it
            pw = prm.buf;

            /* if this was a new user, go straight to S_GETRPW */
            if (newuser)
            {
                state = S_GETRPW;

                /* setup for the new prompt */
                prm.preinit = false;
                prm.buf.clear();
                prm.buflen = MAXUSERNAME;
                prm.pbuf = rpwp;

                statlin = phelper;
                errlin = NULL;

                return NODE_OK;
            }

            if ((rv = sendAuth(cInfo.sock, CPAUTH_LOGIN,
                               nm.c_str(), pw.c_str())) < 0)
                return NODE_EXIT;   /* error */

            if (rv != PERR_OK)
            {                   /* invalid pw */
                /* clear it out and return to  P_GETUNAME state) */
                state = S_GETUNAME;

                pw.clear();

                prm.preinit = true;
                prm.buf = nm;
                prm.buflen = MAXUSERNAME;
                prm.pbuf = unamep;

                statlin = uhelper;

                errlin = pwerr;
            }
            else
            {                   /* valid login */
                nWelcomeInit();
                return NODE_OK;
            }


        }

        return NODE_OK;
        break;                    /* S_GETPW */

    case S_GETRPW:
        if (irv > 0)
        {                       /* a terminator */
            /* see if the passwords match */

            if (pw != prm.buf)
            {                   /* pw's don't match, start over */
                mglBeep(MGL_BEEP_ERR);

                errlin = "Passwords don't match.";
                /* go back to username prompt */
                state = S_GETUNAME;           /* initial state */
                prm.preinit = true;
                prm.buf = nm;
                prm.buflen = MAXUSERNAME;
                prm.pbuf = unamep;

                pw.clear();

                statlin = uhelper;
            }
            else
            {                   /* they match, lets goto welcome node */

                if ((rv = sendAuth(cInfo.sock, CPAUTH_LOGIN,
                                   nm.c_str(), pw.c_str())) < 0)
                    return NODE_EXIT;

                if (rv != PERR_OK)
                {
                    utLog("conquest: CPAUTH_LOGIN returned %d\n",
                          rv);
                    return NODE_EXIT;
                }

                nWelcomeInit();
                return NODE_OK;
            }

        }
        return NODE_OK;
        break;                    /* S_GETRPW */


    default:
        utLog("nAuthInput(): Unknown state: %d", state);
    }

    /* if we get here, something is confused */
    return NODE_EXIT;
}
