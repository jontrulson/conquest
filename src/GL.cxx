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
#include "context.h"
#include "global.h"
#include "color.h"
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"
#include "rndlb.h"
#include "ibuf.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include "textures.h"

#include "gldisplay.h"

#include "anim.h"

// Try out the STB image loader
#define STB_IMAGE_IMPLEMENTATION
# include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#include "blinker.h"

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
#include "hud.h"

#include "initvec.h"
#include "ping.h"

#include "nMenu.h"

#include <vector>

/* bomb (torp) animation state per ship */
static std::vector<animStateRec_t> bombAState;

static int frame=0, timebase=0;
static float FPS = 0.0;


#define TEXT_HEIGHT    ((GLfloat)1.75)   /* 7.0/4.0 - text font height
                                            for viewer */
/* from nCP */
extern animStateRec_t ncpTorpAnims[NUMPLAYERTEAMS];

/* global tables for looking up textures quickly */
typedef struct _gl_planet {
    textureIdx_t tex;           /* index to the proper GLTexture entry */
    GLfloat      size;          /* the prefered size, in prescaled CU's*/
} GLPlanet_t;

static GLPlanet_t *GLPlanets = NULL;

/* storage for doomsday tex */
static struct {
    textureIdx_t doom;            /* doomsday */
    textureIdx_t beam;            /* doomsday AP beam */
} GLDoomsday = {};


/* raw TGA texture data */
typedef struct
{
    // stb_image
    unsigned char *imageData;
    int           components;
    int           width;
    int           height;
    GLuint texID;
} textureImage;

static void resize(int w, int h);
static void charInput(unsigned char key, int x, int y);
static void input(int key, int x, int y);
static int loadGLTextures(void);
static void renderFrame(void);
static nodeStatus_t renderNode(void);

/* It begins... */

float getFPS(void)
{
    return FPS;
}

/* convert an unsigned int color spec (hexcolor in AARRGGBB format) into a
   GLColor */
void hex2GLColor(uint32_t hcol, GLColor_t *col)
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
textureIdx_t findGLTexture(const char *texname)
{
    textureIdx_t i;

    if (!GLTextures.size() || !cqiNumTextures || !cqiTextures)
        return -1;

    for (i=0; i<GLTextures.size(); i++)
    {
        if (GLTextures[i].cqiIndex >= 0)
        {
            if (!strncmp(cqiTextures[GLTextures[i].cqiIndex].name,
                         texname, CQI_NAMELEN))
                return i;
        }
    }

    return -1;
}

