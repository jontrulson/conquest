/* 
 * cprintf defines.  SHOULD BE UI AGNOSTIC!
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _CPRINTF_H
#define _CPRINTF_H

/* cprintf align values */
#define ALIGN_CENTER 3
#define ALIGN_LEFT   2
#define ALIGN_RIGHT  1
#define ALIGN_NONE   0

void cprintf(int lin, int col, int align, char *fmt, ...);

#endif /* _CPRINTF_H */
