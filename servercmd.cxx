#include "c_defs.h"

/************************************************************************
 *
 * server operator command handling
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "cb.h"
#include "conf.h"
#include "server.h"
#include "serverpkt.h"
#include "context.h"
#include "record.h"
#include "servercmd.h"
#include "conqlb.h"
#include "conqutil.h"
#include "conqai.h"

void startRecord(int f)
{
    char fname[PATH_MAX] = {};
    char bname[PATH_MAX] = {};
    char cbuf[MESSAGE_SIZE] = {};
    time_t thetime;
    struct tm *tmtime;
    char tbuf[128] = {};

    if (Context.recmode == RECMODE_ON)
    {
        utLog("conquestd: startRecord: already recording.");
        if (f)
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, Context.snum,
                        "We are already recording.");
        return;
    }

    /* make a filename.  This will be something like
       CONQSTATE/conquest-rec-MMDDYYYY-HHMM.cpr */

    thetime = time(0);
    tmtime = localtime(&thetime);

    strftime(tbuf, 128 - 1, "%m%d%Y-%H%M", tmtime);

    snprintf(bname, PATH_MAX, "conquest-rec-%s.cqr", tbuf);

    snprintf(fname, PATH_MAX, "%s/%s", CONQSTATE, bname);

    if (recOpenOutput(fname, true))
    {                     /* we are almost ready... */
        Context.recmode = RECMODE_STARTING;
    }
    else
    {
        Context.recmode = RECMODE_OFF;
        utLog("conquestd: Cannot record to %s", fname);
        snprintf(cbuf, MESSAGE_SIZE, "Cannot record to %s", bname);
        if (f)
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, Context.snum, cbuf);
        return;
    }

    /* start recording */
    if (Context.recmode == RECMODE_STARTING)
    {
        if (recInitOutput(Context.unum, time(0), Context.snum,
                          true))
        {
            Context.recmode = RECMODE_ON;
            utLog("conquestd: Recording to %s", fname);
            snprintf(cbuf, MESSAGE_SIZE, "Recording to %s", bname);
            if (f)
                clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, Context.snum, cbuf);

            /* now reset the recorded packet cache so we can start
               fresh */
            spktInitRec();
        }
        else
        {
            Context.recmode = RECMODE_OFF;
            utLog("conquestd: recInitOutput failed");
            if (f)
                clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, Context.snum,
                            "conquestd: recInitOutput failed");
        }
    }

    return;
}

static void stopRecord(void)
{
    if (Context.recmode != RECMODE_ON)
    {
        utLog("conquestd: stopRecord: not recording.");
        clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, Context.snum,
                    "We aren't recording.");
        return;
    }


    recCloseOutput();
    utLog("conquestd: stopRecord: recording stopped");
    clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, Context.snum,
                "Recording stopped");

    return;
}

static void CreateRobots(int snumFrom, char *arg1, char *arg2, char *arg3)
{
    int i, j, num, anum, unum, snum, warlike;
    char buf[MSGMAXLINE];

    /* arg1 = username, arg2 is number to create (default 0), arg3, if
       present, makes them warlike (default peaceful) */

    /* user to create */
    if (!arg1)
        return;
    else
    {
        if ( ! clbGetUserNum( &unum, arg1, USERTYPE_BUILTIN ) )
        {
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, (uint16_t)snumFrom,
                        "No such user.");
            return;
        }
    }

    /* number to create */
    if (!arg2)
        num = 1;
    else
    {
        num = atoi(arg2);

        if (num <= 0)
            num = 1;
    }

    /* warlike - if there was an arg3, it's warlike */
    if (arg3)
        warlike = true;
    else
        warlike = false;

    /* now create them. */

    anum = 0;
    for ( i = 1; i <= num; i++ )
    {
        if ( ! newrob( &snum, unum ) )
	{
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, (uint16_t)snumFrom,
                        "Failed to create robot ship.");
            break;
	}

        anum++;

        /* If requested, make the robot war-like. */
        if ( warlike )
	{
            for ( j = 0; j < NUMPLAYERTEAMS; j++ )
                cbShips[snum].war[j] = true;
            cbShips[snum].war[cbShips[snum].team] = false;
	}
    }

    /* Report the good news. */
    utLog("conquestd: %s created %d %s%s (%s) robot(s)",
          cbUsers[Context.unum].username,
          anum,
          (warlike == true) ? "WARLIKE " : "",
          cbUsers[unum].alias,
          cbUsers[unum].username);

    sprintf( buf, "%sAutomation %s (%s) is now flying ",
             (warlike) ? "Warlike " : "",
             cbUsers[unum].alias, cbUsers[unum].username );
    if ( anum == 1 )
        utAppendShip(buf , snum) ;
    else
    {
        utAppendInt(buf , anum) ;
        strcat(buf , " new ships.") ;
    }
    clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, (uint16_t)snumFrom, buf);

    return;
}

