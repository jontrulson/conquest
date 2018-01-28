//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


#ifndef _DEFS_H
#define _DEFS_H
/* Basic information */

/*
 * CONQUEST_GROUP - this is the group that all conquest files are chgrp'd
 *   to... This is also the group that the conquest executables setgid()
 *   to as well.  You should leave it as is... Default: conquest.
 */
#ifndef CONQUEST_GROUP
# define CONQUEST_GROUP "conquest"
#endif

/*
 * Some variables that used to be in conqfig.c - relative to conquest
 *  installation directories
 */

#define C_CONQ_CONQDRIV  "conqdriv"
#define C_CONQ_COMMONBLK "conquest.cb"
#define C_CONQ_ERRLOG    "conquest.log"
#define C_CONQ_NEWSFILE  "HISTORY"
#define C_CONQ_HELPFILE  "conquest.doc"

#define C_CONQ_INITRC    "conqinitrc"

/*
 * Macro defines
 */
#define MAX_MACROS 64		/* max number of macros supported */
#define MAX_MACRO_LEN 70	/* max length of a macro */

/*
 * OPER_MSG_BEEP - if defined, beep when a message arrives in conqoper.
 */
#undef OPER_MSG_BEEP


/*
 * AUTO_RECORD - have conquestd automatically record all games.
 */

#undef AUTO_RECORD

/* DEBUG* - debug some things */

#undef DEBUG_FLOW		/* trace init flow */
#undef DEBUGGING		/* general debugging */
#undef DEBUG_LOCKING		/* debugging common block locking */
#undef DEBUG_CONFIG             /* debug configuration file processing */
#undef DEBUG_SEM		/* semaphore debugging */
#undef DEBUG_MACROS		/* MACRO debugging */
#undef DEBUG_IO                 /* IO debugging */
#undef DEBUG_ETA                /* Estimated Time of Arrival debugging */
#undef DEBUG_SIG		/* debug signal handling */
#undef DEBUG_AI                 /* debug the robots */
#undef DEBUG_MISC		/* debug other misc stuff */
#undef DEBUG_RANDOM		/* debug the random number library */
#undef DEBUG_COLOR		/* debug color processing */
#undef DEBUG_IOGTIMED           /* debug timed input */
#undef DEBUG_SERVERAUTH         /* debug server auth operation */
#undef DEBUG_OPTIONS            /* debug option screens/handling */
#undef DEBUG_REC                /* debug recording */
#undef DEBUG_SERVERPROC         /* server proc*() routines */
#undef DEBUG_SERVERCLNTSTAT     /* server sendClientStat() */
#undef DEBUG_SERVERSEND         /* server send*() routines */
#undef DEBUG_CLIENTPROC         /* client proc* routines */
#undef DEBUG_CLIENTSEND         /* client send* routines */
#undef DEBUG_META               /* meta server debugging */
#undef DEBUG_PKT                /* debug packet handling */
#undef DEBUG_GL                 /* debug misc GL stuff */
#undef DEBUG_SOUND              /* debug sound */

#endif /* _DEFS_H */
