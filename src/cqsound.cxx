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

#include "c_defs.h"

#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "global.h"

#include "conf.h"
#include "conqinit.h"
#include "conqutil.h"

#define NOEXTERN_CQSOUND
#include "cqsound.h"
#undef  NOEXTERN_CQSOUND

#ifndef CQS_NO_SOUND
#include "SDL/SDL.h"
#include "SDL/SDL_mixer.h"

/* effects channels */
static std::vector<cqsChannelRec_t> cqsChannels;

#define CQS_MAX_CHANNELS (cqiSoundConf->fxchannels)

#define EFFECT_VOL(_fxvol) (int)((real)(_fxvol) *                       \
                                 ((real)(UserConf.effectsVol) / 100.0))

#define MUSIC_VOL(_musvol) (int)((real)(_musvol) *                      \
                                 ((real)(UserConf.musicVol) / 100.0))

static int _mus_idx = -1;       /* current music index */

/* need to start an new song when the current one fades out */
static int _mus_que = -1;

extern uint32_t frameTime;

/* called when music finishes */
static void _music_finished(void)
{
    int curque = _mus_que;

    _mus_idx = -1;

#if defined(DEBUG_SOUND)
    utLog("%s: called, curque %d", __FUNCTION__,
          curque);
#endif

    if (Mix_PlayingMusic())
    {                           /* shouldn't happen */
        utLog("%s: Music is still playing!", __FUNCTION__);
        return;
    }

    /* if there is an index que'd, start playing it */
    if (curque >= 0)
    {
        _mus_que = -1;               /* turn off the que */
        cqsMusicPlay(curque, false); /* and play it */
    }

    return;
}


/* called when a tracked effect finishes */
static void _effect_finished(int channel)
{
    int i;

    for (i=0; i<CQS_MAX_CHANNELS; i++)
    {
        if (cqsChannels[i].channel == channel)
        {                       /* turn it off */
            cqsChannels[i].channel = -1;
            cqsChannels[i].active = false;
            cqsChannels[i].idx = -1;
        }
    }

    return;
}

/* search effect list by filename */
static int cqsFindEffectByFile(const char *name)
{
    int i;

    for (i=0; i<cqsEffects.size(); i++)
        if (!strncmp(cqiSoundEffects[cqsEffects[i].cqiIndex].filename,
                     name, CQI_NAMELEN))
            return i;

    return -1;
}

static int cqsFindMusicByFile(const char *name)
{
    int i;

    for (i=0; i<cqsMusic.size(); i++)
        if (!strncmp(cqiSoundMusic[cqsMusic[i].cqiIndex].filename,
                     name, CQI_NAMELEN))
            return i;

    return -1;
}

int cqsFindEffect(const std::string& name)
{
    int i;

    for (i=0; i<cqsEffects.size(); i++)
        if (!strncmp(cqiSoundEffects[cqsEffects[i].cqiIndex].name,
                     name.c_str(), CQI_NAMELEN))
            return i;

    return -1;
}

int cqsFindMusic(const std::string& name)
{
    int i;

    for (i=0; i<cqsMusic.size(); i++)
        if (!strncmp(cqiSoundMusic[cqsMusic[i].cqiIndex].name,
                     name.c_str(), CQI_NAMELEN))
            return i;

    return -1;
}

/* look for a sound file and return a file name.  We look
   first in the users ~/.conquest/sound/ dir (allowing users to override
   the pre-defined samples), then in CONQSHARE/sound.  Return NULL if not
   found */
