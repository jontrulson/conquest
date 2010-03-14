/* 
 * userauth.c - user stuff
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"
#include "color.h"

#include "userauth.h"

#define MAX_USERLEN 10		/* only 10 chars for users */

/* check the validity of a supplied username */  
int checkuname(char *username)
{
  char *s = username;

  while (*s)
    {
      if (!isalnum(*s) && *s != '-' && *s != '_')
	{			/* if invalid character... */
	  return(FALSE);
	}
      s++;
    }

  return(TRUE);
}