/* search the cqi animations, and return it's animdef index */
int findGLAnimDef(const char *animname)
{
    int i;

    if (!GLTextures.size() || !cqiNumTextures || !cqiTextures ||
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
static int findGLTextureByFile(char *texfile, uint32_t flags)
{
    int i;

    if (!GLTextures.size() || !cqiNumTextures || !cqiTextures)
        return -1;

    /* we check both the filename and flags */
    for (i=0; i<GLTextures.size(); i++)
    {
        if (!strncmp(cqiTextures[GLTextures[i].cqiIndex].filename,
                     texfile, CQI_NAMELEN) &&
            (flags == cqiTextures[GLTextures[i].cqiIndex].flags) )
        {
            return i;
        }
    }

    return -1;
}

static int initGLAnimDefs(void)
{
    int i, j;
    int ndx;
    char buffer[CQI_NAMELEN];

    utLog("%s: Initializing...", __FUNCTION__);

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
        utLog("%s: ERROR: malloc(%lu) failed.", __FUNCTION__,
              sizeof(GLAnimDef_t) * cqiNumAnimDefs);

        return false;
    }

    memset((void *)GLAnimDefs, 0, sizeof(GLAnimDef_t) * cqiNumAnimDefs);

    for (i=0; i<cqiNumAnimDefs; i++)
    {
        /* if there is a texname, and no texanim, setup 'default' texid */
        if (cqiAnimDefs[i].texname[0] && !(cqiAnimDefs[i].anims & CQI_ANIMS_TEX))
        {
            if ((ndx = findGLTexture(cqiAnimDefs[i].texname)) >= 0)
                GLAnimDefs[i].texid = GLTEX_ID(ndx);
            else
                utLog("%s: could not locate texture '%s' for animdef '%s'.",
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
                GLAnimDefs[i].itexid = GLTEX_ID(ndx);
            else
            {
                utLog("%s: could not locate istate texture '%s' for animdef '%s'.",
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
            GLAnimDefs[i].iangle = utMod360(cqiAnimDefs[i].iangle);

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
                utLog("%s: ERROR: _anim_texture_ent malloc(%lu) failed.",
                      __FUNCTION__,
                      sizeof(struct _anim_texture_ent) * GLAnimDefs[i].tex.stages);

                /* this is fatal */
                return false;
            }

            memset((void *)GLAnimDefs[i].tex.tex,
                   0,
                   sizeof(struct _anim_texture_ent) * GLAnimDefs[i].tex.stages);

            /* now setup the texture entry array */

            for (j=0; j<GLAnimDefs[i].tex.stages; j++)
            {

                /* if the texanim only contains a single stage (one texture)
                 * then do not append the stage number to the texname.
                 */
                if (GLAnimDefs[i].tex.stages == 1)
                    snprintf(buffer, CQI_NAMELEN, "%s",
                             cqiAnimDefs[i].texname);
                else
                    snprintf(buffer, CQI_NAMELEN, "%s%d",
                             cqiAnimDefs[i].texname, j);

                if ((ndx = findGLTexture(buffer)) >= 0)
                {
                    GLAnimDefs[i].tex.tex[j].id =
                        GLTEX_ID(ndx);

                    if (HAS_GLCOLOR(GLAnimDefs[i].tex.color))
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
                    utLog("%s: could not locate texanim texture '%s' for animdef '%s'.",
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

    return true;
}



/* init the explosion animation states. */
static bool initGLExplosions(std::vector<std::vector<animStateRec_t>>& torpStates)
{
    animStateRec_t initastate;    /* initial state of an explosion */

    utLog("%s: Initializing...", __FUNCTION__);

    /* we only need to do this once.  When we have a properly initted state,
       we will simply copy it into the torpAState array.  */
    if (!animInitState("explosion", &initastate, NULL))
        return true;

    /* we start out expired of course :) */
    initastate.expired = CQI_ANIMS_MASK;

    // init the vector
    torpStates.clear();
    for (int i=0; i<cbLimits.maxShips(); i++)
        torpStates.push_back(std::vector<animStateRec_t>(cbLimits.maxTorps()));

    // copy in our prepared animation state
    for (int i=0; i<cbLimits.maxShips(); i++)
        for (int j=0; j<cbLimits.maxTorps(); j++)
            torpStates[i][j] = initastate;

    return false;
}

static textureIdx_t _get_tex(const char *name)
{
    textureIdx_t tex;

    if (!name)                    /* should never happen, but... */
        return defaultTextureIdx;

    if ((tex = findGLTexture(name)) >= 0)
        return tex;
    else
        utLog("%s: Could not find texture '%s'", __FUNCTION__, name);

    return defaultTextureIdx;
}

/* initialize the GLShips array.  We also load the tac ring colors here too. */
static int initGLShips(void)
{
    int i, j;
    char shipPfx[CQI_NAMELEN];
    char buffer[CQI_NAMELEN];

    utLog("%s: Initializing...", __FUNCTION__);

    memset((void *)&GLShips, 0, sizeof(GLShips));

    /* for each possible ship, lookup and set all the texids. */
    /* NOTE: I think there are too many... */
    for (i=0; i<NUMPLAYERTEAMS; i++)
    {
        /* build the prefix for this ship */
        snprintf(shipPfx, CQI_NAMELEN, "ship%c",
                 cbTeams[i].teamchar);

        for (j=0; j<MAXNUMSHIPTYPES; j++)
        {
            snprintf(buffer, CQI_NAMELEN, "%s%c%c", shipPfx,
                     cbShipTypes[j].name[0], cbShipTypes[j].name[1]);
            GLShips[i][j].ship = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s%c%c-sh", shipPfx,
                     cbShipTypes[j].name[0], cbShipTypes[j].name[1]);
            GLShips[i][j].sh = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-phaser", shipPfx);
            GLShips[i][j].phas = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s%c%c-ico", shipPfx,
                     cbShipTypes[j].name[0], cbShipTypes[j].name[1]);
            GLShips[i][j].ico = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s%c%c-ico-sh", shipPfx,
                     cbShipTypes[j].name[0], cbShipTypes[j].name[1]);
            GLShips[i][j].ico_sh = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-torp", shipPfx);
            GLShips[i][j].ico_torp = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1", shipPfx);
            GLShips[i][j].decal1 = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1-lamp-sh", shipPfx);
            GLShips[i][j].decal1_lamp_sh = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1-lamp-hull", shipPfx);
            GLShips[i][j].decal1_lamp_hull = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1-lamp-fuel", shipPfx);
            GLShips[i][j].decal1_lamp_fuel = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1-lamp-eng", shipPfx);
            GLShips[i][j].decal1_lamp_eng = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1-lamp-wep", shipPfx);
            GLShips[i][j].decal1_lamp_wep = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1-lamp-rep", shipPfx);
            GLShips[i][j].decal1_lamp_rep = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1-lamp-cloak",
                     shipPfx);
            GLShips[i][j].decal1_lamp_cloak = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal1-lamp-tow", shipPfx);
            GLShips[i][j].decal1_lamp_tow = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-ico-decal2", shipPfx);
            GLShips[i][j].decal2 = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-dial", shipPfx);
            GLShips[i][j].dial = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-dialp", shipPfx);
            GLShips[i][j].dialp = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-warp", shipPfx);
            GLShips[i][j].warp = _get_tex(buffer);

            snprintf(buffer, CQI_NAMELEN, "%s-warp2", shipPfx);
            GLShips[i][j].warp2 = _get_tex(buffer);

            /* here we just want the color, but we get a whole tex anyway */
            snprintf(buffer, CQI_NAMELEN, "%s-warp-col", shipPfx);
            GLShips[i][j].warpq_col = _get_tex(buffer);

            /* if we failed to find some of them, you'll see soon enough. */
        }
    }

    // Load and initialize the tactical ring colors

    // 1K ring
    tacRing1K = _get_tex("tac-ring1k");
    tacRing2K = _get_tex("tac-ring2k");
    tacRing3K = _get_tex("tac-ring3k");
    tacRingXK = _get_tex("tac-ringxk");
    tacRing10K = _get_tex("tac-ring10k");

    return true;
}

/* figure out the texture, color, and size info for a planet */
static int _get_glplanet_info(GLPlanet_t *curGLPlanet, int plani)
{
    int ndx;
    GLfloat size;
    int gltndx = -1;

    if (!GLPlanets)
        return false;

    if (plani < 0 || plani >= cbLimits.maxPlanets())
        return false;

    /* first find the appropriate texture.  We look for one in this order:
     *
     * 1. Look for a texture named the same as the planet.  If
     *    available, use it.
     * 2. All else failing, then select a default texture (classm, etc)
     *    based on the planet type.
     */

    /* still no texture? */
    if (gltndx == -1)
    {                       /* see if there is a texture available
                               named after the planet */
        if ((ndx = findGLTexture(cbPlanets[plani].name)) >= 0)
            gltndx = ndx;       /* yes */
    }

    /* still no luck? What a loser. */
    if (gltndx == -1)
    {                       /* now we just choose a default */
        switch (cbPlanets[plani].type)
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

        case PLANET_CLASSA:
            gltndx = findGLTexture("classa");
            break;

        case PLANET_CLASSO:
            gltndx = findGLTexture("classo");
            break;

        case PLANET_CLASSZ:
            gltndx = findGLTexture("classz");
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
        utLog("%s: ERROR: Unable to locate a texture for planet '%s'.",
              __FUNCTION__,
              cbPlanets[plani].name);

        return false;
    }

    /* now we are set, get the GLTexture */

    curGLPlanet->tex = gltndx;

    size = (real)cbPlanets[plani].size * GLTextures[gltndx].prescale;

#if 0
    utLog("Computed size %f for planet %s\n",
          size, cbPlanets[plani].name);
#endif

    curGLPlanet->size = size;

    return true;
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
        return false;

    if (plani < 0 || plani >= cbLimits.maxPlanets())
        return false;

    curGLPlanet = &GLPlanets[plani];

    return _get_glplanet_info(curGLPlanet, plani);
}


/* initialize the GLPlanets array */
static int initGLPlanets(void)
{
    int i;
    GLPlanet_t curGLPlanet;

    utLog("%s: Initializing...", __FUNCTION__);

    /* first, if there's already one present, free it */

    if (GLPlanets)
    {
        free(GLPlanets);
        GLPlanets = NULL;
    }

    /* allocate enough space */

    if (!(GLPlanets = (GLPlanet_t *)malloc(sizeof(GLPlanet_t) * cbLimits.maxPlanets())))
    {
        utLog("%s: ERROR: malloc(%lu) failed.", __FUNCTION__,
              sizeof(GLPlanet_t) * cbLimits.maxPlanets());

        return false;
    }

    /* now go through each one, setting up the proper values */
    for (i=0; i<cbLimits.maxPlanets(); i++)
    {
        memset((void *)&curGLPlanet, 0, sizeof(GLPlanet_t));

        if (!_get_glplanet_info(&curGLPlanet, i))
            return false;

        /* we're done, assign it and go on to the next one */
        GLPlanets[i] = curGLPlanet;
    }

    return true;
}

/* render a 'decal' for renderHud()  */
void drawIconHUDDecal(GLfloat rx, GLfloat ry, GLfloat w, GLfloat h,
                      textureHUDItem_t imgp, cqColor icol)
{
    int steam = cbShips[Context.snum].team,
        stype = cbShips[Context.snum].shiptype;
    static int norender = false;
    textureIdx_t tex = defaultTextureIdx;

    if (norender)
        return;

    /* choose the correct texture and render it */

    if (!GLShips[0][0].ship)
        if (!initGLShips())
        {
            utLog("%s: initGLShips failed, bailing.",
                  __FUNCTION__);
            norender = true;
            return;                 /* we need to bail here... */
        }

    switch (imgp)
    {
    case TEX_HUD_ICO:
        tex = GLShips[steam][stype].ico;
        break;
    case TEX_HUD_SHI:
        tex = GLShips[steam][stype].ico_sh;
        break;
    case TEX_HUD_DECAL1:
        tex = GLShips[steam][stype].decal1;
        break;
    case TEX_HUD_DECAL1_LAMP_SH:
        tex = GLShips[steam][stype].decal1_lamp_sh;
        break;
    case TEX_HUD_DECAL1_LAMP_HULL:
        tex = GLShips[steam][stype].decal1_lamp_hull;
        break;
    case TEX_HUD_DECAL1_LAMP_FUEL:
        tex = GLShips[steam][stype].decal1_lamp_fuel;
        break;
    case TEX_HUD_DECAL1_LAMP_ENG:
        tex = GLShips[steam][stype].decal1_lamp_eng;
        break;
    case TEX_HUD_DECAL1_LAMP_WEP:
        tex = GLShips[steam][stype].decal1_lamp_wep;
        break;
    case TEX_HUD_DECAL1_LAMP_REP:
        tex = GLShips[steam][stype].decal1_lamp_rep;
        break;
    case TEX_HUD_DECAL1_LAMP_CLOAK:
        tex = GLShips[steam][stype].decal1_lamp_cloak;
        break;
    case TEX_HUD_DECAL1_LAMP_TOW:
        tex = GLShips[steam][stype].decal1_lamp_tow;
        break;
    case TEX_HUD_DECAL2:
        tex = GLShips[steam][stype].decal2;
        break;
    case TEX_HUD_HEAD:
        tex = GLShips[steam][stype].dial;
        break;
    case TEX_HUD_HDP:
        tex = GLShips[steam][stype].dialp;
        break;
    case TEX_HUD_WARP:
        tex = GLShips[steam][stype].warp;
        break;
    case TEX_HUD_WARP2:
        tex = GLShips[steam][stype].warp2;
        break;
    default:
        break;
    }

    glBindTexture(GL_TEXTURE_2D, GLTEX_ID(tex));
    if (icol)
        uiPutColor(icol);
    else                          /* if 0, do a faint grey */
        glColor4f(0.1, 0.1, 0.1, 1.0);

    glBegin(GL_POLYGON);

    /* The HUD elements are drawn in an orthographic projection, in
     *  which y is inverted.  So we map the tex coordinates here to
     *  specificly 're-invert' the texture's y coords so all looks right.
     */

    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(rx, ry);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(rx + w, ry);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(rx + w, ry + h);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(rx, ry + h);

    glEnd();
}

static void mouse(int b, int state, int x, int y)
{
    static mouseData_t mdata;
    uint32_t kmod = glutGetModifiers();
    int rv = NODE_OK;
    scrNode_t *node = getTopNode();
    scrNode_t *onode = getTopONode();

#if 0
    utLog("%s: b = %d, state = %d x = %d, y = %d",
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
    utLog("drawLine: x = %f, y = %f len = %f",
          x, y, len);
#endif

    glLineWidth(lw);

    glBegin(GL_LINES);
    glVertex3f(x, y, 0.0); /* ul */
    glVertex3f(x + len, y, 0.0); /* ur */
    glEnd();

    return;
}

// Draw a circle.  This algorithm came from:
//
// http://slabode.exofire.net/circle_draw.shtml
//
// It works well.  I think we could make it way faster if we used
// a vertex array...
void drawCircle(float x, float y, float r, int num_segments)
{
    float theta = 2 * 3.1415926 / float(num_segments);
    float c = cosf(theta);//precalculate the sine and cosine
    float s = sinf(theta);
    float t;

    float xx = r;                // we start at angle = 0
    float yy = 0;

    glBegin(GL_LINE_LOOP);
    for (int ii = 0; ii < num_segments; ii++)
    {
        glVertex3f(xx + x, yy + y, TRANZ); // always draw at z = TRANZ

        // apply the rotation matrix
        t = xx;
        xx = c * xx - s * yy;
        yy = s * t + c * yy;
    }
    glEnd();
}



void drawLineBox(GLfloat x, GLfloat y, GLfloat z,
                 GLfloat w, GLfloat h, int color,
                 GLfloat lw)
{

#if 0
    utLog("drawLineBox: x = %f, y = %f, z = %f, w = %f, h = %f",
          x, y, z, w, h);
#endif

    glLineWidth(lw);

    glBegin(GL_LINE_LOOP);
    uiPutColor(color);
    glVertex3f(x, y, z);          /* ul */
    glVertex3f(x + w, y, z);      /* ur */
    glVertex3f(x + w, y + h, z);  /* lr */
    glVertex3f(x, y + h, z);      /* ll */
    glEnd();

    return;
}

void drawQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h, GLfloat z)
{
    glBegin(GL_POLYGON);
    glVertex3f(x, y, z);          /* ll */
    glVertex3f(x + w, y, z);      /* lr */
    glVertex3f(x + w, y + h, z);  /* ur */
    glVertex3f(x, y + h, z);      /* ul */
    glEnd();

    return;
}

void drawTexQuad(GLfloat x, GLfloat y, GLfloat z, GLfloat w, GLfloat h,
                 int ortho, int rot90)
{
    /* perspective */
    static const GLfloat tc_perspective[4][2] = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };
    /* perspective, tex coords rotated 90 degrees */
    static const GLfloat tc_perspective90[4][2] = {
        { 0.0f, 1.0f },
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f }
    };
    /* ortho inverts Y, so we need to invert texture T to compensate */
    static const GLfloat tc_ortho[4][2] = {
        { 0.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 0.0f },
        { 0.0f, 0.0f }
    };
    /* ortho, tex coords rotated 90 degrees */
    static const GLfloat tc_ortho90[4][2] = {
        { 0.0f, 0.0f },
        { 0.0f, 1.0f },
        { 1.0f, 1.0f },
        { 1.0f, 0.0f }
    };

    GLfloat *tc;

    if (ortho)
    {
        if (rot90)
            tc = (GLfloat *)&tc_ortho90;
        else
            tc = (GLfloat *)&tc_ortho;
    }
    else
    {
        if (rot90)
            tc = (GLfloat *)&tc_perspective90;
        else
            tc = (GLfloat *)&tc_perspective;
    }

    glBegin(GL_QUADS);

    glTexCoord2fv(tc + 0);
    glVertex3f(x, y, z);          /* ll */

    glTexCoord2fv(tc + 2);
    glVertex3f(x + w, y, z);      /* lr */

    glTexCoord2fv(tc + 4);
    glVertex3f(x + w, y + h, z);  /* ur */

    glTexCoord2fv(tc + 6);
    glVertex3f(x, y + h, z);      /* ul */

    glEnd();

    return;
}

/* draw textured square centered on x,y */
void drawTexBoxCentered(GLfloat x, GLfloat y, GLfloat z, GLfloat size,
                        int ortho, int rot90)
{
    drawTexQuad(x - (size / 2),
                y - (size / 2),
                z, size, size, ortho, rot90);

    return;
}

void drawExplosion(GLfloat x, GLfloat y, int snum, int torpnum, int scale)
{
    static int norender = false;
    scrNode_t *curnode = getTopNode();
    static int explodefx = -1;
    GLfloat scaleFac = (scale == MAP_SR_FAC) ? dConf.vScaleSR : dConf.vScaleLR;
    GLfloat size;
    // torp animation states per torp per ship
    static std::vector<std::vector<animStateRec_t>> torpAStates;


    if (norender)
        return;

    // see if it has been initialized
    if (!torpAStates.size())
    {
        if (initGLExplosions(torpAStates))
        {
            utLog("%s: initGLExplosions failed, bailing.",
                  __FUNCTION__);
            norender = true;
            return;                 /* we need to bail here... */
        }
    }

    if (explodefx == -1)
        explodefx = cqsFindEffect("explosion");

    /* if it expired and moved, reset and que a new one */
    if (ANIM_EXPIRED(&torpAStates[snum][torpnum]) &&
        (torpAStates[snum][torpnum].state.x != cbShips[snum].torps[torpnum].x &&
         torpAStates[snum][torpnum].state.y != cbShips[snum].torps[torpnum].y))
    {

        /* start the 'exploding' sound */
        if (cqsSoundAvailable)
        {
            real ang;
            real dis;

            ang = utAngle(cbShips[Context.snum].x, cbShips[Context.snum].y,
                          cbShips[snum].torps[torpnum].x,
                          cbShips[snum].torps[torpnum].y);
            dis = dist(cbShips[Context.snum].x, cbShips[Context.snum].y,
                       cbShips[snum].torps[torpnum].x,
                       cbShips[snum].torps[torpnum].y);
            cqsEffectPlay(explodefx, NULL, YELLOW_DIST, dis, ang);
        }

        if (curnode->animQue)
        {
            animResetState(&torpAStates[snum][torpnum], frameTime);

            /* we cheat a little by abusing the animstate's x and
               y as per-state storage. This allows us to detect if
               we really should Reset if expired */
            torpAStates[snum][torpnum].state.x = cbShips[snum].torps[torpnum].x;
            torpAStates[snum][torpnum].state.y = cbShips[snum].torps[torpnum].y;

            animQueAdd(curnode->animQue, &torpAStates[snum][torpnum]);
        }
    }

    size = cu2GLSize(torpAStates[snum][torpnum].state.size, -scale);

    if (scale == MAP_LR_FAC)
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

    glColor4fv(torpAStates[snum][torpnum].state.col.vec);

    drawTexBoxCentered(0.0, 0.0, 0.0, size, false, false);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glPopMatrix();

    return;
}

/* draw bombing graphics */
void drawBombing(int snum, int scale)
{
    scrNode_t *curnode = getTopNode();
    GLfloat x, y, size;
    int i;
    static animStateRec_t initastate;    /* initial state of a bomb explosion */
    GLfloat scaleFac = (scale == MAP_SR_FAC) ? dConf.vScaleSR : dConf.vScaleLR;
    struct _rndxy {               /* anim state private area */
        real rndx;                  /* random X offset from planet */
        real rndy;                  /* random Y offset from planet */
    } *rnd;
    int pnum;

    if (snum < 0 || snum >= cbLimits.maxShips())
        return;

    /* don't bother if we aren't orbiting anything */
    if (cbShips[snum].lock != LOCK_PLANET)
        return;

    pnum = (int)cbShips[snum].lockDetail;

    // init bombing animation storage and states
    if (!bombAState.size())
    {
        // allocate the array
        _INIT_VEC1D(bombAState, animStateRec_t, cbLimits.maxShips());

        if (!animInitState("bombing", &initastate, NULL))
            return;

        /* start out expired */
        initastate.expired = CQI_ANIMS_MASK;

        for (i=0; i < cbLimits.maxShips(); i++)
        {
            bombAState[i] = initastate;

            /* setup the private area we'll store with the state */
            if (!(rnd = (struct _rndxy*)malloc(sizeof(struct _rndxy))))
            {                   /* malloc failure, undo everything */
                int j;

                for (j=0; j < i; j++)
                    free(bombAState[j].state.privptr);
                // clear bombAState so we can retry again later */
                bombAState.clear();
                utLog("%s: malloc(%lu) failed", __FUNCTION__,
                      sizeof(struct _rndxy));
                return;
            }
            else
            {
                bombAState[i].state.privptr = (void *)rnd;
            }
        }
    }

    /* get the state's private area */
    if (!(rnd = (struct _rndxy *)bombAState[snum].state.privptr))
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
            real psize = (real)cbPlanets[pnum].size / 2.0;
            // reduce it by 20% to reduce "space" blasts in the "corners"
            psize *= 0.80;
            rnd->rndx = rnduni(-psize, psize); /* rnd X */
            rnd->rndy = rnduni(-psize, psize); /* rnd Y */
            animQueAdd(curnode->animQue, &bombAState[snum]);
        }

    }

    glPushMatrix();
    glLoadIdentity();

    /* calc and translate to correct position */
    GLcvtcoords( cbShips[Context.snum].x,
                 cbShips[Context.snum].y,
                 cbPlanets[pnum].x + rnd->rndx,
                 cbPlanets[pnum].y + rnd->rndy,
                 -scale,
                 &x,
                 &y);

    size = cu2GLSize(bombAState[snum].state.size, -scale);
    if (scale == MAP_LR_FAC)
        size = size * 2.0;

    glScalef(scaleFac, scaleFac, 1.0);

    glTranslatef(x, y, TRANZ);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bombAState[snum].state.id);

    glColor4fv(bombAState[snum].state.col.vec);

    drawTexBoxCentered(0.0, 0.0, 0.0, size, false, false);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glPopMatrix();

    return;
}

