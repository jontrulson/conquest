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


#ifndef TEAM_H_INCLUDED
#define TEAM_H_INCLUDED

#include "conqdef.h"

// use int to preserve CB compatibility
enum Team : int {
    // "real" teams players can be on
    TEAM_FEDERATION = 0,       /* Feddie Bears */
    TEAM_ROMULAN = 1,          /* Rom Dogs */
    TEAM_KLINGON = 2,          /* Klings */
    TEAM_ORION = 3,            /* Bugs */

    // not so real teams, not for players
    TEAM_SELFRULED = 4,        /* self ruled */
    TEAM_NOTEAM = 5,           /* nope. */
    TEAM_GOD = 6,              /* god team. should change to Alliance */
    TEAM_EMPIRE = 7,           /* Imperial Empire armies */
};

#define NUM_PLAYERTEAMS 4        /* four "real" teams */
#define NUM_ALLTEAMS 8           /* total number of teams */

#define MAX_TEAMNAME 12          /* maximum team name... */

// The order of these characters must match The team numbers (F = 0, R
// = 1, etc)
static const char TEAM_PLAYERCHARS[NUM_PLAYERTEAMS] = { 'F', 'R', 'K', 'O'};

// team stats
#define MAX_TEAM_STATS 20

// use int to preserve CB compatibility
enum TeamStats : int {
    TSTAT_GENOCIDE = 0,        /* number of genocides completed */
    TSTAT_SECONDS = 1,         /* elapsed seconds for each team */
    TSTAT_CPUSECONDS = 2,      /* cpu seconds used per team */
    TSTAT_WINS = 3,            /* wins for each team */
    TSTAT_LOSSES = 4,          /* losses for each team */
    TSTAT_ENTRIES = 5,         /* number of entries for each team */
    TSTAT_CONQUERS = 6,        /* number of conquers for each team */
    TSTAT_COUPS = 7,           /* number of coups completed */
    TSTAT_TORPS = 8,           /* number of entries for each team */
    TSTAT_PHASERS = 9,         /* number of entries for each team */
    TSTAT_ARMBOMB = 10,        /* armies killed by bombing */
    TSTAT_ARMSHIP = 11,        /* armies killed on a ship */
    TSTAT_CONQPLANETS = 12,    /* number of planets conquered */
    // can't have more than MAX_TEAM_STATS.  Maybe should be FIXME in
    // future protocol to actual number used.  Doubt we will add more.
};

// Some flags
#define TEAM_F_NONE             0x0000
#define TEAM_F_COUPINFO         0x0001 // can get coup info?
// To be enabled, there must be at least one planet owned by the team
// at universe init (via cqiInitPlanets()) time.
#define TEAM_F_ENABLED          0x0002 // owns at least one planet?

#define TEAM_COUPINFO(x) ( cbTeams[(x)].flags & TEAM_F_COUPINFO )
#define TEAM_ENABLED(x)  ( cbTeams[(x)].flags & TEAM_F_ENABLED )

#define TEAM_SETF(x, b)  ( cbTeams[(x)].flags |=  (b) )
#define TEAM_CLRF(x, b)  ( cbTeams[(x)].flags &= ~(b) )


typedef struct {
    int shiptype;               /* type of ship, one of the ST_* types */
    int couptime;               /* couptime */
    int homeplanet;             /* 'official' team home planet */
    char torpchar;              /* char to display for torps */
    char teamchar;              /* F = F  R = R  K = K  O = O */
    uint16_t flags;             // Team flags
    int stats[MAX_TEAM_STATS];    /* team stats */
    char name[MAX_TEAMNAME];     /* team name */
} Team_t;

#endif /* TEAM_H_INCLUDED */
