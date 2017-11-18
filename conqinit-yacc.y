%{
/*
 * conqinit - yacc parser for conqinit
 *
 */

#include "c_defs.h"

#include "conqdef.h"
#include "conqcom.h"

#include "global.h"
#include "color.h"

#define NOEXTERN_CONQINIT
#include "conqinit.h"

#include "conqutil.h"
#include "rndlb.h"

/* The default initdata */
#if 1
#include "initdata.h"
#include "sounddata.h"
#else
#warning "Enable this for debugging only! It will core if you use it in anything other than conqinit!"
  cqiGlobalInitRec_t    defaultGlobalInit = {};
  cqiShiptypeInitPtr_t  defaultShiptypes = NULL;
  cqiPlanetInitPtr_t    defaultPlanets = NULL;

  cqiSoundConfRec_t     defaultSoundConf = {};
  cqiSoundPtr_t         defaultSoundEffects = NULL;
  int                   defaultNumSoundEffects = 0;
  cqiSoundPtr_t         defaultSoundMusic = NULL;
  int                   defaultNumSoundMusic = 0;
#endif

static int cqiVerbose = 0;
int cqiDebugl = 0;

static char *ptr;

/* a nesting depth of 0 implies the top-level, though currently only
 * other sections can be defined in it, so PREVSECTION looks only at
 * nesting levels > 1.  Bascause of this, your max available nesting
 * depth is really MAX_NESTING_DEPTH - 1.
 */
#define MAX_NESTING_DEPTH     16
static int sections[MAX_NESTING_DEPTH] = {};
static int curDepth = 0;

#define CURSECTION()          (sections[curDepth])
#define PREVSECTION()         ((curDepth > 1) ? sections[curDepth - 1] : 0)

extern int lineNum;
extern int goterror;
extern void yyerror(char *s);

extern int yylex(void);

static cqiGlobalInitPtr_t      _cqiGlobal;
static cqiShiptypeInitPtr_t    _cqiShiptypes;
static cqiPlanetInitPtr_t      _cqiPlanets;
static cqiTextureInitPtr_t     _cqiTextures;
static cqiAnimationInitPtr_t   _cqiAnimations;
static cqiAnimDefInitPtr_t     _cqiAnimDefs;

static cqiSoundConfPtr_t       _cqiSoundConf;
static cqiSoundPtr_t           _cqiSoundEffects;
static cqiSoundPtr_t           _cqiSoundMusic;

static int globalRead        = FALSE;
static int numShiptypes      = 0;
static int numPlanets        = 0;
static int numTextures       = 0;
static int numAnimations     = 0;
static int numAnimDefs       = 0;
static int numSoundEffects   = 0;
static int numSoundMusic     = 0;

/* # loaded per file */
static int fileNumTextures   = 0;
static int fileNumAnimations = 0;
static int fileNumAnimDefs   = 0;
static int fileNumEffects    = 0;
static int fileNumMusic      = 0;

static cqiPlanetInitRec_t      currPlanet;
static cqiTextureInitRec_t     currTexture;

static cqiAnimationInitRec_t   currAnimation;
static cqiAnimDefInitRec_t     currAnimDef;

static cqiTextureAreaRec_t     currTexArea;
static int numTexAreas       = 0;
static cqiTextureAreaPtr_t currTexAreas = NULL;

static cqiSoundRec_t           currSound;

static void startSection(int section);
static void endSection(void);

static void cfgSectioni(int item, int val);
static void cfgSectionil(int item, int val1, int val2);
static void cfgSectionf(int item, real val);
static void cfgSections(int item, char *val);
static void cfgSectionb(int item, char *val);
static char *sect2str(int section);
static char *item2str(int item);
static char *team2str(int pteam);
static int str2team(char *str);
static void initrun(int rcid);
static void checkStr(char *str);
static int parsebool(char *str);


%}

%union
{
  int num;
  char *ptr;
  real rnum;
};

%token <num> TOK_OPENSECT TOK_CLOSESECT TOK_NUMBER
%token <num> TOK_GLOBAL TOK_PLANETMAX TOK_SHIPMAX TOK_USERMAX TOK_MSGMAX
%token <num> TOK_HISTMAX
%token <num> TOK_SHIPTYPE TOK_ENGFAC TOK_WEAFAC TOK_ACCELFAC TOK_TORPWARP
%token <num> TOK_WARPMAX
%token <num> TOK_ARMYMAX TOK_SHMAX TOK_DAMMAX TOK_TORPMAX TOK_FUELMAX
%token <num> TOK_NAME TOK_SIZE
%token <num> TOK_PLANET TOK_PRIMARY TOK_ANGLE TOK_VELOCITY TOK_RADIUS
%token <num> TOK_PTYPE TOK_PTEAM
%token <num> TOK_ARMIES TOK_VISIBLE TOK_CORE TOK_XCOORD TOK_YCOORD
%token <num> TOK_TEXNAME TOK_COLOR
%token <num> TOK_HOMEPLANET TOK_TEXTURE TOK_FILENAME
%token <num> TOK_ANIMATION TOK_ANIMDEF
%token <num> TOK_STAGES TOK_LOOPS TOK_DELAYMS TOK_LOOPTYPE TOK_TIMELIMIT
%token <num> TOK_TEXANIM TOK_COLANIM TOK_GEOANIM TOK_TOGANIM TOK_ISTATE
%token <num> TOK_DELTAA TOK_DELTAR TOK_DELTAG TOK_DELTAB TOK_DELTAX
%token <num> TOK_DELTAY TOK_DELTAZ TOK_DELTAS

%token <num> TOK_SOUNDCONF TOK_SAMPLERATE TOK_VOLUME TOK_PAN TOK_STEREO
%token <num> TOK_FXCHANNELS TOK_CHUNKSIZE
%token <num> TOK_EFFECT TOK_FADEINMS TOK_FADEOUTMS TOK_LIMIT TOK_FRAMELIMIT
%token <num> TOK_MUSIC
%token <num> TOK_DELTAT TOK_SCOORD TOK_TCOORD TOK_WIDTH TOK_HEIGHT TOK_TEXAREA
%token <num> TOK_MIPMAP TOK_TEX_LUMINANCE

%token <ptr>  TOK_STRING
%token <rnum> TOK_RATIONAL

%type <ptr>  string
%type <num>  number
%type <rnum> rational

%start conqinit

%%
conqinit        : sections
                ;

sections        : /* empty */
                | sections section
                ;

section         : globalconfig
                | shiptypeconfig
                | planetconfig
                | textureconfig
                | animationconfig
                | animdefconfig
                | soundconfconfig
                | effectconfig
                | musicconfig
                ;

globalconfig    : startglobal stmts closesect
                ;

startglobal     : globalword opensect
                {
                   startSection(TOK_GLOBAL);
                }
                ;

globalword      : TOK_GLOBAL
                {;}
                ;

shiptypeconfig  : startshiptype stmts closesect
                ;

startshiptype   : shiptypeword opensect
                {
                   startSection(TOK_SHIPTYPE);
                }
                ;

shiptypeword    : TOK_SHIPTYPE
                {;}
                ;

planetconfig    : startplanet stmts closesect
                ;

startplanet     : planetword opensect
                {
                   startSection(TOK_PLANET);
                }
                ;

planetword      : TOK_PLANET
                {;}
                ;

textureconfig   : starttexture stmts closesect
                ;

starttexture    : textureword opensect
                {
                   startSection(TOK_TEXTURE);
                }
                ;

textureword     : TOK_TEXTURE
                {;}
                ;

animationconfig : startanimation stmts closesect
                ;

startanimation  : animationword opensect
                {
                   startSection(TOK_ANIMATION);
                }
                ;

animationword   : TOK_ANIMATION
                {;}
                ;

animdefconfig   : startanimdef stmts closesect
                ;


startanimdef    : animdefword opensect
                {
                    startSection(TOK_ANIMDEF);
                }
                ;

animdefword     : TOK_ANIMDEF
                {;}
                ;

texanimconfig   : starttexanim stmts closesect
                ;

starttexanim    : texanimword opensect
                {
                    startSection(TOK_TEXANIM);
                }
                ;

texanimword     : TOK_TEXANIM
                {;}
                ;

colanimconfig   : startcolanim stmts closesect
                ;

startcolanim    : colanimword opensect
                {
                    startSection(TOK_COLANIM);
                }
                ;

colanimword     : TOK_COLANIM
                {;}
                ;

geoanimconfig   : startgeoanim stmts closesect
                ;

startgeoanim    : geoanimword opensect
                {
                    startSection(TOK_GEOANIM);
                }
                ;

geoanimword     : TOK_GEOANIM
                {;}
                ;

toganimconfig   : starttoganim stmts closesect
                ;

starttoganim    : toganimword opensect
                {
                    startSection(TOK_TOGANIM);
                }
                ;

toganimword     : TOK_TOGANIM
                {;}
                ;

istateconfig    : startistate stmts closesect
                ;

startistate     : istateword opensect
                {
                    startSection(TOK_ISTATE);
                }
                ;

istateword     : TOK_ISTATE
                {;}
                ;


texareaconfig  : starttexarea stmts closesect
                ;

starttexarea   : texareaword opensect
                {
                    startSection(TOK_TEXAREA);
                }
                ;

texareaword     : TOK_TEXAREA
                {;}
                ;

soundconfconfig    : startsoundconf stmts closesect
                   ;

startsoundconf     : soundconfword opensect
                   {
                      startSection(TOK_SOUNDCONF);
                   }
                   ;

soundconfword      : TOK_SOUNDCONF
                   {;}
                   ;

effectconfig   : starteffect stmts closesect
               ;

starteffect    : effectword opensect
               {
                  startSection(TOK_EFFECT);
               }
               ;

effectword     : TOK_EFFECT
               {;}
               ;


musicconfig    : startmusic stmts closesect
               ;

startmusic     : musicword opensect
               {
                  startSection(TOK_MUSIC);
               }
               ;

musicword      : TOK_MUSIC
               {;}
               ;


opensect        : TOK_OPENSECT
                {;}
                ;

closesect       : TOK_CLOSESECT
                {endSection();}
                ;

stmts           : /* empty */
                | stmts stmt
                | stmts texanimconfig
                | stmts colanimconfig
                | stmts geoanimconfig
                | stmts toganimconfig
                | stmts istateconfig
                | stmts texareaconfig
                | stmts animdefconfig
                ;

