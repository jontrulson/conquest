/*
 * conqai
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef CONQAI_H_INCLUDED
#define CONQAI_H_INCLUDED

void defend( int attacker, int pnum );
int newrob( int *snum, int unum );
void robotloop(void);
void robreply( char buf[] );
void robstr( int token, char buf[] );
void robotai( int snum );

#endif /* CONQAI_H_INCLUDED */
