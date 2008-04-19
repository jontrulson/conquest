/* GL.c - OpenGL rendering for Conquest
 *
 * Jon Trulson, 1/2003
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "conqdef.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "conqcom.h"
#include "conqlb.h"
#include "ibuf.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#define NOEXTERN_GLTEXTURES
#include "textures.h"
#undef NOEXTERN_GLTEXTURES
#define NOEXTERN_DCONF
#include "gldisplay.h"
#undef NOEXTERN_DCONF

#define NOEXTERN_GLANIM
#include "anim.h"
#undef NOEXTERN_GLANIM

#include "node.h"
#include "client.h"
#include "conf.h"
#include "cqkeys.h"
#include "cqmouse.h"
#include "record.h"

extern void conqend(void);

#include <assert.h>

#include "ui.h"

#include "glmisc.h"
#include "glfont.h"

#define NOEXTERN_GL
#include "GL.h"
#undef NOEXTERN_GL

#include "conquest.h"

#include "cqsound.h"


/* torp direction tracking */
static real torpdir[MAXSHIPS + 1][MAXTORPS]; 

/* loaded texture list (the list itself is exported in textures.h) */
static int loadedGLTextures = 0; /* count of total successful tex loads */

/* torp animation states */
static animStateRec_t torpAStates[MAXSHIPS + 1][MAXTORPS] = {};
 
/* bomb (torp) animation state */
static animStateRec_t bombAState[MAXSHIPS + 1] = {};
 
dspData_t dData;

int frame=0, timebase=0;
static float FPS = 0.0;

/* a 'prescaling factor for certain objects, so they are drawn a
   little larger than their 'native' CU size */
#define OBJ_PRESCALE        1.3 

#define TEXT_HEIGHT    ((GLfloat)1.75)   /* 7.0/4.0 - text font height
                                            for viewer */
/* from nCP */
extern animStateRec_t ncpTorpAnims[NUMPLAYERTEAMS];

/* global tables for looking up textures quickly */
typedef struct _gl_planet {
  GLTexture_t  *gltex;          /* pointer to the proper GLTexture entry */
  GLint        id;              /* copy of gltex texture id */
  GLColor_t    col;             /* the desired color */
  GLfloat      size;            /* the prefered size, in prescaled CU's*/
} GLPlanet_t;

static GLPlanet_t *GLPlanets = NULL;

/* storage for doomsday tex */
static struct {
  GLint id;                     /* of the machine */
  GLint beamid;                 /* of the anti-proton beam. Oh yes. */
  GLColor_t col;
  GLColor_t beamcol;
} GLDoomsday = {};


/* raw TGA texture data */
typedef struct              
{
  GLubyte	*imageData; 
  GLuint	bpp;        
  GLuint	width;      
  GLuint	height;     
  GLuint	texID;      
} textureImage;             

static void resize(int w, int h);
static void charInput(unsigned char key, int x, int y);
static void input(int key, int x, int y);
static int loadGLTextures(void);
static void renderFrame(void);
static int renderNode(void);

/* It begins... */

float getFPS(void)
{
  return FPS;
}

/* convert an unsigned int color spec (hexcolor in AARRGGBB format) into a
   GLColor */
void hex2GLColor(Unsgn32 hcol, GLColor_t *col)
{
  if (!col)
    return;

  col->a = (1.0 / 256.0) * (double)((hcol & 0xff000000) >> 24);
  col->r = (1.0 / 256.0) * (double)((hcol & 0x00ff0000) >> 16);
  col->g = (1.0 / 256.0) * (double)((hcol & 0x0000ff00) >> 8);
  col->b = (1.0 / 256.0) * (double) (hcol & 0x000000ff);
  return;
}

/* convert a size in 'CU's to GL pixels */
real cu2GLSize(real size, int scale)
{
  GLfloat x, y;

  GLcvtcoords(0.0, 0.0, size, 0.0, scale, &x, &y);
  
  return x;
}


/* search texture list and return index */
int findGLTexture(char *texname)
{
  int i;

  if (!loadedGLTextures || !GLTextures || !cqiNumTextures || !cqiTextures)
    return -1;

  for (i=0; i<loadedGLTextures; i++)
    {
      if (!strncmp(cqiTextures[GLTextures[i].cqiIndex].name, 
                   texname, CQI_NAMELEN))
        return i;
    }
  
  return -1;
}

/* search the cqi animations, and return it's animdef index */
int findGLAnimDef(char *animname)
{
  int i;

  if (!loadedGLTextures || !GLTextures || !cqiNumTextures || !cqiTextures ||
      !GLAnimDefs)
    return -1;

  for (i=0; i<cqiNumAnimations; i++)
    {
      if (!strncmp(cqiAnimations[i].name, 
                   animname, CQI_NAMELEN))
        return cqiAnimations[i].adIndex;
    }
  
  return -1;
}

/* search texture list (by filename) and return index if found */
static int findGLTextureByFile(char *texfile)
{
  int i;

  if (!loadedGLTextures || !GLTextures || !cqiNumTextures || !cqiTextures)
    return -1;

  for (i=0; i<loadedGLTextures; i++)
    {
      if (!strncmp(cqiTextures[GLTextures[i].cqiIndex].filename, 
                   texfile, CQI_NAMELEN))
        return i;
    }
  
  return -1;
}

static int initGLAnimDefs(void)
{
  int i, j;
  int ndx;
  char buffer[CQI_NAMELEN];

  clog("%s: Initializing...", __FUNCTION__);

  /* first, if there's already one present, free it */

  if (GLAnimDefs)
    {
      /* for each one, if there is a texanim, free the tex pointer */
      for (i=0; i<cqiNumAnimDefs; i++)
        if ((GLAnimDefs[i].anims & CQI_ANIMS_TEX) && GLAnimDefs[i].tex.tex)
          free(GLAnimDefs[i].tex.tex);

      free(GLAnimDefs);
      GLAnimDefs = NULL;
    }

  /* allocate enough space */

  if (!(GLAnimDefs = (GLAnimDef_t *)malloc(sizeof(GLAnimDef_t) * 
                                           cqiNumAnimDefs)))
    {
      clog("%s: ERROR: malloc(%d) failed.", __FUNCTION__,
           sizeof(GLAnimDef_t) * cqiNumAnimDefs);

      return FALSE;
    }

  memset((void *)GLAnimDefs, 0, sizeof(GLAnimDef_t) * cqiNumAnimDefs);

  for (i=0; i<cqiNumAnimDefs; i++)
    {
      /* if there is a texname, and no texanim, setup 'default' texid */
      if (cqiAnimDefs[i].texname[0] && !(cqiAnimDefs[i].anims & CQI_ANIMS_TEX))
        {
          if ((ndx = findGLTexture(cqiAnimDefs[i].texname)) >= 0)
            GLAnimDefs[i].texid = GLTextures[ndx].id;
          else
            clog("%s: could not locate texture '%s' for animdef '%s'.", 
                 __FUNCTION__, cqiAnimDefs[i].texname, cqiAnimDefs[i].name);

          /* it may not look pretty, but we will not go fatal here
             if the tex could not be found */
        }

      /* timelimit */
      GLAnimDefs[i].timelimit = cqiAnimDefs[i].timelimit;

      GLAnimDefs[i].anims = cqiAnimDefs[i].anims; /* CQI_ANIMS_* */
          
      /* initial state (istate) */
      GLAnimDefs[i].istates = cqiAnimDefs[i].istates;

      if (GLAnimDefs[i].istates & AD_ISTATE_TEX)
        {                       /* an initial texture was specified. */
          if ((ndx = findGLTexture(cqiAnimDefs[i].itexname)) >= 0)
            GLAnimDefs[i].itexid = GLTextures[ndx].id;
          else
            {
              clog("%s: could not locate istate texture '%s' for animdef '%s'.", 
                   __FUNCTION__, cqiAnimDefs[i].itexname, cqiAnimDefs[i].name);
              GLAnimDefs[i].istates &= ~AD_ISTATE_TEX; /* turn it off */
            }
        }          

      if (GLAnimDefs[i].istates & AD_ISTATE_COL)
        hex2GLColor(cqiAnimDefs[i].icolor, 
                    &GLAnimDefs[i].icolor);
      
      /* copy the CU size */
      GLAnimDefs[i].isize = cqiAnimDefs[i].isize; 

      if (cqiAnimDefs[i].iangle < 0.0) /* neg is special (random)  */
        GLAnimDefs[i].iangle = cqiAnimDefs[i].iangle;
      else
        GLAnimDefs[i].iangle = mod360(cqiAnimDefs[i].iangle);

      /* animation types */

      /* texanim */
      if (cqiAnimDefs[i].anims & CQI_ANIMS_TEX)
        {
          if (cqiAnimDefs[i].texanim.color)
            hex2GLColor(cqiAnimDefs[i].texanim.color, 
                        &GLAnimDefs[i].tex.color);

          GLAnimDefs[i].tex.stages = cqiAnimDefs[i].texanim.stages;
          GLAnimDefs[i].tex.loops = cqiAnimDefs[i].texanim.loops;
          GLAnimDefs[i].tex.delayms = cqiAnimDefs[i].texanim.delayms;
          GLAnimDefs[i].tex.looptype = cqiAnimDefs[i].texanim.looptype;

          GLAnimDefs[i].tex.deltas = cqiAnimDefs[i].texanim.deltas;
          GLAnimDefs[i].tex.deltat = cqiAnimDefs[i].texanim.deltat;

          /* now allocate and build the _anim_texure_ent array */
          if (!(GLAnimDefs[i].tex.tex = 
                (struct _anim_texture_ent *)malloc(sizeof(struct _anim_texture_ent) * GLAnimDefs[i].tex.stages)))
            {
              clog("%s: ERROR: _anim_texture_ent malloc(%d) failed.", 
                   __FUNCTION__,
                   sizeof(struct _anim_texture_ent) * GLAnimDefs[i].tex.stages);

              /* this is fatal */
              return FALSE;
            }

          memset((void *)GLAnimDefs[i].tex.tex, 
                 0, 
                 sizeof(struct _anim_texture_ent) * GLAnimDefs[i].tex.stages);

          /* now setup the texture entry array */

          for (j=0; j<GLAnimDefs[i].tex.stages; j++)
            {
              snprintf(buffer, CQI_NAMELEN - 1, "%s%d", 
                       cqiAnimDefs[i].texname, j);

              if ((ndx = findGLTexture(buffer)) >= 0)
                {
                  GLAnimDefs[i].tex.tex[j].id = GLTextures[ndx].id;

                  if (HAS_GLCOLOR(&GLAnimDefs[i].tex.color))
                    {           /* override per-tex colors */
                      GLAnimDefs[i].tex.tex[j].col = GLAnimDefs[i].tex.color;
                    }
                  else
                    {           /* use texture colors */
                      GLAnimDefs[i].tex.tex[j].col = GLTextures[ndx].col;
                    }
                }
              else
                {
                  clog("%s: could not locate texanim texture '%s' for animdef '%s'.", 
                       __FUNCTION__, buffer, 
                       cqiAnimDefs[i].name);
                  continue;     /* not fatal, just not going to look good
                                   (invisible, probably) */
                }
            }
        } /* texanim */

      /* colanim */
      if (cqiAnimDefs[i].anims & CQI_ANIMS_COL)
        {
          if (cqiAnimDefs[i].colanim.color)
            hex2GLColor(cqiAnimDefs[i].colanim.color, 
                        &GLAnimDefs[i].col.color);
          
          GLAnimDefs[i].col.stages = cqiAnimDefs[i].colanim.stages;
          GLAnimDefs[i].col.loops = cqiAnimDefs[i].colanim.loops;
          GLAnimDefs[i].col.delayms = cqiAnimDefs[i].colanim.delayms;
          GLAnimDefs[i].col.looptype = cqiAnimDefs[i].colanim.looptype;

          GLAnimDefs[i].col.deltaa = cqiAnimDefs[i].colanim.deltaa;
          GLAnimDefs[i].col.deltar = cqiAnimDefs[i].colanim.deltar;
          GLAnimDefs[i].col.deltag = cqiAnimDefs[i].colanim.deltag;
          GLAnimDefs[i].col.deltab = cqiAnimDefs[i].colanim.deltab;

        }
      
      /* geoanim */
      if (cqiAnimDefs[i].anims & CQI_ANIMS_GEO)
        {
          GLAnimDefs[i].geo.stages = cqiAnimDefs[i].geoanim.stages;
          GLAnimDefs[i].geo.loops = cqiAnimDefs[i].geoanim.loops;
          GLAnimDefs[i].geo.delayms = cqiAnimDefs[i].geoanim.delayms;
          GLAnimDefs[i].geo.looptype = cqiAnimDefs[i].geoanim.looptype;

          GLAnimDefs[i].geo.deltax = cqiAnimDefs[i].geoanim.deltax;
          GLAnimDefs[i].geo.deltay = cqiAnimDefs[i].geoanim.deltay;
          GLAnimDefs[i].geo.deltaz = cqiAnimDefs[i].geoanim.deltaz;
          GLAnimDefs[i].geo.deltar = cqiAnimDefs[i].geoanim.deltar;

          /* cqi size delta is specified in CU's. */
          GLAnimDefs[i].geo.deltas = cqiAnimDefs[i].geoanim.deltas;
        }
      
      /* toganim */
      if (cqiAnimDefs[i].anims & CQI_ANIMS_TOG)
          GLAnimDefs[i].tog.delayms = cqiAnimDefs[i].toganim.delayms;

    } /* for */

  return TRUE;
}



/* init the explosion animation states. */
static int initGLExplosions(void)
{
  int i, j;
  animStateRec_t initastate;    /* initial state of an explosion */

  clog("%s: Initializing...", __FUNCTION__);

  /* we only need to do this once.  When we have a properly initted state,
     we will simply copy it into the torpAState array.  */
  if (!animInitState("explosion", &initastate, NULL))
    return FALSE;

  /* we start out expired of course :) */
  initastate.expired = CQI_ANIMS_MASK;

  /* init the anim states for them all */
  for (i=1; i<=MAXSHIPS; i++)
    for (j=0; j<MAXTORPS; j++)
      torpAStates[i][j] = initastate;

  return TRUE;
}

