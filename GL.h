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
  GLint     ico_cloak;        /* cloak indicator */
  GLint     ico_shcrit;       /* shields critical */
  GLint     ico_engcrit;      /* engines critical */
  GLint     ico_wepcrit;      /* weapons critical */
  GLint     ico_hulcrit;      /* hull critical */
  GLint     ico_repair;       /* repairing */
  GLint     decal1;           /* sh/hull gauges */
  GLint     decal2;           /* fuel/w/e/alloc/kills gauges */
  GLint     dial;             /* ships astrogator dial */
  GLint     dialp;            /* astrogator pointer */
  GLint     warp;             /* id of warp gauge */
  GLColor_t warpq_col;        /* the color of the warp indicator quad */
} GLShip_t;

#if defined(NOEXTERN_GL)
GLShip_t GLShips[NUMPLAYERTEAMS][MAXNUMSHIPTYPES] = {}; 
#else
extern GLShip_t GLShips[NUMPLAYERTEAMS][MAXNUMSHIPTYPES]; 
#endif

#endif /* _C_GL_H */