stmt            : TOK_PLANETMAX number
                   {
                        cfgSectioni(TOK_PLANETMAX, $2);
                   }
                | TOK_SHIPMAX number
                   {
                        cfgSectioni(TOK_SHIPMAX, $2);
                   }
                | TOK_USERMAX number
                   {
                        cfgSectioni(TOK_USERMAX, $2);
                   }
                | TOK_HISTMAX number
                   {
                        cfgSectioni(TOK_HISTMAX, $2);
                   }
                | TOK_MSGMAX number
                   {
                        cfgSectioni(TOK_MSGMAX, $2);
                   }
                | TOK_NAME string
                   {
                        cfgSections(TOK_NAME, $2);
                   }
                | TOK_ENGFAC rational
                   {
                        cfgSectionf(TOK_ENGFAC, $2);
                   }
                | TOK_WEAFAC rational
                   {
                        cfgSectionf(TOK_WEAFAC, $2);
                   }
                | TOK_ACCELFAC rational
                   {
                        cfgSectionf(TOK_ACCELFAC, $2);
                   }
                | TOK_TORPWARP number
                   {
                        cfgSectioni(TOK_TORPWARP, $2);
                   }
                | TOK_WARPMAX number
                   {
                        cfgSectioni(TOK_WARPMAX, $2);
                   }
                | TOK_ARMYMAX number
                   {
                        cfgSectioni(TOK_ARMYMAX, $2);
                   }
                | TOK_SHMAX number
                   {
                        cfgSectioni(TOK_SHMAX, $2);
                   }
                | TOK_DAMMAX number
                   {
                        cfgSectioni(TOK_DAMMAX, $2);
                   }
                | TOK_TORPMAX number
                   {
                        cfgSectioni(TOK_TORPMAX, $2);
                   }
                | TOK_FUELMAX number
                   {
                        cfgSectioni(TOK_FUELMAX, $2);
                   }
                | TOK_SIZE number
                   {
                        cfgSectioni(TOK_SIZE, $2);
                   }
                | TOK_HOMEPLANET string
                   {
                        cfgSectionb(TOK_HOMEPLANET, $2);
                   }
                | TOK_PRIMARY string
                   {
                        cfgSections(TOK_PRIMARY, $2);
                   }
                | TOK_ANGLE rational
                   {
                        cfgSectionf(TOK_ANGLE, $2);
                   }
                | TOK_VELOCITY rational
                   {
                        cfgSectionf(TOK_VELOCITY, $2);
                   }
                | TOK_RADIUS rational
                   {
                        cfgSectionf(TOK_RADIUS, $2);
                   }
                | TOK_PTYPE string
                   {
                        cfgSections(TOK_PTYPE, $2);
                   }
                | TOK_PTEAM string
                   {
                        cfgSections(TOK_PTEAM, $2);
                   }
                | TOK_ARMIES number number
                   {
                        cfgSectionil(TOK_ARMIES, $2, $3);
                   }
                | TOK_ARMIES number
                   {
                        cfgSectioni(TOK_ARMIES, $2);
                   }
                | TOK_VISIBLE string
                   {
                        cfgSectionb(TOK_VISIBLE, $2);
                   }
                | TOK_CORE string
                   {
                        cfgSectionb(TOK_CORE, $2);
                   }
                | TOK_MIPMAP string
                   {
                        cfgSectionb(TOK_MIPMAP, $2);
                   }
                | TOK_XCOORD rational
                   {
                        cfgSectionf(TOK_XCOORD, $2);
                   }
                | TOK_YCOORD rational
                   {
                        cfgSectionf(TOK_YCOORD, $2);
                   }
                | TOK_TEXNAME string
                   {
                        cfgSections(TOK_TEXNAME, $2);
                   }
                | TOK_COLOR string
                   {
                        cfgSections(TOK_COLOR, $2);
                   }
                | TOK_FILENAME string
                   {
                        cfgSections(TOK_FILENAME, $2);
                   }
                | TOK_ANIMDEF string
                   {            /* in this form, it's a statement
                                   rather than a section */
                        cfgSections(TOK_ANIMDEF, $2);
                   }
                | TOK_STAGES number
                   {
                        cfgSectioni(TOK_STAGES, $2);
                   }
                | TOK_LOOPS number
                   {
                        cfgSectioni(TOK_LOOPS, $2);
                   }
                | TOK_DELAYMS number
                   {
                        cfgSectioni(TOK_DELAYMS, $2);
                   }
                | TOK_LOOPTYPE number
                   {
                        cfgSectioni(TOK_LOOPTYPE, $2);
                   }
                | TOK_DELTAA rational
                   {
                        cfgSectionf(TOK_DELTAA, $2);
                   }
                | TOK_DELTAR rational
                   {
                        cfgSectionf(TOK_DELTAR, $2);
                   }
                | TOK_DELTAG rational
                   {
                        cfgSectionf(TOK_DELTAG, $2);
                   }
                | TOK_DELTAB rational
                   {
                        cfgSectionf(TOK_DELTAB, $2);
                   }
                | TOK_DELTAX rational
                   {
                        cfgSectionf(TOK_DELTAX, $2);
                   }
                | TOK_DELTAY rational
                   {
                        cfgSectionf(TOK_DELTAY, $2);
                   }
                | TOK_DELTAZ rational
                   {
                        cfgSectionf(TOK_DELTAZ, $2);
                   }
                | TOK_DELTAS rational
                   {
                        cfgSectionf(TOK_DELTAS, $2);
                   }
                | TOK_TIMELIMIT number
                   {
                        cfgSectioni(TOK_TIMELIMIT, $2);
                   }
                | TOK_SAMPLERATE number
                   {
                        cfgSectioni(TOK_SAMPLERATE, $2);
                   }
                | TOK_VOLUME number
                   {
                        cfgSectioni(TOK_VOLUME, $2);
                   }
                | TOK_PAN number
                   {
                        cfgSectioni(TOK_PAN, $2);
                   }
                | TOK_STEREO string
                   {
                        cfgSectionb(TOK_STEREO, $2);
                   }
                | TOK_FXCHANNELS number
                   {
                        cfgSectioni(TOK_FXCHANNELS, $2);
                   }
                | TOK_CHUNKSIZE number
                   {
                        cfgSectioni(TOK_CHUNKSIZE, $2);
                   }
                | TOK_FADEINMS number
                   {
                        cfgSectioni(TOK_FADEINMS, $2);
                   }
                | TOK_FADEOUTMS number
                   {
                        cfgSectioni(TOK_FADEOUTMS, $2);
                   }
                | TOK_FRAMELIMIT number
                   {
                        cfgSectioni(TOK_FRAMELIMIT, $2);
                   }
                | TOK_LIMIT number
                   {
                        cfgSectioni(TOK_LIMIT, $2);
                   }
                | TOK_SCOORD rational
                   {
                        cfgSectionf(TOK_SCOORD, $2);
                   }
                | TOK_TCOORD rational
                   {
                        cfgSectionf(TOK_TCOORD, $2);
                   }
                | TOK_DELTAT rational
                   {
                        cfgSectionf(TOK_DELTAT, $2);
                   }
                | TOK_WIDTH rational
                   {
                        cfgSectionf(TOK_WIDTH, $2);
                   }
                | TOK_HEIGHT rational
                   {
                        cfgSectionf(TOK_HEIGHT, $2);
                   }
                | TOK_TEX_LUMINANCE string
                   {
                        cfgSectionb(TOK_TEX_LUMINANCE, $2);
                   }
                | error closesect
                ;

string		: TOK_STRING
                  {
                    ptr = (char *)strdup($1);
                    if (ptr)
                      checkStr(ptr);
                    $$ = ptr;
                  }
                ;
number		: TOK_NUMBER		{ $$ = $1; }
		;
rational	: TOK_RATIONAL		{ $$ = $1; }
		;

%%

                  /* search the 'public' planet list */
int cqiFindPlanet(char *str)
{
    int i;

    for (i=0; i < cqiGlobal->maxplanets; i++)
        if (!strncmp(cqiPlanets[i].name, str, MAXPLANETNAME))
            return i;

    return -1;
}

/* search internal planet list */
static int _cqiFindPlanet(char *str)
{
    int i;

    for (i=0; i < numPlanets; i++)
        if (!strncmp(_cqiPlanets[i].name, str, MAXPLANETNAME))
            return i;

    return -1;
}

/* search internal texture list */
static int _cqiFindTexture(char *texname)
{
    int i;

    for (i=0; i<numTextures; i++)
        if (!strncmp(_cqiTextures[i].name, texname, CQI_NAMELEN))
            return i;

    return -1;
}

/* search the public tex area list */
cqiTextureAreaPtr_t cqiFindTexArea(char *texnm, char *tanm,
                                   cqiTextureAreaPtr_t defaultta)
{
    int tidx, i;

    if (!texnm || !tanm)
    {
        return defaultta;
    }

    if ((tidx = _cqiFindTexture(texnm)) == -1)
    {
        return defaultta;
    }

    for (i=0; i<_cqiTextures[tidx].numTexAreas; i++)
        if (!strncmp(_cqiTextures[tidx].texareas[i].name, tanm, MAXPLANETNAME))
            return &(_cqiTextures[tidx].texareas[i]);

    utLog("%s: could not find texarea %s in texture %s",
          __FUNCTION__,
          tanm, texnm);

    return defaultta;
}

/* search internal animation list */
static int _cqiFindAnimation(char *animname)
{
    int i;

    for (i=0; i<numAnimations; i++)
        if (!strncmp(_cqiAnimations[i].name, animname, CQI_NAMELEN))
            return i;

    return -1;
}

static int _cqiFindAnimDef(char *defname)
{
    int i;

    for (i=0; i<numAnimDefs; i++)
        if (!strncmp(_cqiAnimDefs[i].name, defname, CQI_NAMELEN))
            return i;

    return -1;
}

/* search internal effect list */
static int _cqiFindEffect(char *name)
{
    int i;

    for (i=0; i<numSoundEffects; i++)
        if (!strncmp(_cqiSoundEffects[i].name, name, CQI_NAMELEN))
            return i;

    return -1;
}

static int _cqiFindMusic(char *name)
{
    int i;

    for (i=0; i<numSoundMusic; i++)
        if (!strncmp(_cqiSoundMusic[i].name, name, CQI_NAMELEN))
            return i;

    return -1;
}

/* search public effect list */
int cqiFindEffect(char *name)
{
    int i;

    for (i=0; i<cqiNumSoundEffects; i++)
        if (!strncmp(cqiSoundEffects[i].name, name, CQI_NAMELEN))
            return i;

    return -1;
}

int cqiFindMusic(char *name)
{
    int i;

    for (i=0; i<cqiNumSoundMusic; i++)
        if (!strncmp(cqiSoundMusic[i].name, name, CQI_NAMELEN))
            return i;

    return -1;
}

static uint32_t hex2color(char *str)
{
    uint32_t v;

    /* default to 0 (black/transparent) */

    if (!str)
        return 0;

    if (sscanf(str, "%x", &v) != 1)
    {
        utLog("hex2color(): invalid color specification '%s' at line %d, setting to 0",
              str, lineNum);
        v = 0;
    }

    return v;
}


