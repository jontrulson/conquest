/* 
 * userauth.c - user stuff
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "conf.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "color.h"

#define MAX_USERLEN 10		/* only 10 chars for users */

static int checkuname(char *username);
static void expire_users(void);

static void PrintStatus(int lin)
{
  char buf[MID_BUFFER_SIZE];
  int numusers = 0;
  int numships = 0;
  int numshipsactive = 0;
  int numshipsvacant = 0;
  int numshipsrobot = 0;
  int i;

  sprintf(buf, 
	  "#%d#STATUS: #%d#Users #%d#%%d, #%d#Ships #%d#%%d/%%d #%d#"
	  "(#%d#%%d #%d#active, #%d#%%d #%d#vacant, "
	  "#%d#%%d #%d#robot)",
	  MagentaColor, NoColor, CyanColor, NoColor, CyanColor, NoColor,
	  CyanColor, NoColor, CyanColor, NoColor, 
	  CyanColor, NoColor);

  /* get total users */
  for ( i = 0; i < MAXUSERS; i++)
    if ( Users[i].live)
      numusers++;

  /* count ships */
  for ( i = 1; i <= MAXSHIPS; i++ )
    {
      if ( Ships[i].status == SS_LIVE )
	{
	  numships++;

	  if (Users[Ships[i].unum].robot)
	    numshipsrobot++;
	  else
	    {
	      if (CheckPid(Ships[i].pid))
		numshipsactive++;
	      else
		numshipsvacant++;
	    }
	}
    }
    
  cprintf(lin, 0, ALIGN_CENTER, buf,
	  numusers, numships, MAXSHIPS, numshipsactive, 
	  numshipsvacant, numshipsrobot);

  return;
}

