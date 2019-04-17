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


#ifndef CONQDEF_H_INCLUDED
#define CONQDEF_H_INCLUDED

// Original history entries - just for fun
/* 4.22 05Sep86 CAL .*(): Removed tutorial feature. Made tuning constants */
/*                   constants again. Increased username sizes. */
/*                  .mcuUserStats(): Tweak heading. */
/*                  .dowarp(): Don't need to clbFixDeltas(). */
/*                  .executeai(): Stop repair mode when setting warp. */
/*                  .clbChalkup(): New routine to help with kills accounting. */
/*                  .clbIKill(): Fixed longstanding mutual kill bug. */
/*                  .cloak(): Charge cloaking engage as engine energy use. */
/* 4.21 13Aug86 CAL .Conqinit(): Use t_getbpri(). */
/* 4.20 22Jun86 CAL .Getamsg(): Don't read messages from us to GOD. */
/* 4.19 03Nov85 CAL .Statline(): Tweak cpu printout. */
/* 4.18 30Oct85 CAL .Secdrive(): Added some death checks from clbStillAlive(). */
/*                   Moved robot stats code to robotai(). */
/* 4.17 29Oct85 CAL .*(): Add snum arguments to clbStillAlive(), remove user check. */
/*                  .Robotai(): Use clbStillAlive(). */
/*                  .Kiss(): Add "all" option. */
/* 4.16 17Oct85 CAL .*(): Add seperate robot cpu and elapsed stats. */
/* 4.15 09Oct85 CAL .Display(): Fixed weird yellow alert bug. */
/* 4.14 06Oct85 CAL .Oprobot(): Add multiple robot creation. */
/*                  .Operate(), watch(): Get msg timestamp after action switch. */
/*                  .Putship(): Increase smear factor slightly. */
/*                  .*(): Converted maxwarp(), engeff() and weaeff() to macros. */
/* 8.12  7Oct05     .clbInitPlanets(): mod for .planetsrc data input */

/* Common block time stamp. */
/* This define should be changed every time the common block changes. */

/*#define COMMONSTAMP 861104             stardate? */
/*#define COMMONSTAMP 940910             stardate? */
/*#define COMMONSTAMP 961019             stardate? */
/*#define COMMONSTAMP 971207             stardate? */
/*#define COMMONSTAMP 980628             stardate? */
/*#define COMMONSTAMP 990124             stardate? */
/*#define COMMONSTAMP 20001231           stardate? */
/*#define COMMONSTAMP 20030829 */
// #define COMMONSTAMP 20101027
#define COMMONSTAMP 20180509

// Absolute maximum values for certain arrays.  Changing these will
// require significant changes everywhere (CB/protocol mostly).  These
// are items that can be changed in the global {} section of
// conqinitrc, but only up to these values as an absolute maximum.

#define ABS_MAXPLANETS             (256)
#define ABS_MAXSHIPS               (256)
#define ABS_MAXTORPS               (256)
#define ABS_MAXHIST                (256)
#define ABS_MAXMSGS                (256)
#define ABS_MAXUSERS               (65536)
#define ABS_MAXSHIPTYPES           (256)

// This is the maximum size of the universe - 200 million CU's in each
// direction.  After hitting this limit, we'll simply wrap around.
#define MAX_UNIVERSAL_COORD        (200000000.0)

#ifndef M_PI
# define M_PI 3.141592654
#endif

#define CONQ_HTTP   "https://github.com/jtrulson/conquest"

/* Values for sstatus() */
#define SS_OFF 1                /* available for use */
#define SS_ENTERING 2           /* outfitting the ship in newship() */
#define SS_LIVE 3               /* flying around */
#define SS_DYING 4              /* used inside kill() */
#define SS_DEAD 5               /* used between kill() and the user menu */
#define SS_RESERVED 6           /* reserved for our later use */

/* killedBy values.  The values should never exceed 255 since only a
 * uint8_t is used in the protocol.
 */
