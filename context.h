
/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                             C O N Q C O M 2 */
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

#ifndef CONTEXT_H
#define CONTEXT_H

#ifdef NOEXTERN
#define C2EXTERN
#else
#define C2EXTERN extern
#endif

				/* a 'Conquest Context' for the current
				   player/operator */

typedef struct {
  int unum;			/* user number */
  int snum;			/* ship number */
  int histslot;			/* history slot number */
  int pid;			/* pid of our owner process */
  int childpid;			/* pid of driver we spawned */
  int maxlin;			/* number of lines */
  int maxcol;			/* number of columns */
  int msgrand;			/* hundreths timestamp since last message */
  int drchklastime;		/* last time drcheck was called */
  int hasnewsfile;		/* says if a news file is available */
  int leave;			/* just a flag */
  int display;			/* should do anything in astservice() */
  int redraw;			/* should redraw in display() */
  int msgok;			/* allow new messages */
  int entship;			/* entered a ship? */
  int recmode;			/* recording mode: one of RECMODE_* */
  int lasttang;			/* last target angle */
  int lasttdist;		/* last target distance */
  int hascolor;			/* do we have color support? */
  int updsec;			/* updates per second */
  char lasttarg[BUFFER_SIZE];   /* string of last ship/planet (I)nfo'd */
  char lastinfostr[MSGMAXLINE]; /* last string used in doinfo() */
  char intrchar;		/* users interrupt char */
} Context_t;

C2EXTERN Context_t Context;

#undef C2EXTERN
#endif
