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

/* we store geometry here that should only need to be recomputed
   when the screen is resized, not every single frame :) */

typedef struct _obj {
  GLfloat x, y;
  GLfloat w, h;
} obj_t;

static struct {
  GLfloat x, y;                 /* stored x/y/w/h of game window */
  GLfloat w, h;

  GLfloat xstatw;               /* width of status area */

  GLfloat sb_ih;                /* statbox item height */

  obj_t alertb;                 /* alert border */

  obj_t head;
  obj_t headb;
  obj_t headl;

  obj_t warp;
  obj_t warpb;
  obj_t warpl;

  obj_t sh;
  obj_t dam;
  obj_t fuel;
  obj_t etemp;
  obj_t wtemp;
  obj_t alloc;

  /* 'stat' box */
  obj_t kills;
  obj_t tow;
  obj_t arm;
  obj_t cloakdest;
  obj_t alert;

  obj_t althud;
  obj_t rectime;

  /* msg/prompt lines */
  obj_t msg1;
  obj_t msg2;
  obj_t msgmsg;
} o = {};

/* blink status */
static int cloakbon = FALSE;    /* blinking - high */
static int alertbon = FALSE;

static int cloakbtime = 0;           /* last cloak blink */
static int alertbtime = 0;           /* last alert blink */

/* update hud geometry if stuff changes (resize).  Just return if everything's
   in order*/
void updateHudGeo(void)
{				/* assumes context is current*/
  GLfloat tx, ty;

  if (o.x == dConf.wX && o.y == dConf.wY && o.w == dConf.wW &&
      o.h == dConf.wH)
    return;                     /* the universe is fine */

                                /* else, it is not, and must be fixed :) */
  o.x = dConf.wX;
  o.y = dConf.wY;
  o.w = dConf.wW;
  o.h = dConf.wH;

  o.xstatw = (dConf.vX - (dConf.borderW * 2.0));

  o.alertb.x = dConf.vX - 3.0;
  o.alertb.y = dConf.vY - 3.0;
  o.alertb.w = dConf.vW + (3.0 * 2.0);
  o.alertb.h = dConf.vH + (3.0 * 2.0);

  tx = dConf.borderW;
  ty = dConf.borderW;

  o.head.x = tx + 2.0;
  o.head.y = ty;
  o.head.w = ((o.xstatw / 6.0) * 3.0);
  o.head.h = (dConf.vH / 15.0);

  o.headb.x = tx;
  o.headb.y = ty;
  o.headb.w = ((o.xstatw / 6.0) * 3.0) + 4.0;
  o.headb.h = (dConf.vH / 15.0) + 4.0;

  o.headl.x = tx + 2.0;
  o.headl.y = ty + (dConf.vH / 15.0);
  o.headl.w = ((o.xstatw / 6.0) * 3.0);
  o.headl.h = (dConf.vH / 20.0);

  tx = tx + ((o.xstatw / 6.0) * 4.0);

  o.warp.x = tx + 2.0;
  o.warp.y = ty;
  o.warp.w = o.xstatw - (tx - 2.0);
  o.warp.h = (dConf.vH / 15.0);

  o.warpb.x = tx;
  o.warpb.y = ty;
  o.warpb.w = o.xstatw - (tx - 2.0);
  o.warpb.h = (dConf.vH / 15.0);

  o.warpl.x = tx + 2.0;
  o.warpl.y = ty + (dConf.vH / 15.0);
  o.warpl.w = o.xstatw - (tx - 2.0);
  o.warpl.h = (dConf.vH / 20.0);

  tx = dConf.borderW;
  ty = (dConf.vH / 15.0) * 2.5;

  o.sh.x = tx;
  o.sh.y = ty;
  o.sh.w = o.xstatw;
  o.sh.h = (dConf.vH / 15.0);

  ty = (dConf.vH / 15.0) * 4.0;

  o.dam.x = tx;
  o.dam.y = ty;
  o.dam.w = o.xstatw;
  o.dam.h =  (dConf.vH / 15.0);

  ty = (dConf.vH / 15.0) * 5.5;

  o.fuel.x = tx;
  o.fuel.y = ty;
  o.fuel.w = o.xstatw;
  o.fuel.h =  (dConf.vH / 15.0);

  ty = (dConf.vH / 15.0) * 7.0;

  o.etemp.x = tx;
  o.etemp.y = ty;
  o.etemp.w = o.xstatw;
  o.etemp.h =  (dConf.vH / 15.0);

  ty = (dConf.vH / 15.0) * 8.5;

  o.wtemp.x = tx;
  o.wtemp.y = ty;
  o.wtemp.w = o.xstatw;
  o.wtemp.h =  (dConf.vH / 15.0);

  ty = (dConf.vH / 15.0) * 10.0;

  o.alloc.x = tx;
  o.alloc.y = ty;
  o.alloc.w = o.xstatw;
  o.alloc.h =  (dConf.vH / 15.0);

  ty = (dConf.vH / 15.0) * 11.5; /* top of stat box*/
  o.sb_ih = ((dConf.vY + dConf.vH) - ty) / 5.0; /* height per item */

  o.kills.x = tx + 2.0;
  o.kills.y = ty;
  o.kills.w = o.xstatw;
  o.kills.h = o.sb_ih;

  o.tow.x = tx + 2.0;
  o.tow.y = ty + o.sb_ih;
  o.tow.w = o.xstatw;
  o.tow.h = o.sb_ih;

  o.arm.x = tx + 2.0;
  o.arm.y = ty + (o.sb_ih * 2.0);
  o.arm.w = o.xstatw;
  o.arm.h = o.sb_ih;

  o.cloakdest.x = tx + 2.0;
  o.cloakdest.y = ty + (o.sb_ih * 3.0);
  o.cloakdest.w = o.xstatw;
  o.cloakdest.h = o.sb_ih;

  o.alert.x = tx + 2.0;
  o.alert.y = ty + (o.sb_ih * 4.0);
  o.alert.w = o.xstatw;
  o.alert.h = o.sb_ih;

  tx = dConf.vX;
  ty = dConf.vY + dConf.vH;
  o.sb_ih = (dConf.wH - (dConf.borderW * 2.0) - ty) / 4.0;
  
  o.althud.x = tx;
  o.althud.y = ty + (o.sb_ih * 0.2);
  o.althud.w = (dConf.vX + dConf.vW) - tx;
  o.althud.h =  o.sb_ih * 0.7;

  tx = dConf.borderW;

  o.rectime.x = tx;
  o.rectime.y = ty + (o.sb_ih * 0.2);
  o.rectime.w =  o.xstatw;
  o.rectime.h =  o.sb_ih * 0.7;

  /* the 3 prompt/msg lines  */

  o.msg1.x = tx;
  o.msg1.y = ty + (o.sb_ih * 1.0);
  o.msg1.w = dConf.wW  - (dConf.borderW * 2.0);
  o.msg1.h = o.sb_ih;

  o.msg2.x = tx;
  o.msg2.y = ty + (o.sb_ih * 2.0);
  o.msg2.w = dConf.wW  - (dConf.borderW * 2.0);
  o.msg2.h = o.sb_ih;

  o.msgmsg.x = tx;
  o.msgmsg.y = ty + (o.sb_ih * 3.0);
  o.msgmsg.w = dConf.wW  - (dConf.borderW * 2.0);
  o.msgmsg.h = o.sb_ih;

  return;

}

