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


#if defined(NOEXTERN_GLOBALS)
int        ConquestGID;         /* Conquest's GID */

int        cqDebug = 0;
#else
extern int ConquestGID;         /* Conquest's GID */

/* verbosity/debugging */
extern int cqDebug;
#endif

#endif /* GLOBAL_H */
