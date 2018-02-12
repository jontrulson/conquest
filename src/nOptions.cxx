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
#include "glmisc.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "conqutil.h"
#include "conqlb.h"
#include "prm.h"
#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nOptions.h"
#include "cqkeys.h"
#include "cqsound.h"
#include "udp.h"

static int retnode;             /* node to return to */
static int uopts = false;       /* are we doing user opts? */

#define S_NONE          0
#define S_USRMENU       1
#define S_SYSMENU       2
#define S_UOPTS         3       /* changing user opts */
#define S_MACROS        4       /* changing macros */
#define S_SOPTS         5       /* changing system opts */
#define S_GETPW         6       /* changing password */
#define S_GETRPW        7
#define S_HELP          8
#define S_MOUSE         9       /* changing mouse macros */
static int state;

static prm_t prm;
static int prompting;

static char cbuf[BUFFER_SIZE_256];

static const char *pwp = "Password: ";
static const char *rpwp = "Retype Password: ";
static const char *phelper = "Use any printable characters.";
static const char *nodef = "<Not Defined>";

static const char *mheader = "View/Edit Macros";
static const char *mmheader = "View/Edit Mouse Macros";
static const char *mmheader2 = "'a' = Alt, 'c' = Control 's' = Shift";
static const char *uheader = "User Configurable Options";
static const char *sheader = "System-wide Configurable Options";

static const char *meprompt = "Enter Conquest commands.  ([ENTER] = \\r, [TAB] = \\t)";
static const char *mmeprompt = "Enter Conquest commands.  ([ENTER] = \\r, [TAB] = \\t <angle> = \\a)";
static const char *eprompt = "Arrow keys to select an item, [SPACE] to change, any other key to quit.";
static const char *eprompt2 = "Type '?' for help on an item.";
static const char *mprompt = "Arrow keys to select a macro, [SPACE] to change, any other key to quit.";
static const char *mprompt2 = "Type '?' for help.";

static const int settingcol = 65;

static char cursor = ' ';       /* the cursor */

static const int umenuopts = 4; /* don't exceed 9 - one char input is used */

static int flin, llin, clin, pages, curpage;
static const int items_per_page = 18;

#define MAXOPTCOLS 128
static int cvec[MAXOPTCOLS];         /* hopefully big enough */
static int maxuopts;
static int maxsopts = 0;        /* not impl yet */
static char *macrovec[MAX_MACROS];

#define MAX_MOUSE (CONF_MAXBUTTONS * CONF_MAXMODIFIERS)
static char *mousevec[MAX_MOUSE];

#define MOUSE2VEC(_but, _mod)   ((_(but) * CONF_MAXMODIFIERS) + (_mod))
#define VEC2MOUSE(_vecnum, _but, _mod) {                        \
        (_but) = ((_vecnum) / CONF_MAXMODIFIERS);               \
        (_mod) = (((_vecnum) - ((_but) * CONF_MAXMODIFIERS)));  \
    }

static struct Conf *macroptr = NULL; /* points to macro element of ConfData */
static struct Conf *mouseptr = NULL; /* points to mouse macro element of ConfData */
static struct Conf *ConfigData = NULL;

static const char *oheader = "";

static int oldstate;            /* for returning from help */

static nodeStatus_t nOptionsDisplay(dspConfig_t *);
static nodeStatus_t nOptionsIdle(void);
static nodeStatus_t nOptionsInput(int ch);

static scrNode_t nOptionsNode = {
    nOptionsDisplay,              /* display */
    nOptionsIdle,                 /* idle */
    nOptionsInput,                /* input */
    NULL,                         /* minput */
    NULL                          /* animQue */

};

/* return to the correct node, and (re)set certain options */
static void quitNode(void)
{
    SaveUserConfig();
    Context.updsec = UserConf.UpdatesPerSecond;
    sendCommand(CPCMD_SETRATE, Context.updsec);

    /* set the volumes */
    cqsUpdateVolume();

    switch (retnode)
    {
    case DSP_NODE_CP:
        setONode(NULL);
        nCPInit(false);
        break;
    case DSP_NODE_MENU:
    default:
        setONode(NULL);
        nMenuInit();
        break;
    }
    return;
}

