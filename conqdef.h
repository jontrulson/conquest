
/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                              C O N Q D E F */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */


/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#ifndef CONQDEF_H_INCLUDED
#define CONQDEF_H_INCLUDED

/* 4.22 05Sep86 CAL .*(): Removed tutorial feature. Made tuning constants */
/*                   constants again. Increased username sizes. */
/*                  .userstats(): Tweak heading. */
/*                  .dowarp(): Don't need to fixdeltas(). */
/*                  .executeai(): Stop repair mode when setting warp. */
/*                  .chalkup(): New routine to help with kills accounting. */
/*                  .ikill(): Fixed longstanding mutual kill bug. */
/*                  .cloak(): Charge cloaking engage as engine energy use. */
/* 4.21 13Aug86 CAL .Conqinit(): Use t_getbpri(). */
/* 4.20 22Jun86 CAL .Getamsg(): Don't read messages from us to GOD. */
/* 4.19 03Nov85 CAL .Statline(): Tweak cpu printout. */
/* 4.18 30Oct85 CAL .Secdrive(): Added some death checks from stillalive(). */
/*                   Moved robot stats code to robotai(). */
/* 4.17 29Oct85 CAL .*(): Add snum arguments to stillalive(), remove user check. */
/*                  .Robotai(): Use stillalive(). */
/*                  .Kiss(): Add "all" option. */
/* 4.16 17Oct85 CAL .*(): Add seperate robot cpu and elapsed stats. */
/* 4.15 09Oct85 CAL .Display(): Fixed weird yellow alert bug. */
/* 4.14 06Oct85 CAL .Oprobot(): Add multiple robot creation. */
/*                  .Operate(), watch(): Get msg timestamp after action switch. */
/*                  .Putship(): Increase smear factor slightly. */
/*                  .*(): Converted maxwarp(), engeff() and weaeff() to macros. */

/* Common block time stamp. */
/* This define should be changed every time the common block changes. */
/*#define COMMONSTAMP 861104 		 stardate? */
/*#define COMMONSTAMP 940910 		 stardate? */
/*#define COMMONSTAMP 961019 		 stardate? */
/*#define COMMONSTAMP 971207 		 stardate? */
/*#define COMMONSTAMP 980628 		 stardate? */
/*#define COMMONSTAMP 990124 		 stardate? */
#define COMMONSTAMP 20001231 		 /* stardate? */

/* Copyright notice string. */
#define COPYRIGHT "(C) 1983-1986 by Jef Poskanzer and Craig Leres"

#ifndef PI
# define PI 3.141592654
#endif

#define MAXSHIPS 20 		/* number of available ships */

  /* Values for sstatus() */
#define SS_OFF 1 		/* available for use */
#define SS_ENTERING 2 		/* outfitting the ship in newship() */
#define SS_LIVE 3 		/* flying around */
#define SS_DYING 4 		/* used inside kill() */
#define SS_DEAD 5 		/* used between kill() and the user menu */
#define SS_RESERVED 6 		/* reserved for our later use */

  /* Special values for skilledby() */
#define KB_SELF -100 		/* self-destruct */
#define KB_NEGENB -101 	/* damage from negative energy barrier */
#define KB_CONQUER -102 	/* you conquered the universe */
#define KB_NEWGAME -103 	/* new game */
#define KB_EVICT -104 		/* game closed for repairs */
#define KB_SHIT -105 		/* placed on shit list */
#define KB_DOOMSDAY -106 	/* the planet eater got you */
#define KB_GOTDOOMSDAY -107 	/* you got the planet eater */
#define KB_GOD -108 		/* killed by the Lord */
#define KB_DEATHSTAR -109 	/* killed by a Lucas device */
#define KB_LIGHTNING -110 	/* killed by a lightning bolt from GOD */

#define MAXTORPS 9 		/* maximum torps per ship */
  /* Values for tstatus() */
#define TS_OFF 1 		/* ready for use */
#define TS_LAUNCHING 2 	/* being created */
#define TS_LIVE 3 		/* armed and flying */
#define TS_DETONATE 4 		/* detonate signal sent */
#define TS_FIREBALL 5 		/* exploding, needed for display() */

