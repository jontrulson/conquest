/* 
 *  gl font handling
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _GLFONT_H
#define _GLFONT_H

#include "TexFont.h"

/* font DL's  */
#ifdef NOEXTERN_GLFONT

/* Texture mapped fonts */
TexFont *fontLargeTxf = NULL;
TexFont *fontFixedTxf = NULL;
TexFont *fontTinyFixedTxf = NULL;
TexFont *fontMsgTxf = NULL;
#else

extern TexFont *fontLargeTxf;
extern TexFont *fontFixedTxf;
extern TexFont *fontTinyFixedTxf;
extern TexFont *fontMsgTxf;
#endif /* NOEXTERN_GLFONT */

void glfRender(GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLfloat h,
                       TexFont *font, char *str, int color, int scalex,
               int dofancy, int ortho);
void initTexFonts(void);


#endif /* _GLFONT_H */
