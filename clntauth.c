/* 
 * clntauth.c - user stuff
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"
#include "color.h"
#include "ui.h"
#include "cd2lb.h"
#include "cumisc.h"

#include "conqnet.h"
#include "packet.h"
#include "userauth.h"
#include "client.h"
#include "clientlb.h"
#include "conqutil.h"

static void PrintStatus(int lin)
{
  static char buf1[MID_BUFFER_SIZE];
  static char buf2[MID_BUFFER_SIZE];
  static char buf3[MID_BUFFER_SIZE];
  static char buf4[MID_BUFFER_SIZE];
  static char buf5[MID_BUFFER_SIZE];
  static char buf6[MID_BUFFER_SIZE];
  static int inited = FALSE;
  time_t servtm;

  if (!inited)
    {
      inited = TRUE;
      sprintf(buf1, "#%d#Server: #%d#  %%s", MagentaColor, NoColor);
      sprintf(buf2, "#%d#Version: #%d# %%s", MagentaColor, NoColor);
      sprintf(buf3, "#%d#Time: #%d#    %%s", MagentaColor, NoColor);
      sprintf(buf4, 
              "#%d#Status: #%d#  Users #%d#%%d, #%d#Ships #%d#%%d/%%d #%d#"
              "(#%d#%%d #%d#active, #%d#%%d #%d#vacant, "
              "#%d#%%d #%d#robot)",
              MagentaColor, NoColor, CyanColor, NoColor, CyanColor, NoColor,
              CyanColor, NoColor, CyanColor, NoColor, 
              CyanColor, NoColor);
      sprintf(buf5, "#%d#Flags: #%d#   %%s", MagentaColor, NoColor);
      sprintf(buf6, "#%d#MOTD: #%d#    %%s", MagentaColor, NoColor);
    }

  lin++;

  cprintf(lin++, 0, ALIGN_NONE, buf1, sHello.servername);

  cprintf(lin++, 0, ALIGN_NONE, buf2, sHello.serverver);

  servtm = sStat.servertime; /* fix alignment */
  cprintf(lin++, 0, ALIGN_NONE, buf3, ctime(&servtm));

  cprintf(lin++, 0, ALIGN_NONE, buf4,
	  sStat.numusers, sStat.numtotal, MAXSHIPS, sStat.numactive, 
	  sStat.numvacant, sStat.numrobot);

  cprintf(lin++, 0, ALIGN_NONE, buf5, clntServerFlagsStr(sStat.flags));

  cprintf(lin++, 0, ALIGN_NONE, buf6, sHello.motd);

  return;
}

