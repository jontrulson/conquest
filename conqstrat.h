/* 
 * conqstrat.h - some defines for the parser 
 * 
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/


#define OP_LT 101
#define OP_LE 102
#define OP_GT 103
#define OP_GE 104
#define OP_EQ 105
#define OP_NE 106
 
/* prototypes for the parser */

void initrun(void);
void dumprun(void);
void displayrun(FILE *filefd);
void dumprule( int action );
void addrule( int var, int op, int num );
int invertop( int op );
void valstr( int value, char *buf );
void tokstr( int token, char *buf );
unsigned int ibset(int start, int end);
void print_usage(void);



