/* 
 * Message structure
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef MSG_H_INCLUDED
#define MSG_H_INCLUDED

#include "conqdef.h"

#define MSG_FLAGS_NONE       0x00 /* normal message */
#define MSG_FLAGS_FEEDBACK   0x01 /* server feedback - no formatting */
#define MSG_FLAGS_TERSABLE   0x02 /* can be ignored if UserConf.Terse is
				     set */
#define MSG_FLAGS_ROBOT      0x04 /* an insult from a robot */
#define MSG_FLAGS_INTRUDER   0x08 /* intruder alert */

typedef struct {
  int msgfrom;			/* >0: ship the message is from */
				/* =0: from GOD */
				/* <0: planet with intruder alert */
  int msgto;			/* >0: ship the message is to */
				/* =0: to nobody */
				/* <0: team the message is to, and */
				/*     TEAM_SELFRULED means everyone */
  unsigned char flags;		/* some flags. */
  char msgbuf[MESSAGE_SIZE];	/* the message itself */
} Msg_t;

#endif /* MSG_H_INCLUDED */
