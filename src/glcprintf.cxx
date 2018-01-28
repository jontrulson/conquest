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
#include "cb.h"
#include "ibuf.h"
#include "gldisplay.h"
#include "conf.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "glmisc.h"
#include "glfont.h"

static char buf[BUFFER_SIZE_1024];

/*
 * int lin - line if pertinent
 * int col - column if pertinent
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
    int l;
    GLfloat x, y, w;

    va_start(ap, fmt);
    (void)vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    if (DSP_INITED())
    {
        l = uiCStrlen(buf);
        if (align == ALIGN_CENTER)
            x = dConf.ppCol * ((80.0 / 2.0) - ((GLfloat)l / 2.0));
        else
            x = dConf.ppCol * (GLfloat)col;

        y = dConf.ppRow * (GLfloat)lin;
        w = dConf.ppCol * (GLfloat)l;

        glfRenderFont(x + dConf.wBorderW,
                      y + dConf.wBorderW,
                      0.0,
                      w, dConf.ppRow,
                      glfFontFixed, buf,
                      NoColor, NULL,
                      GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);
    }

    return;
}