static void _changePasswd(int init)
{
    static char pw[MAXUSERNAME], rpw[MAXUSERNAME];

    if (init)
    {                           /* start prompting */
        prm.pbuf = pwp;
        pw[0] = 0;
        prm.buf = pw;

        prm.preinit = false;
        prm.buflen = MAXUSERNAME - 1;
        prm.terms = TERMS;
        prm.index = MSG_LIN1;
        prompting = true;
    }
    else
    {
        if (state == S_GETPW)
        {                       /* got the pw, set up for the retry */
            state = S_GETRPW;
            prm.pbuf = rpwp;
            rpw[0] = 0;
            prm.buf = rpw;

            prm.preinit = false;
            prm.buflen = MAXUSERNAME - 1;
            prm.terms = TERMS;
            prm.index = MSG_LIN1;
            prompting = true;
        }
        else
        {                       /* done. do a compare and send new pw
                                   if they match */
            if (strcmp(pw, rpw) != 0)
            {                       /* pw's don't match */
                mglBeep(MGL_BEEP_ERR);
                state = S_USRMENU;
                return;
            }
            else
                sendAuth(cInfo.sock, CPAUTH_CHGPWD, "", pw);

            prompting = false;
            state = S_USRMENU;
        }

    }

    return;
}

static void _changeMacro(int macronum, int init)
{
    int lin;
    static char prmpt[BUFFER_SIZE_256];
    lin = MSG_LIN1;

    if (init)
    {                           /* start prompting */
        sprintf(prmpt, "f%2d = ", macronum);
        strcpy(cbuf, Macro2Str(UserConf.MacrosF[macronum - 1]));

        prm.preinit = true;
        prm.buf = cbuf;
        prm.buflen = MAX_MACRO_LEN - 1;
        prm.pbuf = prmpt;
        prm.terms = TERMS;
        prm.index = lin;
        prompting = true;

    }
    else
    {                           /* change it for real */
        utStrncpy(UserConf.MacrosF[macronum - 1], Str2Macro(prm.buf),
                  MAX_MACRO_LEN);
    }

    return;
}

static void _changeMouse(int mousevec, int init)
{
    int lin;
    uint32_t mod, but;
    static char prmpt[BUFFER_SIZE_256];
    static char modstr[16];
    lin = MSG_LIN1;

    VEC2MOUSE(mousevec, but, mod);

    if (init)
    {                           /* start prompting */
        modstr[0] = 0;

        if (mod & (CQ_KEY_MOD_CTRL  >> CQ_MODIFIER_SHIFT))
            strcat(modstr, "c");
        if (mod & (CQ_KEY_MOD_SHIFT >> CQ_MODIFIER_SHIFT))
            strcat(modstr, "s");
        if (mod & (CQ_KEY_MOD_ALT   >> CQ_MODIFIER_SHIFT))
            strcat(modstr, "a");

        sprintf(prmpt, "%3s%2d = ", modstr, but);
        strcpy(cbuf, Macro2Str(UserConf.Mouse[but][mod]));

        prm.preinit = true;
        prm.buf = cbuf;
        prm.buflen = MAX_MACRO_LEN - 1;
        prm.pbuf = prmpt;
        prm.terms = TERMS;
        prm.index = lin;
        prompting = true;

    }
    else
    {                           /* change it for real */
        utStrncpy(UserConf.Mouse[but][mod], Str2Macro(prm.buf),
                  MAX_MACRO_LEN);
    }

    return;
}

