/************************************************************************
 *
 * $Id$
 *
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
				/* allow users to (s)witchteams from main
				   menu */
CEXTERN int sysconf_AllowSwitchteams;
				/* number of days of inactivity before
				   expiring a user */
CEXTERN int sysconf_UserExpiredays;

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
				/* whether or not to clear out old msgs after
				   a reincarnate */
CEXTERN int conf_ClearOldMsgs;

				/* Macro keys - F1-F12 */
CEXTERN char conf_MacrosF[MAX_MACROS][MAX_MACRO_LEN];

				/* for the semaphores */

#define LOCKMSG      (0)	/* lock the message portion  */
#define LOCKCMN      (1)	/* lock everything else  */

				/* attribute initialization.
				   initialized in cdinit() */
CEXTERN int RedLevelColor, YellowLevelColor, GreenLevelColor;
CEXTERN int LabelColor, InfoColor, SpecialColor;

/* cprintf align values */
#define ALIGN_CENTER 3
#define ALIGN_LEFT   2
#define ALIGN_RIGHT  1
#define ALIGN_NONE   0

CEXTERN int headerflag;         /* whether to show header flag */

#undef CEXTERN			/* cleanup */
#endif
