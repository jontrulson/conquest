/* 
 * Per client (conquest) info
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef CLIENTLB_H_INCLUDED
#define CLIENTLB_H_INCLUDED

#include "conqdef.h"
#include "datatypes.h"
#include "protocol.h"

void  clntStoreMessage(spMessage_t *msg);
void  clntDisplayFeedback(char *msg);
char *clntServerFlagsStr(Unsgn32 flags);

#endif /* CLIENTLB_H_INCLUDED */
