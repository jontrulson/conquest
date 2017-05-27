/* 
 * auth node
 *
 * can NODE_EXIT
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
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

static char *unamep = "Username:";
static char *pwp = "Password:";
static char *rpwp = "Retype Password:";
static char *newuserp = "User doesn't exist. Is this a new user?";
static char *pwerr = "Invalid Password.";
static char *uhelper = "You can use A-Z, a-z, 0-9, '_', or '-'.";
static char *phelper = "Use any printable characters.";

static char cursor = ' ';       /* the cursor */

/* the current prompt */
static prm_t prm;

#define YN_BUFLEN  10
static char ynbuf[YN_BUFLEN];

static char nm[MAXUSERNAME], pw[MAXUSERNAME], pwr[MAXUSERNAME];

static char *statlin = NULL;
static char *errlin = NULL;
static int  newuser = FALSE;    /* a new user? */

/* the current state */
#define S_DONE         0
#define S_GETUNAME     1
#define S_GETPW        2
#define S_GETRPW       3
#define S_CONFIRMNEW   4

static int state;

static int nAuthDisplay(dspConfig_t *dsp);
static int nAuthInput(int ch);
static int nAuthIdle(void);

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
  static char buf1[BUFFER_SIZE];
  static char buf2[BUFFER_SIZE];
  static char buf3[BUFFER_SIZE];
  static char buf4[BUFFER_SIZE];
  static char buf5[BUFFER_SIZE];
  static char buf6[BUFFER_SIZE];
  static char pbuf1[BUFFER_SIZE];
  static char pbuf2[BUFFER_SIZE];
  static char pbuf3[BUFFER_SIZE];
  static char pbuf4[BUFFER_SIZE];
  static char pbuf5[BUFFER_SIZE];
  static char pbuf6[BUFFER_SIZE];
  static int inited = FALSE;
  static const int hcol = 1, icol = 11;
  static char timebuf[BUFFER_SIZE];
  time_t servtm;

  if (!inited)
    {
      inited = TRUE;
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
      sprintf(buf4, "#%d#%%s", NoColor);

      sprintf(pbuf5, "#%d#MOTD: ", MagentaColor);
      sprintf(buf5, "#%d#%%s", NoColor);

      servtm = sStat.servertime; /* fix alignment */
      strncpy(timebuf, ctime(&servtm), BUFFER_SIZE - 1);
      timebuf[strlen(timebuf) - 1] = 0; /* remove the NL */

    }

  cprintf(tlin, hcol, ALIGN_NONE, pbuf1);
  cprintf(tlin++, icol, ALIGN_NONE, buf1, sHello.servername);

  cprintf(tlin, hcol, ALIGN_NONE, pbuf2);
  cprintf(tlin++, icol, ALIGN_NONE, buf2, sHello.serverver);

  cprintf(tlin, hcol, ALIGN_NONE, pbuf6);
  cprintf(tlin++, icol, ALIGN_NONE, buf6, timebuf);

  cprintf(tlin, hcol, ALIGN_NONE, pbuf3);
  cprintf(tlin++, icol, ALIGN_NONE, buf3,
          sStat.numusers, sStat.numtotal, MAXSHIPS, sStat.numactive,
          sStat.numvacant, sStat.numrobot);

  cprintf(tlin, hcol, ALIGN_NONE, pbuf4);
  cprintf(tlin++, icol, ALIGN_NONE, buf4, 
          clntServerFlagsStr(sStat.flags));

  cprintf(tlin, hcol, ALIGN_NONE, pbuf5);
  cprintf(tlin++, icol, ALIGN_NONE, buf5, sHello.motd);

  return;
}


void nAuthInit(void)
{
  ynbuf[0] = pw[0] = pwr[0] = nm[0] = 0;

  state = S_GETUNAME;           /* initial state */
  prm.preinit = FALSE;
  prm.buf = nm;
  prm.buflen = MAX_USERLEN;
  prm.pbuf = unamep;
  prm.terms = TERMS;

  statlin = uhelper;

  setNode(&nAuthNode);

  return;
}

/* all we do here is 'blink' the cursor ;-) */
static int nAuthIdle(void)
{
  cursor = (BLINK_QTRSEC) ? '_' : ' ';

  return NODE_OK;
}
      
      