/* utility functions for initGLShips */
static GLint _get_ship_texid(char *name)
{
  int ndx;

  if (!name)
    return 0;

  if ((ndx = findGLTexture(name)) >= 0)
    return GLTextures[ndx].id;
  else
    clog("%s: Could not find texture '%s'", __FUNCTION__, name);

  return 0;
}
static int _get_ship_texcolor(char *name, GLColor_t *col)
{
  int ndx;

  if (!name)
    return -1;

  if ((ndx = findGLTexture(name)) >= 0)
    {
      if (col)
        *col = GLTextures[ndx].col;
    }
  else
    clog("%s: Could not find color '%s'", __FUNCTION__, name);

  return ndx;
}

/* initialize the GLShips array */
static int initGLShips(void)
{
  int i, j;
  char shipPfx[CQI_NAMELEN];
  char buffer[CQI_NAMELEN];

  clog("%s: Initializing...", __FUNCTION__);

  memset((void *)&GLShips, 0, sizeof(GLShips));

  /* for each possible ship, lookup and set all the texids. */
  /* NOTE: I think there are too many... */
  for (i=0; i<NUMPLAYERTEAMS; i++)
    {
      /* build the prefix for this ship */
      snprintf(shipPfx, CQI_NAMELEN - 1, "ship%c",
               Teams[i].name[0]);

      for (j=0; j<MAXNUMSHIPTYPES; j++)
        {

          snprintf(buffer, CQI_NAMELEN - 1, "%s%c%c", shipPfx, 
                   ShipTypes[j].name[0], ShipTypes[j].name[1]);
          GLShips[i][j].ship = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s%c%c-sh", shipPfx, 
                   ShipTypes[j].name[0], ShipTypes[j].name[1]);
          GLShips[i][j].sh = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s-tac", shipPfx);
          GLShips[i][j].tac = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s-phaser", shipPfx);
          GLShips[i][j].phas = _get_ship_texid(buffer);

          snprintf(buffer, CQI_NAMELEN - 1, "%s%c%c-ico", shipPfx, 
                   ShipTypes[j].name[0], ShipTypes[j].name[1]);
          GLShips[i][j].ico = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s%c%c-ico-sh", shipPfx, 
                   ShipTypes[j].name[0], ShipTypes[j].name[1]);
          GLShips[i][j].ico_sh = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s-ico-decal1", shipPfx);
          GLShips[i][j].decal1 = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s-ico-decal2", shipPfx);
          GLShips[i][j].decal2 = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s-dial", shipPfx);
          GLShips[i][j].dial = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s-dialp", shipPfx);
          GLShips[i][j].dialp = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s-warp", shipPfx);
          GLShips[i][j].warp = _get_ship_texid(buffer);

          snprintf(buffer, CQI_NAMELEN - 1, "%s-warp2", shipPfx);
          GLShips[i][j].warp2 = _get_ship_texid(buffer);

          /* here we just want the color */
          snprintf(buffer, CQI_NAMELEN - 1, "%s-warp-col", shipPfx);
          if (_get_ship_texcolor(buffer, &GLShips[i][j].warpq_col) < 0)
            {                   /* do something sane */
              hex2GLColor(0xffeeeeee, &GLShips[i][j].warpq_col);
            }

          snprintf(buffer, CQI_NAMELEN - 1, "%s-ico-cloak", shipPfx);
          GLShips[i][j].ico_cloak = _get_ship_texid(buffer);
          
          snprintf(buffer, CQI_NAMELEN - 1, "%s-ico-repair", shipPfx);
          GLShips[i][j].ico_repair = _get_ship_texid(buffer);
          
          GLShips[i][j].ico_torp = _get_ship_texid("ico-torp");
          
          /* this is ugly... we want to fail if any of these texid's are
             0, indicating the texture wasn't found */
          if (!(GLShips[i][j].ship &&
                GLShips[i][j].sh &&
                GLShips[i][j].tac &&
                GLShips[i][j].phas &&
                GLShips[i][j].ico &&
                GLShips[i][j].ico_sh &&
                GLShips[i][j].decal1 &&
                GLShips[i][j].decal2 &&
                GLShips[i][j].dial &&
                GLShips[i][j].dialp &&
                GLShips[i][j].warp &&
                GLShips[i][j].warp2 &&
                GLShips[i][j].phas &&
                GLShips[i][j].ico_cloak &&
                GLShips[i][j].ico_torp &&
                GLShips[i][j].ico_repair))
            return FALSE;        /* one was missing */
        }
    }
  
  return TRUE;

}

/* figure out the texture, color, and size info for a planet */
static int _get_glplanet_info(GLPlanet_t *curGLPlanet, int plani)
{
  int ndx;
  GLfloat size;
  int gltndx = -1;
  int plnndx = -1;

  if (!GLPlanets)
    return FALSE;

  if (plani < 1 || plani > NUMPLANETS)
    return FALSE;

  /* first find the appropriate texture.  We look for one in this order: 
   *
   * 1. look for a cqiPlanets entry for the planet.  If so, and it
   *    specifies a texture, try to use it if present.  
   * 2. If no cqiPlanets[] entry for the planet, (or the specified
   *    texture was not available) look for a texture named
   *    the same as the planet.  If available, use it.
   * 3. All else failing, then select a default texture (classm, etc)
   *    based on the planet type.
   */
  
  if ((ndx = cqiFindPlanet(Planets[plani].name)) >= 0)
    {                       /* found one. */
      /* save the index for possible color and size setup */
      plnndx = ndx;
      
      /* see if it specified a texture */
      
      if (strlen(cqiPlanets[ndx].texname))
        if ((ndx = findGLTexture(cqiPlanets[ndx].texname)) >= 0)
          gltndx = ndx;     /* yes, and it is present */
    }
  
  /* still no texture? */
  if (gltndx == -1)
    {                       /* see if there is a texture available
                               named after the planet */
      if ((ndx = findGLTexture(Planets[plani].name)) >= 0)
        gltndx = ndx;       /* yes */
    }
  
  /* still no luck? What a loser. */
  if (gltndx == -1)
    {                       /* now we just choose a default */
      switch (Planets[plani].type)
        {
        case PLANET_SUN:
          gltndx = findGLTexture("star");
          break;
          
        case PLANET_CLASSM:
          gltndx = findGLTexture("classm");
          break;
          
        case PLANET_MOON:
          gltndx = findGLTexture("luna");
          break;

        case PLANET_DEAD:
        default:
          gltndx = findGLTexture("classd");
          break;

        }
    }
  
  /* if we still haven't found one, it's time to accept defeat
     and bail... */
  
  if (gltndx == -1)
    {
      clog("%s: ERROR: Unable to locate a texture for planet '%s'.", 
           __FUNCTION__,
           Planets[plani].name);
      
      return FALSE;
    }
  
  /* now we are set, setup initial state for the glplanet */
  
  curGLPlanet->gltex = &GLTextures[gltndx];
  curGLPlanet->id = curGLPlanet->gltex->id; /* a copy of the texid */
  
  /* choose the base color */
  
  /* if the texture has specified a color, use that */
  if (HAS_GLCOLOR(&(curGLPlanet->gltex->col)))
    curGLPlanet->col = curGLPlanet->gltex->col;
  else if (plnndx != -1 && cqiPlanets[plnndx].color)/* cqiPlanet */
    hex2GLColor(cqiPlanets[plnndx].color, &(curGLPlanet->col));
  else
    {                       /* we choose a default */
      switch (Planets[plani].type)
        {
        case PLANET_SUN:    /* default to red for unknown suns */
          hex2GLColor(0xffcc0000, &(curGLPlanet->col)); 
          break;
        default:            /* off white for everything else */
          hex2GLColor(0xffe6e6e6, &(curGLPlanet->col)); 
          break;
        }
    }
  
  /* size - should use server values if available someday, for now use
     cqi, and if not that, the standard defaults */
  
  if (plnndx == -1)
    {
      /* no cqi planet was found so will set a default.  One day the
         server might send this data :) */
      switch (Planets[plani].type)
        {
        case PLANET_SUN:
          size = 1500.0;
          break;
        case PLANET_MOON:
          size = 160.0;
          break;
        default:
          size = 300.0;
          break;
        }
    }
  else
    {                       /* cqi was found, so get it. */
      size = cqiPlanets[plnndx].size * OBJ_PRESCALE;
#if 0
      clog("Computed size %f for planet %s\n",
           size, Planets[plani].name);
#endif
    }
  
  curGLPlanet->size = size;
  
  return TRUE;
}

/* Ala Cataboligne, we will 'directionalize' all torp angles.  */
int uiUpdateTorpDir(int snum, int tnum)
{

  if (snum < 1 || snum > MAXSHIPS)
    return FALSE;

  if (tnum < 0 || tnum >= MAXTORPS)
    return FALSE;

  torpdir[snum][tnum] = angle(0.0, 0.0, 
                              Ships[snum].torps[tnum].dx, 
                              Ships[snum].torps[tnum].dy);
  return TRUE;
}


/* FIXME - implement a 'notify' interface for this kind of thing. */

/* uiUpdatePlanet - this is called by the client when a packet that
   could change the appearance of a planet arrives.  Here, we load the
   GLPlanet struct for the planet, go through the process of
   determining it's texture, color, etc and set up the relevant
   GLPlanet items so that you will see the change.
*/
int uiUpdatePlanet(int plani)
{
  GLPlanet_t *curGLPlanet = NULL;

  if (!GLPlanets)
    return FALSE;

  if (plani < 1 || plani > NUMPLANETS)
    return FALSE;

  curGLPlanet = &GLPlanets[plani - 1];

  return _get_glplanet_info(curGLPlanet, plani);
}


/* initialize the GLPlanets array */
static int initGLPlanets(void)
{
  int i;
  GLPlanet_t curGLPlanet;

  clog("%s: Initializing...", __FUNCTION__);

  /* first, if there's already one present, free it */

  if (GLPlanets)
    {
      free(GLPlanets);
      GLPlanets = NULL;
    }

  /* allocate enough space */

  if (!(GLPlanets = (GLPlanet_t *)malloc(sizeof(GLPlanet_t) * NUMPLANETS)))
    {
      clog("%s: ERROR: malloc(%d) failed.", __FUNCTION__,
           sizeof(GLPlanet_t) * NUMPLANETS);

      return FALSE;
    }

  /* now go through each one, setting up the proper values */
  /* GLPlanets is 0 based, as opposed to CB 1 based Planets[].  */
  for (i=0; i<NUMPLANETS; i++)
    {
      int plani = i + 1;        /* cleaner looking instead of +1 crap? */

      memset((void *)&curGLPlanet, 0, sizeof(GLPlanet_t));

      if (!_get_glplanet_info(&curGLPlanet, plani))
        return FALSE;

      /* we're done, assign it and go on to the next one */
      GLPlanets[i] = curGLPlanet;
    }
  
  return TRUE;
}

/* render a 'decal' for renderHud()  */
void drawIconHUDDecal(GLfloat rx, GLfloat ry, GLfloat w, GLfloat h, 
                  int imgp, cqColor icol)
{
  int steam = Ships[Context.snum].team, stype = Ships[Context.snum].shiptype;
  static int norender = FALSE;
  GLint id = 0;

  if (norender)
    return;

  /* choose the correct texture and render it */

  if (!GLShips[0][0].ship)
    if (!initGLShips())
      {
        clog("%s: initGLShips failed, bailing.", 
             __FUNCTION__);
        norender = TRUE;
        return;                 /* we need to bail here... */
      }

  switch (imgp)
    {
    case HUD_ICO:
      id = GLShips[steam][stype].ico;
      break;
    case HUD_SHI:
      id = GLShips[steam][stype].ico_sh;
      break;
    case HUD_DECAL1:
      id = GLShips[steam][stype].decal1;
      break;
    case HUD_DECAL2:
      id = GLShips[steam][stype].decal2;
      break;
    case HUD_HEAD:
      id = GLShips[steam][stype].dial;
      break;
    case HUD_HDP:    
      id = GLShips[steam][stype].dialp;
      break;
    case HUD_WARP:   
      id = GLShips[steam][stype].warp;
      break;
    case HUD_WARP2:   
      id = GLShips[steam][stype].warp2;
      break;
    case HUD_ICLOAK:   
      id = GLShips[steam][stype].ico_cloak;
      break;
    case HUD_IREPAIR:  
      id = GLShips[steam][stype].ico_repair;
     break;
    default:
      break;
    }
  
  glBindTexture(GL_TEXTURE_2D, id);
  uiPutColor(icol);

  glBegin(GL_POLYGON);

  glTexCoord2f(0.0f, 0.0f);
  glVertex2f(rx, ry);

  glTexCoord2f(1.0f, 0.0f);
  glVertex2f(rx + w, ry);

  glTexCoord2f(1.0f, 1.0f);
  glVertex2f(rx + w, ry + h);

  glTexCoord2f(0.0f, 1.0f);
  glVertex2f(rx, ry + h);

  glEnd();
}

/* return a 'space' buffer for padding */
char *padstr(int l)
{
  static char padding[256 + 1];

  if (l > 256)
    l = 256;

  if (l < 0)
    l = 0;
  
  if (l > 0)
    memset(padding, ' ', l);

  padding[l] = 0;

  return padding;
}

static void mouse(int b, int state, int x, int y)
{
  static mouseData_t mdata;
  Unsgn32 kmod = glutGetModifiers();
  int rv = NODE_OK;
  scrNode_t *node = getTopNode();
  scrNode_t *onode = getTopONode();

#if 0
  clog("%s: b = %d, state = %d x = %d, y = %d", 
       __FUNCTION__,
       b, state, x, y);
#endif

  if (node)
    {
      mdata.x = x;
      mdata.y = y;
      /* pretty glut specific for now, easy to change later */
      mdata.button = b;
      mdata.state = state;
      mdata.mod = 0;

      if (kmod & GLUT_ACTIVE_SHIFT)
        mdata.mod |= CQ_KEY_MOD_SHIFT;
      if (kmod & GLUT_ACTIVE_CTRL)
        mdata.mod |= CQ_KEY_MOD_CTRL;
      if (kmod & GLUT_ACTIVE_ALT)
        mdata.mod |= CQ_KEY_MOD_ALT;
      
      if (mdata.mod)
        mdata.mod = (mdata.mod >> CQ_MODIFIER_SHIFT);

      /* we give input priority to the overlay node */
      if (onode && onode->minput)
        {
          rv = (*onode->minput)(&mdata);
          if (rv == NODE_EXIT)
            {
              conqend();
              exit(1);
            }
        }
      else
        {
          if (node->minput)
            rv = (*node->minput)(&mdata);
          
          if (rv == NODE_EXIT)
            {
              conqend();
              exit(1);
            }
        }
    }

  return;
}

