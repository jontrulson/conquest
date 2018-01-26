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
                   TexFont *font, const char *str, int color, GLColor_t *col,
                   uint32_t flags);
void glfInitFonts(void);


#endif /* _GLFONT_H */
