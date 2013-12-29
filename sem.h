/************************************************************************
 *
 * sem.h - semaphore operations on the common block.
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef __SEM_H
#define __SEM_H

char *semGetName(int what);
int   semInit(void);
void  Lock(int what);
void  Unlock(int what);
char *semGetStatusStr(void);

#endif /* __SEM_H */
