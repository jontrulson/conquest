#include "c_defs.h"

/************************************************************************
 *
 * server command handling
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "conf.h"
#include "server.h"
#include "serverpkt.h"
#include "context.h"
#include "record.h"
#include "servercmd.h"
#include "conqlb.h"

void startRecord(void)
{
  char fname[MID_BUFFER_SIZE];
  char bname[MID_BUFFER_SIZE];
  char cbuf[MID_BUFFER_SIZE];
  time_t thetime;
  struct tm *tmtime;
  char tbuf[128];
  
  if (Context.recmode == RECMODE_ON)
    {
      clog("conquestd: startRecord: already recording.");
      clbStoreMsg(MSG_GOD, Context.snum, "We are already recording.");
      return;
    }

  /* make a filename.  This will be something like
     CONQSTATE/conquest-rec-MMDDYYYY-HHMM.cpr */

  thetime = getnow(NULL, 0);
  tmtime = localtime(&thetime);

  strftime(tbuf, 128 - 1, "%m%d%Y-%H%M", tmtime);

  sprintf(bname, "conquest-rec-%s.cqr", tbuf);

  sprintf(fname, "%s/%s", CONQSTATE, bname);

  if (recordOpenOutput(fname, TRUE))
    {                     /* we are almost ready... */
      Context.recmode = RECMODE_STARTING;
    }
  else
    {
      Context.recmode = RECMODE_OFF;
      clog("conquestd: Cannot record to %s", fname);
      sprintf(cbuf, "Cannot record to %s", bname);
      clbStoreMsg(MSG_GOD, Context.snum, cbuf);
      return;
    }

  /* start recording */
  if (Context.recmode == RECMODE_STARTING)
    {
      if (recordInitOutput(Context.unum, getnow(NULL, 0), 0))
        {
          Context.recmode = RECMODE_ON;
          clog("conquestd: Recording to %s", fname);
          sprintf(cbuf, "Recording to %s", bname);
          clbStoreMsg(MSG_GOD, Context.snum, cbuf);
        }
      else
        {
          Context.recmode = RECMODE_OFF;
          clog("conquestd: recordInitOutput failed");
          clbStoreMsg(MSG_GOD, Context.snum, 
                  "conquestd: recordInitOutput failed");
        }
    }

  return;
}

void stopRecord(void)
{
  if (Context.recmode != RECMODE_ON)
    {
      clog("conquestd: stopRecord: not recording.");
      clbStoreMsg(MSG_GOD, Context.snum, "We aren't recording.");
      return;
    }


  clog("conquestd: stopRecord: recording stopped");
  clbStoreMsg(MSG_GOD, Context.snum, "Recording stopped");
  recordCloseOutput();

  return;
}

int checkOperExec(int from, int to, char *msg)
{
  char tmsg[MESSAGE_SIZE];
  char *p;
  char *oerror = "Non sequiter. Your facts are uncoordinated.";

  /* first, if the message isn't to GOD, ignore */
  if (to != MSG_GOD)
    return FALSE;

  /* if it's not from a valid ship, ignore */
  if (from < 1 || from > MAXSHIPS)
    return FALSE;

  strncpy(tmsg, msg, MESSAGE_SIZE);
  upper(tmsg);

  if (strncmp(tmsg, "EXEC ", 5))
    return FALSE;               /* not for us. */

  /* it is for us, now check for allowability */
  if (!Users[Ships[from].unum].ooptions[OOPT_OPER])
    {                           /* nice try... */
      clbStoreMsg(MSG_GOD, from, "You are not a Conquest Operator.");
      clog("conquestd: EXEC from unprivileged ship: %d, '%s'", from,
           tmsg);
      return FALSE;
    }

  /* ok, let's see what is up. */
  p = &(tmsg[5]);

  if (!*p)
    {
      clbStoreMsg(MSG_GOD, from, oerror);
      return FALSE;
    }

  if (!strncmp(p, "RECON", 5))
    {
      startRecord();
      return TRUE;
    }

  if (!strncmp(p, "RECOFF", 6))
    {
      stopRecord();
      Context.recmode = RECMODE_OFF;
      return TRUE;
    }

  /* that's all we understand for now. */

  clbStoreMsg(MSG_GOD, from, oerror);
  return FALSE;
}
