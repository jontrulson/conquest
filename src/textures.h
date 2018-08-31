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

#include <vector>
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
    int       cqiIndex;    /* index into cqiTextures[] */
    GLint     id;          /* GL texture id */
    GLColor_t col;         /* default texture color */
    uint32_t  w, h;        /* width/height of texture */
    real      prescale;    /* prescale to apply when computing size */
} GLTexture_t;

// rather than use ptrs, let's just use an int index,,,
typedef int textureIdx_t;

typedef struct _GLtexcoord {
    GLfloat s, t;                 /* texture coordinates or tc offsets */
} GLTexcoord_t;

#ifdef NOEXTERN_TEXTURES
std::vector<GLTexture_t> GLTextures;
#else
extern std::vector<GLTexture_t> GLTextures;
#endif

int findGLTexture(const char *texname);
GLTexture_t *getGLTexture(const char *texname);


/* helpers */
#define HAS_GLCOLOR(x) ((x).a || (x).r || (x).g || (x).b)

#define GLTEX_COLOR(x)    (GLTextures[(x)].col)
#define GLTEX_ID(x)       (GLTextures[(x)].id)
#define GLTEX_WIDTH(x)    (GLTextures[(x)].w)
#define GLTEX_HEIGHT(x)   (GLTextures[(x)].h)
#define GLTEX_PRESCALE(x) (GLTextures[(x)].prescale)

/* ico texture identifiers for drawIconHUDDecal() (GL.c) */
typedef enum {
    TEX_HUD_ICO                      = 0,
    TEX_HUD_SHI,
    TEX_HUD_DECAL1,
    TEX_HUD_DECAL2,
    TEX_HUD_HEAD,
    TEX_HUD_HDP,                     // heading pointer
    TEX_HUD_WARP,                    // warp number
    TEX_HUD_WARP2,                   // warp guage

    // Decal1 lamps
    TEX_HUD_DECAL1_LAMP_SH,
    TEX_HUD_DECAL1_LAMP_HULL,
    TEX_HUD_DECAL1_LAMP_FUEL,
    TEX_HUD_DECAL1_LAMP_ENG,
    TEX_HUD_DECAL1_LAMP_WEP,
    TEX_HUD_DECAL1_LAMP_REP,
    TEX_HUD_DECAL1_LAMP_CLOAK,
    TEX_HUD_DECAL1_LAMP_TOW,
} textureHUDItem_t;

#endif /* _TEXTURES_H */
