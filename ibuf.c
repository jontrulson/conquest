#include "c_defs.h"

/************************************************************************
 *
 * ibuf.c - input buffering to support Function key macros
 * Jon Trulson 3/30/96
 *
 * $Id$
 *
 **********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include "global.h"
#include "defs.h"
#include "ibuf.h"

/* iBufInit - intialize the input buffer */

void iBufInit(void)
{
  iBufOffset = -1;

  iBuffer[0] = EOS;

  return;
}


/* iBufEmpty - return TRUE or FALSE depending on whether the buffer is 
 * empty 
 */

int iBufEmpty(void)
{
  if (iBufOffset == -1)
    return(TRUE);
  else
    return(FALSE);
}

/* iBufPut - put a string into the buffer */

void iBufPut(char *thestr)
{
  int i, j, k;

  i = iBufOffset + 1;

  j = strlen(thestr);

  if (j > 0)
    {
      for (k=(j - 1); k >= 0 && i < IBUFMAX; k--)
	{
	  iBuffer[i] = thestr[k];	/* copy it in backwards */
	  i++;
	}
      
      iBufOffset = i - 1;
    }

  return;
}

/* iBufPutc - put a char into the buffer */

void iBufPutc(char thechar)
{
  int i;

  i = iBufOffset + 1;

  if (i < IBUFMAX)
    {
      iBuffer[i] = thechar;
      iBufOffset++;
    }

  return;
}


/* iBufGetCh - return next char from the input buffer */

char iBufGetCh(void)
{
  static char c;

  if (iBufEmpty() == TRUE)
    return('\0');

  c = iBuffer[iBufOffset--];

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

  iBufPut(conf_MacrosF[fkey]);	

#ifdef DEBUG_MACROS
  clog("DoMacro(): got an FKey: %d", ch);
#endif
  
  return(TRUE);

}
      
