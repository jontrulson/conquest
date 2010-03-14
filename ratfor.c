/* Functions that appear to be intrinsic to RATFOR */
/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/


#include "c_defs.h"		/* includes everthing else */
#include "conqutil.h"

/* alldig(char *) - returns YES if all chars in str are digits, else NO */
int alldig(char *buf)
{
  char *s;
  
  s = buf;
  while (*s)
    {
      if (!isdigit(*s))
	return(FALSE);
      s++;
    }
  
  return(TRUE);
}

/* c_type(char) - returns LETTER or DIGIT if letter or digit, returns ERR 
   otherwise */
int c_type(char c)
{

  if (isdigit(c))
    return(DIGIT);
  else if (isalpha(c))
    return(LETTER);

  return(ERR);
}

/* void concat(char str1[], char str2[], char buf[]) - concatenate str1 
   and str2 into buf */
void concat(char str1[], char str2[], char buf[])
{
  strcat(buf, str1);
  strcat(buf, str2);
}

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

  return(ERR);			/* No match */
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
    
/* Find first bit that is set and return bit number */
int lib_ffs(int start, int len, int bits, int *rule)
{
  register int i;

  *rule = -1;
  for (i = start; i < len; i++)
    {				/* check the bits */
      if ((bits & ( 1 << i)) != 0)
	{			/* bit is set */
	  *rule = i;
	  break;
	}
    }

#ifdef DEBUG_AI
    utLog("bits = %8X, rule = %d", bits, *rule);
#endif

  if (*rule == -1)
    return(ERR);
  else
    return(OK);
}

/* sleep with a millisecond granularity using poll() */
void c_sleep(real sleeptime)
{
#ifdef USE_SELECT

  unsigned int secs, usecs;
  struct timeval stime;


  if (sleeptime == 0.0)
    return;

				/* some systems don't grok usecs >1,000,000 */
  if (sleeptime >= 1.0)
    {
      secs = (int) sleeptime;
      usecs = (int) (1000000 * (sleeptime - (real) secs));
    }
  else
    {
      secs = 0;
      usecs = (1000000 * sleeptime);
    }
  
  stime.tv_sec = secs;
  stime.tv_usec  = usecs;
  
# if defined(LINUX)
  while (1)
  {
# endif

  if (select(0, NULL, NULL, NULL, &stime) == -1)
    {
      if (errno != EINTR) /* Interrupted system call */
           utLog("c_sleep(): select() failed: errno = %d, %s",
                errno,strerror(errno));
    }

# if defined(LINUX)
  if (stime.tv_sec == 0 && stime.tv_usec == 0)
    break;
  } /* end while */
# endif

#else

  if (sleeptime == 0.0)
    return;

  /* For windows (and eventually, maybe everyone else, use usleep */
#if defined(MINGW)
  usleep((useconds_t)sleeptime * 1000000);
#else

  /* Here I use poll to provide a msec */
  /* granularity sleep.  Actually it's */
  /* limited to 0.1 sec, but that's close */
  /* enough.  I don't know if it's wise */
  /* to supply a NULL poll struct, but it seems to work... */
    
  if (poll(NULL, 0, (int) (sleeptime * 1000)) == -1)
    {
      if (errno != EINTR) /* Interrupted system call */
	utLog("csleep(): poll() failed: %s",
	     strerror(errno));
    }
#endif  /* MINGW */

#endif

  return;

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