static int cqiValidateAnimations(void)
{
    int i, j;
    char tbuf[CQI_NAMELEN];
    int ndx;

    /* if no textures, no point */
    if (!_cqiTextures)
        return FALSE;

    if (!numAnimDefs || !numAnimations)
        return FALSE;

    /* go through each anim def, checking various things */
    for (i=0; i<numAnimDefs; i++)
    {
        /* first, if a texname specified, and there is no texanim
           associated with this animdef, make sure the texname
           exists. */
        if (_cqiAnimDefs[i].texname[0] &&
            !(_cqiAnimDefs[i].anims & CQI_ANIMS_TEX))
        {                         /* texname and no tex anim */
            /* make sure the texture exists. */
            if (_cqiFindTexture(_cqiAnimDefs[i].texname) < 0)
            {                     /* nope */
                utLog("%s: animdef %s: texture %s does not exist.",
                      __FUNCTION__,
                      _cqiAnimDefs[i].name,
                      _cqiAnimDefs[i].texname);
                return FALSE;
            }
        }

        /* now check each anim type
         *
         * stages must be > 0
         * a stage == 0 means to disable an anim type, in which case
         *  the anim type will never have been be enabled (endSection)
         */

        /* texanim */
        if (_cqiAnimDefs[i].anims & CQI_ANIMS_TEX)
        {
            if (_cqiAnimDefs[i].texanim.stages <= 0)
            {
                utLog("%s: animdef %s: texanim: stages must greater than zero.",
                      __FUNCTION__,
                      _cqiAnimDefs[i].name);
                return FALSE;
            }

            /* must be a texname */
            if (!_cqiAnimDefs[i].texname[0])
            {
                /* copy the animdef name over */
                strncpy(_cqiAnimDefs[i].texname, _cqiAnimDefs[i].name,
                        CQI_NAMELEN - 1);
            }

            /* for each stage, build a texname and make sure it
               exists */
            for (j=0; j<_cqiAnimDefs[i].texanim.stages; j++)
            {

                /* if the texanim only contains a single stage (one texture)
                 * then do not append the stage number to the texname.
                 */

                if (_cqiAnimDefs[i].texanim.stages == 1)
                    snprintf(tbuf, CQI_NAMELEN - 1, "%s",
                             _cqiAnimDefs[i].texname);
                else
                    snprintf(tbuf, CQI_NAMELEN - 1, "%s%d",
                             _cqiAnimDefs[i].texname,
                             j);

                /* locate the texture */
                if (_cqiFindTexture(tbuf) < 0)
                {                     /* nope */
                    utLog("%s: animdef %s: texanim: texture %s does not exist.",
                          __FUNCTION__,
                          _cqiAnimDefs[i].name,
                          tbuf);
                    return FALSE;
                }
            }
        }

        /* colanim */
        if (_cqiAnimDefs[i].anims & CQI_ANIMS_COL)
        {
            if (_cqiAnimDefs[i].colanim.stages <= 0)
            {
                utLog("%s: animdef %s: colanim: stages must greater than zero.",
                      __FUNCTION__,
                      _cqiAnimDefs[i].name);
                return FALSE;
            }
        }

        /* geoanim */
        if (_cqiAnimDefs[i].anims & CQI_ANIMS_GEO)
        {
            /* if stages is 0 (meaning infinite) then loops is meaningless -
               set to 0 and warn. */
            if (_cqiAnimDefs[i].geoanim.stages <= 0)
            {
                utLog("%s: animdef %s: geoanim: stages must greater than zero.",
                      __FUNCTION__,
                      _cqiAnimDefs[i].name);
                return FALSE;
            }
        }
    } /* for */


    /* now, make sure each animation specifies an existing
       animdef, set up adIndex */
    for (i=0; i < numAnimations; i++)
    {
        if ((ndx = _cqiFindAnimDef(_cqiAnimations[i].animdef)) < 0)
        {                       /* nope */
            utLog("%s: animdef %s does not exist for animation %s.",
                  __FUNCTION__,
                  _cqiAnimations[i].animdef,
                  _cqiAnimations[i].name);
            return FALSE;
        }
        else                      /* save the index */
            _cqiAnimations[i].adIndex = ndx;
    }

    return TRUE;
}

static int cqiValidatePlanets(void)
{
    int i;
    /* first things first... If there was no global read, then no
       point in continuing */

    if (!globalRead)
        return FALSE;

    unsigned int homeplan[NUMPLAYERTEAMS] = {}; /* count of home planets */

    /* first fill in any empty slots */
    if (numPlanets < MAXPLANETS)
    {
        for (i = numPlanets; i < _cqiGlobal->maxplanets; i++)
        {
            /* use the slot number in the name to reduce chance of dup names */
            snprintf(_cqiPlanets[i].name, MAXPLANETNAME - 1, "ZZExtra %d", i);

            // these planets are stationary, so set the primary name
            // to be the same as the planet.
            strncpy(_cqiPlanets[i].primname, _cqiPlanets[i].name,
                    MAXPLANETNAME - 1);

            _cqiPlanets[i].primary = i;
            _cqiPlanets[i].angle = 0.0;
            _cqiPlanets[i].velocity = 0.0;
            _cqiPlanets[i].radius = 0.0;
            _cqiPlanets[i].ptype = PLANET_GHOST;
            _cqiPlanets[i].pteam = TEAM_NOTEAM;
            _cqiPlanets[i].armies = 0;
            _cqiPlanets[i].visible = FALSE;
            _cqiPlanets[i].core = FALSE;
            _cqiPlanets[i].xcoord = 0.0;
            _cqiPlanets[i].ycoord = 0.0;
            /* leave texname blank so default rules will fire if neccessary */
        }

        if (cqiVerbose)
            utLog("%s: filled %d unspecified planet slots.",
                  __FUNCTION__, MAXPLANETS - numPlanets);
    }


    for (i=0; i < numPlanets; i++)
    {
        // see if the primary name == name, if so, set velocity = 0,
        // and primary planet number (itself)

        if (!strncmp(_cqiPlanets[i].name, _cqiPlanets[i].primname,
                     MAXPLANETNAME))
        {
            _cqiPlanets[i].velocity = 0.0;

            _cqiPlanets[i].primary = i; // orbits itself == stationary
        }
        else
        {                       /* else, find the primary, default to mur */
            if ((_cqiPlanets[i].primary = _cqiFindPlanet(_cqiPlanets[i].primname)) < 0)
            {                   /* couldn't find it */
                utLog("%s: can't find primary '%s' for planet '%s', defaulting to itself: '%s'",
                      __FUNCTION__,
                      _cqiPlanets[i].primname,
                      _cqiPlanets[i].name,
                      _cqiPlanets[i].name);

                strncpy(_cqiPlanets[i].primname, _cqiPlanets[i].name,
                        MAXPLANETNAME - 1);
                _cqiPlanets[i].primary = i;
            }
        }

        // count home planets, each team MUST have at least one

        if (_cqiPlanets[i].homeplanet)
            switch (_cqiPlanets[i].pteam)
            {
            case TEAM_FEDERATION:
            case TEAM_ROMULAN:
            case TEAM_KLINGON:
            case TEAM_ORION:
                homeplan[_cqiPlanets[i].pteam]++;
                break;
            default:
                break;
            }
    }


    // make sure at least one is specified per team
    for (i=0; i < NUMPLAYERTEAMS; i++)
    {
        if (homeplan[i] < 1)
        {
            utLog("%s: team %s must have at least 1 homeplanet. %d were "
                  "specified.",
                  __FUNCTION__, team2str(i), homeplan[i]);
            return FALSE;
        }
    }

    if (cqiVerbose)
        utLog("%s: total planets %d (%d loaded, %d extra)",
              __FUNCTION__,
              _cqiGlobal->maxplanets,
              numPlanets,
              _cqiGlobal->maxplanets - numPlanets);


    return TRUE;
}

/* parse a file */
int cqiLoadRC(int rcid, char *filename, int verbosity, int debugl)
{
    FILE *infile;
    extern FILE *yyin;
    int fail = FALSE;
    char buffer[BUFFER_SIZE_256];

    cqiDebugl = debugl;
    cqiVerbose = verbosity;

    switch (rcid)
    {
    case CQI_FILE_CONQINITRC:   /* optional */
        if (filename)
            strncpy(buffer, filename, BUFFER_SIZE_256 - 1);
        else
            snprintf(buffer, sizeof(buffer)-1, "%s/%s", utGetPath(CONQETC),
                     "conqinitrc");
        break;
    case CQI_FILE_TEXTURESRC:
    case CQI_FILE_TEXTURESRC_ADD:
        if (filename)
            strncpy(buffer, filename, BUFFER_SIZE_256 - 1);
        else
            snprintf(buffer, sizeof(buffer)-1, "%s/%s", utGetPath(CONQETC),
                     "texturesrc");
        break;
    case CQI_FILE_SOUNDRC:
    case CQI_FILE_SOUNDRC_ADD:
        if (filename)
            strncpy(buffer, filename, BUFFER_SIZE_256 - 1);
        else
            snprintf(buffer, sizeof(buffer)-1, "%s/%s", utGetPath(CONQETC),
                     "soundrc");
        break;
    default:                    /* programmer error */
        utLog("%s: invalid rcid %d, bailing.", __FUNCTION__, rcid);
        return FALSE;
        break;
    }

    utLog("%s: Loading '%s'...", __FUNCTION__, buffer);
    if ((infile = fopen(buffer, "r")) == NULL)
    {
        utLog("%s: fopen(%s) failed: %s",
              __FUNCTION__,
              buffer,
              strerror(errno));

        /* a failed CQI_FILE_TEXTURESRC_ADD is no big deal,
           CQI_FILE_TEXTURESRC/CONQINITRC is another story however... */

        utLog("%s: using default init tables.", __FUNCTION__);
        switch(rcid)
        {
        case CQI_FILE_TEXTURESRC:
        {
            utLog("%s: FATAL: no textures.", __FUNCTION__);
            return FALSE;
        }
        break;

        case CQI_FILE_SOUNDRC:
        {
            cqiSoundConf = &defaultSoundConf;
            cqiSoundEffects  = defaultSoundEffects;
            cqiNumSoundEffects = defaultNumSoundEffects;
            cqiSoundMusic = defaultSoundMusic;
            cqiNumSoundMusic = defaultNumSoundMusic;
        }
        break;
        }

        return FALSE;
    }

    initrun(rcid);

    yyin = infile;

    goterror = FALSE;
    lineNum = 0;
    if ( yyparse() == ERR || goterror )
    {
        utLog("conqinit: parse error." );
        fail = TRUE;
    }

    fclose(infile);

    /* check textures early */
    if (rcid == CQI_FILE_TEXTURESRC ||
        rcid == CQI_FILE_TEXTURESRC_ADD)
    {
        if (fail && rcid == CQI_FILE_TEXTURESRC)
        {
            utLog("%s: FATAL: no textures.", __FUNCTION__);
            return FALSE;
        }

        cqiTextures = _cqiTextures;
        cqiNumTextures = numTextures;

        if (cqiVerbose)
            utLog("%s: loaded %d texture descriptors.",
                  __FUNCTION__, fileNumTextures);

        /* now validate any animations */
        if (!cqiValidateAnimations())
        {
            utLog("%s: FATAL: no animations.", __FUNCTION__);
            return FALSE;
        }

        if (cqiVerbose)
        {
            utLog("%s: loaded %d Animation descriptors.",
                  __FUNCTION__, fileNumAnimations);
            utLog("%s: loaded %d Animation definitions.",
                  __FUNCTION__, fileNumAnimDefs);
        }

        cqiAnimations = _cqiAnimations;
        cqiNumAnimations = numAnimations;
        cqiAnimDefs = _cqiAnimDefs;
        cqiNumAnimDefs = numAnimDefs;

        return TRUE;
    }

    /* sounds */
    if (rcid == CQI_FILE_SOUNDRC || rcid == CQI_FILE_SOUNDRC_ADD)
    {
        if (fail && rcid == CQI_FILE_SOUNDRC)
        {
            utLog("%s: using default sound data.", __FUNCTION__);
            cqiSoundConf = &defaultSoundConf;
            cqiSoundEffects  = defaultSoundEffects;
            cqiNumSoundEffects = defaultNumSoundEffects;
            cqiSoundMusic = defaultSoundMusic;
            cqiNumSoundMusic = defaultNumSoundMusic;

            return FALSE;
        }

        utLog("%s: loaded %d Music definitions.",
              __FUNCTION__, fileNumMusic);
        utLog("%s: loaded %d Effect definitions.",
              __FUNCTION__, fileNumEffects);
        cqiSoundConf = _cqiSoundConf;
        cqiSoundEffects = _cqiSoundEffects;
        cqiNumSoundEffects = numSoundEffects;
        cqiSoundMusic = _cqiSoundMusic;
        cqiNumSoundMusic = numSoundMusic;

        return TRUE;
    }


    if (!fail && !cqiValidatePlanets())
    {
        utLog("%s: cqiValidatePlanets() failed.", __FUNCTION__);

        cqiGlobal    = &defaultGlobalInit;
        cqiShiptypes = defaultShiptypes;
        cqiPlanets   = defaultPlanets;
        utLog("%s: using default init tables.", __FUNCTION__);
        return FALSE;
    }

    if (!fail)
    {
        /* if we were successful, export the new tables */
        cqiGlobal    = _cqiGlobal;
        cqiShiptypes = _cqiShiptypes;
        cqiPlanets   = _cqiPlanets;
    }
    else
    {                           /* use the defaults */
        utLog("%s: using default init tables.", __FUNCTION__);
        cqiGlobal    = &defaultGlobalInit;
        cqiShiptypes = defaultShiptypes;
        cqiPlanets   = defaultPlanets;
    }

    return (fail) ? FALSE: TRUE;
}

