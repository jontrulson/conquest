/* 
 * Robot Strategy structure
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef ROBOT_H_INCLUDED
#define ROBOT_H_INCLUDED

#include "conqdef.h"

typedef struct {
  int rstrat[REAL_MAX_VAR][10];	/* robot strategy table */
  int rvec[32];			/* robot rule to action token */
} Robot_t;

#endif /* ROBOT_H_INCLUDED */