typedef enum {
    KB_NONE           = 0,   /* this must always be the first item, = 0 */
    KB_SHIP           = 1,   /* killed by a ship of course */
    KB_PLANET         = 2,   /* killed by a planet or sun */
    KB_SELF           = 3,   /* self-destruct */
    KB_NEGENB         = 4,   /* damage from negative energy barrier */
    KB_CONQUER        = 5,   /* you conquered the universe */
    KB_NEWGAME        = 6,   /* new game */
    KB_EVICT          = 7,   /* game closed for repairs */
    KB_SHIT           = 8,   /* placed on shit list */
    KB_DOOMSDAY       = 9,   /* the planet eater got you */
    KB_GOTDOOMSDAY    = 10,  /* you got the planet eater */
    KB_GOD            = 11,  /* killed by the Lord */
    KB_LIGHTNING      = 12,  /* killed by a lightning bolt from GOD */

    KB_MAX                   /* maximum KB_* number, must always be last */
} killedBy_t;

/* Course lock values.  The values should never exceed 255 since only
 * a uint8_t is used in the protocol.
 */
typedef enum {
    LOCK_NONE         = 0,
    LOCK_SHIP         = 1, // locked onto a ship (only doomsday for now)
    LOCK_PLANET       = 2, // locked onto a planet

    LOCK_MAX
} courseLock_t;

#define REFIT_GRAND 10000       /* millis to complete a refit */

#define TEAM_UPDATE_INTERVAL 60 /* update client team data every x seconds */
#define CONQINFO_UPDATE_INTERVAL 120
#define HISTORY_UPDATE_INTERVAL 60

// maximum updates per second
#define MAX_UPDATE_PER_SEC 10

#define MAX_LASTWORDS 60       /* maximum size of conquering remarks */

#define MAX_DATESIZE 20          /* hh:mm:ss ddmmmyy */

/* Tokens for slastmsg(), must be negative. */
#define LMSG_NEEDINIT -1        /* need to initialize slastmsg */
#define LMSG_READALL -2         /* can read all messages */

/* Tokens for mcuPutThing(), must be negative */
#define THING_EXPLOSION -1      /* a torpedo explosion */

/* Return values for utIsSpecial(). */
#define NEAR_SHIP 1
#define NEAR_PLANET 2
#define NEAR_NONE 3
#define NEAR_DIRECTION 4
#define NEAR_ERROR 5

/* Tokens for utIsSpecial() and clbFindSpecial() */
#define SPECIAL_NOTSPECIAL 1
#define SPECIAL_ENEMYSHIP 2
#define SPECIAL_FUELPLANET 3
#define SPECIAL_ENEMYPLANET 4
#define SPECIAL_SHIP 5
#define SPECIAL_PLANET 6
#define SPECIAL_TEAMSHIP 7
#define SPECIAL_ARMYPLANET 8
#define SPECIAL_WEAKPLANET 9
#define SPECIAL_TEAMPLANET 10
#define SPECIAL_REPAIRPLANET 11
#define SPECIAL_HOMEPLANET 12

/* Constants. */
#define BOMBARD_KILLS 0.05      /* kills per army bombarded */
#define KILLS_KILLS 0.10        /* extra kills per enemy kill */
#define ARMY_KILLS 0.10         /* extra kills per carried army killed */
#define PLANET_KILLS 0.25       /* kills per planet taken */
#define CONQUER_KILLS 3.0       /* kills per universe conquered */
#define DOOMSDAY_KILLS 5.0      /* kills per planet eater eaten */

#define MM_PER_SEC_PER_WARP 18.0 /* mega-meters per second per warp */
/*  distance = MM_PER_SEC_PER_WARP * warp * time */
#define MAX_ORBIT_WARP 2.0      /* maximum speed for entering orbit */
#define MAX_TRACTOR_WARP 4.0    /* maximum speed difference when tractoring */

#define ACCINFO_DIST 3000.0     /* maximum distance for accurate ship info */
#define YELLOW_DIST 3000.0
#define ALERT_DIST 2000.0       /* 2/3 of YELLOW_DIST */
#define SCANNED_FUSE 5          /* seconds of timeout for sscanned(,) */