static char *ptype2str(int ptype)
{
    switch (ptype)
    {
    case PLANET_CLASSM:
        return "M";
        break;
    case PLANET_DEAD:
        return "D";
        break;
    case PLANET_SUN:
        return "S";
        break;
    case PLANET_MOON:
        return "m";
        break;
    case PLANET_GHOST:
        return "G";
        break;
    case PLANET_CLASSA:
        return "A";
        break;
    case PLANET_CLASSO:
        return "O";
        break;
    case PLANET_CLASSZ:
        return "Z";
        break;
    }

    return "?";
}

static int str2ptype(char *str)
{

    /* we will just look at the first byte  */
    switch (str[0])
    {
    case 'M':
        return PLANET_CLASSM;
        break;
    case 'D':
        return PLANET_DEAD;
        break;
    case 'S':
        return PLANET_SUN;
        break;
    case 'm':
        return PLANET_MOON;
        break;
    case 'G':
        return PLANET_GHOST;
        break;
    case 'A':
        return PLANET_CLASSA;
        break;
    case 'O':
        return PLANET_CLASSO;
        break;
    case 'Z':
        return PLANET_CLASSZ;
        break;
    }

    return PLANET_DEAD;           /* default */
}


static char *team2str(int pteam)
{
    switch (pteam)
    {
    case TEAM_FEDERATION:
        return "F";
        break;
    case TEAM_ROMULAN:
        return "R";
        break;
    case TEAM_KLINGON:
        return "K";
        break;
    case TEAM_ORION:
        return "O";
        break;
    case TEAM_SELFRULED:
        return "S";
        break;
    case TEAM_NOTEAM:
        return "N";
        break;
    case TEAM_GOD:
        return "G";
        break;
    case TEAM_EMPIRE:
        return "E";
        break;
    }

    return "?";
}

static int str2team(char *str)
{
    switch (str[0])
    {
    case 'F':
        return TEAM_FEDERATION;
        break;
    case 'R':
        return TEAM_ROMULAN;
        break;
    case 'K':
        return TEAM_KLINGON;
        break;
    case 'O':
        return TEAM_ORION;
        break;
    case 'S':
        return TEAM_SELFRULED;
        break;
    case 'N':
        return TEAM_NOTEAM;
        break;
    case 'G':
        return TEAM_GOD;
        break;
    case 'E':
        return TEAM_EMPIRE;
        break;
    }

    return TEAM_NOTEAM;           /* default */
}

/* Dump the parsed soundrc to stdout in sounddata.h format */
/* this includes soundconf, effects, and music */
void dumpSoundDataHdr(void)
{
    char buf[BUFFER_SIZE_128];
    int i;


    if (!cqiSoundConf || !cqiNumSoundEffects)
        return;

    /* preamble */
    utFormatTime( buf, 0 );
    printf("/* Generated by conqinit on %s */\n", buf);
    printf("\n\n");

    printf("#ifndef _SOUNDDATA_H\n");
    printf("#define _SOUNDDATA_H\n\n");


    printf("static cqiSoundConfRec_t defaultSoundConf = {\n");
    printf("  %d,\n", cqiSoundConf->samplerate);
    printf("  %d,\n", cqiSoundConf->stereo);
    printf("  %d,\n", cqiSoundConf->fxchannels);
    printf("  %d\n", cqiSoundConf->chunksize);
    printf("};\n\n");


    printf("static int defaultNumSoundMusic = %d;\n\n", cqiNumSoundMusic);

    /* if there is no music built in... */
    if (!cqiNumSoundMusic)
    {
        printf("static cqiSoundRec_t *defaultSoundMusic = NULL;\n");
        printf("\n\n");
    }
    else
    {
        printf("static cqiSoundRec_t defaultSoundMusic[%d] = {\n",
               cqiNumSoundMusic);

        /* music */
        for (i=0; i<cqiNumSoundMusic; i++)
            printf(" { \"%s\", \"%s\", %d, %d, %d, %d, %d, %d, %d, %d },\n",
                   cqiSoundMusic[i].name,
                   cqiSoundMusic[i].filename,
                   cqiSoundMusic[i].volume,
                   cqiSoundMusic[i].pan,
                   cqiSoundMusic[i].fadeinms,
                   cqiSoundMusic[i].fadeoutms,
                   cqiSoundMusic[i].loops,
                   cqiSoundMusic[i].limit,
                   cqiSoundMusic[i].framelimit,
                   cqiSoundMusic[i].delayms);

        printf("};\n\n");
    }

    /* effect */
    printf("static int defaultNumSoundEffects = %d;\n\n", cqiNumSoundEffects);
    printf("static cqiSoundRec_t defaultSoundEffects[%d] = {\n",
           cqiNumSoundEffects);

    for (i=0; i<cqiNumSoundEffects; i++)
        printf(" { \"%s\", \"%s\", %d, %d, %d, %d, %d, %d, %d, %d },\n",
               cqiSoundEffects[i].name,
               cqiSoundEffects[i].filename,
               cqiSoundEffects[i].volume,
               cqiSoundEffects[i].pan,
               cqiSoundEffects[i].fadeinms,
               cqiSoundEffects[i].fadeoutms,
               cqiSoundEffects[i].loops,
               cqiSoundEffects[i].limit,
               cqiSoundEffects[i].framelimit,
               cqiSoundEffects[i].delayms);

    printf("};\n\n");

    printf("#endif /* _SOUNDDATA_H */\n\n");


    return;
}


/* Dump the parsed initdata to stdout in initdata.h format */
void dumpInitDataHdr(void)
{
    char buf[BUFFER_SIZE_128];
    int i;

    /* preamble */
    utFormatTime( buf, 0 );
    printf("/* Generated by conqinit on %s */\n", buf);
    printf("\n\n");

    printf("#ifndef _INITDATA_H\n");
    printf("#define _INITDATA_H\n\n");

    /* FIXME, need to use dynamics when ready */
    /* globals always first! */

    printf("static cqiGlobalInitRec_t defaultGlobalInit = {\n");
    printf("\t %d,\n", cqiGlobal->maxplanets);
    printf("\t %d,\n", cqiGlobal->maxships);
    printf("\t %d,\n", cqiGlobal->maxusers);
    printf("\t %d,\n", cqiGlobal->maxhist);
    printf("\t %d,\n", cqiGlobal->maxmsgs);
    printf("\t %d,\n", cqiGlobal->maxtorps);
    printf("\t %d\n", cqiGlobal->maxshiptypes);
    printf("};\n\n");

    /* shiptypes */

    printf("/* The shiptype sections are currently ignored. */\n");

    printf("static cqiShiptypeInitRec_t defaultShiptypes[%d] = {\n",
           cqiGlobal->maxshiptypes);
    for (i=0; i < cqiGlobal->maxshiptypes; i++)
    {
        printf(" { \n");
        printf("   \"%s\",\n", cqiShiptypes[i].name);
        printf("   %f,\n", cqiShiptypes[i].engfac);
        printf("   %f,\n", cqiShiptypes[i].weafac);
        printf("   %f,\n", cqiShiptypes[i].accelfac);
        printf("   %d,\n", cqiShiptypes[i].torpwarp);
        printf("   %d,\n", cqiShiptypes[i].warpmax);
        printf("   %d,\n", cqiShiptypes[i].armymax);
        printf("   %d,\n", cqiShiptypes[i].shmax);
        printf("   %d,\n", cqiShiptypes[i].dammax);
        printf("   %d,\n", cqiShiptypes[i].torpmax);
        printf("   %d\n", cqiShiptypes[i].fuelmax);
        printf(" },\n");
    }
    printf("};\n\n");

    /* planets */
    printf("static cqiPlanetInitRec_t defaultPlanets[%d] = {\n",
           cqiGlobal->maxplanets);

    for (i=0; i < cqiGlobal->maxplanets; i++)
    {
        printf(" { \n");
        printf("   \"%s\",\n", cqiPlanets[i].name);

        // also handles the case where a planet orbits itself
        // (stationary)
        printf("   \"%s\",\n", cqiPlanets[cqiPlanets[i].primary].name);

        printf("   %f,\n", cqiPlanets[i].angle);
        printf("   %f,\n", cqiPlanets[i].velocity);
        printf("   %f,\n", cqiPlanets[i].radius);
        printf("   %d,\n", cqiPlanets[i].ptype);
        printf("   %d,\n", cqiPlanets[i].pteam);
        printf("   %d,\n", cqiPlanets[i].armies);
        printf("   %d,\n", cqiPlanets[i].visible);
        printf("   %d,\n", cqiPlanets[i].core);
        printf("   %d,\n", cqiPlanets[i].homeplanet);

        printf("   %f,\n", cqiPlanets[i].xcoord);
        printf("   %f,\n", cqiPlanets[i].ycoord);

        printf("   %d,\n", (int)cqiPlanets[i].size);

        /* we never write out a planet texture name */
        printf("   \"\",\n");

        printf(" },\n");

    }
    printf("};\n\n");

    printf("#endif /* _INITDATA_H */\n\n");


    return;
}


