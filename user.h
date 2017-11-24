/*
 * User structure(s)
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include "conqdef.h"

typedef enum {
    USERTYPE_NORMAL           = 0, // All normal users (connecting via client)
    USERTYPE_BUILTIN          = 1, // special users like "Federation",
                                   // etc that are "builtin" to
                                   // Conquest.
    USERTYPE_ANY              = 2, // match any user type

    USERTYPE_MAX                // Should be last entry
} userTypes_t;


typedef struct {
    int live;			/* alive or not */
    userTypes_t type;           /* type of user */
    int team;			/* user's team */
    real rating;		/* user's rating */
    int robot;			/* a robot? */
    time_t lastentry;		/* last entry time */
    int war[NUMPLAYERTEAMS];	/* teams you're at war with */
    int ooptions[MAXOOPTIONS];	/* operator settable options */
    char username[MAXUSERNAME];	/* user's name (login name) */
    char pw[MAXUSERNAME];	/* user's password if server login */
    char alias[MAXUSERALIAS];	/* user's pseudonym */
    int stats[MAXUSTATS];	/* user's stats */
} User_t;


#endif /* USER_H_INCLUDED */
