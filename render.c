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
#include "conqlb.h"
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
#include "textures.h"
#include "anim.h"

extern dspData_t dData;

/* from nCP.c */
extern animStateRec_t ncpBlinkerOneSec;
extern animStateRec_t ncpBlinkerHalfSec;
extern animStateRec_t ncpBlinkerQtrSec;
/* easier macros */
#define BLINK_ONESEC      (ncpBlinkerOneSec.state.armed)
#define BLINK_HALFSEC     (ncpBlinkerHalfSec.state.armed)
#define BLINK_QTRSEC      (ncpBlinkerQtrSec.state.armed)

/*
 Cataboligne - new controls for icon HUD, pass code to render
*/

extern void hexColor(Unsgn32);

/* we store geometry here that should only need to be recomputed
   when the screen is resized, not every single frame :) */

typedef struct _obj
{
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
  obj_t headl;

  obj_t warp;
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

  obj_t althud;
  obj_t rectime;

  /* msg/prompt lines */
  obj_t msg1;
  obj_t msg2;
  obj_t msgmsg;

  /* iconhud specific */

  obj_t decal1;                 /* decal 1 location */
  obj_t d1shg;                  /* shield gauge */
  obj_t d1shn;                  /* shield value (number) */
  obj_t d1damg;           
  obj_t d1damn;           
  obj_t d1icon;                 /* the icon area */
  obj_t d1abar1;                /* first alert bar */
  obj_t d1abar2;                /* second alert bar */
  obj_t d1atarg;                /* ship causing alert */
  obj_t decal2;                 /* decal 2 location */
  obj_t d2fuelg;                /* fuel gauge */
  obj_t d2fueln;                /* fuel value (number) */
  obj_t d2engtg;
  obj_t d2engtn;
  obj_t d2weptg;
  obj_t d2weptn;
  obj_t d2allocg;
  obj_t d2allocn;
  obj_t d2killb;                /* location of kills box */

} o = {};