#define MAXSTNAME 32
#define MAXNUMSHIPTYPES 3
#define ST_SCOUT      0		/* scout vessel (Ori) */
#define ST_DESTROYER  1		/* Destroyer (Fed/Kli) */
#define ST_CRUISER    2		/* Cruiser (Rom) */

#define REFIT_GRAND 10000       /* hunderths to complete a refit */

#define NUMALLTEAMS 8 		/* total number of teams */
#define MAXTEAMNAME 12 
#define TEAM_FEDERATION 0 	/* Feddie Bears */
#define TEAM_ROMULAN 1 	/* Rom Dogs */
#define TEAM_KLINGON 2 	/* Klings */
#define TEAM_ORION 3 		/* Bugs */
#define NUMPLAYERTEAMS 4 		/* four "real" teams */
#define TEAM_SELFRULED 4 
#define TEAM_NOTEAM 5 
#define TEAM_GOD 6 
#define TEAM_EMPIRE 7

#define MAXPLANETTYPES 8 	/* number of different planet types */
#define MAXPTYPENAME 20 		/* maximum size of a planet type's name */
#define MAXPLANETNAME 12 	/* maximum size of a planet's name */
#define PLANET_CLASSM 1 	/* "Sensors indicate a Class M planet..." */
#define PLANET_DEAD 2 
#define PLANET_SUN 3 		/* a bit of a contradiction... */
#define PLANET_MOON 4 		/* a bit of fun */
#define PLANET_GHOST 5 	/* anchor for binary systems */
#define PLANET_CLASSA 6 
#define PLANET_CLASSO 7 
#define PLANET_CLASSZ 0 

				/* Number of planets with specific
				   identities within conquest.  This
				   will always be 40 for posterity */
#define NUM_BASEPLANETS 40

                                /* This is the number of 'extra' planets
				   to be used for whatever.  None of these
				   can be core planets, and will be
				   initialized to invisible, unreal,
				   with unimaginative names like Extra 1
				   etc... Eventually, these planets
				   will be initializable in the sys-wide
				   config file.  REMEMBER: If you change
				   this number, change the common block id
				   (COMMONSTAMP) to reflect the change. Also
				   make sure you don't choose a number large
				   enough to overflow the common block
				   (SIZEOF_COMMONBLOCK) */

#define NUM_EXTRAPLANETS 20

				/* number of planets */
#define NUMPLANETS (NUM_BASEPLANETS + NUM_EXTRAPLANETS)

  /* Planet numbers. */
#define PNUM_EARTH 1
#define PNUM_ROMULUS 2 
#define PNUM_KLINGUS 3 
#define PNUM_ORION 4 
#define PNUM_REMUS 5 
#define PNUM_LEUDUS 6 
#define PNUM_OBERON 7 
#define PNUM_TELOS 8 
#define PNUM_TARSUS 9 
#define PNUM_UMBRIEL 10 
#define PNUM_OMEGA 11 
#define PNUM_RHO 12 
#define PNUM_JANUS 13 
#define PNUM_ELAS 14 
#define PNUM_SHERMAN 15 
#define PNUM_SERITIL 16 
#define PNUM_CHERON 17 
#define PNUM_DAKEL 18 
#define PNUM_SARAC 19 
#define PNUM_VENAR 20 
#define PNUM_XIDEX 21 
#define PNUM_OLDAR 22 
#define PNUM_EMINIAR 23 
#define PNUM_DYNEB 24 
#define PNUM_RIGELB 25 

#define NUMCONPLANETS 25 	/* last planet need to conquer the universe */

#define PNUM_LUNA 26 
#define PNUM_ALTAIR 27 
#define PNUM_HELL 28 
#define PNUM_JINX 29 

  /* Sun numbers. */
#define PNUM_SOL 30 
#define PNUM_SIRIUS 31 
#define PNUM_KEJELA 32 
#define PNUM_BETELGEUSE 33 
#define PNUM_MURISAK 34 
#define PNUM_SYRINX 35 

  /* Ghost numbers. */
#define PNUM_GHOST1 36 
#define PNUM_GHOST2 37 
#define PNUM_GHOST3 38 
#define PNUM_GHOST4 39 
#define PNUM_GHOST5 40		/* LAST 'base' planet, any after this number
				   are extra */


