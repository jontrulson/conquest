/*
 * server command handling
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _SERVERCMD_H
#define _SERVERCMD_H

#include "conqdef.h"
#include "conqnet.h"

#include "protocol.h"
#include "packet.h"
#include "record.h"

int checkOperExec(int from, int to, char *msg);

#endif /* _SERVERCMD_H */
