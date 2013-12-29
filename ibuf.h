/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

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