#define MAXUSERS 500 		/* maximum number of registered users */
#define MAXUSERNAME 30 		/* maximum size of a user's login name */
#define SIZEUSERNAME 32 		/* actual size (for alignment purposes) */
#define MAXUSERPNAME 22 		/* maximum size of a user's pseudonym */
#define SIZEUSERPNAME 24 	/* actual size (for alignment purposes) */
#define MAXLASTWORDS 60 		/* maximum size of conquering remarks */
#define MAXHISTLOG 40 		/* maximum number of user history entries */

#define MAXTSTATS 20 
#define TSTAT_SECONDS 1 	/* elapsed seconds for each team */
#define TSTAT_CPUSECONDS 2 	/* cpu seconds used per team */
#define TSTAT_WINS 3 		/* wins for each team */
#define TSTAT_LOSSES 4		/* losses for each team */
#define TSTAT_ENTRIES 5 	/* number of entries for each team */
#define TSTAT_CONQUERS 6 	/* number of conquers for each team */
#define TSTAT_COUPS 7 		/* number of coups completed */
#define TSTAT_TORPS 8 		/* number of entries for each team */
#define TSTAT_PHASERS 9 	/* number of entries for each team */
#define TSTAT_ARMBOMB 10 	/* armies killed by bombing */
#define TSTAT_ARMSHIP 11 	/* armies killed on a ship */
#define TSTAT_CONQPLANETS 12 	/* number of planets conquered */
#define TSTAT_GENOCIDE 0 	/* number of genocides completed */

#define MAXUSTATS 20 
#define USTAT_SECONDS 1 	/* elapsed seconds for each user */
#define USTAT_CPUSECONDS 2 	/* cpu seconds used per user */
#define USTAT_WINS 3 		/* total kills for each user */
#define USTAT_LOSSES 4		/* total losses for each user */
#define USTAT_MAXKILLS 5 	/* maximum kills for each user */
#define USTAT_ENTRIES 6 	/* number of entries for each user */
#define USTAT_CONQUERS 7 	/* number conquers for each player */
#define USTAT_COUPS 8 		/* number of coups completed */
#define USTAT_TORPS 9 		/* number of torps fired for each user */
#define USTAT_PHASERS 10 	/* number of phaser shots for each user */
#define USTAT_ARMBOMB 11 	/* armies killed by bombing */
#define USTAT_ARMSHIP 12 	/* armies killed on a ship */
#define USTAT_CONQPLANETS 13 	/* number of planets conquered */
#define USTAT_GENOCIDE 0 	/* number of genocides completed */

#define MAXOOPTIONS 12 		/* number of operator options */
#define OOPT_PLAYWHENCLOSED 0 	/* can play when the game is closed */
#define OOPT_MULTIPLE 1 	/* can play on more than one terminal */
#define OOPT_SWITCHTEAMS 2 	/* can switch teams */
#define OOPT_SHITLIST 3 	/* can't play at all */
#define OOPT_GODMSG 4 		/* read messages send to GOD */
#define OOPT_LOSE 5 		/* heh, heh, heh... */
#define OOPT_AUTOPILOT 6 	/* can use the autopilot */

#define MAXOPTIONS 12 		/* number of user options */
#define OPT_PHASERGRAPHICS 0 	/* show phaser shots */
#define OPT_PLANETNAMES 1 	/* show planet names on the display */
#define OPT_ALARMBELL 2 	/* beep the bell when interesting things happen */
#define OPT_INTRUDERALERT 3 	/* show intruder alert messages */
#define OPT_NUMERICMAP 4 	/* show army counts on map */
#define OPT_TERSE 5 		/* terse more prompts */
#define OPT_EXPLOSIONS 6 	/* show torpedo explosions */

#define DATESIZE 20 		/* hh:mm:ss ddmmmyy */
#define MAXMESSAGES 60 		/* maximum number of messages */
#define MESSAGE_SIZE 70		/* maximum size of messages */
#define MSGMAXLINE 90 		/* maximum size for formatting messages */
#define MSG_ALL -100 		/* message to everyone */
#define MSG_NOONE -101		/* message from/to no one */
#define MSG_GOD -102 		/* message from/to GOD */
#define MSG_DOOM -103 		/* message from the planet eater */
#define MSG_OUTSIDE -104 	/* message from the outside world */
#define MSG_IMPLEMENTORS -105 	/* message to the implementors */
#define MSG_COMP -106 		/* message from the ship's computer */
#define MSG_FRIENDLY -107	/* message to friendly ships */