/* update icon hud geometry if stuff changes (resize).  */
void updateIconHudGeo(void)
{				/* assumes context is current*/
  GLfloat tx, ty, th, tw;

  o.x = dConf.wX;
  o.y = dConf.wY;
  o.w = dConf.wW;
  o.h = dConf.wH;

  o.xstatw = dConf.vX - (dConf.borderW * 2.0);

  tx = dConf.vX;
  ty = dConf.vY + dConf.vH;
  o.sb_ih = (dConf.wH - (dConf.borderW * 2.0) - ty) / 4.0;

  o.alertb.x = dConf.vX - 3.0;
  o.alertb.y = dConf.vY - 3.0;
  o.alertb.w = dConf.vW + (3.0 * 2.0);
  o.alertb.h = dConf.vH + (3.0 * 2.0);

  tx = dConf.borderW;
  ty = dConf.borderW;

  /* the real fun begins. */

  /* figure out hud dimensions and decal sizes/locations 
   *
   * we divide the hud area of the screen into 10 parts vertically 
   * 2 parts for warp/head, 4 for decal 1, 4 for decal 2
   *
   * we calculate the pixel positions by dividing the drawing area
   * into sections, and then refering to offsets in these sections by
   * a percentage.  For example, assuming a region with a length of
   * 1.0, 0.2 would refer to 20% of of the total size.  This is then
   * used to locate a specific position in the drawing areas, which
   * will always be correct regardless of the true pixel dimensions
   * and size of the area.  Ie, the calculations are used to compute
   * 'real' pixel positions based on percentages of total area.  Never
   * use absolute screen pixel x/y positions in computing these
   * values or scaling/resizing will be broken.
   *
   * Of course with the decals, these calculations assume a texture
   * size of 256x256.  The actual texture pixel values were determined
   * by loading the tex into gimp, positioning the cursor on the area
   * of interest, and recording the texture pixel start, length, and
   * height of the area of interest (like the fuel gauge in decal2).
   * These coordinates are hardcoded in the calulations below.  Yes,
   * this does suck.  On the plus side, these calculated positions seem
   * to be consistant among all of the decal textures.  Actually, this
   * is pretty much a requirement now.
   *
   */

  /* heading tex and label 
   * we devide the section horizontally into 6.0 parts
   * the heading icon occupies 2 parts, and the warp 3, then
   * centered within the 6-part area.
   */
  o.head.x = tx + 2.0;
  o.head.y = ty;
  o.head.w = ((o.xstatw / 6.0) * 2.0);
  o.head.h = (dConf.vH / 10.0) * 1.5;

  o.headl.x = o.head.x;
  o.headl.y = o.head.y + o.head.h;
  o.headl.w = ((o.xstatw / 6.0) * 1.5);
  o.headl.h = (o.head.h / 10.0) * 2.0;

  /* warp tex and label */
  o.warp.x = (o.xstatw / 6.0) * 3.0;
  o.warp.y = ty;
  o.warp.w = (o.xstatw / 6.0) * 3.0;
  o.warp.h = (dConf.vH / 10.0) * 2.0;

  o.warpl.x = o.warp.x;
  o.warpl.y = o.headl.y + (o.headl.h * 0.7);
  o.warpl.w = ((o.xstatw / 6.0) * 1.5);
  o.warpl.h = o.headl.h * 0.9;


  /* pixel height of decal 1 & 2 */
  th = (dConf.vH / 10.0) * 4.0;
  /* pixel width of decal 1 & 2 */
  tw = o.xstatw - ((o.xstatw / 10.0) * 2.5);

  /* decal 1 */

  o.decal1.x = tx;
  o.decal1.y = ty + ((dConf.vH / 10.0) * 2.0);
  o.decal1.w = tw;
  o.decal1.h = (dConf.vH / 10.0) * 4.0;

  /* shield gauge */
  o.d1shg.x = tx + ((96.0 / 256.0) * tw);
  o.d1shg.y = o.decal1.y + ((18.0 / 256.0) * th);
  o.d1shg.w = (((175.0 - 96.0) / 256.0) * tw);
  o.d1shg.h = ((22.0 / 256.0) * th);

  /* shield number (value) */
  o.d1shn.x = o.decal1.x + o.decal1.w;
  o.d1shn.y = o.d1shg.y - ((o.d1shg.h * 2.0) * 0.25);
  o.d1shn.w = o.xstatw - (tx + o.decal1.w);
  o.d1shn.h = o.d1shg.h * 2.0;

  o.d1damg.x = tx + ((96.0 / 256.0) * tw);
  o.d1damg.y = o.decal1.y + ((218.0 / 256.0) * th);
  o.d1damg.w = (((175.0 - 96.0) / 256.0) * tw);
  o.d1damg.h = ((22.0 / 256.0) * th);

  o.d1damn.x = o.decal1.x + o.decal1.w;
  o.d1damn.y = o.d1damg.y - ((o.d1damg.h * 2.0) * 0.25);
  o.d1damn.w = o.xstatw - (tx + o.decal1.w);
  o.d1damn.h = o.d1damg.h * 2.0;

  /* position the icon area within decal 1 */
  o.d1icon.x = tx + ((o.xstatw / 6.0) * 1.0);
  o.d1icon.y = o.decal1.y + ((43.0 / 256.0) * th) ;
  o.d1icon.w = ((o.xstatw / 6.0) * 4.0);
  o.d1icon.h = (o.decal1.y + ((209.0 / 256.0) * th)) - o.d1icon.y;

  /* decal 2 */
  o.decal2.x = tx;
  o.decal2.y = o.decal1.y + o.decal1.h;
  o.decal2.w = tw;
  o.decal2.h = (dConf.vH / 10.0) * 4.0;

  o.d2fuelg.x = tx + ((96.0 / 256.0) * tw);
  o.d2fuelg.y = o.decal2.y + ((22.0 / 256.0) * th);
  o.d2fuelg.w = (((175.0 - 96.0) / 256.0) * tw);
  o.d2fuelg.h = ((17.0 / 256.0) * th);

  o.d2fueln.x = o.decal2.x + o.decal2.w;
  o.d2fueln.y = o.d2fuelg.y - ((o.d2fuelg.h * 2.0) * 0.25);
  o.d2fueln.w = o.xstatw - (tx + o.decal2.w);
  o.d2fueln.h = o.d2fuelg.h * 2.0;
  
  o.d2engtg.x = tx + ((96.0 / 256.0) * tw);
  o.d2engtg.y = o.decal2.y + ((75.0 / 256.0) * th);
  o.d2engtg.w = (((175.0 - 96.0) / 256.0) * tw);
  o.d2engtg.h = ((17.0 / 256.0) * th);

  o.d2engtn.x = o.decal2.x + o.decal2.w;
  o.d2engtn.y = o.d2engtg.y - ((o.d2engtg.h * 2.0) * 0.25);
  o.d2engtn.w = o.xstatw - (tx + o.decal2.w);
  o.d2engtn.h = o.d2engtg.h * 2.0;

  o.d2weptg.x = tx + ((96.0 / 256.0) * tw);
  o.d2weptg.y = o.decal2.y + ((127.0 / 256.0) * th);
  o.d2weptg.w = (((175.0 - 96.0) / 256.0) * tw);
  o.d2weptg.h = ((17.0 / 256.0) * th);

  o.d2weptn.x = o.decal2.x + o.decal2.w;
  o.d2weptn.y = o.d2weptg.y - ((o.d2weptg.h * 2.0) * 0.25);
  o.d2weptn.w = o.xstatw - (tx + o.decal2.w);
  o.d2weptn.h = o.d2weptg.h * 2.0;

  o.d2allocg.x = tx + ((96.0 / 256.0) * tw);
  o.d2allocg.y = o.decal2.y + ((185.0 / 256.0) * th);
  o.d2allocg.w = (((175.0 - 96.0) / 256.0) * tw);
  o.d2allocg.h = ((17.0 / 256.0) * th);

  o.d2allocn.x = o.decal2.x + o.decal2.w;
  o.d2allocn.y = o.d2allocg.y - ((o.d2allocg.h * 2.0) * 0.25);
  o.d2allocn.w = o.xstatw - (tx + o.decal2.w);
  o.d2allocn.h = o.d2allocg.h * 2.0;

  /* kills, embedded in decal2 */
  o.d2killb.x = tx + ((164.0 / 256.0) * tw);
  o.d2killb.y = o.decal2.y + ((217.0 / 256.0) * th);
  o.d2killb.w = (((249.0 - 164.0) / 256.0) * tw);
  o.d2killb.h = ((16.0 / 256.0) * th);

  /* tow message, overlayed in upper left corner of icon area */
  o.tow.x = tx + 2.0;
  o.tow.y = o.d1icon.y;
  o.tow.w = o.d1icon.w * 0.40;
  o.tow.h = o.d1icon.h * 0.15;

  /* armies in upper right */
  o.arm.x = tx + ((o.xstatw / 2.0) - (o.arm.w / 2.0));
  o.arm.y = o.tow.y;
  o.arm.w = o.tow.w;
  o.arm.h = o.tow.h;

  /* alert bars */
  o.d1abar1.x = tx + 2.0;
  o.d1abar1.y = o.tow.y + o.tow.h;
  o.d1abar1.w = o.d1icon.w * 0.10;
  o.d1abar1.h = o.d1icon.h - (o.arm.h * 2.0);

  o.d1abar2.x = o.d1icon.x + o.d1icon.w + o.d1abar1.w;
  o.d1abar2.y = o.tow.y + o.tow.h;
  o.d1abar2.w = o.d1icon.w * 0.10;
  o.d1abar2.h = o.d1icon.h - (o.arm.h * 2.0);

  /* alert text (area below the icon decal) */
  o.d1atarg.x = tx + 2.0;
  o.d1atarg.y = (o.d1icon.y + o.d1icon.h) - o.tow.h;
  o.d1atarg.w = (o.d1abar2.x + o.d1abar2.w) - o.d1atarg.x;
  o.d1atarg.h = o.arm.h * 0.9;

  /* destructing message.  try to center within viewer. */
  o.cloakdest.x = dConf.vX + ((dConf.vW / 6.0) * 1.0);
  o.cloakdest.y = dConf.vY + (dConf.vH / 2.0) - ((dConf.vH / 6.0) / 2.0);
  o.cloakdest.w = ((dConf.vW / 6.0) * 4.0);
  o.cloakdest.h = (dConf.vH / 6.0);

  /* althud/altinfo */
  tx = dConf.vX;
  ty = dConf.vY + dConf.vH;

  o.althud.x = tx;
  o.althud.y = ty + (o.sb_ih * 0.2);
  o.althud.w = ((dConf.vX + dConf.vW) - tx);
  o.althud.h =  (o.sb_ih * 0.7);

  /* rectime or stats info */
  tx = dConf.borderW;

  o.rectime.x = tx;
  o.rectime.y = (ty + (o.sb_ih * 0.2));
  o.rectime.w =  o.xstatw;
  o.rectime.h =  (o.sb_ih * 0.7);

  /* the 3 prompt/msg lines  */
  o.msg1.x = tx;
  o.msg1.y = ty + (o.sb_ih * 1.0);
  o.msg1.w = (dConf.wW  - (dConf.borderW * 2.0));
  o.msg1.h = o.sb_ih;
  
  o.msg2.x = tx;
  o.msg2.y = ty + (o.sb_ih * 2.0);
  o.msg2.w = (dConf.wW  - (dConf.borderW * 2.0));
  o.msg2.h = o.sb_ih;
  
  o.msgmsg.x = tx;
  o.msgmsg.y = ty + (o.sb_ih * 3.0);
  o.msgmsg.w = (dConf.wW  - (dConf.borderW * 2.0));
  o.msgmsg.h = o.sb_ih;

  return;

}