/* Dump the current universe to stdout in conqinitrc format */
void dumpUniverse(void)
{
    char buf[BUFFER_SIZE_128];
    int i, j;

    map_common();

    utFormatTime( buf, 0 );
    printf("# Generated by conqinit on %s\n", buf);
    printf("#\n#\n");
    /* comments */
    printf("# Valid values for planets->ptype: \n");
    printf("#     \"M\"               Class M (fuel)\n");
    printf("#     \"D\"               Class D (dead)\n");
    printf("#     \"S\"               Sun\n");
    printf("#     \"m\"               Moon\n");
    printf("#     \"G\"               Ghost\n");
    printf("#     \"A\"               Class A\n");
    printf("#     \"O\"               Class O\n");
    printf("#     \"Z\"               Class Z\n");
    printf("#\n#\n");
    printf("# Valid values for planets->pteam: \n");
    printf("#     \"F\"               Federation\n");
    printf("#     \"R\"               Romulan\n");
    printf("#     \"K\"               Klingon\n");
    printf("#     \"O\"               Orion\n");
    printf("#     \"S\"               Self Ruled\n");
    printf("#     \"N\"               No Team (non)\n");
    printf("#     \"G\"               God\n");
    printf("#     \"E\"               Empire\n");
    printf("#\n");
    printf("# armies can be specified as a single or pair of numbers.\n");
    printf("#\n");
    printf("# armies 50             sets armies to 50\n");
    printf("# armies 20 100         set armies to random value between 20 and 100\n");
    printf("#\n");
    printf("# If the angle specified for a planet is negative, then a random angle\n");
    printf("# will be chosen\n");

    printf("#\n\n");

    /* FIXME, need to use dynamics when ready */

    /* global is always first */

    printf("# DO NOT CHANGE VALUES IN THE GLOBAL SECTION\n");
    printf("# Doing so will break compatibility, and we aren't ready\n");
    printf("# for that yet.\n");
    printf("global {\n");
    printf("  planetmax 60\n");
    printf("  shipmax   20\n");
    printf("  usermax   500\n");
    printf("  histmax   40\n");
    printf("  msgmax    60\n");
    printf("}\n\n");

    /* shiptypes */

    printf("# The shiptype sections are currently ignored.\n");
    for (i=0; i < MAXNUMSHIPTYPES; i++)
    {
        printf("shiptype {\n");
        printf("  name       \"%s\"\n", ShipTypes[i].name);
        printf("  engfac     %f\n", ShipTypes[i].engfac);
        printf("  weafac     %f\n", ShipTypes[i].weafac);
        printf("  accelfac   %f\n", ShipTypes[i].accelfac);
        printf("  torpwarp   %d\n", (int)ShipTypes[i].torpwarp);
        printf("  warpmax    %d\n", (int)ShipTypes[i].warplim);
        printf("  armymax    %d\n", ShipTypes[i].armylim);
        printf("  shmax      100\n");
        printf("  dammax     100\n");
        printf("  torpmax    9\n");
        printf("  fuelmax    999\n");
        printf("}\n\n");
    }

    /* planets */
    for (i=0; i < MAXPLANETS; i++)
    {
        printf("planet {\n");
        printf("  name        \"%s\"\n", Planets[i].name);

        // also handles case of stationary planets where the primary
        // is the same as the planet
        printf("  primary     \"%s\"\n", Planets[Planets[i].primary].name);

        printf("  angle       %f\n", Planets[i].orbang);
        printf("  velocity    %f\n", Planets[i].orbvel);
        printf("  radius      %f\n", Planets[i].orbrad);
        printf("  ptype       \"%s\"\n", ptype2str(Planets[i].type));
        printf("  pteam       \"%s\"\n", team2str(Planets[i].team));
        printf("  armies      %d\n", Planets[i].armies);
        printf("  visible     \"%s\"\n", PVISIBLE(i) ? "yes" : "no");
        printf("  core        \"%s\"\n", PCORE(i) ? "yes" : "no");
        printf("  homeplanet  \"%s\"\n", PHOMEPLANET(i) ? "yes" : "no");

        printf("  x           %f\n", Planets[i].x);
        printf("  y           %f\n", Planets[i].y);

        switch(Planets[i].type)
        {
        case PLANET_SUN:
            printf("  size        1500\n");
            printf("  texname     \"star\"\n");
            break;
        case PLANET_CLASSM:
            printf("  size        300\n");
            printf("  texname     \"classm\"\n");
            break;
        case PLANET_MOON:
            printf("  size        160\n");
            printf("  texname     \"luna\"\n");
            break;
        default:
            printf("  size        300\n");
            printf("  texname     \"classd\"\n");
            break;
        }

        printf("}\n\n");
    }

    return;
}


static void startSection(int section)
{
    if (cqiDebugl)
        utLog("%s: [%d] %s", __FUNCTION__,
              curDepth + 1,
              sect2str(section));

    /* check for overflow */
    if ((curDepth + 1) >= MAX_NESTING_DEPTH)
    {
        utLog("CQI: %s: maximum nesting depth (%d) exceeded, ignoring "
              "section %s, near line %d",
              __FUNCTION__, MAX_NESTING_DEPTH, sect2str(section),
              lineNum);
        goterror++;
        /* just return here as we haven't changed curDepth yet */
        return;
    }

    /* add it to the list */
    sections[++curDepth] = section;

    switch (section)
    {
    case TOK_GLOBAL:
    {
        if (globalRead)
        {
            utLog("%s: global section already configured\n",
                  __FUNCTION__);
            goterror++;
            goto error_return;
        }

        _cqiGlobal = malloc(sizeof(cqiGlobalInitRec_t));
        if (!_cqiGlobal)
        {
            utLog("%s: Could not allocate GlobalInitRec",
                  __FUNCTION__);
            goterror++;
            goto error_return;
        }
        else
            memset((void *)_cqiGlobal, 0, sizeof(cqiGlobalInitRec_t));
    }
    break;

    case TOK_SHIPTYPE:
    {
        if (!globalRead)
        {
            utLog("%s: Have not read the global section (which must always be first). Ignoring SHIPTYPE",
                  __FUNCTION__);
            goterror++;
            goto error_return;
        }
    }
    break;
    case TOK_PLANET:
    {
        if (!globalRead)
        {
            utLog("%s: Have not read the global section (which must always be first). Ignoring PLANET",
                  __FUNCTION__);
            goterror++;
            return;
        }
        /* clear and init the planet for parsing */
        memset((void *)&currPlanet, 0, sizeof(cqiPlanetInitRec_t));

        currPlanet.primary = -1;
        currPlanet.size = 300;  /* default */
    }
    break;

    case TOK_TEXTURE:
    {
        memset((void *)&currTexture, 0, sizeof(cqiTextureInitRec_t));
        currTexAreas = NULL;
        numTexAreas = 0;
    }
    break;

    case TOK_ANIMATION:
    {
        memset((void *)&currAnimation, 0, sizeof(cqiAnimationInitRec_t));
        currAnimation.adIndex = -1;
    }
    break;

    case TOK_ANIMDEF:
    {
        if (PREVSECTION() == TOK_ANIMATION) /* an inlined animdef */
        {
            int _adndx = -1;
            char tmpname[CQI_NAMELEN];

            /* inlined animdef
             *
             * for inlined animdefs, the animation name it's a part of,
             * must have been specified already.  If not, then
             * something is wrong, declare an error and bail.
             */
            if (!currAnimation.name[0])
            {
                utLog("CQI: can't inline animdef at or near line %d: "
                      "animation's name has not been specified.",
                      lineNum);
                goterror++;
                goto error_return;
            }

            /* Now, see if an animdef name was specified in the
             * animation.  If so, then we want to derive our new
             * animdef from a previously existing one, which must
             * already have been defined.
             */
            if (currAnimation.animdef[0])
            {
                /* it's been specified, look for it and init our new
                 * animdef with it
                 */
                if ((_adndx = _cqiFindAnimDef(currAnimation.animdef)) < 0)
                {
                    /* couldn't find it, error */
                    utLog("CQI: can't inline animdef at or near line %d: "
                          "source animdef %s is not defined.",
                          lineNum, currAnimation.animdef);
                    goterror++;
                    goto error_return;
                }

                /* initialize it, the new animdef name will be overridden
                 * below
                 */
                currAnimDef = _cqiAnimDefs[_adndx];
            }
            else
            {                 /* just inlining a complete animdef, init */
                memset((void *)&currAnimDef, 0, sizeof(cqiAnimDefInitRec_t));
            }

            /* choose a unique name for this animdef.  We prefix it
             * with '.' (since '.' cannot be specified as part of a
             * name in a config file), and add the current (projected)
             * slot number to make it unique.
             */
            snprintf(tmpname,
                     CQI_NAMELEN - 1 - (strlen(".-NNNNNN")),
                     ".%s",
                     currAnimation.name);
            snprintf(currAnimDef.name, CQI_NAMELEN - 1, "%s-%06d",
                     tmpname, numAnimDefs + 1);

            /* now, reset the animation's animdef specification
             * (whether or not it had one) to point toward the new
             * animdef
             */
            strncpy(currAnimation.animdef, currAnimDef.name, CQI_NAMELEN - 1);
        }
        else
        {
            /* a global animdef */
            memset((void *)&currAnimDef, 0, sizeof(cqiAnimDefInitRec_t));
        }
    }
    break;

    case TOK_SOUNDCONF:
    {
        if (!_cqiSoundConf)
        {                     /* starting fresh */
            _cqiSoundConf = malloc(sizeof(cqiSoundConfRec_t));
            if (!_cqiSoundConf)
            {
                utLog("%s: Could not allocate SoundConf",
                      __FUNCTION__);
                goterror++;
                goto error_return;
            }
            else
            {
                memset((void *)_cqiSoundConf, 0, sizeof(cqiSoundConfRec_t));
                _cqiSoundConf->stereo = TRUE; /* default to stereo */
            }

        } /* else we are just overriding */
    }
    break;

    case TOK_EFFECT:
    case TOK_MUSIC:
    {
        memset((void *)&currSound, 0, sizeof(cqiSoundRec_t));

        currSound.loops = 1;    /* default to 1 loop */
        currSound.volume = 100;
        currSound.pan = 0;
    }
    break;

    default:
        break;
    }

    return;

    /* clean things up if there was an error */
error_return:
    /* remove the section from the list */
    sections[curDepth] = 0;
    curDepth--;
    return;
}

