#include "c_defs.h"
/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

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
