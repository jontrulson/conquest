
/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#ifndef CONTEXT_H
#define CONTEXT_H

#include "ship.h"

/* a 'Conquest Context' for the current player/operator */

typedef struct {
    int  unum;			/* user number */
    int  snum;			/* ship number */
    int  histslot;                /* history slot number */
    int  pid;			/* pid of our owner process */
    int  childpid;                /* pid of driver we spawned */
    int  maxlin;			/* number of lines */
    int  maxcol;			/* number of columns */
    int  msgrand;			/* hundreths timestamp since last message */
    int  drchklastime;		/* last time drcheck was called */
    int  hasnewsfile;		/* says if a news file is available */
    bool accessDenied;			/* banned from playing */
    int  display;			/* should do anything in astservice() */
    int  redraw;			/* should redraw in display() */
    int  msgok;			/* allow new messages */
    int  entship;			/* entered a ship? */
    int  recmode;			/* recording mode: one of RECMODE_* */
    int  lasttang;                /* last target angle */
    int  lasttdist;		/* last target distance */
    int  hascolor;                /* do we have color support? */
    int  updsec;			/* updates per second */

    char lasttarg[BUFFER_SIZE_256];   /* string of last ship/planet (I)nfo'd */
    char lastinfostr[MSGMAXLINE]; /* last string used in doinfo() */
    char lastInfoTerm;            /* last terminator (EXTRA/NORMAL) */
    char intrchar;		/* users interrupt char */

    uint8_t ipaddr[SHIP_IPADDR_LEN]; /* IP address */

} Context_t;


#ifdef NOEXTERN_CONTEXT
Context_t Context = {};
#else
extern Context_t Context;
#endif

#endif /* CONTEXT_H */
