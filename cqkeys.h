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

#ifndef _CQKEYS_H
#define _CQKEYS_H

#include <GL/glut.h>

/* we just want to encode both 'normal' keys as well as 'special' keys
   (Arrows, Fkeys, etc) into a single int.  Glut special keys
   are shifted 8 bits left during input handling, and we will represent
   that here by making our own key defs for special keys. We will also
   encode the modifier state shifted 16 */

#define CQ_MODIFIER_MASK        (0x00ff0000)
#define CQ_MODIFIER_SHIFT       (16)
#define CQ_FKEY_MASK            (0x0000ff00)
#define CQ_FKEY_SHIFT           (8)
#define CQ_CHAR_MASK            (0x000000ff)

#define CQ_MODIFIER(x)          (x & CQ_MODIFIER_MASK)
#define CQ_FKEY(x)              (x & CQ_FKEY_MASK)
#define CQ_CHAR(x)              (x & CQ_CHAR_MASK)

#define CQ_KEY_MOD_SHIFT        (0x00010000)
#define CQ_KEY_MOD_CTRL         (0x00020000)
#define CQ_KEY_MOD_ALT          (0x00040000)

#define CQ_KEY_F1               ((GLUT_KEY_F1 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F2               ((GLUT_KEY_F2 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F3               ((GLUT_KEY_F3 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F4               ((GLUT_KEY_F4 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F5               ((GLUT_KEY_F5 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F6               ((GLUT_KEY_F6 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F7               ((GLUT_KEY_F7 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F8               ((GLUT_KEY_F8 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F9               ((GLUT_KEY_F9 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F10              ((GLUT_KEY_F10 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F11              ((GLUT_KEY_F11 & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_F12              ((GLUT_KEY_F12 & 0x000000ff) << CQ_FKEY_SHIFT)

/* directional keys */
#define CQ_KEY_LEFT             ((GLUT_KEY_LEFT & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_UP               ((GLUT_KEY_UP & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_RIGHT            ((GLUT_KEY_RIGHT & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_DOWN             ((GLUT_KEY_DOWN & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_PAGE_UP          ((GLUT_KEY_PAGE_UP & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_PAGE_DOWN        ((GLUT_KEY_PAGE_DOWN & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_HOME             ((GLUT_KEY_HOME & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_END              ((GLUT_KEY_END & 0x000000ff) << CQ_FKEY_SHIFT)
#define CQ_KEY_INSERT           ((GLUT_KEY_INSERT & 0x000000ff) << CQ_FKEY_SHIFT)

#endif /* _CQKEYS_H */
