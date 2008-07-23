/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include "defs.h"
#include "cprintf.h"

#ifndef GLOBAL_H
#define GLOBAL_H

/* for the semaphores */
#define LOCKMSG      (0)	/* lock the message portion  */
#define LOCKCMN      (1)	/* lock everything else  */


/* verbosity/debugging */
#if defined(NOEXTERN_GLOBALS)
int        PollInputfd = -1; /* a copy of stdin's fd for poll()/select() */
int        ConquestGID;         /* Conquest's GID */
int        RMsg_Line;           /* this is stupid and should go away */


int        cqDebug = 0;
int        headerflag;         /* whether to show header flag */
#else
extern int PollInputfd;	/* a copy of stdin's fd for poll()/select() */
extern int ConquestGID;	/* Conquest's GID */
extern int RMsg_Line;

extern int cqDebug;
extern int headerflag;         /* whether to show header flag */
#endif

#endif /* GLOBAL_H */