static void _changeOption(struct Conf *cdata, int init)
{
#define CBUFLEN 128
    int j;

    switch(cdata->ConfType)
    {
    case CTYPE_BOOL:
        j = *(int *)(cdata->ConfValue);

        if (j == true)
            j = false;
        else
            j = true;

        *(int *)(cdata->ConfValue) = j;

        break;

        /* these will need prompting */
    case CTYPE_STRING:
        if (cdata->max > CBUFLEN)
	{
            utLog("_changeOption: conf data max exceeds local buffer size.");
            break;
	}

        if (init)
        {
            prm.preinit = true;
            utStrncpy(cbuf, ((char *)cdata->ConfValue), CBUFLEN);
            prm.buf = cbuf;
            prm.buflen = cdata->max - 1;
            prm.pbuf = "Value: ";
            prm.terms = TERMS;
            prm.index = MSG_LIN1;
            prompting = true;
        }
        else
        {
            utStrncpy((char *)cdata->ConfValue, prm.buf, cdata->max);
        }

        break;

    case CTYPE_NUMERIC:
        if (init)
        {
            prm.preinit = true;
            sprintf(cbuf, "%d", *(int *)(cdata->ConfValue));
            prm.buf = cbuf;
            prm.buflen = 20;
            prm.pbuf = "Enter a number: ";
            prm.terms = TERMS;
            prm.index = MSG_LIN1;
            prompting = true;
        }
        else
        {
            if (utIsDigits(prm.buf))
            {
                int j = atoi(prm.buf);
                if (j >= cdata->min && j <= cdata->max)
                    *(int *)(cdata->ConfValue) = j;
            }
        }
        break;
    }

    return;
}

static void _dispUserOptsMenu(void)
{
    static const char *header = "User Options Menu";
    static const char *mopts[] = {
        "View/Edit Options",
        "View/Edit Macros",
        "View/Edit Mouse Macros",
        "Change Password"
    };
    static const char *prompt = "Enter a number to select an item, any other key to quit.";
    int lin = 0, col = 0;
    int i;
    static unsigned int lastReadSeq = 0;
    static uint32_t udpLastRead = 0;
    static const uint32_t udpUpdateDelay = 1000; // 1 second update rate
    uint32_t currentMillis = cInfo.nodeMillis;

    if (macroptr == NULL)
    {				/* if this happens, something is
				   seriously confused */
        utLog("_dispUserOptsMenu(): ERROR: macroptr == NULL, no CTYPE_MACRO found in ConfData");
    }

    if (mouseptr == NULL)
    {				/* if this happens, something is
				   seriously confused */
        utLog("_dispUserOptsMenu(): ERROR: mouseptr == NULL, no CTYPE_MOUSE found in ConfData");
    }

    /* First clear the display. */
    lin = 1;
    cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", NoColor, header);

    lin += 3;
    col = 5;

    for (i = 0; i < umenuopts; i++)
    {
        cprintf(lin, col, ALIGN_NONE, "#%d#%d.#%d# %s#%d#", InfoColor,
                i + 1, NoColor, mopts[i], NoColor);
        lin++;
    }

    lin = 17;
    cprintf(lin, col, ALIGN_NONE, "#%d#Server Name:   #%d# %s#%d#", MagentaColor,
            InfoColor, sHello.servername,
            NoColor);
    lin++;
    cprintf(lin, col, ALIGN_NONE, "#%d#Server Version: #%d#%s#%d#", MagentaColor,
            InfoColor, sHello.serverver,
            NoColor);
    lin++;
    cprintf(lin, col, ALIGN_NONE, "#%d#UDP:  #%d# %s#%d#", MagentaColor,
            InfoColor, (cInfo.doUDP) ? "On" : "Off",
            NoColor);

    if (cInfo.doUDP)
    {
        if ((currentMillis - udpLastRead) > udpUpdateDelay)
        {
            lastReadSeq = udpGetReadSeq();
            udpLastRead = currentMillis;
        }

        cprintf(lin, col + 11, ALIGN_NONE, "#%d#Stats: OoO: #%d#%u #%d# Dup: #%d#%u #%d#Lost: #%d#%u #%d#readSeq: #%d#%u",
                MagentaColor,
                InfoColor, pktStats.oooPackets,
                MagentaColor,
                InfoColor, pktStats.duplicatePackets,
                MagentaColor,
                InfoColor, pktStats.lostPackets,
                MagentaColor,
                InfoColor, lastReadSeq);
    }

    lin++;
    cprintf(lin, col, ALIGN_NONE, "#%d#Flags:#%d#", MagentaColor,
            InfoColor);

    lin = clntPrintServerFlags(lin, col + 7, sStat.serverFlags, InfoColor);

    if (!prompting)
    {
        hudClearPrompt(MSG_LIN1);
        hudClearPrompt(MSG_LIN2);
        cprintf(MSG_LIN2, 0, ALIGN_NONE, "%s", prompt);
    }

    return;
}