void drawPlanet( GLfloat x, GLfloat y, int pnum, int scale,
                 int textcolor )
{
    GLfloat size;
    char buf[BUFFER_SIZE_256];
    char torpchar;
    char planame[BUFFER_SIZE_256];
    static int norender = false;
    GLfloat scaleFac = (scale == MAP_SR_FAC) ? dConf.vScaleSR : dConf.vScaleLR;

    if (norender)
        return;

#if 0
    utLog("drawPlanet: pnum = %d, x = %.1f, y = %.1f\n",
          pnum, x, y);
#endif

    /* sanity */
    if (pnum < 0 || pnum >= cbLimits.maxPlanets())
    {
        utLog("uiGLdrawPlanet(): invalid pnum = %d", pnum);

        return;
    }

    /* if there's nothing available to render, no point in being here :( */
    if (!GLPlanets)
        if (!initGLPlanets())
        {
            utLog("%s: initGLPlanets failed, bailing.",
                  __FUNCTION__);
            norender = true;
            return;                 /* we need to bail here... */
        }

    glPushMatrix();
    glLoadIdentity();

    glScalef(scaleFac, scaleFac, 1.0);

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, GLTEX_ID(GLPlanets[pnum].tex));
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    glColor4fv(GLTEX_COLOR(GLPlanets[pnum].tex).vec);

    size = cu2GLSize(GLPlanets[pnum].size, -scale);

    /* so it's more visible... */
    if (scale == MAP_LR_FAC)
        size *= 2.0;

    drawTexBoxCentered(x, y, TRANZ, size, false, false);

    glDisable(GL_TEXTURE_2D);

    /*  text data... */
    glBlendFunc(GL_ONE, GL_ONE);

    if (cbPlanets[pnum].type == PLANET_SUN || cbPlanets[pnum].type == PLANET_MOON ||
        !cbPlanets[pnum].scanned[cbShips[Context.snum].team] )
        torpchar = ' ';
    else
        if ( cbPlanets[pnum].armies <= 0 || cbPlanets[pnum].team < 0 ||
             cbPlanets[pnum].team >= NUMPLAYERTEAMS )
            torpchar = '-';
        else
            torpchar = cbTeams[cbPlanets[pnum].team].torpchar;

    if (scale == MAP_SR_FAC)
    {
        if (UserConf.DoNumMap && (torpchar != ' '))
            snprintf(buf, BUFFER_SIZE_256, "#%d#%c#%d#%d#%d#%c%s",
                     textcolor,
                     torpchar,
                     InfoColor,
                     cbPlanets[pnum].armies,
                     textcolor,
                     torpchar,
                     cbPlanets[pnum].name);
        else
            snprintf(buf, BUFFER_SIZE_256, "%s", cbPlanets[pnum].name);

        glfRenderFont(x,
                      y - cu2GLSize((real)cbPlanets[pnum].size / 2.0,
                                    -scale),
                      TRANZ, /* planet's Z */
                      ((GLfloat)uiCStrlen(buf) * 2.0) / ((scale == MAP_SR_FAC) ? 1.0 : 2.0),
                      TEXT_HEIGHT, glfFontFixedTiny, buf, textcolor, NULL,
                      GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR);
    }
    else
    {                           /* MAP_LR_FAC */
        /* just want first 3 chars */
        planame[0] = cbPlanets[pnum].name[0];
        planame[1] = cbPlanets[pnum].name[1];
        planame[2] = cbPlanets[pnum].name[2];
        planame[3] = 0;

        if (UserConf.DoNumMap && (torpchar != ' '))
            snprintf(buf, BUFFER_SIZE_256, "#%d#%c#%d#%d#%d#%c%s",
                     textcolor,
                     torpchar,
                     InfoColor,
                     cbPlanets[pnum].armies,
                     textcolor,
                     torpchar,
                     planame);
        else
            snprintf(buf, BUFFER_SIZE_256, "#%d#%c#%d#%c#%d#%c%s",
                     textcolor,
                     torpchar,
                     InfoColor,
                     cbConqInfo->chrplanets[cbPlanets[pnum].type],
                     textcolor,
                     torpchar,
                     planame);

        glfRenderFont(x,
                      y - cu2GLSize((real)cbPlanets[pnum].size / 2.0, -scale),

                      TRANZ,
                      ((GLfloat)uiCStrlen(buf) * 2.0) / ((scale == MAP_SR_FAC) ? 1.0 : 2.0),
                      TEXT_HEIGHT, glfFontFixedTiny, buf, textcolor, NULL,
                      GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR);

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
    static const GLfloat fuzz = 2.0; /* 'fuzz' factor to pad the limit
                                        a little */
    int ascale = abs(scale);
    GLfloat limitx, limity;
    GLfloat vscale;
    GLfloat magscale;

    magscale = (ascale == MAP_SR_FAC) ? dConf.vScaleSR : dConf.vScaleLR;

    /* if scale is negative, do not scale the x/y with the current
       magfactor */
    if (scale < 0.0)
        vscale = 1.0;
    else
        vscale = magscale;

    /* determine number of CU's that could be seen vertically in LR or
       SR. (no magfactor scaling) */
    rscale = ((GLfloat)DISPLAY_LINS * ascale / (VIEWANGLE * 2.0));

    /* we must always scale the limit, regardless of whether X/Y are
       being scaled by vscale.  We multiply limity with fuzz to
       allow a little leeyway to the limit, so objects are less likely
       to just appear/disappear at the edges of the viewer, at the cost
       of potentially rendering an object that can't be seen. */
    limity = (VIEWANGLE * (1.0 / magscale)) * fuzz;
    limitx = limity * dConf.vAspect; /* account for the viewer's aspect ratio */

    *rx = ((x-cenx) / rscale) * vscale;
    *ry = ((y-ceny) / rscale) * vscale;

#if 0
    utLog("GLCVTCOORDS: rscale = %f limit = %f cx = %.2f, cy = %.2f, \n\tx = %.2f, y = %.2f, glx = %.2f,"
          " gly = %.2f \n",
          rscale, limit, cenx, ceny, x, y, *rx, *ry);
#endif

    if (*rx < -limitx|| *rx > limitx)
    {
        return false;
    }
    if (*ry < -limity || *ry > limity)
    {
        return false;
    }

    return true;
}

