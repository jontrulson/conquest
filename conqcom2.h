
/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 06:28:00  jon
 * Initial revision
 *
 * Revision 1.3  1996/07/02  19:44:47  jon
 * - code cleanup
 *
 * Revision 1.2  1995/01/09  00:46:44  jon
 * Added PVLOCK/UNLOCK capability, fixed some minor bugs, changed cloak
 * code so that a cloaked, warp 0 ship is invisible to alert scanners.
 * Increase torp fuse to 50 seconds, thereby increasing range.  Added
 * msync() to implement the upchuck() function.
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 *
 *
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
/* Have Phun!                                                         */
/**********************************************************************/

#ifndef CONQCOM2_H
#define CONQCOM2_H

#ifdef NOEXTERN
#define C2EXTERN
#else
#define C2EXTERN extern
#endif

C2EXTERN    int cunum;			/* user number */
C2EXTERN    int csnum;			/* ship number */
C2EXTERN    int cpid;			/* pid of our owner process */
C2EXTERN    int childpid;			/* pid of driver we spawned */
C2EXTERN    int cmaxlin;			/* number of lines */
C2EXTERN    int cmaxcol;			/* number of columns */
C2EXTERN    int ctimflag;			/* event flag for sys$setimr */
C2EXTERN    int ctimid;			/* request id for sys$setimr */
C2EXTERN    int cmsgrand;			/* hundreths timestamp since last message */
C2EXTERN    int cpriority;			/* normal base priority */
C2EXTERN    int clastime;			/* last time drcheck was called */
C2EXTERN    int csubdcl;			/* says if we can spawn to DCL */
C2EXTERN    int cnewsfile;			/* says if a news file is available */
C2EXTERN    int cleave;			/* just a flag */
C2EXTERN    int cdisplay;			/* should do anything in astservice() */
C2EXTERN    int credraw;			/* should redraw in display() */
C2EXTERN    int cmsgok;			/* allow new messages */
C2EXTERN    char cbuf[MSGMAXLINE];		/* general purpose buffer */
C2EXTERN    char clastinfostr[MSGMAXLINE];	/* last string used in doinfo() */

#endif
