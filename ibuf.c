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


/* iBufEmpty - return TRUE or FALSE depending on whether the buffer is 
 * empty 
 */

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
      clog("IBUF GETC EMPTY, returning 0\n");
      return('\0');
    }
  getRing(&c, 1, TRUE);

  return(c);
}
  
/* DoMacro - stuff the buffer if an fkey pressed */

int DoMacro(int ch)
{
  int fkey;
  
  fkey = (ch - KEY_F(0)) - 1 ;

  if (fkey < 0 || fkey >= MAX_MACROS)
  {
#ifdef DEBUG_MACROS
    
    clog("DoMacro(): KEY_F(0) = %d, KEY_F(MAX_MACROS) = %d ch = %d, fkey = %d", 
	 KEY_F(0), KEY_F(MAX_MACROS), ch, fkey);
    clog("DoMacro(): got invalid Fkey: %d", ch);
#endif

    return(FALSE);
  }

  iBufPut(UserConf.MacrosF[fkey]);	

#ifdef DEBUG_MACROS
  clog("DoMacro(): got an FKey: %d", ch);
#endif
  
  return(TRUE);

}
      
