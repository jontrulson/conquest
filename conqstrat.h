/*
 * conqstrat.h - some defines for the parser
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _CONQSTRAT_H
#define _CONQSTRAT_H

#define OP_LT 101
#define OP_LE 102
#define OP_GT 103
#define OP_GE 104
#define OP_EQ 105
#define OP_NE 106

/* prototypes for the parser */

void         initrun(void);
void         dumprun(void);
void         displayrun(FILE *filefd);
void         dumprule( int action );
void         addrule( int var, int op, int num );
int          invertop( int op );
void         valstr( int value, char *buf );
void         tokstr( int token, char *buf );
unsigned int ibset(int start, int end);
void         print_usage(void);

#endif /* _CONQSTRAT_H */
