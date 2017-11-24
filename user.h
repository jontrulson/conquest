/*
 * User structure(s)
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include "conqdef.h"

typedef struct {
    int live;			/* alive or not */
    int type;			/* type of user: UT_LOCAL, UT_REMOTE */
    int team;			/* user's team */
    int multiple;		/* ship count def=1, shitlist=0?, >1 = mult? */
    int stats[MAXUSTATS];	/* user's stats */
    real rating;		/* user's rating */
    int robot;			/* a robot? */
    int war[NUMPLAYERTEAMS];	/* teams you're at war with */
    int ooptions[MAXOOPTIONS];	/* operator settable options */
    char username[MAXUSERNAME];	/* user's name (login name) */
    char pw[MAXUSERNAME];	/* user's password if server login */
    char alias[MAXUSERALIAS];	/* user's pseudonym */
    time_t lastentry;		/* last entry time */
} User_t;


#endif /* USER_H_INCLUDED */