static void _initOptsScreen(void)
{
    curpage = 0;
    flin = 4;                     /* first option line */
    llin = 0;                     /* last option line */
    clin = 0;                     /* current option line (also idx into
                                     cvec/macrovec) */

    /* set the right header */
    switch (state)
    {
    case S_UOPTS:
        oheader = uheader;
        if (maxuopts >= items_per_page)
        {
            pages = maxuopts / items_per_page;
            if ((maxuopts % items_per_page) != 0)
                pages++;                /* for runoff */
        }
        else
            pages = 1;

        break;

    case S_SOPTS:
        /* not implemented */
        oheader = sheader;
        break;

    case S_MACROS:
        oheader = mheader;
        if (MAX_MACROS >= items_per_page)
        {
            pages = MAX_MACROS / items_per_page;
            if ((MAX_MACROS % items_per_page) != 0)
                pages++;                /* for runoff */
        }
        else
            pages = 1;

        break;

    case S_MOUSE:
        oheader = mmheader;
        if (MAX_MOUSE >= items_per_page)
        {
            pages = MAX_MOUSE / items_per_page;
            if ((MAX_MOUSE % items_per_page) != 0)
                pages++;                /* for runoff */
        }
        else
            pages = 1;

        break;
    }

    return;
}

/* DisplayHelpScreen() - display a help (actually the conf item comment)
 *                       screen for a Configuration item.
 */
static void _dispHelpScreen(void)
{
    int i, col, lin;
    struct Conf *confitem;

    if (oldstate == S_UOPTS)
        confitem = &ConfigData[cvec[(curpage * items_per_page) + clin]];
    else if (oldstate == S_MACROS)
        confitem = macroptr;
    else
        confitem = mouseptr;


    lin = 1;
    col = 0;

    cprintf(lin, col, ALIGN_CENTER, "#%d#%s#%d#",
            NoColor, confitem->OneLineDesc, NoColor);

    lin += 2;

    i = 0;

    while (confitem->ConfComment[i] != NULL)
    {
        if (confitem->ConfComment[i][0] != 0)
	{
            cprintf(lin, col, ALIGN_NONE, "#%d#%s",
                    InfoColor,
                    &(confitem->ConfComment[i][1]));
            lin++;
	}
        i++;
    }

    cprintf(MSG_LIN2, 0, ALIGN_CENTER, MTXT_DONE);

    return;
}

