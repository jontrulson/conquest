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
animStateRec_t GLBlinkerOneSec = {};
animStateRec_t GLBlinkerHalfSec = {};
animStateRec_t GLBlinkerQtrSec = {};
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
  GLint     ship;             /* main ship texture */
  GLint     sh;               /* main ship shield texture */
  GLint     tac;              /* team tactical grid */
  GLint     phas;             /* phaser */
  GLint     ico;              /* ship icon */
  GLint     ico_sh;           /* icon shields id */
  GLint     decal1;           /* sh/hull gauges */
  GLint     decal2;           /* fuel/w/e/alloc/kills gauges */
  GLint     dial;             /* ships astrogator dial */
  GLint     dialp;            /* astrogator pointer */
  GLint     warp;             /* id of warp gauge */
  GLint     warp2;            /* id of warp gauge background*/
  GLColor_t warpq_col;        /* the color of the warp indicator quad */
  GLint     ico_torp;         /* torpedo icon(s) */
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

#else
extern GLShip_t GLShips[NUMPLAYERTEAMS][MAXNUMSHIPTYPES]; 

extern GLfloat scaleFactorsLR[10];
extern GLfloat scaleFactorsSR[10];
#endif

#endif /* _C_GL_H */

