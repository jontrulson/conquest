
/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/*                              C O N Q C O M */
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

#ifndef CONQCOM_H
#define CONQCOM_H

#ifdef NOCOMEXTERN
#define CEXTERN 
#else
#define CEXTERN extern 
#endif

#include "user.h"
#include "ship.h"
#include "planet.h"
#include "team.h"
#include "msg.h"
#include "doomsday.h"

/* Special data items - these must be at the very beginning. */
/* int commonrev;			/* common block rev number */
CEXTERN int *commonrev;			/* common block rev number */

/* int closed;			/* the game is closed, everybody out */
CEXTERN int *closed;			/* the game is closed, everybody out */

/* int lockword;			/* exclusionary flag */
CEXTERN int *lockword;			/* exclusionary flag */

/* int lockmesg;			/* exclusive access to the message area */
CEXTERN int *lockmesg;			/* exclusive access to the message area */


/* Start of permanent data. */

    /* Global statistics. */
/* char conqueror[SIZEUSERPNAME];	/* user who last conquered */
CEXTERN char *conqueror;	/* user who last conquered */

/* char conqteam[MAXTEAMNAME];	/* team s/he was on */
CEXTERN char *conqteam;	/* team s/he was on */

/* char conqtime[DATESIZE];	/* time of last conquering */
CEXTERN char *conqtime;	/* time of last conquering */

/* char lastwords[MAXLASTWORDS];	/* remarks upon conquering */
CEXTERN char *lastwords;	/* remarks upon conquering */

/* char inittime[DATESIZE];	/* time the universe was initialized */
CEXTERN char *inittime;	/* time the universe was initialized */

    /* Hidden global statistics. */
/* int ccpuseconds;			/* cpu seconds used by conquest */
CEXTERN int *ccpuseconds;			/* cpu seconds used by conquest */

/* int celapsedseconds;		/* elapsed seconds for conquest */
CEXTERN int *celapsedseconds;		/* elapsed seconds for conquest */

/* int dcpuseconds;			/* cpu seconds used by conqdriv */
CEXTERN int *dcpuseconds;			/* cpu seconds used by conqdriv */

/* int delapsedseconds;		/* elapsed seconds for conqdriv */
CEXTERN int *delapsedseconds;		/* elapsed seconds for conqdriv */

/* int rcpuseconds;			/* cpu seconds used by robots */
CEXTERN int *rcpuseconds;			/* cpu seconds used by robots */

/* int relapsedseconds;		/* elapsed seconds for robots */
CEXTERN int *relapsedseconds;		/* elapsed seconds for robots */

/* int raccum;			/* accumulated cpu hundredths */
CEXTERN int *raccum;			/* accumulated cpu hundredths */

/* char lastupchuck[DATESIZE];	/* last time the common block was updated */
CEXTERN char *lastupchuck;	/* last time the common block was updated */

    /* User statistics. */

				/* per user data */
CEXTERN User_t *Users;		/* User structures */

/* End of permanent data. */

/* Start of static data. */

    /* Robots. */
/* int rstrat[REAL_MAX_VAR][10];	/* robot strategy table */
CEXTERN int **rstrat;	/* robot strategy table */

/* int rvec[32];			/* robot rule to action token */
CEXTERN int *rvec;			/* robot rule to action token */


    /* Planets. */

CEXTERN Planet_t *Planets;	/* planet structures */

/* char ptname[MAXPLANETTYPES][MAXPTYPENAME];*/
CEXTERN char **ptname;

    /* Teams. */

CEXTERN Team_t *Teams;

    /* Characters. */
/* char chrplanets[MAXPLANETTYPES];/* class M = M, dead = D, sun = S */
CEXTERN char *chrplanets;/* class M = M, dead = D, sun = S */

    /* Doomsday machine. */

CEXTERN Doomsday_t *Doomsday;

/* End of semi-static data. */

/* Start of semi-volatile data. */

    /* Last usage stuff. */
/* int histptr;			/* points to most recent usage */
CEXTERN int *histptr;			/* points to most recent usage */

/* int histunum[MAXHISTLOG];	/* user number */
CEXTERN int *histunum;	/* user number */

/* char histlog[MAXHISTLOG][DATESIZE]; /* date string of entry */
CEXTERN char **histlog; /* date string of entry */

/* End of semi-volatile data. */

/* Start of volatile data. */

    /* Driver. */
/* int drivstat;			/* driver status; one of the DRS_ values */
CEXTERN int *drivstat;			/* driver status; one of the DRS_ values */

/* int drivpid;			/* pid of the driver process */
CEXTERN int *drivpid;			/* pid of the driver process */

/* int drivcnt;			/* number of driver timeouts (modp1 1000) */
CEXTERN int *drivcnt;			/* number of driver timeouts (modp1 1000) */

/* int drivsecs;			/* phase of the universe in seconds */
CEXTERN int *drivsecs;			/* phase of the universe in seconds */

					/*  (modp1 five minutes) */
/* char drivowner[SIZEUSERNAME];	/* username of the driver's owner */
CEXTERN char *drivowner; /* username of the driver's owner */

    /* The following are seconds since midnight. */
/* int drivtime;			/* last time the driver looped */
CEXTERN int *drivtime;			/* last time the driver looped */

/* int playtime;			/* last time a player looped */
CEXTERN int *playtime;			/* last time a player looped */

    /* Ships. */

CEXTERN Ship_t *Ships;		/* ship structures */

    /* Messages. */

CEXTERN Msg_t *Msgs;

/* int lastmsg;			/* the last message sent */
CEXTERN int *lastmsg;			/* the last message sent */

/* int externrobots;		/* used to disable internal robot code */
CEXTERN int *externrobots;		/* used to disable internal robot code */

    /* Note: glastmsg must be the last variable in the common block. */
/* int glastmsg;		/* the last message read by God */
CEXTERN int *glastmsg;		/* the last message read by God */

/* End of volatile data. */

#undef CEXTERN

#endif /* CONQCOM_H */
