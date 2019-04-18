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
#include "conqutil.h"
#include "ibuf.h"
#include "gldisplay.h"
#include "conf.h"
#include "node.h"

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>

#if !defined(MINGW)
#include <GL/glx.h>
#endif

#include "ui.h"

#include "glfont.h"
#include "glmisc.h"
#include "GL.h"
#include "gldisplay.h"
#include "textures.h"

#include "cqsound.h"


int _GLError(const char *funcname, const char *filename, int line)
{
    int i;
    int rv = false;

    while ((i = glGetError()) != GL_NO_ERROR)
    {
        utLog("GL ERROR: %s:%s@%d: %s\n",
              filename, funcname, line, gluErrorString(i));
        rv = 1;
    }

    return rv;
}

/*
 * Cataboligne - multi lingual giant conquest title
 */

void mglConqLogo(dspConfig_t *dsp, bool useGLTeamLogo)
{
    static const char *Conquest = "CONQUEST";
    GLfloat x, y, w, h;
    static int firstime = true;
    static GLint logos[NUM_PLAYERTEAMS]; /* texid for each team */
    static GLColor_t colors[NUM_PLAYERTEAMS]; /* colors for each */

    h = dsp->ppRow * 5.0;
    w = (dsp->wW * 0.70);

    x = dsp->wW * 0.15;
    y = dsp->ppRow;


    if (firstime)
    {                           /* init the texids */
        int i;
        char buffer[CQI_NAMELEN];

        firstime = false;

        for (i=0; i<NUM_PLAYERTEAMS; i++)
        {
            int ndx;
            snprintf(buffer, CQI_NAMELEN, "conqlogo%c",
                     TEAM_PLAYERCHARS[i]);

            if ((ndx = findGLTexture(buffer)) >= 0)
            {
                logos[i] = GLTextures[ndx].id;
                if (HAS_GLCOLOR(GLTextures[ndx].col))
                    colors[i] = GLTextures[ndx].col;
                else
                    hex2GLColor(0xffe6e6e6, &colors[i]);
            }
            else
            {
                logos[i] = 0;
                utLog("%s: could not find texture '%s'\n",
                      __FUNCTION__, buffer);
            }
        }

    }

    /* Cataboligne - new title set */
    // make sure this only renders when a common block is mapped...
    if ( cbIsMapped() && logos[cbShips[Context.snum].team] && useGLTeamLogo)
    {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glEnable(GL_BLEND);

        glEnable(GL_TEXTURE_2D);

        glBindTexture(GL_TEXTURE_2D, logos[cbShips[Context.snum].team]);

        glColor4f(colors[cbShips[Context.snum].team].r,
                  colors[cbShips[Context.snum].team].g,
                  colors[cbShips[Context.snum].team].b,
                  colors[cbShips[Context.snum].team].a);

        drawTexQuad(x, y, 0.0, w, h, true, false);

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
    }
    else
    {
        glfRenderFont(x, y, 0.0, w, h, glfFontLarge, Conquest, RedLevelColor,
                      NULL, GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
    }

    drawLineBox(x, y + (h / 20.0), 0.0,
                w,
                h,
                NoColor,
                1.0);

    return;
}

/* For mingw, we cannot use X11 bell obviously, so we do not use a bell
 * at all for these systems
 */

void mglBeep(int type)
{
#if !defined(MINGW)
    static Display *dpy = NULL;
#endif
    time_t i;
    static time_t old = 0, oldmsg = 0, olderr = 0;
    static int beep_alertfx = -1;
    static int beep_msgfx = -1;
    static int beep_errfx = -1;

    i = time(0);

    /* the effects way */
    if (CQS_ISENABLED(CQS_EFFECTS))
    {
        if (beep_alertfx == -1)
            beep_alertfx = cqsFindEffect("beep-alert");
        if (beep_msgfx == -1)
            beep_msgfx = cqsFindEffect("beep-msg");
        if (beep_errfx == -1)
            beep_errfx = cqsFindEffect("beep-err");

        switch (type)
        {
        case MGL_BEEP_ERR:
            if (i != olderr)
            {
                cqsEffectPlay(beep_errfx, NULL, 0, 0, 0);
                olderr = i;
            }
            break;
        case MGL_BEEP_MSG:
            if (i != oldmsg)
            {
                cqsEffectPlay(beep_msgfx, NULL, 0, 0, 0);
                oldmsg = i;
            }
            break;
        case MGL_BEEP_ALERT:
        default:
            if (i != old)
            {
                cqsEffectPlay(beep_alertfx, NULL, 0, 0, 0);
                old = i;
            }
            break;
        }
    }
#if !defined(MINGW)
    else if (old != i)
    {
        /* this is cheesy, but glut does not provide a bell. */

        old = i;

        if (dpy == NULL)
            dpy = XOpenDisplay(NULL);

        if (dpy)
        {
            XBell(dpy, 0);
            XFlush(dpy);
        }
    }
#endif

    return;
}

/* draw a window sized, semi-opaque quad for overlay nodes */
void mglOverlayQuad(void)
{
    if (getTopONode())
    {                           /* if an overlay node is active, fade
                                   the CP some */
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glColor4f(0.0, 0.0, 0.0, 0.5);
        drawQuad((GLfloat)dConf.wX, (GLfloat)dConf.wY, (GLfloat)dConf.wW,
                 (GLfloat)dConf.wH,
                 0.0);
        glDisable(GL_BLEND);
    }

    return;
}