void dspInitData(void)
{
    memset((void *)&dConf, 0, sizeof(dspConfig_t));

    DSPFCLR(DSP_F_INITED);
    DSPFCLR(DSP_F_FULLSCREEN);

    // 720p by default
    dConf.initWidth = 1280;
    dConf.initHeight = 720;

    dConf.wX = dConf.wY = 0;
    dConf.vScaleLR = dConf.vScaleSR = 1.0;

    hudInitData();
    GLGeoChange = 1;

    return;
}

int uiGLInit(int *argc, char **argv)
{
#ifdef DEBUG_GL
    utLog("uiGLInit: ENTER");
#endif

    glutInit(argc, argv);
    glutInitDisplayMode(/*GLUT_DEPTH |*/ GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);

    // let the window manager decide...
    glutInitWindowPosition(-1, -1);

    glutInitWindowSize(dConf.initWidth, dConf.initHeight);

    dConf.mainw = glutCreateWindow(CONQUEST_NAME);

    if(DSP_FULLSCREEN())
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

    GLTextures.clear();
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

    utLog("graphicsInit: OpenGL Vendor:      %s", glVendor);
    utLog("graphicsInit: OpenGL Renderer:    %s", glRenderer);
    utLog("graphicsInit: OpenGL Version:     %s", glVersion);

    if (cqDebug > 1)
    {
        utLog("graphicsInit: OpenGL limits:");

        for (i = 0; limits[i].count; i++)
        {
            glGetIntegerv(limits[i].token, max);
            if (glGetError() == GL_NONE)
            {
                if (limits[i].count == 1)
                    utLog("    %s = %d", limits[i].name, max[0]);
                else /* XXX fix if we ever query something with more than 2 values */
                    utLog("    %s = %d, %d", limits[i].name, max[0], max[1]);
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
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    _print_gl_info();

    if (!loadGLTextures())
        utLog("ERROR: loadGLTextures() failed\n");
    else
    {
        if (!initGLAnimDefs())
            utLog("ERROR: initGLAnimDefs() failed\n");
    }

    /* init the blinkers */
    blinkerInit();

    return;
}

static void
resize(int w, int h)
{
    static int minit = false;
    real aspectCorrection;

    if (!minit)
    {
        minit = true;
        graphicsInit();
        glfInitFonts();
        GLError();
    }

    dConf.wW = (GLfloat)w;
    dConf.wH = (GLfloat)h;
    dConf.wAspect = (GLfloat)w/(GLfloat)h;

    /* for aspects less than 1.4 (actually 1.333 for 4:3) use 30% for
     *  the hud width, else use 25% - better for widescreen monitors.
     */

    if (dConf.wAspect > 1.4)
        aspectCorrection = 0.25;
    else
        aspectCorrection = 0.30;

    /* calculate the border width */
    dConf.wBorderW = ((dConf.wW * 0.01) + (dConf.wH * 0.01)) / 2.0;

    /* calculate viewer geometry */
    dConf.vX = dConf.wX + (dConf.wW * aspectCorrection);
    dConf.vY = dConf.wY + dConf.wBorderW;
    dConf.vW = (dConf.wW - dConf.vX) - dConf.wBorderW;
    dConf.vH = (dConf.wH - (dConf.wH * 0.20)); /* y + 20% */

    dConf.vAspect = dConf.vW/dConf.vH;

#ifdef DEBUG_GL
    utLog("GL: RESIZE: WIN = %d w = %d h = %d, \n"
          "    vX = %f, vY = %f, vW = %f, vH = %f",
          glutGetWindow(), w, h,
          dConf.vX, dConf.vY, dConf.vW, dConf.vH);
#endif

    /* we will pretend we have an 80x25 char display for
       the 'text' nodes. we account for the border area too */
    dConf.ppRow = (dConf.wH - (dConf.wBorderW * 2.0)) / 25.0;
    dConf.ppCol = (dConf.wW - (dConf.wBorderW * 2.0)) / 80.0;

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
    glGetFloatv(GL_PROJECTION_MATRIX, dConf.hudProjection);

    /* viewer */

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(VIEWANGLE, dConf.vAspect,
                   1.0, 1000.0);

    /* save a copy of this matrix */
    glGetFloatv(GL_PROJECTION_MATRIX, dConf.viewerProjection);

    /* restore hudProjection */
    glLoadMatrixf(dConf.hudProjection);

    glMatrixMode(GL_MODELVIEW);

    DSPFSET(DSP_F_INITED);

    GLGeoChange++;
    glutPostRedisplay();

    return;
}

static nodeStatus_t renderNode(void)
{
    scrNode_t *node = getTopNode();
    scrNode_t *onode = getTopONode();
    nodeStatus_t rv = NODE_OK;
    static bool lostNetwork = false;

    // update the node time
    cInfo.nodeMillis = clbGetMillis();

    /* always iter the blinker que */
    animQueRun(&blinkerQue);

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

        // the *Display() nodes can return NODE_OK_NO_PKTPROC to
        // indicate that the node will handle it's own packet
        // processing.  If we see that, we don't process packets.  We
        // also do not process packets while playing back a recording,
        // of course.

        if (!(Context.recmode == RECMODE_PLAYING
              || Context.recmode == RECMODE_PAUSED
              || rv == NODE_OK_NO_PKTPROC))
        {
            // look for packets and process them here, if the node's
            // display indicated it is not doing packet handling.
            int pkttype = 0;
            char buf[PKT_MAXSIZE];

            while (((pkttype = pktRead(buf, PKT_MAXSIZE, 0)) > 0)
                   && !pktNoNetwork())
                processPacket(buf);

            if (pkttype < 0 && !pktNoNetwork())          /* some error */
            {
                utLog("%s: pktRead returned %d", __FUNCTION__, pkttype);
                return NODE_EXIT;
            }
        }


        if (node->idle)
        {
            rv = (*node->idle)();
            if (rv == NODE_EXIT)
                return rv;
        }

        if (onode && onode->idle)
        {
            rv = (*onode->idle)();
            if (rv == NODE_EXIT)
                return rv;
        }

        if (pktNoNetwork() && !lostNetwork)
        {
            // take us back to the menu node, which will show an
            // appropriate error screen and allow you to exit
            // gracefully.
            nMenuInit();
            lostNetwork = true;
            return NODE_OK;
        }

        // send a udp keep alive if it's time
        sendUDPKeepAlive(frameTime);
        // send a ping if it's time
        pingSend(cInfo.nodeMillis);

    }

    return NODE_OK;
}

static void renderFrame(void)
{				/* assumes context is current*/
    nodeStatus_t rv = NODE_OK;

    /* don't render anything until we are ready */
    if (!DSP_INITED())
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
            utLog("Exiting...");
            conqend();
            exit(1);
        }
    }

    /* if we are playing back a recording, we use the current
       frame delay, else the default throttle */

    if (recFrameDelay != 0.0)
    {
        if (Context.recmode == RECMODE_PLAYING ||
            Context.recmode == RECMODE_PAUSED)
            utSleep(recFrameDelay);
        else
        {
            if (FPS > 75.0)
                utSleep(0.01);
        }
    }

    return;

}

