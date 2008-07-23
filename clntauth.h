/* 
 * Per client (conquest) info
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef CLNTAUTH_H_INCLUDED
#define CLNTAUTH_H_INCLUDED

#include "conqdef.h"
#include "datatypes.h"
#include "protocol.h"

int  Logon(char *username);
void ChangePassword(int unum, int isoper);

#endif /* CLNTAUTH_H_INCLUDED */
