/*
 * User structure(s)
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
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

// User flags
#define USER_F_NONE          0x0000
#define USER_F_LIVE          0x0001
#define USER_F_ROBOT         0x0002

// helpers
#define ULIVE(x)             ( Users[(x)].flags & USER_F_LIVE )
#define UROBOT(x)            ( Users[(x)].flags & USER_F_ROBOT )

#define UFSET(x, b)          ( Users[(x)].flags |=  (b) )
#define UFCLR(x, b)          ( Users[(x)].flags &= ~(b) )

// Operator set flags
#define USER_OP_NONE           0x0000
#define USER_OP_PLAYWHENCLOSED 0x0001
#define USER_OP_BANNED         0x0002 // shitlist
#define USER_OP_ISOPER         0x0004 // conquest operator privilege
#define USER_OP_AUTOPILOT      0x0008 // Can enable autopilot (robot ai control)

#define UPLAYWHENCLOSED(x)     ( Users[(x)].opFlags & USER_OP_PLAYWHENCLOSED )
#define UBANNED(x)             ( Users[(x)].opFlags & USER_OP_BANNED )
#define UISOPER(x)             ( Users[(x)].opFlags & USER_OP_ISOPER )
#define UAUTOPILOT(x)          ( Users[(x)].opFlags & USER_OP_AUTOPILOT )

#define UOPSET(x, b)           ( Users[(x)].opFlags |=  (b) )
#define UOPCLR(x, b)           ( Users[(x)].opFlags &= ~(b) )


typedef struct {
    userTypes_t type;           /* type of user */
    uint16_t flags;             /* bitmask of USER_UF_* values */
    uint16_t opFlags;           /* bitmask of USER_OP_* (operator set) values */
    int team;			/* user's team */
    real rating;		/* user's rating */
    time_t lastentry;		/* last entry time */
    bool war[NUMPLAYERTEAMS];	/* teams you're at war with */
    char username[MAXUSERNAME];	/* user's name (login name) */
    char pw[MAXUSERNAME];	/* user's password if server login */
    char alias[MAXUSERNAME];	/* user's pseudonym */
    int stats[MAXUSTATS];	/* user's stats */
} User_t;


#endif /* USER_H_INCLUDED */
