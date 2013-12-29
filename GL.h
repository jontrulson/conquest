/* GL.h - OpenGL rendering for Conquest.  Exported GL.c interfaces.  In
 *  general, I'd like to keep this stuff localized to GL.c, but for
 *  various reasons (performance mainly), some of it needs to be directly
 *  accessible to other modules.
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _C_GL_H
#define _C_GL_H

#include "anim.h"

/* storage for ship textures, colors, other GL related things, indexed by
   team/shiptype */
typedef struct _glship {
  GLTexture_t *ship;             /* main ship texture */
  GLTexture_t *sh;               /* main ship shield texture */
  GLTexture_t *tac;              /* team tactical grid */
  GLTexture_t *phas;             /* phaser */
  GLTexture_t *ico;              /* ship icon */
  GLTexture_t *ico_sh;           /* icon shields id */
  GLTexture_t *ico_torp;         /* torp pip */
  GLTexture_t *decal1;           /* sh/hull gauges */
  GLTexture_t *decal1_lamp_sh;   /* sh lamp */
  GLTexture_t *decal1_lamp_hull; /* hull lamp */
  GLTexture_t *decal1_lamp_fuel; /* fuel lamp */
  GLTexture_t *decal1_lamp_eng;  /* eng lamp */
  GLTexture_t *decal1_lamp_wep;  /* wep lamp */
  GLTexture_t *decal1_lamp_rep;  /* repairing lamp */
  GLTexture_t *decal1_lamp_cloak;/* cloaking lamp */
  GLTexture_t *decal1_lamp_tow;  /* towing/towedby lamp */
  GLTexture_t *decal2;           /* fuel/w/e/alloc/kills gauges */
  GLTexture_t *dial;             /* ships astrogator dial */
  GLTexture_t *dialp;            /* astrogator pointer */
  GLTexture_t *warp;             /* warp gauge */
  GLTexture_t *warp2;            /* warp gauge background*/
  GLTexture_t *warpq_col;        /* warp indicator quad (color only) */
} GLShip_t;

#if defined(NOEXTERN_GL)
GLShip_t GLShips[NUMPLAYERTEAMS][MAXNUMSHIPTYPES] = {}; 

/* the LR scaling array (-5 - 5) */
GLfloat scaleFactorsLR[10] = {
  /* zoom out (mag-) */
  0.75,
  0.8,
  0.85,
  0.9,
  0.95,
  /* zoom in (mag+) */
  1.25,
  1.3,
  1.35,
  1.4,
  1.45
};

/* the SR scaling array (-5 - 5) */
GLfloat scaleFactorsSR[10] = {
  /* zoom out (mag-) */
  0.5,
  0.6,
  0.7,
  0.8,
  0.9,
  /* zoom in (mag+) */
  1.45, 
  1.90, 
  2.35, 
  2.80, 
  3.25
};

GLTexture_t defaultTexture;

/* count resize/scale change events so functions that care, can detect
 * them and do the right thing.
 */
Unsgn32 GLGeoChange = 0;

#else
extern GLShip_t GLShips[NUMPLAYERTEAMS][MAXNUMSHIPTYPES]; 

extern GLfloat  scaleFactorsLR[10];
extern GLfloat  scaleFactorsSR[10];
extern const GLTexture_t defaultTexture;
extern const Unsgn32 GLGeoChange;
#endif

void drawPlanet( GLfloat x, GLfloat y, int pnum, int scale,
                   int textcolor );
real cu2GLSize(real size, int scale);

int GLcvtcoords(real cenx, real ceny, real x, real y, real scale,
		 GLfloat *rx, GLfloat *ry );

void drawTorp(GLfloat x, GLfloat y, int scale,
              int snum, int torpnum);
void drawShip(GLfloat x, GLfloat y, GLfloat angle, char ch, int i, 
	      int color, GLfloat scale);
void drawDoomsday(GLfloat x, GLfloat y, GLfloat angle, GLfloat scale);
void drawViewerBG(int snum, int dovbg);
void drawNEB(int snum);
float getFPS(void);

void drawLine(GLfloat x, GLfloat y, GLfloat len, GLfloat lw);
void drawLineBox(GLfloat x, GLfloat y, GLfloat z,
                 GLfloat w, GLfloat h, int color, 
                 GLfloat lw);
void drawQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat z);
void drawTexQuad(GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLfloat h,
                 int ortho, int rot90);
void drawTexBoxCentered(GLfloat x, GLfloat y, GLfloat z, GLfloat size,
                        int ortho, int rot90);
void drawExplosion(GLfloat x, GLfloat y, int snum, int torpnum, int scale);
void drawBombing(int snum, int scale);

void dspInitData(void);

void drawIconHUDDecal(GLfloat rx, GLfloat ry, GLfloat w, GLfloat h, 
                  int imgp, cqColor icol);
void hex2GLColor(Unsgn32 hcol, GLColor_t *col);
void setViewerScaling(int scale, int isLR);

#endif /* _C_GL_H */

