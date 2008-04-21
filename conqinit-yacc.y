%{
/*
 * conqinit - yacc parser for conqinit
 *
 * $Id$
 *
 */

#include "c_defs.h"

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
  
#include "global.h"
#include "color.h"

#define NOEXTERN_CONQINIT
#include "conqinit.h"
#undef NOEXTERN_CONQINIT

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
 
static int cursection = 0; /* current section */
static int cursubsection = 0; /* current subsection */

extern int lineNum;
extern int goterror;
extern void yyerror(char *s);

int yylex(void);

static cqiGlobalInitPtr_t      _cqiGlobal;
static cqiShiptypeInitPtr_t    _cqiShiptypes;
static cqiPlanetInitPtr_t      _cqiPlanets;
static cqiTextureInitPtr_t     _cqiTextures;
static cqiAnimationInitPtr_t   _cqiAnimations;
static cqiAnimDefInitPtr_t     _cqiAnimDefs;
 
static cqiSoundConfPtr_t       _cqiSoundConf;
static cqiSoundPtr_t           _cqiSoundEffects;
static cqiSoundPtr_t           _cqiSoundMusic;

static int globalRead   = FALSE;
static int numShiptypes = 0;
static int numPlanets   = 0;
static int numTextures  = 0;
static int numAnimations= 0;
static int numAnimDefs  = 0;
static int numSoundEffects   = 0;
static int numSoundMusic     = 0;

static int fileNumTextures = 0; /* # of textures loaded per file */
static int fileNumAnimations = 0;
static int fileNumAnimDefs = 0;
static int fileNumEffects = 0;
static int fileNumMusic = 0;

static cqiPlanetInitRec_t  currPlanet;
static cqiTextureInitRec_t currTexture;

static cqiAnimationInitRec_t   currAnimation;
static cqiAnimDefInitRec_t     currAnimDef;

static cqiTextureAreaRec_t     currTexArea;
static int numTexAreas = 0;
static cqiTextureAreaPtr_t     currTexAreas = NULL;

static cqiSoundRec_t currSound;

static void startSection(int section);
static void startSubSection(int subsection);
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
static void rmDQuote(char *str);
static int parsebool(char *str);


%}

%union
{
  int num;
  char *ptr;
  real rnum;
};

%token <num> OPENSECT CLOSESECT NUMBER 
%token <num> GLOBAL PLANETMAX SHIPMAX USERMAX MSGMAX HISTMAX
%token <num> SHIPTYPE ENGFAC WEAFAC ACCELFAC TORPWARP WARPMAX
%token <num> ARMYMAX SHMAX DAMMAX TORPMAX FUELMAX NAME SIZE
%token <num> PLANET PRIMARY ANGLE VELOCITY RADIUS PTYPE PTEAM
%token <num> ARMIES VISIBLE CORE XCOORD YCOORD TEXNAME COLOR
%token <num> HOMEPLANET TEXTURE FILENAME
%token <num> ANIMATION ANIMDEF 
%token <num> STAGES LOOPS DELAYMS LOOPTYPE TIMELIMIT
%token <num> TEXANIM COLANIM GEOANIM TOGANIM ISTATE
%token <num> DELTAA DELTAR DELTAG DELTAB DELTAX DELTAY DELTAZ DELTAS 

%token <num> SOUNDCONF SAMPLERATE VOLUME PAN STEREO FXCHANNELS CHUNKSIZE
%token <num> EFFECT FADEINMS FADEOUTMS LIMIT FRAMELIMIT
%token <num> MUSIC
%token <num> DELTAT SCOORD TCOORD WIDTH HEIGHT TEXAREA

%token <ptr>  STRING 
%token <rnum> RATIONAL

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
                   startSection(GLOBAL);
                }
                ;

globalword      : GLOBAL
                {;}
                ;

shiptypeconfig  : startshiptype stmts closesect
                ;

startshiptype   : shiptypeword opensect
                {
                   startSection(SHIPTYPE);
                }
                ;

shiptypeword    : SHIPTYPE
                {;}
                ;

planetconfig    : startplanet stmts closesect
                ;

startplanet     : planetword opensect
                {
                   startSection(PLANET);
                }
                ;

planetword      : PLANET
                {;}
                ;                

textureconfig   : starttexture stmts closesect
                ;

starttexture    : textureword opensect
                {
                   startSection(TEXTURE);
                }
                ;

textureword     : TEXTURE
                {;}
                ;                

animationconfig : startanimation stmts closesect
                ;

startanimation  : animationword opensect
                {
                   startSection(ANIMATION);
                }
                ;

animationword   : ANIMATION
                {;}
                ;

animdefconfig   : startanimdef stmts closesect
                ;


startanimdef    : animdefword opensect
                {
                    startSection(ANIMDEF);
                }
                ;

animdefword     : ANIMDEF
                {;}
                ;

texanimconfig   : starttexanim stmts closesect
                ;

starttexanim    : texanimword opensect
                {
                    startSubSection(TEXANIM);
                }
                ;

texanimword     : TEXANIM
                {;}
                ;

colanimconfig   : startcolanim stmts closesect
                ;

startcolanim    : colanimword opensect
                {
                    startSubSection(COLANIM);
                }
                ;

colanimword     : COLANIM
                {;}
                ;

geoanimconfig   : startgeoanim stmts closesect
                ;

startgeoanim    : geoanimword opensect
                {
                    startSubSection(GEOANIM);
                }
                ;

geoanimword     : GEOANIM
                {;}
                ;

toganimconfig   : starttoganim stmts closesect
                ;

starttoganim    : toganimword opensect
                {
                    startSubSection(TOGANIM);
                }
                ;

toganimword     : TOGANIM
                {;}
                ;

istateconfig    : startistate stmts closesect
                ;

startistate     : istateword opensect
                {
                    startSubSection(ISTATE);
                }
                ;

istateword     : ISTATE
                {;}
                ;


texareaconfig  : starttexarea stmts closesect
                ;

starttexarea   : texareaword opensect
                {
                    startSubSection(TEXAREA);
                }
                ;

texareaword     : TEXAREA
                {;}
                ;

soundconfconfig    : startsoundconf stmts closesect
                   ;

startsoundconf     : soundconfword opensect
                   {
                      startSection(SOUNDCONF);
                   }
                   ;

soundconfword      : SOUNDCONF
                   {;}
                   ;

effectconfig   : starteffect stmts closesect
               ;

starteffect    : effectword opensect
               {
                  startSection(EFFECT);
               }
               ;

effectword     : EFFECT
               {;}
               ;


musicconfig    : startmusic stmts closesect
               ;

startmusic     : musicword opensect
               {
                  startSection(MUSIC);
               }
               ;

musicword      : MUSIC
               {;}
               ;


opensect        : OPENSECT
                {;}
                ;

closesect       : CLOSESECT
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
                ;

