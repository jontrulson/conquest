/************************************************************************
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _DISPLAY_H
#define _DISPLAY_H

#if 0
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

int InitGL(int argc, char **argv);

void startRendering(void);
void stopRendering(void);

void startDraw(void);
void finishDraw(void);

void procEvent(void);
void procEvents(void);
void showXtraInfo(int);

void glputhing(int, GLfloat, GLfloat, int);
void GLcvtcoords(real cenx, real ceny, real x, real y, real scale,
		 GLfloat *rx, GLfloat *ry );

void drawTorp(GLfloat x, GLfloat y, char torpchar, int color);
void drawShip(GLfloat x, GLfloat y, GLfloat angle, char ch, int i, 
	      int color, GLfloat scale);
void drawDoomsday(GLfloat x, GLfloat y, GLfloat angle, GLfloat scale);
void drawString(GLfloat x, GLfloat y, char *str, int color);
void showMessage(int line, char *buf);
void showHeading(char *);
void showWarp(char *);
void showKills(char *);
void showFuel(int, int);
void showShields(int, unsigned int);
void showAlloc(char *alloc);
void showTemp(int etemp, int ecolor, int wtemp, int wcolor, 
	      int efuse, int wfuse);
void showDamage(int dam, int color);
void showDamageLabel(char *buf, int color);
void showArmies(char *labelbuf, char *buf);
void showCloakDestruct(char *buf);
void showTow(char *buf);
void setAlertBorder(int color);
void showAlertLabel(char *buf, int color);

#endif

#endif /* _DISPLAY_H */
