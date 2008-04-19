#include "c_defs.h"

/************************************************************************
 *
 * client specific stuff
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "global.h"
#include "conf.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"
#include "client.h"
#include "clientlb.h"
#include "conqcom.h"
#include "context.h"
#include "record.h"
#include "disputil.h"


/* put a recieved message into the clinet's copy of the message buffer
   (Msgs[]).  Pretty much a copy of clbStoreMsg() without the locking */
void clntStoreMessage(spMessage_t *msg)
{
  int nlastmsg;

  if (!msg)
    return;

  nlastmsg = modp1( ConqInfo->lastmsg + 1, MAXMESSAGES );
  strncpy(Msgs[nlastmsg].msgbuf, (char *)msg->msg, MESSAGE_SIZE);
  Msgs[nlastmsg].msgfrom = (int)((Sgn16)ntohs(msg->from));
  Msgs[nlastmsg].msgto = (int)((Sgn16)ntohs(msg->to));
  Msgs[nlastmsg].flags = msg->flags;
  ConqInfo->lastmsg = nlastmsg;

  /* Remove allowable last message restrictions. */
  Ships[Context.snum].alastmsg = LMSG_READALL;

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
char *clntServerFlagsStr(Unsgn32 flags)
{
  static char serverflags[256];

  if (flags == SPSSTAT_FLAGS_NONE)
    strcpy(serverflags, "None");
  else
    strcpy(serverflags, "");

  if (flags & SPSSTAT_FLAGS_REFIT)
    strcat(serverflags, "Refit ");
  
  if (flags & SPSSTAT_FLAGS_VACANT)
    strcat(serverflags, "Vacant ");
  
  if (flags & SPSSTAT_FLAGS_SLINGSHOT)
    strcat(serverflags, "SlingShot ");
  
  if (flags & SPSSTAT_FLAGS_NODOOMSDAY)
    strcat(serverflags, "NoDoomsday ");
  
  if (flags & SPSSTAT_FLAGS_KILLBOTS)
    strcat(serverflags, "Killbots ");
  
  if (flags & SPSSTAT_FLAGS_SWITCHTEAM)
    strcat(serverflags, "SwitchTeam ");

  return serverflags;
}
  


