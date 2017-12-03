#include "c_defs.h"
/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

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
void cprintf(int lin, int col, int align, char *fmt, ...)
{
    va_list ap;
    register int i, j, k;
    int len, vcol;
    int color_num;
    int two_passes = FALSE;
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
        two_passes = TRUE;
    while (TRUE)
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
            two_passes = FALSE;
	}
        else
            break; /* one pass and we're done. */

    } /* end while (TRUE) */

    uiPutColor(0);

    return;

}