int Logon(char *username)
{
  int lin, slin;
  int ch, statline;
  char nm[MAXUSERNAME], pw[MAXUSERNAME], pwr[MAXUSERNAME];
  int done, rv;
  extern char *ConquestVersion;
  extern char *ConquestDate;

  done = FALSE;

  while (!done)
    {
      cdclear();

      /* Display the logo. */

      lin = mcuConqLogo();

      if ( sHello.flags & SPHELLO_FLAGS_CLOSED )
	{
	  cprintf( lin, 0, ALIGN_CENTER, "#%d#%s", RedLevelColor,
		   "The game is closed.");
	  cdrefresh();
	}
      else
	cprintf( lin, 1, ALIGN_CENTER, "#%d#Welcome to #%d#Conquest#%d# %s (%s)", 
		 YellowLevelColor,
		 RedLevelColor,
		 YellowLevelColor,
		 ConquestVersion, ConquestDate);

      lin++;
      statline = lin;

      lin += 7;
      slin = lin;

      nm[0] = EOS;

      cdrefresh();

      while (TRUE)			/* login loop */
	{
	  PrintStatus(statline);
	  slin = lin;
	  cdclrl( slin, Context.maxlin - slin - 1 );
	  slin += 3;
	  cdputs("You can use A-Z, a-z, 0-9, '_', or '-'.", MSG_LIN1, 1);
	  cprintf( slin - 2, 1, ALIGN_LEFT,
		   "#%dPlease login. Press [ENTER] to exit.",
		   SpecialColor);
	  
	  cprintf( slin - 1, 1, ALIGN_LEFT,
		   "#%d(New Users: Just pick a username)",
		   SpecialColor);
	  
	  ch = cdgetx( "Username: ", slin, 1, TERMS, nm, MAX_USERLEN, TRUE );

	  if (ch == TERM_EXTRA )
	    break;		/* redraw stats and things */

	  if (ch == TERM_ABORT || nm[0] == EOS)
	    {			/* if empty user, or abort char, leave */
	      cdend();
	      sleep(1);
	      exit(2);
	    }

	  if (checkuname(nm) == FALSE)
	    {			/* invalid username */
	      cdbeep();
	      uiPutColor(RedLevelColor);
	      cdputs("Invalid character in username.", MSG_LIN2, 1);
	      uiPutColor(NoColor);
	      nm[0] = EOS;
	      continue;
	    }

	  if ((rv = sendAuth(cInfo.sock, CPAUTH_CHECKUSER, nm, "")) < 0)
	    return FALSE;

	  if (rv == PERR_NOUSER)
	    {			/* nope... */
	      slin++;
	      if (mcuAskYN("User doesn't exist. Is this a new user? ", slin, 1))
		{			/* yep */
		  pw[0] = EOS;
		  cdclrl( MSG_LIN1, 2  );
		  cdputs("Use any printable characters.", MSG_LIN1, 1);
		  ch = cdgetx( "Password: ", slin, 1, 
			       TERMS, pw, MAXUSERNAME - 1, FALSE );
	      
		  slin++;
		  pwr[0] = EOS;
		  cdclrl( MSG_LIN1, 2  );
		  cdputs("Use any printable characters.", MSG_LIN1, 1);
		  ch = cdgetx( "Retype Password: ", slin, 1, 
			       TERMS, pwr, MAXUSERNAME - 1, FALSE );
	      
		  if (strcmp(pw, pwr) != 0)
		    {			/* pw's don't match, start over */
		      cdbeep();
		      cdclrl( MSG_LIN2, 1  );
		      uiPutColor(RedLevelColor);
		      cdputs("Passwords don't match.", MSG_LIN2, 1);
		      uiPutColor(NoColor);
		      cdrefresh();
		      sleep(2);
		      continue;
		    }
		  /* if we're here, we have a username
		     and password (new user) - time
		     to rock. */

		  if ((rv = sendAuth(cInfo.sock, CPAUTH_LOGIN, 
				     nm, pw)) < 0)
		    return FALSE;

		  if (rv != PERR_OK)
		    {
		      utLog("conquest: CPAUTH_LOGIN returned %d\n",
			   rv);
		      continue;
		    }

		  utLog("clntauth: INFO: new user '%s' logged in", nm);
		  done = TRUE;
		  break;
		}
	      else
		{
		  continue;		/* restart */
		}
	    }
	  else
	    {			/* exists, verify that the pw is valid */
	      slin++;
	      pw[0] = EOS;
	      cdclrl( MSG_LIN1, 2  );
	      cdputs("Use any printable characters.", MSG_LIN1, 1);
	      ch = cdgetx( "Password: ", slin, 1, 
			   TERMS, pw, MAXUSERNAME - 1, FALSE );

	      if ((rv = sendAuth(cInfo.sock, CPAUTH_LOGIN, 
				 nm, pw)) < 0)
		return FALSE;

	      

	      if (rv != PERR_OK)
		{			/* invalid pw */
		  cdbeep();
		  cdclrl( MSG_LIN2, 1  );
		  uiPutColor(RedLevelColor);
		  cdputs("Invalid Password.", MSG_LIN2, 1);
		  uiPutColor(NoColor);
		  cdrefresh();
		  utLog("INFO: Invalid password for user '%s'", nm);
		  sleep(2);
		  continue;
		}
	  
	      /* good pw - go for the gusto */
	      done = TRUE;
	      utLog("clntauth:INFO: user '%s' logged in", nm);
	      break;
	    }
	} /* end while */
    }
				/* if we're here, we're legal */
  strncpy(username, nm, MAXUSERNAME);

  return(TRUE);
}

/* ChangePassword() - change a users password - called from UserOptsMenu() */
void ChangePassword(int unum, int isoper)
{
  static char *header = "Change Password";
  char pw[MAXUSERNAME], pwr[MAXUSERNAME], epw[MAXUSERNAME];
  char salt[3];
  int ch;
  int lin = 0, col = 0;

  if (isoper == FALSE)
    cdclear();
  else
    {
      cdclrl(MSG_LIN1, 2);
    }

  if (isoper == FALSE)
    {
      lin = 1;
      col = ((Context.maxcol / 2) - (strlen(header) / 2));
      
      cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, header);
    } /* ! isoper */
				/* get and recheck new passwd */
  pw[0] = EOS;
  cdclrl( MSG_LIN1, 2  );
  cdputs("Use any printable characters.", MSG_LIN2, 1);

  ch = cdgetx( "New Password: ", MSG_LIN1, 1, 
	       TERMS, pw, MAXUSERNAME - 1, FALSE );

  if (isoper == FALSE)
    {
      pwr[0] = EOS;
      cdclrl( MSG_LIN1, 2  );
      cdputs("Use any printable characters.", MSG_LIN2, 1);
      ch = cdgetx( "Retype Password: ", MSG_LIN1, 1, 
		   TERMS, pwr, MAXUSERNAME - 1, FALSE );
      
      if (strcmp(pw, pwr) != 0)
	{			/* pw's don't match, start over */
	  cdbeep();
	  cdclrl( MSG_LIN2, 1  );
	  uiPutColor(RedLevelColor);
	  cdputs("Passwords don't match.", MSG_LIN2, 1);
	  uiPutColor(NoColor);
	  cdrefresh();
	  sleep(2);
	  return;
	}
    } /* ! isoper */
				/* ok, we have a new password -
				   make it so */

  if (isoper)
    {				/* do it locally */
      salt[0] = (Users[unum].username[0] != EOS) ? Users[unum].username[0] : 'J';
      salt[1] = (Users[unum].username[1] != EOS) ? Users[unum].username[1] : 'T';
      salt[2] = EOS;
      
      strncpy(epw, (char *)crypt(pw, salt), MAXUSERNAME - 2);
      epw[MAXUSERNAME - 1] = EOS;
      
      strncpy(Users[unum].pw, epw, MAXUSERNAME);
    }
  else				/* send a packet */
    sendAuth(cInfo.sock, CPAUTH_CHGPWD, "", pw);

  cdclrl(MSG_LIN1, 2);

  return;
}  