static void _showOptScreen(void)
{
    int i, j, k;
    static const char *header2fmt = "(Page %d of %d)";
    static char headerbuf[BUFFER_SIZE_256];
    static char header2buf[BUFFER_SIZE_256];
    static const char *header;
    static const char *prompt;
    static const char *prompt2;
    int vattrib;
    const char *dispmac;
    int lin = 0, col = 0;

    switch (state)
    {
    case S_UOPTS:
        header = uheader;
        prompt = eprompt;
        prompt2 = eprompt2;
        if (curpage == (pages - 1)) /* last page - might be less than full */
        {
            if (maxuopts == items_per_page)
                llin = items_per_page;
            else
                llin = (maxuopts % items_per_page); /* ..or more than empty? ;-) */
        }
        else
            llin = items_per_page;

        break;

    case S_SOPTS:
        header = sheader;
        prompt = eprompt;
        prompt2 = eprompt2;
        if (curpage == (pages - 1)) /* last page - might be less than full */
            llin = (maxsopts % items_per_page); /* ..or more than empty? ;-) */
        else
            llin = items_per_page;

        break;

    case S_MACROS:
        header = mheader;
        prompt = mprompt;
        prompt2 = mprompt2;
        if (curpage == (pages - 1)) /* last page - might be less than full */
            llin = (MAX_MACROS % items_per_page); /* ..or more than empty? ;-) */
        else
            llin = items_per_page;
        break;

    case S_MOUSE:
        header = mmheader;
        prompt = mprompt;
        prompt2 = mprompt2;
        if (curpage == (pages - 1)) /* last page - might be less than full */
            llin = (MAX_MOUSE % items_per_page); /* ..or more than empty? ;-) */
        else
            llin = items_per_page;
        break;

    default:                    /* shouldn't get here */
        header = "";
        break;
    }

    sprintf(header2buf, header2fmt, curpage + 1, pages);
    sprintf(headerbuf, "%s %s", header, header2buf);

    lin = 1;
    cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", NoColor, headerbuf);

    if (state == S_MOUSE)         /* need an extra header for mouse */
        cprintf(lin + 1, 0, ALIGN_CENTER, "#%d#%s", NoColor, mmheader2);

    lin = flin;
    col = 1;

    i = 0;			/* start at index 0 */

    while (i < llin)
    {			/* display this page */

        if (state == S_MACROS)
        {
            /* get the macro number for this line */
            k = (curpage * items_per_page) + i;

            if (macrovec[k][0] == 0)
            {			/* not defined */
                dispmac = nodef;
                vattrib = RedLevelColor;
            }
            else
            {
                dispmac = Macro2Str(macrovec[k]);
                vattrib = GreenLevelColor;
            }

#ifdef DEBUG_OPTIONS
            utLog("_showOptScreen(): k = %d, dispmac = '%s'",
                  k,
                  dispmac);
#endif
            cprintf(lin, col, ALIGN_NONE, "#%d#f%2d = #%d#%s#%d#",
                    (k == ((curpage * items_per_page) + clin)) ? RedColor : InfoColor,
                    k + 1, vattrib,
                    dispmac,
                    NoColor);

            lin++;
            i++;
        }

        if (state == S_MOUSE)
        {
            static char modstr[16];
            uint32_t mod, but;

            /* get the mouse macro number for this line */
            k = (curpage * items_per_page) + i;

            VEC2MOUSE(k, but, mod);
            modstr[0] = 0;

            if (mod & (CQ_KEY_MOD_CTRL  >> CQ_MODIFIER_SHIFT))
                strcat(modstr, "c");
            if (mod & (CQ_KEY_MOD_SHIFT >> CQ_MODIFIER_SHIFT))
                strcat(modstr, "s");
            if (mod & (CQ_KEY_MOD_ALT   >> CQ_MODIFIER_SHIFT))
                strcat(modstr, "a");

            if (mousevec[k][0] == 0)
            {			/* not defined */
                dispmac = nodef;
                vattrib = RedLevelColor;
            }
            else
            {
                dispmac = Macro2Str(mousevec[k]);
                vattrib = GreenLevelColor;
            }

#ifdef DEBUG_OPTIONS
            utLog("_showOptScreen(): k = %d, dispmac = '%s'",
                  k,
                  dispmac);
            utLog("\t: but = %d, mod = %d",
                  but,
                  mod);
#endif
            cprintf(lin, col, ALIGN_NONE, "#%d#%3s%2d = #%d#%s#%d#",
                    (k == ((curpage * items_per_page) + clin)) ? RedColor : InfoColor,
                    modstr, but, vattrib,
                    dispmac,
                    NoColor);

            lin++;
            i++;
        }

        if (state == S_UOPTS)
        {
            k = (curpage * items_per_page) + i;

            cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d#",
                    (k == ((curpage * items_per_page) + clin)) ? RedColor : InfoColor,
                    ConfigData[cvec[k]].OneLineDesc,
                    NoColor);


            switch(ConfigData[cvec[k]].ConfType)
            {
            case CTYPE_BOOL:
                j = *(int *)ConfigData[cvec[k]].ConfValue;

                if (j == true)
                    vattrib = GreenLevelColor;
                else
                    vattrib = RedLevelColor;

                cprintf(lin, settingcol, ALIGN_NONE, "#%d#%s#%d#",
                        vattrib, (j == true) ? "True" : "False",
                        NoColor);
                break;

            case CTYPE_STRING:
                cprintf(lin, settingcol, ALIGN_NONE, "#%d#%13s#%d#",
                        InfoColor, (char *) ConfigData[cvec[k]].ConfValue,
                        NoColor);
                break;

            case CTYPE_NUMERIC:
                j = *(int *)ConfigData[cvec[k]].ConfValue;
                cprintf(lin, settingcol, ALIGN_NONE, "#%d#%d#%d#",
                        NoColor, j,
                        NoColor);
                break;


            } /* switch */


            lin++;
            i++;
        }

    }


    /* now the editing phase */
    if (!prompting)
    {
        cprintf(MSG_LIN1, 1, ALIGN_NONE, "%s", prompt);
        cprintf(MSG_LIN2, 1, ALIGN_NONE, "%s", prompt2);
    }

    return;
}


