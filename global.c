/* Global.c */

/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 06:57:12  jon
 * Initial revision
 *
 * Revision 1.4  1996/07/02  20:26:57  jon
 * - code cleanup
 *
 * Revision 1.3  1996/04/01  00:29:07  jon
 * - comment changes.
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

#include "c_defs.h"

#define NOEXTERNGLOBALS
#include "global.h"

/* That's it! */
