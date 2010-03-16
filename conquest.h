
/************************************************************************
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef CONQUEST_H
#define CONQUEST_H

#include "config.h"

#if defined(MINGW)
#warning "HI!"
#define CONQUESTGL_NAME "ConquestGL (Windows)"
#else
#define CONQUESTGL_NAME "ConquestGL (Unix)"
#endif

/* soon to be retired curses client */
#define CONQUEST_NAME "Conquest"

#endif
