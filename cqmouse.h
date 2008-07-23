/* 
 * Conquest mouse data
 *
 * $Id$
 *
 * Copyright 1999-2006 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _CQMOUSE_H
#define _CQMOUSE_H

/* generic 'mouse' data struct */

#define CQ_MOUSE_BDOWN      0
#define CQ_MOUSE_BUP        1

typedef struct m_mouse_data {
  int     button;               /* button number, starting at 0 */
  int     state;                /* CQ_MOUSE_B* */
  Unsgn32 mod;                  /* key modifiers (CQ_MOD*) */
  int     x, y;                 /* ortho coords of mouse event
                                   (topleft = 0,0) */
} mouseData_t;
#endif /* _CQMOUSE_H */
