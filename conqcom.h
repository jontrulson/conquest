
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

/* Special data items - these must be at the very beginning. */
/*CEXTERN    int commonrev;			/* common block rev number */
CEXTERN    int *commonrev;			/* common block rev number */

/*CEXTERN    int closed;			/* the game is closed, everybody out */
CEXTERN    int *closed;			/* the game is closed, everybody out */

/*CEXTERN    int lockword;			/* exclusionary flag */
CEXTERN    int *lockword;			/* exclusionary flag */

/*CEXTERN    int lockmesg;			/* exclusive access to the message area */
CEXTERN    int *lockmesg;			/* exclusive access to the message area */


/* Start of permanent data. */

    /* Global statistics. */
/*CEXTERN    char conqueror[SIZEUSERPNAME];	/* user who last conquered */
CEXTERN    char *conqueror;	/* user who last conquered */

/*CEXTERN    char conqteam[MAXTEAMNAME];	/* team s/he was on */
CEXTERN    char *conqteam;	/* team s/he was on */

/*CEXTERN    char conqtime[DATESIZE];	/* time of last conquering */
CEXTERN    char *conqtime;	/* time of last conquering */

/*CEXTERN    char lastwords[MAXLASTWORDS];	/* remarks upon conquering */
CEXTERN    char *lastwords;	/* remarks upon conquering */

/*CEXTERN    char inittime[DATESIZE];	/* time the universe was initialized */
CEXTERN    char *inittime;	/* time the universe was initialized */

/*CEXTERN    int tstats[NUMTEAMS][MAXTSTATS];	/* random team statistics */
CEXTERN    int **tstats;	/* random team statistics */

/*CEXTERN    int tcoupinfo[NUMTEAMS];		/* knows coup time for own team */
CEXTERN    int *tcoupinfo;		/* knows coup time for own team */

    /* Hidden global statistics. */
/*CEXTERN    int ccpuseconds;			/* cpu seconds used by conquest */
CEXTERN    int *ccpuseconds;			/* cpu seconds used by conquest */

/*CEXTERN    int celapsedseconds;		/* elapsed seconds for conquest */
CEXTERN    int *celapsedseconds;		/* elapsed seconds for conquest */

/*CEXTERN    int dcpuseconds;			/* cpu seconds used by conqdriv */
CEXTERN    int *dcpuseconds;			/* cpu seconds used by conqdriv */

/*CEXTERN    int delapsedseconds;		/* elapsed seconds for conqdriv */
CEXTERN    int *delapsedseconds;		/* elapsed seconds for conqdriv */

/*CEXTERN    int rcpuseconds;			/* cpu seconds used by robots */
CEXTERN    int *rcpuseconds;			/* cpu seconds used by robots */

/*CEXTERN    int relapsedseconds;		/* elapsed seconds for robots */
CEXTERN    int *relapsedseconds;		/* elapsed seconds for robots */

/*CEXTERN    int raccum;			/* accumulated cpu hundredths */
CEXTERN    int *raccum;			/* accumulated cpu hundredths */

/*CEXTERN    char lastupchuck[DATESIZE];	/* last time the common block was updated */
CEXTERN    char *lastupchuck;	/* last time the common block was updated */

    /* User statistics. */

				/* per user data */
CEXTERN User_t *Users;		/* User structures */

/* End of permanent data. */

/* Start of static data. */

    /* Robots. */
/*CEXTERN    int rstrat[REAL_MAX_VAR][10];	/* robot strategy table */
CEXTERN    int **rstrat;	/* robot strategy table */

/*CEXTERN    int rvec[32];			/* robot rule to action token */
CEXTERN    int *rvec;			/* robot rule to action token */


    /* Planets. */
/*CEXTERN    int pprimary[NUMPLANETS];	/* planet this planet orbits */
CEXTERN    int *pprimary;	/* planet this planet orbits */

/*CEXTERN    real porbrad[NUMPLANETS];		/* radius of the orbit */
CEXTERN    real *porbrad;		/* radius of the orbit */

/*CEXTERN    real porbvel[NUMPLANETS];		/* velocity of the orbit in deg/min */
CEXTERN    real *porbvel;		/* velocity of the orbit in deg/min */

/*CEXTERN    int ptype[NUMPLANETS];		/* type of planet - M, dead, sun */
CEXTERN    int *ptype;		/* type of planet - M, dead, sun */

/*CEXTERN    int preal[NUMPLANETS];		/* tells if planet can be seen */
CEXTERN    int *preal;		/* tells if planet can be seen */

/*CEXTERN    char pname[NUMPLANETS][MAXPLANETNAME];*/
CEXTERN    char **pname;