static char *_getSoundFile(const char *filenm)
{
    char *homevar;
    FILE *fd;
    static char buffer[PATH_MAX];

    /* look for a user sounds */
    if ((homevar = getenv(CQ_USERHOMEDIR)))
    {
        /* try OGG first, then wav */
        snprintf(buffer, PATH_MAX, "%s/%s/sound/%s.ogg",
                 homevar, CQ_USERCONFDIR, filenm);

        if ((fd = fopen(buffer, "r")))
        {                       /* found one */
            fclose(fd);
            return buffer;
        }

        snprintf(buffer, PATH_MAX, "%s/%s/sound/%s.wav",
                 homevar, CQ_USERCONFDIR, filenm);

        if ((fd = fopen(buffer, "r")))
        {                       /* found one */
            fclose(fd);
            return buffer;
        }
    }

    /* if we are here, look for the system one */
    /* first ogg, then wav */
    snprintf(buffer, PATH_MAX, "%s/sound/%s.ogg",
             utGetPath(CONQSHARE), filenm);


    if ((fd = fopen(buffer, "r")))
    {                       /* found one */
        fclose(fd);
        return buffer;
    }

    snprintf(buffer, PATH_MAX, "%s/sound/%s.wav",
             utGetPath(CONQSHARE), filenm);


    if ((fd = fopen(buffer, "r")))
    {                       /* found one */
        fclose(fd);
        return buffer;
    }

    return NULL;
}

/* go through the cqi arrays, load all of the sounds, and populate
   cqsEffects and cqsMusic .*/
static int cqsLoadSounds(void)
{
    int i;
    cqsSoundRec_t cursound;
    Mix_Chunk     *mc = NULL;
    Mix_Music     *mm = NULL;
    char *ch;
    int numfx, nummus;

    /* first the music */
    nummus = 0;
    for (i=0; i<cqiNumSoundMusic; i++)
    {
        int ndx;

        memset((void *)&cursound, 0, sizeof(cqsSoundRec_t));

        /* first see if this soundfile was already loaded */
        if ((ndx = cqsFindMusicByFile(cqiSoundMusic[i].filename)) > 0)
        {                       /* the sound was previously loaded */
            cursound.chunk = cqsMusic[ndx].chunk;
        }
        else
        {
            /* need to load it */

            if ((ch = _getSoundFile(cqiSoundMusic[i].filename)) == NULL)
            {
                utLog("%s: Could not find Music file '%s'",
                      __FUNCTION__,
                      cqiSoundMusic[i].filename);
                continue;
            }

            if ((mm = Mix_LoadMUS(ch)) == NULL)
            {
                utLog("%s: Could not load Music file '%s': %s",
                      __FUNCTION__,
                      cqiSoundMusic[i].filename, Mix_GetError());
                continue;
            }

            cursound.chunk = mm;
            nummus++;
        }

        /* if we are here, we can setup the cqs entry for it */
        cursound.cqiIndex = i;
        cursound.vol = (int)((real)MIX_MAX_VOLUME *
                             ((real)cqiSoundMusic[i].volume / 100.0));
        /* this is really the left pan volume.  right = 254 - left */
        cursound.pan = CLAMP(0, 254, (cqiSoundMusic[i].pan + 128));
        cursound.fadeinms = cqiSoundMusic[i].fadeinms;
        cursound.fadeoutms = cqiSoundMusic[i].fadeoutms;

        /* for music, -1 == infinite, 0 = never, so... */
        if (cqiSoundMusic[i].loops == 0)
            cursound.loops = -1;
        else
            cursound.loops = cqiSoundMusic[i].loops;

        cursound.limit = cqiSoundMusic[i].limit;

        cursound.framelimit = cqiSoundMusic[i].framelimit;
        cursound.lastframe = 0;
        cursound.framecount = 0;

        cursound.delayms = cqiSoundMusic[i].delayms;
        cursound.lasttime = 0;

        /* now add into list */
        cqsMusic.push_back(cursound);
    }

    /* now the effects */
    numfx = 0;
    for (i=0; i<cqiNumSoundEffects; i++)
    {
        int ndx;

        memset((void *)&cursound, 0, sizeof(cqsSoundRec_t));

        /* first see if this soundfile was already loaded */
        if ((ndx = cqsFindEffectByFile(cqiSoundEffects[i].filename)) > 0)
        {                       /* the sound was previously loaded */
            cursound.chunk = cqsEffects[ndx].chunk;
        }
        else
        {
            /* need to load it */

            if ((ch = _getSoundFile(cqiSoundEffects[i].filename)) == NULL)
            {
                utLog("%s: Could not find Effects file '%s'",
                      __FUNCTION__,
                      cqiSoundEffects[i].filename);
                continue;
            }

            if ((mc = Mix_LoadWAV(ch)) == NULL)
            {
                utLog("%s: Could not load Effects file '%s': %s",
                      __FUNCTION__,
                      cqiSoundEffects[i].filename, Mix_GetError());
                continue;
            }

            cursound.chunk = mc;
            numfx++;
        }

        /* if we are here, we can setup the cqs entry for it */
        cursound.cqiIndex = i;
        cursound.vol = (int)((real)MIX_MAX_VOLUME * ((real)cqiSoundEffects[i].volume / 100.0));
        /* this is really the left pan volume.  right = 254 - left */
        cursound.pan = CLAMP(0, 254, (cqiSoundEffects[i].pan + 128));
        cursound.fadeinms = cqiSoundEffects[i].fadeinms;
        cursound.fadeoutms = cqiSoundEffects[i].fadeoutms;

        /* for fx, -1 is infinite, 0 == 1 loop, etc.. */
        if (cqiSoundEffects[i].loops == 0)
            cursound.loops = -1;
        else
            cursound.loops = cqiSoundEffects[i].loops - 1;

        cursound.limit = cqiSoundEffects[i].limit;

        cursound.framelimit = cqiSoundEffects[i].framelimit;
        cursound.lastframe = 0;
        cursound.framecount = 0;

        cursound.delayms = cqiSoundEffects[i].delayms;
        cursound.lasttime = 0;

        cqsEffects.push_back(cursound);
    }

    utLog("%s: Loaded %d Music files.", __FUNCTION__, nummus);
    utLog("%s: Loaded %d Effect files.", __FUNCTION__, numfx);

    return true;
}


