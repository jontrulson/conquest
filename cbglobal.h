/************************************************************************
 *
 * $Id$
 *
 * This file contains the common block version of the CQI 'global' section.
 *
 * Copyright 1999-2010 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "datatypes.h"
#include "defs.h"

#ifndef _CBGLOBAL_H
#define _CBGLOBAL_H

typedef struct _cb_global {
  int maxplanets;
  int maxships;
  int maxusers;
  int maxhist;
  int maxmsgs;

  int maxtorps;                 /* computed at Universal Init time */
  int maxshiptypes;
} CBGlobal_t;

#endif /* _CBGLOBAL_H */