/*CEXTERN    char ptname[MAXPLANETTYPES][MAXPTYPENAME];*/
CEXTERN    char **ptname;

    /* Teams. */
/*CEXTERN    int homeplanet[NUMTEAMS];	/* planet number of the home planet */
CEXTERN    int *homeplanet;	/* planet number of the home planet */

/*CEXTERN    int homesun[NUMTEAMS];		/* sun number of the home system */
CEXTERN    int *homesun;		/* sun number of the home system */

/*CEXTERN    int teamplanets[NUMTEAMS][3];	/* planet numbers of the home system */
CEXTERN    int **teamplanets;	/* planet numbers of the home system */

/*CEXTERN    int armylim[NUMTEAMS];		/* maximum armies a team ship can carry */
CEXTERN    int *armylim;		/* maximum armies a team ship can carry */

/*CEXTERN    real warplim[NUMTEAMS];		/* maximum warp based on team */
CEXTERN    real *warplim;		/* maximum warp based on team */

/*CEXTERN    real engfac[NUMTEAMS];		/* team engine constants */
CEXTERN    real *engfac;		/* team engine constants */

/*CEXTERN    real accelfac[NUMTEAMS];		/* team acceleration constants */
CEXTERN    real *accelfac;		/* team acceleration constants */

/*CEXTERN    real weafac[NUMTEAMS];		/* team weapons constants */
CEXTERN    real *weafac;		/* team weapons constants */

/*CEXTERN    real torpwarp[NUMTEAMS];		/* speed of each teams torpedos */
CEXTERN    real *torpwarp;		/* speed of each teams torpedos */

/*CEXTERN    char tname[NUMALLTEAMS][MAXTEAMNAME];*/
CEXTERN    char **tname;

    /* Characters. */
/*CEXTERN    char chrplanets[MAXPLANETTYPES];/* class M = M, dead = D, sun = S */
CEXTERN    char *chrplanets;/* class M = M, dead = D, sun = S */

/*CEXTERN    char chrtorps[NUMTEAMS];	/* char to display for torps */
CEXTERN    char *chrtorps;	/* char to display for torps */

/*CEXTERN    char chrteams[NUMALLTEAMS];	/* F = F  R = R  K = K  O = O */
CEXTERN    char *chrteams;	/* F = F  R = R  K = K  O = O */

/* End of static data. */

/* Start of semi-static data. */

    /* Teams. */
/*CEXTERN    int couptime[NUMTEAMS];		/* minutes left till coup possible */
CEXTERN    int *couptime;		/* minutes left till coup possible */


    /* Planets. */
/*CEXTERN    int puninhabtime[NUMPLANETS];	/* minutes left till inhabitable */
CEXTERN    int *puninhabtime;	/* minutes left till inhabitable */


    /* Doomsday machine. */
/*CEXTERN    int dstatus;			/* doomsday status; one of the DS_ values */
CEXTERN    int *dstatus;			/* doomsday status; one of the DS_ values */

/*CEXTERN    int dtype;			/* type of doomsday machine */
CEXTERN    int *dtype;			/* type of doomsday machine */

/*CEXTERN    real dx;				/* x and y coordinates of doomsday */
CEXTERN    real *dx;				/* x and y coordinates of doomsday */

/*CEXTERN    real dy;*/
CEXTERN    real *dy;

/*CEXTERN    real ddx;				/* x and y increments in mega-meters */
CEXTERN    real *ddx;				/* x and y increments in mega-meters */

/*CEXTERN    real ddy;*/
CEXTERN    real *ddy;

/*CEXTERN    real dhead;				/* heading */
CEXTERN    real *dhead;				/* heading */

/*CEXTERN    int dlock;			/* destination: 0, ship #, or -planet # */
CEXTERN    int *dlock;			/* destination: 0, ship #, or -planet # */

/*CEXTERN    char dname[SIZEUSERPNAME];	/* name of doomsday machine */
CEXTERN    char *dname;	/* name of doomsday machine */

/* End of semi-static data. */

/* Start of semi-volatile data. */

    /* Planets. */
/*CEXTERN    real porbang[NUMPLANETS];		/* current angle of the orbit */
CEXTERN    real *porbang;		/* current angle of the orbit */

/*CEXTERN    real px[NUMPLANETS];			/* x coordinates of planet */
CEXTERN    real *px;			/* x coordinates of planet */

/*CEXTERN    real py[NUMPLANETS];			/* y coordinates of planet */
CEXTERN    real *py;			/* y coordinates of planet */

/*CEXTERN    int pteam[NUMPLANETS];		/* which team owns the planet */
CEXTERN    int *pteam;		/* which team owns the planet */

/*CEXTERN    int parmies[NUMPLANETS];		/* how many armies are on the planet */
CEXTERN    int *parmies;		/* how many armies are on the planet */