/* initialize SDL and the sound stuff. */
void cqsInitSound(void)
{
    const int bufSize = 128;
    char buf[bufSize];
    int i;

    utLog("%s: Initializing...", __FUNCTION__);
    cqsMusic.clear();
    cqsEffects.clear();
    cqsChannels.clear();
    cqsSoundAvailable = false;
    CQS_DISABLE(CQS_EFFECTS | CQS_MUSIC);

    if (!cqiSoundConf || (!cqiSoundEffects && !cqiSoundMusic))
    {
        utLog("%s: cqiSound not initialized, bailing", __FUNCTION__);
        return;
    }

    /* first init SDL */
    if (SDL_Init(SDL_INIT_AUDIO))
    {
        utLog("%s: SDL_Init() failed: %s",
              __FUNCTION__,
              SDL_GetError());
        return;
    }

    /* init the mixer */
    if (Mix_OpenAudio(cqiSoundConf->samplerate,
                      AUDIO_S16SYS,
                      (cqiSoundConf->stereo) ? 2 : 1,
                      cqiSoundConf->chunksize))
    {
        utLog("%s: SDL_OpenAudio() failed: %s",
              __FUNCTION__, Mix_GetError());
        return;
    }

    Mix_AllocateChannels(CQS_MAX_CHANNELS);

    for (i=0; i<CQS_MAX_CHANNELS; i++)
        cqsChannels.push_back( { .channel = -1, .active = false, .idx = -1 });

    /* now loadup the sounds */
    if (!cqsLoadSounds())
    {
        utLog("%s: _cqiLoadSounds() failed, sound disabled",
              __FUNCTION__);
        return;
    }

    /* init all of the team effects and music structures, of course these
       will all silently fail if no music and/or effects were loaded */

    for (i=0; i<NUM_PLAYERTEAMS; i++)
    {
        snprintf(buf, bufSize, "ship%c-phaser", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].phaser = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-torp", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].torp = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-torp3", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].torp3 = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-alert", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].alert = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-beam-down", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].beamd = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-beam-up", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].beamu = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-hit", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].hit = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-info", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].info = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-mag", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].mag = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-warp-up", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].warpu = cqsFindEffect(buf);

        snprintf(buf, bufSize, "ship%c-warp-down", TEAM_PLAYERCHARS[i]);
        cqsTeamEffects[i].warpd = cqsFindEffect(buf);

        /* music */
        snprintf(buf, bufSize, "ship%c-intro", TEAM_PLAYERCHARS[i]);
        cqsTeamMusic[i].intro = cqsFindMusic(buf);

        snprintf(buf, bufSize, "ship%c-battle", TEAM_PLAYERCHARS[i]);
        cqsTeamMusic[i].battle = cqsFindMusic(buf);

        snprintf(buf, bufSize, "ship%c-approach", TEAM_PLAYERCHARS[i]);
        cqsTeamMusic[i].approach = cqsFindMusic(buf);

        snprintf(buf, bufSize, "ship%c-theme", TEAM_PLAYERCHARS[i]);
        cqsTeamMusic[i].theme = cqsFindMusic(buf);
    }

    /* doomsday music */
    snprintf(buf, bufSize, "doomsday");
    cqsDoomsdayMusic.doom = cqsFindMusic(buf);

    snprintf(buf, bufSize, "doomsday-in");
    cqsDoomsdayMusic.doomin = cqsFindMusic(buf);

    snprintf(buf, bufSize, "doomsday-kill");
    cqsDoomsdayMusic.doomkill = cqsFindMusic(buf);

    /* now, enable sound */
    if (cqsEffects.size() || cqsMusic.size())
    {
        cqsSoundAvailable = true;

        if (cqsEffects.size())
            CQS_ENABLE(CQS_EFFECTS);

        if (cqsMusic.size())
            CQS_ENABLE(CQS_MUSIC);

        utLog("%s: samplerate = %d channels = %d chunksize = %d stereo = %s",
              __FUNCTION__,
              cqiSoundConf->samplerate, CQS_MAX_CHANNELS,
              cqiSoundConf->chunksize, (cqiSoundConf->stereo ? "yes" : "no"));
    }

    return;
}

