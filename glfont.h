/* 
 * gl font handling
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _GLFONT_H
#define _GLFONT_H

#include "TexFont.h"

/* font DL's  */
#ifdef NOEXTERN_GLFONT

/* Texture mapped fonts */
TexFont        *glfFontLarge = NULL;
TexFont        *glfFontFixed = NULL;
TexFont        *glfFontFixedTiny = NULL;
TexFont        *glfFontMsg = NULL;
#else

extern TexFont *glfFontLarge;
extern TexFont *glfFontFixed;
extern TexFont *glfFontFixedTiny;
extern TexFont *glfFontMsg;
#endif /* NOEXTERN_GLFONT */

/* font flags for glfRenderFont */
#define GLF_FONT_F_NONE             0x00000000 /* nothing */
#define GLF_FONT_F_SCALEX           0x00000001 /* scale to fit width */
#define GLF_FONT_F_DOCOLOR          0x00000002 /* parse embedded colors */
#define GLF_FONT_F_ORTHO            0x00000004 /* use an ortho projection */

void glfRenderFont(GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLfloat h,
                   TexFont *font, char *str, int color, GLColor_t *col,
                   uint32_t flags);
void glfInitFonts(void);


#endif /* _GLFONT_H */
