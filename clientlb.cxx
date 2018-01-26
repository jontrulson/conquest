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


/* put a recieved message into the clinet's copy of the message buffer
   (cbMsgs[]).  Pretty much a copy of clbStoreMsg() without the locking */
void clntStoreMessage(spMessage_t *msg)
{
    int nlastmsg;

    if (!msg)
        return;

    nlastmsg = utModPlusOne( cbConqInfo->lastmsg + 1, cbLimits.maxMsgs() );
    utStrncpy(cbMsgs[nlastmsg].msgbuf, (char *)msg->msg, MESSAGE_SIZE);
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

/* return a static string containing the server's stringified  flags */
char *clntServerFlagsStr(uint32_t flags)
{
    static const int maxFlagsLen = 256;
    static char serverflags[maxFlagsLen];

    if (flags == SERVER_F_NONE)
        utStrncpy(serverflags, "None", maxFlagsLen);
    else
        utStrncpy(serverflags, "", maxFlagsLen);

    if (flags & SERVER_F_REFIT)
        utStrncat(serverflags, "Refit ", maxFlagsLen);

    if (flags & SERVER_F_VACANT)
        utStrncat(serverflags, "Vacant ", maxFlagsLen);

    if (flags & SERVER_F_SLINGSHOT)
        utStrncat(serverflags, "SlingShot ", maxFlagsLen);

    if (flags & SERVER_F_NODOOMSDAY)
        utStrncat(serverflags, "NoDoomsday ", maxFlagsLen);

    if (flags & SERVER_F_KILLBOTS)
        utStrncat(serverflags, "Killbots ", maxFlagsLen);

    if (flags & SERVER_F_SWITCHTEAM)
        utStrncat(serverflags, "SwitchTeam ", maxFlagsLen);

    if (flags & SERVER_F_NOTEAMWAR)
        utStrncat(serverflags, "NoTeamWar ", maxFlagsLen);

    if (flags & SERVER_F_NODRIFT)
        utStrncat(serverflags, "NoDrift ", maxFlagsLen);

    return serverflags;
}
