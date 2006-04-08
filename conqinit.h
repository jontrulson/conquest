/* 
 * conqinit.h 
 * 
 * $Id$
 *
 * Copyright 1999-2006 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _CONQINIT_H
#define _CONQINIT_H 

#include "datatypes.h"
/* structures for the init block */

/* global */
typedef struct _cqi_global_init {
  int maxplanets;
  int maxships;
  int maxusers;
  int maxhist;
  int maxmsgs;

  int maxtorps;                 /* computed at validation time */
  int maxshiptypes;
} cqiGlobalInitRec_t, *cqiGlobalInitPtr_t;

/* shiptypes - not used yet */
typedef struct _cqi_shiptype_init {
  char name[MAXSTNAME];
  real engfac;
  real weafac;
  real accelfac;
  int  torpwarp;
  int  warpmax;
  int  armymax;
  int  shmax;
  int  dammax;
  int  torpmax;
  int  fuelmax;
} cqiShiptypeInitRec_t, *cqiShiptypeInitPtr_t ;

/* planets */
typedef struct _cqi_planet_init {
  char name[MAXPLANETNAME];
  char primname[MAXPLANETNAME]; /* primary's name */
  int  primary;
  real angle;
  real velocity;
  real radius;
  int  ptype;
  int  pteam;
  int  armies;
  int  visible;
  int  core;
  int  homeplanet;              /* homeplanet for this team? */
  real xcoord;
  real ycoord;
  real size;                    /* in CU's (Conquest Units (mega meters))  */
  char texname[TEXFILEMAX];     /* texid */
  Unsgn32 color;
} cqiPlanetInitRec_t, *cqiPlanetInitPtr_t;

#define CQITEX_F_COLOR_SPEC       0x00000001 /* This texture definition
                                                only really specifies a
                                                color and not a texture */


/* textures */
typedef struct _cqi_texture_init {
  char name[TEXFILEMAX];        /* texid */
  char filename[TEXFILEMAX];    /* if different from textid */
  Unsgn32 flags;                /* flags for this cqi texture (CQITEX_F_*)*/
  Unsgn32 color;                /* hex encoded color (AARRGGBB) */
} cqiTextureInitRec_t, *cqiTextureInitPtr_t ;


/* animations */

/* animation types */
#define CQI_ANIMS_TEX      0x00000001
#define CQI_ANIMS_COL      0x00000002
#define CQI_ANIMS_GEO      0x00000004
#define CQI_ANIMS_TOG      0x00000008

#define CQI_ANIMS_MASK     (CQI_ANIMS_TEX | \
                            CQI_ANIMS_COL | \
                            CQI_ANIMS_GEO | \
                            CQI_ANIMS_TOG)

/* animation declarations - these associate animation names to
   an animation definition (animdef) */
typedef struct _cqi_animation_init {
  char name[TEXFILEMAX];        /* animation name */
  char animdef[TEXFILEMAX];     /* anim defintition to use for this
                                   animation */
  int  adIndex;                 /* set at Validate time -
                                   specifies the index into animdef
                                   for this animation.  There is a 1-1
                                   correspondance between cqiAnimDefs
                                   and GLAnimDefs. so this can be used
                                   to index both */
} cqiAnimationInitRec_t, *cqiAnimationInitPtr_t ; 

/* we keep track of what istate was actually specified in the 
   animdef so that animInitState() can do the right thing when
   setting up the animdef's initial state */
#define AD_ISTATE_TEX           0x00000001 /* texname specified */
#define AD_ISTATE_COL           0x00000002 /* color was specified */
#define AD_ISTATE_SZ            0x00000004 /* size was specified */
#define AD_ISTATE_ANG           0x00000008 /* angle was specified */

