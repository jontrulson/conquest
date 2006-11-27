/* 
 * cqsound.h
 * 
 * $Id$
 *
 * Copyright 1999-2006 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 *
 * Sound for Conquest, curtesy of Cataboligne
 *
 */

#ifndef _CQSOUND_H
#define _CQSOUND_H 

#include "datatypes.h"
#include "conqinit.h"

typedef struct _cqs_channel {
  int channel;                  /* mixer channel */
  int active;
  int idx;                      /* index into Effects/Music array */
} cqsChannelRec_t, *cqsChannelPtr_t;

typedef struct _cqs_sound {
  Unsgn32 cqiIndex;             /* index to cqi sound entry */
  void *chunk;                  /* ptr to mix/music chunk */
  int vol;                      /* SDL volume */
  int pan;                      /* SDL pan */
  Unsgn32 lasttime;             /* last time this sample was played */
  int fadeinms;                 /* copies of cqi data */
  int fadeoutms;
  int loops;
  int limit;
  Unsgn32 framelimit;
  Unsgn32 lastframe;
  Unsgn32 framecount;
  Unsgn32 delayms;
} cqsSoundRec_t, *cqsSoundPtr_t;

typedef Unsgn32 cqsHandle;      /* sound handle used for some
                                   functions */

/* an 'invalid' handle */
#define CQS_INVHANDLE 0xffffffff

/* a struct for holding all of a team's sound indexes */
typedef struct _team_fx {
  int phaser;
  int torp;
  int torp3;
  int alert;
  int beamd;
  int beamu;
  int hit;
  int info;
  int mag;
  int warpu;
  int warpd;
} teamFX_t;

/* a struct for the team's musical preferences. */
typedef struct _team_mus {
  int intro;
  int battle;
  int approach;
  int theme;
} teamMus_t;

/* and another for the doomsday music */
typedef struct _doom_mus {
  int doom;
  int doomin;
  int doomkill;
} doomMus_t;


/* enable flags */
#define CQS_EFFECTS      0x00000001
#define CQS_MUSIC        0x00000002
#define CQS_ENABLE_MASK  (CQS_EFFECTS | CQS_MUSIC)

#define CQS_ISENABLED(x)        (cqsSoundAvailable && ((x) & cqsSoundEnables))
#define CQS_ENABLE(x)           (cqsSoundEnables |= (x))
#define CQS_DISABLE(x)          (cqsSoundEnables &= ~(x))
#define CQS_SOUND_DISABLED()    (!cqsSoundAvailable || ((cqsSoundEnables & CQS_ENABLE_MASK) == 0))

#ifdef _CQSOUND_NOEXTERN
Unsgn32 cqsSoundEnables  = 0;
int cqsSoundAvailable    = FALSE;
cqsSoundPtr_t cqsMusic   = NULL;
cqsSoundPtr_t cqsEffects = NULL;
int cqsNumMusic = 0;
int cqsNumEffects = 0;
teamFX_t teamEffects[NUMPLAYERTEAMS] = {};
teamMus_t teamMusic[NUMPLAYERTEAMS] = {};
doomMus_t doomMusic = {};
#else
extern Unsgn32 cqsSoundEnables;
extern int cqsSoundAvailable;
extern cqsSoundPtr_t cqsMusic;
extern cqsSoundPtr_t cqsEffects;
extern int cqsNumMusic;
extern int cqsNumEffects;
extern teamFX_t teamEffects[NUMPLAYERTEAMS];
extern teamMus_t teamMusic[NUMPLAYERTEAMS];
extern doomMus_t doomMusic;
#endif

#if !(HAVE_SDL && HAVE_SDL_MIXER)
#define CQS_NO_SOUND            /* we will not compile with sound support */

/* fake them out */
#define cqsInitSound()                      
#define cqsFindEffect(x)                    (-1)
#define cqsFindMusic(x)                     (-1)
#define cqsMusicPlay(x, y)                  FALSE
#define cqsMusicStop(x)                     FALSE
#define cqsEffectPlay(x, y, z, a)           FALSE
#define cqsEffectPlayTracked(x, y, z, a, b) FALSE
#define cqsEffectStop(x, y)                 FALSE
#define cqsUpdateVolume()                   
#define cqsMusicPlaying()

#else  /* !(HAVE_SDL && HAVE_SDL_MIXER) */
/* the real thing */
void cqsInitSound(void);
int cqsFindEffect(char *name);
int cqsFindMusic(char *name);
int cqsMusicPlay(int musidx, int halt);
int cqsMusicStop(int halt);
int cqsEffectPlay(int fxidx, real maxdist, real dist, real ang);
int cqsEffectPlayTracked(int fxidx, cqsHandle *handle, real maxdist, 
                         real dist, real ang);
int cqsEffectStop(cqsHandle handle, int halt);
void cqsUpdateVolume(void);
int cqsMusicPlaying(void);
 
#endif /* !(HAVE_SDL && HAVE_SDL_MIXER) */

#endif /* _CQSOUND_H */
