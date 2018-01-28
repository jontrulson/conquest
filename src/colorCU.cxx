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
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"
#include "ui.h"


#define COL_BACKGROUND 1
#define COL_REDBLACK  2
#define COL_GREENBLACK 3
#define COL_YELLOWBLACK 4
#define COL_MAGENTABLACK 5
#define COL_CYANBLACK 6
#define COL_BLUEBLACK 7

void uiPutColor(cqColor color)
{
    int attrib = 0;
    int col;

    /* convert a 'conquest' color into something more useful in curses */

    if (color & CQC_A_BOLD)
        attrib |= A_BOLD;

    if (color & CQC_A_REVERSE)
        attrib |= A_REVERSE;

    if (color & CQC_A_BLINK)
        attrib |= A_BLINK;

    col = color & CQC_FG_MASK;

    if (!Context.hascolor)
        col = 0;

    if (col == NoColor)         /* white */
        attrset(0 | attrib);
    else if (col == RedColor)   /* red */
        attrset(COLOR_PAIR(COL_REDBLACK) | attrib);
    else if (col == GreenColor) /* green */
        attrset(COLOR_PAIR(COL_GREENBLACK) | attrib);
    else if (col == BlueColor)  /* blue */
        attrset(COLOR_PAIR(COL_BLUEBLACK) | attrib);
    else if (col == YellowColor) /* yellow */
        attrset(COLOR_PAIR(COL_YELLOWBLACK) | attrib);
    else if (col == CyanColor)  /* cyan */
        attrset(COLOR_PAIR(COL_CYANBLACK) | attrib);
    else if (col == MagentaColor) /* magenta */
        attrset(COLOR_PAIR(COL_MAGENTABLACK) | attrib);
    else if (col == BlackColor) /* black */
        attrset(0 | attrib);
    else                          /* unknown */
        attrset(0 | attrib);

    return;
}


/* initialize color variables.  assumes curses has been initialized. */
void uiInitColors(void)
{
    if (has_colors())
        Context.hascolor = true;
    else
        Context.hascolor = false;

    if (Context.hascolor)
    {
#ifdef DEBUG_COLOR
        utLog("Terminal has colors, COLORS = %d, COLOR_PAIRS = %d",
              COLORS,
              COLOR_PAIRS);
#endif

        init_pair(COL_BACKGROUND, COLOR_WHITE, COLOR_BLACK);
        init_pair(COL_REDBLACK, COLOR_RED, COLOR_BLACK);
        init_pair(COL_GREENBLACK, COLOR_GREEN, COLOR_BLACK);
        init_pair(COL_YELLOWBLACK, COLOR_YELLOW, COLOR_BLACK);
        init_pair(COL_BLUEBLACK, COLOR_BLUE, COLOR_BLACK);
        init_pair(COL_MAGENTABLACK, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(COL_CYANBLACK, COLOR_CYAN, COLOR_BLACK);

        /* now init the backgound */
        /* ncurses seems to toast the colors if you do this */
        /*  so no background init is done with ncurses. */
        /* This means that if your using ncurses, run conquest */
        /*  on a display with a black background for best results */
#if !defined(HAVE_NCURSES_H) && !defined(NCURSES_VERSION)
        bkgdset(COLOR_PAIR(COL_BACKGROUND) | ' ');
#endif
    }

    return;
}