void drawLine(GLfloat x, GLfloat y, GLfloat len, GLfloat lw)
{

#if 0
  clog("drawLine: x = %f, y = %f len = %f",
       x, y, len);
#endif

  glLineWidth(lw);

  glBegin(GL_LINES);
  glVertex3f(x, y, 0.0); /* ul */
  glVertex3f(x + len, y, 0.0); /* ur */
  glEnd();

  return;
}

void drawLineBox(GLfloat x, GLfloat y, 
                 GLfloat w, GLfloat h, int color, 
                 GLfloat lw)
{

#if 0
  clog("drawLineBox: x = %f, y = %f, w = %f, h = %f",
       x, y, w, h);
#endif

  glLineWidth(lw);

  glBegin(GL_LINE_LOOP);
  uiPutColor(color);
  glVertex3f(x, y, 0.0); /* ul */
  glVertex3f(x + w, y, 0.0); /* ur */
  glVertex3f(x + w, y + h, 0.0); /* lr */
  glVertex3f(x, y + h, 0.0); /* ll */
  glEnd();

  return;
}

void drawQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat z)
{
  glBegin(GL_POLYGON);
  glVertex3f(x, y, z); /* ll */
  glVertex3f(x + w, y, z); /* lr */
  glVertex3f(x + w, y + h, z); /* ur */
  glVertex3f(x, y + h, z); /* ul */
  glEnd();

  return;
}

void drawTexQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat z)
{
  glBegin(GL_POLYGON);

  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(x, y, z); /* ll */

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(x + w, y, z); /* lr */

  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(x + w, y + h, z); /* ur */

  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(x, y + h, z); /* ul */

  glEnd();

  return;
}


void drawTexBox(GLfloat x, GLfloat y, GLfloat z, GLfloat size)
{				/* draw textured square centered on x,y
				   USE BETWEEN glBegin/End pair! */
  GLfloat rx, ry;

#ifdef DEBUG
  clog("%s: x = %f, y = %f\n", __FUNCTION__, x, y);
#endif

  rx = x - (size / 2);
  ry = y - (size / 2);

  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(rx, ry, z); /* ll */

  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(rx + size, ry, z); /* lr */

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(rx + size, ry + size, z); /* ur */

  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(rx, ry + size, z); /* ul */

  return;
}

void drawExplosion(GLfloat x, GLfloat y, int snum, int torpnum, int scale)
{
  static int norender = FALSE;
  scrNode_t *curnode = getTopNode();
  static int explodefx = -1;
  GLfloat scaleFac = (scale == SCALE_FAC) ? dConf.vScaleSR : dConf.vScaleLR;
  GLfloat size;

  if (norender)
    return;

  /* just check first ship, torp 0 */
  if (!torpAStates[1][0].anims)
    if (!initGLExplosions())
      {
        clog("%s: initGLExplosions failed, bailing.", 
             __FUNCTION__);
          norender = TRUE;
          return;                 /* we need to bail here... */
      }

  if (explodefx == -1)
    explodefx = cqsFindEffect("explosion");

  /* if it expired and moved, reset and que a new one */
  if (ANIM_EXPIRED(&torpAStates[snum][torpnum]) && 
      (torpAStates[snum][torpnum].state.x != Ships[snum].torps[torpnum].x &&
       torpAStates[snum][torpnum].state.y != Ships[snum].torps[torpnum].y))
    {

      /* start the 'exploding' sound */
      if (cqsSoundAvailable)
        {
          real ang;
          real dis;
          
          ang = angle(Ships[Context.snum].x, Ships[Context.snum].y, 
                      Ships[snum].torps[torpnum].x,
                      Ships[snum].torps[torpnum].y);
          dis = dist(Ships[Context.snum].x, Ships[Context.snum].y,
                     Ships[snum].torps[torpnum].x,
                     Ships[snum].torps[torpnum].y);
          cqsEffectPlay(explodefx, YELLOW_DIST, dis, ang);
        }

      if (curnode->animQue)
        {
          animResetState(&torpAStates[snum][torpnum], frameTime);

          /* we cheat a little by abusing the animstate's x and
             y as per-state storage. This allows us to detect if
             we really should Reset if expired */
          torpAStates[snum][torpnum].state.x = Ships[snum].torps[torpnum].x;
          torpAStates[snum][torpnum].state.y = Ships[snum].torps[torpnum].y;

          animQueAdd(curnode->animQue, &torpAStates[snum][torpnum]);
        }
    }

  size = cu2GLSize(torpAStates[snum][torpnum].state.size, -scale);

  if (scale == MAP_FAC)
    size = size * 2.0;

  glPushMatrix();
  glLoadIdentity();
  
  glScalef(scaleFac, scaleFac, 1.0);
  /* translate to correct position, */
  glTranslatef(x , y , TRANZ);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, torpAStates[snum][torpnum].state.id);
  
  glColor4f(torpAStates[snum][torpnum].state.col.r, 
            torpAStates[snum][torpnum].state.col.g,
            torpAStates[snum][torpnum].state.col.b, 
            torpAStates[snum][torpnum].state.col.a);

  glBegin(GL_POLYGON);
  drawTexBox(0.0, 0.0, 0.0, size);
  glEnd();

  glDisable(GL_TEXTURE_2D); 
  glDisable(GL_BLEND);

  glPopMatrix();

  return;
}

/* draw bombing graphics */
void drawBombing(int snum)
{
  scrNode_t *curnode = getTopNode();
  GLfloat x, y, size;
  int i;
  static animStateRec_t initastate;    /* initial state of a bomb explosion */
  GLfloat scaleFac = dConf.vScaleSR;
  struct _rndxy {               /* anim state private area */
    real rndx;                  /* random X offset from planet */
    real rndy;                  /* random Y offset from planet */
  } *rnd;

  if (snum < 1 || snum > MAXSHIPS)
    return;

  /* don't bother if we aren't orbiting anything */
  if (Ships[snum].lock >= 0)
    return;

  if (SMAP(Context.snum))
    return;                     /* no bombing graphics in LRS */

  /* init - look at first ship */
  if (!bombAState[1].anims)
    {
      if (!animInitState("bombing", &initastate, NULL))
        return;

      /* start out expired */
      initastate.expired = CQI_ANIMS_MASK;

      for (i=1; i <= MAXSHIPS; i++)
        {
          bombAState[i] = initastate;

          /* setup the private area we'll store with the state */
          if (!(rnd = malloc(sizeof(struct _rndxy))))
            {                   /* malloc failure, undo everything */
              int j;

              for (j=1; j < i; j++)
                free(bombAState[j].state.private);
              bombAState[1].anims = 0; /* clear 1st so we can retry
                                          again later */
              clog("%s: malloc(%d) failed", sizeof(struct _rndxy));
              return;
            }
          else
            {
              bombAState[i].state.private = (void *)rnd;
            }
        }
    }

  /* get the state's private area */
  if (!(rnd = (struct _rndxy *)bombAState[snum].state.private))
    return;             /* shouldn't happen */

  /* if it's expired, reset for a new one */
  if (ANIM_EXPIRED(&bombAState[snum]))
    {
      /* reset it, adjust the initial starting conditions, and
         pick a nice place for it. */
      
      if (curnode->animQue)
        {
          animResetState(&bombAState[snum], frameTime);
          
          /* choose a psuedorandom offset from planet x/y, and store
             them in the state's private area. */
          /* FIXME - use the planet size to limit, when available someday */
          rnd->rndx = rnduni(-100.0, 100.0); /* rnd X */
          rnd->rndy = rnduni(-100.0, 100.0); /* rnd Y */
          animQueAdd(curnode->animQue, &bombAState[snum]);
        }

    }

  glPushMatrix();
  glLoadIdentity();

  /* calc and translate to correct position */
  GLcvtcoords( Ships[Context.snum].x, 
               Ships[Context.snum].y, 
               Planets[-Ships[snum].lock].x + rnd->rndx,
               Planets[-Ships[snum].lock].y + rnd->rndy,
               -(SMAP(snum) ? MAP_FAC : SCALE_FAC), 
               &x, 
               &y);

  size = cu2GLSize(bombAState[snum].state.size, -SCALE_FAC);
  
  glScalef(scaleFac, scaleFac, 1.0);

  glTranslatef(x, y, TRANZ);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, bombAState[snum].state.id);
  
  glColor4f(bombAState[snum].state.col.r,
            bombAState[snum].state.col.g,
            bombAState[snum].state.col.b,
            bombAState[snum].state.col.a);

  glBegin(GL_POLYGON);
  drawTexBox(0.0, 0.0, 0.0, size);
  glEnd();

  glDisable(GL_TEXTURE_2D); 
  glDisable(GL_BLEND);

  glPopMatrix();

  return;
}

void uiDrawPlanet( GLfloat x, GLfloat y, int pnum, int scale, 
                  int textcolor, int scanned )
{
  int what;
  GLfloat size;
  char buf[BUFFER_SIZE];
  char torpchar;
  char planame[BUFFER_SIZE];
  int showpnams = UserConf.ShowPlanNames;
  static int norender = FALSE;
  GLfloat scaleFac = (scale == SCALE_FAC) ? dConf.vScaleSR : dConf.vScaleLR;
  if (norender)
    return;

#if 0
  clog("uiDrawPlanet: pnum = %d, x = %.1f, y = %.1f\n",
       pnum, x, y);
#endif

  /* sanity */
  if (pnum < 1 || pnum > NUMPLANETS)
    {
      clog("uiGLdrawPlanet(): invalid pnum = %d", pnum);

      return;
    }

  /* if there's nothing available to render, no point in being here :( */
  if (!GLPlanets)
    if (!initGLPlanets())
      {
        clog("%s: initGLPlanets failed, bailing.", 
             __FUNCTION__);
        norender = TRUE;
        return;                 /* we need to bail here... */
      }

  what = Planets[pnum].type;

  glPushMatrix();
  glLoadIdentity();
  
  glScalef(scaleFac, scaleFac, 1.0);
  
  glEnable(GL_TEXTURE_2D); 

  glBindTexture(GL_TEXTURE_2D, GLPlanets[pnum - 1].id);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  
  glBegin(GL_POLYGON);
  
  glColor4f(GLPlanets[pnum - 1].col.r, GLPlanets[pnum - 1].col.g,
            GLPlanets[pnum - 1].col.b, GLPlanets[pnum - 1].col.a);	

  size = cu2GLSize(GLPlanets[pnum - 1].size, -scale);

  /* so it's more visible... */
  if (scale == MAP_FAC)
    size *= 2.0;
    
  drawTexBox(x, y, TRANZ, size);
  
  glEnd();
  
  glDisable(GL_TEXTURE_2D); 

  /*  text data... */
  glBlendFunc(GL_ONE, GL_ONE);

  if (Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON ||
      !Planets[pnum].scanned[Ships[Context.snum].team] )
    torpchar = ' ';
  else
    if ( Planets[pnum].armies <= 0 || Planets[pnum].team < 0 || 
         Planets[pnum].team >= NUMPLAYERTEAMS )
      torpchar = '-';
    else
      torpchar = Teams[Planets[pnum].team].torpchar;

  if (scale == SCALE_FAC)
    {
      if (showpnams)
        {
          if (UserConf.DoNumMap && (torpchar != ' '))
            snprintf(buf, BUFFER_SIZE - 1, "#%d#%c#%d#%d#%d#%c%s", 
                     textcolor,
                     torpchar,
                     InfoColor,
                     Planets[pnum].armies,
                     textcolor,
                     torpchar,
                     Planets[pnum].name);
          else
            snprintf(buf, BUFFER_SIZE - 1, "%s", Planets[pnum].name);


          glfRender(x, 
                    ((scale == SCALE_FAC) ? y - 4.0 : y - 1.0), 
                    TRANZ, /* planet's Z */
                    ((GLfloat)uiCStrlen(buf) * 2.0) / ((scale == SCALE_FAC) ? 1.0 : 2.0), 
                    TEXT_HEIGHT, fontTinyFixedTxf, buf, textcolor, NULL,
                    TRUE, TRUE, FALSE);
        }
    }
  else
    {                           /* MAP_FAC */
      if (showpnams)
        {                       /* just want first 3 chars */
          planame[0] = Planets[pnum].name[0];
          planame[1] = Planets[pnum].name[1];
          planame[2] = Planets[pnum].name[2];
          planame[3] = EOS;
        }
      else
        planame[0] = EOS;

      if (UserConf.DoNumMap && (torpchar != ' '))
        snprintf(buf, BUFFER_SIZE - 1, "#%d#%c#%d#%d#%d#%c%s", 
                 textcolor,
                 torpchar,
                 InfoColor,
                 Planets[pnum].armies,
                 textcolor,
                 torpchar,
                 planame);
      else
        snprintf(buf, BUFFER_SIZE - 1, "#%d#%c#%d#%c#%d#%c%s", 
                 textcolor,
                 torpchar,
                 InfoColor,
                 ConqInfo->chrplanets[Planets[pnum].type],
                 textcolor,
                 torpchar,
                 planame);

      glfRender(x, 
                ((scale == SCALE_FAC) ? y - 2.0 : y - 1.0), 
                TRANZ,  
                ((GLfloat)uiCStrlen(buf) * 2.0) / ((scale == SCALE_FAC) ? 1.0 : 2.0), 
                TEXT_HEIGHT, fontTinyFixedTxf, buf, textcolor, NULL,
                TRUE, TRUE, FALSE);

    }

  glDisable(GL_BLEND); 

  glPopMatrix();

  return;
  
}


/* convert 'CU' coordinates into GL coordinates.  Account for LR/SR
 *  'baseline' scaling as well as magfactor scaling.  If 'scale' is
 *  negative, then we do not scale the GL x/y coordinates according to
 *  the current magfactor, but we must always scale the limits with
 *  the inverse magfactor, so limit checking will work correctly.  You
 *  will typically only want to do this when the caller will be
 *  responsible for proper magfactor scaling during drawing (using
 *  glScale for example).
 */
