/*
 * rndlb
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef RNDLB_H_INCLUDED
#define RNDLB_H_INCLUDED

void rndini (int seed1, int seed2);
void rndseq (int *value, int multiplier, int increment, int modulus);
real rnd (void);
real rnduni (real rlow, real rhigh);
int rndint (int ilow, int ihigh);
real rndnor (real mean, real stddev);
real rndexp (real mean);
real rndchi (int v);
real rndF (int v1, int v2);
real rndt (int v);
int rndgeo (real prob);
int rndbin (int trials, real prob);
int rndpoi (real mean);

#endif /* RNDLB_H_INCLUDED */
