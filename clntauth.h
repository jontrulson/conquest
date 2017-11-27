/*
 * Per client (conquest) info
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef CLNTAUTH_H_INCLUDED
#define CLNTAUTH_H_INCLUDED

#include "conqdef.h"

#include "protocol.h"

int  Logon(char *username);
void ChangePassword(int unum, int isoper);

#endif /* CLNTAUTH_H_INCLUDED */