/* some of this ripped right from conqoper kiss() */
static void Murder(int from, char *what)
{
    static const char *cant_kill_ship_str = "You can't kill ship %c%d (%s) status (%s).";
    static const char *kill_ship_str1 = "Killing ship %c%d (%s).";
    static const char *kill_ship_str2 = "Killing ship %c%d (%s) user (%s).";
    static const char *no_user_str = "No such user.";
    static const char *no_ship_str = "No such ship.";
    static const char *not_flying_str = "User %s (%s) isn't flying right now.";
    int snum = -1, unum = -1, didany;
    char ssbuf[MSGMAXLINE], mbuf[MSGMAXLINE];

    uint16_t fromShip = (uint16_t)from;

    /* first see if it's a number (indicating a ship) */
    if (utIsDigits(what))
    {                           /* yes */
        snum = atoi(what);
        if ( snum < 0 || snum >= MAXSHIPS )
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromShip, no_ship_str);
        else if ( cbShips[snum].status != SS_LIVE )
        {
            ssbuf[0] = 0;
            utAppendShipStatus(ssbuf, cbShips[snum].status) ;
            sprintf(mbuf, cant_kill_ship_str,
                    cbTeams[cbShips[snum].team].teamchar,
                    snum,
                    cbShips[snum].alias,
                    ssbuf);
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromShip, mbuf);
        }
        else
        {
            sprintf(mbuf, kill_ship_str1,
                    cbTeams[cbShips[snum].team].teamchar, snum, cbShips[snum].alias);
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromShip, mbuf);
            clbKillShip( snum, KB_GOD, 0 );
        }

        return;
    }

    /* a user? */
    /* Kill a user? */
    if ( ! clbGetUserNum( &unum, what, USERTYPE_ANY ) )
    {
        clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromShip, no_user_str);
        return;
    }

    didany = false;
    for ( snum = 0; snum < MAXSHIPS; snum++ )
        if ( cbShips[snum].status == SS_LIVE )
            if ( cbShips[snum].unum == unum )
            {
                didany = true;
                sprintf(mbuf, kill_ship_str2,
                        cbTeams[cbShips[snum].team].teamchar,
                        snum,
                        cbShips[snum].alias,
                        what);
                clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromShip, mbuf);
                clbKillShip( snum, KB_GOD, 0 );
            }


    if ( ! didany )
    {
        sprintf(mbuf, not_flying_str, cbUsers[unum].username,
                cbUsers[unum].alias);
        clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromShip, mbuf);
    }

    return;
}