static void endSection(void)
{
    if (cqiDebugl)
        utLog("%s: [%d] %s", __FUNCTION__, curDepth, sect2str(CURSECTION()));

    switch (CURSECTION())
    {
    case TOK_GLOBAL:
    {                         /* make sure everything is specified, alloc
                                 new planet/shiptype arrays, reset counts
                              */

        if (!_cqiGlobal->maxplanets || !_cqiGlobal->maxships ||
            !_cqiGlobal->maxusers || !_cqiGlobal->maxhist ||
            !_cqiGlobal->maxmsgs)
        {                     /* something missing */
            utLog("CQI: GLOBAL section is incomplete, ignoring.");
            globalRead = FALSE; /* redundant I know, but.... */
        }
        else
            globalRead = TRUE;

        if (globalRead)
        {                     /* alloc new planet array */
            _cqiPlanets = malloc(sizeof(cqiPlanetInitRec_t) *
                                 _cqiGlobal->maxplanets);

            numPlanets = 0;

            if (!_cqiPlanets)
            {
                utLog("CQI: could not allocate memory for planets.");
                globalRead = FALSE; /* redundant I know, but.... */
                goterror++;
            }
            else
                memset((void *)_cqiPlanets, 0, sizeof(cqiPlanetInitRec_t) *
                       _cqiGlobal->maxplanets);
        }
    }
    break;

    case TOK_SHIPTYPE:
        break;

    case TOK_TEXANIM:
    {
        /* stage == 0 means to disable the anim type */
        if (currAnimDef.texanim.stages)
            currAnimDef.anims |= CQI_ANIMS_TEX;
        else
            currAnimDef.anims &= ~CQI_ANIMS_TEX;

    }
    break;

    case TOK_COLANIM:
    {
        /* stage == 0 means to disable the anim type */
        if (currAnimDef.colanim.stages)
            currAnimDef.anims |= CQI_ANIMS_COL;
        else
            currAnimDef.anims &= ~CQI_ANIMS_COL;
    }
    break;

    case TOK_GEOANIM:
    {
        /* stage == 0 means to disable the anim type */
        if (currAnimDef.geoanim.stages)
            currAnimDef.anims |= CQI_ANIMS_GEO;
        else
            currAnimDef.anims &= ~CQI_ANIMS_GEO;
    }
    break;

    case TOK_TOGANIM:
    {
        /* delayms on a toganim == 0 means to disable the anim type */
        if (currAnimDef.toganim.delayms)
            currAnimDef.anims |= CQI_ANIMS_TOG;
        else
            currAnimDef.anims &= ~CQI_ANIMS_TOG;
    }
    break;

    case TOK_TEXAREA:
        /* this is only valid from within a texture definition */
        if (PREVSECTION() == TOK_TEXTURE)
        {
            cqiTextureAreaPtr_t taptr;

            if (strlen(currTexArea.name))
            {

                /* resize the current list and add to it. */

                taptr = (cqiTextureAreaPtr_t)realloc((void *)currTexAreas,
                                                     sizeof(cqiTextureAreaRec_t) *
                                                     (numTexAreas + 1));

                if (!taptr)
                {
                    utLog("CQI: Could not realloc %d texareas for texture %s, "
                          "ignoring texarea '%s'",
                          numTexAreas + 1,
                          currTexture.name,
                          currTexArea.name);
                    break;
                }

                currTexAreas = taptr;
                currTexAreas[numTexAreas] = currTexArea;
                numTexAreas++;
                currTexture.texareas = currTexAreas;
                currTexture.numTexAreas = numTexAreas;

            }
            else
            {
                utLog("CQI: texarea name at or near line %d was not specified, "
                      "ignoring.",
                      lineNum);
                goto endsection;
            }
        }
        break;

    case TOK_PLANET:
    {
        /* check some basic things */
        if (!currPlanet.name[0] || !currPlanet.primname[0])
        {
            utLog("CQI: planet %d is missing name and/or primary",
                  numPlanets);
            goterror++;
            return;
        }

        if (numPlanets >= _cqiGlobal->maxplanets)
        {
            utLog("CQI: planet '%s' (%d) exceeds maxplanets (%d), ignoring.",
                  currPlanet.name, numPlanets,
                  _cqiGlobal->maxplanets);
            goto endsection;
        }

        /* need more checks here ? */

        /* add it */
        _cqiPlanets[numPlanets] = currPlanet;
        numPlanets++;
    }

    break;

    case TOK_TEXTURE:
    {
        cqiTextureInitPtr_t texptr;
        int exists = -1;

        /* verify the required info was provided */
        if (!strlen(currTexture.name))
        {
            utLog("CQI: texture name at or near line %d was not specified, "
                  "ignoring.",
                  lineNum);
            goto endsection;
        }

        /* if the texture was overridden by a later definition
           just copy the new definition over it */
        exists = _cqiFindTexture(currTexture.name);

        /* if a filename wasn't specified, and this is not a 'color only'
           texture, then copy in the texname as the default */
        if (!strlen(currTexture.filename) &&
            !(currTexture.flags & CQITEX_F_COLOR_SPEC))
            strncpy(currTexture.filename, currTexture.name, CQI_NAMELEN - 1);

        if (exists >= 0)
        {
            /* overwrite existing texture def */
            _cqiTextures[exists] = currTexture;
            if (cqiDebugl)
                utLog("CQI: texture '%s' near line %d: overriding already "
                      "loaded texture.",
                      currTexture.name, lineNum);
        }
        else
        {                     /* make a new one */
            texptr = (cqiTextureInitPtr_t)realloc((void *)_cqiTextures,
                                                  sizeof(cqiTextureInitRec_t) *
                                                  (numTextures + 1));

            if (!texptr)
            {
                utLog("CQI: Could not realloc %d textures, ignoring texture '%s'",
                      numTextures + 1,
                      currTexture.name);
                goto endsection;
            }

            _cqiTextures = texptr;
            _cqiTextures[numTextures] = currTexture;
            numTextures++;

            /* warn if the texture did not specify a color (will be
             *  black and transparent)
             */

            if (!(currTexture.flags & CQITEX_F_HAS_COLOR))
                utLog("CQI: warning, texture '%s' does not specify a color",
                      currTexture.name);

        }
        fileNumTextures++;
    }
    break;

    case TOK_ANIMATION:
    {
        cqiAnimationInitPtr_t animptr;
        int exists = -1;

        /* verify the required info was provided */
        if (!strlen(currAnimation.name))
        {
            utLog("CQI: animation name at or near line %d was not specified, "
                  "ignoring.",
                  lineNum);
            goto endsection;
        }

        /* if the animation was overridden by a later definition
           just copy the new definition over it */

        exists = _cqiFindAnimation(currAnimation.name);

        /* if a animdef wasn't specified, then copy in the
           name as the default */
        if (!strlen(currAnimation.animdef))
            strncpy(currAnimation.animdef, currAnimation.name, CQI_NAMELEN - 1);

        if (exists >= 0)
        {
            _cqiAnimations[exists] = currAnimation;
            if (cqiDebugl)
                utLog("CQI: animation '%s' near line %d: overriding already "
                      "loaded animation.",
                      currAnimation.name, lineNum);
        }
        else
        {                     /* make a new one */
            animptr = (cqiAnimationInitPtr_t)realloc((void *)_cqiAnimations,
                                                     sizeof(cqiAnimationInitRec_t) *
                                                     (numAnimations + 1));

            if (!animptr)
            {
                utLog("CQI: Could not realloc %d animations, ignoring "
                      "animation '%s'",
                      numAnimations + 1,
                      currAnimation.name);
                goto endsection;
            }

            _cqiAnimations = animptr;
            _cqiAnimations[numAnimations] = currAnimation;
            numAnimations++;
        }
        fileNumAnimations++;
    }
    break;

    case TOK_ANIMDEF:
    {
        cqiAnimDefInitPtr_t animptr;
        int exists = -1;

        /* verify the required info was provided */
        if (!strlen(currAnimDef.name))
        {
            utLog("CQI: animdef name at or near line %d was not specified, "
                  "ignoring.",
                  lineNum);
            goto endsection;
        }

        exists = _cqiFindAnimDef(currAnimDef.name);

        if (exists >= 0)
        {
            _cqiAnimDefs[exists] = currAnimDef;
            if (cqiDebugl)
                utLog("CQI: animdef '%s' near line %d: overriding already loaded "
                      "animdef.",
                      currAnimDef.name, lineNum);
        }
        else if (!currAnimDef.anims)
        {                     /* no animation types were declared */
            utLog("CQI: animdef '%s' near line %d: declared no animation "
                  "type sections. Ignoring.",
                  currAnimDef.name, lineNum);
            goto endsection;
        }
        else
        {                     /* make a new one */
            animptr = (cqiAnimDefInitPtr_t)realloc((void *)_cqiAnimDefs,
                                                   sizeof(cqiAnimDefInitRec_t) *
                                                   (numAnimDefs + 1));

            if (!animptr)
            {
                utLog("CQI: Could not realloc %d animdefs, ignoring "
                      "animdef '%s'",
                      numAnimDefs + 1,
                      currAnimDef.name);
                goto endsection;
            }

            _cqiAnimDefs = animptr;
            _cqiAnimDefs[numAnimDefs] = currAnimDef;
            numAnimDefs++;
        }
        fileNumAnimDefs++;
    }
    break;

    case TOK_SOUNDCONF:
    {                         /* make sure everything is specified */

        if (!_cqiSoundConf->samplerate)
            _cqiSoundConf->samplerate = 22050;

        /* stereo is already enabled by default in startSection() */

        if (!_cqiSoundConf->fxchannels)
            _cqiSoundConf->fxchannels = 16;

        if (!_cqiSoundConf->chunksize)
            _cqiSoundConf->chunksize = 512;
    }
    break;


    case TOK_EFFECT:
    {
        cqiSoundPtr_t sndptr;
        int exists = -1;

        /* verify the required info was provided */
        if (!strlen(currSound.name))
        {
            utLog("CQI: effect name at or near line %d was not specified, "
                  "ignoring.",
                  lineNum);
            goto endsection;
        }

        /* if the effect was overridden by a later definition
           just copy the new definition over it */
        exists = _cqiFindEffect(currSound.name);

        /* if a filename wasn't specified, then copy in the name
           as the default */
        if (!strlen(currSound.filename))
            strncpy(currSound.filename, currSound.name, CQI_NAMELEN - 1);

        if (exists >= 0)
        {
            /* overwrite existing def */
            _cqiSoundEffects[exists] = currSound;
            if (cqiDebugl)
                utLog("CQI: effect '%s' near line %d: overriding already "
                      "loaded effect.",
                      currSound.name, lineNum);
        }
        else
        {                     /* make a new one */
            sndptr = (cqiSoundPtr_t)realloc((void *)_cqiSoundEffects,
                                            sizeof(cqiSoundRec_t) *
                                            (numSoundEffects + 1));

            if (!sndptr)
            {
                utLog("CQI: Could not realloc %d effect, ignoring effect '%s'",
                      numSoundEffects + 1,
                      currSound.name);
                goto endsection;
            }

            _cqiSoundEffects = sndptr;
            _cqiSoundEffects[numSoundEffects] = currSound;
            numSoundEffects++;
        }
        fileNumEffects++;
    }
    break;

    case TOK_MUSIC:
    {
        cqiSoundPtr_t sndptr;
        int exists = -1;

        /* verify the required info was provided */
        if (!strlen(currSound.name))
        {
            utLog("CQI: music name at or near line %d was not specified, "
                  "ignoring.",
                  lineNum);
            goto endsection;
        }

        /* if the music was overridden by a later definition
           just copy the new definition over it */
        exists = _cqiFindMusic(currSound.name);

        /* if a filename wasn't specified, then copy in the name
           as the default */
        if (!strlen(currSound.filename))
            strncpy(currSound.filename, currSound.name, CQI_NAMELEN - 1);

        if (exists >= 0)
        {
            /* overwrite existing def */
            _cqiSoundMusic[exists] = currSound;
            if (cqiDebugl)
                utLog("CQI: music '%s' near line %d: overriding already "
                      "loaded music slot.",
                      currSound.name, lineNum);
        }
        else
        {                     /* make a new one */
            sndptr = (cqiSoundPtr_t)realloc((void *)_cqiSoundMusic,
                                            sizeof(cqiSoundRec_t) *
                                            (numSoundMusic + 1));

            if (!sndptr)
            {
                utLog("CQI: Could not realloc %d music slots, "
                      "ignoring music '%s'",
                      numSoundMusic + 1,
                      currSound.name);
                goto endsection;
            }

            _cqiSoundMusic = sndptr;
            _cqiSoundMusic[numSoundMusic] = currSound;
            numSoundMusic++;
        }
        fileNumMusic++;
    }
    break;

    default:
        break;
    }

endsection:

    sections[curDepth] = 0;
    if (curDepth > 0)
        curDepth--;

    return;
}

