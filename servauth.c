/* 
 * servauth.c - user stuff
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "conqlb.h"
#include "conqutil.h"
#include "context.h"
#include "conf.h"
#include "color.h"

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

      salt[0] = (login[0] != 0) ? login[0] : 'J';
      salt[1] = (login[1] != 0) ? login[1] : 'T';
      salt[2] = 0;
      
      strncpy(epw, (char *)crypt(pw, salt), MAXUSERNAME - 2);
      epw[MAXUSERNAME - 1] = 0;
      
      if (strcmp(epw, Users[unum].pw) != 0)
	{			/* invalid pw */
	  utLog("INFO: Invalid password for user '%s'", login);
	  return PERR_BADPWD;
	}

      utLog("INFO: user '%s' logged in", login);
      return PERR_OK;		/* it's all good */
    }

  /* if we're here, it's a new user.  encrypt pw for welcome. */

  salt[0] = (login[0] != 0) ? login[0] : 'J';
  salt[1] = (login[1] != 0) ? login[1] : 'T';
  salt[2] = 0;
  
  strncpy(epw, (char *)crypt(pw, salt), MAXUSERNAME - 2);
  epw[MAXUSERNAME - 1] = 0;

  utLog("INFO: New user '%s' logged in", login);
  
  return PERR_OK;
}
  

/* here, we wait for auth packets from the client until fatal error,
   or successful login */
int Authenticate(char *username, char *password)
{
  cpAuthenticate_t *cauth;
  int unum;
  char buf[PKT_MAXSIZE];
  int done, rv;
  char epw[MAXUSERNAME];
  int logcount = 3;

  expire_users();		/* now is a good time to expire users */

#if defined(DEBUG_SERVERAUTH)
  utLog("INFO: Authenticate ENTERING\n");
#endif

  done = FALSE;

  while (!done)
    {
      rv = pktWaitForPacket(CP_AUTHENTICATE, buf, 
                            PKT_MAXSIZE, (60 * 10), "Waiting for Auth");

      if (rv <= 0)
	{
	  utLog("conquestd:Authenticate: waitforpacket returned %d", rv);
	  return FALSE;
	}

      cauth = (cpAuthenticate_t *)buf;
      cauth->login[MAXUSERNAME - 1] = 0;
      cauth->pw[MAXUSERNAME - 1] = 0;

      if (checkuname((char *)cauth->login) == FALSE)
	{
	  pktSendAck(PSEV_ERROR, PERR_INVUSER,
                     NULL);
	  continue;
	}

      switch (cauth->flag)
	{
	case CPAUTH_CHECKUSER:

	  if (clbGetUserNum( &unum, (char *)cauth->login, 0 ) == TRUE)
	    {			/* user exits */
	      pktSendAck(PSEV_INFO, PERR_OK,
                         NULL);
	    }
	  else
	    {
	      pktSendAck(PSEV_ERROR, PERR_NOUSER,
                         NULL);
	    }

	  break;

	case CPAUTH_LOGIN:

	  if (logcount <= 0)	/* too many tries, fail */
	    {
	      pktSendAck(PSEV_FATAL, PERR_BADPWD,
                      NULL);
	      return FALSE;
	    }

	  if ((rv = doLogin((char *)cauth->login, (char *)cauth->pw, 
                            epw)) != PERR_OK)
	    {			/* somethings wrong, bad/inv pw, etc */
	      pktSendAck(PSEV_ERROR, rv,
		      NULL);
	      logcount--;
	    }
	  else
	    {			/* login successful */
	      done = TRUE;
	      pktSendAck(PSEV_INFO, PERR_OK,
                      NULL);
	    }

	  break;

	default:
	  utLog("servauth: invalid auth flag %d\n",
	       cauth->flag);
	  break;
	}
    }

  strncpy(username, (char *)cauth->login, MAXUSERNAME);
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
  utLog("INFO: expire_users(): Expiring users...");
#endif

  if (SysConf.UserExpiredays == 0)
    {				/* expiration has been disabled */
#if defined(DEBUG_SERVERAUTH)
      utLog("INFO: expire_users(): SysConf.UserExpiredays == 0, expiration disabled");
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




      difftime = time(0) - Users[i].lastentry;

      if (difftime < 0)
	{			/* screen out negative expirations -
				   only happens when system clock goes
				   way back */
	  utLog("INFO: expire_users(): difftime (%d) is less than 0, skipping user %s\n",
	       difftime, Users[i].username);
	  continue;
	}

#if defined(DEBUG_SERVERAUTH)
          utLog("expire_users(): time(0) = %d, Users[%d].lastentry = %d",
	       time(0),
	       i, Users[i].lastentry);
#endif

      if ((unsigned int)Users[i].lastentry != 0 && 
	  (unsigned int) difftime > expire_secs)
	{			/* we have a candidate... */
				/* loop thru the ships, making sure he
				   doesn't have one active */

#if defined(DEBUG_SERVERAUTH)
	  utLog("expire_users(): have a candidate: user '%s' (%d), difftime = %d > expire_secs = %d, Users[i].lastentry = %d",
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
	      utLog("INFO: expire_users(): Couldn't expire remote user '%s' due to active ship(s)",
		   Users[i].username);
	    }
	  else
	    {			/* waste him */
				/* have to play some trickery here
				   since resign locks the commonblock */
	      PVUNLOCK(&ConqInfo->lockword);

#if defined(DEBUG_SERVERAUTH)
	      utLog("expire_users(): calling clbResign(%d, %d)", i, TRUE);
#endif

	      clbResign(i, TRUE);
	      utLog("INFO: expire_users(): Expired remote user '%s' after %d days of inactivity. (limit %d days)",
		   Users[i].username,
		   difftime / SECS_PER_DAY,
                   SysConf.UserExpiredays);
				/* re-aquire the lock */
	      PVLOCK(&ConqInfo->lockword);
	    }
	}
    }

				/* done */
  PVUNLOCK(&ConqInfo->lockword);

#if defined(DEBUG_SERVERAUTH)
    utLog("expire_users(): ...Done");
#endif

  return;
}
