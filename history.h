/* 
 * History structure
 *
 * $Id$
 *
 * Copyright 1998 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef HISTORY_H_INCLUDED
#define HISTORY_H_INCLUDED

#include "conqdef.h"

typedef struct {
  int histunum;			/* user number */
  char histlog[DATESIZE];	/* date string of entry */
  int elapsed;			/* elapsed time of last entry (not used yet) */
} History_t;

#endif /* HISTORY_H_INCLUDED */