/* integers */
static void cfgSectioni(int item, int val)
{
    if (cqiDebugl)
        utLog(" [%d] section = %s\titem = %s\tvali = %d",
              curDepth, sect2str(CURSECTION()), item2str(item), val);

    switch (CURSECTION())
    {
    case TOK_GLOBAL:
    {
        switch (item)
        {
        case TOK_PLANETMAX:
            _cqiGlobal->maxplanets = abs(val);
            break;
        case TOK_SHIPMAX:
            _cqiGlobal->maxships = abs(val);
            break;
        case TOK_USERMAX:
            _cqiGlobal->maxusers = abs(val);
            break;
        case TOK_HISTMAX:
            _cqiGlobal->maxhist = abs(val);
            break;
        case TOK_MSGMAX:
            _cqiGlobal->maxmsgs = abs(val);
            break;
        }
    }

    break;
    case TOK_SHIPTYPE:
        break;
    case TOK_PLANET:
    {
        switch(item)
        {
        case TOK_ARMIES:
            currPlanet.armies = abs(val);
            break;
        case TOK_SIZE:
            currPlanet.size = fabs((real)val);
            break;
        }
    }
    break;
    case TOK_ANIMDEF:
    {
        currAnimDef.timelimit = abs(val);
    }
    break;

    case TOK_SOUNDCONF:
    {
        switch (item)
        {
        case TOK_SAMPLERATE:
            _cqiSoundConf->samplerate = CLAMP(8192, 44100, abs(val));
            break;
        case TOK_FXCHANNELS:
            _cqiSoundConf->fxchannels = CLAMP(2, 64, abs(val));
            break;
        case TOK_CHUNKSIZE:
            _cqiSoundConf->chunksize = CLAMP(256, 8192, abs(val));
            break;
        default:
            break;
        }
    }

    case TOK_EFFECT:
    case TOK_MUSIC:
    {
        switch(item)
        {
        case TOK_VOLUME:
            currSound.volume = CLAMP(0, 100, abs(val));
            break;
        case TOK_PAN:
            currSound.pan = CLAMP(-128, 128, abs(val));
            break;
        case TOK_FADEINMS:
            currSound.fadeinms = CLAMP(0, 10000, abs(val));
            break;
        case TOK_FADEOUTMS:
            currSound.fadeoutms = CLAMP(0, 10000, abs(val));
            break;
        case TOK_LOOPS:
            currSound.loops = abs(val);
            break;
        case TOK_LIMIT:
            currSound.limit = abs(val);
            break;
        case TOK_FRAMELIMIT:
            currSound.framelimit = abs(val);
            break;
        case TOK_DELAYMS:
            currSound.delayms = abs(val);
            break;
        }

    }

    case TOK_TEXANIM:
    {
        switch(item)
        {
        case TOK_STAGES:
            currAnimDef.texanim.stages = abs(val);
            break;
        case TOK_LOOPS:
            currAnimDef.texanim.loops = abs(val);
            break;
        case TOK_DELAYMS:
            currAnimDef.texanim.delayms = abs(val);
            break;
        case TOK_LOOPTYPE:
            currAnimDef.texanim.looptype = abs(val);
            break;
        }
    }
    break;

    case TOK_COLANIM:
    {
        switch(item)
        {
        case TOK_STAGES:
            currAnimDef.colanim.stages = abs(val);
            break;
        case TOK_LOOPS:
            currAnimDef.colanim.loops = abs(val);
            break;
        case TOK_DELAYMS:
            currAnimDef.colanim.delayms = abs(val);
            break;
        case TOK_LOOPTYPE:
            currAnimDef.colanim.looptype = abs(val);
            break;
        }
    }
    break;

    case TOK_GEOANIM:
    {
        switch(item)
        {
        case TOK_STAGES:
            currAnimDef.geoanim.stages = abs(val);
            break;
        case TOK_LOOPS:
            currAnimDef.geoanim.loops = abs(val);
            break;
        case TOK_DELAYMS:
            currAnimDef.geoanim.delayms = abs(val);
            break;
        case TOK_LOOPTYPE:
            currAnimDef.geoanim.looptype = abs(val);
            break;
        }
    }
    break;

    case TOK_TOGANIM:
    {
        switch(item)
        {
        case TOK_DELAYMS:
            currAnimDef.toganim.delayms = abs(val);
            break;
        }
    }
    break;

    case TOK_ISTATE:
    {
        switch(item)
        {
        case TOK_SIZE:
            currAnimDef.isize = fabs((real)val);
            currAnimDef.istates |= AD_ISTATE_SZ;
            break;
        }
    }
    break;

    default:
        break;
    }

    return;
}

/* integer pair */
void cfgSectionil(int item, int val1, int val2)
{
    if (cqiDebugl)
        utLog(" [%d] section = %s\titem = %s\tvalil = %d, %d",
              curDepth, sect2str(CURSECTION()), item2str(item), val1, val2);

    switch (CURSECTION())
    {
    case TOK_PLANET:
    {
        switch (item)
        {
        case TOK_ARMIES:
        {
            /* if we got a pair, randomly init one */
            /* make sure it's valid of course... */
            if (val1 >= val2 || val2 <= val1)
            {
                utLog("%s: Planet '%s's army min must be less than it's max: min %d max %d is invalid.",
                      __FUNCTION__,
                      currPlanet.name,
                      val1, val2);
                currPlanet.armies = 0;
            }
            else
                currPlanet.armies = rndint(abs(val1), abs(val2));

#if 0
            utLog("ARMIES got %d %d, rnd = %d\n",
                  val1, val2, currPlanet.armies);
#endif
        }
        break;
        }
    }
    }

    return;
}


/* reals */
static void cfgSectionf(int item, real val)
{
    if (cqiDebugl)
        utLog(" [%d] section = %s\titem = %s\tvalf = %f",
              curDepth, sect2str(CURSECTION()), item2str(item), val);

    switch (CURSECTION())
    {
    case TOK_GLOBAL:
        break;
    case TOK_SHIPTYPE:
        break;
    case TOK_PLANET:
    {
        switch (item)
        {
        case TOK_ANGLE:
            currPlanet.angle = val;
            break;
        case TOK_VELOCITY:
            currPlanet.velocity = val;
            break;
        case TOK_RADIUS:
            currPlanet.radius = val;
            break;
        case TOK_XCOORD:
            currPlanet.xcoord = val;
            break;
        case TOK_YCOORD:
            currPlanet.ycoord = val;
            break;
        }
    }

    break;

    case TOK_TEXANIM:
    {
        switch(item)
        {
        case TOK_DELTAS:
            currAnimDef.texanim.deltas = val;
            break;
        case TOK_DELTAT:
            currAnimDef.texanim.deltat = val;
            break;
        }
    }
    break;
    case TOK_COLANIM:
    {
        switch(item)
        {
        case TOK_DELTAA:
            currAnimDef.colanim.deltaa = val;
            break;
        case TOK_DELTAR: /* red */
            currAnimDef.colanim.deltar = val;
            break;
        case TOK_DELTAG:
            currAnimDef.colanim.deltag = val;
            break;
        case TOK_DELTAB:
            currAnimDef.colanim.deltab = val;
            break;
        }
    }
    break;

    case TOK_GEOANIM:
    {
        switch(item)
        {
        case TOK_DELTAX:
            currAnimDef.geoanim.deltax = val;
            break;
        case TOK_DELTAY:
            currAnimDef.geoanim.deltay = val;
            break;
        case TOK_DELTAZ:
            currAnimDef.geoanim.deltaz = val;
            break;
        case TOK_DELTAR: /* rotation */
            currAnimDef.geoanim.deltar = val;
            break;
        case TOK_DELTAS: /* size */
            currAnimDef.geoanim.deltas = val;
            break;
        }
    }
    break;

    case TOK_ISTATE:
    {
        switch(item)
        {
        case TOK_ANGLE:
            currAnimDef.iangle = val;
            currAnimDef.istates |= AD_ISTATE_ANG;
            break;
        }
    }
    break;

    case TOK_TEXAREA:
    {
        switch(item)
        {
        case TOK_XCOORD:
            currTexArea.x = fabs(val);
            break;
        case TOK_YCOORD:
            currTexArea.y = fabs(val);
            break;
        case TOK_WIDTH:
            currTexArea.w = fabs(val);
            break;
        case TOK_HEIGHT:
            currTexArea.h = fabs(val);
            break;
        }
    }
    break;

    default:
        break;
    }
    return;
}

/* strings */
void cfgSections(int item, char *val)
{
    if (cqiDebugl)
        utLog(" [%d] section = %s\titem = %s\tvals = '%s'",
              curDepth, sect2str(CURSECTION()),
              item2str(item), (val) ? val : "(NULL)" );

    if (!val)
        return;

    switch (CURSECTION())
    {
    case TOK_GLOBAL:
        break;
    case TOK_SHIPTYPE:
        break;
    case TOK_TEXAREA:
    {
        switch (item)
        {
        case TOK_NAME:
            strncpy(currTexArea.name, val, CQI_NAMELEN - 1);
            break;
        }
    }
    break;
    case TOK_TEXANIM:
    {
        switch(item)
        {
        case TOK_COLOR:
            currAnimDef.texanim.color = hex2color(val);
            break;
        }
    }
    break;

    case TOK_COLANIM:
    {
        switch(item)
        {
        case TOK_COLOR:
            currAnimDef.colanim.color = hex2color(val);
            break;
        }
    }
    break;

    case TOK_ISTATE:
    {
        switch(item)
        {
        case TOK_COLOR:
            currAnimDef.icolor = hex2color(val);
            currAnimDef.istates |= AD_ISTATE_COL;
            break;
        case TOK_TEXNAME:
            strncpy(currAnimDef.itexname, val, CQI_NAMELEN - 1);
            currAnimDef.istates |= AD_ISTATE_TEX;
            break;
        }
    }
    break;

    case TOK_PLANET:
    {
        switch (item)
        {
        case TOK_NAME:
            strncpy(currPlanet.name, val, MAXPLANETNAME - 1);
            break;
        case TOK_PRIMARY:
            strncpy(currPlanet.primname, val, MAXPLANETNAME - 1);
            break;
        case TOK_PTYPE:
            currPlanet.ptype = str2ptype(val);
            break;
        case TOK_PTEAM:
            currPlanet.pteam = str2team(val);
            break;
        case TOK_TEXNAME:
            strncpy(currPlanet.texname, val, CQI_NAMELEN - 1);
            break;
        case TOK_COLOR:
            /* this used to be allowed, so for now just warn if
             * verbose is on
             */
            if (cqDebug)
                utLog("CQI: 'color' is no longer valid in planet definitions");
            break;
        }
    }
    break;

    case TOK_TEXTURE:
    {
        switch(item)
        {
        case TOK_NAME:
            strncpy(currTexture.name, val, CQI_NAMELEN - 1);
            break;
        case TOK_FILENAME:
            if (val[0] == 0)    /* empty filename means only color matters */
                currTexture.flags |= CQITEX_F_COLOR_SPEC;
            else
                strncpy(currTexture.filename, val, CQI_NAMELEN - 1);
            break;
        case TOK_COLOR:
            currTexture.flags |= CQITEX_F_HAS_COLOR;
            currTexture.color = hex2color(val);
            break;
        }
    }
    break;

    case TOK_ANIMATION:
    {
        switch(item)
        {
        case TOK_NAME:
            strncpy(currAnimation.name, val, CQI_NAMELEN - 1);
            break;
        case TOK_ANIMDEF:
            strncpy(currAnimation.animdef, val, CQI_NAMELEN - 1);
            break;
        }
    }
    break;

    case TOK_ANIMDEF:
    {
        switch(item)
        {
        case TOK_NAME:
            if (PREVSECTION() == TOK_ANIMATION)
            {
                /* not allowed to set a name on inlined animdefs - it's
                 * already been done for you.
                 */
                utLog("CQI: field 'name' is ignored for inlined animdefs.");
            }
            else
                strncpy(currAnimDef.name, val, CQI_NAMELEN - 1);
            break;
        case TOK_TEXNAME:
            strncpy(currAnimDef.texname, val, CQI_NAMELEN - 1);
            break;
        }
    }
    break;

    case TOK_EFFECT:
    case TOK_MUSIC:
    {
        switch(item)
        {
        case TOK_NAME:
            strncpy(currSound.name, val, CQI_NAMELEN - 1);
            break;
        case TOK_FILENAME:
            strncpy(currSound.filename, val, CQI_NAMELEN - 1);
        }
    }
    break;

    }

    /* be sure to free the value allocated */
    free(val);
    return;
}

