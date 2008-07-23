#include "c_defs.h"
/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/
#include "conqdef.h"
#include "global.h"
#include "context.h"
#include "color.h"
#include "conqcom.h"
#include "ibuf.h"
#include "gldisplay.h"
#include "conf.h"

static char buf[MID_BUFFER_SIZE];
/* 
 * int lin - line if pertinent
 * int col - column if pertinent
 * int align - ALIGN_CENTER 3
 *             ALIGN_RIGHT  2
 *             ALIGN_LEFT   1
 *             ALIGN_NONE   0
 * char *fmt - modified fmt string with color codes embedded
 *             '#' color code
 *             '%' regular format specs
 * ...       - args 
 */
void cprintf(int lin, int col, int align, char *fmt, ...)
{
  va_list ap;
  int l;
  GLfloat x, y, w;
  
  va_start(ap, fmt);
  (void)vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
  va_end(ap);
  
  if (DSP_INITED())
    {
      l = uiCStrlen(buf);
      if (align == ALIGN_CENTER)
        x = dConf.ppCol * ((80.0 / 2.0) - ((GLfloat)l / 2.0));
      else
        x = dConf.ppCol * (GLfloat)col;
      
      y = dConf.ppRow * (GLfloat)lin;
      w = dConf.ppCol * (GLfloat)l;
      uiPrintFixed(x + dConf.wBorderW, y + dConf.wBorderW, w, dConf.ppRow, buf);
    }

  return;
}
