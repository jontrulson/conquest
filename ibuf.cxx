//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "c_defs.h"
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

    rbGet(ibufRB, (uint8_t *)&c, I2RB_LEN(1), true);

    return c;
}

/* ibufExpandMacro - stuff the buffer if an fkey pressed */

int ibufExpandMacro(int fkey)
{
    if (fkey < 0 || fkey >= MAX_MACROS)
        return(false);

    ibufPut(UserConf.MacrosF[fkey]);

#ifdef DEBUG_MACROS
    utLog("ibufExpandMacro(): got an FKey: %d", fkey);
#endif

    return(true);

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
        return(false);

    if (mods >= CONF_MAXMODIFIERS)
        return(false);

    /* we need to translate any occurances of \a into the mangle (angle) */

    s = UserConf.Mouse[but][mods];

    /* return false for empty/undeclared macros */
    if (!s || !*s)
        return false;

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
                snprintf(buf, MAX_MACRO_LEN, "%d",
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

    return(true);
}
