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


#ifndef _COLOR_H
#define _COLOR_H



#define CQC_FG_MASK      (0x000000ff)
#define CQC_BG_MASK      (0x0000ff00)
#define CQC_ATTR_MASK    (0x00ff0000)
#define CQC_ALPHA_MASK   (0xff000000)

#define CQC_FG_SHIFT     (0)
#define CQC_BG_SHIFT     (8)
#define CQC_ATTR_SHIFT   (16)
#define CQC_ALPHA_SHIFT  (24)

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

   0xAAaabbff

   where

   AA = Alpha component
   aa = attrib
   bb = background color
   ff = forground color
*/

typedef uint32_t cqColor;

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
