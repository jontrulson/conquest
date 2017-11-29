/*
 * Misc Conquest Info structure
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef CONQINFO_H_INCLUDED
#define CONQINFO_H_INCLUDED

#include "conqdef.h"

typedef struct {
    bool  closed;                  /* the game is closed, everybody out */
    int  lockword;                /* exclusionary flag */
    int  lockmesg;                /* exclusive access to the message area */
    int  ccpuseconds;		/* cpu seconds used by conquest */
    int  celapsedseconds;		/* elapsed seconds for conquest */
    int  dcpuseconds;		/* cpu seconds used by conqdriv */
    int  delapsedseconds;		/* elapsed seconds for conqdriv */
    int  rcpuseconds;		/* cpu seconds used by robots */
    int  relapsedseconds;		/* elapsed seconds for robots */
    int  raccum;			/* accumulated cpu hundredths */
    int  histptr;			/* points to most recent usage */
    int  lastmsg;			/* the last message sent */
    bool  externrobots;		/* used to disable internal robot code */
    int  glastmsg;                /* the last message read by God */
    char lastupchuck[MAXDATESIZE];	/* last time the common block was updated */
    char ptname[MAXPLANETTYPES][MAXPTYPENAME]; /* planet type names */
    char chrplanets[MAXPLANETTYPES]; /* class M = M, dead = D, sun = S */
    char conqueror[MAXUSERNAME]; /* user who last conquered */
    char conqteam[MAXTEAMNAME];	/* team s/he was on */
    char conqtime[MAXDATESIZE];	/* time of last conquering */
    char lastwords[MAXLASTWORDS];	/* remarks upon conquering */
    char inittime[MAXDATESIZE];	/* time the universe was initialized */
    int  conqservPID;		/* the conqserv pid */
} cbConqInfo_t;

#endif /* CONQINFO_H_INCLUDED */
