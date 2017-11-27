/************************************************************************
 *
 * sem.h - semaphore operations on the common block.
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#ifndef __SEM_H
#define __SEM_H

char *semGetName(int what);
int   semInit(void);
void  semLock(int what);
void  semUnlock(int what);
char *semGetStatusStr(void);

#endif /* __SEM_H */