stmt            : /* error */
                | PLANETMAX number
                   {
                        cfgSectioni(PLANETMAX, $2);
                   }                      
                | SHIPMAX number
                   {
                        cfgSectioni(SHIPMAX, $2);
                   }                      
                | USERMAX number
                   {
                        cfgSectioni(USERMAX, $2);
                   }                      
                | HISTMAX number
                   {
                        cfgSectioni(HISTMAX, $2);
                   }                      
                | MSGMAX number
                   {
                        cfgSectioni(MSGMAX, $2);
                   }                      
                | NAME string
                   {
                        cfgSections(NAME, $2);
                   }                      
                | ENGFAC rational
                   {
                        cfgSectionf(ENGFAC, $2);
                   }                      
                | WEAFAC rational
                   {
                        cfgSectionf(WEAFAC, $2);
                   }                      
                | ACCELFAC rational
                   {
                        cfgSectionf(ACCELFAC, $2);
                   }                      
                | TORPWARP number
                   {
                        cfgSectioni(TORPWARP, $2);
                   }                      
                | WARPMAX number
                   {
                        cfgSectioni(WARPMAX, $2);
                   }                      
                | ARMYMAX number
                   {
                        cfgSectioni(ARMYMAX, $2);
                   }                      
                | SHMAX number
                   {
                        cfgSectioni(SHMAX, $2);
                   }                      
                | DAMMAX number
                   {
                        cfgSectioni(DAMMAX, $2);
                   }                      
                | TORPMAX number
                   {
                        cfgSectioni(TORPMAX, $2);
                   }                      
                | FUELMAX number
                   {
                        cfgSectioni(FUELMAX, $2);
                   }                      
                | SIZE number
                   {
                        cfgSectioni(SIZE, $2);
                   }                      
                | HOMEPLANET string
                   {
                        cfgSectionb(HOMEPLANET, $2);
                   }                      
                | PRIMARY string
                   {
                        cfgSections(PRIMARY, $2);
                   }                      
                | ANGLE rational
                   {
                        cfgSectionf(ANGLE, $2);
                   }                      
                | VELOCITY rational
                   {
                        cfgSectionf(VELOCITY, $2);
                   }                      
                | RADIUS rational
                   {
                        cfgSectionf(RADIUS, $2);
                   }                      
                | PTYPE string
                   {
                        cfgSections(PTYPE, $2);
                   }                      
                | PTEAM string
                   {
                        cfgSections(PTEAM, $2);
                   }                      
                | ARMIES number number
                   {
                        cfgSectionil(ARMIES, $2, $3);
                   }                      
                | ARMIES number
                   {
                        cfgSectioni(ARMIES, $2);
                   }                      
                | VISIBLE string
                   {
                        cfgSectionb(VISIBLE, $2);
                   }                      
                | CORE string
                   {
                        cfgSectionb(CORE, $2);
                   }                      
                | XCOORD rational
                   {
                        cfgSectionf(XCOORD, $2);
                   }                      
                | YCOORD rational
                   {
                        cfgSectionf(YCOORD, $2);
                   }                      
                | TEXNAME string
                   {
                        cfgSections(TEXNAME, $2);
                   }                      
                | COLOR string
                   {
                        cfgSections(COLOR, $2);
                   }                      
                | FILENAME string
                   {
                        cfgSections(FILENAME, $2);
                   }                      
                | ANIMDEF string
                   {            /* in this form, it's a statement
                                   rather than a section */
                        cfgSections(ANIMDEF, $2);
                   }                      
                | STAGES number
                   {
                        cfgSectioni(STAGES, $2);
                   }                      
                | LOOPS number
                   {
                        cfgSectioni(LOOPS, $2);
                   }                      
                | DELAYMS number
                   {
                        cfgSectioni(DELAYMS, $2);
                   }                      
                | LOOPTYPE number
                   {
                        cfgSectioni(LOOPTYPE, $2);
                   }                      
                | DELTAA rational
                   {
                        cfgSectionf(DELTAA, $2);
                   }                      
                | DELTAR rational
                   {
                        cfgSectionf(DELTAR, $2);
                   }                      
                | DELTAG rational
                   {
                        cfgSectionf(DELTAG, $2);
                   }                      
                | DELTAB rational
                   {
                        cfgSectionf(DELTAB, $2);
                   }                      
                | DELTAX rational
                   {
                        cfgSectionf(DELTAX, $2);
                   }                      
                | DELTAY rational
                   {
                        cfgSectionf(DELTAY, $2);
                   }                      
                | DELTAZ rational
                   {
                        cfgSectionf(DELTAZ, $2);
                   }                      
                | DELTAS rational
                   {
                        cfgSectionf(DELTAS, $2);
                   }                      
                | TIMELIMIT number
                   {
                        cfgSectioni(TIMELIMIT, $2);
                   }                      
                | SAMPLERATE number
                   {
                        cfgSectioni(SAMPLERATE, $2);
                   }                      
                | VOLUME number
                   {
                        cfgSectioni(VOLUME, $2);
                   }                      
                | PAN number
                   {
                        cfgSectioni(PAN, $2);
                   }                      
                | STEREO string
                   {
                        cfgSectionb(STEREO, $2);
                   }                      
                | FXCHANNELS number
                   {
                        cfgSectioni(FXCHANNELS, $2);
                   }                      
                | CHUNKSIZE number
                   {
                        cfgSectioni(CHUNKSIZE, $2);
                   }                      
                | FADEINMS number
                   {
                        cfgSectioni(FADEINMS, $2);
                   }                      
                | FADEOUTMS number
                   {
                        cfgSectioni(FADEOUTMS, $2);
                   }                      
                | FRAMELIMIT number
                   {
                        cfgSectioni(FRAMELIMIT, $2);
                   }                      
                | LIMIT number
                   {
                        cfgSectioni(LIMIT, $2);
                   }                      
                | SCOORD rational
                   {
                        cfgSectionf(SCOORD, $2);
                   }                      
                | TCOORD rational
                   {
                        cfgSectionf(TCOORD, $2);
                   }                      
                | DELTAT rational
                   {
                        cfgSectionf(DELTAT, $2);
                   }                      
                | WIDTH rational
                   {
                        cfgSectionf(WIDTH, $2);
                   }                      
                | HEIGHT rational
                   {
                        cfgSectionf(HEIGHT, $2);
                   }                      
                | error closesect
                ;

string		: STRING		{ ptr = (char *)malloc(strlen($1)+1);
                                          if (ptr)
                                          { 
                                             strcpy(ptr, $1);
                                             rmDQuote(ptr);
                                          }
					  $$ = ptr;
					}
                ;
number		: NUMBER		{ $$ = $1; }
		;
rational	: RATIONAL		{ $$ = $1; }
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