static int nAuthDisplay(dspConfig_t *dsp)
{
  int lin;
  int statline;
  int slin;
  int tmpcolor;
  extern char *ConquestVersion;
  extern char *ConquestDate;

  /* display the logo */
  mglConqLogo(dsp, FALSE);

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

  cprintf( slin - 2, 1, ALIGN_LEFT,
           "#%d#Please login. Press [ENTER] to exit.",
           SpecialColor);
  
  cprintf( slin - 1, 1, ALIGN_LEFT,
           "#%d#(New Users: Just pick a username)",
           SpecialColor);

  cprintf(slin, 1, ALIGN_LEFT, "#%d#%s",
          CyanColor,
          unamep);
  cprintf(slin, 11, ALIGN_LEFT, "#%d#%s%c",
          tmpcolor,
          nm, (state == S_GETUNAME) ? cursor: ' ');

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
            ynbuf,
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
    
  return NODE_OK;
}
  
  
static int nAuthInput(int ch)
{
  int rv, irv;

  ch = CQ_CHAR(ch);
  irv = prmProcInput(&prm, ch);  
  switch (state)
    {
    case S_GETUNAME:               
      if (irv > 0)
        {                       /* a terminator */
          /* we wish to leave. chicken... */
          if (irv == TERM_ABORT || prm.buf[0] == EOS)
            return NODE_EXIT;

          /* check validity */
          if (checkuname(prm.buf) == FALSE)
            {                   /* invalid username */
              mglBeep(MGL_BEEP_ERR);
              errlin = "Invalid character in username.";
              prm.buf[0] = EOS;
              return NODE_OK;
            }

          /* check if new user */
          if ((rv = sendAuth(cInfo.sock, CPAUTH_CHECKUSER, 
                             prm.buf, "")) < 0)
            return NODE_EXIT;       /* pkt error */

          if (rv == PERR_NOUSER)
            {                   /* new */
              /* for a new user, we will want confirmation */
              state = S_CONFIRMNEW;

              ynbuf[0] = EOS;

              prm.preinit = FALSE;
              prm.buf = ynbuf;
              prm.buflen = MAXUSERNAME;
              prm.pbuf = newuserp;
              prm.terms = TERMS;

              errlin = NULL;
              return NODE_OK;
            }
          else
            {                   /* old user */
              state = S_GETPW;

              /* setup for the new prompt */
              prm.preinit = FALSE;
              prm.buf = pw;
              prm.buflen = MAX_USERLEN;
              prm.pbuf = pwp;
              prm.terms = TERMS;

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
          if (irv == TERM_NORMAL && (prm.buf[0] == 'y' || prm.buf[0] == 'Y'))
            {                   /* confirming new user */
              state = S_GETPW;

              /* setup for the new prompt */
              prm.preinit = FALSE;
              prm.buf = pw;
              prm.buflen = MAX_USERLEN;
              prm.pbuf = pwp;
              prm.terms = TERMS;

              statlin = phelper;
              errlin = NULL;
              newuser = TRUE;
            }
          else
            {                   /* oops */
              state = S_GETUNAME;  

              prm.preinit = TRUE;
              prm.buf = nm;
              prm.buflen = MAX_USERLEN;
              prm.pbuf = unamep;
              prm.terms = TERMS;

              statlin = uhelper;
              errlin = NULL;
              newuser = FALSE;
            }
        }

      return NODE_OK;
      break;                    /* S_CONFIRMNEW */

    case S_GETPW:
      if (irv > 0)
        {                       /* a terminator */
          /* we have a password */

          /* if this was a new user, go straight to S_GETRPW */
          if (newuser)
            {
              state = S_GETRPW;

              /* setup for the new prompt */
              prm.preinit = FALSE;
              prm.buf = pwr;
              prm.buflen = MAX_USERLEN;
              prm.pbuf = rpwp;
              prm.terms = TERMS;

              statlin = phelper;
              errlin = NULL;

              return NODE_OK;
            }

          if ((rv = sendAuth(cInfo.sock, CPAUTH_LOGIN,
                             nm, pw)) < 0)
            return NODE_EXIT;   /* error */

          if (rv != PERR_OK)
            {                   /* invalid pw */
              /* clear it out and return to  P_GETUNAME state) */
              pw[0] = EOS;
              state = S_GETUNAME;           /* initial state */

              prm.preinit = TRUE;
              prm.buf = nm;
              prm.buflen = MAX_USERLEN;
              prm.pbuf = unamep;
              prm.terms = TERMS;

              pw[0] = EOS;
              pwr[0] = EOS;

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

          if (strcmp(pw, pwr) != 0)
            {                   /* pw's don't match, start over */
              mglBeep(MGL_BEEP_ERR);

              errlin = "Passwords don't match.";
              /* go back to username prompt */
              state = S_GETUNAME;           /* initial state */
              prm.preinit = TRUE;
              prm.buf = nm;
              prm.buflen = MAX_USERLEN;
              prm.pbuf = unamep;
              prm.terms = TERMS;

              pw[0] = EOS;
              pwr[0] = EOS;

              statlin = uhelper;
            }
          else
            {                   /* they match, lets goto welcome node */

              if ((rv = sendAuth(cInfo.sock, CPAUTH_LOGIN,
                                 nm, pw)) < 0)
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

