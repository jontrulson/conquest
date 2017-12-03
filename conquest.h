
/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#ifndef CONQUEST_H
#define CONQUEST_H

#include "config.h"

#if defined(MINGW)
#define CONQUEST_NAME "Conquest (Windows)"
#elif defined(DARWIN)
#define CONQUEST_NAME "Conquest (MacOS)"
#else
#define CONQUEST_NAME "Conquest (Unix/Linux)"
#endif

#endif
