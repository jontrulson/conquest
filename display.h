/************************************************************************
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _DISPLAY_H
#define _DISPLAY_H

void display( int snum, int display_info );
void display_headers(int snum);
void do_bottomborder(int snum, char *buf, int attrib, int bufattrib);
void do_border(int snum, int attr);
void draw_alertborder(int snum, int alert);

#endif /* _DISPLAY_H */
