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

#include <vector>
#include <string>

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
    char cbuf[MAX_MSGSIZE] = {};
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
        snprintf(cbuf, MAX_MSGSIZE, "Cannot record to %s", bname);
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
            snprintf(cbuf, MAX_MSGSIZE, "Recording to %s", bname);
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

static void CreateRobots(int snumFrom, const char *arg1,
                         const char *arg2, const char *arg3)
{
    int i, j, num, anum, unum, snum, warlike;
    char buf[MAX_MSGSIZE];

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
    for ( i=0; i<num; i++ )
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

    snprintf( buf, MAX_MSGSIZE, "%sAutomation %s (%s) is now flying ",
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
static void Murder(int from, const char *what)
{
    static const char *cant_kill_ship_str = "You can't kill ship %c%d (%s) status (%s).";
    static const char *kill_ship_str1 = "Killing ship %c%d (%s).";
    static const char *kill_ship_str2 = "Killing ship %c%d (%s) user (%s).";
    static const char *no_user_str = "No such user.";
    static const char *no_ship_str = "No such ship.";
    static const char *not_flying_str = "User %12.s (%12.s) isn't flying right now.";
    int snum = -1, unum = -1, didany;
    char ssbuf[MAX_MSGSIZE], mbuf[MAX_MSGSIZE];

    uint16_t fromShip = (uint16_t)from;

    /* first see if it's a number (indicating a ship) */
    if (utIsDigits(what))
    {                           /* yes */
        snum = atoi(what);
        if ( snum < 0 || snum >= cbLimits.maxShips() )
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
    for ( snum = 0; snum < cbLimits.maxShips(); snum++ )
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
        snprintf(mbuf, MAX_MSGSIZE, not_flying_str, cbUsers[unum].username,
                cbUsers[unum].alias);
        clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromShip, mbuf);
    }

    return;
}

/* Split up a string into a vector of strings (thanks stackexchange!) */
static std::vector<std::string> _parseArgs(const std::string& str, char chr)
{
    std::string::const_iterator first = str.cbegin();
    std::string::const_iterator second = std::find(first+1, str.cend(), chr);
    std::vector<std::string> vec;

    while(second != str.cend())
    {
        if (first[0] == chr)
            vec.emplace_back(first + 1, second);
        else
            vec.emplace_back(first, second);
        first = second;
        second = std::find(second+1, str.cend(), chr);
    }

    if (first[0] == chr)
        vec.emplace_back(first + 1, str.cend());
    else
        vec.emplace_back(first, str.cend());

    return vec;
}

int checkOperExec(msgFrom_t from, uint16_t fromDetail,
                  msgTo_t to, uint16_t toDetail, char *msg)
{
    std::string tmsg;
    static const char *oerror = "/recon, /recoff, /r[obot], /k[ill]";

    /* first, if the message isn't to GOD, ignore */
    if (to != MSG_TO_GOD)
        return false;

    /* if it's not from a valid ship, ignore */
    if (from != MSG_FROM_SHIP || fromDetail >= cbLimits.maxShips())
        return false;

    tmsg = msg;

    if (tmsg[0] != '/')
        return false;               /* not for us. */

    // erase the leading slash
    tmsg.erase(0, 1);

    /* it is for us, now check for allowability */
    if (!UISOPER(cbShips[fromDetail].unum))
    {                           /* nice try... */
        clbStoreMsg(MSG_FROM_GOD, 0,
                    MSG_TO_SHIP, fromDetail,
                    "You are not a Conquest Operator.");
        utLog("conquestd: EXEC from unprivileged ship: %d, '%s'", fromDetail,
              tmsg.c_str());
        return false;
    }

    /* ok, let's see what is up. */
    if (tmsg.empty())
    {
        // just sent a '/', send usage msg
        clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail, oerror);
        return false;
    }

    /* check commands requiring args (no upper-case) */

    std::string cmd;
    std::vector<std::string> cmdVec = _parseArgs(tmsg, ' ');

#if 0
    for (int i=0; i<cmdVec.size(); i++)
        utLog("JET: vec %d = '%s'", i, cmdVec[i].c_str());
#endif

    if (cmdVec.empty())
    {
        clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail, oerror);
        return false;
    }

    cmd = cmdVec[0];

    /* check the simple commands first - look at the upcase version */

    /* recording */
    if (cmd == "recon")
    {
        startRecord(true);
        return true;
    }

    if (cmd == "recoff")
    {
        stopRecord();
        Context.recmode = RECMODE_OFF;
        return true;
    }

    /* commands requiring arguments */

    /* kill something */
    if (cmd[0] == 'K' || cmd[0] == 'k')
    {                           /* fur is murder! */
        if (cmdVec.size() < 2)
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail,
                        "Usage: /k[ill] <what>");
        else
            Murder(fromDetail, cmdVec[1].c_str());

        return true;
    }

    /* create robots */
    if (cmd[0] == 'R' || cmd[0] == 'r')
    {                           /* little tin men */
        // must have 1 arg, can have 2 or 3 args
        switch(cmdVec.size())
        {
            case 2:
                CreateRobots((int)fromDetail, cmdVec[1].c_str(),
                             NULL, NULL);
                break;
            case 3:
                CreateRobots((int)fromDetail, cmdVec[1].c_str(),
                             cmdVec[2].c_str(), NULL);
                break;
            case 4:
                CreateRobots((int)fromDetail, cmdVec[1].c_str(),
                             cmdVec[2].c_str(), cmdVec[3].c_str());
                break;
            default:
            clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail,
                        "Usage: /r[obot] <username> [<number> <warlike if non-null>]");
            break;
        }

        return true;
    }

    /* that's all we understand for now. */

    clbStoreMsg(MSG_FROM_GOD, 0, MSG_TO_SHIP, fromDetail, oerror);
    return false;
}
