/* 
 * anim.c - animator, anim API and animQue maintenance
 * 
 * $Id$
 *
 * Copyright 1999-2006 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "conqdef.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "conqcom.h"
#include "conqlb.h"
#include "conqutil.h"
#include "ibuf.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "textures.h"
#include "gldisplay.h"
#include "anim.h"

#include "node.h"
#include "conf.h"
#include "record.h"

/* init an animstate.  can specify an optional istate drawing state */
int animInitState(char *animname, animStatePtr_t astate, 
                  animDrawStatePtr_t istate)
{
  int ndx;

  if (!animname || !astate)
    return FALSE;

  /* first get the animdef index */
  if ((ndx = findGLAnimDef(animname)) < 0)
    {
      utLog("%s: could not find animdef for animation '%s'", 
           __FUNCTION__, animname);
      return FALSE;
    }

    memset((void *)astate, 0, sizeof(animStateRec_t));

    astate->adIndex = ndx;
    astate->flags = 0;          /* not currently used */
    
    if (istate)                 /* initial state override */
      astate->istate = *istate;
    else
      {                         /* if there are cqi istates, use them. */
        if (GLAnimDefs[ndx].istates & AD_ISTATE_TEX)
          astate->istate.id = GLAnimDefs[ndx].itexid;
        else
          {            /* if no texanim, and texid is valid, use it */
            if (GLAnimDefs[ndx].texid)
              astate->istate.id = GLAnimDefs[ndx].texid;
          }

        if (GLAnimDefs[ndx].istates & AD_ISTATE_COL)
          astate->istate.col = GLAnimDefs[ndx].icolor;
        
        if (GLAnimDefs[ndx].istates & AD_ISTATE_SZ)
          astate->istate.size = GLAnimDefs[ndx].isize;

        if (GLAnimDefs[ndx].istates & AD_ISTATE_ANG)
          astate->istate.angle = GLAnimDefs[ndx].iangle;
      }

    /* if no istate was specified, then we will build it up below as we
       handle each section in turn. */

    astate->anims = GLAnimDefs[ndx].anims;
    astate->expired = 0;        /* initialized in animResetState() */

    /* texanim */
    if (astate->anims & CQI_ANIMS_TEX)
      {
        if (!istate && !(GLAnimDefs[ndx].istates & AD_ISTATE_TEX))
          {
            astate->istate.id = GLAnimDefs[ndx].tex.tex[0].id;
            astate->istate.col = GLAnimDefs[ndx].tex.tex[0].col;
          }

        /* setup default s/t offsets */
        astate->istate.tc.s = 0.0;
        astate->istate.tc.t = 0.0;
      }
    
    /* colanim */
    if (astate->anims & CQI_ANIMS_COL)
      {
        if (!istate && !(astate->anims & CQI_ANIMS_TEX) && 
            !(GLAnimDefs[ndx].istates & AD_ISTATE_COL))
          astate->istate.col = GLAnimDefs[ndx].col.color;
      }
    
    /* the rest handled by animResetState(). */
    
    animResetState(astate, frameTime);
    
    return TRUE;
}

/* reset's an anim state.  Hopefully you called animInitState() at least once
   on the state first, before calling this function. 
   When complete, the state is suitable to be run. */
void animResetState(animStatePtr_t astate, Unsgn32 thetime)
{
  int ndx;

  if (!astate)
    return;

  ndx = astate->adIndex;        /* hope it's correct... */

  /* set the starting time in case there is a time limit */
  astate->starttime = thetime;

  /* first reset the draw state from initial draw state - we only copy the
     non private items - explicitly */
  astate->state.id    = astate->istate.id;
  astate->state.tc.s  = astate->istate.tc.s;
  astate->state.tc.t  = astate->istate.tc.t;
  astate->state.col   = astate->istate.col;
  astate->state.x     = astate->istate.x;
  astate->state.y     = astate->istate.y;
  astate->state.z     = astate->istate.z;
  astate->state.angle = astate->istate.angle;
  astate->state.size  = astate->istate.size;

  /* need to check for a negative angle.  If so, gen a
     random one. */
  if (astate->state.angle < 0.0)
    astate->state.angle = rnduni( 0.0, 360.0 );

  /* setup exired so that non-existant anim types are already 'pre-expired' */
  astate->expired = (~(astate->anims & CQI_ANIMS_MASK) & CQI_ANIMS_MASK);

  /* reset the counters, loops, timers, etc */

  /* texanim */
  if (astate->anims & CQI_ANIMS_TEX)
    {
      astate->tex.flags = 0; /* not currently used */
      astate->tex.lasttime = thetime;
      astate->tex.curstage = 0;
      astate->tex.curloop = GLAnimDefs[ndx].tex.loops;
    }
  
  /* colanim */
  if (astate->anims & CQI_ANIMS_COL)
    {
      astate->col.flags = 0;
      astate->col.lasttime = thetime;
      astate->col.curstage = 0;
      astate->col.curloop = GLAnimDefs[ndx].col.loops;
    }

  /* geoanim */
  if (astate->anims & CQI_ANIMS_GEO)
    {
      astate->geo.flags = 0;
      astate->geo.lasttime = thetime;
      astate->geo.curstage = 0;
      astate->geo.curloop = GLAnimDefs[ndx].geo.loops;
    }

  /* toganim */
  if (astate->anims & CQI_ANIMS_TOG)
    {
      astate->tog.flags = 0;
      astate->tog.lasttime = thetime;
    }

  return;
}

