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
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"		/* instantiate externs here */
#include "ui.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

void uiPutColor(cqColor color)
{
  GLfloat base = 0.7;
  
  /* convert a 'conquest' color into something more useful in GL :) */

  if (color & CQC_A_BOLD)
    base += 0.2;

  if (color & CQC_A_DIM)
    base -= 0.2;

  color &= CQC_FG_MASK;         /* strip everything but color info */

  if (color == NoColor)         /* white */
    glColor3f(base, base, base);
  else if (color == RedColor)   /* red */
    glColor3f(base, 0.0, 0.0);
  else if (color == GreenColor) /* green */
    glColor3f(0.0, base, 0.0);
  else if (color == BlueColor)  /* blue */
    glColor3f(0.0, 0.0, base);
  else if (color == YellowColor) /* yellow */
    glColor3f(base, base, 0.0);
  else if (color == CyanColor)  /* cyan */
    glColor3f(0.0, base, base);
  else if (color == MagentaColor) /* magenta */
    glColor3f(base, 0.0, base);
  else if (color == BlackColor) /* black */
    glColor3f(0.0, 0.0, 0.0);
  else                          /* unknown */
    glColor3f(1.0, 1.0, 1.0);
  
  return;
}

/* initialize color variables.  assumes curses has been initialized. */
void uiInitColors(void)
{
  /* nothing to do here - we translate in uiPutColor */
  return;

}
