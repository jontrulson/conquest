/* 
 * higher level rendering for the CP
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"
#include "datatypes.h"
#include "color.h"
#include "conf.h"
#include "conqcom.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "record.h"
#include "ui.h"
#include <GL/glut.h>

#include "glmisc.h"
#include "glfont.h"

#include "render.h"

extern dspData_t dData;

/* blink status */
static int cloakbon = FALSE;    /* blinking - high */
static int alertbon = FALSE;

static int cloakbtime = 0;           /* last cloak blink */
static int alertbtime = 0;           /* last alert blink */

void renderHud(void)
{				/* assumes context is current*/
  GLfloat tx, ty;
  GLfloat sb_ih;                /* stat box item height */
  char buf1024[1024];
  char fbuf[128];
  GLfloat xstatstart = dConf.borderW;
  GLfloat xstatend = (GLfloat)(dConf.vX - dConf.borderW);
  GLfloat xstatw = xstatend - xstatstart;
  float FPS = getFPS();
  int gnow = glutGet(GLUT_ELAPSED_TIME);
  static int rxtime = 0;
  static int oldrx = 0;
  static int rxdiff = 0;

  /* draw fps/bps... */

  if ((gnow - rxtime) > 1000)
    {
      rxdiff = pktRXBytes - oldrx;
      oldrx = pktRXBytes;
      rxtime = gnow;
    }

  if (FPS > 999.0)
    sprintf(fbuf, "FPS: 999+");
  else
    sprintf(fbuf, "FPS: %3.1f", FPS);

  sprintf(buf1024, "%3.1fKB/s       %s", ((float)rxdiff / 1000.0),
          fbuf);

  glColor3f(0.7, 0.7, 0.7);
  drawString((dConf.wW - 
              (strlen(buf1024) * mFontTinyW)),
             dConf.wH - dConf.borderW,
             0.0,
             buf1024, 
             mFontTinyDL,
             -1, TRUE);

  GLError();

  /* check the blinkers */
  if ((gnow - cloakbtime) > 1000)
    {                           /* 1 sec */
      cloakbon = !cloakbon;
      cloakbtime = gnow;
    }

  if ((gnow - alertbtime) > 500)
    {                           /* .5 sec */
      alertbon = !alertbon;
      alertbtime = gnow;
    }

  /* draw alert border */
  drawLineBox(dConf.vX - 3.0, dConf.vY - 3.0, 
              dConf.vW + (3.0 * 2.0),
              dConf.vH + (3.0 * 2.0),
              dData.aBorder.alertColor,
              2.0);

  tx = dConf.borderW;
  ty = dConf.borderW;

  /* heading */
  glfRender(tx + 2.0, ty, 0.0, ((xstatw / 6.0) * 3.0), 
             (dConf.vH / 15.0),
             fontLargeTxf, dData.heading.heading, NoColor, TRUE, FALSE, TRUE);
  drawLineBox(tx, ty, ((xstatw / 6.0) * 3.0) + 4.0, 
              (dConf.vH / 15.0) + 4.0,
              NoColor, 1.0);

  glfRender(tx + 2.0, ty + (dConf.vH / 15.0), 0.0, 
            ((xstatw / 6.0) * 3.0),
            (dConf.vH / 20.0),
            fontFixedTxf, "Heading", LabelColor, TRUE, FALSE, TRUE);
  
  tx = tx + ((xstatw / 6.0) * 4.0);
  /* warp */
  glfRender(tx + 2.0, ty, 0.0, 
            xstatw - (tx - 2.0), 
             (dConf.vH / 15.0),
             fontLargeTxf, dData.warp.warp, InfoColor, TRUE, FALSE, TRUE);
  drawLineBox(tx, ty, xstatw - tx + 2.0, 
              (dConf.vH / 15.0) + 4.0,
              InfoColor, 1.0);

  glfRender(tx + 2.0, 
            ty + (dConf.vH / 15.0), 
            0.0, 
            xstatw - (tx - 2.0), 
            (dConf.vH / 20.0),
            fontFixedTxf, "Warp", LabelColor, TRUE, FALSE, TRUE);

  /* shields */
  tx = dConf.borderW;
  ty = (dConf.vH / 15.0) * 2.5;
  renderScale(tx, ty, xstatw, (dConf.vH / 15.0),
              0, 100, dData.sh.shields, dData.sh.label, 
              dData.sh.lcolor, dData.sh.color, fontLargeTxf,
              fontFixedTxf);

  /* damage */
  tx = dConf.borderW;
  ty = (dConf.vH / 15.0) * 4.0;
  renderScale(tx, ty, xstatw, (dConf.vH / 15.0),
              0, 100, dData.dam.damage, dData.dam.label, 
              dData.dam.lcolor, dData.dam.color, fontLargeTxf,
              fontFixedTxf);

  /* fuel */
  tx = dConf.borderW;
  ty = (dConf.vH / 15.0) * 5.5;
  renderScale(tx, ty, xstatw, (dConf.vH / 15.0),
              0, 999, dData.fuel.fuel, dData.fuel.label, 
              dData.fuel.lcolor, dData.fuel.color, fontLargeTxf,
              fontFixedTxf);

  /* etemp */
  tx = dConf.borderW;
  ty = (dConf.vH / 15.0) * 7.0;
  renderScale(tx, ty, xstatw, (dConf.vH / 15.0),
              0, 100, dData.etemp.etemp, dData.etemp.label, 
              dData.etemp.lcolor, dData.etemp.color, fontLargeTxf,
              fontFixedTxf);

  /* wtemp */
  tx = dConf.borderW;
  ty = (dConf.vH / 15.0) * 8.5;
  renderScale(tx, ty, xstatw, (dConf.vH / 15.0),
              0, 100, dData.wtemp.wtemp, dData.wtemp.label, 
              dData.wtemp.lcolor, dData.wtemp.color, fontLargeTxf,
              fontFixedTxf);

  /* alloc */
  tx = dConf.borderW;
  ty = (dConf.vH / 15.0) * 10.0;
  renderAlloc(tx, ty, xstatw, (dConf.vH / 15.0),
            &dData.alloc, fontLargeTxf, fontFixedTxf);

  
  /* BEGIN "stat" box -
     kills, towing/towed by, x armies, CLOAKED/destruct */
     
  ty = (dConf.vH / 15.0) * 11.5; /* top of stat box*/
  sb_ih = ((dConf.vY + dConf.vH) - ty) / 5.0; /* hwight per item */

  /* kills */
  sprintf(buf1024, "#%d#%5s #%d#kills", InfoColor, dData.kills.kills,
          CyanColor);
  glfRender(tx + 2.0, ty, 0.0, xstatw, sb_ih, fontFixedTxf, buf1024, NoColor,
            TRUE, TRUE, TRUE);

  /* towed/towing */
  if (strlen(dData.tow.str))
      glfRender(tx + 2.0, ty + sb_ih, 0.0, xstatw, sb_ih, 
                 fontFixedTxf, dData.tow.str, MagentaColor, TRUE, FALSE, TRUE);

  /* armies */
  if (strlen(dData.armies.str))
    glfRender(tx + 2.0, ty + (sb_ih * 2.0), 0.0, xstatw, sb_ih,
               fontFixedTxf, dData.armies.str, InfoColor, TRUE, FALSE, TRUE);

  /* cloak/Destruct */
  if (strlen(dData.cloakdest.str))
    glfRender(tx + 2.0, ty + (sb_ih * 3.0), 0.0, xstatw, sb_ih,
              fontFixedTxf, dData.cloakdest.str, 
              (cloakbon) ? dData.cloakdest.color : dData.cloakdest.color | CQC_A_BOLD, 
              TRUE, FALSE, TRUE);

  /* alert stat */
  if (strlen(dData.aStat.alertStatus))
    glfRender(tx + 2.0, ty + (sb_ih * 4.0), 0.0, xstatw, sb_ih,
              fontFixedTxf, dData.aStat.alertStatus, 
              (alertbon) ? dData.aStat.color & ~CQC_A_BOLD : 
              dData.aStat.color | CQC_A_BOLD, 
              TRUE, FALSE, TRUE);

  /* END stat box */

  tx = dConf.vX;
  ty = dConf.vY + dConf.vH;
  sb_ih = (dConf.wH - (dConf.borderW * 2.0) - ty) / 4.0;
  
  if ((Context.recmode != RECMODE_PLAYING) &&
      (Context.recmode != RECMODE_PAUSED))
    {
      /* althud */
      if (UserConf.AltHUD)
        glfRender(tx, ty + (sb_ih * 0.2), 
                  0.0, 
                  (dConf.vX + dConf.vW) - tx, sb_ih * 0.7,
                  fontFixedTxf, dData.xtrainfo.str, InfoColor, TRUE, TRUE, TRUE);
    }
  else
    {
      /* for playback, the ship/item we are watching */
      glfRender(tx, ty + (sb_ih * 0.2), 
                0.0, 
                (dConf.vX + dConf.vW) - tx, sb_ih * 0.7,
                fontFixedTxf, dData.recId.str, 
                MagentaColor | CQC_A_BOLD, 
                TRUE, TRUE, TRUE);

      tx = dConf.borderW;

      glfRender(tx, ty + (sb_ih * 0.2), 0.0, xstatw, sb_ih * 0.7,
                fontFixedTxf, dData.recTime.str, NoColor, TRUE, TRUE, TRUE);
    }


  tx = dConf.borderW;

  /* the 3 prompt/msg lines  */

  /* MSG_LIN1 */
  if (strlen(dData.p1.str))
    glfRender(tx, ty + (sb_ih * 1.0), 0.0, 
              dConf.wW  - (dConf.borderW * 2.0), sb_ih,
               fontFixedTxf, dData.p1.str, InfoColor, TRUE, TRUE, TRUE);

  /* MSG_LIN2 */
  if (strlen(dData.p2.str))
    glfRender(tx, ty + (sb_ih * 2.0), 0.0,
              dConf.wW  - (dConf.borderW * 2.0), sb_ih,
               fontFixedTxf, dData.p2.str, InfoColor, TRUE, TRUE, TRUE);

  /* MSG_MSG */
  if (strlen(dData.msg.str))
    glfRender(tx, ty + (sb_ih * 3.0), 0.0, 
              dConf.wW  - (dConf.borderW * 2.0), sb_ih,
               fontMsgTxf, dData.msg.str, InfoColor, TRUE, TRUE, TRUE);

  return;

}

