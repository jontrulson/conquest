#include "c_defs.h"

/************************************************************************
 *
 * options.c - handle view/edit options feature
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 *
 ***********************************************************************/

#include "global.h"
#include "conqdef.h"
#include "cb.h"
#include "color.h"
#include "ui.h"
#include "conf.h"
#include "clntauth.h"
#include "cd2lb.h"
#include "iolb.h"
#include "client.h"
#include "clientlb.h"
#include "conqutil.h"

#include "context.h"

struct compile_options {
    const char *name;
    const char *oneliner;
    int type;			/* We'll use the CTYPE_*'s from conf.h */
    void *value;
};

static void ChangeOption(struct Conf *cdata, int lin);
static void DisplayCompileOptions(void);
static int ViewEditOptions(struct Conf ConfigData[], int ConfSize,
			   int editable);
static void DisplayHelpScreen(struct Conf *confitem);

static int ChangedSomething = false;


/*************************************************************************
 * DisplayCompileOptions() - do what name indicates
 *
 *
 *************************************************************************/

static void DisplayCompileOptions(void)
{
    const int settingcol = 60;
    int i, vattrib;
    long j = 0;
    int lin = 0, col = 0;
    static const char *header = "Compile Time Options";
    static const char *prompt = MTXT_DONE;

    static struct compile_options CompileOptions[] =
        {
            {"HAVE_SETITIMER", "can do fastupdates (2 per sec)", CTYPE_BOOL,
#ifdef HAVE_SETITIMER
             (void *)true
#else
             (void *)false
#endif /* HAVE_SETITIMER */
            },

            {"WARP0CLOAK", "can't be seen when cloaked at warp 0", CTYPE_BOOL,
#ifdef WARP0CLOAK
             (void *)true
#else
             (void *)false
#endif /* WARP0CLOAK */
            },

            {"DO_EXPLODING_SHIPS", "ships explode when they die", CTYPE_BOOL,
#ifdef DO_EXPLODING_SHIPS
             (void *)true
#else
             (void *)false
#endif /* DO_EXPLODING_SHIPS */
            },

            /* last (NULL) entry */
            {NULL, NULL, CTYPE_NULL, (void *)NULL}
        };

    lin = 1;
    col = ((int)(Context.maxcol - strlen(header)) / 2);

    cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, header);

    lin += 3;
    col = 2;

    i = 0;

    while (CompileOptions[i].name != NULL)
    {				/* this won't handle more than one screen */
        cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d# - %s#%d#",
                NoColor, CompileOptions[i].name,
                InfoColor, CompileOptions[i].oneliner,
                NoColor);

        /* output the value */
        switch(CompileOptions[i].type)
	{
	case CTYPE_BOOL:
            j = (long)CompileOptions[i].value;

            if (j == (long)true)
                vattrib = GreenLevelColor;
            else
                vattrib = RedLevelColor;

            cprintf(lin, settingcol, ALIGN_NONE, "#%d#%s#%d#",
                    vattrib, (j == true) ? "True" : "False",
                    NoColor);
            lin++;

            break;

	case CTYPE_STRING:
            cprintf(lin, settingcol, ALIGN_NONE, "#%d#%s#%d#",
                    InfoColor, (char *) CompileOptions[i].value,
                    NoColor);
            lin++;

            break;
	} /* switch */

        i++;
    }

    cdclrl( MSG_LIN1, 2  );
    cdputc(prompt, MSG_LIN1);

    /* Get a char */
    (void)iogchar();

    return;
}


/*************************************************************************
 * SysOptsMenu() - display System options menu
 *
 *
 *************************************************************************/