int Logon(char *username, char *password)
{
  int col, lin, slin, lenc1, unum;
  int ch, statline;
  char nm[SIZEUSERNAME], pw[SIZEUSERNAME], pwr[SIZEUSERNAME], 
    epw[SIZEUSERNAME];
  char salt[3];
  string c1=" CCC    OOO   N   N   QQQ   U   U  EEEEE   SSSS  TTTTT";
  string c2="C   C  O   O  NN  N  Q   Q  U   U  E      S        T";
  string c3="C      O   O  N N N  Q   Q  U   U  EEE     SSS     T";
  string c4="C   C  O   O  N  NN  Q  Q   U   U  E          S    T";
  string c5=" CCC    OOO   N   N   QQ Q   UUU   EEEEE  SSSS     T";
  int done;
  extern char *ConquestVersion;
  extern char *ConquestDate;

  expire_users();		/* now is a good time to expire users */

  if (!IsRemoteUser())
    {				/* if local, just get uname */
      glname(username, SIZEUSERNAME);
      password[0] = EOS;	/* locals don't have passwords */
      clog("INFO: Local user '%s' logged in", username);
      return(UT_LOCAL);
    }
				/* if we're here it means we're remote,
				   need to get password and username here */
#if defined DEBUG_SERVER
  clog("INFO: Detected a remote user\n");
#endif

  done = FALSE;

  while (!done)
    {

      cdclear();

      /* Display the logo. */
      lenc1 = strlen( c1 );
      col = (Context.maxcol - lenc1) / 2;
      lin = 2;
      cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c1);
      lin++;
      cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c2);
      lin++;
      cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c3);
      lin++;
      cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c4);
      lin++;
      cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | A_BOLD, c5);

      /* Draw a box around the logo. */
      lin++;
      attrset(A_BOLD);
      cdbox( 1, col - 2, lin, col + lenc1 + 1 );
      attrset(0);

      lin++;

      if ( ConqInfo->closed )
	{
	  cprintf( lin, 0, ALIGN_CENTER, "#%d#%s", RedLevelColor,
		   "The game is closed.");
	  cdend();
	  exit(2);
	}
      else
	cprintf( lin, 1, ALIGN_CENTER, "#%d#Welcome to #%d#Conquest#%d# %s (%s)", 
		 YellowLevelColor,
		 RedLevelColor,
		 YellowLevelColor,
		 ConquestVersion, ConquestDate);

      lin++;
      statline = lin;
      PrintStatus(statline);

      lin += 4;
      slin = lin;

      nm[0] = EOS;

      cdrefresh();

      while (TRUE)			/* login loop */
	{
	  slin = lin;
	  cdclrl( slin, Context.maxlin - slin - 1 );
	  slin += 3;
	  cdputs("You can use A-Z, a-z, 0-9, '_', or '-'.", MSG_LIN1, 1);
	  cprintf( slin - 1, 1, ALIGN_LEFT,
		   "#%dPlease login. Press [RETURN] to exit, [TAB] to refresh status.",
		   SpecialColor);
	  ch = cdgetx( "Username: ", slin, 1, TERMS, nm, MAX_USERLEN, TRUE );

	  if (ch == TERM_EXTRA )
	    break;		/* redraw stats and things */

	  if (ch == TERM_ABORT || nm[0] == EOS)
	    {			/* if empty user, or abort char, leave */
	      cdend();
	      exit(2);
	    }

	  if (checkuname(nm) == FALSE)
	    {			/* invalid username */
	      cdbeep();
	      attrset(RedLevelColor);
	      cdputs("Invalid character in username.", MSG_LIN2, 1);
	      attrset(NoColor);
	      nm[0] = EOS;
	      continue;
	    }
	  /* see if remote user exists */
	  if (!gunum( &unum, nm, UT_REMOTE ) )
	    {			/* nope... */
	      slin++;
	      if (askyn("User doesn't exist. Is this a new user? ", slin, 1))
		{			/* yep */
		  pw[0] = EOS;
		  cdclrl( MSG_LIN1, 2  );
		  cdputs("Use any printable characters.", MSG_LIN1, 1);
		  ch = cdgetx( "Password: ", slin, 1, 
			       TERMS, pw, SIZEUSERNAME - 1, FALSE );
	      
		  slin++;
		  pwr[0] = EOS;
		  cdclrl( MSG_LIN1, 2  );
		  cdputs("Use any printable characters.", MSG_LIN1, 1);
		  ch = cdgetx( "Retype Password: ", slin, 1, 
			       TERMS, pwr, SIZEUSERNAME - 1, FALSE );
	      
		  if (strcmp(pw, pwr) != 0)
		    {			/* pw's don't match, start over */
		      cdbeep();
		      cdclrl( MSG_LIN2, 1  );
		      attrset(RedLevelColor);
		      cdputs("Passwords don't match.", MSG_LIN2, 1);
		      attrset(NoColor);
		      cdrefresh();
		      sleep(2);
		      continue;
		    }
		  /* if we're here, we have a username
		     and password (new user) - time
		     to rock. */

		  /* ENCRYPT it here */
		  salt[0] = (nm[0] != EOS) ? nm[0] : 'J';
		  salt[1] = (nm[1] != EOS) ? nm[1] : 'T';
		  salt[2] = EOS;

		  strncpy(epw, (char *)crypt(pw, salt), SIZEUSERNAME - 2);
		  epw[SIZEUSERNAME - 1] = EOS;

		  clog("INFO: New remote user '%s' logged in", nm);
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
			   TERMS, pw, SIZEUSERNAME - 1, FALSE );

	      /* ENCRYPT and compare here... */

	      salt[0] = (nm[0] != EOS) ? nm[0] : 'J';
	      salt[1] = (nm[1] != EOS) ? nm[1] : 'T';
	      salt[2] = EOS;
	  
	      strncpy(epw, (char *)crypt(pw, salt), SIZEUSERNAME - 2);
	      epw[SIZEUSERNAME - 1] = EOS;
	  
	      if (strcmp(epw, Users[unum].pw) != 0)
		{			/* invalid pw */
		  cdbeep();
		  cdclrl( MSG_LIN2, 1  );
		  attrset(RedLevelColor);
		  cdputs("Invalid Password.", MSG_LIN2, 1);
		  attrset(NoColor);
		  cdrefresh();
		  clog("INFO: Invalid password for user '%s'", nm);
		  sleep(2);
		  continue;
		}
	  
	      /* good pw - go for the gusto */
	      done = TRUE;
	      clog("INFO: Remote user '%s' logged in", nm);
	      break;
	    }
	} /* end while */
    }
				/* if we're here, we're legal */
  strncpy(username, nm, SIZEUSERNAME);
  strncpy(password, epw, SIZEUSERNAME);

  return(UT_REMOTE);
}