void renderViewer(int dovbg)
{
  /* setup the proper viewport and projection matrix for the viewer */
  glViewport(dConf.vX, 
             dConf.vY + (dConf.wH - dConf.vH - (dConf.borderW * 2.0)), 
             dConf.vW, 
             dConf.vH);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(dConf.vmat);
  glMatrixMode(GL_MODELVIEW);
  
#if 0
   /* TEST */
  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glColor4f(0.2, 0.2, 0.2, 1.0);
  drawQuad(-((GLfloat)dConf.vW / 2.0), 
           -((GLfloat)dConf.vH / 2.0), 
           (GLfloat)dConf.vW, (GLfloat)dConf.vH, TRANZ/ 2.0);
  glDisable(GL_BLEND);
#endif


  if (dovbg)
    drawViewerBG(Context.snum);

  display( Context.snum, FALSE );

  /* reset for everything else */
  glViewport(0, 0, dConf.wW, dConf.wH);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(dConf.hmat);
  glMatrixMode(GL_MODELVIEW);

  return;
}

void renderScale(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
                 int min, int max, int val, char *label,
                 int lcolor, int scalecolor, 
                 TexFont *lfont, TexFont *vfont)
{
  GLfloat valxoff = (w / 10.0) * 3.0;
  GLfloat scaleend = x + w - valxoff - (w / 10.0);
  GLfloat scaleh = h / 2.0;
  char buf32[32];
  
  if (val < min)
    val = min;
  if (val > max)
    val = max;

  sprintf(buf32, "%3d", val);

  /* gauge */
  uiPutColor(scalecolor);
  drawQuad(x, y, 
           scaleend * (GLfloat)((GLfloat)val / ((GLfloat)max - (GLfloat)min)),
           scaleh, 0.0);
  drawLineBox(x, y, scaleend, scaleh, scalecolor, 1.0);

  /* label */
  glfRender(x, y + scaleh, 0.0, w/2.0, scaleh, vfont, label, lcolor, 
            TRUE, FALSE,
            TRUE);

  /* value */
  glfRender(x + ((w / 10.0) * 7.5), 
            y, 
            0.0,
            (w / 10.0) * 2.5, 
            h * 0.8, vfont, buf32, scalecolor, TRUE, FALSE, TRUE);

  drawLineBox(x + ((w / 10.0) * 7.5),
              y,
              (w / 10.0) * 2.5,
              h,
              BlueColor,
              1.0);

  return;
}

