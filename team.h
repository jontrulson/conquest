/* 
 * Team structure
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef TEAM_H_INCLUDED
#define TEAM_H_INCLUDED

#include "conqdef.h"

typedef struct {
  int homeplanet;		/* planet number of the home planet */
  int homesun;			/* sun number of the home system */
  int teamhplanets[3];		/* planet numbers of the home system */
  int armylim;			/* maximum armies a team ship can carry */
  real warplim;			/* maximum warp based on team */
  real engfac;			/* team engine constants */
  real accelfac;		/* team acceleration constants */
  real weafac;			/* team weapons constants */
  real torpwarp;		/* torp speed for team */
  int stats[MAXTSTATS];		/* team stats */
  int coupinfo;			/* coup info */
  int couptime;			/* couptime */
  char torpchar;		/* char to display for torps */
  char teamchar;		/* F = F  R = R  K = K  O = O */
  char name[MAXTEAMNAME];	/* team name */
} Team_t;

#endif /* TEAM_H_INCLUDED */