void renderScaleVal(GLfloat x, GLfloat y, GLfloat w, GLfloat h, 
                    TexFont *lfont, int val, int col)
{
  char buf32[32];

  sprintf(buf32, "%3d", val);

  /* a scale value (number) */
  glfRender(x, y, 0.0, w * 0.9, h * 0.8, lfont, buf32, col, TRUE, FALSE, TRUE);
  
  drawLineBox(x, y, w, h, BlueColor, 1.0);

  return;
}

void renderAllocVal(GLfloat x, GLfloat y, GLfloat w, GLfloat h, 
                    TexFont *lfont, char *val, int col)
{
  /* a scale value (number) */
  glfRender(x, y, 0.0, w * 0.9, h * 0.8, lfont, val, col, TRUE, FALSE, TRUE);
  
  drawLineBox(x, y, w, h, BlueColor, 1.0);

  return;
}

/* This is Cat's icon hud */
void renderHud(int dostats)
{				/* assumes context is current*/
  char sbuf1024[1024];
  char fbuf[128];
  float FPS = getFPS();
  int team = Ships[Context.snum].team;
  int icl;
  real warp = Ships[Context.snum].warp;
  real maxwarp = ShipTypes[Ships[Context.snum].shiptype].warplim;
  static int rxtime = 0;
  static int oldrx = 0;
  static int rxdiff = 0;
  
  if (!GLTextures)
    return;                     /* don't render until we actually can */

  /* draw fps/bps... */

  if ((frameTime - rxtime) > 1000)
    {
      rxdiff = pktRXBytes - oldrx;
      oldrx = pktRXBytes;
      rxtime = frameTime;
    }

  if (FPS > 999.0)
    sprintf(fbuf, "FPS: 999+");
  else
    sprintf(fbuf, "FPS: %3.1f", FPS);

  sprintf(sbuf1024, "%4dms %3.1fKB/s %s",  
          pingAvgMS, 
          ((float)rxdiff / 1000.0),
          fbuf);

  if (o.x != dConf.wX || o.y != dConf.wY || o.w != dConf.wW ||
      o.h != dConf.wH)
    {
      updateIconHudGeo();
    }

  /* draw alert border */
  drawLineBox(o.alertb.x, o.alertb.y, 
              o.alertb.w,
              o.alertb.h,
              dData.aBorder.alertColor,
              2.0);


  /* The Icon Hud (TM) in all it's glory... */

  /* heading val */
  glfRender(o.headl.x, o.headl.y, 0.0, o.headl.w, 
            o.headl.h, fontLargeTxf, dData.heading.heading, 
            NoColor, TRUE, FALSE, TRUE);

  /* warp val */
  glfRender(o.warpl.x, o.warpl.y, 0.0, 
            o.warpl.w, o.warpl.h,
            fontLargeTxf, dData.warp.warp, InfoColor, TRUE, FALSE, TRUE);
  
  /* warp quad indicator color */
  if (team == TEAM_ORION) 
    hexColor(0xFF9FEFFF);
  else if (team == TEAM_ROMULAN) 
    hexColor(0xFFBFBCBC);
  else
    hexColor(0xFFD72D00);

  /* warp indicator quad */
  if (warp >= 0.1)
    drawQuad(o.warp.x, 
             o.warp.y, (o.warp.w * (warp / maxwarp)), 
             o.warp.h * 0.79 /*empirical. ick.*/, 0.0);

  /* shields gauge */
  if (dData.sh.shields > 0.0)
    renderScale(o.d1shg.x, o.d1shg.y, o.d1shg.w, o.d1shg.h,
                0, 100, dData.sh.shields, dData.sh.color);
  
  /* shields num */
  renderScaleVal(o.d1shn.x, o.d1shn.y,
                 o.d1shn.w, o.d1shn.h,
                 fontFixedTxf, 
                 (dData.sh.shields < 0) ? 0 : dData.sh.shields, 
                  dData.sh.color);
  
  /* damage gauge */

  renderScale(o.d1damg.x, o.d1damg.y, 
              o.d1damg.w, o.d1damg.h,
              0, 100, dData.dam.damage, dData.dam.color);

  /* damage num */
  renderScaleVal(o.d1damn.x, o.d1damn.y,
                 o.d1damn.w, o.d1damn.h,
                 fontFixedTxf, dData.dam.damage, dData.dam.color);

  /* fuel guage */
  renderScale(o.d2fuelg.x, o.d2fuelg.y, o.d2fuelg.w, o.d2fuelg.h,
              0, 999, dData.fuel.fuel, dData.fuel.color);
  
  /* fuel value */
  renderScaleVal(o.d2fueln.x, o.d2fueln.y,
                 o.d2fueln.w, o.d2fueln.h,
                 fontFixedTxf, dData.fuel.fuel, dData.fuel.color);

  /* etemp guage */
  renderScale(o.d2engtg.x, o.d2engtg.y, o.d2engtg.w, o.d2engtg.h,
              0, 100, dData.etemp.etemp, dData.etemp.color);

  /* etemp value */
  renderScaleVal(o.d2engtn.x, o.d2engtn.y,
                 o.d2engtn.w, o.d2engtn.h,
                 fontFixedTxf, dData.etemp.etemp, dData.etemp.color);

  /* wtemp gauge */
  renderScale(o.d2weptg.x, o.d2weptg.y, o.d2weptg.w, o.d2weptg.h,
              0, 100, dData.wtemp.wtemp, dData.wtemp.color);

  /* wtemp value*/
  renderScaleVal(o.d2weptn.x, o.d2weptn.y,
                 o.d2weptn.w, o.d2weptn.h,
                 fontFixedTxf, dData.wtemp.wtemp, dData.wtemp.color);

  /* alloc */
  renderAlloc(o.d2allocg.x, o.d2allocg.y, o.d2allocg.w, o.d2allocg.h,
              &dData.alloc, fontLargeTxf, fontFixedTxf, 
              TRUE);
  
  renderAllocVal(o.d2allocn.x, o.d2allocn.y,
                 o.d2allocn.w, o.d2allocn.h,
                 fontFixedTxf, dData.alloc.allocstr, InfoColor);
  
  /* BEGIN "stat" box -
     kills, towing/towed by, x armies, CLOAKED/destruct */

  /* kills */

  glfRender(o.d2killb.x, o.d2killb.y, 0.0, 
            o.d2killb.w, 
            o.d2killb.h, fontFixedTxf, dData.kills.kills, InfoColor,
            TRUE, TRUE, TRUE);

  /* towed/towing */
  if (dData.tow.str[0]) 
    glfRender(o.tow.x, o.tow.y, 0.0, 
              o.tow.w, o.tow.h, 
              fontFixedTxf, dData.tow.str, 
              MagentaColor | CQC_A_BOLD, TRUE, FALSE, TRUE);

  /* alert stat bars */
  if (dData.aStat.alertStatus[0])
    {
      switch(dData.aStat.alertStatus[0])  /* define alert decal */
        {                       /* need to blink these */
        case 'R':               /* red alert (not Alert) */
        case 'Y':               /* yellow alert (not Prox) */
          icl = (BLINK_HALFSEC) ? dData.aStat.color & ~CQC_A_BOLD : 
            dData.aStat.color | CQC_A_BOLD;
          break;
        default:
          icl = dData.aStat.color;
          break;
        }

      uiPutColor(icl);

      /* left bar */
      drawQuad(o.d1abar1.x, o.d1abar1.y, o.d1abar1.w, o.d1abar1.h, 0.0);
      /* the object of our affection */
      glfRender(o.d1atarg.x, o.d1atarg.y, 0.0, 
                o.d1atarg.w, o.d1atarg.h, 
                fontFixedTxf, dData.aStat.alertStatus, 
                icl, TRUE, FALSE, TRUE);
      /* right bar */
      drawQuad(o.d1abar2.x, o.d1abar2.y, o.d1abar2.w, o.d1abar2.h, 0.0);
    }
  
  /* GL */
  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  glEnable(GL_TEXTURE_2D);
  
  /* it's pretty hacky how we determine some of this stuff... */

  /* icon shield decal */
  if (dData.sh.shields > 0.0)
    {
      if (dData.cloakdest.str[1] == 'C') 
        icl = dData.sh.color | 0x80000000; /* set some alpha if cloaked */
      else 
        icl = dData.sh.color;
      
      drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h, 
                       HUD_SHI, icl);
    }
  
  /* ship icon decal */
  if (dData.cloakdest.str[1] == 'C') 
    icl = dData.dam.color | 0x80000000; /* set some alpha if cloaked */
  else 
    icl = dData.dam.color;

  drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h, 
                   HUD_ICO, icl);

  icl = 0xFFFFFFFF;
  
  /* heading decal */
  drawIconHUDDecal(o.head.x, o.head.y, o.head.w, o.head.h,
                   HUD_HEAD, icl);
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  /* draw the heading pointer (dialp) */
  /* GL */
  glPushMatrix();
  glTranslatef(o.head.x + (o.head.w / 2.0), 
               o.head.y + (o.head.h / 2.0), 0.0);
  glRotatef(Ships[Context.snum].head, 0.0, 0.0, -1.0);
  drawIconHUDDecal(-(o.head.w / 2.0), -(o.head.h / 2.0), 
                   o.head.w, o.head.h,
                   HUD_HDP, icl);
  glPopMatrix();

  /* regular conq stuff - tractor beam, armies, cloak */
  
  if (dData.tow.str[0]) 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_ITRACTOR, icl);
  if (dData.armies.str[0] && !SROBOT(Context.snum)) 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_IARMIES, icl);

  /* hacky.. for sure. */
  if (dData.cloakdest.str[1] == 'C') 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_ICLOAK, icl);
  
  /*
    Cataboligne + display critical alerts decals
    
    critical levels - display decal
    
    shields  < 31
    eng temp > 80
    wep temp > 80
    hull dmg > 69
    
    failure levels - display decal
    
    shields < 4
    hull dmg > 95
  */

  if ((dData.sh.shields < 4 && SSHUP(Context.snum) 
       && !SREPAIR(Context.snum)))  
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_ISHFAIL, icl);
  else if ((dData.sh.shields < 31 && SSHUP(Context.snum) && 
            !SREPAIR(Context.snum))) 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_ISHCRIT, icl);
  if (dData.etemp.etemp > 80) 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_IENGCRIT, icl);
  if (dData.wtemp.wtemp > 80) 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_IWEPCRIT, icl);
  if (dData.dam.damage > 90) 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_IENGFAIL, icl);
  if (dData.dam.damage > 69) 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_IHULCRIT, icl);
  if (SREPAIR(Context.snum)) 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_IREPAIR, icl);
  
  /* draw warp and decals */
  drawIconHUDDecal(o.warp.x, o.warp.y, o.warp.w, o.warp.h, 
                   HUD_WARP, icl);
  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   HUD_DECAL1, icl);
  drawIconHUDDecal(o.decal2.x, o.decal2.y, o.decal2.w, o.decal2.h, 
                   HUD_DECAL2, icl);


  /* GL */
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);

  /* armies */
  if (dData.armies.str[0])
    {
      glfRender(o.arm.x, o.arm.y, 0.0, 
                o.arm.w, o.arm.h, 
                fontFixedTxf, 
                dData.armies.str, InfoColor, TRUE, FALSE, TRUE);
    }

  /* Destruct msg, centered in the viewer */
  if (dData.cloakdest.str[0] == 'D') /* destructing */
    {
      glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
      glEnable(GL_BLEND);
      /* we want to add an alhpa for this */
      glfRender(o.cloakdest.x, o.cloakdest.y, 0.0, 
                o.cloakdest.w, o.cloakdest.h, 
                fontMsgTxf, 
                dData.cloakdest.str, 
                (BLINK_ONESEC) ? 
                dData.cloakdest.color | CQC_A_BOLD | 0x50000000: 
                (dData.cloakdest.color | 0x50000000) & ~CQC_A_BOLD, 
                TRUE, FALSE, TRUE);
      glDisable(GL_BLEND);
    }

  /* END stat box */
  
  if ((Context.recmode != RECMODE_PLAYING) &&
      (Context.recmode != RECMODE_PAUSED))
    {
      /* althud */
      if (UserConf.AltHUD)
        glfRender(o.althud.x, o.althud.y, 
                  0.0, 
                  o.althud.w, o.althud.h,
                  fontFixedTxf, dData.xtrainfo.str, InfoColor, 
                  TRUE, TRUE, TRUE);
      
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
  if (dData.p1.str[0])
    glfRender(o.msg1.x, o.msg1.y, 0.0, 
              o.msg1.w, o.msg1.h,
              fontFixedTxf, dData.p1.str, InfoColor, TRUE, TRUE, TRUE);

  /* MSG_LIN2 */
  if (dData.p2.str[0])
    glfRender(o.msg2.x, o.msg2.y, 0.0,
              o.msg2.w, o.msg2.h,
              fontFixedTxf, dData.p2.str, InfoColor, TRUE, TRUE, TRUE);

  /* MSG_MSG */
  if (dData.msg.str[0])
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


  drawViewerBG(Context.snum, dovbg);

  display( Context.snum, FALSE );

  /* reset for everything else */
  glViewport(0, 0, dConf.wW, dConf.wH);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(dConf.hmat);
  glMatrixMode(GL_MODELVIEW);

  return;
}

void renderScale(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
                 int min, int max, int val, int scalecolor) 
{
  GLfloat valxoff = (w / 10.0) * 3.0;
  GLfloat scaleend = x + w - valxoff - (w / 10.0);
  GLfloat scaleh = h / 2.0;
  
  if (val < min)
    val = min;
  if (val > max)
    val = max;

  uiPutColor(scalecolor);
  
  /* gauge */
  drawQuad(x, y, 
           scaleend * (GLfloat)((GLfloat)val / ((GLfloat)max - (GLfloat)min)),
           scaleh, 0.0);
  
  return;
}

/* like renderScale, but more specific */
void renderAlloc(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
               struct _alloc *a,
               TexFont *lfont, TexFont *vfont, int HUD)
{
  GLfloat valxoff = (w / 10.0) * 3.0;
  GLfloat scaleend = x + w - valxoff - (w / 10.0);
  GLfloat scaleh = h / 2.0;
  
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

  return;
}

