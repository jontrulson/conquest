#include "c_defs.h"

/************************************************************************
 *
 * ibuf.c - input buffering to support Function key macros
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 **********************************************************************/

#include "conf.h"
#include "global.h"
#include "defs.h"
#include "ibuf.h"
#include "rb.h"
#include "conqutil.h"

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

/* size of input ringbuffer (in integers) */
#define IBUF_RBMAX 1024


/* ibufInit - intialize the input buffer */

void ibufInit(void)
{
  ibufRB = rbCreate(I2RB_LEN(IBUF_RBMAX));

  return;
}

void ibufFlush(void)
{
  rbFlush(ibufRB);

  return;
}


int ibufCount(void)
{
  if (ibufRB)
    return RB2I_LEN(rbBytesUsed(ibufRB));
  else
    return 0;
}


/* ibufPut - put a string into the buffer */

void ibufPut(char *thestr)
{
  int i;
  int n;
  int c;

  if (!thestr)
    return;

  n = strlen(thestr);

  /* cvt to int's and insert into rb */
  for (i=0; i<n; i++)
    {
      c = thestr[i] & 0xff;
      /* hopefully there is enough room, or... */
      rbPut(ibufRB, (uint8_t *)&c, I2RB_LEN(1));
    }

  return;
}

/* ibufPutc - put a char into the buffer */

void ibufPutc(unsigned int thechar)
{
  rbPut(ibufRB, (uint8_t *)&thechar, I2RB_LEN(1));

  return;
}


/* ibufGetc - return next char from the input buffer */

unsigned int ibufGetc(void)
{
  int c;

  if (!ibufCount())
    {
      return 0;
    }

  rbGet(ibufRB, (uint8_t *)&c, I2RB_LEN(1), TRUE);

  return c;
}
  
/* ibufExpandMacro - stuff the buffer if an fkey pressed */

int ibufExpandMacro(int fkey)
{
  if (fkey < 0 || fkey >= MAX_MACROS)
    return(FALSE);

  ibufPut(UserConf.MacrosF[fkey]);	

#ifdef DEBUG_MACROS
  utLog("ibufExpandMacro(): got an FKey: %d", fkey);
#endif
  
  return(TRUE);

}
      
int ibufExpandMouseMacro(int but, uint32_t mods, real mangle)
{
  int myangle = ((mangle < 0.0) ? 0 : (int)utMod360(mangle));
  char *s;
  static char buf[MAX_MACRO_LEN];

#if defined(DEBUG_MACROS)
  utLog("ibufExpandMouseMacro(): mod %08x but %d", mods, but);
#endif
  
  if (but < 0 || but >= CONF_MAXBUTTONS)
    return(FALSE);

  if (mods < 0 || mods >= CONF_MAXMODIFIERS)
    return(FALSE);

  /* we need to translate any occurances of \a into the mangle (angle) */

  s = UserConf.Mouse[but][mods];

  /* return FALSE for empty/undeclared macros */
  if (!s || !*s)
    return FALSE;

#if defined(DEBUG_MACROS)
  utLog("ibufExpandMouseMacro(): got MOUSE Macro Key: %d, mod %d string = '%s'", but,
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
              ibufPut(buf); 
            }
          else
            ibufPutc('\\');
        }
      else
        {
          ibufPutc((int)*s);
          s++;
        }
    }

  return(TRUE);
}
      