#define PLANET_ORBIT_FAC 5.0    /* planet orbit speed in degrees per minute */
#define ENTRY_SMEAR_DIST 2500.0 /* standard deviation for rndnor for entry */
#define ENTRY_ENEMY_DIST ACCINFO_DIST
/* closest entry to an enemy ship */

#define ORBIT_FAC 10.0          /* orbit speed in degrees per second */

#define ORBIT_CW -1.0           /* warp factor for clockwise orbiting */
#define ORBIT_CCW -2.0          /* warp factor for counter-clockwise orbiting */

#define PLANET_DIST_OFF 200.0   /* distance from planets (planet size
                                 * + this offset) to get scanned, get
                                 * blasted, etc. */
#define PLANET_HIT 2.5          /* hits per hostile planet per second */
#define ARMY_HIT 0.08           /* extra hits per hostile army per second */

#define NEGENB_DIST 30000.0     /* distance to negative energy barrier */
#define NEGENBEND_DIST 33500.0  /* distance to outer edge of negenb */
#define NEGENB_HIT 5.0          /* hits per second inside negenb */

#define FUEL_FAC 2.5            /* fuel generated per second */
#define MPLANET_FUEL_MULT 5.0   /* extra fuel per second when orbiting a class m */
#define FUELWARP_FAC 0.5        /* fuel consumed per warp factor per second */
#define ENGINES_ON_FUEL 10.0    /* fuel consumed by turning on engines */
#define FUELSHIELDS_MULT 1.25   /* extra fuel used when cruising with shields up */
#define FUELWARPARMY_FAC 0.04   /* extra fuel per warp per army per second */

#define CLOAK_ON_FUEL 50.0      /* fuel used by turning on the cloaker */

#define CLOAK_WARP_FUEL 2.0     /* fuel used per warp per second while cloaked */

#define CLOAK_FUEL 1.0          /* fuel used per second while cloaked */

#define CLOAK_SMEAR_DIST 600.0  /* standard deviation of smearing while cloaked */

#define WEAPON_COOL_FAC 0.8     /* cool-down per second */
#define ENGINE_COOL_FAC 0.5     /* cool-down per second */

#define TEMPFUEL_FAC 0.25       /* heating per fuel unit used */

#define SHIELD_FAC 2.0          /* shield percents per second */

#define REPAIR_FAC 0.333333     /* damage percents per second */
#define RMODE_REPAIR_MULT 3.0   /* multiplier for repair when repairing */

#define PLANET_REPAIR_MULT 3.0  /* multiplier for repair when orbiting */
                                /*  a friendly planet */

#define REPOP_LOWER_BOUND 1     /* minimum births per repop */
#define REPOP_UPPER_BOUND 3     /* maximum births per repop */
#define SPARSE_THRESH 3         /* less than this has trouble repopping */
#define SPARSE_REPOP_PROB 0.5   /* probability of repop on sparse planets */
#define MALTHUS_M_THRESH 90     /* threshold for a class M Malthusian crisis */
#define MALTHUS_D_THRESH 70     /* threshold for a dead planet Malthusian crisis */
#define MALTHUS_PROB 0.2        /* probability of Malthusian crisis */
#define MIN_BEAM_ARMIES 3       /* you can't beam up armies past this */
#define MIN_BOMB_ARMIES 3       /* you can't bomb armies down past this */

#define WEAPON_DOWN_PROB 0.5    /* probability weapons breaking when overheated */
#define ENGINE_DOWN_PROB 0.5    /* probability engines breaking when overheated */
#define MIN_DOWN_FUSE 9         /* minimum weapon/engine down time */
#define MAX_DOWN_FUSE 36        /* minimum weapon/engine down time */

/* Various weapons constants. */
#define PHASER_DIST 1000.0      /* maximum range of phasers */
#define PHASER_HIT 80.0         /* blast from phasers at zero range */
#define PHASER_SPREAD 10.0      /* half-angle of phaser beam in degrees */
#define PHASER_FUEL 20.0        /* fuel required to fire phasers */
#define PHASER_TENTHS 10        /* tenths for phasers to re-charge */