void drawTorp(GLfloat x, GLfloat y,
              int scale, int snum, int torpnum)
{
    static const GLfloat z = 1.0;
    GLfloat size;
    int steam = cbShips[snum].team;
    GLfloat scaleFac = (scale == MAP_SR_FAC) ? dConf.vScaleSR : dConf.vScaleLR;

    /* these need to exist first... */
    if (!GLShips[0][0].ship)
        if (!initGLShips())
        {
            utLog("%s: initGLShips failed.",
                  __FUNCTION__);
            return;                 /* we need to bail here... */
        }

    /* we only draw torps for 'valid teams' */
    if (steam < 0 || steam >= NUMPLAYERTEAMS)
        return;

    size = cu2GLSize(ncpTorpAnims[steam].state.size, -scale);

    if (scale == MAP_LR_FAC)
        size = size * 2.0;

    glPushMatrix();
    glLoadIdentity();

    glScalef(scaleFac, scaleFac, 1.0);

    glTranslatef(x , y , TRANZ);

    // "directionalize" torp movement
    if (ncpTorpAnims[steam].state.angle) /* use it */
        glRotatef((GLfloat)ncpTorpAnims[steam].state.angle, 0.0, 0.0, z);
    else
        glRotatef((GLfloat)utAngle(0.0, 0.0,
                                  cbShips[snum].torps[torpnum].dx,
                                  cbShips[snum].torps[torpnum].dy),
                  0.0, 0.0, z);  /* face firing angle */

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glEnable(GL_BLEND);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ncpTorpAnims[steam].state.id);

    glColor4fv(ncpTorpAnims[steam].state.col.vec);

    drawTexBoxCentered(0.0, 0.0, z, size, false, false);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glPopMatrix();

    return;
}

/* utility func for converting a shield level into a color */
static inline uint32_t _get_sh_color(real sh)
{
    uint32_t color;
    uint8_t alpha;

    if (sh >= 0.0 && sh < 50.0)
    {
        alpha = (uint8_t)(256.0 * (sh/50.0));
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
    static int norender = false;
    int steam = cbShips[snum].team, stype = cbShips[snum].shiptype;
    real shipSize = cbShipTypes[cbShips[snum].shiptype].size;
    GLfloat scaleFac = (scale == MAP_SR_FAC) ? dConf.vScaleSR : dConf.vScaleLR;
    GLfloat phaserRadius;

    if (norender)
        return;

    /* if there's nothing available to render, no point in being here :( */
    if (!GLShips[0][0].ship)
        if (!initGLShips())
        {
            utLog("%s: initGLShips failed, bailing.",
                  __FUNCTION__);
            norender = true;
            return;                 /* we need to bail here... */
        }

    // figure out the size of things
    size = cu2GLSize(shipSize * GLTEX_PRESCALE(GLShips[steam][stype].ship),
                     -scale);
    phaserRadius = cu2GLSize(PHASER_DIST, -scale);

    /* make a little more visible in LR */
    if (scale == MAP_LR_FAC)
        size = size * 2.0;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

    /* phasers - we draw this before the ship */
    if (cbShips[snum].pfuse > 0) /* phaser action */
    {
        GLfloat phaserwidth = ((scale == MAP_SR_FAC) ? 1.5 : 0.5);

        glPushMatrix();
        glLoadIdentity();

        glScalef(scaleFac, scaleFac, 1.0);

        /* translate to correct position, */
        glTranslatef(x , y , TRANZ);
        glRotatef(cbShips[snum].lastphase - 90.0, 0.0, 0.0, z);

        glColor4fv(GLTEX_COLOR(GLShips[steam][stype].phas).vec);
        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, GLTEX_ID(GLShips[steam][stype].phas));
        glBegin(GL_POLYGON);

        /* can't use drawTexBoxCentered here since we need to change
         *  color (alpha) halfway through, and tweak the vertex coords
         *  properly for beams
         */
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-phaserwidth, 0.0, -1.0); /* ll */

        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(phaserwidth, 0.0, -1.0); /* lr */

        /* increase transparency at the end of the beam */
        glColor4f(GLTEX_COLOR(GLShips[steam][stype].phas).r,
                  GLTEX_COLOR(GLShips[steam][stype].phas).g,
                  GLTEX_COLOR(GLShips[steam][stype].phas).b,
                  0.3);

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(phaserwidth, phaserRadius, -1.0); /* ur */

        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-phaserwidth, phaserRadius, -1.0); /* ul */

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

        glBindTexture(GL_TEXTURE_2D, GLTEX_ID(GLShips[steam][stype].sh));

        /* standard sh graphic */
        uiPutColor(_get_sh_color(cbShips[snum].shields));
        /* draw the shield textures at twice the ship size */
        drawTexBoxCentered(0.0, 0.0, z, size * 2.0, false, false);

        glDisable(GL_TEXTURE_2D);
        glPopMatrix();
    }

    sprintf(buf, "%c%d", ch, snum);

    /* set a lower alpha if we are cloaked. */
    if (ch == '~')
        alpha = 0.4;		/* semi-transparent */