int GLcvtcoords(real cenx, real ceny, real x, real y, real scale,
		 GLfloat *rx, GLfloat *ry )
{
  GLfloat rscale;
  static const GLfloat fuzz = 1.3; /* 'fuzz' factor to pad the limit
                                      a little */
  int ascale = abs(scale);
  GLfloat limit;
  GLfloat vscale;
  GLfloat magscale; 

  magscale = (ascale == SCALE_FAC) ? dConf.vScaleSR : dConf.vScaleLR;

  /* if scale is negative, do not scale the x/y with the current
     magfactor */
  if (scale < 0.0)
    vscale = 1.0;
  else
    vscale = magscale;

  /* determine number of CU's that could be seen vertically in LR or
     SR. (no magscale scaling) */
  rscale = ((GLfloat)DISPLAY_LINS * ascale / (VIEWANGLE * 2.0));

  /* we must always scale the limit, regardless of whether X/Y are
     being scaled by vscale.  We multiply the VIEWANGLE with fuzz to
     allow a little leeyway to the limit, so objects are less likely
     to just appear/disappear at the edges of the viewer, at the cost
     of potentially rendering an object that can't be seen. */
  limit = ((VIEWANGLE * fuzz) * (1.0 / magscale)); 

  *rx = ((x-cenx) / rscale) * vscale; 
  *ry = ((y-ceny) / rscale) * vscale; 

#if 0
  clog("GLCVTCOORDS: rscale = %f limit = %f cx = %.2f, cy = %.2f, \n\tx = %.2f, y = %.2f, glx = %.2f,"
       " gly = %.2f \n",
       rscale, limit, cenx, ceny, x, y, *rx, *ry);
#endif

  if (*rx < -limit || *rx > limit)
    {
      return FALSE;
    }
  if (*ry < -limit || *ry > limit)
    {
      return FALSE;
    }

  return TRUE; 
}

/* ship currently being viewed during playback */
void setRecId(char *str)
{
  if (str)
    strcpy(dData.recId.str, str);
  else
    dData.recId.str[0] = EOS;

  return;
}

/* ship currently being viewed during playback */
void setRecTime(char *str)
{
  if (str)
    strcpy(dData.recTime.str, str);
  else
    dData.recTime.str[0] = EOS;

  return;
}

void setXtraInfo(void)
{
  int l = sizeof(dData.xtrainfo.str);

  snprintf(dData.xtrainfo.str, l,
          "#%d#FA:#%d#%3d #%d#TA/D:#%d#%3s#%d#:#%d#%3d#%d#/#%d#%5d",
          LabelColor,
          InfoColor,
          (int)Ships[Context.snum].lastblast,
          LabelColor,
          SpecialColor,
          Context.lasttarg,
          LabelColor,
          InfoColor,
          Context.lasttang,
          LabelColor,
          InfoColor,
          Context.lasttdist);

  dData.xtrainfo.str[l - 1] = 0;

  return;
}

void setHeading(char *heading)
{
  int l = sizeof(dData.heading.heading);
  strncpy(dData.heading.heading, heading, 
          l - 1);
  dData.heading.heading[l - 1] = 0;

  return;
}

#define WARP_UP     0
#define WARP_DOWN   1

void setWarp(real warp)
{
  static cqsHandle warpHandle = CQS_INVHANDLE;
  static cqsHandle engineHandle = CQS_INVHANDLE;
  static const int l = sizeof(dData.warp.warp);
  static int warpufx = -1;
  static int warpdfx = -1;
  static int enginefx = -1;
  real dwarp = Ships[Context.snum].dwarp;
  static int lastwarpdir = -1;
  int warpdir;
  static real lastwarp = 0;
  static char buf[128];

  if (warpufx == -1)
    {
      snprintf(buf, 128 - 1, "ship%c-warp-up", 
               Teams[Ships[Context.snum].team].name[0]);
      warpufx = cqsFindEffect(buf);
    }

  if (warpdfx == -1)
    {
      snprintf(buf, 128 - 1, "ship%c-warp-down", 
               Teams[Ships[Context.snum].team].name[0]);
      warpdfx = cqsFindEffect(buf);
    }

  if (enginefx == -1)
    {
      enginefx = cqsFindEffect("engines");
    }

  if (warp >= 0)
    snprintf(dData.warp.warp, l - 1, "%2.1f", warp );
  else
    strncpy(dData.warp.warp, "Orbit", l - 1);

  dData.warp.warp[l - 1] = 0;

  /* first, the engine sounds */
  if (warp > 0)
    {
      if (engineHandle == CQS_INVHANDLE)
        {                       /* start it */
          cqsEffectPlayTracked(enginefx, &engineHandle, 0.0, 0.0, 0.0);
        }
    }
  else
    {                           /* stop it */
      if (engineHandle != CQS_INVHANDLE)
        {             
          cqsEffectStop(engineHandle, FALSE);
          engineHandle = CQS_INVHANDLE;
        }
    }
          

  /* figure out where we are heading */
#if 0
  clog ("warp = %f, dwarp %f lastwarp %f", warp, dwarp, lastwarp);
#endif

  if (warp == dwarp || warp <= 0.0 || dwarp < 0 || 
      warp == maxwarp(Context.snum))
    {                           /* we are where we want to be */
      if (warpHandle != CQS_INVHANDLE)
        cqsEffectStop(warpHandle, FALSE);
      warpHandle = CQS_INVHANDLE;
      lastwarpdir = -1;
      lastwarp = warp;
      return;
    }

  if (warp < dwarp)
    {
      /* we need to do an extra check here for entering orbit */
      if (warp < lastwarp) /* we're really decelerating into orbit */
        warpdir = WARP_DOWN;
      else
        warpdir = WARP_UP;
    }
  else
    warpdir = WARP_DOWN;

  lastwarp = warp;

  if (warpHandle != CQS_INVHANDLE)
    {                           /* we are still playing one */
      /* we need to see if the direction has changed,
         if so, we need to stop the current effect and start the
         'other one' */

      if (warpdir != lastwarpdir)
        {
          cqsEffectStop(warpHandle, FALSE);
          warpHandle = CQS_INVHANDLE;
          
          if (warpdir == WARP_UP)
            cqsEffectPlayTracked(warpufx, &warpHandle, 0.0, 0.0, 0.0);
          else if (warpdir == WARP_DOWN)
            cqsEffectPlayTracked(warpdfx, &warpHandle, 0.0, 0.0, 0.0);

          lastwarpdir = warpdir;
        }

      return;
    }   
  else
    {                           /* we need to start one */
      if (warpdir == WARP_UP)
        cqsEffectPlayTracked(warpufx, &warpHandle, 0.0, 0.0, 0.0);
      else if (warpdir == WARP_DOWN)
        cqsEffectPlayTracked(warpdfx, &warpHandle, 0.0, 0.0, 0.0);
      
      lastwarpdir = warpdir;
    }

  return;
}

void setKills(char *kills)
{
  int l = sizeof(dData.kills.kills);

  strncpy(dData.kills.kills, kills,
          l - 1);
  dData.kills.kills[l - 1] = 0;
  
  return;
}

void setFuel(int fuel, int color)
{

  dData.fuel.fuel = fuel;
  dData.fuel.color = color;
  dData.fuel.lcolor = color;

  return;
}

void setAlertLabel(char *buf, int color)
{
  int l = sizeof(dData.aStat.alertStatus);

  strncpy(dData.aStat.alertStatus, buf, l - 1);
  dData.aStat.alertStatus[l - 1] = 0;
  dData.aStat.color = color;

  return;
}

void setShields(int shields, int color)
{
  int l = sizeof(dData.sh.label);

  dData.sh.shields = shields;
  dData.sh.color = color;
  dData.sh.lcolor = color;

  if (shields == -1)
    strncpy(dData.sh.label, "Shields D", l - 1);
  else
    strncpy(dData.sh.label, "Shields U", l - 1);

  return;
}

void setAlloc(int w, int e, char *alloc)
{
  int l = sizeof(dData.alloc.allocstr);

  snprintf(dData.alloc.allocstr, l - 1, "%5s", alloc);
  dData.alloc.allocstr[l - 1] = 0;

  dData.alloc.walloc = w;
  dData.alloc.ealloc = e;

  return;
}

void setTemp(int etemp, int ecolor, int wtemp, int wcolor, 
             int efuse, int wfuse)
{
  if (etemp > 100)
    etemp = 100;
  if (wtemp > 100)
    wtemp = 100;

  dData.etemp.etemp = etemp;
  dData.etemp.color = ecolor;
  dData.etemp.lcolor = ecolor;
  if (efuse > 0)
    dData.etemp.overl = TRUE;
  else
    dData.etemp.overl = FALSE;

  dData.wtemp.wtemp = wtemp;
  dData.wtemp.color = wcolor;
  dData.wtemp.lcolor = wcolor;
  if (wfuse > 0)
    dData.wtemp.overl = TRUE;
  else
    dData.wtemp.overl = FALSE;

  return;
}

void setDamage(int dam, int color)
{
  dData.dam.damage = dam;
  dData.dam.color = color;
  dData.dam.lcolor = color;

  return;
}

void setDamageLabel(char *buf, int color)
{
  int l = sizeof(dData.dam.label);

  strncpy(dData.dam.label, buf, l - 1);
  dData.dam.label[l - 1] = 0;

  dData.dam.lcolor = color;

  return;
}

void setArmies(char *labelbuf, char *buf)
{				/* this also displays robot actions... */
  int l = sizeof(dData.armies.str);

  snprintf(dData.armies.str, l - 1, "%s%s", labelbuf, buf);
  dData.armies.str[l - 1] = 0;
  
  return;
}  

void setTow(char *buf)
{
  int l = sizeof(dData.tow.str);

  strncpy(dData.tow.str, buf, l - 1);
  dData.tow.str[l - 1] = 0;

  return;
}

void setCloakDestruct(char *buf, int color)
{
  int l = sizeof(dData.cloakdest.str);

  strncpy(dData.cloakdest.str, buf, l - 1);
  dData.cloakdest.str[l - 1] = 0;
  dData.cloakdest.color = color;

  return;
}

void setAlertBorder(int color)
{
  dData.aBorder.alertColor = color;

  return;
}

/* a shortcut */
void clrPrompt(int line)
{
  setPrompt(line, NULL, NoColor, NULL, NoColor);

  return;
}

void setPrompt(int line, char *prompt, int pcolor, 
               char *buf, int color)
{
  int l = sizeof(dData.p1.str); 
  char *str;
  char *pstr;
  int pl;
  char *bstr;
  int bl;
  const int maxwidth = 80;

  switch(line)
    {
    case MSG_LIN1:
      str = dData.p1.str;
      break;

    case MSG_LIN2:
      str = dData.p2.str;
      break;

    case MSG_MSG:
    default:
      color = InfoColor;
      str = dData.msg.str;
      break;
    }

  if (!buf && !prompt)
    {
      strcpy(str, "");
      return;
    }

  if (!buf)
    {
      bl = 0;
      bstr = "";
    }
  else
    {
      bl = strlen(buf);
      bstr = buf;
    }

  if (!prompt)
    {
      pl = 0;
      pstr = "";
    }
  else
    {
      pl = strlen(prompt);
      pstr = prompt;
    }

  snprintf(str, l,
           "#%d#%s#%d#%s%s",
           pcolor, pstr, color, bstr, padstr(maxwidth - (pl + bl)));

  return;
}


void dspInitData(void)
{
  memset((void *)&dConf, 0, sizeof(dspConfig_t));

  dConf.inited = False;

  dConf.fullScreen = FALSE;
  dConf.initWidth = 1024;
  dConf.initHeight = 768;

  dConf.wX = dConf.wY = 0;
  dConf.vScaleLR = dConf.vScaleSR = 1.0;

  memset((void *)&dData, 0, sizeof(dspData_t));

  strcpy(dData.warp.label, "Warp");
  strcpy(dData.heading.label, "Head");

  strcpy(dData.kills.label, "Kills =");
  dData.kills.lcolor = SpecialColor;

  strcpy(dData.sh.label, "Shields D");
  dData.sh.lcolor = GreenLevelColor;
  strcpy(dData.dam.label, "Damage  ");
  dData.dam.lcolor = GreenLevelColor;
  strcpy(dData.fuel.label, "Fuel    ");
  dData.fuel.lcolor = GreenLevelColor;
  strcpy(dData.alloc.label, "W   Alloc   E");
  strcpy(dData.etemp.label, "E Temp   ");
  dData.etemp.color = GreenLevelColor;
  dData.etemp.lcolor = GreenLevelColor;
  strcpy(dData.wtemp.label, "W Temp   ");
  dData.wtemp.color = GreenLevelColor;
  dData.wtemp.lcolor = GreenLevelColor;

  dConf.inited = False;

  return;
}
  
int uiGLInit(int *argc, char **argv)
{
#ifdef DEBUG_GL
  clog("uiGLInit: ENTER");
#endif

  glutInit(argc, argv);
  glutInitDisplayMode(/*GLUT_DEPTH |*/ GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);

  glutInitWindowPosition(0,0);

  glutInitWindowSize(dConf.initWidth, dConf.initHeight);
  
  dConf.mainw = glutCreateWindow(CONQUESTGL_NAME);

  if(dConf.fullScreen)
    glutFullScreen();

  glutKeyboardFunc       (charInput);
  glutSpecialFunc        (input);
  glutMouseFunc          (mouse);
  glutPassiveMotionFunc  (NULL);
  glutMotionFunc         (NULL);
  glutDisplayFunc        (renderFrame);
  glutIdleFunc           (renderFrame);
  glutReshapeFunc        (resize);
  glutEntryFunc          (NULL);
  glutSetCursor          (GLUT_CURSOR_CROSSHAIR);

  return 0;             
}

/* this function was borrowed (with slight edits) from Brian Paul's
 * xglinfo.c program.
 */