/* animation definitions */
typedef struct _cqi_animdef_init {
  char name[TEXFILEMAX];        /* animation definition name */
  char texname[TEXFILEMAX];     /* base texture if specified. */

  Unsgn32 timelimit;            /* expire anim after this long (ms) 0=inf */
  Unsgn32 anims;                /* bitmask of animation types that
                                   were specified (CQI_ANIMS_*) */

  /* initial state values (istate) specifically defined in the
     animdef definition */
  Unsgn32 istates;              /* AD_ISTATE_* */
  char    itexname[TEXFILEMAX];
  Unsgn32 icolor;
  real    iangle;
  real    isize;

  /* texture animations */
  struct {
    Unsgn32 color;              /* will override per-tex colors */
    Unsgn32 stages;             /* number of stages (textures) */
    Unsgn32 loops;              /* number of loops, 0 = inf */
    Unsgn32 delayms;            /* delay per-stage in ms */
    Unsgn32 looptype;           /* the type of loop (asc/dec/pingpong/etc) */
  } texanim;

  /* color animations */
  struct {
    Unsgn32 color;              /* starting color, if specified */
    
    Unsgn32 stages;             /* number of stages (delta ops)) 0 = inf*/
    Unsgn32 delayms;            /* delay per-stage in ms */
    Unsgn32 loops;              /* number of loops, 0 = inf */
    Unsgn32 looptype;           /* the type of loop (asc/dec//pingpong/etc) */

    real deltaa;                /* deltas to appliy to ARGB components */
    real deltar;
    real deltag;
    real deltab;
  } colanim;

  /* geometry animations */
  struct {
    Unsgn32 stages;             /* number of stages (delta ops)) 0 = inf*/
    Unsgn32 delayms;            /* delay per-stage in ms */
    Unsgn32 loops;              /* number of loops, 0 = inf */
    Unsgn32 looptype;           /* the type of loop (asc/dec//pingpong/etc) */

    real deltax;                /* x y and z deltas */
    real deltay;
    real deltaz;
    real deltar;                /* rotation (degrees) */
    real deltas;                /* size */
  } geoanim;

  /* toggle animations (blinkers) */
  struct {
    Unsgn32 delayms;            /* delay per-stage in ms */
  } toganim;

} cqiAnimDefInitRec_t, *cqiAnimDefInitPtr_t ;


#ifdef NOEXTERN_CONQINIT
cqiGlobalInitPtr_t    cqiGlobal = NULL;
cqiShiptypeInitPtr_t  cqiShiptypes = NULL;
cqiPlanetInitPtr_t    cqiPlanets = NULL;
cqiTextureInitPtr_t   cqiTextures = NULL;
int                   cqiNumTextures = 0;
cqiAnimationInitPtr_t cqiAnimations = NULL;
int                   cqiNumAnimations = 0;
cqiAnimDefInitPtr_t   cqiAnimDefs = NULL;
int                   cqiNumAnimDefs = 0;

#else
extern cqiGlobalInitPtr_t    cqiGlobal;
extern cqiShiptypeInitPtr_t  cqiShiptypes;
extern cqiPlanetInitPtr_t    cqiPlanets;
extern cqiTextureInitPtr_t   cqiTextures;
extern int                   cqiNumTextures; 
extern cqiAnimationInitPtr_t cqiAnimations;
extern int                   cqiNumAnimations;
extern cqiAnimDefInitPtr_t   cqiAnimDefs;
extern int                   cqiNumAnimDefs;
#endif /* NOEXTERN_CONQINIT */


/* defines for which config file to parse (cqiLoadRC()) */
#define CQI_FILE_CONQINITRC     0
#define CQI_FILE_TEXTURESRC     1
#define CQI_FILE_TEXTURESRC_ADD 2

int cqiLoadRC(int rcid, char *filename, int verbosity, int debugl);
int cqiFindPlanet(char *str);
void dumpUniverse(void);

void dumpInitDataHdr(void);
void dumpTexDataHdr(void);

/* planinit.c */
void cqiInitPlanets(void);

#endif /* _CONQINIT_H */
