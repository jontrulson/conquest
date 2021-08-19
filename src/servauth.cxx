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

#include <string>

#include "global.h"
#include "conqdef.h"
#include "cb.h"
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

    if (clbGetUserNum(&unum, login, UserType::Normal))
    {				/* user exists */
				/* ENCRYPT and compare here... */

        salt[0] = (login[0] != 0) ? login[0] : 'J';
        salt[1] = (login[1] != 0) ? login[1] : 'T';
        salt[2] = 0;

        const char* _epw = (char *)crypt(pw, salt);

        if (!_epw)
	{			/* invalid pw */
            utLog("%s: crypt() returned NULL for user '%s'",
                  __FUNCTION__, login);
            return PERR_BADPWD;
	}

        utStrncpy(epw, _epw, MAX_USERNAME);

        if (strcmp(epw, cbUsers[unum].pw) != 0)
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

    utStrncpy(epw, (char *)crypt(pw, salt), MAX_USERNAME);

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
    char epw[MAX_USERNAME];
    int logcount = 3;

    expire_users();		/* now is a good time to expire users */

#if defined(DEBUG_SERVERAUTH)
    utLog("INFO: Authenticate ENTERING\n");
#endif

    done = false;

    unsigned int startTime = clbGetMillis();
    const unsigned int waitTime = 300000; // 5 minutes
    while (!done)
    {
        if ((clbGetMillis() - startTime) >= waitTime)
        {
            utLog("%s: timed out waiting for CP_AUTHENTICATE packet",
                __FUNCTION__);
            return false;
        }

        rv = pktRead(buf, PKT_MAXSIZE, 1);

        if (rv < 0)
	{
            utLog("%s: waitforpacket returned %d", __FUNCTION__, rv);
            return false;
	}

        if (rv == 0)
            continue;

        if (rv != CP_AUTHENTICATE)
        {
            if (rv == CP_COMMAND)
            {
                // then the user has exited
                utLog("%s: got CP_COMMAND, failing Authentication",
                      __FUNCTION__);
                return false;
            }
            utLog("%s: got unexpected packet type %d, continuing...",
                  __FUNCTION__, rv);

            continue;
        }

        cauth = (cpAuthenticate_t *)buf;
        cauth->login[MAX_USERNAME - 1] = 0;
        cauth->pw[MAX_USERNAME - 1] = 0;

        std::string tmpUser = (char *)cauth->login;
        if (!uaValidateUsername(tmpUser))
	{
            pktSendAck(PSEV_ERROR, PERR_INVUSER);
            continue;
	}

        switch (cauth->flag)
	{
	case CPAUTH_CHECKUSER:

            if (clbGetUserNum( &unum, (char *)cauth->login,
                               UserType::Normal ) == true)
	    {			/* user exits */
                pktSendAck(PSEV_INFO, PERR_OK);
	    }
            else
	    {
                pktSendAck(PSEV_ERROR, PERR_NOUSER);
	    }

            break;

	case CPAUTH_LOGIN:

            if (logcount <= 0)	/* too many tries, fail */
	    {
                pktSendAck(PSEV_FATAL, PERR_BADPWD);
                return false;
	    }

            if ((rv = doLogin((char *)cauth->login, (char *)cauth->pw,
                              epw)) != PERR_OK)
	    {			/* somethings wrong, bad/inv pw, etc */
                pktSendAck(PSEV_ERROR, rv);
                logcount--;
	    }
            else
	    {			/* login successful */
                done = true;
                pktSendAck(PSEV_INFO, PERR_OK);
	    }

            break;

	default:
            utLog("servauth: invalid auth flag %d\n",
                  cauth->flag);
            break;
	}
    }

    utStrncpy(username, (char *)cauth->login, MAX_USERNAME);
    utStrncpy(password, epw, MAX_USERNAME);

    return(true);
}


/* expire any old users (unless they have active ships running...) */
/*  (DOES LOCKING) */
void expire_users(void)
{
    int i, j;
    time_t difftime = 0;
    int hasship = false;
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
    cbLock(&cbConqInfo->lockword);

    for (i=0; i < cbLimits.maxUsers(); i++)
    {
        if (!ULIVE(i) || UROBOT(i) || UISOPER(i))
            continue;		/* only living, human, non godlike users
				   are subject to expiration.  */




        difftime = time(0) - cbUsers[i].lastentry;

        if (difftime < 0)
	{			/* screen out negative expirations -
				   only happens when system clock goes
				   way back */
            utLog("INFO: expire_users(): difftime (%ld) is less than 0, skipping user %s\n",
                  difftime, cbUsers[i].username);
            continue;
	}

#if defined(DEBUG_SERVERAUTH)
        utLog("expire_users(): time(0) = %d, cbUsers[%d].lastentry = %d",
              time(0),
              i, cbUsers[i].lastentry);
#endif

        if ((unsigned int)cbUsers[i].lastentry != 0 &&
            (unsigned int) difftime > expire_secs)
	{			/* we have a candidate... */
				/* loop thru the ships, making sure he
				   doesn't have one active */

#if defined(DEBUG_SERVERAUTH)
            utLog("expire_users(): have a candidate: user '%s' (%d), difftime = %d > expire_secs = %d, cbUsers[i].lastentry = %d",
                  cbUsers[i].username,
                  i,
                  difftime,
                  expire_secs,
                  cbUsers[i].lastentry);
#endif

            hasship = false;
            for (j=0; j < cbLimits.maxShips(); j++)
	    {
                if (cbShips[j].unum == i && cbShips[j].status == SS_LIVE)
		{
                    hasship = true;
                    break;
		}
	    }

            if (hasship)
	    {			/* we can't waste him */
                utLog("INFO: expire_users(): Couldn't expire remote user '%s' due to active ship(s)",
                      cbUsers[i].username);
	    }
            else
	    {			/* waste him */
				/* have to play some trickery here
				   since resign locks the commonblock */
                cbUnlock(&cbConqInfo->lockword);

#if defined(DEBUG_SERVERAUTH)
                utLog("expire_users(): calling clbResign(%d, %d)", i, true);
#endif

                clbResign(i, true);
                utLog("INFO: expire_users(): Expired remote user '%s' after %ld days of inactivity. (limit %d days)",
                      cbUsers[i].username,
                      difftime / SECS_PER_DAY,
                      SysConf.UserExpiredays);
                /* re-aquire the lock */
                cbLock(&cbConqInfo->lockword);
	    }
	}
    }

    /* done */
    cbUnlock(&cbConqInfo->lockword);

#if defined(DEBUG_SERVERAUTH)
    utLog("expire_users(): ...Done");
#endif

    return;
}
