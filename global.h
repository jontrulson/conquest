/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

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

/* Home dir/appdir env variable(s) */
#if defined(MINGW)
# define CQ_USERHOMEDIR  "APPDATA"
# define CQ_USERCONFDIR  "Conquest"
#else
# define CQ_USERHOMEDIR  "HOME"
# define CQ_USERCONFDIR  ".conquest"
#endif


#endif /* GLOBAL_H */
