/* Color.h -  Color stuff for conquest - Jon Trulson */

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/


/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#ifndef COLOR_H
#define COLOR_H

#undef CEXTERN
#ifdef NOEXPORT_COLORS
# define CEXTERN
#else
# define CEXTERN extern
#endif


			/* COLOR_PAIR/INIT_PAIR Color definitions */

#define COL_BACKGROUND 1
#define COL_REDBLACK  2
#define COL_GREENBLACK 3
#define COL_YELLOWBLACK 4
#define COL_MAGENTABLACK 5
#define COL_CYANBLACK 6
#define COL_BLUEBLACK 7

CEXTERN int HasColors;      /* TRUE or FALSE if term has color cap. */

				/* general purpose 'colors' 
                                   initialized in InitColors() */
CEXTERN int RedLevelColor, YellowLevelColor, GreenLevelColor;
CEXTERN int LabelColor, InfoColor, SpecialColor;

				/* individual colors */
CEXTERN int RedColor, GreenColor, YellowColor, MagentaColor,
            CyanColor, NoColor, BlueColor;

				/* function prototypes for color.c */

void InitColors(void);
void cprintf(int lin, int col, int align, char *fmt, ...);

#undef CEXTERN
#endif