/* the guts really.  iterate a state according to it's animdef, possibly
   expiring it */
int animIterState(animStatePtr_t astate)
{
  static real lastfdelay = -1.0; /* last frame delay value (REC/PLYBK) */
  static real timescale = 1.0;
  GLAnimDef_t *glad;

  if (!astate)                  /* shouldn't happen */
    return FALSE;

  glad = &GLAnimDefs[astate->adIndex];

  /* fix up our recFrameDelay in case we are playing back a recording */
  if (Context.recmode == RECMODE_PLAYING || Context.recmode == RECMODE_PAUSED)
    {              /* it changed, re-adjust our timing for playback */
      if (lastfdelay != recFrameDelay)
        {
          lastfdelay = recFrameDelay;
          timescale = (recFrameDelay * 10.0); /* 100ms = 1X */
#if 0
          utLog("SETTING TIMESCALE: recFrameDelay = %f, timescale = %f",
               recFrameDelay, timescale);
#endif
        }
    }
  else
    timescale = 1.0;

  /* check if we have a time limit, if so, and we've exceeded it,
     expire */

  if (glad->timelimit)
    if ((frameTime - astate->starttime) > (glad->timelimit * timescale))
      astate->expired = CQI_ANIMS_MASK; /* expire them all */

  /* texanim */
  if (!(astate->expired & CQI_ANIMS_TEX))
    {
      /* check the time */
      if (!glad->tex.delayms || 
          (frameTime - astate->tex.lasttime) > (int)((real)glad->tex.delayms * timescale))
        {
          astate->tex.lasttime = frameTime;

          if (astate->tex.curstage + 1 >= glad->tex.stages)
            {
              if (glad->tex.loops)
                {
                  if (astate->tex.curloop - 1 <= 0)
                    {               /* we can do no more */
                      astate->expired |= CQI_ANIMS_TEX;
                    }
                  else
                    {
                      astate->tex.curloop--;
                      astate->tex.curstage = 0; /* reset it */
                    }
                }
              else              /* no loops, just reset stage */
                astate->tex.curstage = 0; /* reset it */
            }
          else
            astate->tex.curstage++;

          /* now setup the state */
          if (!(astate->expired & CQI_ANIMS_TEX))
            {
              astate->state.id = glad->tex.tex[astate->tex.curstage].id;
              astate->state.col = glad->tex.tex[astate->tex.curstage].col;

              /* iter tc's */
              astate->state.tc.s += glad->tex.deltas;
              astate->state.tc.t += glad->tex.deltat;
            }
        }
    }

  /* colanim - this can only run if no texanim is running */
  if (!(astate->expired & CQI_ANIMS_COL) && 
      (astate->expired & CQI_ANIMS_TEX))
    {
      /* check the time */
      if (!glad->col.delayms || 
          (frameTime - astate->col.lasttime) > (int)((real)glad->col.delayms * timescale))
        {
          astate->col.lasttime = frameTime;

          if (glad->col.stages && 
              (astate->col.curstage + 1 >= glad->col.stages))
            {
              if (glad->col.loops)
                {
                  if (astate->col.curloop - 1 <= 0)
                    {               /* we can do no more */
                      astate->expired |= CQI_ANIMS_COL;
                    }
                  else
                    {
                      astate->col.curloop--;
                      astate->col.curstage = 0; /* reset it */
                    }
                }
            }
          else
            astate->col.curstage++;

          /* now setup the state */
          if (!(astate->expired & CQI_ANIMS_COL))
            {
              astate->state.col.a = 
                CLAMP(0.0, 1.0, astate->state.col.a + glad->col.deltaa);
              astate->state.col.r = 
                CLAMP(0.0, 1.0, astate->state.col.r + glad->col.deltar);
              astate->state.col.g = 
                CLAMP(0.0, 1.0, astate->state.col.g + glad->col.deltag);
              astate->state.col.b = 
                CLAMP(0.0, 1.0, astate->state.col.b + glad->col.deltab);
            }
        }
    }

  /* geoanim - this can only run if no texanim is running */
  if (!(astate->expired & CQI_ANIMS_GEO) && 
      (astate->expired & CQI_ANIMS_TEX))
    {
      /* check the time */
      if (!glad->geo.delayms || 
          (frameTime - astate->geo.lasttime) > (int)((real)glad->geo.delayms * timescale))
        {
          astate->geo.lasttime = frameTime;

          if (glad->geo.stages && 
              (astate->geo.curstage + 1 >= glad->geo.stages))
            {
              if (glad->geo.loops)
                {
                  if (astate->geo.curloop - 1 <= 0)
                    {               /* we can do no more */
                      astate->expired |= CQI_ANIMS_GEO;
                    }
                  else
                    {
                      astate->geo.curloop--;
                      astate->geo.curstage = 0; /* reset it */
                    }
                }
            }
          else
            astate->geo.curstage++;

          /* now setup the state */
          if (!(astate->expired & CQI_ANIMS_GEO))
            {

              astate->state.x += glad->geo.deltax;
              astate->state.y += glad->geo.deltay;
              astate->state.z += glad->geo.deltaz;
              astate->state.size += glad->geo.deltas;
              if (astate->state.size < 0.0)
                astate->state.size = 0.0;

              /* angle is special */
              if (astate->istate.angle < 0.0)
                astate->state.angle = rnduni( 0.0, 360.0 );
              else
                astate->state.angle = 
                  utMod360(astate->state.angle + glad->geo.deltar);
            }
        }
    }

  /* toganim */
  if (!(astate->expired & CQI_ANIMS_TOG))
    {
      /* check the time */
      /* when playing back at hi speed, it's annoying to have the blinker
         anims accelerated according to timescale.  So, if we are the only
         animtype in this animdef (standard blinker anim) then we do not
         rescale the time according to recFrameDelay.  */
      if (!glad->tog.delayms || 
          (frameTime - astate->tog.lasttime) > 
          (int)((real)glad->tog.delayms * 
                ( ((astate->anims & CQI_ANIMS_MASK) == CQI_ANIMS_TOG) ? 
                 1.0 : timescale) ))
        {
#if 0
          if (glad->tog.delayms == 500)
            utLog("%s: TOG RUN timescale = %f scaledelay = %d", 
                 __FUNCTION__, timescale,
                 (int)((real)glad->tog.delayms * timescale));
#endif
          astate->tog.lasttime = frameTime;

          /* just set the state */
          astate->state.armed = !astate->state.armed; 
        }
    }

  /* indicate whether we expired */
  return (ANIM_EXPIRED(astate) ? TRUE : FALSE);
}

