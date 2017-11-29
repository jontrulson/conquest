/************************************************************************
 * This file contains the common block version of the CQI 'global' section.
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/


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

    int pad[32];

} cbGlobalLimits_t;

#endif /* _CBGLOBAL_H */
