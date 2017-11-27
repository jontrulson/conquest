/*
 * rndlb
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef RNDLB_H_INCLUDED
#define RNDLB_H_INCLUDED

void rndini (void);
real rnd (void);
real rnduni (real rlow, real rhigh);
int  rndint (int ilow, int ihigh);
real rndnor (real mean, real stddev);

#endif /* RNDLB_H_INCLUDED */
