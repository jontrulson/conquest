/* 
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _RENDER_H
#define _RENDER_H
void renderHud(void);
void renderViewer(int dovbg);
void renderScale(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
                 int min, int max, int val, char *label,
                 int lcolor, int scalecolor,                  
                 TexFont *lfont, TexFont *vfont);
void renderAlloc(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
               struct _alloc *a,
               TexFont *lfont, TexFont *vfont);

#endif /* _RENDER_H */
