/* 
 * common auth functions/defines
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef USERAUTH_H_INCLUDED
#define USERAUTH_H_INCLUDED

#include "conqdef.h"

#define MAX_USERLEN 10		/* only 10 chars for users */

int checkuname(char *username);

#endif /* USERAUTH_H_INCLUDED */
