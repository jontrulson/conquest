/* misc gl funcs
 *
 * Jon Trulson, 2/2004
 *
 * $Id$
 *
 * Copyright 2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "conqdef.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "conqcom.h"
#include "ibuf.h"
#include "gldisplay.h"
#include "conf.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "glfont.h"
#include "glmisc.h"
#include "gldisplay.h"

void _GLError(char *funcname, int line)
{
  int i;

  while ((i = glGetError()) != GL_NO_ERROR)
    clog("GL ERROR: %s@%d: %s\n",
	 funcname, line, gluErrorString(i));

  return;
}

void mglConqLogo(dspConfig_t *dsp)
{
  static char *Conquest = "CONQUEST";
  GLfloat x, y, w, h;

  h = dsp->ppRow * 5.0;
  w = (dsp->wW * 0.70);

  x = dsp->wW * 0.15;
  y = dsp->ppRow;

  glfRender(x, y, w, h, fontLargeTxf, Conquest, RedLevelColor, 
            TRUE, FALSE);

  drawLineBox(x, y + (h / 20.0),
              w,
              h,
              NoColor,
              1.0);

  return;
}

/* this is cheesy, but glut does not provide a bell. */
void mglBeep(void)
{
  static Display *dpy = NULL;
  time_t i;
  static time_t old = 0;

  if (dpy == NULL)
    dpy = XOpenDisplay(NULL);

  i = time(0);

  if (i != old)
    {
      old = i;
      if (dpy)
        {
          XBell(dpy, 0);
          XFlush(dpy);
        }
    }

  return;
}