static void
_print_gl_info(void)
{
  struct token_name {
    GLuint count;
    GLenum token;
    const char *name;
  };
  static const struct token_name limits[] = {
#if defined(GL_MAX_ATTRIB_STACK_DEPTH)
    { 1, GL_MAX_ATTRIB_STACK_DEPTH, "GL_MAX_ATTRIB_STACK_DEPTH" },
#endif
#if defined(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH)
    { 1, GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, "GL_MAX_CLIENT_ATTRIB_STACK_DEPTH" },
#endif
#if defined(GL_MAX_CLIP_PLANES)
    { 1, GL_MAX_CLIP_PLANES, "GL_MAX_CLIP_PLANES" },
#endif
#if defined(GL_MAX_COLOR_MATRIX_STACK_DEPTH)
    { 1, GL_MAX_COLOR_MATRIX_STACK_DEPTH, "GL_MAX_COLOR_MATRIX_STACK_DEPTH" },
#endif
#if defined(GL_MAX_ELEMENTS_VERTICES)
    { 1, GL_MAX_ELEMENTS_VERTICES, "GL_MAX_ELEMENTS_VERTICES" },
#endif
#if defined(GL_MAX_ELEMENTS_INDICES)
    { 1, GL_MAX_ELEMENTS_INDICES, "GL_MAX_ELEMENTS_INDICES" },
#endif
#if defined(GL_MAX_EVAL_ORDER)
    { 1, GL_MAX_EVAL_ORDER, "GL_MAX_EVAL_ORDER" },
#endif
#if defined(GL_MAX_LIGHTS)
    { 1, GL_MAX_LIGHTS, "GL_MAX_LIGHTS" },
#endif
#if defined(GL_MAX_LIST_NESTING)
    { 1, GL_MAX_LIST_NESTING, "GL_MAX_LIST_NESTING" },
#endif
#if defined(GL_MAX_MODELVIEW_STACK_DEPTH)
    { 1, GL_MAX_MODELVIEW_STACK_DEPTH, "GL_MAX_MODELVIEW_STACK_DEPTH" },
#endif
#if defined(GL_MAX_NAME_STACK_DEPTH)
    { 1, GL_MAX_NAME_STACK_DEPTH, "GL_MAX_NAME_STACK_DEPTH" },
#endif
#if defined(GL_MAX_PIXEL_MAP_TABLE)
    { 1, GL_MAX_PIXEL_MAP_TABLE, "GL_MAX_PIXEL_MAP_TABLE" },
#endif
#if defined(GL_MAX_PROJECTION_STACK_DEPTH)
    { 1, GL_MAX_PROJECTION_STACK_DEPTH, "GL_MAX_PROJECTION_STACK_DEPTH" },
#endif
#if defined(GL_MAX_TEXTURE_STACK_DEPTH)
    { 1, GL_MAX_TEXTURE_STACK_DEPTH, "GL_MAX_TEXTURE_STACK_DEPTH" },
#endif
#if defined(GL_MAX_TEXTURE_SIZE)
    { 1, GL_MAX_TEXTURE_SIZE, "GL_MAX_TEXTURE_SIZE" },
#endif
#if defined(GL_MAX_3D_TEXTURE_SIZE)
    { 1, GL_MAX_3D_TEXTURE_SIZE, "GL_MAX_3D_TEXTURE_SIZE" },
#endif
#if defined(GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB)
    { 1, GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB" },
#endif
#if defined(GL_MAX_RECTANGLE_TEXTURE_SIZE_NV)
    { 1, GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, "GL_MAX_RECTANGLE_TEXTURE_SIZE_NV" },
#endif
#if defined(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB)
    { 1, GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, "GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB" },
#endif
#if defined(GL_MAX_TEXTURE_UNITS_ARB)
    { 1, GL_MAX_TEXTURE_UNITS_ARB, "GL_MAX_TEXTURE_UNITS_ARB" },
#endif
#if defined(GL_MAX_TEXTURE_LOD_BIAS_EXT)
    { 1, GL_MAX_TEXTURE_LOD_BIAS_EXT, "GL_MAX_TEXTURE_LOD_BIAS_EXT" },
#endif
#if defined(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)
    { 1, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT" },
#endif
#if defined(GL_MAX_VIEWPORT_DIMS)
    { 2, GL_MAX_VIEWPORT_DIMS, "GL_MAX_VIEWPORT_DIMS" },
#endif
#if defined(GL_ALIASED_LINE_WIDTH_RANGE)
    { 2, GL_ALIASED_LINE_WIDTH_RANGE, "GL_ALIASED_LINE_WIDTH_RANGE" },
#endif
#if defined(GL_SMOOTH_LINE_WIDTH_RANGE)
    { 2, GL_SMOOTH_LINE_WIDTH_RANGE, "GL_SMOOTH_LINE_WIDTH_RANGE" },
#endif
#if defined(GL_ALIASED_POINT_SIZE_RANGE)
    { 2, GL_ALIASED_POINT_SIZE_RANGE, "GL_ALIASED_POINT_SIZE_RANGE" },
#endif
#if defined(GL_SMOOTH_POINT_SIZE_RANGE)
    { 2, GL_SMOOTH_POINT_SIZE_RANGE, "GL_SMOOTH_POINT_SIZE_RANGE" },
#endif
    { 0, (GLenum) 0, NULL }
  };

  GLint i, max[2];
  const char *glVendor   = (const char *) glGetString(GL_VENDOR);
  const char *glRenderer = (const char *) glGetString(GL_RENDERER);
  const char *glVersion  = (const char *) glGetString(GL_VERSION);
  
  clog("graphicsInit: OpenGL Vendor:      %s", glVendor);
  clog("graphicsInit: OpenGL Renderer:    %s", glRenderer);
  clog("graphicsInit: OpenGL Version:     %s", glVersion);

  if (cqDebug)
    {
      clog("graphicsInit: OpenGL limits:");

      for (i = 0; limits[i].count; i++) 
        {
          glGetIntegerv(limits[i].token, max);
          if (glGetError() == GL_NONE) 
            {
              if (limits[i].count == 1)
                clog("    %s = %d", limits[i].name, max[0]);
              else /* XXX fix if we ever query something with more than 2 values */
                clog("    %s = %d, %d", limits[i].name, max[0], max[1]);
            }
        }
    }

  return;
}

void graphicsInit(void)
{
  glClearDepth(1.0);
  glClearColor(0.0, 0.0, 0.0, 0.0);  /* clear to black */

  glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  _print_gl_info();

  if (!loadGLTextures())
    clog("ERROR: loadGLTextures() failed\n");
  else
    {
      if (!initGLAnimDefs())
        clog("ERROR: initGLAnimDefs() failed\n");
    }

  return;
}

static void
resize(int w, int h)
{
  static int minit = FALSE;

  if (!minit)
    {
      minit = TRUE;
      graphicsInit();
      initTexFonts();
      GLError();
    }

  dConf.wW = (GLfloat)w;
  dConf.wH = (GLfloat)h;
  dConf.mAspect = (GLfloat)w/(GLfloat)h;
  
  /* calculate the border width */
  dConf.borderW = ((dConf.wW * 0.01) + (dConf.wH * 0.01)) / 2.0;
  
  /* calculate viewer geometry */
  dConf.vX = dConf.wX + (dConf.wW * 0.30); /* x + 30% */
  dConf.vY = dConf.wY + dConf.borderW;
  dConf.vW = (dConf.wW - dConf.vX) - dConf.borderW;
  dConf.vH = (dConf.wH - (dConf.wH * 0.20)); /* y + 20% */

  dConf.vAspect = dConf.vW/dConf.vH;
  
#ifdef DEBUG_GL
  clog("GL: RESIZE: WIN = %d w = %d h = %d, \n"
       "    vX = %f, vY = %f, vW = %f, vH = %f",
       glutGetWindow(), w, h, 
       dConf.vX, dConf.vY, dConf.vW, dConf.vH);
#endif

  /* we will pretend we have an 80x25 char display for
     the 'text' nodes. we account for the border area too */
  dConf.ppRow = (dConf.wH - (dConf.borderW * 2.0)) / 25.0;
  dConf.ppCol = (dConf.wW - (dConf.borderW * 2.0)) / 80.0;
  
  glViewport(0, 0, w, h);
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0.0, (GLdouble)w, 0.0, (GLdouble)h);
  
  /* invert the y axis, down is positive */
  glScalef(1.0, -1.0, 1.0);
  
  /*  move the origin from the bottom left corner */
  /*  to the upper left corner */
  glTranslatef(0.0, -dConf.wH, 0.0);
  
  /* save a copy of this matrix */
  glGetFloatv(GL_PROJECTION_MATRIX, dConf.hmat);

  /* viewer */
  
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(VIEWANGLE, dConf.vW / dConf.vH, 
                 1.0, 1000.0);

  /* save a copy of this matrix */
  glGetFloatv(GL_PROJECTION_MATRIX, dConf.vmat);

  /* restore hmat */
  glLoadMatrixf(dConf.hmat);

  glMatrixMode(GL_MODELVIEW);
  
  dConf.inited = True;

  glutPostRedisplay();
  
  return;
}

static int renderNode(void)
{
  scrNode_t *node = getTopNode();
  scrNode_t *onode = getTopONode();
  int rv;

  if (node)
    {
      if (node->display)
        {
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          rv = (*node->display)(&dConf);

          if (rv == NODE_EXIT)
            return rv;

          if (node->animQue)
            animQueRun(node->animQue);

          if (onode && onode->display)
            {
              rv = (*onode->display)(&dConf);
          
              if (rv == NODE_EXIT)
                return rv;

              if (onode->animQue)
                animQueRun(onode->animQue);
            }

          glutSwapBuffers();

        }

      if (node->idle)
        {
          rv = (*node->idle)();
          if (rv == NODE_EXIT)
            return rv;
        }

      /* send a udp keep alive if it's time */
      sendUDPKeepAlive(frameTime);

      if (onode && onode->idle)
        {
          rv = (*onode->idle)();
          if (rv == NODE_EXIT)
            return rv;
        }
    }

  return NODE_OK;
}

static void renderFrame(void)
{				/* assumes context is current*/
  int rv = NODE_OK;

  /* don't render anything until we are ready */
  if (!dConf.inited)
    return;

  /* get FPS */
  frame++;
  frameTime = clbGetMillis();
  if (frameTime - timebase > 1000) 
    {
      FPS = (frame*1000.0/(frameTime-timebase));
      timebase = frameTime;
      frame = 0;
    }

  if (getTopNode())
    {
      rv = renderNode();

      if (rv == NODE_EXIT)
        {
          conqend();
          clog("EXITING!");
          exit(1);
        }
    }

  /* if we are playing back a recording, we use the current
     frame delay, else the default throttle */

  if (frameDelay != 0.0)
    {
      if (Context.recmode == RECMODE_PLAYING)
        c_sleep(frameDelay);
      else
        {
          if (FPS > 75.0)               
            c_sleep(0.01);
        }
    }

  return;

}


void uiPrintFixed(GLfloat x, GLfloat y, GLfloat w, GLfloat h, char *str)
{                               /* this works for non-viewer only */
  glfRender(x, y, 0.0, w, h, fontFixedTxf, str, NoColor, NULL, 
            TRUE, TRUE, TRUE);

  return;
}

void drawTorp(GLfloat x, GLfloat y, char torpchar, int torpcolor, 
              int scale, int snum, int torpnum)
{
  static const GLfloat z = 1.0;
  GLfloat size, sizeh;
  int steam = Ships[snum].team;
  GLfloat scaleFac = (scale == SCALE_FAC) ? dConf.vScaleSR : dConf.vScaleLR;

  /* these need to exist first... */
  if (!GLShips[0][0].ship)
    if (!initGLShips())
      {
        clog("%s: initGLShips failed.", 
             __FUNCTION__);
        return;                 /* we need to bail here... */
      }

  /* we only draw torps for 'valid teams' */
  if (steam < 0 || steam >= NUMPLAYERTEAMS)
    return;

  size = cu2GLSize(ncpTorpAnims[steam].state.size, -scale);

  if (scale == MAP_FAC)
    size = size * 2.0;

  glPushMatrix();
  glLoadIdentity();

  glScalef(scaleFac, scaleFac, 1.0);

  glTranslatef(x , y , TRANZ);

  if (ncpTorpAnims[steam].state.angle) /* use it */
    glRotatef((GLfloat)ncpTorpAnims[steam].state.angle, 0.0, 0.0, z);  
  else
    glRotatef((GLfloat)torpdir[snum][torpnum], 
              0.0, 0.0, z);  /* face firing angle */

  sizeh = (size / 2.0);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);

  glEnable(GL_TEXTURE_2D); 
  glBindTexture(GL_TEXTURE_2D, ncpTorpAnims[steam].state.id); 

  glBegin(GL_POLYGON);		

  uiPutColor(torpcolor |CQC_A_BOLD);

  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(-sizeh, -sizeh, z); /* ll */

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(sizeh, -sizeh, z); /* lr */

  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(sizeh, sizeh, z); /* ur */

  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(-sizeh, sizeh, z); /* ul */

  glEnd();

  glDisable(GL_TEXTURE_2D); 
  glDisable(GL_BLEND);

  glPopMatrix();

  return;
}

/* utility func for converting a shield level into a color */
static inline Unsgn32 _get_sh_color(real sh)
{
  Unsgn32 color;
  Unsgn8 alpha;

  if (sh >= 0.0 && sh < 50.0)
    {
      alpha = (Unsgn8)(256.0 * (sh/50.0)); 
      color = RedLevelColor | (alpha << CQC_ALPHA_SHIFT);
    }
  else if (sh >= 50.0 && sh <= 80.0)
    color = YellowLevelColor;
  else 
    color = GreenLevelColor;
  
  return color;
}    


