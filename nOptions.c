/* 
 * node
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"
#include "datatypes.h"
#include "color.h"
#include "conf.h"
#include "conqcom.h"
#include "gldisplay.h"
#include "glmisc.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "prm.h"
#include "nCP.h"
#include "nMenu.h"
#include "nDead.h"
#include "nOptions.h"
#include "cqkeys.h"

/* from conquestgl */
extern Unsgn8 clientFlags;
extern int lastServerError;

extern void processPacket(char *);

static int retnode;             /* node to return to */
static int uopts = FALSE;       /* are we doing user opts? */

#define S_NONE          0
#define S_USRMENU       1
#define S_SYSMENU       2
#define S_UOPTS         3       /* changing user opts */
#define S_MACROS        4       /* changing macros */
#define S_SOPTS         5       /* changing system opts */
#define S_GETPW         6       /* changing password */
#define S_GETRPW        7
#define S_HELP          8
static int state;

static prm_t prm;
static int prompting;

static char cbuf[BUFFER_SIZE];

static char *pwp = "Password: ";
static char *rpwp = "Retype Password: ";
static char *phelper = "Use any printable characters.";
static char *nodef = "<Not Defined>";

static char *mheader = "View/Edit Macros";
static char *uheader = "User Configurable Options";
static char *sheader = "System-wide Configurable Options";

static char *meprompt = "Enter Conquest commands.  ([RETURN] = \\r, [TAB] = \\t)";
static char *eprompt = "Arrow keys to select an item, [SPACE] to change, any other key to quit.";
static char *eprompt2 = "Type '?' for help on an item.";
static char *mprompt = "Arrow keys to select a macro, [SPACE] to change, any other key to quit.";
static char *mprompt2 = "Type '?' for help.";

static const int settingcol = 65;

static char cursor = ' ';       /* the cursor */

static const int umenuopts = 3; /* don't exceed 9 - one char input is used */

static int flin, llin, clin, pages, curpage;
static const int items_per_page = 18;

#define MAXOPTCOLS 128
static int cvec[MAXOPTCOLS];         /* hopefully big enough */
static int maxuopts;
static int maxsopts = 0;        /* not impl yet */
static char *macrovec[MAX_MACROS];

static struct Conf *macroptr = NULL; /* points to macro element of ConfData */
static struct Conf *ConfigData = NULL;

static char *oheader = "";

static int oldstate;            /* for returning from help */

static int nOptionsDisplay(dspConfig_t *);
static int nOptionsIdle(void);
static int nOptionsInput(int ch);

static scrNode_t nOptionsNode = {
  nOptionsDisplay,               /* display */
  nOptionsIdle,                  /* idle */
  nOptionsInput                  /* input */
};

/* return to the correct node */
static void quitNode(void)
{
  SaveUserConfig();
  Context.updsec = UserConf.UpdatesPerSecond;
  sendCommand(CPCMD_SETRATE, Context.updsec);

  switch (retnode)
    {
    case DSP_NODE_CP:
      nCPInit();
      break;
    case DSP_NODE_MENU:
    default:
      nMenuInit();
      break;
    }
  return;
}

static void _changePasswd(int init)
{
  static char pw[MAXUSERNAME], rpw[MAXUSERNAME];

  clog("CHPW: init = %d, state = %d ", init, state);

  if (init)
    {                           /* start prompting */
      prm.pbuf = pwp;
      pw[0] = EOS;
      prm.buf = pw;

      prm.preinit = FALSE;
      prm.buflen = MAXUSERNAME - 1;
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prompting = TRUE;
    }
  else
    {
      if (state == S_GETPW)
        {                       /* got the pw, set up for the retry */
          state = S_GETRPW;
          prm.pbuf = rpwp;
          rpw[0] = EOS;
          prm.buf = rpw;

          prm.preinit = FALSE;
          prm.buflen = MAXUSERNAME - 1;
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prompting = TRUE;
        }
      else
        {                       /* done. do a compare and send new pw
                                   if they match */
          clog("CHPW: init = %dGETRPW?", init);
  
         if (strcmp(pw, rpw) != 0)
            {                       /* pw's don't match */
              mglBeep();
              state = S_USRMENU;
           clog("CHPW: init = %d, mismatch", init);
              return;
            }
          else
            sendAuth(cInfo.sock, CPAUTH_CHGPWD, "", pw);

          prompting = FALSE;
          state = S_USRMENU;
          clog("CHPW: init = %d, changed", init);

        }
      
    }

  return;
}

