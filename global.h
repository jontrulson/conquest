/* Global variables  - Jon Trulson */

/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/12/25 00:13:26  jon
 * Initial revision
 *
 * Revision 1.10  1996/12/25  00:13:01  jon
 * - added sysconf_allow_sigquit.
 *
 * Revision 1.9  1996/11/23  06:57:15  jon
 * - added RMsg_Line to support line 25 msg line
 * - added sysconf_DoRandomRobotKills
 *
 * Revision 1.8  1996/07/03  02:15:32  jon
 * - added new config option, rcv_player_msgs option.  See ~/.conquestrc
 *   more more info.
 *
 * Revision 1.7  1996/07/02  20:26:57  jon
 * - code cleanup
 *
 * Revision 1.6  1996/05/25  00:59:36  jon
 * - added sysconf_DoLRTorpScan, sysconf_DoLocalLRScan,
 *   sysconf_DoETAStats, and sysconf_NoDoomsday - new
 *   system-wide config options
 *
 * Revision 1.5  1996/05/02  01:21:06  jon
 * - added LOCKMSG and LOCKCMN, used to indicate which semaphore (there
 *   are 2) an operation is for.  (ie: which part of the common block
 *   to lock.)
 *
 * Revision 1.4  1996/04/28  06:18:13  jon
 * - added conf_MessageBell, conf_NoColor, conf_NoRobotMsgs, and
 *   conf_MacrosF[][] globals to support personal customization via
 *   ~/.conquestrc.
 *
 * Revision 1.3  1996/04/01  00:28:09  jon
 * - added ConquestGID, ConquestUID, iBufOffset, and iBuffer globals. Also
 *   IBUFMAX for the size of iBuffer.
 *
 * Revision 1.2  1995/01/09  00:46:44  jon
 * - Added PVLOCK/UNLOCK capability, fixed some minor bugs, changed cloak
 *   code so that a cloaked, warp 0 ship is invisible to alert scanners.
 * - Increase torp fuse to 50 seconds, thereby increasing range.  
 * - added msync() to implement the upchuck() function.
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * - Initial revision
 *
 *
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#include "defs.h"

#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef NOEXTERNGLOBALS
#define CEXTERN 
#else
#define CEXTERN extern 
#endif

CEXTERN    int HAS_COLORS;	/* TRUE or FALSE if term has color cap. */
CEXTERN	   int PollInputfd;	/* a copy of stdin's fd for poll()/select() */
CEXTERN    int ConquestGID;	/* Conquest's GID */
CEXTERN    int ConquestUID;     /* Conquest's UID */
CEXTERN    int RMsg_Line;

/* Configuration values from init file(s) */

				/* System-wide options */
				/* can see friendly torps on LR scan? */
CEXTERN int sysconf_DoLRTorpScan;
				/* (M)ap is actually LongeRange scan? */
CEXTERN int sysconf_DoLocalLRScan;
				/* (I)nfo cmds return ETA data? */
CEXTERN int sysconf_DoETAStats;
				/* randomly gen a doomsday machine? */
CEXTERN int sysconf_NoDoomsday;
				/* allow users to use do_fastupdate */
CEXTERN int sysconf_AllowFastUpdate;
				/* allow randomized robot strengths */
CEXTERN int sysconf_DoRandomRobotKills;
				/* allow non gods to SIGQUIT out */
CEXTERN int sysconf_AllowSigquit;

				/* User options */
				/* Whether to beep on an incoming message */
CEXTERN int conf_MessageBell;
				/* Whether or not to disable color */
CEXTERN int conf_NoColor;
				/* whether or not you want to see robot msgs */
CEXTERN int conf_NoRobotMsgs;
				/* whether or not to send announcements on */
				/* player entries/deaths to you. */
CEXTERN int conf_RecPlayerMsgs;
				/* whether to update the screen twice per */
				/* second instead of once per second. */
CEXTERN int conf_DoFastUpdate;
				/* whether to restrict bells to no more than */
				/* one per second.  Handy when do_fastupdate */
				/* is set. */
CEXTERN int conf_DoLimitBell;


				/* Macro keys - F1-F12 */
CEXTERN char conf_MacrosF[MAX_MACROS][MAX_MACRO_LEN];

				/* for the semaphores */

#define LOCKMSG      (0)	/* lock the message portion  */
#define LOCKCMN      (1)	/* lock the everything else  */


#undef CEXTERN			/* cleanup */
#endif
