/* 
 * History structure
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef HISTORY_H_INCLUDED
#define HISTORY_H_INCLUDED

#include "conqdef.h"

typedef struct {
  int    histunum;              /* user number, if -1, and username is
                                 *  not empty, then this marks a resigned
                                 *  user. 
                                 */
  time_t histlog;		/* date of entry */
  time_t elapsed;		/* elapsed time of this entry */
  char   username[MAXUSERNAME]; /* username */
} History_t;

#endif /* HISTORY_H_INCLUDED */