#define DOUBLE_E_KILLS 50.0     /* kills required to double efficiency */
#define EXPLOSION_FALLOFF 28.0  /* explosions divided by this at PHASER_DIST */
#define EXPLOSION_RADIUS 50.0   /* things within this radius get maximum hits */

#define TORPEDO_FUSE 50         /* seconds a torp flies */

#define FIREBALL_FUSE 3         /* seconds a torp explodes */
#define TORPEDO_PROX 250.0      /* proximity fuse distance */
#define TORPEDO_HIT 72.0        /* blast from a torp at zero range */
#define TORPEDO_FUEL 10.0       /* fuel required to launch a torp */
#define TORPEDOWAIT_GRAND 5000  /* millis to wait for live torps in dead() */

#define DETONATE_DIST PHASER_DIST /* maximum range of detonators */
#define DETONATE_FUEL 5.0       /* fuel required to fire detonators */

#define BOMBARD_FUEL 10.0       /* fuel required to bombard per second */
#define BOMBARD_PROB 0.30       /* base probability of killing an army per try */
#define BOMBARD_GRAND 333       /* millis to get one chance to bomb */

#define BEAM_GRAND 2000         /* millis to beam one army */

#define TRACTOR_DIST 560.0      /* maximum range of tractor beams */
#define TOW_DIST 500.0          /* distance a towed ship trails by */
#define BREAKAWAY_PROB 0.3      /* chance to break free of a hostile tower */
#define BREAKAWAY_GRAND 1000    /* millis to break free from a hostile tower */

#define MIN_UNINHAB_MINUTES 45  /* minimum time till inhabitable */
#define MAX_UNINHAB_MINUTES 120 /* maximum time till inhabitable */

                                /* these are used to assign special
                                   ship numbers to represent non ships
                                   like the doomsday machine,
                                   deathstar, and any other watchable
                                   non-ship/planet items.  We start at
                                   -10 and go down for any new
                                   "devices" (-11, -12, etc). */
#define DISPLAY_DOOMSDAY (-10)   /* doomsday machine num */

/* JET - these were a bit high, 120 and 240 respectively. */
#define MIN_COUP_MINUTES 10     /* minimum time till coup in minutes */
#define MAX_COUP_MINUTES 45     /* maximum time till coup in minutes */

#define MIN_COUP_KILLS 3.0      /* minimum kills required before a coup */
#define MAX_COUP_ENEMY_ARMIES 20 /* maximum enemies allowable for coup to work */

#define MAP_SR_FAC 170.0        /* units per line in normal display
                                 * (short range view) */
#define MAP_LR_FAC 1400.0       /* units per line in map display (long
                                 * range view) */
#define WIDTH_FAC 2.32          /* columns to make one line */
#define DISPLAY_LINS 21         /* maximum lines used in display */
#define STAT_COLS 24            /* maximum columns used in stats */

#define ITER_SECONDS 0.1        /* seconds per minor iteration */
                                /*  in both conquest and conqdriv */
#define ITER_TENTHS 1           /* must agree with above, be an integer, */
                                /*  and an even divisor of 10 */
#define SELFDESTRUCT_FUSE 15    /* seconds to self-destruct */
#define REARM_GRAND 10000       /* millis to declaring war/peace */
#define COUP_GRAND 10000        /* millis to attempt a coup */
#define MIN_REFIT_KILLS 1.0     /* min kill to refit */

#define MSG_LIN1 (DISPLAY_LINS + 2)
#define MSG_LIN2 (MSG_LIN1 + 1) /* line for messages */
#define MSG_MSG  (MSG_LIN1 + 2)

#define TIMEOUT_DRIVER 10       /* seconds of timeout for the driver */
#define TIMEOUT_DRCHECK 2       /* don't do timeout checking after this */
#define TIMEOUT_PLAYER 300      /* seconds of timeout for players */
#define NEWMSG_GRAND 1900       /* millis before displaying a new message */

/* Terminators for dsgetx. */
#define TERM_NORMAL '\r'        /* like NEXT */
#define TERM_EXTRA  '\t'        /* like shift-NEXT or shift-HELP */
#define TERM_ABORT  '\033'      /* escape - like BACK */
#define TERMS "\r\t\033"        /* must agree with above characters! */

