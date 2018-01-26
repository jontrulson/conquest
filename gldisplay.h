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

#ifndef _GLDISPLAY_H
#define _GLDISPLAY_H

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

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

    uint32_t      flags;

    int          initWidth, initHeight; /* initial wxh geometry */
} dspConfig_t;

#ifdef NOEXTERN_DCONF
dspConfig_t dConf;
uint32_t     frameTime;
#else
extern dspConfig_t dConf;
extern uint32_t     frameTime;
#endif


/* a generic GL rectangle struct */
typedef struct _gl_box
{
    GLfloat x, y;
    GLfloat w, h;
} GLRect_t;

/* a macro for clamping a rectable within a min/max width/height */
#define CLAMPRECT(_w, _h, _rect) {                      \
        (_rect)->x = CLAMP(0.0, (_w), (_rect)->x);      \
        (_rect)->y = CLAMP(0.0, (_h), (_rect)->y);      \
        (_rect)->w = CLAMP(1.0, (_w), (_rect)->w);      \
        (_rect)->h = CLAMP(1.0, (_h), (_rect)->h);      \
    }

void display( int snum );

int uiCStrlen(char *buf);
int uiGLInit(int *argc, char **argv);

#endif /* _GLDISPLAY_H */