/* ChangePassword() - change a users password - called from UserOptsMenu() */
void ChangePassword(int unum, int godlike)
{
  static char *header = "Change Password";
  char pw[SIZEUSERNAME], pwr[SIZEUSERNAME], epw[SIZEUSERNAME];
  char salt[3];
  int ch;
  int lin = 0, col = 0;

  if (godlike == FALSE)
    cdclear();
  else
    {
      cdclrl(MSG_LIN1, 2);
    }

  if (godlike == FALSE)
    {
      lin = 1;
      col = ((cdcols() / 2) - (strlen(header) / 2));
      
      cprintf(lin, col, ALIGN_NONE, "#%d#%s", NoColor, header);
      
      lin += 3;
      col = 5;
      /* verify old password */
      pw[0] = EOS;
      cdputs("Use any printable characters.", MSG_LIN1, 1);
      ch = cdgetx( "Old Password: ", lin, 1, 
		   TERMS, pw, SIZEUSERNAME - 1, FALSE );
      
      salt[0] = (Users[unum].username[0] != EOS) ? Users[unum].username[0] : 
	'J';
      salt[1] = (Users[unum].username[1] != EOS) ? Users[unum].username[1] : 
	'T';
      salt[2] = EOS;
  
      strncpy(epw, (char *)crypt(pw, salt), SIZEUSERNAME - 2);
      epw[SIZEUSERNAME - 1] = EOS;

      if (strcmp(epw, Users[unum].pw) != 0)
	{			/* invalid pw */
	  cdbeep();
	  cdclrl( MSG_LIN2, 1  );
	  attrset(RedLevelColor);
	  cdputs("Invalid Password.", MSG_LIN2, 1);
	  attrset(RedLevelColor);
	  cdrefresh();
	  clog("INFO: ChangePassword(): Invalid old password for user '%s'", 
	       Users[unum].username);
	  sleep(2);
	  return;
	}
      
      lin++;
      
    } /* ! godlike */
				/* get and recheck new passwd */
  pw[0] = EOS;
  cdclrl( MSG_LIN1, 2  );
  cdputs("Use any printable characters.", MSG_LIN2, 1);

  ch = cdgetx( "New Password: ", MSG_LIN1, 1, 
	       TERMS, pw, SIZEUSERNAME - 1, FALSE );

  if (godlike == FALSE)
    {
      pwr[0] = EOS;
      cdclrl( MSG_LIN1, 2  );
      cdputs("Use any printable characters.", MSG_LIN2, 1);
      ch = cdgetx( "Retype Password: ", MSG_LIN1, 1, 
		   TERMS, pwr, SIZEUSERNAME - 1, FALSE );
      
      if (strcmp(pw, pwr) != 0)
	{			/* pw's don't match, start over */
	  cdbeep();
	  cdclrl( MSG_LIN2, 1  );
	  attrset(RedLevelColor);
	  cdputs("Passwords don't match.", MSG_LIN2, 1);
	  attrset(NoColor);
	  cdrefresh();
	  sleep(2);
	  return;
	}
    } /* ! godlike */
				/* ok, we have a new password -
				   make it so */

  salt[0] = (Users[unum].username[0] != EOS) ? Users[unum].username[0] : 'J';
  salt[1] = (Users[unum].username[1] != EOS) ? Users[unum].username[1] : 'T';
  salt[2] = EOS;
  
  strncpy(epw, (char *)crypt(pw, salt), SIZEUSERNAME - 2);
  epw[SIZEUSERNAME - 1] = EOS;

  strncpy(Users[unum].pw, epw, SIZEUSERNAME);

  cdclrl(MSG_LIN1, 2);

  return;
}  
  