/*CEXTERN    int pscanned[NUMPLANETS][NUMTEAMS]; /* which planets have been */
CEXTERN    int **pscanned; /* which planets have been */
					/*  scanned by which teams */

    /* Last usage stuff. */
/*CEXTERN    int histptr;			/* points to most recent usage */
CEXTERN    int *histptr;			/* points to most recent usage */

/*CEXTERN    int histunum[MAXHISTLOG];	/* user number */
CEXTERN    int *histunum;	/* user number */

/*CEXTERN    char histlog[MAXHISTLOG][DATESIZE]; /* date string of entry */
CEXTERN    char **histlog; /* date string of entry */

/* End of semi-volatile data. */

/* Start of volatile data. */

    /* Driver. */
/*CEXTERN    int drivstat;			/* driver status; one of the DRS_ values */
CEXTERN    int *drivstat;			/* driver status; one of the DRS_ values */

/*CEXTERN    int drivpid;			/* pid of the driver process */
CEXTERN    int *drivpid;			/* pid of the driver process */

/*CEXTERN    int drivcnt;			/* number of driver timeouts (modp1 1000) */
CEXTERN    int *drivcnt;			/* number of driver timeouts (modp1 1000) */

/*CEXTERN    int drivsecs;			/* phase of the universe in seconds */
CEXTERN    int *drivsecs;			/* phase of the universe in seconds */

					/*  (modp1 five minutes) */
/*CEXTERN    char drivowner[SIZEUSERNAME];	/* username of the driver's owner */
CEXTERN    char *drivowner; /* username of the driver's owner */

    /* The following are seconds since midnight. */
/*CEXTERN    int drivtime;			/* last time the driver looped */
CEXTERN    int *drivtime;			/* last time the driver looped */

/*CEXTERN    int playtime;			/* last time a player looped */
CEXTERN    int *playtime;			/* last time a player looped */

    /* Ships. */

CEXTERN Ship_t *Ships;		/* ship structures */

    /* Torpedoes. */
/*CEXTERN    int tstatus[MAXSHIPS][MAXTORPS];	/* one of the TS_ values */
CEXTERN    int **tstatus;	/* one of the TS_ values */

/*CEXTERN    int tfuse[MAXSHIPS][MAXTORPS];	/* seconds left */
CEXTERN    int **tfuse;	/* seconds left */

/*CEXTERN    real tx[MAXSHIPS][MAXTORPS];		/* x and y coordinates of torps */
CEXTERN    real **tx;		/* x and y coordinates of torps */

/*CEXTERN    real ty[MAXSHIPS][MAXTORPS];*/
CEXTERN    real **ty;

/*CEXTERN    real tdx[MAXSHIPS][MAXTORPS];		/* x and y increments */
CEXTERN    real **tdx;		/* x and y increments */

/*CEXTERN    real tdy[MAXSHIPS][MAXTORPS];		/*  in mega-meters per iter seconds */
CEXTERN    real **tdy;		/*  in mega-meters per iter seconds */

/*CEXTERN    real tmult[MAXSHIPS][MAXTORPS];	/* multiplier when detonated */
CEXTERN    real **tmult;	/* multiplier when detonated */

/*CEXTERN    int twar[MAXSHIPS][MAXTORPS][NUMTEAMS]; /* copy of war when fired */
CEXTERN    int ***twar; /* copy of war when fired */

    /* Messages. */
/*CEXTERN    char msgbuf[MAXMESSAGES][MESSAGE_SIZE];	/* the messages */
CEXTERN    char **msgbuf;	/* the messages */

/*CEXTERN    int msgfrom[MAXMESSAGES];	/* >0: ship the message is from */
CEXTERN    int *msgfrom;	/* >0: ship the message is from */
					/* =0: from GOD */
					/* <0: planet with intruder alert */

/*CEXTERN    int msgto[MAXMESSAGES];		/* >0: ship the message is to */
CEXTERN    int *msgto;		/* >0: ship the message is to */
					/* =0: to nobody */
					/* <0: team the message is to, and */
					/*     TEAM_SELFRULED means everyone */

/*CEXTERN    int lastmsg;			/* the last message sent */
CEXTERN    int *lastmsg;			/* the last message sent */

/*CEXTERN    int externrobots;		/* used to disable internal robot code */
CEXTERN    int *externrobots;		/* used to disable internal robot code */

    /* Note: glastmsg must be the last variable in the common block. */
/*CEXTERN    int glastmsg;		/* the last message read by God */
CEXTERN    int *glastmsg;		/* the last message read by God */

/* End of volatile data. */

#undef CEXTERN

#endif /* CONQCOM_H */
