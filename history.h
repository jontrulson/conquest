/*
 * History structure
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef HISTORY_H_INCLUDED
#define HISTORY_H_INCLUDED

#include "conqdef.h"

typedef struct {
    int    unum;              // user number, if -1, then this marks a
                              // resigned user.
    time_t enterTime;         /* date of entry */
    time_t elapsed;           /* elapsed time of this entry */
    char   username[MAXUSERNAME]; /* username */
} History_t;

#endif /* HISTORY_H_INCLUDED */
