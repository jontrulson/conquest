/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef IBUF_H
#define IBUF_H



void         ibufInit(void);
void         ibufFlush(void);
int          ibufCount(void);
void         ibufPut(char *thestr);
void         ibufPutc(unsigned int thechar);
unsigned int ibufGetc(void);
int          ibufExpandMacro(int fkey);
int          ibufExpandMouseMacro(int but, uint32_t mods, real mangle);

#endif /* IBUF_H */
