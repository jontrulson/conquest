#include "c_defs.h"

/************************************************************************
 *
 * ibuf.c - input buffering to support Function key macros
 * Jon Trulson 3/30/96
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 **********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include "conf.h"
#include "global.h"
#include "defs.h"
#include "ibuf.h"

/* the buffer */
static unsigned int *rp, *wp;
static int ndata;
static unsigned int data[IBUFMAX];

/* iBufInit - intialize the input buffer */

void iBufInit(void)
{
  data[0] = 0;
  rp = wp = data;
  ndata = 0;

  return;
}


int iBufCount(void)
{
  return ndata;
}

static int putRing(int *buf, int len)
{
  int left, wlen = len, i;
  int *rptr = buf;

  left = IBUFMAX - ndata; /* max space available */

  /* a null buf means to simply return the space left */

  if (!rptr)
    return(left);

  if (wlen > left)
    wlen = left;

  if (rptr != NULL)
    {
      for (i=0; i < wlen; i++, rptr++)
        {
          if ( wp >= (data + IBUFMAX) ) 
            wp = data;

          *wp = *rptr;
          ndata++;
	  wp++;
        }
    }

  return(wlen);
}


static int getRing(int *buf, int len, int update)
{
  unsigned int *wptr = buf, *rptr = rp;
  int rlen = len, tlen, numdata = ndata;

  if (rlen > ndata)
    rlen = ndata;

  tlen = rlen;

  while (rlen--)
    {
      if (rptr >= (data + IBUFMAX)) 
        rptr = data;

      if (wptr)                 /* NULL buf doesn't copy data - Pop */
        {
          *wptr = *rptr;
          wptr++;
        }
      rptr++;
      numdata--;
    }

  if (update)
    {
      rp = rptr;
      ndata = numdata;
    }

  return(tlen);
}


/* iBufPut - put a string into the buffer */

void iBufPut(char *thestr)
{
  int i;
  int n = strlen(thestr);
  int idata[IBUFMAX];

  /* cvt to int array */
  for (i=0; i<n; i++)
    idata[i] = thestr[i] & 0xff;

  putRing(idata, n);

  return;
}

/* iBufPutc - put a char into the buffer */

void iBufPutc(unsigned int thechar)
{
  putRing(&thechar, 1);

  return;
}


/* iBufGetCh - return next char from the input buffer */

unsigned int iBufGetCh(void)
{
  static int c;

  if (!iBufCount())
    {
      /*      clog("IBUF GETC EMPTY, returning 0\n");*/
      return('\0');
    }
  getRing(&c, 1, TRUE);

  return(c);
}
  
/* DoMacro - stuff the buffer if an fkey pressed */

int DoMacro(int fkey)
{
  if (fkey < 0 || fkey >= MAX_MACROS)
    return(FALSE);

  iBufPut(UserConf.MacrosF[fkey]);	

#ifdef DEBUG_MACROS
  clog("DoMacro(): got an FKey: %d", ch);
#endif
  
  return(TRUE);

}
      
int DoMouseMacro(int but, Unsgn32 mods, real mangle)
{
  int myangle = ((mangle < 0.0) ? 0 : (int)mod360(mangle));
  char *s;
  static char buf[MAX_MACRO_LEN];

  
  if (but < 0 || but >= CONF_MAXBUTTONS)
    return(FALSE);

  if (mods < 0 || mods >= CONF_MAXMODIFIERS)
    return(FALSE);

  /* we need to translate any occurances of \a into the mangle (angle) */

  s = UserConf.Mouse[but][mods];

#if defined(DEBUG_MACROS)
  clog("DoMouseMacro(): got MOUSE Macro Key: %d, mod %d string = '%s'", but,
       mods, s);
#endif

  while (*s)
    {
      if (*s == '\\')
        {
          s++;

          if (*s == 'a')
            {
              s++;
              snprintf(buf, MAX_MACRO_LEN - 1, "%d",
                       myangle);
              iBufPut(buf); 
            }
          else
            iBufPutc('\\');
        }
      else
        {
          iBufPutc((int)*s);
          s++;
        }
    }

  return(TRUE);
}
      