/* Tokens for slastmsg(), must be negative. */
#define LMSG_NEEDINIT -1 	/* need to initialize slastmsg */
#define LMSG_READALL -2		/* can read all messages */

/* Driver states. */
#define DRS_OFF 0 		/* normal off mode */
#define DRS_RESTART 1 		/* required state for starting a driver */
#define DRS_STARTING 2 		/* transitory starting state */
#define DRS_RUNNING 3 		/* normal running mode */
#define DRS_HOLDING 4 		/* don't execute by will of GOD */
#define DRS_KAMIKAZE 5 		/* used to kill a driver */

/* States for the planet eater */
#define DS_LIVE 1 
#define DS_OFF 2 

/* Phases of a moon. */
#define PHOON_NO -1 		/* doesn't have a phase */
#define PHOON_LAST 0 		/* last quarter */
#define PHOON_NEW 1 		/* new */
#define PHOON_FIRST 2 		/* first quarter */
#define PHOON_FULL 3 		/* full */

/* Tokens for puthing(), must be negative */
#define THING_EXPLOSION -1 	/* a torpedo explosion */
#define THING_DEATHSTAR -2 	/* the Empire's new toy */

/* Return values for special(). */
#define NEAR_SHIP 1 
#define NEAR_PLANET 2 
#define NEAR_NONE 3 
#define NEAR_DIRECTION 4 
#define NEAR_ERROR 5 

/* Tokens for special() and findspecial() */
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

/* Robot variables. */
#define VAR_FUEL 0 
#define VAR_SHIELDS 1 
#define VAR_NUMTORPS 2 
#define VAR_DAMAGE 3 
#define VAR_ETEMP 4 
#define VAR_WTEMP 5 
#define VAR_WARP 6 
#define VAR_SHUP 7 
#define VAR_WALLOC 8 
#define VAR_ORBITING 9 
#define VAR_REPAIRING 10 
#define VAR_CLOAKED 11 
#define VAR_CANREAD 12 
#define VAR_RANDOM 13 
#define VAR_DNE 14 
#define VAR_ENEMYCLOAKED 15
#define VAR_ENEMYDAMAGE 16 
#define VAR_PHASERDAM 17 
#define VAR_TORPDAM 18 
#define VAR_INCOMING 19 
#define MAX_VAR 20 
#define REAL_MAX_VAR 32		/* common block max (leaves room for expansion) */

/* Robot action tokens. */
#define ROB_NOOP 1 		/* do no-thing */
#define ROB_GOHOME 2 		/* head for our home planet */
#define ROB_GOREPAIR 3 		/* head for any planet */
#define ROB_ALLOCATE 4 		/* change engines/weapons allocation */
#define ROB_DETONATE 5 		/* detonate enemy torpedos */
#define ROB_MYDETONATE 6 	/* detonate my torpedos */
#define ROB_PHASER 7 		/* fire phasers */
#define ROB_TORPEDO 8 		/* fire a torpedo */
#define ROB_BURST 9 		/* fire a torpedo burst */
#define ROB_SHIELD 10 		/* raise or lower shields */
#define ROB_UNTRACTOR 11 	/* attempt to untractor */
#define ROB_WARP_0 12 		/* change speed */
#define ROB_WARP_2 13 		/* change speed */
#define ROB_WARP_5 14 		/* change speed */
#define ROB_WARP_8 15 		/* change speed */
#define ROB_TRACK 16 		/* go after an enemy */
#define ROB_SILENT 17 		/* slient running */
#define ROB_MESSAGE 18 		/* send a message */
#define ROB_TAKEDRUGS 19 	/* space out */
#define ROB_REPAIR 20 		/* enter repair mode */
#define ROB_READMSG 21 		/* read (and possibly reply to) a message */
#define ROB_INSULT 22 		/* send a gratuitous insult */
#define ROB_GOFUEL 23 		/* head for any class M planet */
#define ROB_RUNAWAY 24 		/* go away from the nearest enemy */


/* Constants. */
#define BOMBARD_KILLS 0.05 	/* kills per army bombarded */
#define KILLS_KILLS 0.10 	/* extra kills per enemy kill */
#define ARMY_KILLS 0.10		/* extra kills per carried army killed */
#define PLANET_KILLS 0.25 	/* kills per planet taken */
#define CONQUER_KILLS 3.0 	/* kills per universe conquered */
#define DOOMSDAY_KILLS 5.0 	/* kills per planet eater eaten */

