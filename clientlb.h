/*
 * Per client (conquest) info
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef CLIENTLB_H_INCLUDED
#define CLIENTLB_H_INCLUDED

#include "conqdef.h"

#include "protocol.h"

void  clntStoreMessage(spMessage_t *msg);
void  clntDisplayFeedback(char *msg);
char *clntServerFlagsStr(uint32_t flags);

#endif /* CLIENTLB_H_INCLUDED */
