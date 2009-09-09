/* GL.h - OpenGL rendering for Conquest.  Exported GL.c interfaces.  In
 *  general, I'd like to keep this stuff localized to GL.c, but for
 *  various reasons (performance mainly), some of it needs to be directly
 *  accessable to other modules.
 *
 * Jon Trulson, 1/2006
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _C_GL_H
#define _C_GL_H

#include "anim.h"

/* Blinker animdefs useful to other modules */

#if defined(NOEXTERN_GL)
animStateRec_t        GLBlinkerOneSec = {};
animStateRec_t        GLBlinkerHalfSec = {};
animStateRec_t        GLBlinkerQtrSec = {};
#else
extern animStateRec_t GLBlinkerOneSec;
extern animStateRec_t GLBlinkerHalfSec;
extern animStateRec_t GLBlinkerQtrSec;
#endif

/* easier macros */
#define GL_BLINK_ONESEC      (GLBlinkerOneSec.state.armed)
#define GL_BLINK_HALFSEC     (GLBlinkerHalfSec.state.armed)
#define GL_BLINK_QTRSEC      (GLBlinkerQtrSec.state.armed)

/* storage for ship textures, colors, other GL related things, indexed by
   team/shiptype */
typedef struct _glship {
  GLTexture_t *ship;             /* main ship texture */
  GLTexture_t *sh;               /* main ship shield texture */
  GLTexture_t *tac;              /* team tactical grid */
  GLTexture_t *phas;             /* phaser */
  GLTexture_t *ico;              /* ship icon */
  GLTexture_t *ico_sh;           /* icon shields id */
  GLTexture_t *decal1;           /* sh/hull gauges */
  GLTexture_t *decal1_lamp_sh;   /* sh lamp */
  GLTexture_t *decal1_lamp_hull; /* hull lamp */
  GLTexture_t *decal1_lamp_fuel; /* fuel lamp */
  GLTexture_t *decal1_lamp_eng;  /* eng lamp */
  GLTexture_t *decal1_lamp_wep;  /* wep lamp */
  GLTexture_t *decal2;           /* fuel/w/e/alloc/kills gauges */
  GLTexture_t *dial;             /* ships astrogator dial */
  GLTexture_t *dialp;            /* astrogator pointer */
  GLTexture_t *warp;             /* warp gauge */
  GLTexture_t *warp2;            /* warp gauge background*/
  GLTexture_t *warpq_col;        /* warp indicator quad (color only) */
  GLTexture_t *ico_torp;         /* torpedo icon(s) */
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

#else
extern GLShip_t GLShips[NUMPLAYERTEAMS][MAXNUMSHIPTYPES]; 

extern GLfloat  scaleFactorsLR[10];
extern GLfloat  scaleFactorsSR[10];
extern const GLTexture_t defaultTexture;
#endif

#endif /* _C_GL_H */