static void _changeMacro(int macronum, int init)
{
  int lin;
  static char prmpt[BUFFER_SIZE];
  lin = MSG_LIN1;
  
  if (init)
    {                           /* start prompting */
      sprintf(prmpt, "f%2d = ", macronum);
      strcpy(cbuf, Macro2Str(UserConf.MacrosF[macronum - 1]));

      prm.preinit = TRUE;
      prm.buf = cbuf;
      prm.buflen = MAX_MACRO_LEN - 1;
      prm.pbuf = prmpt;
      prm.terms = TERMS;
      prm.index = lin;
      prompting = TRUE;
      
    }
  else
    {                           /* change it for real */
      strncpy(UserConf.MacrosF[macronum - 1], Str2Macro(prm.buf), 
              MAX_MACRO_LEN);
      UserConf.MacrosF[macronum - 1][MAX_MACRO_LEN - 1] = EOS;
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
      
      if (j == TRUE)
	j = FALSE;
      else
	j = TRUE;

      *(int *)(cdata->ConfValue) = j;

      break;

				/* these will need prompting */
    case CTYPE_STRING:
      if (cdata->max > CBUFLEN)
	{
	  clog("_changeOption: conf data max exceeds local buffer size.");
	  break;
	}

      if (init)
        {
          prm.preinit = TRUE;
          strncpy(cbuf, ((char *)cdata->ConfValue), CBUFLEN);
          cbuf[CBUFLEN - 1] = EOS;
          prm.buf = cbuf;
          prm.buflen = cdata->max - 1;
          prm.pbuf = "Value: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prompting = TRUE;
        }
      else
        {
          strncpy((char *)cdata->ConfValue, prm.buf, cdata->max);
          ((char *)cdata->ConfValue)[cdata->max - 1] = EOS;
        }

      break;

    case CTYPE_NUMERIC:
      if (init)
        {
          prm.preinit = TRUE;
          sprintf(cbuf, "%d", *(int *)(cdata->ConfValue));
          prm.buf = cbuf;
          prm.buflen = 20;
          prm.pbuf = "Enter a number: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prompting = TRUE;
        }
      else
        {
          if (alldig(prm.buf))
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
  static char *header = "User Options Menu";
  static char *mopts[] = { 
    "View/Edit Options",
    "View/Edit Macros",
    "Change Password"
  };
  static char *prompt = "Enter a number to select an item, any other key to quit.";
  int lin = 0, col = 0;
  int i;

  if (macroptr == NULL)
    {				/* if this happens, something is
				   seriously confused */
      clog("_dispUserOptsMenu(): ERROR: macroptr == NULL, no CTYPE_MACRO found in ConfData");
    }

				/* First clear the display. */
      lin = 1;
      cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", NoColor, header);
      
      lin += 3;
      col = 5;
      
      for (i = 0; i < umenuopts; i++)
	{
	  cprintf(lin, col, ALIGN_NONE, "#%d#%d.#%d# %s#%d#", InfoColor, 
		  i + 1, LabelColor, mopts[i], NoColor);
	  lin++;
	}

      lin = 20;
      cprintf(lin, col, ALIGN_NONE, "#%d#Flags:#%d# %s#%d#", LabelColor,
              InfoColor, clntServerFlagsStr(sStat.flags),
              NoColor);
      
      if (!prompting)
        {
          clrPrompt(MSG_LIN1);
          clrPrompt(MSG_LIN2);
          cprintf(MSG_LIN2, 0, ALIGN_NONE, prompt);
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
    confitem = &ConfigData[cvec[clin]];
  else
    confitem = macroptr;

  lin = 1;
  col = 0;

  cprintf(lin, col, ALIGN_CENTER, "#%d#%s#%d#", 
	  NoColor, confitem->OneLineDesc, NoColor);

  lin += 2;

  i = 0;

  while (confitem->ConfComment[i] != NULL)
    {
      if (confitem->ConfComment[i][0] != EOS)
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
  static char *header2fmt = "(Page %d of %d)";
  static char headerbuf[BUFFER_SIZE];
  static char header2buf[BUFFER_SIZE];
  static char *header;
  static char *prompt;
  static char *prompt2;
  int vattrib;
  char *dispmac;
  int lin = 0, col = 0;

  switch (state)
    {
    case S_UOPTS:
      header = uheader;
      prompt = eprompt;
      prompt2 = eprompt2;
      if (curpage == (pages - 1)) /* last page - might be less than full */
        llin = (maxuopts % items_per_page); /* ..or more than empty? ;-) */
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

    default:                    /* shouldn't get here */
      header = "";
      break;
    }

  sprintf(header2buf, header2fmt, curpage + 1, pages);
  sprintf(headerbuf, "%s %s", header, header2buf);
  
  lin = 1;
  cprintf(lin, 0, ALIGN_CENTER, "#%d#%s", NoColor, headerbuf);
  
  lin = flin;
  col = 1;
  
  i = 0;			/* start at index 0 */
  
  while (i < llin)
    {			/* display this page */

      if (state == S_MACROS)
        {
          /* get the macro number for this line */
          k = (curpage * items_per_page) + i; 
          
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
          clog("_showOptScreen(): k = %d, dispmac = '%s'", 
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
      cprintf(MSG_LIN1, 1, ALIGN_NONE, prompt);
      cprintf(MSG_LIN2, 1, ALIGN_NONE, prompt2);
    }
  
  return;
}
  

void nOptionsInit(int what, int rnode)
{
  int i, k;
  struct Conf *ConfigData;

  if (dConf.viewerwmapped)
    {
      /* unmap the viewer */
      glutSetWindow(dConf.viewerw);
      glutHideWindow();
      dConf.viewerwmapped = FALSE;

      glutSetWindow(dConf.mainw);
    }

  uopts = (what == NOPT_USER) ? TRUE : FALSE;
  prompting = FALSE;

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
              break;
            }
        }
      
      if (macroptr)
        for (i=0; i < MAX_MACROS; i++)
          {
            macrovec[i] = (char *)(((char *)macroptr->ConfValue)
                                   + (i * MAX_MACRO_LEN));
          }
    }
  
  setNode(&nOptionsNode);

  return;
}


static int nOptionsDisplay(dspConfig_t *dsp)
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

  if ((state == S_UOPTS) || (state == S_MACROS))
    {
      _showOptScreen();

      if (prompting)
        {
          cprintf(prm.index, 0, ALIGN_NONE, "#%d#%s #%d#%s%c",
                  InfoColor, prm.pbuf, MagentaColor, prm.buf, cursor);

          if (state == S_MACROS)
            cprintf(MSG_LIN2, 0, ALIGN_NONE, "#%d#%s",
                    NoColor, meprompt);
        }

    }

  if (state == S_HELP)
    _dispHelpScreen();

  return NODE_OK;
}  

static int nOptionsIdle(void)
{
  int gtime = glutGet(GLUT_ELAPSED_TIME);
  static int old = 0;
  int pkttype;
  Unsgn8 buf[PKT_MAXSIZE];

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
    return NODE_OK;             /* don't process packets is sys modes */

  while ((pkttype = waitForPacket(PKT_FROMSERVER, cInfo.sock, PKT_ANYPKT,
                                  buf, PKT_MAXSIZE, 0, NULL)) > 0)
    processPacket(buf);

  if (pkttype < 0)          /* some error */
    {
      clog("nOptionsIdle: waiForPacket returned %d", pkttype);
      Ships[Context.snum].status = SS_OFF;
      return NODE_EXIT;
    }

  if ((retnode == DSP_NODE_CP) && (clientFlags & SPCLNTSTAT_FLAG_KILLED))
    {
      /* if we were flying, time to die properly. */
      nDeadInit();
      return NODE_OK;
    }

  return NODE_OK;
}

static int nOptionsInput(int ch)
{
  int irv;

  if (prompting)
    {
      irv = prmProcInput(&prm, ch);

      switch (state)
        {
        case S_MACROS:
          if (irv > 0)
            {
              if (irv != TERM_ABORT)
                _changeMacro((curpage * items_per_page) + clin + 1, FALSE);

              prompting = FALSE;
            }
          break;

        case S_UOPTS:
          if (irv > 0)
            {
              if (irv != TERM_ABORT)
                _changeOption(&ConfigData[cvec[clin]], FALSE);

              prompting = FALSE;
            }
          break;

        case S_GETRPW:
        case S_GETPW:
          if (irv > 0)
            {
              if (irv != TERM_ABORT)
                {
                  _changePasswd(FALSE);
                }
              else
                {
                  prompting = FALSE;
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
          state = S_GETPW;
          _changePasswd(TRUE);
          break;

        default:
          quitNode();
          break;
        }


      return NODE_OK;
    }

  if (state == S_UOPTS || state == S_MACROS)
    {
      int maxitms = (state == S_UOPTS) ? maxuopts : MAX_MACROS;

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
		llin = (maxitms % items_per_page);
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
            _changeMacro((curpage * items_per_page) + clin + 1, TRUE);
          else if (state == S_UOPTS)
            _changeOption(&ConfigData[cvec[clin]], TRUE);
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

