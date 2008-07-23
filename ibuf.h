/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#ifndef IBUF_H
#define IBUF_H

#include "datatypes.h"

void         ibufInit(void);
void         ibufFlush(void);
int          ibufCount(void);
void         ibufPut(char *thestr);
void         ibufPutc(unsigned int thechar);
unsigned int ibufGetc(void);
int          ibufExpandMacro(int fkey);
int          ibufExpandMouseMacro(int but, Unsgn32 mods, real mangle);

#endif /* IBUF_H */
