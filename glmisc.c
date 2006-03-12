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
#include "node.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "ui.h"

#include "glfont.h"
#include "glmisc.h"
#include "gldisplay.h"
#include "textures.h"

/*
 Cataboligne - multi lingual giant conquest title
*/

#define TITLE(x)  100 + x


int _GLError(const char *funcname, const char *filename, int line)
{
  int i;
  int rv = FALSE;

  while ((i = glGetError()) != GL_NO_ERROR)
    {
      clog("GL ERROR: %s:%s@%d: %s\n",
           filename, funcname, line, gluErrorString(i));
      rv = 1;
    }

  return rv;
}

void mglConqLogo(dspConfig_t *dsp, bool mult)
{
  static char *Conquest = "CONQUEST";
  GLfloat x, y, w, h;
  static int firstime = TRUE;
  static GLint logos[NUMPLAYERTEAMS]; /* texid for each team */
  static GLColor_t colors[NUMPLAYERTEAMS]; /* colors for each */
  h = dsp->ppRow * 5.0;
  w = (dsp->wW * 0.70);

  x = dsp->wW * 0.15;
  y = dsp->ppRow;


  if (firstime)
    {                           /* init the texids */
      int i;
      char buffer[64];

      firstime = FALSE;

      for (i=0; i<NUMPLAYERTEAMS; i++)
        {
          int ndx;
          snprintf(buffer, 64 - 1, "conqlogo%c", 
                   Teams[i].name[0]);

          if ((ndx = findGLTexture(buffer)) >= 0)
            {
              logos[i] = GLTextures[ndx].id;
              if (HAS_GLCOLOR(&GLTextures[ndx].col))
                colors[i] = GLTextures[ndx].col;
              else
                hex2GLColor(0xffe6e6e6, &colors[i]);
            }
          else
            {
              logos[i] = 0;
              clog("%s: could not find texture '%s'\n", 
                   __FUNCTION__, buffer);
            }
        }
    }

  /* Cataboligne - new title set */

  if ( logos[Ships[Context.snum].team] && mult)
    {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glEnable(GL_BLEND);
      
      glEnable(GL_TEXTURE_2D);
      
      glBindTexture(GL_TEXTURE_2D, logos[Ships[Context.snum].team]);

      glColor4f(colors[Ships[Context.snum].team].r,
               colors[Ships[Context.snum].team].g,
               colors[Ships[Context.snum].team].b,
               colors[Ships[Context.snum].team].a);

      drawTexQuad(x, y, w, h, 0.0);
      
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_BLEND);
    }
  else
    glfRender(x, y, 0.0, w, h, fontLargeTxf, Conquest, RedLevelColor, 
              TRUE, FALSE, TRUE);
  
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

/* draw a window sized, semi-opaque quad for overlay nodes */
void mglOverlayQuad(void)
{
  if (getTopONode())
    {                           /* if an overlay node is active, fade
                                   the CP some */
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glColor4f(0.0, 0.0, 0.0, 0.5);
      drawQuad((GLfloat)dConf.wX, (GLfloat)dConf.wY, (GLfloat)dConf.wW, 
               (GLfloat)dConf.wH, 
               0.0);
      glDisable(GL_BLEND);
    }

  return;
}