#if 0 && defined( DEBUG_GL )
    utLog("DRAWSHIP(%s) x = %.1f, y = %.1f, ang = %.1f\n", buf, x, y, angle);
#endif

    glPushMatrix();
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, GLTEX_ID(GLShips[steam][stype].ship));

    glScalef(scaleFac, scaleFac, 1.0);

    /* translate to correct position, */
    glTranslatef(x , y , TRANZ);

    glRotatef(angle, 0.0, 0.0, z);

    glColor4f(GLTEX_COLOR(GLShips[steam][stype].ship).r,
              GLTEX_COLOR(GLShips[steam][stype].ship).g,
              GLTEX_COLOR(GLShips[steam][stype].ship).b,
              alpha);

    drawTexBoxCentered(0.0, 0.0, z, size, false, false);

    glDisable(GL_TEXTURE_2D);

    glBlendFunc(GL_ONE, GL_ONE);

    /* highlight enemy ships... */
    if (UserConf.EnemyShipBox)
    {
        if (color == RedLevelColor || color == RedColor)
            drawLineBox(-(size / 2.0), -(size / 2.0), z, size, size,
                        RedColor, 1.0);
    }

    /* reset the matrix for the text */

    glLoadIdentity();

    glScalef(scaleFac, scaleFac, 1.0);

    glfRenderFont(x,
                  ((scale == MAP_SR_FAC) ? y - 4.0: y - 1.0),
                  TRANZ,
                  ((GLfloat)strlen(buf) * 2.0) / ((scale == MAP_SR_FAC) ? 1.0 : 2.0),
                  TEXT_HEIGHT, glfFontFixedTiny, buf, color, NULL,
                  GLF_FONT_F_SCALEX);

    glPopMatrix();

    glDisable(GL_BLEND);

    return;
}

void drawDoomsday(GLfloat x, GLfloat y, GLfloat dangle, GLfloat scale)
{
    static const GLfloat z = 1.0;
    static GLfloat doomsizeSR, doomsizeLR;
    GLfloat size, bsize;
    static GLfloat beamRadiusSR, beamRadiusLR;
    static int norender = false;  /* if no tex, no point... */
    bool drawAPBeam = false;
    static animStateRec_t doomapfire = {}; /* animdef state for ap firing */
    static int beamfx = -1;       /* Cataboligne - beam sound */
    real dis, ang;
    GLfloat scaleFac = (scale == MAP_SR_FAC) ? dConf.vScaleSR : dConf.vScaleLR;
    static uint32_t geoChangeCount = 0;

    if (norender)
        return;

    dis = dist( cbShips[Context.snum].x, cbShips[Context.snum].y,
                cbDoomsday->x, cbDoomsday->y );

    ang = utAngle(cbShips[Context.snum].x, cbShips[Context.snum].y,
                  cbDoomsday->x, cbDoomsday->y);

    /* find the textures if we haven't already */
    if (!GLDoomsday.doom)
    {                           /* init first time around */
        if ( (GLDoomsday.doom = findGLTexture("doomsday")) < 0 )
        {
            utLog("%s: Could not find the doomsday texture,  bailing.",
                  __FUNCTION__);
            norender = true;
            return;
        }

        /* find the AP beam */
        if ( (GLDoomsday.beam = findGLTexture("doombeam")) < 0 )
        {
            utLog("%s: Could not find the doombeam texture,  bailing.",
                  __FUNCTION__);
            norender = true;
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

    if (geoChangeCount != GLGeoChange)
    {
        geoChangeCount = GLGeoChange;

        /* doomsday is DOOMSIZE CU's in size */
        doomsizeSR = cu2GLSize(DOOMSIZE * GLTEX_PRESCALE(GLDoomsday.doom),
                               -MAP_SR_FAC);
        doomsizeLR = cu2GLSize(DOOMSIZE * GLTEX_PRESCALE(GLDoomsday.doom),
                               -MAP_LR_FAC);

        beamRadiusSR = cu2GLSize(DOOMSDAY_DIST * GLTEX_PRESCALE(GLDoomsday.beam),
                                 -MAP_SR_FAC);
        beamRadiusLR = cu2GLSize(DOOMSDAY_DIST * GLTEX_PRESCALE(GLDoomsday.beam),
                                 -MAP_LR_FAC);
    }

    size = ((scale == MAP_SR_FAC) ? doomsizeSR : doomsizeLR);
    bsize = ((scale == MAP_SR_FAC) ? beamRadiusSR : beamRadiusLR);

    if (scale == MAP_LR_FAC)
    {
        size = size * 2.0;
        bsize *= 2.0;
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);

#ifdef DEBUG
    utLog("DRAWDOOMSDAY(%s) x = %.1f, y = %.1f, ang = %.1f\n", buf, x, y, dangle);
#endif

    /*
      Cataboligne - fire doomsday antiproton beam!
    */

    if (DOOM_ATTACKING() && doomapfire.state.armed)
        drawAPBeam = true;
    else
        drawAPBeam = false;

    /* if it's time to draw the beam, then let her rip */
    if ( drawAPBeam )
    {
        // FIXME - should be scaled...
        // 3.0
        GLfloat beamwidth = cu2GLSize(150, -scale);

        glPushMatrix();
        glLoadIdentity();

        glScalef(scaleFac, scaleFac, 1.0);

        glTranslatef(x , y , TRANZ);
        glRotatef(cbDoomsday->heading - 90.0, 0.0, 0.0, z);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, GLTEX_ID(GLDoomsday.beam));

        glColor4fv(GLTEX_COLOR(GLDoomsday.beam).vec);

        glBegin(GL_POLYGON);

        /* can't use drawTexBoxCentered here since we need to change
         *  color (alpha) halfway through, and tweak the vertex coords
         *  properly for beams
         */
        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(-beamwidth, 0.0, -1.0); /* ll */

        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(beamwidth, 0.0, -1.0); /* lr */

        glColor4f(GLTEX_COLOR(GLDoomsday.beam).r,
                  GLTEX_COLOR(GLDoomsday.beam).g,
                  GLTEX_COLOR(GLDoomsday.beam).b,
                  GLTEX_COLOR(GLDoomsday.beam).a * 0.1);

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(beamwidth / 2.0, bsize, -1.0); /* ur */

        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-(beamwidth / 2.0), bsize, -1.0); /* ul */

        glEnd();

        glDisable(GL_TEXTURE_2D);
        glPopMatrix();

    }  /* drawAPBeam */

    /* Cataboligne - sound code 11.16.6
     * play doombeam sound
     */
    if (drawAPBeam && dis < YELLOW_DIST)
    {
        cqsEffectPlay(beamfx, NULL, YELLOW_DIST * 2, dis, ang);
    }

    glPushMatrix();
    glLoadIdentity();

    glScalef(scaleFac, scaleFac, 1.0);

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, GLTEX_ID(GLDoomsday.doom));

    glTranslatef(x , y , TRANZ);
    glRotatef(dangle, 0.0, 0.0, z);

    glColor4fv(GLTEX_COLOR(GLDoomsday.doom).vec);

    drawTexBoxCentered(0.0, 0.0, z, size, false, false);

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
   unlike the original neb rendering which was done all the time
   regardless of barrier visibility.
