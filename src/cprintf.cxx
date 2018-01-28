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
#include "conqdef.h"
#include "global.h"
#include "context.h"
#include "color.h"
#include "cd2lb.h"
#include "cb.h"
#include "ibuf.h"
#include "conf.h"
#include "ui.h"

/*
 * int lin - line num if pertinent
 * int col - line num if pertinent
 * int align - ALIGN_CENTER 3
 *             ALIGN_RIGHT  2
 *             ALIGN_LEFT   1
 *             ALIGN_NONE   0
 * char *fmt - modified fmt string with color codes embedded
 *             '#' color code
 *             '%' regular format specs
 * ...       - args
 */
void cprintf(int lin, int col, int align, const char *fmt, ...)
{
    va_list ap;
    int i, j, k;
    int len, vcol;
    int color_num;
    int two_passes = false;
    static char buf[BUFFER_SIZE_1024] = {};
    static char xbuf[BUFFER_SIZE_1024] = {};
    static char color_code[BUFFER_SIZE_256] = {};

    vcol = col;
    buf[0] = 0;
    xbuf[0] = 0;
    color_code[0] = 0;

    /* use vsprintf */
    va_start(ap, fmt);
    (void)vsnprintf(buf, BUFFER_SIZE_1024, fmt, ap);
    va_end(ap);
    /* end of it */

    if ( align == ALIGN_CENTER )
        two_passes = true;
    while (true)
    {
        i=0;j=0;
        xbuf[0]=0;
        while (buf[i] != 0 && i < BUFFER_SIZE_1024)
	{
            if ( buf[i] == '#' )   /* color code warning */
	    {
                if ( buf[i+1] == '#' )
		{
                    xbuf[j++] = buf[i++]; /* escape'd # */
                    continue;
		}
                /* test for phrase to output */

                xbuf[j] = 0;
                if ( !two_passes )
                    if ( strlen(xbuf) > 0 && i > 0 )
                    {
                        cdputs(xbuf, lin, vcol);        /* if have something to */
                        vcol += strlen(xbuf);           /* display then do so */
                        j=0;
                    }

                k=0;
                color_code[0] = 0;
                if ( buf[i] == '#' )
                    i++;                /* advance past '#' */
                /* get color code */

                while ( buf[i] != 0 && isdigit(buf[i]) )
		{
                    if ( !two_passes )
		    {
                        color_code[k++] = buf[i++];
                        color_code[k] = 0;
		    }
                    else
                        i++;
		}

                if ( buf[i] == '#' )
                    i++;                /* advance past '#' */
                /* have color code, convert to int and set the color attribute */
                if ( !two_passes )
		{
                    color_num = atoi(color_code);
                    uiPutColor(color_num);
		}
	    } /* end if color code start*/
            xbuf[j++] = buf[i++];  /* save char to output xbuf */
	} /* end outer while */
        xbuf[j] = 0;

        len = strlen(xbuf);
        /* display the last part of the line. */
        if ( !two_passes )
            if ( len > 0 && i > 0 )
                cdputs(xbuf, lin, vcol);

        /* calculate where the complete line should start for centered text */
        if ( two_passes )
	{
            vcol = ((Context.maxcol / 2) - (len / 2));
            two_passes = false;
	}
        else
            break; /* one pass and we're done. */

    } /* end while (true) */

    uiPutColor(0);

    return;

}