/* like renderScale, but more specific */
void renderAlloc(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
               struct _alloc *a,
               TexFont *lfont, TexFont *vfont)
{
  GLfloat valxoff = (w / 10.0) * 3.0;
  GLfloat scaleend = x + w - valxoff - (w / 10.0);
  GLfloat scaleh = h / 2.0;
  char buf32[32];
  
  sprintf(buf32, "%5s", a->allocstr);

  /* bg */
  if (a->ealloc > 0)
    uiPutColor(BlueColor);
  else
    uiPutColor(RedLevelColor);

  drawQuad(x, y, scaleend, scaleh, 0.0);

  /* fg */
  if (a->walloc > 0)
    uiPutColor(NoColor);
  else
    {
      uiPutColor(RedLevelColor);
      a->walloc = 100 - a->ealloc;
    }

  drawQuad(x, y, 
           scaleend * (GLfloat)((GLfloat)a->walloc / 100.0),
           scaleh, 0.0);

  uiPutColor(NoColor);
  drawLineBox(x, y, scaleend, scaleh, BlueColor, 1.0);

  /* label */
  glfRender(x, y + scaleh, 0.0, scaleend, scaleh, vfont, a->label, 
            NoColor, TRUE, FALSE, TRUE);

  /* value */
  glfRender(x + ((w / 10.0) * 7.5), 
            y, 
            0.0,
            (w / 10.0) * 2.5, 
            h * 0.8, vfont, buf32, InfoColor, TRUE, FALSE, TRUE);

  drawLineBox(x + ((w / 10.0) * 7.5),
              y,
              (w / 10.0) * 2.5,
              h,
              BlueColor,
              1.0);

  return;
}
