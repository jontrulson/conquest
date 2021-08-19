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

namespace UserType {
    // Don't change these numbers
    enum UserType : int {
        Normal           = 0, // All normal users (connecting via client)
        Builtin          = 1, // special users like "Federation",
                              // etc that are "builtin" to
                              // Conquest.
        Any              = 2, // match any user type

        MaxTypes              // Should be last entry
    };
};

#define MAX_USER_STATS 20

namespace UserStats {
    // Don't change these numbers
    enum UserStats : int {
        Genocide         = 0,  /* number of genocides completed */
        Seconds          = 1,  /* elapsed seconds for each user */
        CpuSeconds       = 2,  /* cpu seconds used per user */
        Wins             = 3,  /* total kills for each user */
        Losses           = 4,  /* total losses for each user */
        MaxKills         = 5,  /* maximum kills for each user */
        Entries          = 6,  /* number of entries for each user */
        Conquers         = 7,  /* number conquers for each player */
        Coups            = 8,  /* number of coups completed */
        Torps            = 9,  /* number of torps fired for each user */
        Phasers          = 10, /* number of phaser shots for each user */
        ArmiesBombed     = 11, /* armies killed by bombing */
        ArmiesSpaced     = 12, /* armies killed on a ship */
        PlanetsConquered = 13, /* number of planets conquered */

        // FIXME for next proto - proto currently uses and sends only
        // UserStats::TotalStats items, rather than larger
        // MAX_USER_STATS, unlike the way TeamStats are sent.  In next
        // revision, should remove UserStats::TotalStats and use
        // MAX_USER_STATS.  This leaves additional slots that could be
        // used someday.
        TotalStats       = 14,
    };
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
    UserType::UserType type;    /* type of user */
    uint16_t flags;             /* bitmask of USER_UF_* values */
    uint16_t opFlags;           /* bitmask of USER_OP_* (operator set) values */
    Team::Team team;		/* user's team */
    real rating;		/* user's rating */
    time_t lastentry;		/* last entry time */
    bool war[NUM_PLAYERTEAMS];	/* teams you're at war with */
    char username[MAX_USERNAME];	/* user's name (login name) */
    char pw[MAX_USERNAME];	/* user's password if server login */
    char alias[MAX_USERNAME];	/* user's pseudonym */
    int stats[MAX_USER_STATS];	/* user's stats */
} User_t;


#endif /* USER_H_INCLUDED */
