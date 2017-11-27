/*
 * server command handling
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _SERVERCMD_H
#define _SERVERCMD_H

#include "conqdef.h"
#include "conqnet.h"

#include "protocol.h"
#include "packet.h"
#include "record.h"

int checkOperExec(msgFrom_t from, uint16_t fromDetail,
                  msgTo_t to, uint16_t toDetail, char *msg);

#endif /* _SERVERCMD_H */
