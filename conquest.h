
/************************************************************************
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef CONQUEST_H
#define CONQUEST_H

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#ifdef NOEXTERN
#define C2EXTERN
#else
#define C2EXTERN extern
#endif

#define CONQUESTGL_NAME "ConquestGL"

typedef struct _ConquestDataRec *ConquestDataPtr;

typedef struct _ConquestDataRec {
  Display *dpy;
  XtAppContext app;
  XtIntervalId timerId;
  XtWorkProcId workId;

  Bool rendering;		/* are we rendering? */

  Widget toplevel;
  
  /* some colors */
  Pixel whitePixel;
  Pixel blackPixel;
  Pixel redPixel;
  Pixel yellowPixel;
  Pixel greenPixel;
  Pixel bluePixel;
  Pixel cyanPixel;
  Pixel magentaPixel;

  Pixel defaultPixel;
  Pixel infoPixel;
  Pixel specialPixel;
  Pixel statsepPixel;
  
} ConquestDataRec;


C2EXTERN ConquestDataRec ConqData;

#undef C2EXTERN
#endif