void
drawShip(GLfloat x, GLfloat y, GLfloat angle, char ch, int snum, int color,
	 GLfloat scale)
{
  char buf[16];
  GLfloat alpha = 1.0;
  static const GLfloat z = 1.0;
  GLfloat size;
  GLfloat sizeh;
  static GLfloat shipsizeSR, shipsizeLR;
  static int norender = FALSE;
  int steam = Ships[snum].team, stype = Ships[snum].shiptype;
  GLfloat scaleFac = (scale == SCALE_FAC) ? dConf.vScaleSR : dConf.vScaleLR;
  static int firsttime = TRUE;
  static GLfloat phaseradius;

  if (norender)
    return;

  /* if there's nothing available to render, no point in being here :( */
  if (!GLShips[0][0].ship)
    if (!initGLShips())
      {
        clog("%s: initGLShips failed, bailing.", 
             __FUNCTION__);
        norender = TRUE;
        return;                 /* we need to bail here... */
      }

  if (firsttime)
    {                         /* firsttime things */
      firsttime = FALSE;
      /* setup the ship sizes - ships are SHIPSIZE CU's. */
      shipsizeSR = cu2GLSize(SHIPSIZE * OBJ_PRESCALE, -SCALE_FAC);
      shipsizeLR = cu2GLSize(SHIPSIZE * OBJ_PRESCALE, -MAP_FAC);

      /* can only see phaser in SR, so... */
      phaseradius = cu2GLSize(PHASER_DIST, -SCALE_FAC);
    }

  size = ((scale == SCALE_FAC) ? shipsizeSR : shipsizeLR);

  /* make a little more visible in LR */
  if (scale == MAP_FAC) 
    size = size * 2.0; 

  sizeh = size / 2.0;

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  /* phasers - we draw this before the ship */
  if (((scale == SCALE_FAC) && Ships[snum].pfuse > 0)) /* phaser action */
    {
      static const GLfloat phaserwidth = 1.5;

      glPushMatrix();
      glLoadIdentity();

      glScalef(scaleFac, scaleFac, 1.0);

      /* translate to correct position, */
      glTranslatef(x , y , TRANZ);
      glRotatef(Ships[snum].lastphase - 90.0, 0.0, 0.0, z);

      glColor4f(1.0, 1.0, 1.0, 1.0);
      glEnable(GL_TEXTURE_2D); 

      glBindTexture(GL_TEXTURE_2D, GLShips[steam][stype].phas);
      glBegin(GL_POLYGON);
      
      glTexCoord2f(1.0f, 0.0f);
      glVertex3f(-phaserwidth, 0.0, -1.0); /* ll */
      
      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(phaserwidth, 0.0, -1.0); /* lr */
      
      glColor4f(1.0, 1.0, 1.0, 0.3);
      glTexCoord2f(0.0f, 1.0f);
      glVertex3f(phaserwidth, phaseradius, -1.0); /* ur */
      
      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(-phaserwidth, phaseradius, -1.0); /* ul */
      
      glEnd();
      
      glDisable(GL_TEXTURE_2D);   
      glPopMatrix();
    }

  /*
   *  Cataboligne - shield visual
   */

  if (UserConf.DoShields && SSHUP(snum) && !SREPAIR(snum))  
    {             /* user opt, shield up, not repairing */
      glPushMatrix();
      glLoadIdentity();

      glScalef(scaleFac, scaleFac, 1.0);
      
      glTranslatef(x , y , TRANZ);
      glRotatef(angle, 0.0, 0.0, z);
      
      glEnable(GL_TEXTURE_2D);
      
      glBindTexture(GL_TEXTURE_2D, GLShips[steam][stype].sh);


      /* standard sh graphic */
      uiPutColor(_get_sh_color(Ships[snum].shields));
      glBegin(GL_POLYGON);
      
      glTexCoord2f(1.0f, 0.0f);
      glVertex3f(-size, -size, z); /* ll */
      
      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(size, -size, z); /* lr */
      
      glTexCoord2f(0.0f, 1.0f);
      glVertex3f(size, size, z); /* ur */
      
      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(-size, size, z); /* ul */
      
      glEnd();

      glDisable(GL_TEXTURE_2D);
      glPopMatrix();
    }

  sprintf(buf, "%c%d", ch, snum);

  /* set a lower alpha if we are cloaked. */
  if (ch == '~')
    alpha = 0.4;		/* semi-transparent */

#if 0 && defined( DEBUG_GL )
  clog("DRAWSHIP(%s) x = %.1f, y = %.1f, ang = %.1f\n", buf, x, y, angle);
#endif

  glPushMatrix();
  glLoadIdentity();

  glEnable(GL_TEXTURE_2D); 

  glBindTexture(GL_TEXTURE_2D, GLShips[steam][stype].ship);
  
  glScalef(scaleFac, scaleFac, 1.0);

  /* translate to correct position, */
  glTranslatef(x , y , TRANZ);

  /* THEN rotate ;-) */
  glRotatef(angle, 0.0, 0.0, z);

  glColor4f(1.0, 1.0, 1.0, alpha);	

  glBegin(GL_POLYGON);

  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(-sizeh, -sizeh, z); /* ll */

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(sizeh, -sizeh, z); /* lr */

  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(sizeh, sizeh, z); /* ur */

  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(-sizeh, sizeh, z); /* ul */

  glEnd();

  glDisable(GL_TEXTURE_2D);   
  
  glBlendFunc(GL_ONE, GL_ONE);

  /* highlight enemy ships... */
  if (UserConf.EnemyShipBox)
    if (color == RedLevelColor || color == RedColor)
      drawLineBox(-sizeh, -sizeh, size, size, RedColor, 1.0);

  /* reset the matrix for the text */
  glLoadIdentity();

  glScalef(scaleFac, scaleFac, 1.0);

  glfRender(x, 
            ((scale == SCALE_FAC) ? y - 4.0: y - 1.0), 
            TRANZ,  
            ((GLfloat)strlen(buf) * 2.0) / ((scale == SCALE_FAC) ? 1.0 : 2.0), 
            TEXT_HEIGHT, fontTinyFixedTxf, buf, color, NULL,
            TRUE, FALSE, FALSE);

  glPopMatrix();

  glDisable(GL_BLEND);

  return;
}

void drawDoomsday(GLfloat x, GLfloat y, GLfloat dangle, GLfloat scale)
{
  static const GLfloat z = 1.0;
  static GLfloat doomsizeSR, doomsizeLR;
  GLfloat size;  
  GLfloat sizeh;
  static GLfloat beamradius;
  static int norender = FALSE;  /* if no tex, no point... */
  static real ox = 0.0, oy = 0.0; /* for doomsday weapon antiproton beam */
  static int drawAPBeam = TRUE;
  static animStateRec_t doomapfire = {}; /* animdef state for ap firing */
  static int last_apstate;      /* toggle this when it expires */
  static int beamfx = -1;       /* Cataboligne - beam sound */
  static const Unsgn32 beamfx_delay = 1000; /* 1 second */
  static Unsgn32 lastbeam = 0;
  real dis, ang;
  GLfloat scaleFac = (scale == SCALE_FAC) ? dConf.vScaleSR : dConf.vScaleLR;
  static int firsttime = TRUE;

  if (norender)
    return;

  dis = dist( Ships[Context.snum].x, Ships[Context.snum].y, 
              Doomsday->x, Doomsday->y );
  
  ang = angle(Ships[Context.snum].x, Ships[Context.snum].y,  
              Doomsday->x, Doomsday->y); 

  /* find the textures if we haven't already */
  if (!GLDoomsday.id)
    {                           /* init first time around */
      int ndx = findGLTexture("doomsday");

      if (ndx >= 0)
        {
          GLDoomsday.id = GLTextures[ndx].id;
          if (HAS_GLCOLOR(&GLTextures[ndx].col))
            GLDoomsday.col = GLTextures[ndx].col;
          else
            hex2GLColor(0xffffffff, &GLDoomsday.col);
        }
      else
        {
          clog("%s: Could not find the doomsday texture,  bailing.", 
               __FUNCTION__);
          norender = TRUE;
          return;
        }

      /* find the AP beam */
      ndx = findGLTexture("doombeam");
      if (ndx >= 0)
        {
          GLDoomsday.beamid = GLTextures[ndx].id;
          
          if (HAS_GLCOLOR(&GLTextures[ndx].col))
            GLDoomsday.beamcol = GLTextures[ndx].col;
          else
            hex2GLColor(0xffffffff, &GLDoomsday.beamcol);
        }
      else
        {
          clog("%s: Could not find the doombeam texture,  bailing.", 
               __FUNCTION__);
          norender = TRUE;
          return;
        }

      /* init and startup the doom ap blinker */
      if (animInitState("doomsday-ap-fire", &doomapfire, NULL))
        {
          scrNode_t *node = getTopNode();
          animQueAdd(node->animQue, &doomapfire);
        }

      if (beamfx == -1) 
        beamfx = cqsFindEffect("doomsday-beam");
    }        

  if (firsttime)
    {
      firsttime = FALSE;

      /* doomsday is DOOMSIZE CU's in size */
      doomsizeSR = cu2GLSize(DOOMSIZE * OBJ_PRESCALE, -SCALE_FAC);
      doomsizeLR = cu2GLSize(DOOMSIZE * OBJ_PRESCALE, -MAP_FAC);

      beamradius = cu2GLSize(PHASER_DIST * 1.333, -SCALE_FAC);
    }

  size = ((scale == SCALE_FAC) ? doomsizeSR : doomsizeLR);

  if (scale == MAP_FAC)
    size = size * 2.0;

  sizeh = size / 2.0;

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

#ifdef DEBUG
  clog("DRAWDOOMSDAY(%s) x = %.1f, y = %.1f, ang = %.1f\n", buf, x, y, dangle);
#endif

  /*
    Cataboligne - fire doomsday antiproton beam!
  */

  if (last_apstate != doomapfire.state.armed)
    {
      last_apstate = doomapfire.state.armed;

      /* we only want to draw it if we think it's stationary */
      if (ox == Doomsday->x && oy == Doomsday->y)
        drawAPBeam = !drawAPBeam;
      else
        {
          drawAPBeam = FALSE;
          ox = Doomsday->x;
          oy = Doomsday->y;
        }          
    }

  /* if it's time to draw the beam, then let her rip */
  if ( drawAPBeam && (scale == SCALE_FAC))
    {
      static const GLfloat beamwidth = 3.0;

      glPushMatrix();
      glLoadIdentity();
      
      glScalef(scaleFac, scaleFac, 1.0);

      glTranslatef(x , y , TRANZ);
      glRotatef(Doomsday->heading - 90.0, 0.0, 0.0, z);
      
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, GLDoomsday.beamid);
      
      glColor4f(GLDoomsday.beamcol.r, 
                GLDoomsday.beamcol.g,
                GLDoomsday.beamcol.b,
                GLDoomsday.beamcol.a);
      
      glBegin(GL_POLYGON);
      
      glTexCoord2f(1.0f, 0.0f);
      glVertex3f(-beamwidth, 0.0, -1.0); /* ll */
      
      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(beamwidth, 0.0, -1.0); /* lr */
      
      glColor4f(GLDoomsday.beamcol.r, 
                GLDoomsday.beamcol.g,
                GLDoomsday.beamcol.b,
                GLDoomsday.beamcol.a * 0.1);
      
      glTexCoord2f(0.0f, 1.0f);
      glVertex3f(beamwidth / 2.0, beamradius, -1.0); /* ur */
      
      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(-(beamwidth / 2.0), beamradius, -1.0); /* ul */
      
      glEnd();
      
      glDisable(GL_TEXTURE_2D);
      glPopMatrix();

      /*
        Cataboligne - sound code 11.16.6
        play doombeam sound
      */
      if (dis < YELLOW_DIST && ((frameTime - lastbeam) > beamfx_delay))
        {
          cqsEffectPlay(beamfx, YELLOW_DIST * 2, dis, ang);
          lastbeam = frameTime;
        }
  }  /* drawAPBeam */

  glPushMatrix();
  glLoadIdentity();

  glScalef(scaleFac, scaleFac, 1.0);

  glEnable(GL_TEXTURE_2D); 

  glBindTexture(GL_TEXTURE_2D, GLDoomsday.id);
  
  glTranslatef(x , y , TRANZ);
  glRotatef(dangle, 0.0, 0.0, z);

  glColor4f(GLDoomsday.col.r, GLDoomsday.col.g, 
            GLDoomsday.col.b, GLDoomsday.col.a);	

  glBegin(GL_POLYGON);

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(-sizeh, -sizeh, z); /* ll */

  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(sizeh, -sizeh, z); /* lr */

  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(sizeh, sizeh, z); /* ur */

  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(-sizeh, sizeh, z); /* ul */

  glEnd();

  glDisable(GL_TEXTURE_2D);   

  glPopMatrix();

  glDisable(GL_BLEND);

  return;
}