int cqsMusicPlay(int musidx, int halt)
{
    int rv;

    if (!CQS_ISENABLED(CQS_MUSIC) || musidx < 0 || musidx >= cqsMusic.size())
        return false;

#if defined(DEBUG_SOUND)
    utLog("%s: playing %d (%s)", __FUNCTION__,
          musidx, cqiSoundMusic[cqsMusic[musidx].cqiIndex].name);
#endif

    if (Mix_PlayingMusic())
    {
        if (!halt && _mus_idx >= 0 &&
            cqsMusic[_mus_idx].fadeoutms)
        {                   /* need to fade out first */
#if defined(DEBUG_SOUND)
            utLog("%s: queing %d, fadeout %d", __FUNCTION__,
                  musidx, cqsMusic[_mus_idx].fadeoutms);
#endif
            _mus_que = musidx;
            Mix_HookMusicFinished(_music_finished);
            Mix_FadeOutMusic(cqsMusic[_mus_idx].fadeoutms);
            return true;
        }
        else
        {
            Mix_HaltMusic();
            _mus_idx = -1;
        }
    }

    /* we need to fadein or just start */
    Mix_VolumeMusic(MUSIC_VOL(cqsMusic[musidx].vol));
    if (cqsMusic[musidx].fadeinms)
        rv = Mix_FadeInMusic((Mix_Music *)cqsMusic[musidx].chunk,
                             cqsMusic[musidx].loops,
                             cqsMusic[musidx].fadeinms);
    else
        rv = Mix_PlayMusic((Mix_Music *)cqsMusic[musidx].chunk,
                           cqsMusic[musidx].loops);

    if (rv == -1)
    {
        utLog("%s: PlayMusic/FadeInMusic failed: %s", __FUNCTION__,
              Mix_GetError());
        _mus_idx = -1;
        return false;
    }
    else
    {
        _mus_idx = musidx;
    }

    return true;
}

