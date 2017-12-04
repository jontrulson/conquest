/*
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _CONQINIT_H
#define _CONQINIT_H


#ifdef __cplusplus
extern "C" {
#endif


#define CQI_NAMELEN 64           /* maximum length of a WKN or filename (no
                                    path) */



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
    char texname[CQI_NAMELEN];     /* texid */
} cqiPlanetInitRec_t, *cqiPlanetInitPtr_t;

/* texture flags */
#define CQITEX_F_COLOR_SPEC       0x00000001 /* This texture
                                                definition only
                                                specifies a color and
                                                not a texture */
#define CQITEX_F_GEN_MIPMAPS      0x00000002 /* Generate mipmaps for this
                                                texture */

#define CQITEX_F_IS_LUMINANCE     0x00000004 /* gen tex as a luminance
                                                texture */

#define CQITEX_F_HAS_COLOR        0x00000008 /* tex specified a color */


typedef struct _cqi_texture_area {
    char name[CQI_NAMELEN];       /* WKN of area */
    real x, y;                    /* X/Y coord of lower left of area as
                                     measured from the upper left (0.0)
                                     of the texture as a whole */
    real w, h;                    /* w/h of area */
} cqiTextureAreaRec_t, *cqiTextureAreaPtr_t;

/* textures */
typedef struct _cqi_texture_init {
    char                 name[CQI_NAMELEN]; /* texid */
    char                 filename[CQI_NAMELEN]; /* if different from texid */
    uint32_t             flags; /* flags for this cqi texture (CQITEX_F_*)*/
    uint32_t             color;  /* hex encoded color (AARRGGBB) */
    real                 prescale;
    int                  numTexAreas; /* number of texture areas */
    cqiTextureAreaPtr_t  texareas; /* optional texture areas */
} cqiTextureInitRec_t, *cqiTextureInitPtr_t ;


/* animations */

/* animation types */
#define CQI_ANIMS_TEX      0x00000001
#define CQI_ANIMS_COL      0x00000002
#define CQI_ANIMS_GEO      0x00000004
#define CQI_ANIMS_TOG      0x00000008

#define CQI_ANIMS_MASK     (CQI_ANIMS_TEX |     \
                            CQI_ANIMS_COL |     \
                            CQI_ANIMS_GEO |     \
                            CQI_ANIMS_TOG)

/* animation declarations - these associate animation names to
   an animation definition (animdef) */
typedef struct _cqi_animation_init {
    char name[CQI_NAMELEN];        /* animation name */
    char animdef[CQI_NAMELEN];     /* anim defintition to use for this
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
    char     name[CQI_NAMELEN];   /* animation definition name */
    char     texname[CQI_NAMELEN]; /* base texture if specified. */

    uint32_t  timelimit;            /* expire anim after this long (ms) 0=inf */
    uint32_t  anims;                /* bitmask of animation types that
                                       were specified (CQI_ANIMS_*) */

    /* initial state values (istate) specifically defined in the
       animdef definition */
    uint32_t  istates;              /* AD_ISTATE_* */
    char     itexname[CQI_NAMELEN];
    uint32_t  icolor;
    real     iangle;
    real     isize;

    /* texture animations */
    struct {
        uint32_t color;              /* will override per-tex colors */
        uint32_t stages;             /* number of stages (textures) */
        uint32_t delayms;            /* delay per-stage in ms */
        uint32_t loops;              /* number of loops, 0 = inf */
        uint32_t looptype;           /* the type of loop (asc/dec/pingpong/etc) */

        /* texcoord anims */
        real    deltas;             /* s and t deltas */
        real    deltat;
    } texanim;

    /* color animations */
    struct {
        uint32_t color;              /* starting color, if specified */

        uint32_t stages;             /* number of stages (delta ops)) 0 = inf*/
        uint32_t delayms;            /* delay per-stage in ms */
        uint32_t loops;              /* number of loops, 0 = inf */
        uint32_t looptype;           /* the type of loop (asc/dec//pingpong/etc) */

        real    deltaa;          /* deltas to appliy to ARGB components */
        real    deltar;
        real    deltag;
        real    deltab;
    } colanim;

    /* geometry animations */
    struct {
        uint32_t stages;             /* number of stages (delta ops)) 0 = inf*/
        uint32_t delayms;            /* delay per-stage in ms */
        uint32_t loops;              /* number of loops, 0 = inf */
        uint32_t looptype;           /* the type of loop (asc/dec//pingpong/etc) */

        real    deltax;             /* x y and z deltas */
        real    deltay;
        real    deltaz;
        real    deltar;             /* rotation (degrees) */
        real    deltas;             /* size */
    } geoanim;

    /* toggle animations (blinkers) */
    struct {
        uint32_t delayms;            /* delay per-stage in ms */
    } toganim;

} cqiAnimDefInitRec_t, *cqiAnimDefInitPtr_t ;

