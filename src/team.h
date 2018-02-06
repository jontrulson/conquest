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

#define TEAM_FEDERATION 0       /* Feddie Bears */
#define TEAM_ROMULAN 1          /* Rom Dogs */
#define TEAM_KLINGON 2          /* Klings */
#define TEAM_ORION 3            /* Bugs */

#define NUMPLAYERTEAMS 4        /* four "real" teams */

#define TEAM_SELFRULED 4        /* self ruled */
#define TEAM_NOTEAM 5           /* nope. */
#define TEAM_GOD 6              /* god team. should change to Alliance */
#define TEAM_EMPIRE 7           /* Imperial Empire armies */

#define NUMALLTEAMS 8           /* total number of teams */

#define MAXTEAMNAME 12          /* maximum team name... */

// The order of these characters must match The team numbers (F = 0, R
// = 1, etc)
static const char TEAM_PLAYERCHARS[NUMPLAYERTEAMS] = { 'F', 'R', 'K', 'O'};

// Some flags
#define TEAM_F_NONE             0x0000
// To be enabled, there must be at least one planet owned by the team
// at universe init (via cqiInitPlanets()) time.
#define TEAM_F_COUPINFO         0x0001 // can get coup info?

#define TEAM_COUPINFO(x) ( cbTeams[(x)].flags & TEAM_F_COUPINFO )

#define TEAM_SETF(x, b)  ( cbTeams[(x)].flags |=  (b) )
#define TEAM_CLRF(x, b)  ( cbTeams[(x)].flags &= ~(b) )


typedef struct {
    int shiptype;		/* type of ship, one of the ST_* types */
    int couptime;		/* couptime */
    int homeplanet;             /* 'official' team home planet */
    char torpchar;		/* char to display for torps */
    char teamchar;		/* F = F  R = R  K = K  O = O */
    uint16_t flags;             // Team flags
    int stats[MAXTSTATS];	/* team stats */
    char name[MAXTEAMNAME];	/* team name */
} Team_t;

#endif /* TEAM_H_INCLUDED */
