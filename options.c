#include "c_defs.h"

/************************************************************************
 *
 * options.c - handle view/edit options feature
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 *
 ***********************************************************************/

#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "color.h"
#include "conf.h"
#include "clntauth.h"
#include "client.h"
#include "clientlb.h"

#include "context.h"
struct compile_options {
  char *name;
  char *oneliner;
  int type;			/* We'll use the CTYPE_*'s from conf.h */
  void *value;
};

static void ChangeOption(struct Conf *cdata, int lin);
static void ChangeMacro(int macronum);
static void DisplayCompileOptions(void);
static int ViewEditOptions(struct Conf ConfigData[], int ConfSize, 
			   int editable);
static int ViewEditMacros(struct Conf *ConfigData);
static void DisplayHelpScreen(struct Conf *confitem);

static int ChangedSomething = FALSE;


/*************************************************************************
 * DisplayCompileOptions() - do what name indicates
 *
 *
 *************************************************************************/

static void DisplayCompileOptions(void)
{
  const int settingcol = 60;
  int i,j, vattrib;
  int lin = 0, col = 0;
  static char *header = "Compile Time Options";
  static char *prompt = MTXT_DONE;

  static struct compile_options CompileOptions[] = 
  {
    {"HAS_SETITIMER", "can do fastupdates (2 per sec)", CTYPE_BOOL, 
#ifdef HAS_SETITIMER
     (void *)TRUE
#else
     (void *)FALSE
#endif /* HAS_SETITIMER */
    },

    {"USE_COMMONMLOCK", "can lock common block in memory", CTYPE_BOOL, 
#ifdef USE_COMMONMLOCK
     (void *)TRUE
#else
     (void *)FALSE
#endif /* USE_COMMONMLOCK */
    },

    {"SET_PRIORITY", "can raise process priority", CTYPE_BOOL, 
#ifdef SET_PRIORITY
     (void *)TRUE
#else
     (void *)FALSE
#endif /* SET_PRIORITY */
    },

    {"WARP0CLOAK", "can't be seen when cloaked at warp 0", CTYPE_BOOL,
#ifdef WARP0CLOAK
     (void *)TRUE
#else
     (void *)FALSE
#endif /* WARP0CLOAK */
    },

    {"DO_EXPLODING_SHIPS", "ships explode when they die", CTYPE_BOOL,
#ifdef DO_EXPLODING_SHIPS
     (void *)TRUE
#else
     (void *)FALSE
#endif /* DO_EXPLODING_SHIPS */
    },

    {"ENABLE_MACROS", "can use macro keys", CTYPE_BOOL,
#ifdef ENABLE_MACROS
     (void *)TRUE
#else
     (void *)FALSE
#endif /* ENABLE_MACROS */
    },
				/* last (NULL) entry */
    {NULL, NULL, CTYPE_NULL, (void *)NULL}
  };

  lin = 1;
  col = ((int)(cdcols() - strlen(header)) / 2);

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
	  j = (int) CompileOptions[i].value;

	  if (j == TRUE)
	    vattrib = GreenLevelColor;
	  else
	    vattrib = RedLevelColor;

	  cprintf(lin, settingcol, ALIGN_NONE, "#%d#%s#%d#",
		  vattrib, (j == TRUE) ? "True" : "False",
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
  static char *header = "System Options Menu";
  static char *mopts[] = { 
    "View compile-time Options",
    "View/Edit System-wide Options"
  };
  const int numoptions = 2;	/* don't exceed 9 - one char input is used */
  
  static char *prompt = "Enter a number to select an item, any other key to quit.";
  int lin = 0, col = 0;

  int i;
  int ch;
  int Done = FALSE;


  while (Done == FALSE)
    {
				/* First clear the display. */
      cdclear();
      lin = 1;
      col = ((int)(cdcols() - strlen(header))/ 2);
      
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
	  ChangedSomething = FALSE;
	  ViewEditOptions(SysConfData, SysCfEnd, TRUE);
	  if (ChangedSomething == TRUE)
	    SaveSysConfig();
	  break;
	default:
	  Done = TRUE;
	  break;
	}
    }

  return;
}

/*************************************************************************
 * UserOptsMenu() - display user options menu
 *
 *
 *************************************************************************/

