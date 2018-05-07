//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


/*
 * Sound for Conquest, courtesy of Cataboligne for the concept.
 *
 */

#ifndef _CQSOUND_H
#define _CQSOUND_H

#include <string>
#include "conqinit.h"

typedef struct _cqs_channel {
    int channel;                  /* mixer channel */
    int active;
    int idx;                      /* index into Effects/Music array */
} cqsChannelRec_t, *cqsChannelPtr_t;

typedef struct _cqs_sound {
    uint32_t  cqiIndex;            /* index to cqi sound entry */
    void    *chunk;               /* ptr to mix/music chunk */
    int      vol;                 /* SDL volume */
    int      pan;                 /* SDL pan */
    uint32_t  lasttime;            /* last time this sample was played */
    int      fadeinms;            /* copies of cqi data */
    int      fadeoutms;
    int      loops;
    int      limit;
    uint32_t  framelimit;
    uint32_t  lastframe;
    uint32_t  framecount;
    uint32_t  delayms;
} cqsSoundRec_t, *cqsSoundPtr_t;

typedef uint32_t cqsHandle;      /* sound handle used for some
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

#ifdef NOEXTERN_CQSOUND
uint32_t             cqsSoundEnables  = 0;
int                  cqsSoundAvailable    = false;
cqsSoundPtr_t        cqsMusic   = NULL;
cqsSoundPtr_t        cqsEffects = NULL;
int                  cqsNumMusic = 0;
int                  cqsNumEffects = 0;
teamFX_t             cqsTeamEffects[NUMPLAYERTEAMS] = {};
teamMus_t            cqsTeamMusic[NUMPLAYERTEAMS] = {};
doomMus_t            cqsDoomsdayMusic = {};
#else
extern uint32_t      cqsSoundEnables;
extern int           cqsSoundAvailable;
extern cqsSoundPtr_t cqsMusic;
extern cqsSoundPtr_t cqsEffects;
extern int           cqsNumMusic;
extern int           cqsNumEffects;
extern teamFX_t      cqsTeamEffects[NUMPLAYERTEAMS];
extern teamMus_t     cqsTeamMusic[NUMPLAYERTEAMS];
extern doomMus_t     cqsDoomsdayMusic;
#endif

#if !((defined(HAVE_SDL) && HAVE_SDL == 1) && (defined(HAVE_SDL_MIXER) && HAVE_SDL_MIXER == 1))
#define CQS_NO_SOUND            /* we will not compile with sound support */

/* fake them out */
#define cqsInitSound()
#define cqsFindEffect(x)                    (-1)
#define cqsFindMusic(x)                     (-1)
#define cqsMusicPlay(x, y)                  do {;} while (0)
#define cqsMusicStop(x)                     do {;} while (0)
#define cqsEffectPlay(x, y, z, a, b)        do {;} while (0)
#define cqsEffectStop(x, y)                 do {;} while (0)
#define cqsUpdateVolume()
#define cqsMusicPlaying()                   (0)
#define cqsSetWarp(x)                       do {;} while (0)

#else  /* !(HAVE_SDL && HAVE_SDL_MIXER) */
/* the real thing */
void cqsInitSound(void);
int  cqsFindEffect(const std::string& name);
int  cqsFindMusic(const std::string& name);
int  cqsMusicPlay(int musidx, int halt);
int  cqsMusicStop(int halt);
int  cqsEffectPlay(int fxidx, cqsHandle *handle, real maxdist,
                   real dist, real ang);
int  cqsEffectStop(cqsHandle handle, int halt);
void cqsUpdateVolume(void);
int  cqsMusicPlaying(void);

// control warp up/down/engine noise
void cqsSetWarp(real warp);

#endif /* !(HAVE_SDL && HAVE_SDL_MIXER) */

#endif /* _CQSOUND_H */
