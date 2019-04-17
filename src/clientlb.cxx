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
#include "global.h"
#include "conf.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"
#include "client.h"
#include "clientlb.h"
#include "cb.h"
#include "context.h"
#include "record.h"
#include "disputil.h"
#include "conqutil.h"
#include "cprintf.h"


/* put a recieved message into the clinet's copy of the message buffer
   (cbMsgs[]).  Pretty much a copy of clbStoreMsg() without the locking */
void clntStoreMessage(spMessage_t *msg)
{
    int nlastmsg;

    if (!msg)
        return;

    nlastmsg = mod( cbConqInfo->lastmsg + 1, cbLimits.maxMsgs() );
    utStrncpy(cbMsgs[nlastmsg].msgbuf, (char *)msg->msg, MAX_MSGSIZE);
    cbMsgs[nlastmsg].from = static_cast<msgFrom_t>(msg->from);
    cbMsgs[nlastmsg].fromDetail = msg->fromDetail;
    cbMsgs[nlastmsg].to = static_cast<msgTo_t>(msg->to);
    cbMsgs[nlastmsg].toDetail = msg->toDetail;
    cbMsgs[nlastmsg].flags = msg->flags;

    cbConqInfo->lastmsg = nlastmsg;

    /* Remove allowable last message restrictions. */
    cbShips[Context.snum].alastmsg = LMSG_READALL;

    return;
}


/* feedback messages are sent by the server using spMessage_t's like
   normal messages.  However, these messages are displayed immediately,
   as well as being displayed on MSG_LIN1 */
void clntDisplayFeedback(char *msg)
{
    if (!msg)
        return;

    displayFeedback(msg, MSG_LIN1);

    return;
}

/* return a static string containing the server's stringified flags */
const std::vector<std::string>& clntServerFlagsStr(uint32_t flags)
{
    static std::vector<std::string> flagsList;
    static uint32_t savedFlags = ~0;

    if (flags != savedFlags)
    {
        savedFlags = flags;
        flagsList.clear();

        if (flags == SERVER_F_NONE)
            flagsList.push_back(std::string("None"));

        if (flags & SERVER_F_REFIT)
            flagsList.push_back(std::string("Refit"));

        if (flags & SERVER_F_VACANT)
            flagsList.push_back(std::string("Vacant"));

        if (flags & SERVER_F_SLINGSHOT)
            flagsList.push_back(std::string("SlingShot"));

        if (flags & SERVER_F_NODOOMSDAY)
            flagsList.push_back(std::string("NoDoomsday"));

        if (flags & SERVER_F_KILLBOTS)
            flagsList.push_back(std::string("Killbots"));

        if (flags & SERVER_F_SWITCHTEAM)
            flagsList.push_back(std::string("SwitchTeam"));

        if (flags & SERVER_F_NOTEAMWAR)
            flagsList.push_back(std::string("NoTeamWar"));

        if (flags & SERVER_F_NODRIFT)
            flagsList.push_back(std::string("NoDrift"));

        if (flags & SERVER_F_CLOSED)
            flagsList.push_back(std::string("Closed"));
    }

    return flagsList;
}

// Print out the server's flags.  Return the possibly modified line
// increment.
int clntPrintServerFlags(int lin, int col, uint32_t flags, int color)
{
    int mycol = col;
    const std::vector<std::string>& flagsList =
        clntServerFlagsStr(flags);

    for (int i=0; i<flagsList.size(); i++)
    {
        cprintf(lin, mycol, ALIGN_NONE, "#%d#%s", color,
                flagsList[i].c_str());
        // include the space separator
        mycol += flagsList[i].length() + 1;

        // check next flag to see if it would fit
        if (i + 1 < flagsList.size())
        {
            if (mycol + (flagsList[i + 1].length() + 1) >= Context.maxcol)
            {
                // start a new line
                lin++;
                mycol = col;
            }

        }
    }

    return lin;
}
