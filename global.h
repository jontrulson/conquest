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

#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef NOEXTERNGLOBALS
#define CEXTERN 
#else
#define CEXTERN extern 
#endif

CEXTERN	   int PollInputfd;	/* a copy of stdin's fd for poll()/select() */
CEXTERN    int ConquestGID;	/* Conquest's GID */
CEXTERN    int ConquestUID;     /* Conquest's UID */
CEXTERN    int RMsg_Line;

				/* for the semaphores */

#define LOCKMSG      (0)	/* lock the message portion  */
#define LOCKCMN      (1)	/* lock everything else  */

/* cprintf align values */
#define ALIGN_CENTER 3
#define ALIGN_LEFT   2
#define ALIGN_RIGHT  1
#define ALIGN_NONE   0

CEXTERN int headerflag;         /* whether to show header flag */

#undef CEXTERN			/* cleanup */
#endif