/* (maybe) draw the Negative Energy Barrier */
/* this routine is a bit more complicated than the original neb rendering,
   however it's a lot faster and 'works' better by providing more
   accurate scaling and motion in relation to the ship.  In addition,
   if you can't see it, we won't waste time rendering it.

   The neb is rendered as a series (maximum of 2) barrier 'walls' that
   are only drawn when they would be in view.  They are scaled
   appropriately and drawn at the ship's depth plane.  The old neb
   renderer would draw 1 large quad underneath the VBG, which made the
   perspective wrong.  This also required that the neb quad be
   rendered all the time, even if you couldn't see it.

   First we determine what quadrant we are in.  From there we plot two
   imaginary points aligned with the ship's x/y coordinates, and
   placed on the nearest wall(s) edge.  Then a determination is made
   (using a fixed GLcvtcoords()) to determine whether that point would
   be visible.

   If so, then the appropriate X and/or Y walls are drawn.  At most
   only 2 walls can ever be displayed (if you are in view of a
   'corner' of the barrier for example).

   We save alot of cycles by not rendering what we don't need to,
   unlike the original neb rendering, which was done all the time,
   regardless of barrier visibility.
*/
void drawNEB(int snum)
{
  int nebXVisible = FALSE;
  int nebYVisible = FALSE;
  static int inited = FALSE;
  static const GLfloat nebCenter = ((NEGENBEND_DIST - NEGENB_DIST) / 2.0);
  real nearx, neary;
  GLfloat tx, ty;
  GLfloat nebWidth, nebHeight;
  GLfloat nebWidthSR, nebHeightSR;
  GLfloat nebWidthLR, nebHeightLR;
  GLfloat nebX, nebY;
  static int norender = FALSE;
  /* our wall animation state */
  static animStateRec_t nebastate;    /* initial state of neb texture */


  if (norender)
    return;

  if (!inited)
    {
      inited = TRUE;

      /* get our anim */
      if (!animInitState("neb", &nebastate, NULL))
        {
          norender = TRUE;
          return;
        }
    }

  /* figure out appropriate width/height of neb quad in SR/LR */
  /* width/height SR */
  GLcvtcoords(0.0, 0.0, NEGENBEND_DIST * 2.0, 
              (NEGENBEND_DIST - NEGENB_DIST), 
              SCALE_FAC,
              &nebWidthSR, &nebHeightSR);
  
  /* width/height LR */
  GLcvtcoords(0.0, 0.0, NEGENBEND_DIST * 2.0, 
              (NEGENBEND_DIST - NEGENB_DIST), 
              MAP_FAC,
              &nebWidthLR, &nebHeightLR);

  /* see if a neb wall is actually visible.  If not, we can save alot of
     cycles... */

  /* first, if we're LR and !UserConf.DoLocalLRScan, then you
     can't see it */

  if (SMAP(snum) && !UserConf.DoLocalLRScan)
    return;

  /* if we are inside the barrier. of course we
     can see it */
   if (fabs( Ships[snum].x ) >= NEGENB_DIST && 
       fabs( Ships[snum].x ) <= NEGENBEND_DIST)
     nebXVisible = TRUE;
     
   if (fabs( Ships[snum].y ) >= NEGENB_DIST && 
       fabs( Ships[snum].y ) <= NEGENBEND_DIST)
     nebYVisible = TRUE;

   if (!nebXVisible)
     {
       /* test for x wall */
       if (Ships[snum].x < 0.0)
         {
           /* find out which side of the barrier we are on */
           if (Ships[snum].x > (-NEGENB_DIST - nebCenter))
             nearx = -NEGENB_DIST;
           else
             nearx = -NEGENBEND_DIST;
         }
       else
         {
           if (Ships[snum].x < (NEGENB_DIST + nebCenter))
             nearx = NEGENB_DIST;
           else
             nearx = NEGENBEND_DIST;
         }
       
       /* we check against a mythical Y point aligned on the nearest X
          NEB wall edge to test for visibility. */
       
       if (GLcvtcoords(Ships[snum].x, Ships[snum].y,
                       nearx, 
                       CLAMP(-NEGENBEND_DIST, NEGENBEND_DIST, Ships[snum].y), 
                       (SMAP(snum) ? MAP_FAC : SCALE_FAC),
                       &tx, &ty))
         {
           nebXVisible = TRUE;
         }
     }  

#if 0                           /* debugging test point (murisak) */
  uiDrawPlanet( tx, ty, 34, (SMAP(snum) ? MAP_FAC : SCALE_FAC), 
                             MagentaColor, TRUE);
#endif

  if (!nebYVisible)
    {
      /* test for y wall */
      if (Ships[snum].y < 0.0)
        {
          if (Ships[snum].y > (-NEGENB_DIST - nebCenter))
            neary = -NEGENB_DIST;
          else
            neary = -NEGENBEND_DIST;
        }
      else
        {
          if (Ships[snum].y < (NEGENB_DIST + nebCenter))
            neary = NEGENB_DIST;
          else
            neary = NEGENBEND_DIST;
        }
      
      /* we check against a mythical X point aligned on the nearest Y NEB wall
         edge to test for visibility. */
      
      if (GLcvtcoords(Ships[snum].x, Ships[snum].y,
                      CLAMP(-NEGENBEND_DIST, NEGENBEND_DIST, Ships[snum].x), 
                      neary, 
                      (SMAP(snum) ? MAP_FAC : SCALE_FAC),
                      &tx, &ty))
        {
          nebYVisible = TRUE;
        }
    }
#if 0                           /* debugging test point (murisak) */
  uiDrawPlanet( tx, ty, 34, (SMAP(snum) ? MAP_FAC : SCALE_FAC), 
                             MagentaColor, TRUE);
#endif

  /* nothing to see here */
  if (!nebXVisible && !nebYVisible)
    return;

  /* here we iterate the neb anim state 'manually'.  We do not que
   *   this in the normal animQue, since we only want to run it when
   *   this function is executed, rather than all the time.  Saves cycles.
   */

  animIterState(&nebastate);

  /* draw it/them */

  glPushMatrix();
  glLoadIdentity();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, nebastate.state.id);

  nebWidth = (SMAP(snum) ? nebWidthLR : nebWidthSR);
  nebHeight = (SMAP(snum) ? nebHeightLR : nebHeightSR);

  glColor4f(nebastate.state.col.r,
            nebastate.state.col.g,
            nebastate.state.col.b,
            nebastate.state.col.a);

  if (nebYVisible)
    {
      if (Ships[snum].y > 0.0)
        {
          /* top */
          GLcvtcoords(Ships[snum].x, Ships[snum].y,
                      -NEGENBEND_DIST, NEGENB_DIST, 
                      (SMAP(snum) ? MAP_FAC : SCALE_FAC),
                      &nebX, &nebY);
          /*           clog("Y TOP VISIBLE"); */
        }
      else
        {
          /* bottom */
          GLcvtcoords(Ships[snum].x, Ships[snum].y,
                      -NEGENBEND_DIST, -NEGENBEND_DIST, 
                      (SMAP(snum) ? MAP_FAC : SCALE_FAC),
                      &nebX, &nebY);
          /*           clog("Y BOTTOM VISIBLE"); */
        }
      
      /* draw the Y neb wall */

      glBegin(GL_POLYGON);
      
      glTexCoord2f(0.0 + nebastate.state.tc.s, 
                   0.0 + nebastate.state.tc.t);
      glVertex3f(nebX, nebY, TRANZ); /* ll */
      
      glTexCoord2f(1.0 + nebastate.state.tc.s, 
                   0.0 + nebastate.state.tc.t);
      glVertex3f(nebX + nebWidth, nebY, TRANZ); /* lr */
      
      glTexCoord2f(1.0 + nebastate.state.tc.s, 
                   1.0 + nebastate.state.tc.t);
      glVertex3f(nebX + nebWidth, nebY + nebHeight, TRANZ); /* ur */
      
      glTexCoord2f(0.0 + nebastate.state.tc.s, 
                   1.0 + nebastate.state.tc.t);
      glVertex3f(nebX, nebY + nebHeight, TRANZ); /* ul */
      
      glEnd();
    }

  if (nebXVisible)
    {
      if (Ships[snum].x > 0.0)
        {
          /* right */
          GLcvtcoords(Ships[snum].x, Ships[snum].y,
                      NEGENB_DIST, -NEGENBEND_DIST, 
                      (SMAP(snum) ? MAP_FAC : SCALE_FAC),
                      &nebX, &nebY);
          /*           clog("X RIGHT VISIBLE"); */
        }
      else
        {
          /* left */
          GLcvtcoords(Ships[snum].x, Ships[snum].y,
                      -NEGENBEND_DIST, -NEGENBEND_DIST, 
                      (SMAP(snum) ? MAP_FAC : SCALE_FAC),
                      &nebX, &nebY);
          /*           clog("X LEFT VISIBLE"); */
        }

      /* draw the X neb wall */

      /* like we drew the Y neb wall, but since we play games by swapping
         the w/h (since we are drawing the quad 'on it's side') we need
         to swap the texcoords as well so things don't look squashed.  */
      glBegin(GL_POLYGON);
      
      glTexCoord2f(0.0 + nebastate.state.tc.s, 
                   1.0 + nebastate.state.tc.t);
      glVertex3f(nebX, nebY, TRANZ); /* ll */
      
      glTexCoord2f(0.0 + nebastate.state.tc.s, 
                   0.0 + nebastate.state.tc.t);
      glVertex3f(nebX + nebHeight, nebY, TRANZ); /* lr */
      
      glTexCoord2f(1.0 + nebastate.state.tc.s, 
                   0.0 + nebastate.state.tc.t);
      glVertex3f(nebX + nebHeight, nebY + nebWidth, TRANZ); /* ur */
      
      glTexCoord2f(1.0 + nebastate.state.tc.s, 
                   1.0 + nebastate.state.tc.t);
      glVertex3f(nebX, nebY + nebWidth, TRANZ); /* ul */
      
      glEnd();
    }

  glDisable(GL_TEXTURE_2D); 
  glDisable(GL_BLEND);
  
  glPopMatrix();

  return;
}

void drawViewerBG(int snum, int dovbg)
{
  /* depth of VBG, seems about right :) */
  static const GLfloat z = TRANZ * 1.65; 

  /* star field inside barrier - the galaxy */
  GLfloat sizeb = (VIEWANGLE * (20.0 + 14)) / 2.0;

  GLfloat x, y, x2, y2, rx, ry;
  static GLint texid_vbg = 0;
  GLfloat scaleFac = (SMAP(snum)) ? dConf.vScaleLR : dConf.vScaleSR;

  /* half-width of vbg at TRANZ */
  static const GLfloat vbgrad = NEGENBEND_DIST * 1.2; 

  if (snum < 1 || snum > MAXSHIPS)
    return;

  if (!GLTextures || !GLShips[0][0].ship)
    return;

      /* try to init them */
  if (!texid_vbg)
    {
      int ndx;

      if ((ndx = findGLTexture("vbg")) >= 0)
        texid_vbg = GLTextures[ndx].id;
      else
        {
          texid_vbg = 0;
          return;
        }
    }

  if (!dovbg && !(UserConf.DoTacBkg && SMAP(snum)))
    return;
  
  if (SMAP(snum) && !UserConf.DoLocalLRScan)
    {                           /* murisak centered LR */
      GLcvtcoords(0.0, 0.0,
                  -vbgrad, -vbgrad, 
                  -SFAC(snum),
                  &x, &y);
      GLcvtcoords(0.0, 0.0,
                  vbgrad, vbgrad, 
                  -SFAC(snum),
                  &x2, &y2);
    }
  else
    {                           /* everything else */
      GLcvtcoords(Ships[snum].x, Ships[snum].y,
                  -vbgrad, -vbgrad, 
                  -SFAC(snum),
                  &x, &y);
      GLcvtcoords(Ships[snum].x, Ships[snum].y,
                  vbgrad, vbgrad, 
                  -SFAC(snum),
                  &x2, &y2);
    }

  glPushMatrix();
  glLoadIdentity();

  glScalef(scaleFac, scaleFac, 1.0); 

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  
  glEnable(GL_TEXTURE_2D);

  if (dovbg)
    {
      glBindTexture(GL_TEXTURE_2D, texid_vbg); 
      
      glColor3f(0.8, 0.8, 0.8);	
      
      glBegin(GL_POLYGON);
      
      glTexCoord2f(0.0f, 1.0f);
      glVertex3f(x, y, z); /* ll */
      
      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(x2, y, z); /* lr */
      
      glTexCoord2f(1.0f, 0.0f);
      glVertex3f(x2, y2, z); /* ur */
      
      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(x, y2, z); /* ul */

      glEnd();
    }
  
  if (UserConf.DoTacBkg && SMAP(snum))  
    {                           /* draw tac? */
      glLoadIdentity();
      
      glTranslatef(0.0, 0.0, -1.0);
      
      glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      
      sizeb = 0.990;
      
      glBindTexture(GL_TEXTURE_2D, 
                    GLShips[Ships[snum].team][Ships[snum].shiptype].tac);

      glColor4f(1.0, 1.0, 1.0, UserConf.DoTacShade/100.0);

      
      glBegin(GL_POLYGON);
      
      rx = ry = 0.0 - (sizeb / 2.0);
      
      glTexCoord2f(0.0f, 0.0f);
      glVertex3f(rx, ry, 0.0); 
      
      glTexCoord2f(1.0f, 0.0f);
      glVertex3f(rx + sizeb, ry, 0.0); 
      
      glTexCoord2f(1.0f, 1.0f);
      glVertex3f(rx + sizeb, ry + sizeb, 0.0); 
      
      glTexCoord2f(0.0f, 1.0f);
      glVertex3f(rx, ry + sizeb, 0.0); 
      
      glEnd();
    }

  glDisable(GL_TEXTURE_2D); 
  glDisable(GL_BLEND);
  
  glPopMatrix();

  return;
}



/* glut's a little.. lacking when it comes to keyboards... */
static void procInput(int key, int x, int y)
{
  int rv = NODE_OK;
  scrNode_t *node = getTopNode();
  scrNode_t *onode = getTopONode();

#if 0
  clog("GL: procInput: key = %d, x = %d y = %d",
       key, x, y);
#endif

  if (node)
    {
      /* we give input priority to the overlay node */
      if (onode && onode->input)
        {
          rv = (*onode->input)(key);
          if (rv == NODE_EXIT)
            {
              conqend();
              exit(1);
            }
        }
      else
        {
          if (node->input)
            rv = (*node->input)(key);
          
          if (rv == NODE_EXIT)
            {
              conqend();
              exit(1);
            }
        }
    }

  return;
}

/* get called for arrows, Fkeys, etc... */
static void
input(int key, int x, int y)
{
  Unsgn32 kmod = glutGetModifiers();
  Unsgn32 jmod = 0;

  if (kmod & GLUT_ACTIVE_SHIFT)
    jmod |= CQ_KEY_MOD_SHIFT;
  if (kmod & GLUT_ACTIVE_CTRL)
    jmod |= CQ_KEY_MOD_CTRL;
  if (kmod & GLUT_ACTIVE_ALT)
    jmod |= CQ_KEY_MOD_ALT;

  procInput(((key & CQ_CHAR_MASK) << CQ_FKEY_SHIFT) | jmod, x, y);
  return;
}

/* for 'normal' keys */
static void charInput(unsigned char key, int x, int y)
{
  Unsgn32 kmod = glutGetModifiers();
  Unsgn32 jmod = 0;

  if (kmod & GLUT_ACTIVE_SHIFT)
    jmod |= CQ_KEY_MOD_SHIFT;
  if (kmod & GLUT_ACTIVE_CTRL)
    jmod |= CQ_KEY_MOD_CTRL;
  if (kmod & GLUT_ACTIVE_ALT)
    jmod |= CQ_KEY_MOD_ALT;

  procInput((key & CQ_CHAR_MASK) | jmod, x, y);
  return;
}

/* try to instantiate a texture using a PROXY_TEXTURE_2D to see if the
   implementation can handle it. */