int cqsMusicStop(int halt)
{
    if (!cqsSoundAvailable)
        return false;

#if defined(DEBUG_SOUND)
    utLog("%s: stopping music halt = %d idx %d (fade %d)", __FUNCTION__,
          halt,_mus_idx, cqsMusic[_mus_idx].fadeoutms);
#endif

    _mus_que = -1;                /* turn off the que */

    if (Mix_PlayingMusic())
    {
        if (halt)
            Mix_HaltMusic();
        else
            Mix_FadeOutMusic(cqsMusic[_mus_idx].fadeoutms);
    }

    _mus_idx = -1;

    return true;
}

/* play an effect, returning the cqsHandle if handle is non-NULL */
int cqsEffectPlay(int fxidx, cqsHandle *handle, real maxdist,
                  real dist, real ang)
{
    int i;
    int empty = -1;             /* empty slot we might use */
    int limit = 0;              /* limit we found if effect is limited */

    if (!CQS_ISENABLED(CQS_EFFECTS) || fxidx < 0 || fxidx >= cqsEffects.size())
        return false;

#if defined(DEBUG_SOUND)
    utLog("%s: playing %d (%s) loops %d vol = %d pan = %d  - %f %f %f",
          __FUNCTION__,
          fxidx, cqiSoundEffects[cqsEffects[fxidx].cqiIndex].name,
          cqsEffects[fxidx].loops, cqsEffects[fxidx].vol,
          cqsEffects[fxidx].pan, maxdist, dist, ang);
#endif

    dist = CLAMP(0.0, maxdist, dist);

    /* first, go through the channel array, looking for a possible empty slot
       and counting how many instances of this effect are already playing */
    for (i=0; i<CQS_MAX_CHANNELS; i++)
    {
        if (empty < 0 && !cqsChannels[i].active)
            empty = i;              /* save it */

        if (cqsEffects[fxidx].limit && cqsChannels[i].active &&
            cqsChannels[i].idx == fxidx)
            limit++;
    }

    if (empty < 0)
        return false;               /* no hope */

    if (cqsEffects[fxidx].limit && limit >= cqsEffects[fxidx].limit)
        return false;               /* already playing the limit */

    /* check the delay limit if any */
    /* we make sure that lasttime is non-zero so it will always play
       the first time without delay */
    if (cqsEffects[fxidx].lasttime && cqsEffects[fxidx].delayms &&
        ((frameTime - cqsEffects[fxidx].lasttime) < cqsEffects[fxidx].delayms))
    {
        return false;
    }

    /* see if there's a framelimit */
    if (cqsEffects[fxidx].framelimit)
    {
        if (frameTime != cqsEffects[fxidx].lastframe)
        {
            cqsEffects[fxidx].lastframe = frameTime;
            cqsEffects[fxidx].framecount = 1;
        }
        else
            cqsEffects[fxidx].framecount++;

        if (cqsEffects[fxidx].framecount > cqsEffects[fxidx].framelimit)
            return false;        /* hit the limit */
    }

    /* ok, we are good to go.  Init the channel slot, que the effect
       and setup the callback handler for it. */

    if (handle)
        *handle = (cqsHandle)empty;

    if ((cqsChannels[empty].channel =
         Mix_PlayChannel(-1, (Mix_Chunk *)cqsEffects[fxidx].chunk,
                         cqsEffects[fxidx].loops)) == -1)
    {                           /* some failure */
#ifdef DEBUG_SOUND
        utLog("%s: could not play '%s'", __FUNCTION__,
              cqiSoundEffects[cqsEffects[fxidx].cqiIndex].name);
#endif

        return false;
    }

    cqsChannels[empty].active = true;
    cqsChannels[empty].idx = fxidx;

    cqsEffects[fxidx].lasttime = frameTime;

    Mix_ChannelFinished(_effect_finished);

    /* set default pan if no angle specified */
    if (!ang)
    {
        /* if no panning required, make sure no pan effects are registered */
        if (!cqiSoundEffects[cqsEffects[fxidx].cqiIndex].pan)
            Mix_SetPanning(cqsChannels[empty].channel, 255, 255);
        else
            Mix_SetPanning(cqsChannels[empty].channel,
                           cqsEffects[fxidx].pan,
                           254 - cqsEffects[fxidx].pan);
    }

    Mix_Volume(cqsChannels[empty].channel, EFFECT_VOL(cqsEffects[fxidx].vol));

    /* if dist and maxdist specified, then set it with angle (if specified) */
    if (dist && maxdist)
    {
        Uint8 mdist = (Uint8)(255.0 * (dist / maxdist));
        Sint16 mangle;

        if (!ang)
            mangle = 0;             /* directly in front */
        else
            mangle = (Sint16)utMod360(fabs(360.0 - ang) + 90.0);

        Mix_SetPosition(cqsChannels[empty].channel, mangle, mdist);
    }

    return true;
}

