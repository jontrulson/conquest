/* 
 * Planet structure
 *
 * $Id$
 *
 * Copyright 1998 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef PLANET_H_INCLUDED
#define PLANET_H_INCLUDED

#include "conqdef.h"

typedef struct {
  real x;			/* x coordinates of planet */
  real y;			/* y coordinates of planet */
  real orbrad;			/* radius of the orbit */
  real orbvel;			/* velocity of the orbit in deg/min */
  real orbang;			/* current angle of the orbit */
  int primary;			/* planet this planet orbits */
  int type;			/* type of planet - M, dead, sun */
  int real;			/* tells if planet can be seen */
  int team;			/* which team owns the planet */
  int armies;			/* how many armies are on the planet */
  int uninhabtime;		/* minutes left till inhabitable */
  int scanned[NUMPLAYERTEAMS];	/* which planets have been */
				/*  scanned by which teams */
  char name[MAXPLANETNAME];	/* planet name */
} Planet_t;

#endif /* PLANET_H_INCLUDED */
