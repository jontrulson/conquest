/* Color.h -  Color stuff for conquest - Jon Trulson */

/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 06:20:07  jon
 * Initial revision
 *
 * Revision 1.4  1996/11/23  06:19:40  jon
 * - deleted dup pair.
 * - mods to handle ncurses quirks.
 *
 * Revision 1.3  1996/07/02  03:05:05  jon
 * *** empty log message ***
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


/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#ifndef COLOR_H
#define COLOR_H
	
			/* Color definitions */

#define COL_BACKGROUND 1
#define COL_REDBLACK  2
#define COL_GREENBLACK 3
#define COL_YELLOWBLACK 4
#define COL_MAGENTABLACK 5
#define COL_CYANBLACK 6
#define COL_BLUEBLACK 7


#endif
