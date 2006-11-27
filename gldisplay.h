/************************************************************************
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _GLDISPLAY_H
#define _GLDISPLAY_H

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include "textures.h"

typedef struct _dspConfig {
  Bool inited;
  /* glut win ids */
  int mainw;                    /* main window */

  /* main window */
  GLfloat wX, wY;               /* x/y origin */
  GLfloat wW, wH;               /* width/height  */
  GLfloat mAspect;

  /* viewer window */
  GLfloat vX, vY;               /* viewer X/Y */
  GLfloat vW, vH;               /* viewer width/height */
  GLfloat vAspect;

  GLfloat ppCol, ppRow;         /* pixels per [Row|Col] */

  GLfloat borderW;              /* width of outside mainw border */

  GLfloat hmat[16];             /* hud proj matrix */
  GLfloat vmat[16];             /* viewer proj matrix */

  unsigned int flags; 

  int fullScreen;
  int initWidth, initHeight;    /* initial wxh geometry */
} dspConfig_t;

#ifdef NOEXTERN_DCONF
dspConfig_t dConf;
Unsgn32     frameTime;
#else
extern dspConfig_t dConf;
extern Unsgn32     frameTime;
#endif


/* cockpit display items */
struct _warp {
  char warp[16];
  int color;
  char label[16];
  int lcolor;
};

struct _heading {
  char heading[16];
  int color;
  char label[16];
  int lcolor;
};

struct _kills {
  char kills[16];
  int color;
  char label[16];
  int lcolor;
};

struct _alertStatus {
  char alertStatus[64];
  int color;
};

struct _alertBorder {
  int alertColor;
};

struct _shields {
  int shields;
  int color;
  char label[32];
  int lcolor;
};

struct _damage {
  int damage;
  int color;
  char label[16];
  int lcolor;
};

struct _fuel {
  int fuel;
  int color;
  char label[16];
  int lcolor;
};

struct _alloc {
  char allocstr[16];
  int walloc, ealloc;
  int color;
  char label[32];
  int lcolor;
};

struct _etemp {
  int etemp;
  int color;
  char label[32];
  int lcolor;
  int overl;
};

struct _wtemp {
  int wtemp;
  int color;
  char label[32];
  int lcolor;
  int overl;
};

struct _tow {
  char str[32];
  int color;
};

struct _armies {
  char str[16];
  char label[32];
  int color;
};

struct _cloakdest {             /* cloak OR destruct msg */
  char str[32];
  int color;
  int bgcolor;
};

/* prompt areas */

struct _prompt_lin {
  char str[1024];
};

struct _xtrainfo {
  char str[256];
};

struct _recId {
  char str[256];
};

struct _recTime {
  char str[256];
};




/* This holds all of the info for the cockpit display. */
typedef struct _dspData {
  struct _warp warp;
  struct _heading heading;
  struct _kills kills;
  struct _alertStatus aStat;
  struct _alertBorder aBorder;
  struct _shields sh;
  struct _damage dam;
  struct _fuel fuel;
  struct _alloc alloc;
  struct _etemp etemp;
  struct _wtemp wtemp;
  struct _tow tow;
  struct _armies armies;
  struct _cloakdest cloakdest;
  struct _prompt_lin p1;
  struct _prompt_lin p2;
  struct _prompt_lin msg;
  struct _xtrainfo xtrainfo;
  struct _recId recId;
  struct _recTime recTime;
} dspData_t;


void display( int snum, int display_info );

int uiCStrlen(char *buf);
void uiPrintFixed(GLfloat x, GLfloat y, GLfloat w, GLfloat h, char *str);
int uiGLInit(int *argc, char **argv);
void uiDrawPlanet( GLfloat x, GLfloat y, int pnum, int scale,
                   int textcolor, int scanned );
void setXtraInfo(void);

real cu2GLSize(real size);

int GLcvtcoords(real cenx, real ceny, real x, real y, real scale,
		 GLfloat *rx, GLfloat *ry );

void drawTorp(GLfloat x, GLfloat y, char torpchar, int color, int scale,
              int snum, int torpnum);
void drawShip(GLfloat x, GLfloat y, GLfloat angle, char ch, int i, 
	      int color, GLfloat scale);
void drawDoomsday(GLfloat x, GLfloat y, GLfloat angle, GLfloat scale);
void drawViewerBG(int snum, int dovbg);
void drawNEB(int snum);

void clrPrompt(int line);
void setPrompt(int line, char *prompt, int pcolor,
               char *buf, int color);
void setHeading(char *);
void setWarp(real);
void setKills(char *);
void setFuel(int, int);
void setShields(int, int);
void setAlloc(int w, int e, char *alloc);
void setTemp(int etemp, int ecolor, int wtemp, int wcolor, 
	      int efuse, int wfuse);
void setDamage(int dam, int color);
void setDamageLabel(char *buf, int color);
void setArmies(char *labelbuf, char *buf);
void setCloakDestruct(char *buf, int color);
void setTow(char *buf);
void setAlertBorder(int color);
void setAlertLabel(char *buf, int color);
void setRecTime(char *str);
void setRecId(char *str);


void setPrompt(int line, char *prompt, int pcolor,
               char *buf, int color);
float getFPS(void);

void drawLine(GLfloat x, GLfloat y, GLfloat len, GLfloat lw);
void drawLineBox(GLfloat x, GLfloat y, 
                 GLfloat w, GLfloat h, int color, 
                 GLfloat lw);
void drawQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat z);
void drawTexQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat z);
void drawExplosion(GLfloat x, GLfloat y, int snum, int torpnum, int scale);
void drawBombing(int snum);

void dspInitData(void);
void hex2GLColor(Unsgn32 hcol, GLColor_t *col);

#endif /* _GLDISPLAY_H */
