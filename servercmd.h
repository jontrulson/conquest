/* 
 * server command handling
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _SERVERCMD_H
#define _SERVERCMD_H

#include "conqdef.h"
#include "conqnet.h"
#include "datatypes.h"
#include "protocol.h"
#include "packet.h"
#include "record.h"

int checkOperExec(int from, int to, char *msg);
void startRecord(void);
void stopRecord(void);

#endif /* _SERVERCMD_H */
