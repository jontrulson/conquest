/************************************************************************
 * 
 * defs.h - modify Conquest's behavior - hopefully for the better ;-)
 *
 * Jon Trulson
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

#ifndef _DEFS_H
#define _DEFS_H
/* Basic information */

/* ROOT_USER - username that game related files created by conquest
 *  are chown()'d to.  Default: root
 */
#define ROOT_USER "root"

/* CONQUEST_USER - When this username runs conquest, server mode is active.
 *   - the user is prompted for their username, and their password,
 *   - user is automatically added to conquest userdb if a new user
 *   - .conquestrc file for such users are created in ~CONQUEST_USER/ 
 *     directory as .conquestrc.UNUM (UNUM = user number)
 *   Default: conquest
 */
#define CONQUEST_USER "conquest"


/* CONQUEST_GROUP - this is the group that all conquest files are chgrp'd
 *   to... This is also the group that the conquest executables setgid()
 *   to as well.  You should leave it as is... Default: conquest.  
 */
#ifndef CONQUEST_GROUP
# define CONQUEST_GROUP "conquest"
#endif

/* Some variables that used to be in conqfig.c - relative to conquest
 *  installation directories
 */

#define C_CONQ_CONQDRIV  "conqdriv"
#define C_CONQ_COMMONBLK "conquest_common.img"
#define C_CONQ_ERRLOG    "conquest.log"
#define C_CONQ_NEWSFILE  "conqnews.doc"
#define C_CONQ_HELPFILE  "conquest.doc"


/* HAS_SETITIMER - Define if your system supports setitimer/getitimer().
 *  Otherwise alarm() is used.  With setitimer() it's possible to get a
 *  faster refresh rate (say 2 updates per second) rather than 1 update
 *  per second as is the case with alarm().
 */
#if defined(HAVE_SETITIMER)
# define HAS_SETITIMER
#endif

/* USE_COMMONMLOCK - Lock the common block into memory via memctl(). 
 *  Requires the PLOCK privilege (unixware).
 */

#if defined(UNIXWARE)
# define USE_COMMONMLOCK 
#endif

/* WARP0CLOAK - Although the code made it difficult to scan a ship that was
 *  cloaked at warp 0, it was still possible to scan such a ship if it was
 *  within alert range.  Defining this means you CANNOT be detected (even by 
 *  robots) if you are cloaked and at warp 0.0.  An enemy can fly right over
 *  you and won't even know your there... heeheehee
 */
#define WARP0CLOAK 

/* DO_EXPLODING_SHIPS - In this mode, ships explode when they die, with
 *  a force equal to the number of torpedos the ship has available, and
 *  only if the ship had the fuel to 'fire' them, were it alive to do
 *  so. 
 */
#define DO_EXPLODING_SHIPS

/* SET_PRIORITY - increase our priority a bit, increase the driver's a bit 
 * more.  Requires the TSHAR privilege (unixware).
 */

#if defined(UNIXWARE)
# define SET_PRIORITY
#endif
				/* Priorities, if we're using them */
#ifdef SET_PRIORITY
# define CONQUEST_PRI (-4)
# define CONQDRIV_PRI (-7)
#endif

/* ENABLE_MACROS - enable use of the macro F keys 
 */
#define ENABLE_MACROS

#define MAX_MACROS 64		/* max number of macros supported */
#define MAX_MACRO_LEN 70	/* max length of a macro */

/* OPER_MSG_BEEP - if defined, beep when a message arrives in conqoper.
 */
#undef OPER_MSG_BEEP


/* DEBUG* - debug some things */

#undef DEBUG_FLOW		/* trace init flow */
#undef DEBUGGING		/* general debugging */
#undef DEBUG_LOCKING		/* debugging common block locking */
#undef DEBUG_CONFIG             /* debug configuration file processing */
#undef DEBUG_SEM		/* semaphore debugging */
#undef DEBUG_MACROS		/* MACRO debugging */
#undef DEBUG_IO                 /* IO debugging */
#undef DEBUG_SIG		/* debug signal handling */
#undef DEBUG_AI                 /* debug the robots */
#undef DEBUG_MISC		/* debug other misc stuff */
#undef DEBUG_RANDOM		/* debug the random number library */
#undef DEBUG_COLOR		/* debug color processing */
#undef DEBUG_IOGTIMED           /* debug timed input */
#undef DEBUG_SERVERAUTH         /* debug server auth operation */
#undef DEBUG_OPTIONS            /* debug option screens/handling */
#undef DEBUG_REC                /* debug recording */
#define DEBUG_SERVERPROC         /* server proc*() routines */
#undef DEBUG_SERVERCLNTSTAT     /* server sendClientStat() */
#undef DEBUG_SERVERSEND         /* server send*() routines */
#undef DEBUG_CLIENTPROC         /* client proc* routines */
#define DEBUG_CLIENTSEND         /* client send* routines */
#undef DEBUG_META               /* meta server debugging */

#endif /* _DEFS_H */
