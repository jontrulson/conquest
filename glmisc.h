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

void _GLError(char *funcname, int line);

#define GLError() _GLError(__FUNCTION__, __LINE__)


void mglConqLogo(dspConfig_t *dsp);
void mglBeep(void);  

#endif /* _MISCGL_H */
