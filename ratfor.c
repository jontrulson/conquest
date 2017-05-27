/************************************************************************
 * Functions that appear to be intrinsic to RATFOR
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "c_defs.h"		/* includes everthing else */
#include "conqutil.h"

/* c_index(char *, char) - get the index of ch in *buf */
int c_index(char *buf, char ch)
{
  char *s;
  int count;

  s = buf;
  count = 0;

  while (*s)
    {
      if (*s == ch)
	return(count);
      s++;
      count++;
    }

  return(-1);			/* No match */
}

/* void lower(char *buf) - lowercase a string */
void lower(char *buf)
{
  char *s;

  s = buf;

  while(*s)
    {
      *s = (char)tolower(*s);
      s++;
    }
}

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

/* load the now[] array with the current time - make sure it's
   decl [NOWSIZE]
   always returns the current time in time_t format.
*/
time_t getnow (int now[NOWSIZE], time_t thetime)
{
  struct tm *thetm;
  time_t thetimet, curtime;

  curtime = time(0);

  if (thetime == 0)
    thetimet = curtime;		/* wants current time */
  else
    thetimet = thetime;		/* wants supplied time conversion */

  if (now != NULL)		/* want to actually fill now[] */
    {
      thetm = localtime(&thetimet);

      now[1] = thetm->tm_year + 1900; /* year */
      now[2] = thetm->tm_mon + 1; /* month */
      now[3] = thetm->tm_mday;	/* day */
      now[4] = thetm->tm_hour;	/* hour */
      now[5] = thetm->tm_min;	/* minutes */
      now[6] = thetm->tm_sec;	/* seconds */
      now[7] = 500;		/* thousandths of seconds - not here */
    }

  return(curtime);
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
