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
#include "rb.h"

/* the input ringbuffer */
static ringBuffer_t *ibufRB = NULL;

/* we use the ringbuffer stuff in rb.c, however, the input
 *  buffer is really a buffer of integers, so we hide all that here, since
 *  the RB itself is byte oriented.
 */

/* convert the lengths into the 'real' lengths between the length we
 *  care about here (integers) vs the RB length (bytes).
 */
#define I2RB_LEN(x)       ((x) * sizeof(unsigned int))
#define RB2I_LEN(x)       ((x) / sizeof(unsigned int))

/* size of input ringbuffer */
#define IBUF_RBMAX 1024


/* iBufInit - intialize the input buffer */

void iBufInit(void)
{
  ibufRB = rbCreate(I2RB_LEN(IBUF_RBMAX));

  return;
}


int iBufCount(void)
{
  if (ibufRB)
    return RB2I_LEN(ibufRB->ndata);
  else
    return 0;
}


/* iBufPut - put a string into the buffer */

void iBufPut(char *thestr)
{
  int i;
  int n = strlen(thestr);
  int c;

  /* cvt to int's and insert into rb */
  for (i=0; i<n; i++)
    {
      c = thestr[i] & 0xff;
      /* hopefully there is enough room, or... */
      rbPut(ibufRB, (Unsgn8 *)&c, I2RB_LEN(1));
    }

  return;
}

/* iBufPutc - put a char into the buffer */

void iBufPutc(unsigned int thechar)
{
  rbPut(ibufRB, (Unsgn8 *)&thechar, I2RB_LEN(1));

  return;
}


/* iBufGetCh - return next char from the input buffer */

unsigned int iBufGetCh(void)
{
  int c;

  if (!iBufCount())
    {
      return 0;
    }

  rbGet(ibufRB, (Unsgn8 *)&c, I2RB_LEN(1), TRUE);

  return c;
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
      