#define MM_PER_SEC_PER_WARP 18.0 /* mega-meters per second per warp */
				/*  distance = MM_PER_SEC_PER_WARP * warp * time */
#define MAX_ORBIT_WARP 2.0 	/* maximum speed for entering orbit */
#define MAX_TRACTOR_WARP 4.0 	/* maximum speed difference when tractoring */

#define ACCINFO_DIST 3000.0 	/* maximum distance for accurate ship info */
#define YELLOW_DIST 3000.0 
#define ALERT_DIST 2000.0 	/* 2/3 of YELLOW_DIST */
#define SCANNED_FUSE 5 		/* seconds of timeout for sscanned(,) */

#define PLANET_ORBIT_FAC 5.0 	/* planet orbit speed in degrees per minute */
#define ENTRY_SMEAR_DIST 2500.0	/* standard deviation for rndnor for entry */
#define ENTRY_ENEMY_DIST ACCINFO_DIST 
				/* closest entry to an enemy ship */

#define ORBIT_FAC 10.0 		/* orbit speed in degrees per second */
#define ORBIT_DIST 300.0 	/* distance from planets to enter orbit and orbit */
#define ORBIT_CW -1.0 		/* warp factor for clockwise orbiting */
#define ORBIT_CCW -2.0 		/* warp factor for counter-clockwise orbiting */
#define PLANET_DIST 500.0 	/* distance from planets scan, get blasted, etc. */
#define PLANET_HIT 2.5 		/* hits per hostile planet per second */
#define ARMY_HIT 0.08 		/* extra hits per hostile army per second */

#define NEGENB_DIST 30000.0 	/* distance to negative energy barrier */
#define NEGENBEND_DIST 33500.0 	/* distance to outer edge of negenb */
#define NEGENB_HIT 5.0 		/* hits per second inside negenb */

#define FUEL_FAC 2.5 		/* fuel generated per second */
#define MPLANET_FUEL_MULT 5.0 	/* extra fuel per second when orbiting a class m */
#define FUELWARP_FAC 0.5 	/* fuel consumed per warp factor per second */
#define ENGINES_ON_FUEL 10.0 	/* fuel consumed by turning on engines */
#define FUELSHIELDS_MULT 1.25 	/* extra fuel used when cruising with shields up */
#define FUELWARPARMY_FAC 0.04 	/* extra fuel per warp per army per second */

#define CLOAK_ON_FUEL 50.0 	/* fuel used by turning on the cloaker */

#define CLOAK_WARP_FUEL 2.0 	/* fuel used per warp per second while cloaked */

#define CLOAK_FUEL 1.0 		/* fuel used per second while cloaked */

#define CLOAK_SMEAR_DIST 600.0 	/* standard deviation of smearing while cloaked */

#define WEAPON_COOL_FAC 0.8 	/* cool-down per second */
#define ENGINE_COOL_FAC 0.5 	/* cool-down per second */

#define TEMPFUEL_FAC 0.25 	/* heating per fuel unit used */

#define SHIELD_FAC 2.0 		/* shield percents per second */

#define REPAIR_FAC 0.333333 	/* damage percents per second */
#define RMODE_REPAIR_MULT 3.0 	/* multiplier for repair when repairing */

#define PLANET_REPAIR_MULT 3.0 	/* multiplier for repair when orbiting */
				/*  a friendly planet */

#define REPOP_LOWER_BOUND 1 	/* minimum births per repop */
#define REPOP_UPPER_BOUND 3 	/* maximum births per repop */
#define SPARSE_THRESH 3		/* less than this has trouble repopping */
#define SPARSE_REPOP_PROB 0.5 	/* probability of repop on sparse planets */
#define MALTHUS_M_THRESH 90 	/* threshold for a class M Malthusian crisis */
#define MALTHUS_D_THRESH 70 	/* threshold for a dead planet Malthusian crisis */
#define MALTHUS_PROB 0.2 	/* probability of Malthusian crisis */
#define MIN_BEAM_ARMIES 3 	/* you can't beam up armies past this */
#define MIN_BOMB_ARMIES 3 	/* you can't bomb armies down past this */

