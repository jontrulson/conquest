/* 
 * $Id$
 *
 * Copyright 2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _IOLB_H
#define _IOLB_H

int  iochav( void );
int  iogchar ( void );
int  iogtimed ( int *ch, real seconds );
void ioeat(void);

#endif /* _IOLB_H */