void SysOptsMenu(void)
{
    static const char *header = "System Options Menu";
    static const char *mopts[] = {
        "View compile-time Options",
        "View/Edit System-wide Options"
    };
    const int numoptions = 2;	/* don't exceed 9 - one char input is used */

    static const char *prompt = "Enter a number to select an item, any other key to quit.";
    int lin = 0, col = 0;

    int i;
    int ch;
    int Done = false;


    while (Done == false)
    {
        /* First clear the display. */
        cdclear();
        lin = 1;
        col = ((int)(Context.maxcol - strlen(header))/ 2);

        cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, header);

        lin += 3;
        col = 5;

        for (i = 0; i < numoptions; i++)
	{
            cprintf(lin, col, ALIGN_NONE, "#%d#%d.#%d# %s#%d#", InfoColor,
                    i + 1, LabelColor, mopts[i], NoColor);
            lin++;
	}

        cdclrl( MSG_LIN1, 2  );
        cdputs(prompt, MSG_LIN1, 1);

        /* Get a char */
        ch = iogchar();

        switch(ch)
	{
	case '1':			/* compile time options */
            DisplayCompileOptions();
            break;
	case '2':			/* sys-wide opts */
            ChangedSomething = false;
            ViewEditOptions(SysConfData, SysCfEnd, true);
            if (ChangedSomething == true)
                SaveSysConfig();
            break;
	default:
            Done = true;
            break;
	}
    }

    return;
}


/*************************************************************************
 * ChangeOption() - change an option (or prompt for new value)
 *
 *
 *************************************************************************/

static void ChangeOption(struct Conf *cdata, int lin)
{
#define CBUFLEN 1024
    int j, rv;
    char buf[CBUFLEN];

    switch(cdata->ConfType)
    {
    case CTYPE_BOOL:
        j = *(int *)(cdata->ConfValue);

        if (j == true)
            j = false;
        else
            j = true;

        *(int *)(cdata->ConfValue) = j;

        /* signal that something has been changed */
        ChangedSomething = true;

        break;

        /* these will need prompting on line 'lin' */
    case CTYPE_STRING:
        if (cdata->max > CBUFLEN)
	{
            utLog("ChangeOption: conf data max exceeds local buffer size.");
            break;
	}
        cdclrl(lin, 1);
        utStrncpy(buf, ((char *)cdata->ConfValue), CBUFLEN);
        rv = cdgets("Value: ", lin, 1, buf, cdata->max - 1);

        if (rv != -1)
	{
            utStrncpy((char *)cdata->ConfValue, buf, cdata->max);
            /* signal that something has been changed */
            ChangedSomething = true;
	}

        break;

    case CTYPE_NUMERIC:
        cdclrl(lin, 1);
        rv = cdgetn("Enter a number: ", lin, 1, &j);

        if (rv != -1)
	{
            if (j >= cdata->min && j <= cdata->max)
	    {
                *(int *)(cdata->ConfValue) = j;
                /* signal that something has been changed */
                ChangedSomething = true;
	    }
	}

        break;
    }

    return;
}


/*************************************************************************
 * ViewEditOptions() - View or edit an options array
 *
 *
 *************************************************************************/

