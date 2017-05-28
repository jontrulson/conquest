/*
 * conqunix
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef CONQUNIX_H_INCLUDED
#define CONQUNIX_H_INCLUDED

int getUID(char *name);
int getConquestGID(void);
void comsize(unsigned long *size);
void conqinit(void);
void conqstats(int snum);
void drcheck(void);
void drcreate(void);
void drkill(void);
void drpexit(void);
void drstart(void);
void gcputime(int *cpu);
void initstats(int *ctemp, int *etemp);
int isagod(int unum );
int mailimps(char *subject, char *msg);
int checkPID(int pidnum);
void upchuck(void);
void upstats(int *ctemp, int *etemp, int *caccum, int *eaccum,
             int *ctime, int *etime);

#endif /* CONQUNIX_H_INCLUDED */
