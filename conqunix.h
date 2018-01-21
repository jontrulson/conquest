/*
 * conqunix
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef CONQUNIX_H_INCLUDED
#define CONQUNIX_H_INCLUDED

int getUID(char *name);
int getConquestGID(void);
void conqinit(void);
void conqstats(int snum);
void drcheck(void);
void drcreate(void);
void drkill(void);
void drpexit(void);
void drstart(void);
void gcputime(unsigned int *cpu);
void initstats(unsigned int *ctemp, unsigned int *etemp);
int isagod(int unum );
int mailimps(char *subject, char *msg);
int checkPID(int pidnum);
void upchuck(void);
void upstats(unsigned int *ctemp, unsigned int *etemp, unsigned int *caccum,
             unsigned int *eaccum, unsigned int *ctime, unsigned int *etime);

#endif /* CONQUNIX_H_INCLUDED */
