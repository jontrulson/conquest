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
#include "color.h"		/* instantiate externs here */
#include "ui.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

void uiPutColor(cqColor color)
{
    GLfloat base = 0.7;
    GLfloat alpha = color & CQC_ALPHA_MASK;

    if (!alpha)
        alpha = 1.0;
    else
        alpha = (1.0 / 256.0) *
            (GLfloat)((color & CQC_ALPHA_MASK) >> CQC_ALPHA_SHIFT);

    /* convert a 'conquest' color into something more useful in GL :) */

    if (color & CQC_A_BOLD)
        base += 0.2;

    if (color & CQC_A_DIM)
        base -= 0.2;

    color &= CQC_FG_MASK;         /* strip everything but color info */

    if (color == NoColor)         /* white */
        glColor4f(base, base, base, alpha);
    else if (color == RedColor)   /* red */
        glColor4f(base, 0.0, 0.0, alpha);
    else if (color == GreenColor) /* green */
        glColor4f(0.0, base, 0.0, alpha);
    else if (color == BlueColor)  /* blue */
        glColor4f(0.0, 0.0, base, alpha);
    else if (color == YellowColor) /* yellow */
        glColor4f(base, base, 0.0, alpha);
    else if (color == CyanColor)  /* cyan */
        glColor4f(0.0, base, base, alpha);
    else if (color == MagentaColor) /* magenta */
        glColor4f(base, 0.0, base, alpha);
    else if (color == BlackColor) /* black */
        glColor4f(0.0, 0.0, 0.0, alpha);
    else                          /* unknown */
        glColor4f(1.0, 1.0, 1.0, alpha);

    return;
}

/* initialize color variables. */
void uiInitColors(void)
{
    /* nothing to do here - we translate in uiPutColor */
    return;

}