void UserOptsMenu(int unum)
{
  static char *header = "User Options Menu";
  static char *mopts[] = { 
    "View/Edit Options",
    "View/Edit Macros",
    "Change Password"
  };
  const int numoptions = 3;	/* don't exceed 9 - one char input is used */
  static char *prompt = "Enter a number to select an item, any other key to quit.";
  int lin = 0, col = 0;
  int i;
  int ch;
  int Done = FALSE;
  struct Conf *macroptr = NULL;	/* points to macro element of ConfData */

  for (i=0; i < CfEnd; i++)
    {
      if (ConfData[i].ConfType == CTYPE_MACRO)
	macroptr = &ConfData[i];
    }

  if (macroptr == NULL)
    {				/* if this happens, something is
				   seriously confused */
      clog("UserOptsMenu(): ERROR: macroptr == NULL, no CTYPE_MACRO found in ConfData");
    }

  while (Done == FALSE)
    {
				/* First clear the display. */
      cdclear();
      lin = 1;
      col = ((cdcols() / 2) - (strlen(header) / 2));
      
      cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, header);
      
      lin += 3;
      col = 5;
      
      for (i = 0; i < numoptions; i++)
	{
	  cprintf(lin, col, ALIGN_NONE, "#%d#%d.#%d# %s#%d#", InfoColor, 
		  i + 1, LabelColor, mopts[i], NoColor);
	  lin++;
	}

      lin++;
      lin++;
      cprintf(lin, col, ALIGN_NONE, "#%d#Flags:#%d# %s#%d#", LabelColor,
              InfoColor, clntServerFlagsStr(&sStat),
              NoColor);
      
      cdclrl( MSG_LIN1, 2  );
      cdputs(prompt, MSG_LIN1, 1);
      
      /* Get a char */
      ch = iogchar();
      
      switch(ch)
	{
	case '1':			/* user opts */
	  ChangedSomething = FALSE; 
	  ViewEditOptions(ConfData, CfEnd, TRUE);
				/* save and reload the config */
	  if (ChangedSomething == TRUE)
	    {
	      SaveUserConfig(unum);
	      /* set new update rate */
	      Context.updsec = UserConf.UpdatesPerSecond;
	      sendCommand(CPCMD_SETRATE, Context.updsec);
	    }
	  break;

	case '2':		/* macros */
	  if (macroptr != NULL)
	    {
	      ChangedSomething = FALSE; 
	      ViewEditMacros(macroptr);
	      if (ChangedSomething == TRUE)
		SaveUserConfig(unum);
	    }

	  break;

	case '3':			/* chg passwd */
	  ChangePassword(unum, FALSE);
	  break;

	default:
	  Done = TRUE;
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
#define CBUFLEN 128
  int j, rv;
  char buf[CBUFLEN];

  switch(cdata->ConfType)
    {
    case CTYPE_BOOL:
      j = *(int *)(cdata->ConfValue);
      
      if (j == TRUE)
	j = FALSE;
      else
	j = TRUE;

      *(int *)(cdata->ConfValue) = j;

				/* signal that something has been changed */
      ChangedSomething = TRUE;

      break;

				/* these will need prompting on line 'lin' */
    case CTYPE_STRING:
      if (cdata->max > CBUFLEN)
	{
	  clog("ChangeOption: conf data max exceeds local buffer size.");
	  break;
	}
      cdclrl(lin, 1);
      strncpy(buf, ((char *)cdata->ConfValue), CBUFLEN);
      buf[CBUFLEN - 1] = 0;
      rv = cdgets("Value: ", lin, 1, buf, cdata->max - 1);

      if (rv != ERR)
	{
	  strncpy((char *)cdata->ConfValue, buf, cdata->max);
	  ((char *)cdata->ConfValue)[cdata->max - 1] = 0;
				/* signal that something has been changed */
	  ChangedSomething = TRUE;
	}

      break;
      
    case CTYPE_NUMERIC:
      cdclrl(lin, 1);
      rv = cdgetn("Enter a number: ", lin, 1, &j);

      if (rv != ERR)
	{
	  if (j >= cdata->min && j <= cdata->max)
	    {
	      *(int *)(cdata->ConfValue) = j;
	      /* signal that something has been changed */
	      ChangedSomething = TRUE;
	    }
	}

      break;
    }

  return;
}

/*************************************************************************
 * ChangeMacro() - change a macro... duh...
 *
 *
 *************************************************************************/

static void ChangeMacro(int macronum)
{
  int lin;
  static char *prmpt2 = "Enter Conquest commands.  ([RETURN] = \\r, [TAB] = \\t)";
  char buf[MAX_MACRO_LEN + 1];
  char prmpt[BUFFER_SIZE];
  int ch;
  lin = MSG_LIN1;
  
  

  cdclrl(lin, 2);
  cdputs(prmpt2, MSG_LIN2, 1);
  sprintf(prmpt, "f%2d = ", macronum);

  strcpy(buf, Macro2Str(UserConf.MacrosF[macronum - 1]));
  ch = cdgetx(prmpt, lin, 1, TERMS, buf, MAX_MACRO_LEN - 1, TRUE);

  if (ch == TERM_ABORT)
    {
      return;			/* if abort, cancel */
    }
      
  strncpy(UserConf.MacrosF[macronum - 1], Str2Macro(buf), MAX_MACRO_LEN);
  UserConf.MacrosF[macronum - 1][MAX_MACRO_LEN - 1] = EOS;
  
  ChangedSomething = TRUE;

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
  static char *uheader = "User Configurable Options";
  static char *sheader = "System-wide Configurable Options";
  static char *header;
  static char *eprompt = "Arrow keys to select an item, [SPACE] to change, any other key to quit.";
  static char *vprompt = MTXT_DONE;
  static char *eprompt2 = "Type '?' for help on an item.";
  int Done = FALSE;
  int ch;
  int lin = 0, col = 0, flin, llin, clin, maxllin;
  const int settingcol = 65;
  int vattrib;
#define MAXOPTCOLS 128
  int cvec[MAXOPTCOLS];		/* hopefully big enough */
  int usingSysOpts;

  usingSysOpts = ((ConfigData == SysConfData) ? TRUE : FALSE);

  if (usingSysOpts)
    header = sheader;
  else
    header = uheader;

  cdclear();			/* First clear the display. */

  flin = 4;			/* first option line */
  maxllin = MSG_LIN1 - 2;	/* max last option line */
  llin = 0;			/* last option line */
  clin = 0;			/* current option line (also idx into cvec) */

				/* clear out cvec */

  for (i=0; i < MAXOPTCOLS; i++)
    cvec[i] = -1;

  while (Done == FALSE)
    {
      cdclrl( 1, MSG_LIN2);	/* clear screen area */
      lin = 1;
      col = ((int)(cdcols() - strlen(header))/ 2);

      cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, header);
      
      lin = flin;
      col = 1;
      
      i = 0;			/* start at index 0 */
      k = 0;			/* init cvec */

      while (i < ConfSize)
	{
#ifdef DEBUG_OPTIONS
	  clog("ViewEditOptions(): ConfSize = %d, i = %d, type = %d, %s", 
	       ConfSize, i, ConfigData[i].ConfType,  
	       ConfigData[i].OneLineDesc);	  
#endif
	  if (ConfigData[i].ConfType != CTYPE_STRING && 
	      ConfigData[i].ConfType != CTYPE_BOOL &&
	      ConfigData[i].ConfType != CTYPE_NUMERIC)
	    {		/* skip special elements */
#ifdef DEBUG_OPTIONS
	      clog("ViewEditOptions():\tSKIPPING");
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
	      
	      if (j == TRUE)
		vattrib = GreenLevelColor;
	      else
		vattrib = RedLevelColor;
	      
	      cprintf(lin, settingcol, ALIGN_NONE, "#%d#%s#%d#",
		      vattrib, (j == TRUE) ? "True" : "False",
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
      llin = k - 1;

#ifdef DEBUG_OPTIONS
clog("ViewEditOptions(): maxllin = %d, llin = %d, k = %d", maxllin, llin, k);
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
	  Done = TRUE;
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
	  Done = TRUE;
	  break;
	}

#ifdef DEBUG_OPTIONS
      clog("ViewEditOptions():###\tllin = %d, clin = %d", llin, clin);
#endif
    }

  return TRUE;
}

/*************************************************************************
 * ViewEditMacros() - view and edit your macros ;-)
 *
 *
 *************************************************************************/

static int ViewEditMacros(struct Conf *ConfigData)
{
  int i, k;
  static char *header = "View/Edit Macros";
  static char *header2fmt = "(Page %d of %d)";
  static char headerbuf[BUFFER_SIZE];
  static char header2buf[BUFFER_SIZE];
  static char *nodef = "<Not Defined>";
  static char *eprompt = "Arrow keys to select a macro, [SPACE] to change, any other key to quit.";
  static char *eprompt2 = "Type '?' for help.";
  int Done = FALSE;
  int ch, vattrib;
  char *dispmac;
  int lin = 0, col = 0, flin, llin, clin, pages, curpage;
  const int macros_per_page = 18;
  char *macrovec[MAX_MACROS];

				/* this is the number of required pages,
				   though page accesses start at 0 */
  if (MAX_MACROS >= macros_per_page)
    {
      pages = MAX_MACROS / macros_per_page;
      if ((MAX_MACROS % macros_per_page) != 0)
	pages++;		/* for runoff */
    }
  else
    pages = 1;


				/* init the macrovec array */
  for (i=0; i < MAX_MACROS; i++)
  {
    macrovec[i] = (char *)(((char *)ConfigData->ConfValue)
                                        + (i * MAX_MACRO_LEN));
  }

  curpage = 0;

  cdclear();			/* First clear the display. */

  flin = 4;			/* first macro line */
  llin = 0;			/* last macro line on this page */
  clin = 0;			/* current macro line */


  while (Done == FALSE)
    {
      sprintf(header2buf, header2fmt, curpage + 1, pages);
      sprintf(headerbuf, "%s %s", header, header2buf);

      cdclrl( 1, MSG_LIN2);	/* clear screen area */
      lin = 1;
      col = ((int)(cdcols() - strlen(headerbuf)) / 2);

      cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, headerbuf);
      
      lin = flin;
      col = 1;
      
      i = 0;			/* start at index 0 */

				/* figure out the last editable line on
				   this page */

      if (curpage == (pages - 1)) /* last page - might be less than full */
	llin = (MAX_MACROS % macros_per_page);	/* ..or more than empty? ;-) */
      else
	llin = macros_per_page;

      i = 0;
      while (i < llin)
	{			/* display this page */
				/* get the macro number for this line */
	  k = (curpage * macros_per_page) + i; 

	  if (macrovec[k][0] == EOS)
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
	  clog("ViewEditMacros(): k = %d, dispmac = '%s'", 
	       k, 
	       dispmac);
#endif

	  cprintf(lin, col, ALIGN_NONE, "#%d#f%2d = #%d#%s#%d#",
		  InfoColor, k + 1, vattrib, 
		  dispmac,
		  NoColor);

	  lin++;
	  i++;
	}

				/* now the editing phase */
      cdclrl( MSG_LIN1, 2  );
      cdputs(eprompt, MSG_LIN1, 1);
      cdputs(eprompt2, MSG_LIN2, 1);

      if (clin >= llin)
	clin = llin - 1;

      cdmove(flin + clin, 1); 
      
      /* Get a char */
      ch = iogchar();
      

      switch(ch)
	{
	case KEY_UP:		/* up */
	case KEY_LEFT:
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

				/* setup llin  for current page */
	      if (curpage == (pages - 1)) 
		llin = (MAX_MACROS % macros_per_page);
	      else
		llin = macros_per_page;

	      clin = llin - 1; 
	    }
	  break;

	case KEY_DOWN:		/* down */
	case KEY_RIGHT:
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

	case KEY_PPAGE:		/* prev page */
	  if (pages != 1)
	    {
	      curpage--;
	      if (curpage < 0)
		{
		  curpage = pages - 1;
		}
	    }

	  break;

	case KEY_NPAGE:		/* next page */
	  if (pages != 1)
	    {
	      curpage++;
	      if (curpage >= pages)
		{
		  curpage = 0;
		}
	    }

	  break;

	case ' ':	/* change something */
	  ChangeMacro((curpage * macros_per_page) + clin + 1);
	  break;

	case '?':
	  DisplayHelpScreen(ConfigData); /* macro */
	  break;

	default:		/* everything else */
	  Done = TRUE;
	  break;
	}

#ifdef DEBUG_OPTIONS
      clog("ViewEditMacros():###\tllin = %d, clin = %d, curpage = %d", 
	   llin, clin, curpage);
#endif
    }

  return TRUE;
}

/* DisplayHelpScreen() - display a help (actually the conf item comment)
 *                       screen for a Configuration item.
 */
void DisplayHelpScreen(struct Conf *confitem)
{
  int i, col, lin;

  cdclear();

  lin = 1;
  col = 0;

  cprintf(lin, col, ALIGN_CENTER, "#%d#%s#%d#", 
	  NoColor, confitem->OneLineDesc, NoColor);

  lin += 2;

  i = 0;
  attrset(InfoColor);

  while (confitem->ConfComment[i] != NULL)
    {
      if (confitem->ConfComment[i][0] != EOS)
	{
	  cdputs(&(confitem->ConfComment[i][1]), lin, col);
	  lin++;
	}
      i++;
    }

  attrset(NoColor);
  cdclrl( MSG_LIN1, 2  );
  cdputc(MTXT_DONE, MSG_LIN2);

  /* Get a char */
  (void)iogchar();

  return;
}