*/
void drawNEB(int snum)
{
    int nebXVisible = false;
    int nebYVisible = false;
    static int inited = false;
    static const GLfloat nebCenter = ((NEGENBEND_DIST - NEGENB_DIST) / 2.0);
    real nearx, neary;
    GLfloat tx, ty;
    GLfloat nebWidth, nebHeight;
    static GLfloat nebWidthSR, nebHeightSR;
    static GLfloat nebWidthLR, nebHeightLR;
    GLfloat nebX, nebY;
    static int norender = false;
    /* our wall animation state */
    static animStateRec_t nebastate;    /* initial state of neb texture */
    static uint32_t geoChangeCount = 0;

    if (norender)
        return;

    if (!inited)
    {
        inited = true;

        /* get our anim */
        if (!animInitState("neb", &nebastate, NULL))
        {
            norender = true;
            return;
        }
    }

    /* figure out appropriate width/height of neb quad in SR/LR */

    if (geoChangeCount != GLGeoChange)
    {
        geoChangeCount = GLGeoChange;

        /* width/height SR */
        GLcvtcoords(0.0, 0.0, NEGENBEND_DIST * 2.0,
                    (NEGENBEND_DIST - NEGENB_DIST),
                    MAP_SR_FAC,
                    &nebWidthSR, &nebHeightSR);

        /* width/height LR */
        GLcvtcoords(0.0, 0.0, NEGENBEND_DIST * 2.0,
                    (NEGENBEND_DIST - NEGENB_DIST),
                    MAP_LR_FAC,
                    &nebWidthLR, &nebHeightLR);

    }

    /* see if a neb wall is actually visible.  If not, we can save alot of
       cycles... */

    /* first, if we're LR and !UserConf.DoLocalLRScan, then you
       can't see it */

    if (SMAP(snum) && !UserConf.DoLocalLRScan)
        return;

    /* if we are inside the barrier, of course we
       can see it */
    if (fabs( cbShips[snum].x ) >= NEGENB_DIST &&
        fabs( cbShips[snum].x ) <= NEGENBEND_DIST)
        nebXVisible = true;

    if (fabs( cbShips[snum].y ) >= NEGENB_DIST &&
        fabs( cbShips[snum].y ) <= NEGENBEND_DIST)
        nebYVisible = true;

    if (!nebXVisible)
    {
        /* test for x wall */
        if (cbShips[snum].x < 0.0)
        {
            /* find out which side of the barrier we are on */
            if (cbShips[snum].x > (-NEGENB_DIST - nebCenter))
                nearx = -NEGENB_DIST;
            else
                nearx = -NEGENBEND_DIST;
        }
        else
        {
            if (cbShips[snum].x < (NEGENB_DIST + nebCenter))
                nearx = NEGENB_DIST;
            else
                nearx = NEGENBEND_DIST;
        }

        /* we check against a mythical Y point aligned on the nearest X
           NEB wall edge to test for visibility. */

        if (GLcvtcoords(cbShips[snum].x, cbShips[snum].y,
                        nearx,
                        CLAMP(-NEGENBEND_DIST, NEGENBEND_DIST, cbShips[snum].y),
                        (SMAP(snum) ? MAP_LR_FAC : MAP_SR_FAC),
                        &tx, &ty))
        {
            nebXVisible = true;
        }
    }

#if 0                           /* debugging test point (murisak) */
    drawPlanet( tx, ty, 34, (SMAP(snum) ? MAP_LR_FAC : MAP_SR_FAC),
                MagentaColor);
#endif

    if (!nebYVisible)
    {
        /* test for y wall */
        if (cbShips[snum].y < 0.0)
        {
            if (cbShips[snum].y > (-NEGENB_DIST - nebCenter))
                neary = -NEGENB_DIST;
            else
                neary = -NEGENBEND_DIST;
        }
        else
        {
            if (cbShips[snum].y < (NEGENB_DIST + nebCenter))
                neary = NEGENB_DIST;
            else
                neary = NEGENBEND_DIST;
        }

        /* we check against a mythical X point aligned on the nearest Y NEB wall
           edge to test for visibility. */

        if (GLcvtcoords(cbShips[snum].x, cbShips[snum].y,
                        CLAMP(-NEGENBEND_DIST, NEGENBEND_DIST, cbShips[snum].x),
                        neary,
                        (SMAP(snum) ? MAP_LR_FAC : MAP_SR_FAC),
                        &tx, &ty))
        {
            nebYVisible = true;
        }
    }
#if 0                           /* debugging test point (murisak) */
    drawPlanet( tx, ty, 34, (SMAP(snum) ? MAP_LR_FAC : MAP_SR_FAC),
                MagentaColor);
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

    glColor4fv(nebastate.state.col.vec);

    if (nebYVisible)
    {
        if (cbShips[snum].y > 0.0)
        {
            /* top */
            GLcvtcoords(cbShips[snum].x, cbShips[snum].y,
                        -NEGENBEND_DIST, NEGENB_DIST,
                        (SMAP(snum) ? MAP_LR_FAC : MAP_SR_FAC),
                        &nebX, &nebY);
#if 0
            utLog("Y TOP VISIBLE");
#endif
        }
        else
        {
            /* bottom */
            GLcvtcoords(cbShips[snum].x, cbShips[snum].y,
                        -NEGENBEND_DIST, -NEGENBEND_DIST,
                        (SMAP(snum) ? MAP_LR_FAC : MAP_SR_FAC),
                        &nebX, &nebY);
#if 0
            utLog("Y BOTTOM VISIBLE");
#endif
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
        if (cbShips[snum].x > 0.0)
        {
            /* right */
            GLcvtcoords(cbShips[snum].x, cbShips[snum].y,
                        NEGENB_DIST, -NEGENBEND_DIST,
                        (SMAP(snum) ? MAP_LR_FAC : MAP_SR_FAC),
                        &nebX, &nebY);
#if 0
            utLog("X RIGHT VISIBLE");
#endif
        }
        else
        {
            /* left */
            GLcvtcoords(cbShips[snum].x, cbShips[snum].y,
                        -NEGENBEND_DIST, -NEGENBEND_DIST,
                        (SMAP(snum) ? MAP_LR_FAC : MAP_SR_FAC),
                        &nebX, &nebY);
#if 0
            utLog("X LEFT VISIBLE");
#endif
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

    GLfloat x, y, x2, y2;
    static GLint texid_vbg = -1;
    GLfloat scaleFac = (SMAP(snum)) ? dConf.vScaleLR : dConf.vScaleSR;

    /* half-width of vbg at TRANZ */
    static const GLfloat vbgrad = NEGENBEND_DIST * 1.2;

    if (snum < 0 || snum >= cbLimits.maxShips())
        return;

    if (!GLTextures.size() || !GLShips[0][0].ship)
        return;

    /* try to init them */
    if (texid_vbg == -1)
    {
        int ndx;

        if ((ndx = findGLTexture("vbg")) >= 0)
            texid_vbg = GLTEX_ID(ndx);
        else
        {
            texid_vbg = GLTEX_ID(defaultTextureIdx);
            return;
        }
    }

    if (!dovbg)
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
        GLcvtcoords(cbShips[snum].x, cbShips[snum].y,
                    -vbgrad, -vbgrad,
                    -SFAC(snum),
                    &x, &y);
        GLcvtcoords(cbShips[snum].x, cbShips[snum].y,
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
    utLog("GL: procInput: key = %d, x = %d y = %d",
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

/* gets called for arrows, Fkeys, etc... */
static void
input(int key, int x, int y)
{
    uint32_t kmod = glutGetModifiers();
    uint32_t jmod = 0;

    if (kmod & GLUT_ACTIVE_SHIFT)
        jmod |= CQ_KEY_MOD_SHIFT;
    if (kmod & GLUT_ACTIVE_CTRL)
        jmod |= CQ_KEY_MOD_CTRL;
    if (kmod & GLUT_ACTIVE_ALT)
        jmod |= CQ_KEY_MOD_ALT;

    // we pretty much require freeglut now, and the current version
    // supports detection of the modifier (and a couple of other) key
    // presses passed to this function.  We don't use these in
    // conquest (at least currently) so detect and screen them out.

    // GLUT_KEY_INSERT is highest numbered special key in the "std"
    // repertoire, so if it's higher than that, we are dealing with
    // this situation.
    if (key > GLUT_KEY_INSERT)
        return;

    procInput(((key & CQ_CHAR_MASK) << CQ_FKEY_SHIFT) | jmod, x, y);
}

/* for 'normal' keys */
static void charInput(unsigned char key, int x, int y)
{
    uint32_t kmod = glutGetModifiers();
    uint32_t jmod = 0;

    if (kmod & GLUT_ACTIVE_SHIFT)
        jmod |= CQ_KEY_MOD_SHIFT;
    if (kmod & GLUT_ACTIVE_CTRL)
        jmod |= CQ_KEY_MOD_CTRL;
    if (kmod & GLUT_ACTIVE_ALT)
        jmod |= CQ_KEY_MOD_ALT;

    procInput((key & CQ_CHAR_MASK) | jmod, x, y);
    return;
}

/* create the 'default' GLTexture (defaultTexture) */
static void createDefaultTexture(void)
{
    GLTexture_t defaultTexture;

    memset((void *)&defaultTexture, 0, sizeof(GLTexture_t));

    /* clear all the textures */
    GLTextures.clear();

    /* 2x2 checkerboard-like (with interpolation :) */
    static GLuint GL_defaultTexImage[4] = {0x000000ff, 0xffffffff,
                                          0xffffffff, 0x000000ff};
    defaultTexture.cqiIndex = -1;
    defaultTexture.w = 2;
    defaultTexture.h = 2;

    defaultTexture.col.r = 1.0;
    defaultTexture.col.g = 1.0;
    defaultTexture.col.b = 1.0;
    defaultTexture.col.a = 1.0;

    /* create the texture */
    glGenTextures(1, (GLuint *)&defaultTexture.id);
    glBindTexture(GL_TEXTURE_2D, defaultTexture.id);
    GLError();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 defaultTexture.w, defaultTexture.h, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, GL_defaultTexImage);
    GLError();

    // always the first texture, index 0!
    GLTextures.push_back(defaultTexture);
    return;
}

/* try to instantiate a texture using a PROXY_TEXTURE_2D to see if the
   implementation can handle it. */
static int checkTexture(const char *filename, textureImage *texture)
{
    GLint type, components, param;

    GLError();                    /* clear any accumulated GL errors.
                                     If errors are reported in the logfile here,
                                     then the error occured prior to calling
                                     this function.  Look there :) */

    if (texture->components == 4)
    {
        type = GL_RGBA;
        components = GL_RGBA8;
    }
    else if (texture->components == 3)
    {
        type = GL_RGB;
        components = GL_RGB8;
    }
    else if (texture->components == 2)
    {
        type = GL_LUMINANCE_ALPHA;
        components = GL_LUMINANCE8_ALPHA8;
    }
    else
    {
        utLog("%s: %s: ERROR: Unsupported number of components: %d",
              __FUNCTION__, filename, texture->components);
        return false;
    }

    glTexImage2D(GL_PROXY_TEXTURE_2D, 0, components,
                 texture->width, texture->height, 0,
                 type, GL_UNSIGNED_BYTE, texture->imageData);


    glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0,
                             GL_TEXTURE_WIDTH, &param);

    if (GLError() || !param)
    {
        utLog("%s: ERROR: Texture too big, or non power of two in width or height).",
              filename);
        return false;
    }

    return true;
}

// load an image file into a raw "pre-texture".  Pretty simple with
// stb_image :)
static bool loadImageFile(const char *filename, textureImage *texture)
{
    if (!filename || !texture)
        return false;

    stbi_set_flip_vertically_on_load(true);
    texture->imageData = stbi_load(filename,
                                   &(texture->width),
                                   &(texture->height),
                                   &(texture->components),
                                   0);
    if (!texture->imageData)
    {
        utLog("%s: failed to load texture %s", __FUNCTION__,
              filename);
        return false;
    }

    if (cqDebug > 2)
        utLog("%s: %s: w: %d, h: %d, components: %d",
              __FUNCTION__, filename, texture->width,
              texture->height, texture->components);

    /* now test to see if the implementation can handle the texture */

    if (!checkTexture(filename, texture))
    {
        stbi_image_free(texture->imageData);
        texture->imageData = NULL;
        return false;
    }

    return true;
}

/* look for a texture file and return a file name.  We look
   first in the users ~/.conquest/img/ dir (allowing users to override
   the pre-defined textures), then in CONQSHARE/img.  Return NULL if not
   found */
static char *_getTexFile(char *tfilenm)
{
    char *homevar;
    FILE *fd;
    static char buffer[BUFFER_SIZE_256];

    /* look for a user image */
    if ((homevar = getenv(CQ_USERHOMEDIR)))
    {
        snprintf(buffer, sizeof(buffer), "%s/%s/img/%s.png",
                 homevar, CQ_USERCONFDIR, tfilenm);

        if ((fd = fopen(buffer, "r")))
        {                       /* found one */
            fclose(fd);
            return buffer;
        }
    }

    /* if we are here, look for the system one */
    snprintf(buffer, sizeof(buffer), "%s/img/%s.png",
             utGetPath(CONQSHARE), tfilenm);

    if ((fd = fopen(buffer, "r")))
    {                       /* found one */
        fclose(fd);
        return buffer;
    }

    return NULL;
}

static int loadGLTextures()
{
    int rv = false;
    textureImage *texti = NULL;
    int i, type, components;         /* for RGBA */
    char *filenm;
    GLTexture_t curTexture;
    GLTexture_t *texptr = NULL;
    int hwtextures = 0;

    if (!cqiNumTextures || !cqiTextures)
    {                           /* we have a problem */
        utLog("%s: ERROR: cqiNumTextures or cqiTextures is 0! No textures loaded.\n",
              __FUNCTION__);
        return false;
    }

    // This function clear()'s the GLTextures vector, and adds the
    // defaultTexture as the first texture, index 0.
    createDefaultTexture();

    /* now try to load each texture and setup the proper data */
    for (i=0; i<cqiNumTextures; i++)
    {
        int texid = 0, texw = 0, texh = 0;
        int ndx = -1;
        int col_only = false;     /* color-only texture? */

        memset((void *)&curTexture, 0, sizeof(GLTexture_t));
        texid = 0;
        texw = texh = 0;          /* default width/height */
        rv = false;

        if (cqiTextures[i].flags & CQITEX_F_COLOR_SPEC)
            col_only = true;

        /* first see if a texture with the same filename was already loaded.
           if so, no need to do it again, just copy the previously loaded
           data */
        if (GLTextures.size() && !col_only &&
            (ndx = findGLTextureByFile(cqiTextures[i].filename,
                                       cqiTextures[i].flags )) >= 0)
        {                       /* the same hw texture was previously loaded
                                   just save it's texture id and w/h */
            texid = GLTEX_ID(ndx);
            texw  = GLTEX_WIDTH(ndx);
            texh  = GLTEX_HEIGHT(ndx);

            if (cqDebug > 1)
                utLog("%s: texture file '%s' already loaded, using existing tid.",
                      __FUNCTION__, cqiTextures[i].filename);
        }

        if (!texid && !col_only)
        {
            texti = (textureImage *)malloc(sizeof(textureImage));

            if (!texti)
            {
                utLog("%s: texti malloc(%lu) failed\n",
                      __FUNCTION__, sizeof(textureImage));
                return false;
            }

            memset((void *)texti, 0, sizeof(textureImage));

            /* look for a suitable file */
            if (!(filenm = _getTexFile(cqiTextures[i].filename)))
            {
                rv = false;
            }
            else if ((rv = loadImageFile(filenm, texti)))
            {
                /* create the texture */
                glGenTextures(1, (GLuint *)&curTexture.id);
                glBindTexture(GL_TEXTURE_2D, curTexture.id);
                GLError();

                curTexture.w = texti->width;
                curTexture.h = texti->height;

                if (texti->components == 4)
                {
                    type = GL_RGBA;
                    if (cqiTextures[i].flags & CQITEX_F_IS_LUMINANCE)
                        components = GL_LUMINANCE8_ALPHA8;
                    else
                        components = GL_RGBA8;
                }
                else if (texti->components == 2)
                {
                    // Always a luminance (with alpha) texture
                    type = GL_LUMINANCE_ALPHA;
                    components = GL_LUMINANCE8_ALPHA8;
                }
                else
                {
                    // fallback, assume components == 3 (RGB), can't
                    // be anything else at this point
                    type = GL_RGB;
                    if (cqiTextures[i].flags & CQITEX_F_IS_LUMINANCE)
                        components = GL_LUMINANCE8;
                    else
                        components = GL_RGB8;
                }

                /* use linear filtering */
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

                /* gen mipmaps if requested */
                if (cqiTextures[i].flags & CQITEX_F_GEN_MIPMAPS)
                {
                    /* this is highest quality.  It should be configurable. */
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR_MIPMAP_LINEAR);

                    gluBuild2DMipmaps(GL_TEXTURE_2D, components,
                                      texti->width, texti->height,
                                      type, GL_UNSIGNED_BYTE, texti->imageData);
                }
                else
                {
                    /* just generate the texture */
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                    GL_LINEAR);
                    glTexImage2D(GL_TEXTURE_2D, 0, components,
                                 texti->width, texti->height, 0,
                                 type, GL_UNSIGNED_BYTE, texti->imageData);
                }

                hwtextures++;
            } /* if rv */

            GLError();

            /* be free! */
            if (texti && rv)
            {
                if (texti->imageData)
                    stbi_image_free(texti->imageData);

            }

            free(texti);
            texti = NULL;
        }

        if (rv || texid || col_only)     /* tex/color load/locate succeeded,
                                            add it to the list */
        {
            /* now set it up */
            if (texid)
            {
                curTexture.id = texid;
                curTexture.w = texw;
                curTexture.h = texh;
            }

            curTexture.cqiIndex = i;
            curTexture.prescale = cqiTextures[i].prescale;
            hex2GLColor(cqiTextures[i].color, &curTexture.col);

            GLTextures.push_back(curTexture);
        }
    }

    utLog("%s: Successfully loaded %ld textures, (%d files).",
          __FUNCTION__, GLTextures.size(), hwtextures);

    return true;
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

    GLGeoChange++;

    return;
}