/* booleans */
static void cfgSectionb(int item, char *val)
{
    int bval = parsebool(val);

    if (cqiDebugl)
        utLog(" [%d] section = %s\titem = %s\tvalb = '%s'",
              curDepth, sect2str(CURSECTION()), item2str(item),
              (bval ? "yes" : "no"));

    if (bval == -1)
    {                           /* an error, do something sane */
        bval = 0;
    }

    switch (CURSECTION())
    {
    case TOK_GLOBAL:
        break;
    case TOK_SHIPTYPE:
        break;
    case TOK_PLANET:
    {
        switch(item)
        {
        case TOK_VISIBLE:
            currPlanet.visible = bval;
            break;
        case TOK_CORE:
            currPlanet.core = bval;
            break;
        case TOK_HOMEPLANET:
            currPlanet.homeplanet = bval;
            break;
        }
    }
    break;

    case TOK_TEXTURE:
    {
        switch(item)
        {
        case TOK_MIPMAP:
            if (bval)
                currTexture.flags |= CQITEX_F_GEN_MIPMAPS;
            else
                currTexture.flags &= ~CQITEX_F_GEN_MIPMAPS;

            break;

        case TOK_TEX_LUMINANCE:
            if (bval)
                currTexture.flags |= CQITEX_F_IS_LUMINANCE;
            else
                currTexture.flags &= ~CQITEX_F_IS_LUMINANCE;

            break;
        }
    }
    break;

    case TOK_SOUNDCONF:
    {
        switch (item)
        {
        case TOK_STEREO:
            _cqiSoundConf->stereo = bval;
            break;
        }
    }
    break;

    default:
        break;
    }

    return;
}

static char *sect2str(int section)
{
    switch (section)
    {
    case TOK_GLOBAL:
        return "GLOBAL";
        break;
    case TOK_SHIPTYPE:
        return "SHIPTYPE";
        break;
    case TOK_PLANET:
        return "PLANET";
        break;
    case TOK_TEXTURE:
        return "TEXTURE";
        break;
    case TOK_ANIMATION:
        return "ANIMATION";
        break;
    case TOK_ANIMDEF:
        return "ANIMDEF";
        break;
    case TOK_SOUNDCONF:
        return "SOUNDCONF";
        break;
    case TOK_EFFECT:
        return "EFFECT";
        break;
    case TOK_MUSIC:
        return "MUSIC";
        break;
    case TOK_LIMIT:
        return "LIMIT";
        break;
    case TOK_FRAMELIMIT:
        return "FRAMELIMIT";
        break;

        /* nested sections */
    case TOK_TEXANIM:
        return "TEXANIM";
        break;
    case TOK_COLANIM:
        return "COLANIM";
        break;
    case TOK_GEOANIM:
        return "GEOANIM";
        break;
    case TOK_TOGANIM:
        return "TOGANIM";
        break;
    case TOK_ISTATE:
        return "ISTATE";
        break;
    case TOK_TEXAREA:
        return "TEXAREA";
        break;
    }

    return "UNKNOWN";
}

static char *item2str(int item)
{
    switch (item)
    {
    case TOK_PLANETMAX:
        return "PLANETMAX";
        break;
    case TOK_SHIPMAX:
        return "SHIPMAX";
        break;
    case TOK_USERMAX:
        return "USERMAX";
        break;
    case TOK_HISTMAX:
        return "HISTMAX";
        break;
    case TOK_MSGMAX:
        return "MSGMAX";
        break;
    case TOK_NAME:
        return "NAME";
        break;
    case TOK_ENGFAC:
        return "ENGFAC";
        break;
    case TOK_WEAFAC:
        return "WEAFAC";
        break;
    case TOK_ACCELFAC:
        return "ACCELFAC";
        break;
    case TOK_TORPWARP:
        return "TORPWARP";
        break;
    case TOK_WARPMAX:
        return "WARPMAX";
        break;
    case TOK_ARMYMAX:
        return "ARMYMAX";
        break;
    case TOK_SHMAX:
        return "SHMAX";
        break;
    case TOK_DAMMAX:
        return "DAMMAX";
        break;
    case TOK_TORPMAX:
        return "TORPMAX";
        break;
    case TOK_SIZE:
        return "SIZE";
        break;
    case TOK_FUELMAX:
        return "FUELMAX";
        break;
    case TOK_PRIMARY:
        return "PRIMARY";
        break;
    case TOK_ANGLE:
        return "ANGLE";
        break;
    case TOK_VELOCITY:
        return "VELOCITY";
        break;
    case TOK_RADIUS:
        return "RADIUS";
        break;
    case TOK_PTYPE:
        return "PTYPE";
        break;
    case TOK_PTEAM:
        return "PTEAM";
        break;
    case TOK_ARMIES:
        return "ARMIES";
        break;
    case TOK_VISIBLE:
        return "VISIBLE";
        break;
    case TOK_CORE:
        return "CORE";
        break;
    case TOK_XCOORD:
        return "XCOORD";
        break;
    case TOK_YCOORD:
        return "YCOORD";
        break;
    case TOK_SCOORD:
        return "SCOORD";
        break;
    case TOK_TCOORD:
        return "TCOORD";
        break;
    case TOK_TEXNAME:
        return "TEXNAME";
        break;
    case TOK_COLOR:
        return "COLOR";
        break;
    case TOK_HOMEPLANET:
        return "HOMEPLANET";
        break;
    case TOK_FILENAME:
        return "FILENAME";
        break;
    case TOK_ANIMDEF:
        return "ANIMDEF";
        break;
    case TOK_TIMELIMIT:
        return "TIMELIMIT";
        break;
    case TOK_STAGES:
        return "STAGES";
        break;
    case TOK_LOOPS:
        return "LOOPS";
        break;
    case TOK_DELAYMS:
        return "DELAYMS";
        break;
    case TOK_LOOPTYPE:
        return "LOOPTYPE";
        break;
    case TOK_DELTAA:
        return "DELTAA";
        break;
    case TOK_DELTAR:
        return "DELTAR";
        break;
    case TOK_DELTAG:
        return "DELTAG";
        break;
    case TOK_DELTAB:
        return "DELTAB";
        break;
    case TOK_DELTAX:
        return "DELTAX";
        break;
    case TOK_DELTAY:
        return "DELTAY";
        break;
    case TOK_DELTAZ:
        return "DELTAZ";
        break;
    case TOK_DELTAS:
        return "DELTAS";
        break;
    case TOK_DELTAT:
        return "DELTAT";
        break;
    case TOK_WIDTH:
        return "WIDTH";
        break;
    case TOK_HEIGHT:
        return "HEIGHT";
        break;
    case TOK_MIPMAP:
        return "MIPMAP";
        break;

    case TOK_SAMPLERATE:
        return "SAMPLERATE";
        break;
    case TOK_VOLUME:
        return "VOLUME";
        break;
    case TOK_PAN:
        return "PAN";
        break;
    case TOK_STEREO:
        return "STEREO";
        break;
    case TOK_FXCHANNELS:
        return "FXCHANNELS";
        break;
    case TOK_CHUNKSIZE:
        return "CHUNKSIZE";
        break;
    case TOK_FADEINMS:
        return "FADEINMS";
        break;
    case TOK_FADEOUTMS:
        return "FADEOUTMS";
        break;
    case TOK_LIMIT:
        return "LIMIT";
        break;
    }

    return "UNKNOWN";
}

/* go through the string and convert all '.', '/', '\', and
 * non-printable characters into '_'.  Remove any double quotes.
 *
 */
static void checkStr(char *str)
{
    char *s = str, *s2;

    if (!s)
        return;

    while (*s)
    {
        switch (*s)
        {
        case '\\':
        case '/':
        case '.':
        case ' ':
            *s = '_';
            s++;
            break;

        case '"':               /* copy over it */
        {
            s2 = s + 1;
            do
            {
                *(s2 - 1) = *s2;
                s2++;
            } while (*(s2 - 1));
        }
        /* do not increment s here */
        break;

        default:
        {
            if (!isprint((unsigned int)*s))
                *s = '_';
        }
        s++;
        break;
        }

    }

    return;
}

static int parsebool(char *str)
{
    char *s;

    s = str;

    if (!s)
        return(-1);

    while(*s)
    {
        *s = (char)tolower(*s);
        s++;
    }

    if (((char *)strstr("false", str) != NULL) ||
        ((char *)strstr("no", str) != NULL)    ||
        ((char *)strstr("off", str) != NULL))
    {
        return(FALSE);
    }
    else if (((char *)strstr("true", str) != NULL) ||
             ((char *)strstr("yes", str) != NULL)  ||
             ((char *)strstr("on", str) != NULL))
    {
        return(TRUE);
    }
    else
    {
        utLog("parsebool(): error parsing '%s' line %d, \n\t%s\n",
              str, lineNum,
              "Boolean value must be 'yes', 'no', 'true', 'false', 'on', or 'off'.");
        return(-1);
    }
}


/* initrun - initalize for the run */
static void initrun(int rcid)
{

    switch (rcid)
    {
    case CQI_FILE_CONQINITRC:
    {
        if (_cqiGlobal)
        {
            if (_cqiGlobal != &defaultGlobalInit)
                free(_cqiGlobal);
            _cqiGlobal = NULL;
            globalRead = FALSE;
        }

        if (_cqiShiptypes)
        {
            if (_cqiShiptypes != defaultShiptypes)
                free(_cqiShiptypes);
            _cqiShiptypes = NULL;
            numShiptypes = 0;
        }

        if (_cqiPlanets)
        {
            if (_cqiPlanets != defaultPlanets)
                free(_cqiPlanets);
            _cqiPlanets = NULL;
            numPlanets = 0;
        }
    }

    break;

    case CQI_FILE_SOUNDRC:
    case CQI_FILE_SOUNDRC_ADD:
    {
        if (rcid == CQI_FILE_SOUNDRC)
        {                       /* if we are not adding, re-init */
            if (_cqiSoundConf)
            {
                if (_cqiSoundConf != &defaultSoundConf)
                    free(_cqiSoundConf);
                _cqiSoundConf = NULL;
            }

            if (_cqiSoundEffects)
            {
                if (_cqiSoundEffects != defaultSoundEffects)
                    free(_cqiSoundEffects);
                _cqiSoundEffects = NULL;
            }
            numSoundEffects = 0;

            if (_cqiSoundMusic)
            {
                if (_cqiSoundMusic != defaultSoundMusic)
                    free(_cqiSoundMusic);
                _cqiSoundMusic = NULL;
            }
            numSoundMusic = 0;
        }

        fileNumEffects = 0;
        fileNumMusic = 0;

    }
    break;

    case CQI_FILE_TEXTURESRC:
    case CQI_FILE_TEXTURESRC_ADD:
    {      /* free/setup textures here */
        if (rcid == CQI_FILE_TEXTURESRC)
        {                       /* if we are not adding, re-init */
            if (_cqiTextures)
            {
                free(_cqiTextures);
                _cqiTextures = NULL;
            }
            numTextures = 0;

            if (_cqiAnimations)
            {
                free(_cqiAnimations);
                _cqiAnimations = NULL;
            }
            numAnimations = 0;

            if (_cqiAnimDefs)
            {
                free(_cqiAnimDefs);
                _cqiAnimDefs = NULL;
            }
            numAnimDefs = 0;
        }

        fileNumTextures = 0;
        fileNumAnimations = 0;
        fileNumAnimDefs = 0;
    }
    break;

    default:
        utLog("CQI: initrun: unkown rcid %d", rcid);
        break;
    }

    return;

}
