#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/


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
