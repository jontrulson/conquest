/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _COLOR_H
#define _COLOR_H

#include "datatypes.h"

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
#define CQC_A_REVERSE    (0x00080000)

/* colors are formatted like so:

   0x00aabbff 

   where
   
   aa = attrib
   bb = background color
   ff = forground color
*/

typedef Unsgn32 cqColor;

# ifdef NOEXPORT_COLORS
/* make them here */

/* individual colors */
cqColor RedColor = CQC_RED;
cqColor GreenColor = CQC_GREEN;
cqColor YellowColor = CQC_YELLOW;
cqColor MagentaColor = CQC_MAGENTA;
cqColor CyanColor = CQC_CYAN;
cqColor NoColor = CQC_WHITE;
cqColor BlueColor = CQC_BLUE;
cqColor BlackColor = CQC_BLACK;

/* general purpose 'colors' */
cqColor RedLevelColor = (CQC_RED | CQC_A_BOLD);
cqColor YellowLevelColor = (CQC_YELLOW | CQC_A_BOLD);
cqColor GreenLevelColor = (CQC_GREEN | CQC_A_BOLD);
cqColor LabelColor = (CQC_BLUE | CQC_A_BOLD);
cqColor InfoColor = (CQC_CYAN | CQC_A_BOLD);
cqColor SpecialColor = (CQC_MAGENTA | CQC_A_BOLD);
# else
extern cqColor RedColor, GreenColor, YellowColor, MagentaColor;
extern cqColor CyanColor, NoColor, BlueColor, BlackColor;
extern cqColor RedLevelColor, YellowLevelColor, GreenLevelColor;
extern cqColor LabelColor, InfoColor, SpecialColor;
# endif /* NOEXPORT_COLORS */


#endif /* _COLOR_H */

