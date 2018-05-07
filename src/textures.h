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

#ifndef _TEXTURES_H
#define _TEXTURES_H

#include <GL/gl.h>

#include "conqinit.h"
#include "color.h"

typedef union _GLcolor {
    GLfloat vec[4];
    struct {
        GLfloat r, g, b, a;
    };
} GLColor_t;

typedef struct _GLtexture {
    int cqiIndex;                 /* index into cqiTextures[] */
    GLint id;                     /* GL texture id */
    GLColor_t col;                /* default texture color */
    uint32_t w, h;                 /* width/height of texture */
    real prescale;                 /* prescale to apply when computing size */
} GLTexture_t;

typedef struct _GLtexcoord {
    GLfloat s, t;                 /* texture coordinates or tc offsets */
} GLTexcoord_t;

#ifdef NOEXTERN_TEXTURES
GLTexture_t *GLTextures = NULL;
#else
extern GLTexture_t *GLTextures;
#endif

int findGLTexture(const char *texname);
GLTexture_t *getGLTexture(const char *texname);


/* helpers */
#define HAS_GLCOLOR(x) ((x)->a || (x)->r || (x)->g || (x)->b)

#define GLTEX_COLOR(x)    ((x)->col)
#define GLTEX_ID(x)       ((x)->id)
#define GLTEX_WIDTH(x)    ((x)->w)
#define GLTEX_HEIGHT(x)   ((x)->h)
#define GLTEX_PRESCALE(x) ((x)->prescale)

/* ico texture identifiers for drawIconHUDDecal() (GL.c) */
#define TEX_HUD_ICO                   0
#define TEX_HUD_SHI                   1
#define TEX_HUD_DECAL1                2
#define TEX_HUD_DECAL2                3
#define TEX_HUD_HEAD                  4
#define TEX_HUD_HDP                   5
#define TEX_HUD_WARP                  6
#define TEX_HUD_WARP2                 7

#define TEX_HUD_DECAL1_LAMP_SH        8
#define TEX_HUD_DECAL1_LAMP_HULL      9
#define TEX_HUD_DECAL1_LAMP_FUEL      10
#define TEX_HUD_DECAL1_LAMP_ENG       11
#define TEX_HUD_DECAL1_LAMP_WEP       12
#define TEX_HUD_DECAL1_LAMP_REP       13
#define TEX_HUD_DECAL1_LAMP_CLOAK     14
#define TEX_HUD_DECAL1_LAMP_TOW       15

#endif /* _TEXTURES_H */
