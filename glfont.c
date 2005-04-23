/* gl font rendering
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
#include "ui.h"
#include "conqcom.h"
#include "ibuf.h"
#include "gldisplay.h"
#include "conf.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "glmisc.h"

#define NOEXTERN_GLFONT
#include "glfont.h"
#undef NOEXTERN_GLFONT

#include <assert.h>

void initTexFonts(void)
{
  char fbuf[MID_BUFFER_SIZE];
  int fail = FALSE;

#ifdef DEBUG_GL
  clog("%s: ENTER...", __FUNCTION__);
#endif

  sprintf(fbuf, "%s/img/%s", CONQSHARE, "large.txf");

  fontLargeTxf = txfLoadFont(fbuf);

  if (!fontLargeTxf)
    {
      clog("initTexFonts: load %s failed", fbuf);
      fprintf(stderr, "initTexFonts: load %s failed\n", fbuf);
      fail = TRUE;
    }

  sprintf(fbuf, "%s/img/%s", CONQSHARE, "fixed.txf");
  fontFixedTxf = txfLoadFont(fbuf);

  if (!fontFixedTxf)
    {
      clog("initTexFonts: load %s failed", fbuf);
      fprintf(stderr, "initTexFonts: load %s failed\n", fbuf);
      fail = TRUE;
    }

  sprintf(fbuf, "%s/img/%s", CONQSHARE, "tinyfixed.txf");
  fontTinyFixedTxf = txfLoadFont(fbuf);

  if (!fontTinyFixedTxf)
    {
      clog("initTexFonts: load %s failed", fbuf);
      fprintf(stderr, "initTexFonts: load %s failed\n", fbuf);
      fail = TRUE;
    }

  sprintf(fbuf, "%s/img/%s", CONQSHARE, "msg.txf");
  fontMsgTxf = txfLoadFont(fbuf);

  if (!fontMsgTxf)
    {
      clog("initTexFonts: load %s failed", fbuf);
      fprintf(stderr, "initTexFonts: load %s failed\n", fbuf);
      fail = TRUE;
    }

  if (fail)
    exit(1);

  txfEstablishTexture(fontLargeTxf, 0, GL_TRUE);
  GLError();
  txfEstablishTexture(fontFixedTxf, 0, GL_TRUE);
  GLError();
  txfEstablishTexture(fontTinyFixedTxf, 0, GL_TRUE);
  GLError();
  txfEstablishTexture(fontMsgTxf, 0, GL_TRUE);
  GLError();
  return;
}

void glfRender(GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLfloat h,
                       TexFont *font, char *str, int color, 
               int scalex, int dofancy, int ortho)
{
  GLfloat inverty = ((ortho) ? -1.0 : 1.0);
  int width, ascent, descent;
  GLfloat xs = 1.0, ys = 1.0;

  int l;

  if (!font)
    return;
 
  if (!str)
    return;

  l = strlen(str);

  txfBindFontTexture(font);
  txfGetStringMetrics( font, str, l, &width, 
                       &ascent, &descent);
  if (scalex)
    xs = w/(GLfloat)width;

  ys = ((2.0 * (h/(ascent + descent))) * 0.70);
      
#if 0
 clog("glfRender(%s): WINDOW: %d, w = %f h = %f, \n"
      "\twid = %d, asc = %d desc = %d"
      "\ttxf height = %d",
       str, 
      glutGetWindow(), w, h, width, ascent, descent,
      font->tex_height);
#endif

  glEnable(GL_TEXTURE_2D);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                  GL_LINEAR_MIPMAP_LINEAR);

  glPushMatrix();
  glLoadIdentity();

  if (ortho) 
    {
      glTranslatef( x, y + h, z );
      glScalef(xs, inverty * ys, 1.0);
    }
  else
    {
      glTranslatef( x, y - h, z );
      glScalef(xs, inverty * ys, 1.0);
    }

  uiPutColor(color);
  
  if (dofancy)
    txfRenderFancyString(font, str, l);
  else
    txfRenderString(font, str, l);

  glPopMatrix();

  glDisable(GL_TEXTURE_2D);

  return;
}

