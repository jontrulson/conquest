/* 
 * Misc Conquest Info structure
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef CONQINFO_H_INCLUDED
#define CONQINFO_H_INCLUDED

#include "conqdef.h"

typedef struct {
  int  closed;                  /* the game is closed, everybody out */
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
  int  externrobots;		/* used to disable internal robot code */
  int  glastmsg;                /* the last message read by God */
  char lastupchuck[DATESIZE];	/* last time the common block was updated */
  char ptname[MAXPLANETTYPES][MAXPTYPENAME]; /* planet type names */
  char chrplanets[MAXPLANETTYPES]; /* class M = M, dead = D, sun = S */
  char conqueror[MAXUSERPNAME]; /* user who last conquered */
  char conqteam[MAXTEAMNAME];	/* team s/he was on */
  char conqtime[DATESIZE];	/* time of last conquering */
  char lastwords[MAXLASTWORDS];	/* remarks upon conquering */
  char inittime[DATESIZE];	/* time the universe was initialized */
  int  conqservPID;		/* the conqserv pid */
} ConqInfo_t;

#endif /* CONQINFO_H_INCLUDED */
