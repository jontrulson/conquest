/* 
 * userauth.c - user stuff
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "conqlb.h"
#include "context.h"
#include "conf.h"
#include "userauth.h"
#include "color.h"

#include <pwd.h>
#include <sys/types.h>

#include "server.h"
#include "packet.h"
#include "protocol.h"
#include "userauth.h"
#include "servauth.h"


static void expire_users(void);
static int doLogin(char *login, char *pw, char *epw)
{
  char salt[3];
  int unum;

  if (clbGetUserNum(&unum, login, 0))
    {				/* user exists */
				/* ENCRYPT and compare here... */

      salt[0] = (login[0] != EOS) ? login[0] : 'J';
      salt[1] = (login[1] != EOS) ? login[1] : 'T';
      salt[2] = EOS;
      
      strncpy(epw, (char *)crypt(pw, salt), MAXUSERNAME - 2);
      epw[MAXUSERNAME - 1] = EOS;
      
      if (strcmp(epw, Users[unum].pw) != 0)
	{			/* invalid pw */
	  clog("INFO: Invalid password for user '%s'", login);
	  return PERR_BADPWD;
	}

      clog("INFO: user '%s' logged in", login);
      return PERR_OK;		/* it's all good */
    }

  /* if we're here, it's a new user.  encrypt pw for welcome. */

  salt[0] = (login[0] != EOS) ? login[0] : 'J';
  salt[1] = (login[1] != EOS) ? login[1] : 'T';
  salt[2] = EOS;
  
  strncpy(epw, (char *)crypt(pw, salt), MAXUSERNAME - 2);
  epw[MAXUSERNAME - 1] = EOS;

  clog("INFO: New user '%s' logged in", login);
  
  return PERR_OK;
}
  

/* here, we wait for auth packets from the client until fatal error,
   or successful login */
int Authenticate(char *username, char *password)
{
  cpAuthenticate_t *cauth;
  int unum;
  Unsgn8 buf[PKT_MAXSIZE];
  int done, rv;
  char epw[MAXUSERNAME];
  int logcount = 3;

  expire_users();		/* now is a good time to expire users */

#if defined(DEBUG_SERVERAUTH)
  clog("INFO: Authenticate ENTERING\n");
#endif

  done = FALSE;

  while (!done)
    {
      rv = waitForPacket(PKT_FROMCLIENT, sInfo.sock, CP_AUTHENTICATE, 
			 buf, PKT_MAXSIZE, (60 * 10), "Waiting for Auth");

      if (rv <= 0)
	{
	  clog("conquestd:Authenticate: waitforpacket returned %d", rv);
	  return FALSE;
	}

      cauth = (cpAuthenticate_t *)buf;
      cauth->login[MAXUSERNAME - 1] = 0;
      cauth->pw[MAXUSERNAME - 1] = 0;

      if (checkuname(cauth->login) == FALSE)
	{
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_ERROR, PERR_INVUSER,
		  NULL);
	  continue;
	}

      switch (cauth->flag)
	{
	case CPAUTH_CHECKUSER:

	  if (clbGetUserNum( &unum, cauth->login, 0 ) == TRUE)
	    {			/* user exits */
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_OK,
		      NULL);
	    }
	  else
	    {
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_ERROR, PERR_NOUSER,
		      NULL);
	    }

	  break;

	case CPAUTH_LOGIN:

	  if (logcount <= 0)	/* too many tries, fail */
	    {
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_FATAL, PERR_BADPWD,
                      NULL);
	      return FALSE;
	    }

	  if ((rv = doLogin(cauth->login, cauth->pw, epw)) != PERR_OK)
	    {			/* somethings wrong, bad/inv pw, etc */
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_ERROR, rv,
		      NULL);
	      logcount--;
	    }
	  else
	    {			/* login successful */
	      done = TRUE;
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_INFO, PERR_OK,
                      NULL);
	    }

	  break;

	default:
	  clog("servauth: invalid auth flag %d\n",
	       cauth->flag);
	  break;
	}
    }

  strncpy(username, cauth->login, MAXUSERNAME);
  strncpy(password, epw, MAXUSERNAME);

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

#if defined(DEBUG_SERVERAUTH)
  clog("INFO: expire_users(): Expiring users...");
#endif

  if (SysConf.UserExpiredays == 0)
    {				/* expiration has been disabled */
#if defined(DEBUG_SERVERAUTH)
      clog("INFO: expire_users(): SysConf.UserExpiredays == 0, expiration disabled");
#endif

      return;
    }

  expire_secs = (SysConf.UserExpiredays * SECS_PER_DAY);
  PVLOCK(&ConqInfo->lockword);

  for (i=0; i < MAXUSERS; i++)
    {
      if (Users[i].live == FALSE || Users[i].robot ||
	   Users[i].ooptions[OOPT_OPER])
	continue;		/* only living, human, non godlike users
				   are subject to expiration.  */




      difftime = getnow(NULL, 0) - Users[i].lastentry;

      if (difftime < 0)
	{			/* screen out negative expirations -
				   only happens when system clock goes
				   way back */
	  clog("INFO: expire_users(): difftime (%d) is less than 0, skipping user %s\n",
	       difftime, Users[i].username);
	  continue;
	}

#if defined(DEBUG_SERVERAUTH)
          clog("expire_users(): getnow(NULL, 0) = %d, Users[%d].lastentry = %d",
	       getnow(NULL, 0),
	       i, Users[i].lastentry);
#endif

      if ((unsigned int)Users[i].lastentry != 0 && 
	  (unsigned int) difftime > expire_secs)
	{			/* we have a candidate... */
				/* loop thru the ships, making sure he
				   doesn't have one active */

#if defined(DEBUG_SERVERAUTH)
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

#if defined(DEBUG_SERVERAUTH)
	      clog("expire_users(): calling clbResign(%d, %d)", i, TRUE);
#endif

	      clbResign(i, TRUE);
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

#if defined(DEBUG_SERVERAUTH)
    clog("expire_users(): ...Done");
#endif

  return;
}