/* Per node Que handling */
void animQueInit(animQue_t *aque)
{
  if (!aque)
    return;

  /* we'll setup the default number of entries to be 32,
     which you will almost certainly reach at some point. */

  aque->maxentries = 32;
  aque->numentries = 0;

  if (!(aque->que = malloc(aque->maxentries * sizeof(animQue_t *))))
    {
      utLog("%s: malloc(%d) failed.", __FUNCTION__, 
           aque->maxentries * sizeof(animQue_t *));
      aque->maxentries = 0;
    }

  /* we'll try to go on.  Maybe a later realloc (on the first
     animQueAdd() will work...) */

  return;
}

/* add a new anim state pointer to the que.  For speed, we do not
   check to see if it is already there first, so be careful. */
void animQueAdd(animQue_t *aque, animStatePtr_t astate)
{
  animStatePtr_t *newlist = NULL;
  static const int increment = 8; /* allocate 8 at a time */

  if (!aque || !astate)
    return;

  if ((aque->numentries + 1) >= aque->maxentries)
    {                           /* need to reallocate more */
      newlist = (animStatePtr_t *)realloc((void *)aque->que, 
                                          sizeof(animStatePtr_t) * 
                                          (aque->maxentries + increment));
      
      if (!newlist)
        {  
          utLog("%s: Could not realloc %d state pointers, ignoring Add.",
               __FUNCTION__,
               sizeof(animStatePtr_t) * (aque->maxentries + increment));
          return;               /* do nothing */
        }

      aque->que = newlist;
      aque->maxentries += increment;
    }

  aque->que[aque->numentries] = astate;
  aque->numentries++;

  return;
}

void animQueDelete(animQue_t *aque, animStatePtr_t astate)
{
  int i;

  if (!aque || !astate)
    return;

  for (i=0; i<aque->numentries; i++)
    if (aque->que[i] == astate)
      {
        aque->numentries--;
        aque->que[i] = aque->que[aque->numentries];
        aque->que[aque->numentries] = NULL;
      }

  return;
}

/* run the list of que entries, removing thos that expire */
void animQueRun(animQue_t *aque)
{
  int i;

  if (!aque || !aque->numentries)
    return;

  for (i=0; i<aque->numentries; i++)
    {
      if (animIterState(aque->que[i]))
        animQueDelete(aque, aque->que[i]); /* it expired, remove it */
    }

  return;
}