/* check the validity of a supplied username */  
static int checkuname(char *username)
{
  char *s = username;

  while (*s)
    {
      if (!isalnum(*s) && *s != '-' && *s != '_')
	{			/* if invalid character... */
	  return(FALSE);
	}
      s++;
    }

  return(TRUE);
}
	
/* expire any old users (unless they have active ships running...) */
/*  (DOES LOCKING) */
void expire_users(void)
{
  register int i, j;
  time_t difftime = 0;
  int hasship = FALSE;
  unsigned int expire_secs;

#if defined(DEBUG_SERVER)
  clog("INFO: expire_users(): Expiring users...");
#endif

  if (SysConf.UserExpiredays == 0)
    {				/* expiration has been disabled */
#if defined(DEBUG_SERVER)
      clog("INFO: expire_users(): SysConf.UserExpiredays == 0, expiration disabled");
#endif

      return;
    }

  expire_secs = (SysConf.UserExpiredays * SECS_PER_DAY);
  PVLOCK(&ConqInfo->lockword);

  for (i=0; i < MAXUSERS; i++)
    {
      if (Users[i].live == FALSE || Users[i].type != UT_REMOTE)
	continue;		/* only living remote users are subject to
				   expiration.  */

      difftime = getnow(NULL, 0) - Users[i].lastentry;

      if (difftime < 0)
	{			/* screen out negative expirations -
				   only happens when system clock goes
				   way back */
	  clog("INFO: expire_users(): difftime (%d) is less than 0, skipping user %s\n",
	       difftime, Users[i].username);
	  continue;
	}

#if defined(DEBUG_SERVER)
          clog("expire_users(): getnow(NULL, 0) = %d, Users[%d].lastentry = %d",
	       getnow(NULL, 0),
	       i, Users[i].lastentry);
#endif

      if ((unsigned int)Users[i].lastentry != 0 && 
	  (unsigned int) difftime > expire_secs)
	{			/* we have a candidate... */
				/* loop thru the ships, making sure he
				   doesn't have one active */

#if defined(DEBUG_SERVER)
	  clog("expire_users(): have a candidate: user '%s' (%d), difftime = %d > expire_secs = %d, Users[i].lastentry = %d",
	       Users[i].username,
	       i,
	       difftime,
	       expire_secs,
	       Users[i].lastentry);
#endif

	  hasship = FALSE;
	  for (j=1; j <= MAXSHIPS; j++)
	    {
	      if (Ships[j].unum == i && Ships[j].status == SS_LIVE)
		{
		  hasship = TRUE;
		  break;
		}
	    }

	  if (hasship)
	    {			/* we can't waste him */
	      clog("INFO: expire_users(): Couldn't expire remote user '%s' due to active ship(s)",
		   Users[i].username);
	    }
	  else
	    {			/* waste him */
				/* have to play some trickery here
				   since resign locks the commonblock */
	      PVUNLOCK(&ConqInfo->lockword);

#if defined(DEBUG_SERVER)
	      clog("expire_users(): calling resign(%d, %d)", i, TRUE);
#endif

	      resign(i, TRUE);
	      clog("INFO: expire_users(): Expired remote user '%s' after %d days of inactivity",
		   Users[i].username,
		   difftime / SECS_PER_DAY);
				/* re-aquire the lock */
	      PVLOCK(&ConqInfo->lockword);
	    }
	}
    }

				/* done */
  PVUNLOCK(&ConqInfo->lockword);

#if defined(DEBUG_SERVER)
    clog("expire_users(): ...Done");
#endif

  return;
}
