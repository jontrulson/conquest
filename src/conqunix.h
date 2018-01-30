//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


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
void initstats(unsigned int *ctemp, uint64_t *etemp);
int isagod(int unum );
int mailimps(char *subject, char *msg);
int checkPID(int pidnum);
void upchuck(void);
void upstats(unsigned int *ctemp, uint64_t *etemp, unsigned int *caccum,
             unsigned int *eaccum, unsigned int *ctime, unsigned int *etime);

#endif /* CONQUNIX_H_INCLUDED */
