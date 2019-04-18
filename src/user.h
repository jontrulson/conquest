//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef USER_H_INCLUDED
#define USER_H_INCLUDED

#include "conqdef.h"
#include "team.h"

#define MAX_USERNAME  32      /* maximum size of a user's login name */

typedef enum {
    USERTYPE_NORMAL           = 0, // All normal users (connecting via client)
    USERTYPE_BUILTIN          = 1, // special users like "Federation",
                                   // etc that are "builtin" to
                                   // Conquest.
    USERTYPE_ANY              = 2, // match any user type

    USERTYPE_MAX                // Should be last entry
} userTypes_t;

#define MAX_USER_STATS 20

enum UserStats : int {
    USTAT_GENOCIDE = 0,        /* number of genocides completed */
    USTAT_SECONDS = 1,         /* elapsed seconds for each user */
    USTAT_CPUSECONDS = 2,      /* cpu seconds used per user */
    USTAT_WINS = 3,            /* total kills for each user */
    USTAT_LOSSES = 4,          /* total losses for each user */
    USTAT_MAXKILLS = 5,        /* maximum kills for each user */
    USTAT_ENTRIES = 6,         /* number of entries for each user */
    USTAT_CONQUERS = 7,        /* number conquers for each player */
    USTAT_COUPS = 8,           /* number of coups completed */
    USTAT_TORPS = 9,           /* number of torps fired for each user */
    USTAT_PHASERS = 10,        /* number of phaser shots for each user */
    USTAT_ARMBOMB = 11,        /* armies killed by bombing */
    USTAT_ARMSHIP = 12,        /* armies killed on a ship */
    USTAT_CONQPLANETS = 13,    /* number of planets conquered */

    // FIXME for next proto - proto currently uses and sends only
    // USTAT_TOTALSTATS items, rather than larger MAX_USER_STATS,
    // unlike the way TSTATs are sent.  In next revision, should
    // remove USTAT_TOTALSTATS and use MAX_USER_STATS.  This leaves
    // additional slots that could be used someday.
    USTAT_TOTALSTATS = 14,
};


// User flags
#define USER_F_NONE          0x0000
#define USER_F_LIVE          0x0001
#define USER_F_ROBOT         0x0002

// helpers
#define ULIVE(x)             ( cbUsers[(x)].flags & USER_F_LIVE )
#define UROBOT(x)            ( cbUsers[(x)].flags & USER_F_ROBOT )

#define UFSET(x, b)          ( cbUsers[(x)].flags |=  (b) )
#define UFCLR(x, b)          ( cbUsers[(x)].flags &= ~(b) )

// Operator set flags
#define USER_OP_NONE           0x0000
#define USER_OP_PLAYWHENCLOSED 0x0001
#define USER_OP_BANNED         0x0002 // shitlist
#define USER_OP_ISOPER         0x0004 // conquest operator privilege
#define USER_OP_AUTOPILOT      0x0008 // Can enable autopilot (robot ai control)

#define UPLAYWHENCLOSED(x)     ( cbUsers[(x)].opFlags & USER_OP_PLAYWHENCLOSED )
#define UBANNED(x)             ( cbUsers[(x)].opFlags & USER_OP_BANNED )
#define UISOPER(x)             ( cbUsers[(x)].opFlags & USER_OP_ISOPER )
#define UAUTOPILOT(x)          ( cbUsers[(x)].opFlags & USER_OP_AUTOPILOT )

#define UOPSET(x, b)           ( cbUsers[(x)].opFlags |=  (b) )
#define UOPCLR(x, b)           ( cbUsers[(x)].opFlags &= ~(b) )


typedef struct {
    userTypes_t type;           /* type of user */
    uint16_t flags;             /* bitmask of USER_UF_* values */
    uint16_t opFlags;           /* bitmask of USER_OP_* (operator set) values */
    int team;			/* user's team */
    real rating;		/* user's rating */
    time_t lastentry;		/* last entry time */
    bool war[NUM_PLAYERTEAMS];	/* teams you're at war with */
    char username[MAX_USERNAME];	/* user's name (login name) */
    char pw[MAX_USERNAME];	/* user's password if server login */
    char alias[MAX_USERNAME];	/* user's pseudonym */
    int stats[MAX_USER_STATS];	/* user's stats */
} User_t;


#endif /* USER_H_INCLUDED */