int cqsEffectStop(cqsHandle handle, int halt)
{
    if (!cqsSoundAvailable)
        return false;

#if defined(DEBUG_SOUND)
    utLog("%s: stopping effects halt = %d (hndl %d)", __FUNCTION__,
          halt, handle);
#endif

    if (handle == CQS_INVHANDLE)
    {
        if (halt)
        {                           /* this is a special case, meaning to
                                       immediately halt all channels */
            Mix_HaltChannel(-1);
            return true;
        }

        return false;
    }

    if (handle >= CQS_MAX_CHANNELS ||
        !cqsChannels[handle].active || cqsChannels[handle].idx == -1 ||
        cqsChannels[handle].channel == -1)
        return false;

    if (halt || !cqsEffects[cqsChannels[handle].idx].fadeoutms)
        Mix_HaltChannel(cqsChannels[handle].channel);
    else
        Mix_FadeOutChannel(cqsChannels[handle].channel,
                           cqsEffects[cqsChannels[handle].idx].fadeoutms);

    /* callbacks are registered for all tracked channels, so the
       stop/fadeout should result in cqsChannels being cleaned up when
       finished if it was a tracked effect */

    return true;
}

/* set one (or both) of the volumes */
void cqsUpdateVolume(void)
{
    int i;

    if (!cqsSoundAvailable)
        return;

    /* we simply update the volumes based on the userconf options */

    if (UserConf.musicVol == 0)
    {
        if (CQS_ISENABLED(CQS_MUSIC))
        {
            cqsMusicStop(true);
            CQS_DISABLE(CQS_MUSIC);
        }
    }
    else
    {
        if (!CQS_ISENABLED(CQS_MUSIC) && cqsMusic.size())
            CQS_ENABLE(CQS_MUSIC);
    }

    if (UserConf.effectsVol == 0)
    {
        if (CQS_ISENABLED(CQS_EFFECTS))
        {
            cqsEffectStop(CQS_INVHANDLE, true);
            CQS_DISABLE(CQS_EFFECTS);
        }
    }
    else
    {
        if (!CQS_ISENABLED(CQS_EFFECTS) && cqsEffects.size())
            CQS_ENABLE(CQS_EFFECTS);
    }

    /* for tracked effects, go through each active channel and reset
       the volume */
    for (i=0; i<CQS_MAX_CHANNELS; i++)
    {
        if (cqsChannels[i].active)
            Mix_Volume(cqsChannels[i].channel,
                       EFFECT_VOL(cqsEffects[cqsChannels[i].idx].vol));
    }

    /* set the music volume */
    if (cqsMusic.size() && _mus_idx >= 0)
        Mix_VolumeMusic(MUSIC_VOL(cqsMusic[_mus_idx].vol));

    return;
}


int cqsMusicPlaying(void)
{
    return ((cqsMusic.size() && Mix_PlayingMusic()) ? true : false);
}

#endif /* !CQS_NO_SOUND */