scrNode_t *nOptionsInit(int what, int setnode, int rnode)
{
    int i, k;
    struct Conf *ConfigData;

    uopts = (what == NOPT_USER) ? true : false;
    prompting = false;

    if (uopts)
    {
        state = S_USRMENU;
        ConfigData = ConfData;
    }
    else
    {
        state = S_SYSMENU;
        ConfigData = SysConfData;
    }

    retnode = rnode;

    for (i=0; i < MAXOPTCOLS; i++) /* init cvec */
        cvec[i] = -1;

    k = 0;
    for (i=0; i < CfEnd; i++)
    {
        if (ConfigData[i].ConfType != CTYPE_STRING &&
            ConfigData[i].ConfType != CTYPE_BOOL &&
            ConfigData[i].ConfType != CTYPE_NUMERIC)
        {           /* skip special elements */
            continue;
        }

        cvec[k++] = i;
    }

    maxuopts = k;
    if (state == S_USRMENU)
    {
        /* init the macrovec array */
        for (i=0; i < CfEnd; i++)
        {
            if (ConfData[i].ConfType == CTYPE_MACRO)
            {
                macroptr = &ConfData[i];
            }

            if (ConfData[i].ConfType == CTYPE_MOUSE)
            {
                mouseptr = &ConfData[i];
            }
        }

        if (macroptr)
            for (i=0; i < MAX_MACROS; i++)
            {
                macrovec[i] = (char *)(((char *)macroptr->ConfValue)
                                       + (i * MAX_MACRO_LEN));
            }

        if (mouseptr)
        {
            uint32_t but, mod;

            for (i=0; i < MAX_MOUSE; i++)
            {
                VEC2MOUSE(i, but, mod);
                mousevec[i] = UserConf.Mouse[but][mod];
            }
        }
    }

    if (setnode)
        setNode(&nOptionsNode);

    return(&nOptionsNode);
}


static nodeStatus_t nOptionsDisplay(dspConfig_t *dsp)
{

    if (state == S_USRMENU || state == S_GETPW || state == S_GETRPW)
        _dispUserOptsMenu();

    if (state == S_GETPW || state == S_GETRPW)
    {
        cprintf(MSG_LIN1, 1, ALIGN_NONE, "#%d#%s%c",
                CyanColor,
                prm.pbuf,
                cursor);

        cprintf(MSG_LIN2, 0, ALIGN_NONE, "#%d#%s",
                NoColor, phelper);
    }

    if ((state == S_UOPTS) || (state == S_MACROS) || (state == S_MOUSE))
    {
        _showOptScreen();

        if (prompting)
        {
            cprintf(prm.index, 0, ALIGN_NONE, "#%d#%s #%d#%s%c",
                    InfoColor, prm.pbuf, MagentaColor, prm.buf, cursor);

            if (state == S_MACROS)
                cprintf(MSG_LIN2, 0, ALIGN_NONE, "#%d#%s",
                        NoColor, meprompt);
            if (state == S_MOUSE)
                cprintf(MSG_LIN2, 0, ALIGN_NONE, "#%d#%s",
                        NoColor, mmeprompt);
        }

    }

    if (state == S_HELP)
        _dispHelpScreen();

    return NODE_OK;
}