/* Split up a string into a command and 3 arg strings. */
/* this is fairly brute-force hacky */
static void _parseArgs(char *str, char **cmd, char **arg1,
                       char **arg2, char **arg3)
{
    char *ch, *base;
    int i, j, len;

    if (!str || !*str)
        return;

    /* first, go through the string, removing any leading/trailing
       spaces.  We also 'compress' multiple spaces into a
       single one. */

    base = str;
    /* leading */
    while (*base)
        if (*base == ' ')
            base++;
        else
            break;

    /* trailing */
    len = strlen(base);

    while (len > 0 && base[len - 1] == ' ')
    {
        base[len - 1] = 0;
        len--;
    }

    /* now compress them */
    for ( i = 0; base[i] != 0; )
        if ( base[i] == ' ' && base[i+1] == ' ' )
            for ( j = i; base[j] != 0; j++)
                base[j] = base[j+1];
        else
            i++;

    /* finally.  Now just go thru and set up the pointers that have a
       value */

    /* command */
    if (*base)
        *cmd = base;
    /* look for another arg */
    if ((ch = strchr(base, ' ')))
    {
        *ch = 0;
        base = ch + 1;
    }
    else
        return;                     /* we're done */

    /* args */
    if (*base)
        *arg1 = base;
    /* look for another arg */
    if ((ch = strchr(base, ' ')))
    {
        *ch = 0;
        base = ch + 1;
    }
    else
        return;                     /* we're done */

    if (*base)
        *arg2 = base;
    /* look for another arg */
    if ((ch = strchr(base, ' ')))
    {
        *ch = 0;
        base = ch + 1;
    }
    else
        return;                     /* we're done */

    if (*base)
        *arg3 = base;

    return;
}

int checkOperExec(msgFrom_t from, uint16_t fromDetail,
                  msgTo_t to, uint16_t toDetail, char *msg)
{
    char tmsg[MESSAGE_SIZE], umsg[MESSAGE_SIZE];
    char *p;
    static const char *oerror = "/recon, recoff, r[obot], k[ill]";
    char *cmd, *arg1, *arg2, *arg3;

    /* first, if the message isn't to GOD, ignore */
    if (to != MSG_TO_GOD)
        return false;

    /* if it's not from a valid ship, ignore */
    if (from != MSG_FROM_SHIP || fromDetail >= MAXSHIPS)
        return false;

    utStrncpy(tmsg, msg, MESSAGE_SIZE);

    if (tmsg[0] != '/')
        return false;               /* not for us. */

    /* it is for us, now check for allowability */
    if (!UISOPER(cbShips[fromDetail].unum))
    {                           /* nice try... */
        clbStoreMsg(MSG_FROM_GOD, 0,
                    MSG_TO_SHIP, fromDetail,
                    "You are not a Conquest Operator.");
        utLog("conquestd: EXEC from unprivileged ship: %d, '%s'", fromDetail,
              tmsg);
        return false;
    }

    /* ok, let's see what is up. */
    p = &(tmsg[1]);

    if (!*p)
    {
        clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail, oerror);
        return false;
    }

    /* check commands requiring args (no upper-case) */

    cmd = arg1 = arg2 = arg3 = NULL;

    _parseArgs(p, &cmd, &arg1, &arg2, &arg3);

    if (!cmd)
    {
        clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail, oerror);
        return false;
    }

    /* uppercase only version */
    utStrncpy(umsg, cmd, MESSAGE_SIZE);
    utToUpperCase(umsg);

    /* check the simple commands first - look at the upcase version */

    /* recording */
    if (!strncmp(umsg, "RECON", 5))
    {
        startRecord(true);
        return true;
    }

    if (!strncmp(umsg, "RECOFF", 6))
    {
        stopRecord();
        Context.recmode = RECMODE_OFF;
        return true;
    }

    /* commands requiring arguments */

    /* kill something */
    if (cmd[0] == 'K' || cmd[0] == 'k')
    {                           /* fur is murder! */
        if (!arg1)
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail,
                        "Usage: k[ill] <what>");
        else
            Murder(fromDetail, arg1);

        return true;
    }

    /* create robots */
    if (cmd[0] == 'R' || cmd[0] == 'r')
    {                           /* little tin men */
        if (!arg1)
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail,
                        "Usage: r[obot] <username> [<number> <warlike if non-null>]");
        else
            CreateRobots((int)fromDetail, arg1, arg2, arg3);

        return true;
    }



    /* that's all we understand for now. */

    clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail, oerror);
    return false;
}