#define WEAPON_DOWN_PROB 0.5 	/* probability weapons breaking when overheated */
#define ENGINE_DOWN_PROB 0.5 	/* probability engines breaking when overheated */
#define MIN_DOWN_FUSE 9		/* minimum weapon/engine down time */
#define MAX_DOWN_FUSE 36 	/* minimum weapon/engine down time */

/* Various weapons constants. */
#define PHASER_DIST 1000.0 	/* maximum range of phasers */
#define PHASER_HIT 80.0		/* blast from phasers at zero range */
#define PHASER_SPREAD 10.0 	/* half-angle of phaser beam in degrees */
#define PHASER_FUEL 20.0 	/* fuel required to fire phasers */
#define PHASER_TENTHS 10 	/* tenths for phasers to re-charge */

/*#define DOUBLE_E_KILLS 50.0  */	/* kills required to double efficiency */
#define DOUBLE_E_KILLS 40.0 	/* kills required to double efficiency -JET */
#define EXPLOSION_FALLOFF 28.0 	/* explosions divided by this at PHASER_DIST */
#define EXPLOSION_RADIUS 50.0 	/* things within this radius get maximum hits */

#define TORPEDO_FUSE 50		/* seconds a torp flies */

#define FIREBALL_FUSE 3		/* seconds a torp explodes */
#define TORPEDO_PROX 250.0 	/* proximity fuse distance */
#define TORPEDO_HIT 72.0 	/* blast from a torp at zero range */
#define TORPEDO_FUEL 10.0 	/* fuel required to launch a torp */
#define TORPEDOWAIT_GRAND 5000 	/* hundreths to wait for live torps in dead() */

#define DETONATE_DIST PHASER_DIST /* maximum range of detonators */
#define DETONATE_FUEL 5.0 	/* fuel required to fire detonators */

#define BOMBARD_FUEL 10.0 	/* fuel required to bombard per second */
#define BOMBARD_PROB 0.60	/*was .30 - base probability of killing an army per try */
#define BOMBARD_GRAND 333 	/* hundreths to get one chance to bomb */

#define BEAM_GRAND 2000		/* hundreths to beam one army */

#define TRACTOR_DIST 560.0 	/* maximum range of tractor beams */
#define TOW_DIST 500.0 		/* distance a towed ship trails by */
#define BREAKAWAY_PROB 0.3 	/* chance to break free of a hostile tower */
#define BREAKAWAY_GRAND 1000 	/* hundreths to break free from a hostile tower */

#define DOOMSDAY_START_DIST 20000.0 /* distance from the center that it starts at */
#define DOOMSDAY_LENGTH 800.0 	/* length for display purposes */
#define DOOMSDAY_WARP 2.0 	/* warp factor of the planet eater */
#define DOOMSDAY_DIST 1500.0 	/* distance it starts shooting */
#define DOOMSDAY_KILL_DIST 300.0 /* minimum distance for killing it */
#define DOOMSDAY_HIT 7.0 	/* blast per second */
#define DOOMSDAY_PROB 0.0001 	/* probability per minute that it appears */
#define MIN_UNINHAB_MINUTES 45 	/* minimum time till inhabitable */
#define MAX_UNINHAB_MINUTES 120	/* maximum time till inhabitable */

				/* these are used to assign special
				   ship numbers to represent non
				   ships like the doomsday machine,
				   deathstar, and any other watchable
				   non-ship/planet items */
#define SPECIAL_BASE (MAXSHIPS + NUMPLANETS + 1)   
#define DISPLAY_DOOMSDAY ((SPECIAL_BASE + 1) * -1)   /* doomsday machine num */
#define DISPLAY_DEATHSTAR ((SPECIAL_BASE + 2) * -1)

/* JET - these were a bit high, 120 and 240 respectively. */
#define MIN_COUP_MINUTES 10 	/* minimum time till coup in minutes */
#define MAX_COUP_MINUTES 90 	/* maximum time till coup in minutes */

#define MIN_COUP_KILLS 3.0 	/* minimum kills required before a coup */
#define MAX_COUP_ENEMY_ARMIES 20 /* maximum enemies allowable for coup to work */

#define SCALE_FAC 170.0		/* units per line in normal display */
#define MAP_FAC 1400.0 		/* units per line in map display */
#define WIDTH_FAC 2.32 		/* columns to make one line */
#define DISPLAY_LINS 21		/* maximum lines used in display */
#define STAT_COLS 24 		/* maximum columns used in stats */

