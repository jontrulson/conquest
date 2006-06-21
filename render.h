/* 
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _RENDER_H
#define _RENDER_H
void renderHud(int dostats);
void renderViewer(int dovbg, int dobomb);
void renderScale(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
                 int min, int max, int val, int scalecolor);
void renderAlloc(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
               struct _alloc *a,
               TexFont *lfont, TexFont *vfont, int HUD);

#endif /* _RENDER_H */