static int checkTexture(char *filename, textureImage *texture)
{
  GLint type, components, param;
  
  GLError();                    /* clear any accumulated GL errors.
                                   If errors are reported in the logfile here,
                                   then the error occured prior to calling
                                   this function.  Look there :) */

  if (texture->bpp == 32)
    {
      type = GL_RGBA;
      components = 4;
    }
  else
    {
      type = GL_RGB;
      components = 3;
    }
  
  glTexImage2D(GL_PROXY_TEXTURE_2D, 0, components, 
               texture->width, texture->height, 0,
               type, GL_UNSIGNED_BYTE, texture->imageData);
  
  
  glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, 
                           GL_TEXTURE_WIDTH, &param);
  
  if (GLError() || !param) 
    {
      clog("%s: ERROR: Texture too big, or non power of two in width or height).", 
           filename);
      return FALSE;
    }
  
  return TRUE;
}

/* load a tga file into texture.  Supports RLE compressed as well
   as uncompressed TGA files. */
static int LoadTGA(char *filename, textureImage *texture)
{    
  /* TGA headers */
  static GLubyte TGAUncompressedHDR[12] = 
    {0,0,2,0,0,0,0,0,0,0,0,0}; /* Uncompressed header */
  static GLubyte TGACompressedHDR[12] = 
    {0,0,10,0,0,0,0,0,0,0,0,0}; /* Compressed header */

  GLubyte TGAHeaderBytes[12]; /* Used To Compare TGA Header */
  GLubyte header[6]; /* First 6 Useful Bytes From The Header */
  GLuint bytesPerPixel; 
  GLuint imageSize; 
  GLuint temp;   
  FILE *file;
  int i;
  int compressed = FALSE;

  if ((file = fopen(filename, "rb")) == NULL)
    {
      clog("%s: %s", filename, strerror(errno));
      return FALSE;
    }
#if defined(DEBUG_GL)      
  else
    {
      clog("%s: Loading texture %s",
           __FUNCTION__, filename);
    }
#endif


  if (fread(TGAHeaderBytes, 1, sizeof(TGAHeaderBytes), file) != 
      sizeof(TGAHeaderBytes))
    {
      clog("%s: Invalid TGA file: could not read TGA header.", filename);
      fclose(file);
      return FALSE;
    }

  if (!memcmp(TGAUncompressedHDR, 
             TGAHeaderBytes,
              sizeof(TGAUncompressedHDR)))
    {
      compressed = FALSE;
    }
  else if (!memcmp(TGACompressedHDR, 
                   TGAHeaderBytes,
                   sizeof(TGAUncompressedHDR)))
    {
      compressed = TRUE;
    }
  else
    {
      clog("%s: Invalid TGA file header.", filename);
      fclose(file);
      return FALSE;
    }


  if (fread(header, 1, sizeof(header), file) != sizeof(header))
    {
      clog("%s: Invalid TGA image header.", filename);
      fclose(file);
      return FALSE;
    }
  
  texture->width = header[1] * 256 + header[0]; 
  texture->height = header[3] * 256 + header[2];
  texture->bpp	= header[4];
  
  if (texture->width <= 0 || texture->height <= 0 ||
     (texture->bpp !=24 && texture->bpp != 32))
    {
      clog("%s: Invalid file format: must be 24bpp or 32bpp (with alpha)", 
           filename);
      fclose(file);
      return FALSE;
    }
  
  bytesPerPixel	= texture->bpp / 8;
  imageSize = texture->width * texture->height * bytesPerPixel;
  texture->imageData = (GLubyte *)malloc(imageSize);
  
  if (!texture->imageData)
    {
      clog("%s: Texture alloc (%d bytes) failed.", 
           imageSize, filename);
      fclose(file);
      return FALSE;
    }


  if (!compressed)
    {                           /* non-compressed texture data */

      if (fread(texture->imageData, 1, imageSize, file) != imageSize)
        {
          clog("%s: Image data read failed.", filename);
          fclose(file);
          return FALSE;
        }

      /* swap B and R */
      for(i=0; i<imageSize; i+=bytesPerPixel)
        {
          temp=texture->imageData[i];
          texture->imageData[i] = texture->imageData[i + 2];
          texture->imageData[i + 2] = temp;
        }
    }
  else
    {                           /* RLE compressed image data */
      GLuint pixelcount	= texture->height * texture->width; 
      GLuint currentpixel	= 0; /* Current pixel being read */
      GLuint currentbyte	= 0; /* Current byte */
      /* Storage for 1 pixel */
      GLubyte *colorbuffer = (GLubyte *)malloc(bytesPerPixel); 

      if (!colorbuffer)
        {
          clog("%s: Colorbuffer alloc (%d bytes) failed.", 
               filename, bytesPerPixel);
          fclose(file);
          return FALSE;
        }

      do
        {
          GLubyte chunkheader = 0; /* Storage for "chunk" header */
          
          if(fread(&chunkheader, sizeof(GLubyte), 1, file) == 0)
            {
              clog("%s: could not read RLE header.", filename);
              fclose(file);
              return False;
            }
          
          if(chunkheader < 128)
            { 
              /* If the chunkheader is < 128, it means that that is the
                 number of RAW color packets minus 1 that
                 follow the header */
              short counter;
              chunkheader++; /* add 1 to get number of following
                                color values */

              for(counter = 0; counter < chunkheader; counter++)
                {               /* Read RAW color values */
                  if(fread(colorbuffer, 1, 
                           bytesPerPixel, file) != bytesPerPixel) 
                    {           /* Try to read 1 pixel */
                      clog("%s: could not read colorbuffer data.", filename);
                      fclose(file);	
                      
                      if(colorbuffer != NULL)
                        {
                          free(colorbuffer);
                        }
                      
                      return FALSE;
                    }

                  texture->imageData[currentbyte] = colorbuffer[2];
                  /* Flip R and B vcolor values around in the process */
                  texture->imageData[currentbyte + 1] = colorbuffer[1];
                  texture->imageData[currentbyte + 2] = colorbuffer[0];
                  
                  if(bytesPerPixel == 4)
                    {           /* if its a 32 bpp image copy the 4th byte */
                      texture->imageData[currentbyte + 3] = colorbuffer[3];
                    }
                  
                  currentbyte += bytesPerPixel; 
                  currentpixel++;
                  
                  /* Make sure we havent read too many pixels */
                  if(currentpixel > pixelcount)
                    {
                      clog("%s: too many pixels read.", filename);
                      
                      fclose(file);
                      
                      if(colorbuffer != NULL)
                        {
                          free(colorbuffer);
                        }
                      
                      return FALSE;
                    }
                }
            }
          else
            {
              /* chunkheader > 128 RLE data, next color
                 repeated chunkheader - 127 times */
              short counter;
              /* Subteact 127 to get rid of the ID bit */
              chunkheader -= 127;

              /* Attempt to read following color values */
              if(fread(colorbuffer, 1, bytesPerPixel, file) != 
                 bytesPerPixel)
                {
                  clog("%s: could not read colorbuffer data.", filename);
                  
                  fclose(file);
                  
                  if(colorbuffer != NULL)
                    {
                      free(colorbuffer);
                    }
                  
                  return FALSE;
                }
              
              /* copy the color into the image data as many times as
                 dictated */
              for (counter=0; counter<chunkheader; counter++)
                {
                  texture->imageData[currentbyte] = colorbuffer[2];
                  /* switch R and B bytes areound while copying */
                  texture->imageData[currentbyte + 1] = colorbuffer[1];
                  texture->imageData[currentbyte + 2] = colorbuffer[0];
                  
                  if(bytesPerPixel == 4)
                    {
                      texture->imageData[currentbyte + 3] = colorbuffer[3];
                    }
                  
                  currentbyte += bytesPerPixel;
                  currentpixel++;

                  /* Make sure we haven't read too many pixels */
                  if(currentpixel > pixelcount)
                    {
                      clog("%s: too many pixels read.", filename);
                      
                      fclose(file);
                      
                      if(colorbuffer != NULL)
                        {
                          free(colorbuffer);
                        }
                      
                      return FALSE;
                    }
                }
            }
        }
      while (currentpixel < pixelcount);
    } /* compressed tga */
  
  /* now test to see if the implementation can handle the texture */
  
  if (!checkTexture(filename, texture))
    {
      fclose(file);
      return FALSE;
    }

  fclose (file);
  
  return TRUE;	
}

/* look for a texture file and return a file name.  We look
   first in the users ~/.conquest/img/ dir (allowing users to override
   the pre-defined textures), then in CONQSHARE/img.  Return NULL if not
   found */
static char *_getTexFile(char *tfilenm)
{
  char *homevar;
  FILE *fd;
  static char buffer[BUFFER_SIZE];
  
  /* look for a user image */
  if ((homevar = getenv("HOME")))
    {
      snprintf(buffer, sizeof(buffer)-1, "%s/.conquest/img/%s.tga", 
               homevar, tfilenm);

      if ((fd = fopen(buffer, "r")))
        {                       /* found one */
          fclose(fd);
          return buffer;
        }
    }

  /* if we are here, look for the system one */
  snprintf(buffer, sizeof(buffer) - 1, "%s/img/%s.tga", 
           CONQSHARE, tfilenm);

  if ((fd = fopen(buffer, "r")))
    {                       /* found one */
      fclose(fd);
      return buffer;
    }

  return NULL;
}

static int loadGLTextures()   
{
  Bool status;
  int rv = FALSE;
  textureImage *texti;
  int i, type, components;         /* for RGBA */
  char *filenm;
  GLTexture_t curTexture;
  GLTexture_t *texptr;
  int hwtextures = 0;

  if (!cqiNumTextures || !cqiTextures)
    {                           /* we have a problem */
      clog("%s: ERROR: cqiNumTextures or cqiTextures is 0! No textures loaded.\n",
           __FUNCTION__);
      return FALSE;
    }

  /* now try to load each texture and setup the proper data */
  for (i=0; i<cqiNumTextures; i++)
    {
      int texid = 0, texw = 0, texh = 0;
      int ndx = -1;
      int col_only = FALSE;     /* color-only texture? */

      memset((void *)&curTexture, 0, sizeof(GLTexture_t));
      texid = 0;
      texw = texh = 0;          /* default width/height */
      rv = FALSE;

      if (cqiTextures[i].flags & CQITEX_F_COLOR_SPEC)
        col_only = TRUE;

      /* first see if a texture with the same filename was already loaded.
         if so, no need to do it again, just copy the previously loaded
         data */
      if (GLTextures && !col_only && 
          (ndx = findGLTextureByFile(cqiTextures[i].filename)) > 0)
        {                       /* the same hw texture was previously loaded
                                   just save it's texture id and w/h */
          texid = GLTextures[ndx].id;
          texw = GLTextures[ndx].w;
          texh = GLTextures[ndx].h;
#ifdef DEBUG_GL
          clog("%s: texture file '%s' already loaded, using existing tid.", 
               __FUNCTION__, cqiTextures[i].filename);
#endif
        }

      if (!texid && !col_only)
        {
          texti = malloc(sizeof(textureImage));
          
          if (!texti)
            {
              clog("loadGLTextures(): memory allocation failed for %d bytes\n",
                   sizeof(textureImage));
              return FALSE;
            }
          
          memset((void *)texti, 0, sizeof(textureImage));

          /* look for a suitable file */
          if (!(filenm = _getTexFile(cqiTextures[i].filename)))
            {
              rv = FALSE;
            }
          else if ((rv = LoadTGA(filenm, texti)) == TRUE)
            {
              status = TRUE;

              /* create the texture */
              glGenTextures(1, (GLuint *)&curTexture.id); 
              glBindTexture(GL_TEXTURE_2D, curTexture.id);
              GLError();
              
              curTexture.w = texti->width;
              curTexture.h = texti->height;
              /* use linear filtering */
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
              glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
              
              if (texti->bpp == 32)
                {
                  type = GL_RGBA;
                  components = 4;
                }
              else
                {
                  type = GL_RGB;
                  components = 3;
                }
              
              /* generate the texture */
              glTexImage2D(GL_TEXTURE_2D, 0, components, 
                           texti->width, texti->height, 0,
                           type, GL_UNSIGNED_BYTE, texti->imageData);

              hwtextures++;
            } /* if rv */
      
          GLError();

          /* be free! */
          if (texti)
            {
              if (texti->imageData && rv == TRUE)
                free(texti->imageData);
              free(texti);
            }    
        }

      if (rv || texid || col_only)     /* tex/color load/locate succeeded,
                                          add it to the list */
        {
          texptr = (GLTexture_t *)realloc((void *)GLTextures, 
                                          sizeof(GLTexture_t) * 
                                          (loadedGLTextures + 1));
          
          if (!texptr)
            {  
              clog("%s: Could not realloc %d textures, ignoring texture '%s'",
                   __FUNCTION__,
                   loadedGLTextures + 1,
                   cqiTextures[i].name);
              return FALSE;
            }
          
          GLTextures = texptr;
          texptr = NULL;
          
          /* now set it up */
          if (texid)
            {
              curTexture.id = texid;
              curTexture.w = texw;
              curTexture.h = texh;
            }

          curTexture.cqiIndex = i;
          hex2GLColor(cqiTextures[i].color, &curTexture.col);
          
          GLTextures[loadedGLTextures] = curTexture;
          loadedGLTextures++;
        }
    }

  clog("%s: Successfully loaded %d textures, (%d files).", 
       __FUNCTION__, loadedGLTextures, hwtextures);

  return TRUE;
}

/* scale can be between -5 and 5.  negative means zoom out */
void setViewerScaling(int scale, int isLR)
{

  if (scale < -5 || scale > 5)
    return;

  if (isLR)
    {                           /* setting the LR scaling */
      if (scale == 0)           /* 1X */
        dConf.vScaleLR = 1.0;
      else
        {
          if (scale < 0)
            dConf.vScaleLR = scaleFactorsLR[scale + 5];
          else
            dConf.vScaleLR = scaleFactorsLR[(scale - 1) + 5];
            
        }
     }
  else
    {
      if (scale == 0)           /* 1X */
        dConf.vScaleSR = 1.0;
      else
        {
          if (scale < 0)
            dConf.vScaleSR = scaleFactorsSR[scale + 5];
          else
            dConf.vScaleSR = scaleFactorsSR[(scale - 1) + 5];
            
        }
     }
  
  return;
}
