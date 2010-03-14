/* 
 * misc gl funcs
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _MISCGL_H
#define _MISCGL_H

#define VIEWANGLE 45.0
#define TRANZ     -100.0


#define MGL_BEEP_ALERT    0
#define MGL_BEEP_MSG      1
#define MGL_BEEP_ERR      2


int _GLError(const char *funcname, const char *filename, int line);

#define GLError() _GLError(__FUNCTION__, __FILE__, __LINE__)

void mglConqLogo(dspConfig_t *dsp, int mult);
void mglBeep(int type);  
void mglOverlayQuad(void);

#endif /* _MISCGL_H */