#define ITER_SECONDS 0.1 	/* seconds per minor iteration */
				/*  in both conquest and conqdriv */
#define ITER_TENTHS 1 		/* must agree with above, be an integer, */
				/*  and an even divisor of 10 */
#define SELFDESTRUCT_FUSE 15 	/* seconds to self-destruct */
#define REARM_GRAND 10000 	/* hunderths to declaring war/peace */
#define COUP_GRAND 10000 	/* hunderths to attempt a coup */
#define MIN_REFIT_KILLS 1.0     /* min kill to refit */

#define MSG_LIN1 (DISPLAY_LINS + 2) 
#define MSG_LIN2 (MSG_LIN1 + 1)	/* line for messages */

#define TIMEOUT_DRIVER 10 	/* seconds of timeout for the driver */
#define TIMEOUT_DRCHECK 2 	/* don't do timeout checking after this */
#define TIMEOUT_PLAYER 300 	/* seconds of timeout for players */
#define NEWMSG_GRAND 1900 	/* hundreths before displaying a new message */

/* Terminators for dsgetx. */
#define TERM_NORMAL '\r' 	/* like NEXT */
#define TERM_EXTRA  '\t' 	/* like shift-NEXT or shift-HELP */
#define TERM_ABORT  '\033' 	/* escape - like BACK */
#define TERMS "\r\t\033" 	/* must agree with above characters! */

#define TERM_REDRAW '\014'	/* Redraw/Refresh */

/* Magic characters */
#define CHAR_CLOAKED '~' 	/* used to display self-cloaked ship */

/* Actual size of common block */
#define SIZEOF_COMMONBLOCK 262144 /* New and Improved! */

#define MAILADDR ""

				/* for launch() */
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

#define NOWSIZE 8		/* size of now[] arrays */
#define DEFAULT_USEREXPIRE 120	/* days after which to expire a user */
#define SECS_PER_DAY (24 * 3600)

#define MTXT_MORE "--- press [SPACE] to continue, any other key to quit ---"
#define MTXT_DONE "--- press any key when done ---"

				/* validuser types */
#define UT_LOCAL (0)
#define UT_REMOTE (1)

/* Macros, here order is important. */

#define round(x) ((int)rint((real)(x)))	/* int round */

#define around(x) ((real) rint((real)(x))) /* real round */

#define oneplace(x) (real)(around((x) * 10.0)/10.0) /* nearest tenth */
#define dtor(x) (((real)(x) / 180.0) * PI) /* degrees to radians */
#define rtod(x) (((real)(x) / PI) * 180.0) /* radians to degrees */
#define cosd(x) (real)cos((real)dtor((x) )) /* cosine of angle in degrees */
#define sind(x) (real)sin((real)dtor((x) )) /* sine of angle in degrees */
#define dist(w,x,y,z) (real) sqrt(pow((real)((y)-(w)), (real) 2.0) + pow((real)((z)-(x)), (real) 2.0))

				/* fast distance */
#define distf(w,x,y,z) (real)(fabs((real)(y)-(real)(w)) + fabs((real)(z)-(real)(x)))
#define vowel(x) (x=='a' || x=='e' || x=='i' || x=='o' || x=='u' )
#define satwar(x,y) (Ships[x].war[Ships[y].team] || Ships[y].war[Ships[x].team])
#define selfwar(x) Ships[x].war[Ships[x].team]
#define maxwarp(x) (real)min(ShipTypes[Ships[x].shiptype].warplim, \
			     max(0.0, around(((100.0-Ships[x].damage)/100.0)*12.0)))
#define engeff(x) (real)(((Ships[x].engalloc +50.0)/100.0) * \
			 ShipTypes[Ships[x].shiptype].engfac * \
			 (((Ships[x].kills+Ships[x].strkills)+DOUBLE_E_KILLS)/DOUBLE_E_KILLS))
#define weaeff(x) (real)(((Ships[x].weapalloc+50.0)/100.0) * \
			 ShipTypes[Ships[x].shiptype].weafac * \
			 (((Ships[x].kills+Ships[x].strkills)+DOUBLE_E_KILLS)/DOUBLE_E_KILLS))

#endif /*  CONQDEF_H_INCLUDED */