void renderHud(int dostats)
{				/* assumes context is current*/
  char buf1024[1024];
  char sbuf1024[1024];
  char fbuf[128];
  float FPS = getFPS();
  int gnow = glutGet(GLUT_ELAPSED_TIME);
  static int rxtime = 0;
  static int oldrx = 0;
  static int rxdiff = 0;

  updateHudGeo();

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

  sprintf(sbuf1024, "%3.1fKB/s %s", ((float)rxdiff / 1000.0),
          fbuf);

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
  drawLineBox(o.alertb.x, o.alertb.y, 
              o.alertb.w,
              o.alertb.h,
              dData.aBorder.alertColor,
              2.0);

  /* heading */
  glfRender(o.head.x, o.head.y, 0.0, o.head.w, 
            o.head.h,
            fontLargeTxf, dData.heading.heading, NoColor, TRUE, FALSE, TRUE);
  drawLineBox(o.headb.x, o.headb.y, o.headb.w, 
              o.headb.h,
              NoColor, 1.0);
  
  glfRender(o.headl.x, o.headl.y, 0.0, 
            o.headl.w,
            o.headl.h,
            fontFixedTxf, "Heading", LabelColor, TRUE, FALSE, TRUE);
  
  /* warp */
  glfRender(o.warp.x, o.warp.y, 0.0, 
            o.warp.w, 
             o.warp.h,
             fontLargeTxf, dData.warp.warp, InfoColor, TRUE, FALSE, TRUE);
  drawLineBox(o.warpb.x, o.warpb.y, o.warpb.w, 
              o.warpb.h,
              InfoColor, 1.0);

  glfRender(o.warpl.x, 
            o.warpl.y, 
            0.0, 
            o.warpl.w, 
            o.warpl.h,
            fontFixedTxf, "Warp", LabelColor, TRUE, FALSE, TRUE);

  /* shields */
  renderScale(o.sh.x, o.sh.y, o.sh.w, o.sh.h,
              0, 100, dData.sh.shields, dData.sh.label, 
              dData.sh.lcolor, dData.sh.color, fontLargeTxf,
              fontFixedTxf);

  /* damage */
  renderScale(o.dam.x, o.dam.y, o.dam.w, o.dam.h,
              0, 100, dData.dam.damage, dData.dam.label, 
              dData.dam.lcolor, dData.dam.color, fontLargeTxf,
              fontFixedTxf);

  /* fuel */
  renderScale(o.fuel.x, o.fuel.y, o.fuel.w, o.fuel.h,
              0, 999, dData.fuel.fuel, dData.fuel.label, 
              dData.fuel.lcolor, dData.fuel.color, fontLargeTxf,
              fontFixedTxf);

  /* etemp */
  renderScale(o.etemp.x, o.etemp.y, o.etemp.w, o.etemp.h,
              0, 100, dData.etemp.etemp, dData.etemp.label, 
              dData.etemp.lcolor, dData.etemp.color, fontLargeTxf,
              fontFixedTxf);

  /* wtemp */
  renderScale(o.wtemp.x, o.wtemp.y, o.wtemp.w, o.wtemp.h,
              0, 100, dData.wtemp.wtemp, dData.wtemp.label, 
              dData.wtemp.lcolor, dData.wtemp.color, fontLargeTxf,
              fontFixedTxf);

  /* alloc */
  renderAlloc(o.alloc.x, o.alloc.y, o.alloc.w, o.alloc.h,
            &dData.alloc, fontLargeTxf, fontFixedTxf);

  
  /* BEGIN "stat" box -
     kills, towing/towed by, x armies, CLOAKED/destruct */
     
  /* kills */
  sprintf(buf1024, "#%d#%5s #%d#kills", InfoColor, dData.kills.kills,
          CyanColor);
  glfRender(o.kills.x, o.kills.y, 
            0.0, 
            o.kills.w, o.kills.h, fontFixedTxf, buf1024, NoColor,
            TRUE, TRUE, TRUE);

  /* towed/towing */
  if (strlen(dData.tow.str))
      glfRender(o.tow.x, o.tow.y, 0.0, o.tow.w, o.tow.h, 
                 fontFixedTxf, dData.tow.str, MagentaColor, TRUE, FALSE, TRUE);

  /* armies */
  if (strlen(dData.armies.str))
    glfRender(o.arm.x, o.arm.y, 0.0, o.arm.w, o.arm.h,
               fontFixedTxf, dData.armies.str, InfoColor, TRUE, FALSE, TRUE);

  /* cloak/Destruct */
  if (strlen(dData.cloakdest.str))
    glfRender(o.cloakdest.x, o.cloakdest.y, 0.0, o.cloakdest.w, o.cloakdest.h,
              fontFixedTxf, dData.cloakdest.str, 
              (cloakbon) ? dData.cloakdest.color : dData.cloakdest.color | CQC_A_BOLD, 
              TRUE, FALSE, TRUE);

  /* alert stat */
  if (strlen(dData.aStat.alertStatus))
    glfRender(o.alert.x, o.alert.y, 0.0, o.alert.w, o.alert.h,
              fontFixedTxf, dData.aStat.alertStatus, 
              (alertbon) ? dData.aStat.color & ~CQC_A_BOLD : 
              dData.aStat.color | CQC_A_BOLD, 
              TRUE, FALSE, TRUE);

  /* END stat box */

  if ((Context.recmode != RECMODE_PLAYING) &&
      (Context.recmode != RECMODE_PAUSED))
    {
      /* althud */
      if (UserConf.AltHUD)
        glfRender(o.althud.x, o.althud.y, 
                  0.0, 
                  o.althud.w, o.althud.h,
                  fontFixedTxf, dData.xtrainfo.str, InfoColor, TRUE, TRUE, TRUE);

      if (dostats)
        {
          glfRender(o.rectime.x, o.rectime.y, 0.0, 
                    o.rectime.w, o.rectime.h,
                    fontFixedTxf, sbuf1024, NoColor | CQC_A_DIM, 
                    TRUE, TRUE, TRUE);
        }          
    }
  else
    {
      /* for playback, the ship/item we are watching */
      glfRender(o.althud.x, o.althud.y, 
                 0.0, 
                 o.althud.w, o.althud.h,
                 fontFixedTxf, dData.recId.str, 
                 MagentaColor | CQC_A_BOLD, 
                 TRUE, TRUE, TRUE);

      glfRender(o.rectime.x, o.rectime.y, 0.0, 
                o.rectime.w, o.rectime.h,
                fontFixedTxf, dData.recTime.str, NoColor, TRUE, TRUE, TRUE);
    }


  /* the 3 prompt/msg lines  */

  /* MSG_LIN1 */
  if (strlen(dData.p1.str))
    glfRender(o.msg1.x, o.msg1.y, 0.0, 
              o.msg1.w, o.msg1.h,
              fontFixedTxf, dData.p1.str, InfoColor, TRUE, TRUE, TRUE);

  /* MSG_LIN2 */
  if (strlen(dData.p2.str))
    glfRender(o.msg2.x, o.msg2.y, 0.0,
              o.msg2.w, o.msg2.h,
              fontFixedTxf, dData.p2.str, InfoColor, TRUE, TRUE, TRUE);

  /* MSG_MSG */
  if (strlen(dData.msg.str))
    glfRender(o.msgmsg.x, o.msgmsg.y, 0.0, 
              o.msgmsg.w, o.msgmsg.h,
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
