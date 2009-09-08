/* 
 * $Id$
 *
 * Copyright 1999-2006 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _TEXTURES_H
#define _TEXTURES_H 

#include "datatypes.h"
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
  Unsgn32 w, h;                 /* width/height of texture */
} GLTexture_t;  

typedef struct _GLtexcoord {
  GLfloat s, t;                 /* texture coordinates or tc offsets */
} GLTexcoord_t;

#ifdef NOEXTERN_GLTEXTURES
GLTexture_t *GLTextures = NULL;
#else
extern GLTexture_t *GLTextures;
#endif

int findGLTexture(char *texname);
GLTexture_t *getGLTexture(char *texname);


/* helpers */
#define HAS_GLCOLOR(x) ((x)->a || (x)->r || (x)->g || (x)->b)

#define GLTEX_COLOR(x)    ((x)->col)
#define GLTEX_ID(x)       ((x)->id)
#define GLTEX_WIDTH(x)    ((x)->w)
#define GLTEX_HEIGHT(x)   ((x)->h)

/* ico texture identifiers for drawIconHUDDecal() */
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

#define TEX_HUD_ITORPIN               21

void drawIconHUDDecal(GLfloat rx, GLfloat ry, GLfloat w, GLfloat h, 
                  int imgp, cqColor icol);


#endif /* _TEXTURES_H */
