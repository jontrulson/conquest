/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _CQCOLOR_H
#define _CQCOLOR_H

#undef CEXTERN
#ifdef NOEXPORT_COLORS
# define CEXTERN
#else
# define CEXTERN extern
#endif

typedef Unsgn32 cqColor;

#define CQC_FG_MASK      (0x000000ff)
#define CQC_BG_MASK      (0x0000ff00)
#define CQC_ATTR_MASK    (0x00ff0000)

#define CQC_FG_SHIFT     (0)
#define CQC_BG_SHIFT     (8)
#define CQC_ATTR_SHIFT   (16)
/* colors for conquest */

#define CQC_WHITE        (0x00000000)
#define CQC_BLACK        (0x00000001)
#define CQC_RED          (0x00000002)
#define CQC_GREEN        (0x00000004)
#define CQC_YELLOW       (0x00000008)
#define CQC_MAGENTA      (0x00000010)
#define CQC_CYAN         (0x00000020)
#define CQC_BLUE         (0x00000040)

/* attributes */
#define CQC_A_NONE       (0x00000000)
#define CQC_A_BOLD       (0x00010000)
#define CQC_A_DIM        (0x00020000)
#define CQC_A_BLINK      (0x00040000)


				/* general purpose 'colors' 
                                   initialized in colorInit() */
CEXTERN int RedLevelColor, YellowLevelColor, GreenLevelColor;
CEXTERN int LabelColor, InfoColor, SpecialColor;

				/* individual colors */
CEXTERN int RedColor, GreenColor, YellowColor, MagentaColor,
            CyanColor, NoColor, BlueColor, BlackColor;

				/* function prototypes for color.c */

#undef CEXTERN
#endif /* _CQCOLOR_H */