/* sounds */

typedef struct _cqi_sound_conf {
    int samplerate;               /* mixer sample rate */
    int stereo;                   /* enable stereo? */
    int fxchannels;               /* num of fx channels to allocate */
    int chunksize;                /* buffer size for mixing samples */
} cqiSoundConfRec_t, *cqiSoundConfPtr_t;

typedef struct _cqi_sound {
    char    name[CQI_NAMELEN];    /* sound wkn */
    char    filename[CQI_NAMELEN]; /* if different from name */
    int     volume;
    int     pan;
    int     fadeinms;
    int     fadeoutms;
    int     loops;
    int     limit;                /* max # running at one time */
    int     framelimit;           /* max number to run per frame, ignored if
                                     limit is specified */
    uint32_t delayms;              /* minimum delay for multiple instances */
} cqiSoundRec_t, *cqiSoundPtr_t;

#ifdef NOEXTERN_CONQINIT
cqiGlobalInitPtr_t           cqiGlobal = NULL;
cqiShiptypeInitPtr_t         cqiShiptypes = NULL;
cqiPlanetInitPtr_t           cqiPlanets = NULL;
cqiTextureInitPtr_t          cqiTextures = NULL;
int                          cqiNumTextures = 0;
cqiAnimationInitPtr_t        cqiAnimations = NULL;
int                          cqiNumAnimations = 0;
cqiAnimDefInitPtr_t          cqiAnimDefs = NULL;
int                          cqiNumAnimDefs = 0;

cqiSoundConfPtr_t            cqiSoundConf = NULL;
cqiSoundPtr_t                cqiSoundEffects = NULL;
int                          cqiNumSoundEffects = 0;
cqiSoundPtr_t                cqiSoundMusic = NULL;
int                          cqiNumSoundMusic = 0;

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

extern cqiSoundConfPtr_t     cqiSoundConf;
extern cqiSoundPtr_t         cqiSoundEffects;
extern int                   cqiNumSoundEffects;
extern cqiSoundPtr_t         cqiSoundMusic;
extern int                   cqiNumSoundMusic;
#endif /* NOEXTERN_CONQINIT */


/* defines for which config file to parse (cqiLoadRC()) */
#define CQI_FILE_CONQINITRC     0
#define CQI_FILE_TEXTURESRC     1
#define CQI_FILE_TEXTURESRC_ADD 2
#define CQI_FILE_SOUNDRC        3
#define CQI_FILE_SOUNDRC_ADD    4

int                 cqiLoadRC(int rcid, char *filename, int verbosity,
                              int debugl);

int                 cqiFindPlanet(char *str);
cqiTextureAreaPtr_t cqiFindTexArea(char *texnm, const char *tanm,
                                   cqiTextureAreaPtr_t defaultta);
int                 cqiFindEffect(char *str);
int                 cqiFindMusic(char *str);

void                dumpUniverse(void);

void                dumpInitDataHdr(void);
void                dumpSoundDataHdr(void);
void                dumpTexDataHdr(void);

/* planinit.c */
void                cqiInitPlanets(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _CONQINIT_H */