static nodeStatus_t nOptionsIdle(void)
{
    int gtime = frameTime;
    static int old = 0;

    /* 'blink' the cursor */
    if ((gtime - old) > 250)
    {
        if (cursor == ' ')
            cursor = '_';
        else
            cursor = ' ';

        old = gtime;
    }

    if ((state == S_SYSMENU) || (state == S_SOPTS))
        return NODE_OK;             /* don't process packets in sys modes */

    if ((retnode == DSP_NODE_CP) && (clientStatLastFlags & SPCLNTSTAT_FLAG_KILLED))
    {
        /* if we were flying, time to die properly. */
        setONode(NULL);
        nDeadInit();
        return NODE_OK;
    }

    return NODE_OK;
}

static nodeStatus_t nOptionsInput(int ch)
{
    int irv;

    ch = CQ_CHAR(ch) | CQ_FKEY(ch);

    if (prompting)
    {
        irv = prmProcInput(&prm, ch);

        switch (state)
        {
        case S_MACROS:
            if (irv > 0)
            {
                if (irv != TERM_ABORT)
                    _changeMacro((curpage * items_per_page) + clin + 1, false);

                prompting = false;
            }
            break;

        case S_MOUSE:
            if (irv > 0)
            {
                if (irv != TERM_ABORT)
                    _changeMouse((curpage * items_per_page) + clin, false);

                prompting = false;
            }
            break;

        case S_UOPTS:
            if (irv > 0)
            {
                if (irv != TERM_ABORT)
                    _changeOption(&ConfigData[cvec[(curpage * items_per_page) + clin]], false);

                prompting = false;
            }
            break;

        case S_GETRPW:
        case S_GETPW:
            if (irv > 0)
            {
                if (irv != TERM_ABORT)
                {
                    _changePasswd(false);
                }
                else
                {
                    prompting = false;
                    state = S_USRMENU;
                }
            }
            break;

        }

        return NODE_OK;
    }


    if (state == S_USRMENU)
    {                           /* decide what to do. */
        switch (ch)
        {
        case '1':               /* user options */
            state = S_UOPTS;
            ConfigData = ConfData;
            _initOptsScreen();
            break;

        case '2':
            state = S_MACROS;
            _initOptsScreen();
            break;

        case '3':
            state = S_MOUSE;
            _initOptsScreen();
            break;

        case '4':
            state = S_GETPW;
            _changePasswd(true);
            break;

        default:
            quitNode();
            break;
        }


        return NODE_OK;
    }

    if (state == S_UOPTS || state == S_MACROS || state == S_MOUSE)
    {
        int maxitms = ((state == S_UOPTS) ? maxuopts : ((state == S_MACROS) ? MAX_MACROS : MAX_MOUSE));

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
                /* setup llin for current page */
                if (curpage == (pages - 1))
                {
                    if (maxitms == items_per_page)
                        llin = items_per_page;
                    else
                        llin = (maxitms % items_per_page);
                }
                else
                    llin = items_per_page;
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
                /* setup llin for current page */
                if (curpage == (pages - 1))
                    llin = (maxitms % items_per_page);
                else
                    llin = items_per_page;

                if (clin >= llin)
                    clin = llin - 1;
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
                /* setup llin for current page */
                if (curpage == (pages - 1))
                    llin = (maxitms % items_per_page);
                else
                    llin = items_per_page;

                if (clin >= llin)
                    clin = llin - 1;
	    }

            break;

	case ' ':	/* change something */
            if (state == S_MACROS)
                _changeMacro((curpage * items_per_page) + clin + 1, true);
            else if (state == S_MOUSE)
                _changeMouse((curpage * items_per_page) + clin, true);
            else if (state == S_UOPTS)
                _changeOption(&ConfigData[cvec[(curpage * items_per_page) + clin]], true);
            break;

	case '?':
            oldstate = state;
            state = S_HELP;
            break;

	default:		/* everything else */
            state = S_USRMENU;    /* return to the user menu. */
            break;
	}

        return NODE_OK;

    }

    if (state == S_HELP)
    {
        state = oldstate;
        return NODE_OK;
    }

    quitNode();
    return NODE_OK;
}