/* search the 'public' planet list */
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

  clog("%s: could not find texarea %s in texture %s",
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

static Unsgn32 hex2color(char *str)
{
  Unsgn32 v;

  /* default to 0 (black/transparent) */

  if (!str)
    return 0;

  if (sscanf(str, "%x", &v) != 1)
    {
      clog("hex2color(): invalid color specification '%s' at line %d, setting to 0",
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
            clog("%s: animdef %s: texture %s does not exist.",
                 __FUNCTION__, 
                 _cqiAnimDefs[i].name,
                 _cqiAnimDefs[i].texname);
            return FALSE;
          }
      }

      /* now check each anim type */
      
      /* texanim */
      if (_cqiAnimDefs[i].anims & CQI_ANIMS_TEX)
        {
          /* stages must be non-zero */
          if (!_cqiAnimDefs[i].texanim.stages)
            {
              clog("%s: animdef %s: texanim: stages must be non-zero.",
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
              snprintf(tbuf, CQI_NAMELEN - 1, "%s%d",
                       _cqiAnimDefs[i].texname,
                       j);
              
              /* locate the texture */
              if (_cqiFindTexture(tbuf) < 0)
                {                     /* nope */
                  clog("%s: animdef %s: texanim: texture %s does not exist.",
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
          /* if stages is 0 (meaning infinite) then loops is meaningless -
             set to 0 and warn. */
          if (!_cqiAnimDefs[i].colanim.stages && 
              _cqiAnimDefs[i].colanim.loops)
            {
              if (cqiVerbose)
                clog("%s: animdef %s: colanim: stages is 0, forcing loops to 0.",
                     __FUNCTION__, 
                     _cqiAnimDefs[i].name,
                     tbuf);
              
              _cqiAnimDefs[i].colanim.loops = 0;
            }
        }

      /* geoanim */
      if (_cqiAnimDefs[i].anims & CQI_ANIMS_GEO)
        {
          /* if stages is 0 (meaning infinite) then loops is meaningless -
             set to 0 and warn. */
          if (!_cqiAnimDefs[i].geoanim.stages && 
              _cqiAnimDefs[i].geoanim.loops)
            {
              if (cqiVerbose)
                clog("%s: animdef %s: geoanim: stages is 0, forcing loops to 0.",
                     __FUNCTION__, 
                     _cqiAnimDefs[i].name,
                     tbuf);
              
              _cqiAnimDefs[i].geoanim.loops = 0;
            }
        }
    } /* for */


  /* now, make sure each animation specifies an existing
     animdef, set up adIndex */
  for (i=0; i < numAnimations; i++)
    {
      if ((ndx = _cqiFindAnimDef(_cqiAnimations[i].animdef)) < 0)
        {                       /* nope */
          clog("%s: animdef %s does not exist for animation %s.",
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
  int mur = -1;
  int homeplan[NUMPLAYERTEAMS];             /* count of home planets */

  /* first things first... If there was no global read, then no
     point in continuing */

  if (!globalRead)
    return FALSE;

  memset((void *)homeplan, 0, sizeof(int) * NUMPLAYERTEAMS);

  if (mur < 0)
    {
      if ((mur = _cqiFindPlanet("Murisak")) < 0)
        {
          clog("%s: cannot find planet Murisak, which must exist",
                  __FUNCTION__);
          mur = 0;
        }
    }

  /* first fill in any empty slots */
  if (numPlanets < NUMPLANETS)
    {
      for (i = numPlanets; i < _cqiGlobal->maxplanets; i++)
        {
          /* use the slot number in the name to reduce chance of dup names */
          snprintf(_cqiPlanets[i].name, MAXPLANETNAME - 1, "ZZExtra %d", 
                   i);
          /* FIXME - no hc mur */
          strcpy(_cqiPlanets[i].primname, "Murisak");

          _cqiPlanets[i].primary = mur;
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
          _cqiPlanets[i].color = 0xffe6e6e6;
        }

      if (cqiVerbose)
        clog("%s: filled %d unspecified planet slots.",
                __FUNCTION__, NUMPLANETS - numPlanets);
    }


  for (i=0; i < numPlanets; i++)
    {
      /* see if the primary name == name, if so, orbit mur and vel = 0 */

      if (!strncmp(_cqiPlanets[i].name, _cqiPlanets[i].primname, 
                   MAXPLANETNAME))
        {
          _cqiPlanets[i].velocity = 0.0;

          /* FIXME - need a ghost 0, not harcoded mur */
          _cqiPlanets[i].primary = mur;
        }
      else
        {                       /* else, find the primary, default to mur */
          if ((_cqiPlanets[i].primary = _cqiFindPlanet(_cqiPlanets[i].primname)) < 0)
            {                   /* couldn't find it */
              if (cqiVerbose && i != mur)
                clog("%s: can't find primary '%s' for planet '%s', defaulting to '%s'",
                        __FUNCTION__,
                        _cqiPlanets[i].primname,
                        _cqiPlanets[i].name,
                        _cqiPlanets[mur].name);
              
              _cqiPlanets[i].primary = mur;
            }
        }

      /* count home planets */

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


  /* make sure 3 homeplanets per 'normal' team have been specified */
  for (i=0; i < NUMPLAYERTEAMS; i++)
    {
      if (homeplan[i] != 3)
        {
          clog("%s: team %s must have 3 homeplanets. %d were specified.",
                  __FUNCTION__, team2str(i), homeplan[i]);
          return FALSE;
        }
    }


  if (cqiVerbose)
    clog("%s: total planets %d (%d loaded, %d extra)",
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
  char buffer[BUFFER_SIZE];

  cqiDebugl = debugl;
  cqiVerbose = verbosity;

  switch (rcid)
    {
    case CQI_FILE_CONQINITRC:   /* optional */
      if (filename)
        strncpy(buffer, filename, BUFFER_SIZE - 1);
      else
        snprintf(buffer, sizeof(buffer)-1, "%s/%s", CONQETC, "conqinitrc");
      break;
    case CQI_FILE_TEXTURESRC:
    case CQI_FILE_TEXTURESRC_ADD: 
      if (filename)
        strncpy(buffer, filename, BUFFER_SIZE - 1);
      else
        snprintf(buffer, sizeof(buffer)-1, "%s/%s", CONQETC, "texturesrc");
      break;
    case CQI_FILE_SOUNDRC:
    case CQI_FILE_SOUNDRC_ADD: 
      if (filename)
        strncpy(buffer, filename, BUFFER_SIZE - 1);
      else
        snprintf(buffer, sizeof(buffer)-1, "%s/%s", CONQETC, "soundrc");
      break;
    default:                    /* programmer error */
      clog("%s: invalid rcid %d, bailing.", __FUNCTION__, rcid);
      return FALSE;
      break;
    }

  clog("%s: Loading '%s'...", __FUNCTION__, buffer);
  if ((infile = fopen(buffer, "r")) == NULL)
    {
      clog("%s: fopen(%s) failed: %s",
           __FUNCTION__,
           buffer,
           strerror(errno));
      
      /* a failed CQI_FILE_TEXTURESRC_ADD is no big deal,
         CQI_FILE_TEXTURESRC/CONQINITRC is another story however... */

      clog("%s: using default init tables.", __FUNCTION__);
      switch(rcid)
        {
        case CQI_FILE_TEXTURESRC:
          {
            clog("%s: FATAL: no textures.", __FUNCTION__);
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
      clog("conqinit: parse error." );
      fail = TRUE;
    }
  
  fclose(infile);

  /* check textures early */
  if (rcid == CQI_FILE_TEXTURESRC || 
      rcid == CQI_FILE_TEXTURESRC_ADD)
    {
      if (fail && rcid == CQI_FILE_TEXTURESRC)
        {
          clog("%s: FATAL: no textures.", __FUNCTION__);
          return FALSE;
        }

      cqiTextures = _cqiTextures;
      cqiNumTextures = numTextures;

      if (cqiVerbose)
        clog("%s: loaded %d texture descriptors.",
             __FUNCTION__, fileNumTextures);

      /* now validate any animations */
      if (!cqiValidateAnimations())
        {
          clog("%s: FATAL: no animations.", __FUNCTION__);
          return FALSE;
        }

      if (cqiVerbose)
        {
          clog("%s: loaded %d Animation descriptors.",
               __FUNCTION__, fileNumAnimations);
          clog("%s: loaded %d Animation definitions.",
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
          clog("%s: using default sound data.", __FUNCTION__);
          cqiSoundConf = &defaultSoundConf;
          cqiSoundEffects  = defaultSoundEffects;
          cqiNumSoundEffects = defaultNumSoundEffects;
          cqiSoundMusic = defaultSoundMusic;
          cqiNumSoundMusic = defaultNumSoundMusic;

          return FALSE;
        }

      clog("%s: loaded %d Music definitions.",
           __FUNCTION__, fileNumMusic);
      clog("%s: loaded %d Effect definitions.",
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
      clog("%s: cqiValidatePlanets() failed.", __FUNCTION__);

      cqiGlobal    = &defaultGlobalInit;
      cqiShiptypes = defaultShiptypes;
      cqiPlanets   = defaultPlanets;
      clog("%s: using default init tables.", __FUNCTION__);
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
      clog("%s: using default init tables.", __FUNCTION__);
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
  char buf[MAXLINE];
  int i;
  

  if (!cqiSoundConf || !cqiNumSoundEffects)
    return;

  /* preamble */
  getdandt( buf, 0 );
  printf("/* Generated by conqinit on %s */\n", buf);
  printf("/* $Id$ */\n");
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
  char buf[MAXLINE];
  int i;
  
  /* preamble */
  getdandt( buf, 0 );
  printf("/* Generated by conqinit on %s */\n", buf);
  printf("/* $Id$ */\n");
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

/* FIXME - planet name == primary indicates stationary */
      
      if (cqiPlanets[i].primary)
        {
          printf("   \"%s\",\n", cqiPlanets[cqiPlanets[i].primary].name);
          printf("   %d,\n", cqiPlanets[i].primary);
        }
      else  
        {
          printf("   \"Murisak\",\n");      
          printf("   0,\n");
        }

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

      printf("   0x%08x\n", cqiPlanets[i].color);
      printf(" },\n");

    }
  printf("};\n\n");

  printf("#endif /* _INITDATA_H */\n\n");

  
  return;
}


/* Dump the current universe to stdout in conqinitrc format */
void dumpUniverse(void)
{
  char buf[MAXLINE];
  int i, j;
  
  map_common();

  getdandt( buf, 0 );
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
  for (i=1; i <= NUMPLANETS; i++)
    {
      printf("planet {\n");
      printf("  name        \"%s\"\n", Planets[i].name);

/* FIXME - planet name == primary indicates stationary */
      if (Planets[i].primary)
        printf("  primary     \"%s\"\n", Planets[Planets[i].primary].name);
      else  
        printf("  primary     \"\"\n");      

      printf("  angle       %f\n", Planets[i].orbang);
      printf("  velocity    %f\n", Planets[i].orbvel);
      printf("  radius      %f\n", Planets[i].orbrad);
      printf("  ptype       \"%s\"\n", ptype2str(Planets[i].type));
      printf("  pteam       \"%s\"\n", team2str(Planets[i].team));
      printf("  armies      %d\n", Planets[i].armies);
      printf("  visible     \"%s\"\n", (Planets[i].real) ? "yes" : "no");
      if (i <= NUM_BASEPLANETS && Planets[i].real && 
          (Planets[i].type != PLANET_MOON && Planets[i].type != PLANET_SUN &&
           Planets[i].type != PLANET_GHOST))
        printf("  core        \"yes\"\n");
      else
        printf("  core        \"no\"\n");
      
      /* look for homeplanets.  The 'homeplanet' concept should be moved
         into the planet struct of the cmn block someday. */
      for (j=0; j<3; j++)
        if (Teams[Planets[i].team].teamhplanets[j] == i)
          break;

      if (j >= 3)
        printf("  homeplanet  \"no\"\n");
      else
        printf("  homeplanet  \"yes\"\n");

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

static void startSubSection(int subsection)
{
  if (cqiDebugl)
    clog(" %s: %s", __FUNCTION__, sect2str(subsection));

  if (!cursection)
    {                           /* can't have a subsection then */
      cursubsection = 0;
      clog("%s: cursection not active, cannot set subsection %s .",
           __FUNCTION__, sect2str(subsection));
      return;
    }
      
  if (!subsection)
    return;

  switch (cursection)
    {
    case ANIMDEF:
      {
        switch (subsection)
          {
          case TEXANIM:
          case COLANIM:
          case GEOANIM:
          case TOGANIM:
          case ISTATE:
            /* OK */
            break;
          default:
            clog("%s: subsection %s not supported for ANIMDEF.",
                 __FUNCTION__, sect2str(subsection));
            goterror++;
            break;
          }
      }
      break;

    case TEXTURE:
      {
        switch (subsection)
          {
          case TEXAREA:
            {
              memset((void *)&currTexArea, 0, sizeof(cqiTextureAreaRec_t));
            }
            break;
          default:
            clog("%s: subsection %s not supported for TEXTURE.",
                 __FUNCTION__, sect2str(subsection));
            goterror++;
            break;
          }
      }
      break;

    default:
      clog("%s: cursection %s does not support subsection %s.",
           __FUNCTION__, sect2str(cursection), sect2str(subsection));
      return;
      break;
    }

  cursubsection = subsection;
  return;
}
    
static void startSection(int section)
{
  if (cqiDebugl)
    clog("%s: %s", __FUNCTION__, sect2str(section));
  
  switch (section)
    {
    case GLOBAL:    
      {
        if (globalRead)
          {
            clog("%s: global section already configured\n",
                 __FUNCTION__);
            goterror++;
            return;
          }

        _cqiGlobal = malloc(sizeof(cqiGlobalInitRec_t));
        if (!_cqiGlobal)
          {
            clog("%s: Could not allocate GlobalInitRec",
                    __FUNCTION__);
            goterror++;
          }
        else
          memset((void *)_cqiGlobal, 0, sizeof(cqiGlobalInitRec_t));
      }
      break;

    case SHIPTYPE:    
      {
        if (!globalRead)
          {
            clog("%s: Have not read the global section (which must always be first). Ignoring SHIPTYPE",
                    __FUNCTION__);
            goterror++;
            return;
          }
      }
      break;
    case PLANET:    
      {
        if (!globalRead)
          {
            clog("%s: Have not read the global section (which must always be first). Ignoring PLANET",
                    __FUNCTION__);
            goterror++;
            return;
          }
        /* clear and init the planet for parsing */
        memset((void *)&currPlanet, 0, sizeof(cqiPlanetInitRec_t));

        currPlanet.primary = -1;
        currPlanet.size = 300;  /* ### */
        currPlanet.color = 0;
      }
      break;

    case TEXTURE:
      {
        memset((void *)&currTexture, 0, sizeof(cqiTextureInitRec_t));
        currTexAreas = NULL;
        numTexAreas = 0;
      }
      break;

    case ANIMATION:
      {
        memset((void *)&currAnimation, 0, sizeof(cqiAnimationInitRec_t));
        currAnimation.adIndex = -1;
      }
      break;

    case ANIMDEF:
      {
        memset((void *)&currAnimDef, 0, sizeof(cqiAnimDefInitRec_t));
      }
      break;

    case SOUNDCONF:    
      {
        if (!_cqiSoundConf)
          {                     /* starting fresh */
            _cqiSoundConf = malloc(sizeof(cqiSoundConfRec_t));
            if (!_cqiSoundConf)
              {
                clog("%s: Could not allocate SoundConf",
                     __FUNCTION__);
                goterror++;
              }
            else
              {
                memset((void *)_cqiSoundConf, 0, sizeof(cqiSoundConfRec_t));
                _cqiSoundConf->stereo = TRUE; /* default to stereo */
              }
            
          } /* else we are just overriding */
      }
      break;

    case EFFECT:
    case MUSIC:
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
  
  cursection = section;
  return;
}

static void endSection(void)
{
  if (cqiDebugl)
    {
      if (cursubsection)
        clog(" %s: %s", __FUNCTION__, sect2str(cursubsection));
      else
        clog("%s: %s", __FUNCTION__, sect2str(cursection));
    }        
  
  if (cursubsection)
    {                           /* then we are ending a subsection */
      switch (cursubsection)
        {
        case TEXANIM:
          {
            currAnimDef.anims |= CQI_ANIMS_TEX; 
          }
          break;
        case COLANIM:
          {
            currAnimDef.anims |= CQI_ANIMS_COL; 
          }
          break;
        case GEOANIM:
          {
            currAnimDef.anims |= CQI_ANIMS_GEO;
          }
          break;
        case TOGANIM:
          {
            currAnimDef.anims |= CQI_ANIMS_TOG;
          }
          break;

        case TEXAREA:
          /* this is only valid from within a texture definition */
          if (cursection == TEXTURE)
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
                      clog("%s: Could not realloc %d texareas for texture %s, ignoring texarea '%s'",
                         __FUNCTION__,
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
                  clog("%s: texarea name at or near line %d was not specified, ignoring.",
                       __FUNCTION__, lineNum);
                }
            }
          break;

        default:
          break;
        }

      cursubsection = 0;
      return; 
    }

  /* else we are ending a toplevel section */
  switch (cursection)
    {
    case GLOBAL:    
      {                         /* make sure everything is specified, alloc
                                   new planet/shiptype arrays, reset counts
                                */ 
        
        if (!_cqiGlobal->maxplanets || !_cqiGlobal->maxships ||
            !_cqiGlobal->maxusers || !_cqiGlobal->maxhist ||
            !_cqiGlobal->maxmsgs)
          {                     /* something missing */
            clog("%s: GLOBAL section is incomplete, ignoring.",
                 __FUNCTION__);
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
                clog("%s: could not allocate memory for planets.",
                     __FUNCTION__);
                globalRead = FALSE; /* redundant I know, but.... */
                goterror++;
              }
            else
              memset((void *)_cqiPlanets, 0, sizeof(cqiPlanetInitRec_t) *
                     _cqiGlobal->maxplanets);
          }
      }
      break;
    case SHIPTYPE:    
      break;
    case PLANET:    
      {
        /* check some basic things */
        if (!currPlanet.name[0] || !currPlanet.primname[0])
          {
            clog("%s: planet %d is missing name and/or primary",
                 __FUNCTION__, numPlanets);
            goterror++;
            return;
          }
        
        if (numPlanets >= _cqiGlobal->maxplanets)
          {
            clog("%s: planet '%s' (%d) exceeds maxplanets (%d), ignoring.",
                 __FUNCTION__, currPlanet.name, numPlanets, 
                 _cqiGlobal->maxplanets);
            return;
          }
        
        /* need more checks here ? */
        
        /* add it */
        _cqiPlanets[numPlanets] = currPlanet;
        numPlanets++;
      }
      
      break;
      
    case TEXTURE:
      {
        cqiTextureInitPtr_t texptr;
        char *ch;
        int exists = -1;
        
        /* verify the required info was provided */
        if (!strlen(currTexture.name))
          {
            clog("%s: texture name at or near line %d was not specified, ignoring.",
                 __FUNCTION__, lineNum);
            return;
          }
        
        /* check the texname for banned substances ('/' and '.') */
        while ((ch = strchr(currTexture.name, '.')))
          *ch = '_';
        while ((ch = strchr(currTexture.name, '/')))
          *ch = '_';
        
        /* if the texture was overrided by a later definition
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
              clog("%s: texture '%s' near line %d: overriding already "
                   "loaded texture.",
                   __FUNCTION__, currTexture.name, lineNum);
          }
        else
          {                     /* make a new one */
            texptr = (cqiTextureInitPtr_t)realloc((void *)_cqiTextures, 
                                                  sizeof(cqiTextureInitRec_t) * 
                                                  (numTextures + 1));
            
            if (!texptr)
              {  
                clog("%s: Could not realloc %d textures, ignoring texture '%s'",
                     __FUNCTION__,
                     numTextures + 1,
                     currTexture.name);
                return;
              }

            _cqiTextures = texptr;
            _cqiTextures[numTextures] = currTexture;
            numTextures++;
          }
        fileNumTextures++;
      }
      break;

    case ANIMATION:
      {
        cqiAnimationInitPtr_t animptr;
        int exists = -1;
        
        /* verify the required info was provided */
        if (!strlen(currAnimation.name))
          {
            clog("%s: animation name at or near line %d was not specified, ignoring.",
                 __FUNCTION__, lineNum);
            return;
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
              clog("%s: animation '%s' near line %d: overriding already loaded "
                   "animation.",
                   __FUNCTION__, currAnimation.name, lineNum);
          }
        else
          {                     /* make a new one */
            animptr = (cqiAnimationInitPtr_t)realloc((void *)_cqiAnimations, 
                                                  sizeof(cqiAnimationInitRec_t) * 
                                                  (numAnimations + 1));
            
            if (!animptr)
              {  
                clog("%s: Could not realloc %d animations, ignoring animation '%s'",
                     __FUNCTION__,
                     numAnimations + 1,
                     currAnimation.name);
                return;
              }
            
            _cqiAnimations = animptr;
            _cqiAnimations[numAnimations] = currAnimation;
            numAnimations++;
          }
        fileNumAnimations++;
      }        
      break;
      
    case ANIMDEF:
      {
        cqiAnimDefInitPtr_t animptr;
        int exists = -1;
        
        /* verify the required info was provided */
        if (!strlen(currAnimDef.name))
          {
            clog("%s: animdef name at or near line %d was not specified, ignoring.",
                 __FUNCTION__, lineNum);
            return;
          }
        
        exists = _cqiFindAnimDef(currAnimDef.name);

        if (exists >= 0)
          {
            _cqiAnimDefs[exists] = currAnimDef;
            if (cqiDebugl)
              clog("%s: animdef '%s' near line %d: overriding already loaded "
                   "animdef.",
                   __FUNCTION__, currAnimDef.name, lineNum);
          }
        else if (!currAnimDef.anims)
          {                     /* no animation types were declared */
            clog("%s: animdef '%s' near line %d: declared no animation "
                 "type sections. Ignoring.",
                 __FUNCTION__, currAnimDef.name, lineNum);
            return;
          }
        else
          {                     /* make a new one */
            animptr = (cqiAnimDefInitPtr_t)realloc((void *)_cqiAnimDefs, 
                                                  sizeof(cqiAnimDefInitRec_t) * 
                                                  (numAnimDefs + 1));
            
            if (!animptr)
              {  
                clog("%s: Could not realloc %d animdefs, ignoring animdef '%s'",
                     __FUNCTION__,
                     numAnimDefs + 1,
                     currAnimDef.name);
                return;
              }
            
            _cqiAnimDefs = animptr;
            _cqiAnimDefs[numAnimDefs] = currAnimDef;
            numAnimDefs++;
          }
        fileNumAnimDefs++;
      }        
      break;
      
    case SOUNDCONF:    
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


    case EFFECT:
      {
        cqiSoundPtr_t sndptr;
        char *ch;
        int exists = -1;
        
        /* verify the required info was provided */
        if (!strlen(currSound.name))
          {
            clog("%s: effect name at or near line %d was not specified, ignoring.",
                 __FUNCTION__, lineNum);
            return;
          }

        
        /* check the name for banned substances ('/' and '.') */
        while ((ch = strchr(currSound.name, '.')))
          *ch = '_';
        while ((ch = strchr(currSound.name, '/')))
          *ch = '_';
        
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
              clog("%s: effect '%s' near line %d: overriding already "
                   "loaded effect.",
                   __FUNCTION__, currSound.name, lineNum);
          }
        else
          {                     /* make a new one */
            sndptr = (cqiSoundPtr_t)realloc((void *)_cqiSoundEffects, 
                                                  sizeof(cqiSoundRec_t) * 
                                                  (numSoundEffects + 1));
            
            if (!sndptr)
              {  
                clog("%s: Could not realloc %d effect, ignoring effect '%s'",
                     __FUNCTION__,
                     numSoundEffects + 1,
                     currSound.name);
                return;
              }
            
            _cqiSoundEffects = sndptr;
            _cqiSoundEffects[numSoundEffects] = currSound;
            numSoundEffects++;
          }
        fileNumEffects++;
      }
      break;

    case MUSIC:
      {
        cqiSoundPtr_t sndptr;
        char *ch;
        int exists = -1;
        
        /* verify the required info was provided */
        if (!strlen(currSound.name))
          {
            clog("%s: music name at or near line %d was not specified, ignoring.",
                 __FUNCTION__, lineNum);
            return;
          }

        
        /* check the name for banned substances ('/' and '.') */
        while ((ch = strchr(currSound.name, '.')))
          *ch = '_';
        while ((ch = strchr(currSound.name, '/')))
          *ch = '_';
        
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
              clog("%s: music '%s' near line %d: overriding already "
                   "loaded music slot.",
                   __FUNCTION__, currSound.name, lineNum);
          }
        else
          {                     /* make a new one */
            sndptr = (cqiSoundPtr_t)realloc((void *)_cqiSoundMusic, 
                                                  sizeof(cqiSoundRec_t) * 
                                                  (numSoundMusic + 1));
            
            if (!sndptr)
              {  
                clog("%s: Could not realloc %d music slots, ignoring music '%s'",
                     __FUNCTION__,
                     numSoundMusic + 1,
                     currSound.name);
                return;
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
  
  cursection = 0;
  
  return;
}

/* integers */
static void cfgSectioni(int item, int val)
{
  if (cqiDebugl)
    {
      if (cursubsection)
        clog("   subsection = %s\titem = %s\tvali = %d",
             sect2str(cursubsection), item2str(item), val);
      else
        clog(" section = %s\titem = %s\tvali = %d",
             sect2str(cursection), item2str(item), val);
    }
  
  if (cursubsection)
    {
      switch (cursection)
        {
        case ANIMDEF:
          {
            switch(cursubsection)
              {
              case TEXANIM:
                {
                  switch(item)
                    {
                    case STAGES:
                      currAnimDef.texanim.stages = abs(val);
                      break;
                    case LOOPS:
                      currAnimDef.texanim.loops = abs(val);
                      break;
                    case DELAYMS:
                      currAnimDef.texanim.delayms = abs(val);
                      break;
                    case LOOPTYPE:
                      currAnimDef.texanim.looptype = abs(val);
                      break;
                    }
                }
                break;

              case COLANIM:
                {
                  switch(item)
                    {
                    case STAGES:
                      currAnimDef.colanim.stages = abs(val);
                      break;
                    case LOOPS:
                      currAnimDef.colanim.loops = abs(val);
                      break;
                    case DELAYMS:
                      currAnimDef.colanim.delayms = abs(val);
                      break;
                    case LOOPTYPE:
                      currAnimDef.colanim.looptype = abs(val);
                      break;
                    }
                }
                break;

              case GEOANIM:
                {
                  switch(item)
                    {
                    case STAGES:
                      currAnimDef.geoanim.stages = abs(val);
                      break;
                    case LOOPS:
                      currAnimDef.geoanim.loops = abs(val);
                      break;
                    case DELAYMS:
                      currAnimDef.geoanim.delayms = abs(val);
                      break;
                    case LOOPTYPE:
                      currAnimDef.geoanim.looptype = abs(val);
                      break;
                    }
                }
                break;

              case TOGANIM:
                {
                  switch(item)
                    {
                    case DELAYMS:
                      currAnimDef.toganim.delayms = abs(val);
                      break;
                    }
                }
                break;

              case ISTATE:
                {
                  switch(item)
                    {
                    case SIZE:
                      currAnimDef.isize = fabs((real)val);
                      currAnimDef.istates |= AD_ISTATE_SZ;
                      break;
                    }
                }
                break;
              } /* switch cursubsection */

          }
          break;
        } /* switch cursection */

      return;
    }

  switch (cursection)
    {
    case GLOBAL:    
      {
        switch (item)
          {
          case PLANETMAX:
            _cqiGlobal->maxplanets = abs(val);
            break;
          case SHIPMAX: 
            _cqiGlobal->maxships = abs(val);
            break;
          case USERMAX: 
            _cqiGlobal->maxusers = abs(val);
            break;
          case HISTMAX: 
            _cqiGlobal->maxhist = abs(val);
            break;
          case MSGMAX: 
            _cqiGlobal->maxmsgs = abs(val);
            break;
          }            
      }

      break;
    case SHIPTYPE:    
      break;
    case PLANET:    
      {
        switch(item)
          {
          case ARMIES:
            currPlanet.armies = abs(val);
            break;
          case SIZE:
            currPlanet.size = fabs((real)val);
            break;
          }
      }
      break;
    case ANIMDEF:
      {
        currAnimDef.timelimit = abs(val);
      }
      break;

    case SOUNDCONF:    
      {
        switch (item)
          {
          case SAMPLERATE:
            _cqiSoundConf->samplerate = CLAMP(8192, 44100, abs(val));
            break;
          case FXCHANNELS:
            _cqiSoundConf->fxchannels = CLAMP(2, 64, abs(val));
            break;
          case CHUNKSIZE:
            _cqiSoundConf->chunksize = CLAMP(256, 8192, abs(val));
            break;
          default:
            break;
          }
      }

    case EFFECT:
    case MUSIC:
      {
        switch(item)
          {
          case VOLUME:
            currSound.volume = CLAMP(0, 100, abs(val));
            break;
          case PAN:
            currSound.pan = CLAMP(-128, 128, abs(val));
            break;
          case FADEINMS:
            currSound.fadeinms = CLAMP(0, 10000, abs(val));
            break;
          case FADEOUTMS:
            currSound.fadeoutms = CLAMP(0, 10000, abs(val));
            break;
          case LOOPS:
            currSound.loops = abs(val);
            break;
          case LIMIT:
            currSound.limit = abs(val);
            break;
          case FRAMELIMIT:
            currSound.framelimit = abs(val);
            break;
          case DELAYMS:
            currSound.delayms = abs(val);
            break;
          }

      }
      
    default:
      break;
    }

  return;
}

/* integer pair */
void cfgSectionil(int item, int val1, int val2)
{
  if (cqiDebugl)
    {
      if (cursubsection)
        clog("    subsection = %s\titem = %s\tvalil = %d, %d",
             sect2str(cursubsection), item2str(item), val1, val2);
      else
        clog(" section = %s\titem = %s\tvalil = %d, %d",
             sect2str(cursection), item2str(item), val1, val2);
    }

  switch (cursection)
    {
    case PLANET:    
      {
        switch (item)
          {
          case ARMIES:
            {
              /* if we got a pair, randomly init one */
              /* make sure it's valid of course... */
              if (val1 >= val2 || val2 <= val1)
                {
                  clog("%s: Planet '%s's army min must be less than it's max: min %d max %d is invalid.",
                       __FUNCTION__,
                       currPlanet.name,
                       val1, val2);
                  currPlanet.armies = 0;
                }
              else
                currPlanet.armies = rndint(abs(val1), abs(val2));

#if 0
              clog("ARMIES got %d %d, rnd = %d\n",
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
    {

      if (cursubsection)
        clog("   subsection = %s\titem = %s\tvalf = %f",
             sect2str(cursubsection), item2str(item), val);
      else
        clog(" section = %s\titem = %s\tvalf = %f",
             sect2str(cursection), item2str(item), val);
    }

  if (cursubsection)
    {
      switch (cursection)
        {
        case ANIMDEF:
          {
            switch(cursubsection)
              {
              case TEXANIM:
                {
                  switch(item)
                    {
                    case DELTAS:
                      currAnimDef.texanim.deltas = val;
                      break;
                    case DELTAT:
                      currAnimDef.texanim.deltat = val;
                      break;
                    }
                }
                break;
              case COLANIM:
                {
                  switch(item)
                    {
                    case DELTAA:
                      currAnimDef.colanim.deltaa = val;
                      break;
                    case DELTAR: /* red */
                      currAnimDef.colanim.deltar = val;
                      break;
                    case DELTAG:
                      currAnimDef.colanim.deltag = val;
                      break;
                    case DELTAB:
                      currAnimDef.colanim.deltab = val;
                      break;
                    }
                }
                break;

              case GEOANIM:
                {
                  switch(item)
                    {
                    case DELTAX:
                      currAnimDef.geoanim.deltax = val;
                      break;
                    case DELTAY:
                      currAnimDef.geoanim.deltay = val;
                      break;
                    case DELTAZ:
                      currAnimDef.geoanim.deltaz = val;
                      break;
                    case DELTAR: /* rotation */
                      currAnimDef.geoanim.deltar = val;
                      break;
                    case DELTAS: /* size */
                      currAnimDef.geoanim.deltas = val;
                      break;
                    }
                }
                break;

              case ISTATE:
                {
                  switch(item)
                    {
                    case ANGLE:
                      currAnimDef.iangle = val;
                      currAnimDef.istates |= AD_ISTATE_ANG;
                      break;
                    }
                }
                break;

              } /* switch cursubsection */
            
          }
          break;
          
        case TEXTURE:
          {
            switch(cursubsection)
              {
              case TEXAREA:
                {
                  switch(item)
                    {
                    case XCOORD:
                      currTexArea.x = fabs(val);
                      break;
                    case YCOORD:
                      currTexArea.y = fabs(val);
                      break;
                    case WIDTH:
                      currTexArea.w = fabs(val);
                      break;
                    case HEIGHT:
                      currTexArea.h = fabs(val);
                      break;
                    }
                }
                break;
              } /* switch subsection */
          } /* TEXTURE */
          break;
          
        } /* switch cursection */
      
      return;
    }

  switch (cursection)
    {
    case GLOBAL:    
      break;
    case SHIPTYPE:    
      break;
    case PLANET:    
      {
        switch (item)
          {
          case ANGLE:
            currPlanet.angle = val;
            break;
          case VELOCITY:
            currPlanet.velocity = val;
            break;
          case RADIUS:
            currPlanet.radius = val;
            break;
          case XCOORD:
            currPlanet.xcoord = val;
            break;
          case YCOORD:
            currPlanet.ycoord = val;
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
    {
      if (cursubsection)
        clog("   subsection = %s\titem = %s\tvals = '%s'",
             sect2str(cursubsection), item2str(item), (val) ? val : "(NULL)" );
      else
        clog(" section = %s\titem = %s\tvals = '%s'",
             sect2str(cursection), item2str(item), (val) ? val : "(NULL)" );
    }  

  if (!val)
    return;

  if (cursubsection)
    {
      switch (cursection)
        {
        case ANIMDEF:
          {
            switch(cursubsection)
              {
              case TEXANIM:
                {
                  switch(item)
                    {
                    case COLOR:
                      currAnimDef.texanim.color = hex2color(val);
                      break;
                    }
                }
                break;

              case COLANIM:
                {
                  switch(item)
                    {
                    case COLOR:
                      currAnimDef.colanim.color = hex2color(val);
                      break;
                    }
                }
                break;

              case ISTATE:
                {
                  switch(item)
                    {
                    case COLOR:
                      currAnimDef.icolor = hex2color(val);
                      currAnimDef.istates |= AD_ISTATE_COL;
                      break;
                    case TEXNAME:
                      strncpy(currAnimDef.itexname, val, CQI_NAMELEN - 1);
                      currAnimDef.istates |= AD_ISTATE_TEX;
                      break;
                    }
                }
                break;

              } /* switch cursubsection */

          }
          break;

        case TEXTURE:
          {
            switch(cursubsection)
              {
              case TEXAREA:
                {
                  switch (item)
                    {
                    case NAME:
                      strncpy(currTexArea.name, val, CQI_NAMELEN - 1);
                      break;
                    }
                }
                break;
              }
          }
          break;

        } /* switch cursection */
      
      /* be sure to free the value allocated */
      free(val);
      return;
    }

  switch (cursection)
    {
    case GLOBAL:    
      break;
    case SHIPTYPE:    
      break;
    case PLANET:    
      {
        switch (item)
          {
          case NAME:
            strncpy(currPlanet.name, val, MAXPLANETNAME - 1);
            break;
          case PRIMARY:
            strncpy(currPlanet.primname, val, MAXPLANETNAME - 1);
            break;
          case PTYPE:
            currPlanet.ptype = str2ptype(val);
            break;
          case PTEAM:
            currPlanet.pteam = str2team(val);
            break;
          case TEXNAME:
            strncpy(currPlanet.texname, val, CQI_NAMELEN - 1);
            break;
          case COLOR:
            currPlanet.color = hex2color(val);
            break;
          }
      }
      break;

    case TEXTURE:
      {
        switch(item)
          {
          case NAME:
            strncpy(currTexture.name, val, CQI_NAMELEN - 1);
            break;
          case FILENAME:
            if (val[0] == 0)    /* empty filename means only color matters */
              currTexture.flags |= CQITEX_F_COLOR_SPEC;
            else
              strncpy(currTexture.filename, val, CQI_NAMELEN - 1);
            break;
          case COLOR:
            currTexture.color = hex2color(val);
            break;
          }            
      }
      break;
      
    case ANIMATION:
       {
        switch(item)
          {
          case NAME:
            strncpy(currAnimation.name, val, CQI_NAMELEN - 1);
            break;
          case ANIMDEF:
            strncpy(currAnimation.animdef, val, CQI_NAMELEN - 1);
            break;
          }
      }
      break;

    case ANIMDEF:
       {
        switch(item)
          {
          case NAME:
            strncpy(currAnimDef.name, val, CQI_NAMELEN - 1);
            break;
          case TEXNAME:
            strncpy(currAnimDef.texname, val, CQI_NAMELEN - 1);
            break;
          }
      }
      break;

    case EFFECT:
    case MUSIC:
      {
        switch(item)
          {
          case NAME:
            strncpy(currSound.name, val, CQI_NAMELEN - 1);
            break;
          case FILENAME:
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
    {
      if (cursubsection)
        clog("   subsection = %s\titem = %s\tvalb = '%s'",
             sect2str(cursubsection), item2str(item), 
             (bval ? "yes" : "no"));
      else
        clog(" section = %s\titem = %s\tvalb = '%s'",
             sect2str(cursection), item2str(item), 
             (bval ? "yes" : "no"));
    }

  if (bval == -1)
    {                           /* an error, do something sane */
      bval = 0;
    }
  
  switch (cursection)
    {
    case GLOBAL:    
      break;
    case SHIPTYPE:    
      break;
    case PLANET:    
      {
        switch(item) 
          {
          case VISIBLE:
            currPlanet.visible = bval;
            break;
          case CORE:
            currPlanet.core = bval;
            break;
          case HOMEPLANET:
            currPlanet.homeplanet = bval;
            break;
          }
      }
      break;
    case TEXTURE:
      break;

    case SOUNDCONF:    
      {
        switch (item)
          {
          case STEREO:
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
    case GLOBAL:
      return "GLOBAL";
      break;
    case SHIPTYPE:
      return "SHIPTYPE";
      break;
    case PLANET:
      return "PLANET";
      break;
    case TEXTURE:
      return "TEXTURE";
      break;
    case ANIMATION:
      return "ANIMATION";
      break;
    case ANIMDEF:
      return "ANIMDEF";
      break;
    case SOUNDCONF:
      return "SOUNDCONF";
      break;
    case EFFECT:
      return "EFFECT";
      break;
    case MUSIC:
      return "MUSIC";
      break;
    case LIMIT:
      return "LIMIT";
      break;
    case FRAMELIMIT:
      return "FRAMELIMIT";
      break;

      /* subsections */
    case TEXANIM:
      return "TEXANIM";
      break;
    case COLANIM:
      return "COLANIM";
      break;
    case GEOANIM:
      return "GEOANIM";
      break;
    case TOGANIM:
      return "TOGANIM";
      break;
    case ISTATE:
      return "ISTATE";
      break;
    case TEXAREA:
      return "TEXAREA";
      break;
    }
  
  return "UNKNOWN";
}

static char *item2str(int item)
{
  switch (item)
    {
    case PLANETMAX:
      return "PLANETMAX";
      break;
    case SHIPMAX:
      return "SHIPMAX";
      break;
    case USERMAX:
      return "USERMAX";
      break;
    case HISTMAX:
      return "HISTMAX";
      break;
    case MSGMAX:
      return "MSGMAX";
      break;
    case NAME:
      return "NAME";
      break;
    case ENGFAC:
      return "ENGFAC";
      break;
    case WEAFAC:
      return "WEAFAC";
      break;
    case ACCELFAC:
      return "ACCELFAC";
      break;
    case TORPWARP:
      return "TORPWARP";
      break;
    case WARPMAX:
      return "WARPMAX";
      break;
    case ARMYMAX:
      return "ARMYMAX";
      break;
    case SHMAX:
      return "SHMAX";
      break;
    case DAMMAX:
      return "DAMMAX";
      break;
    case TORPMAX:
      return "TORPMAX";
      break;
    case SIZE:
      return "SIZE";
      break;
    case FUELMAX:
      return "FUELMAX";
      break;
    case PRIMARY:
      return "PRIMARY";
      break;
    case ANGLE:
      return "ANGLE";
      break;
    case VELOCITY:
      return "VELOCITY";
      break;
    case RADIUS:
      return "RADIUS";
      break;
    case PTYPE:
      return "PTYPE";
      break;
    case PTEAM:
      return "PTEAM";
      break;
    case ARMIES:
      return "ARMIES";
      break;
    case VISIBLE:
      return "VISIBLE";
      break;
    case CORE:
      return "CORE";
      break;
    case XCOORD:
      return "XCOORD";
      break;
    case YCOORD:
      return "YCOORD";
      break;
    case SCOORD:
      return "SCOORD";
      break;
    case TCOORD:
      return "TCOORD";
      break;
    case TEXNAME:
      return "TEXNAME";
      break;
    case COLOR:
      return "COLOR";
      break;
    case HOMEPLANET:
      return "HOMEPLANET";
      break;
    case FILENAME:
      return "FILENAME";
      break;
    case ANIMDEF:
      return "ANIMDEF";
      break;
    case TIMELIMIT:
      return "TIMELIMIT";
      break;
    case STAGES:
      return "STAGES";
      break;
    case LOOPS:
      return "LOOPS";
      break;
    case DELAYMS:
      return "DELAYMS";
      break;
    case LOOPTYPE:
      return "LOOPTYPE";
      break;
    case DELTAA:
      return "DELTAA";
      break;
    case DELTAR:
      return "DELTAR";
      break;
    case DELTAG:
      return "DELTAG";
      break;
    case DELTAB:
      return "DELTAB";
      break;
    case DELTAX:
      return "DELTAX";
      break;
    case DELTAY:
      return "DELTAY";
      break;
    case DELTAZ:
      return "DELTAZ";
      break;
    case DELTAS:
      return "DELTAS";
      break;
    case DELTAT:
      return "DELTAT";
      break;
    case WIDTH:
      return "WIDTH";
      break;
    case HEIGHT:
      return "HEIGHT";
      break;

    case SAMPLERATE:
      return "SAMPLERATE";
      break;
    case VOLUME:
      return "VOLUME";
      break;
    case PAN:
      return "PAN";
      break;
    case STEREO:
      return "STEREO";
      break;
    case FXCHANNELS:
      return "FXCHANNELS";
      break;
    case CHUNKSIZE:
      return "CHUNKSIZE";
      break;
    case FADEINMS:
      return "FADEINMS";
      break;
    case FADEOUTMS:
      return "FADEOUTMS";
      break;
    case LIMIT:
      return "LIMIT";
      break;
    }
  
  return "UNKNOWN";
}


static void rmDQuote(char *str)
{
  char *i, *o;
  int n;
  int count;
  
  for (i=str+1, o=str; *i && *i != '\"'; o++)
    {
      if (*i == '\\')
	{
          switch (*++i)
	    {
	    case 'n':
              *o = '\n';
              i++;
              break;
	    case 'b':
              *o = '\b';
              i++;
              break;
	    case 'r':
              *o = '\r';
              i++;
              break;
	    case 't':
              *o = '\t';
              i++;
              break;
	    case 'f':
              *o = '\f';
              i++;
              break;
	    case '0':
              if (*++i == 'x')
                goto hex;
              else
                --i;
	    case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
              n = 0;
              count = 0;
              while (*i >= '0' && *i <= '7' && count < 3)
		{
                  n = (n<<3) + (*i++ - '0');
                  count++;
		}
              *o = n;
              break;
	    hex:
	    case 'x':
              n = 0;
              count = 0;
              while (i++, count++ < 2)
		{
                  if (*i >= '0' && *i <= '9')
                    n = (n<<4) + (*i - '0');
                  else if (*i >= 'a' && *i <= 'f')
                    n = (n<<4) + (*i - 'a') + 10;
                  else if (*i >= 'A' && *i <= 'F')
                    n = (n<<4) + (*i - 'A') + 10;
                  else
                    break;
		}
              *o = n;
              break;
	    case '\n':
              i++;	/* punt */
              o--;	/* to account for o++ at end of loop */
              break;
	    case '\"':
	    case '\'':
	    case '\\':
	    default:
              *o = *i++;
              break;
	    }
	}
      else
        *o = *i++;
    }
  *o = '\0';
}

static int parsebool(char *str)
{
  char *s;
  
  s = str;
  
  if (s == NULL)
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
      clog("parsebool(): error parsing '%s' line %d, \n\t%s\n",
              str, lineNum,
              "Boolean value must be 'yes', 'no', 'true', 'false', 'on', or 'off'.");
      return(-1);
    }
}


/* initrun - initalize for the run */
static void initrun(int rcid)
{
  if (rcid == CQI_FILE_CONQINITRC)
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

  if (rcid == CQI_FILE_SOUNDRC || rcid == CQI_FILE_SOUNDRC_ADD)
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

  if (rcid == CQI_FILE_TEXTURESRC || rcid == CQI_FILE_TEXTURESRC_ADD)
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

  return;
  
}


