/************************************************************************
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _GLDISPLAY_H
#define _GLDISPLAY_H

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "textures.h"
#include "hud.h"

/* flags for the dsp (display info) structure */
#define DSP_F_NONE           0x00000000
#define DSP_F_INITED         0x00000001 /* initialized? */
#define DSP_F_FULLSCREEN     0x00000002 /* use fullscreen? */

#define DSPFSET(f)           (dConf.flags |=  (f))
#define DSPFCLR(f)           (dConf.flags &= ~(f))

#define DSP_INITED()         (dConf.flags & DSP_F_INITED)
#define DSP_FULLSCREEN()     (dConf.flags & DSP_F_FULLSCREEN)

typedef struct _dspConfig {
  /* glut win ids */
  int          mainw;           /* main window */

  /* main window */
  GLfloat      wX, wY;          /* x/y origin */
  GLfloat      wW, wH;          /* width/height  */
  GLfloat      wAspect;

  /* viewer window */
  GLfloat      vX, vY;          /* viewer X/Y */
  GLfloat      vW, vH;          /* viewer width/height */
  GLfloat      vAspect;

  /* LR/SR viewer scaling factors (magnification) */
  GLfloat      vScaleLR, vScaleSR;

  GLfloat      ppCol, ppRow;    /* pixels per [Row|Col] */

  GLfloat      wBorderW;        /* width of outside mainw border */

  GLfloat      hudProjection[16]; /* hud proj matrix */
  GLfloat      viewerProjection[16]; /* viewer proj matrix */

  Unsgn32      flags; 

  int          initWidth, initHeight; /* initial wxh geometry */
} dspConfig_t;

#ifdef NOEXTERN_DCONF
dspConfig_t dConf;
Unsgn32     frameTime;
#else
extern dspConfig_t dConf;
extern Unsgn32     frameTime;
#endif


/* a generic GL rectangle struct */
typedef struct _gl_box
{
  GLfloat x, y;
  GLfloat w, h;
} GLRect_t;

/* a macro for clamping a rectable within a min/max width/height */
#define CLAMPRECT(_w, _h, _rect) { \
 (_rect)->x = CLAMP(0.0, (_w), (_rect)->x);    \
 (_rect)->y = CLAMP(0.0, (_h), (_rect)->y);    \
 (_rect)->w = CLAMP(1.0, (_w), (_rect)->w);    \
 (_rect)->h = CLAMP(1.0, (_h), (_rect)->h);    \
}

void display( int snum );

int uiCStrlen(char *buf);
int uiGLInit(int *argc, char **argv);
void drawPlanet( GLfloat x, GLfloat y, int pnum, int scale,
                   int textcolor );
real cu2GLSize(real size, int scale);

int GLcvtcoords(real cenx, real ceny, real x, real y, real scale,
		 GLfloat *rx, GLfloat *ry );

void drawTorp(GLfloat x, GLfloat y, char torpchar, int color, int scale,
              int snum, int torpnum);
void drawShip(GLfloat x, GLfloat y, GLfloat angle, char ch, int i, 
	      int color, GLfloat scale);
void drawDoomsday(GLfloat x, GLfloat y, GLfloat angle, GLfloat scale);
void drawViewerBG(int snum, int dovbg);
void drawNEB(int snum);

float getFPS(void);

void drawLine(GLfloat x, GLfloat y, GLfloat len, GLfloat lw);
void drawLineBox(GLfloat x, GLfloat y, GLfloat z,
                 GLfloat w, GLfloat h, int color, 
                 GLfloat lw);
void drawQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat z);
void drawTexQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat z,
                 int ortho);
void drawExplosion(GLfloat x, GLfloat y, int snum, int torpnum, int scale);
void drawBombing(int snum, int scale);

void dspInitData(void);
void hex2GLColor(Unsgn32 hcol, GLColor_t *col);
void setViewerScaling(int scale, int isLR);

#endif /* _GLDISPLAY_H */
