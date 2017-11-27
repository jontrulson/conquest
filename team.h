/* 
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef TEAM_H_INCLUDED
#define TEAM_H_INCLUDED

#include "conqdef.h"

typedef struct {
    int shiptype;		/* type of ship, one of the ST_* types */
    int stats[MAXTSTATS];	/* team stats */
    int coupinfo;		/* coup info */
    int couptime;		/* couptime */
    int homeplanet;             /* 'official' team home planet */
    char torpchar;		/* char to display for torps */
    char teamchar;		/* F = F  R = R  K = K  O = O */
    char name[MAXTEAMNAME];	/* team name */
} Team_t;

#endif /* TEAM_H_INCLUDED */