static int ViewEditOptions(struct Conf ConfigData[], int ConfSize,
			   int editable)
{
    int i, j, k;
    static const char *uheader = "User Configurable Options";
    static const char *sheader = "System-wide Configurable Options";
    static const char *header;
    static const char *eprompt = "Arrow keys to select an item, [SPACE] to change, any other key to quit.";
    static const char *vprompt = MTXT_DONE;
    static const char *eprompt2 = "Type '?' for help on an item.";
    int ch;
    int lin = 0, col = 0, flin, llin = 0, clin;
    const int settingcol = 65;
    int vattrib;
#define MAXOPTCOLS 128
    int cvec[MAXOPTCOLS];		/* hopefully big enough */
    int usingSysOpts;

    usingSysOpts = ((ConfigData == SysConfData) ? true : false);

    if (usingSysOpts)
        header = sheader;
    else
        header = uheader;

    cdclear();			/* First clear the display. */

    flin = 4;			/* first option line */

#ifdef DEBUG_OPTIONS
    int maxllin = MSG_LIN1 - 2;	/* max last option line */
#endif

    clin = 0;			/* current option line (also idx into cvec) */

				/* clear out cvec */

    for (i=0; i < MAXOPTCOLS; i++)
        cvec[i] = -1;

    bool done = false;
    while (!done)
    {
        cdclrl( 1, MSG_LIN2);	/* clear screen area */
        lin = 1;
        col = ((int)(Context.maxcol - strlen(header))/ 2);

        cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, header);

        lin = flin;
        col = 1;

        i = 0;			/* start at index 0 */
        k = 0;			/* init cvec */

        while (i < ConfSize)
	{
#ifdef DEBUG_OPTIONS
            utLog("ViewEditOptions(): ConfSize = %d, i = %d, type = %d, %s",
                  ConfSize, i, ConfigData[i].ConfType,
                  ConfigData[i].OneLineDesc);
#endif
            if (ConfigData[i].ConfType != CTYPE_STRING &&
                ConfigData[i].ConfType != CTYPE_BOOL &&
                ConfigData[i].ConfType != CTYPE_NUMERIC)
	    {		/* skip special elements */
#ifdef DEBUG_OPTIONS
                utLog("ViewEditOptions():\tSKIPPING");
#endif
                i++;
                continue;
	    }

            cvec[k++] = i;

            cprintf(lin, col, ALIGN_NONE, "#%d#%s#%d#",
                    InfoColor, ConfigData[i].OneLineDesc,
                    NoColor);

            switch(ConfigData[i].ConfType)
	    {
	    case CTYPE_BOOL:
                j = *(int *)ConfigData[i].ConfValue;

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
                        InfoColor, (char *) ConfigData[i].ConfValue,
                        NoColor);
                break;

	    case CTYPE_NUMERIC:
                j = *(int *)ConfigData[i].ConfValue;
                cprintf(lin, settingcol, ALIGN_NONE, "#%d#%d#%d#",
                        NoColor, j,
                        NoColor);
                break;


	    } /* switch */
            lin++;
            i++;
	}

        /* now the editing phase */
        llin = k - 1; // last option line

#ifdef DEBUG_OPTIONS
        utLog("ViewEditOptions(): maxllin = %d, llin = %d, k = %d", maxllin, llin, k);
#endif

        cdclrl( MSG_LIN1, 2  );
        if (editable)
	{
            cdputs(eprompt, MSG_LIN1, 1);
            cdputs(eprompt2, MSG_LIN2, 1);
            cdmove(flin + clin, 1);
	}
        else
            cdputc(vprompt, MSG_LIN1);

        /* Get a char */
        ch = iogchar();

        if (!editable)
	{
            break;		/* exit here after viewing */
	}

        switch(ch)
	{
	case KEY_UP:		/* up */
	case KEY_LEFT:
	case 'w':
	case 'k':
            clin--;
            if (clin < 0)
	    {
                clin = llin;
	    }
            break;

	case KEY_DOWN:		/* down */
	case KEY_RIGHT:
	case 'x':
	case 'j':
            clin++;
            if (clin > llin)
	    {
                clin = 0;
	    }
            break;

	case ' ':	/* change something */
            ChangeOption(&ConfigData[cvec[clin]], MSG_LIN1);
            break;

	case '?':
            DisplayHelpScreen(&ConfigData[cvec[clin]]);
            break;

	default:		/* everything else */
            done = true;
            break;
	}

#ifdef DEBUG_OPTIONS
        utLog("ViewEditOptions():###\tllin = %d, clin = %d", llin, clin);
#endif
    }

    return true;
}

/* DisplayHelpScreen() - display a help (actually the conf item comment)
 *                       screen for a Configuration item.
 */
static void DisplayHelpScreen(struct Conf *confitem)
{
    int i, col, lin;

    cdclear();

    lin = 1;
    col = 0;

    cprintf(lin, col, ALIGN_CENTER, "#%d#%s#%d#",
            NoColor, confitem->OneLineDesc, NoColor);

    lin += 2;

    i = 0;
    uiPutColor(InfoColor);

    while (confitem->ConfComment[i] != NULL)
    {
        if (confitem->ConfComment[i][0] != 0)
	{
            cdputs(&(confitem->ConfComment[i][1]), lin, col);
            lin++;
	}
        i++;
    }

    uiPutColor(NoColor);
    cdclrl( MSG_LIN1, 2  );
    cdputc(MTXT_DONE, MSG_LIN2);

    /* Get a char */
    (void)iogchar();

    return;
}
