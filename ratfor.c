/************************************************************************
 * Functions that appear to be intrinsic to RATFOR
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "c_defs.h"		/* includes everthing else */
#include "conqutil.h"

/* void upper(char *buf) - upcase a string */
void upper(char *buf)
{
  char *s;

  s = buf;

  while(*s)
    {
      *s = (char)toupper(*s);
      s++;
    }
}