#define TERM_REDRAW '\014'      /* ^L Redraw/Refresh */
#define TERM_RELOAD '\022'      /* ^R reload data from server */

/* Magic characters */
#define CHAR_CLOAKED '~'        /* used to display self-cloaked ship */

#define MAILADDR ""

/* for clbLaunch() */
#define LAUNCH_NORMAL   (0)
#define LAUNCH_EXPLODE  (1)

/* exploding ship characteristics */

/* number of stationary torps 'fired'
   when a ship dies */
#define EXPLODESHIP_TORP_COUNT    6
/* Maximum distance from ship an 'explode
   ship' spread of torps will be located.
   ie: This confines the blast radius
   for an exploding ship */
#define EXPLODESHIP_TORP_SPREAD   15.0

#define DEFAULT_USEREXPIRE 120  /* days after which to expire a user */
#define SECS_PER_DAY (24 * 3600)

#define MTXT_MORE "--- press [SPACE] to continue, any other key to quit ---"
#define MTXT_DONE "--- press any key when done ---"

/* Macros, here order is important. */

#define iround(x) int(floor((x) + 0.5))      /* int round */

#define oneplace(x) (real)(round((x) * 10.0)/10.0) /* nearest tenth */
#define dtor(x) (((real)(x) / 180.0) * M_PI) /* degrees to radians */
#define rtod(x) (((real)(x) / M_PI) * 180.0) /* radians to degrees */
#define cosd(x) cos((real)dtor((x) )) /* cosine of angle in degrees */
#define sind(x) sin((real)dtor((x) )) /* sine of angle in degrees */
#define dist(w,x,y,z) sqrt(pow((real)((y)-(w)), 2.0) +  \
                           pow((real)((z)-(x)), 2.0))

/* fast distance */
#define distf(w,x,y,z) (fabs((real)(y)-(real)(w)) +     \
                        fabs((real)(z)-(real)(x)))
#define satwar(x,y) (cbShips[x].war[cbShips[y].team] || \
                     cbShips[y].war[cbShips[x].team])
#define selfwar(x) (cbShips[x].war[cbShips[x].team])
#define maxwarp(x) std::min((real)cbShipTypes[cbShips[x].shiptype].warpMax, \
                       std::max(0.0, round(((100.0 - cbShips[x].damage) / \
                                       100.0) * 12.0)))
#define engeff(x) (((cbShips[x].engalloc + 50.0) / 100.0) *     \
                   cbShipTypes[cbShips[x].shiptype].engfac *    \
                   (((cbShips[x].kills+cbShips[x].strkills) +   \
                     DOUBLE_E_KILLS) / DOUBLE_E_KILLS))
#define weaeff(x) (((cbShips[x].weapalloc+50.0)/100.0) *        \
                   cbShipTypes[cbShips[x].shiptype].weafac *    \
                   (((cbShips[x].kills+cbShips[x].strkills) +   \
                     DOUBLE_E_KILLS) / DOUBLE_E_KILLS))

/* clamp */
#undef CLAMP
#define CLAMP(_min, _max, _val) (((_val) < (_min)) ? (_min) : (((_val) > (_max)) ? (_max) : (_val)))

#if defined(SVR4) && !defined(sun)  /* Sun claims SVR4, but
                                       wants 2 args. */
#define GETTIMEOFDAY(_x) gettimeofday(_x)
#else
#define GETTIMEOFDAY(_x) gettimeofday(_x, NULL)
#endif

#define TIMEDELTA(dest, src1, src2) {                                   \
        if(((dest).tv_usec = (src1).tv_usec - (src2).tv_usec) < 0) {    \
            (dest).tv_usec += 1000000;                                  \
            (dest).tv_sec = (src1).tv_sec - (src2).tv_sec - 1;          \
        } else {                                                        \
            (dest).tv_sec = (src1).tv_sec - (src2).tv_sec;              \
        }                                                               \
    }




#endif /*  CONQDEF_H_INCLUDED */
