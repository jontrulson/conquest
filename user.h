/* 
 * User structure(s)
 *
 * $Id$
 *
 * Copyright 1998 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include "conqdef.h"

typedef struct {
  int live;			/* alive or not */
  int team;			/* user's team */
  int multiple;			/* ship count def=1, shitlist=0?, >1 = mult? */
  int stats[MAXUSTATS];		/* user's stats */
  real rating;			/* user's rating */
  int robot;			/* a robot? */
  int war[NUMPLAYERTEAMS];	/* teams you're at war with */
  int options[MAXOPTIONS];	/* user's options */
  int ooptions[MAXOOPTIONS];	/* operator settable options */
  char username[SIZEUSERNAME];	/* user's name (login name) */
  char pw[SIZEUSERNAME];	/* user's password if server login */
  char alias[SIZEUSERPNAME];	/* user's pseudonym */
  char lastentry[DATESIZE];	/* last entry time */
} User_t;


#endif /* USER_H_INCLUDED */
