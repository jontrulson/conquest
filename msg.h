/* 
 * Message structure
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef MSG_H_INCLUDED
#define MSG_H_INCLUDED

#include "conqdef.h"

typedef struct {
  char msgbuf[MESSAGE_SIZE];	/* the message itself */
  int msgfrom;			/* >0: ship the message is from */
				/* =0: from GOD */
				/* <0: planet with intruder alert */
  int msgto;			/* >0: ship the message is to */
				/* =0: to nobody */
				/* <0: team the message is to, and */
				/*     TEAM_SELFRULED means everyone */
} Msg_t;

#endif /* MSG_H_INCLUDED */
