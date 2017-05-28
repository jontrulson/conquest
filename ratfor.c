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

/* wkday(int, int, int) - all args ignored - returns day num */
int wkday (void)
{
  struct tm *thetm;
  time_t thetimet;

  thetimet = time(0);
  thetm = localtime(&thetimet);

  return(thetm->tm_wday + 1);
}

				/* output an error and exit */
void error(char str[])
{
  printf("%s\n", str);
  fflush(stdout);

  exit(1);
}

real ctor(char *buf)
{
  static real r;
  float f;

  sscanf(buf, "%f", &f);

  r = (real) f;
  return(r);
}
