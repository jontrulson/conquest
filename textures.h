/* 
 * textures.h
 * 
 * $Id$
 *
 * Copyright 1999-2006 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _TEXTURES_H
#define _TEXTURES_H 

#include "datatypes.h"
#include "conqinit.h"

typedef struct _GLcolor {
  GLfloat a, r, g, b;
} GLColor_t;

typedef struct _GLtexture {
  int cqiIndex;                 /* index into cqiTextures[] */
  GLint id;                     /* GL texture id */
  GLColor_t col;                /* default texture color */
  Unsgn32 w, h;                 /* width/height of texture */
} GLTexture_t;  

#ifdef NOEXTERN_GLTEXTURES
GLTexture_t *GLTextures = NULL;
#else
extern GLTexture_t *GLTextures;
#endif

int findGLTexture(char *texname);

/* helpers */
#define HAS_GLCOLOR(x) ((x)->a || (x)->r || (x)->g || (x)->b)

/* ico texture identifiers for drawIconHUDDecal() */
#define HUD_ICO        0
#define HUD_SHI        1
#define HUD_DECAL1     2
#define HUD_DECAL2     3
#define HUD_HEAD       4
#define HUD_HDP        5
#define HUD_WARP       6
#define HUD_WARP2      7
#define HUD_IARMIES    11 
#define HUD_ICLOAK     12 
#define HUD_ITRACTOR   13
#define HUD_ISHCRIT    14
#define HUD_IENGCRIT   15
#define HUD_IWEPCRIT   16
#define HUD_IHULCRIT   17
#define HUD_ISHFAIL    18
#define HUD_IENGFAIL   19
#define HUD_IREPAIR    20
#define HUD_ITORPIN    21

void drawIconHUDDecal(GLfloat rx, GLfloat ry, GLfloat w, GLfloat h, 
                  int imgp, int icol);


#endif /* _TEXTURES_H */
