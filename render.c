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
#include "GL.h"

extern dspData_t dData;

#define GLCOLOR4F(glcol) glColor4f(glcol.r, glcol.g, glcol.b, glcol.a) 

/*
 Cataboligne - 11.20.6
 ack alert klaxon with <ESC>
*/
#include "cqsound.h"

#define ALERT_OFF 0
#define ALERT_ON 1
#define ALERT_ACK 2

cqsHandle alertHandle = CQS_INVHANDLE;

/* critical limits */

/*   shields  < 20
 *   eng temp > 80 or overloaded
 *   wep temp > 80 or overloaded
 *   hull dmg >= 70
 *   fuel < 100 critical
*/

#define E_CRIT     80
#define W_CRIT     80
#define F_CRIT     100
#define SH_CRIT    20
#define HULL_CRIT  75

/* we store geometry here that should only need to be recomputed
   when the screen is resized, not every single frame :) */


static struct {
  GLfloat x, y;                 /* stored x/y/w/h of game window */
  GLfloat w, h;

  GLfloat xstatw;               /* width of status area */

  GLfloat sb_ih;                /* statbox item height */

  GLRect_t alertb;              /* alert border */

  GLRect_t head;
  GLRect_t headl;

  GLRect_t warp;
  GLRect_t warpl;

  GLRect_t sh;
  GLRect_t dam;
  GLRect_t fuel;
  GLRect_t etemp;
  GLRect_t wtemp;
  GLRect_t alloc;

  /* misc info (viewer) */
  GLRect_t tow;
  GLRect_t arm;
  GLRect_t cloakdest;

  GLRect_t althud;
  GLRect_t rectime;

  /* msg/prompt lines */
  GLRect_t msg1;
  GLRect_t msg2;
  GLRect_t msgmsg;

  /* pulse messages (viewer) */
  GLRect_t engfailpulse;        /* wep/eng failure pulses */
  GLRect_t wepfailpulse;
  GLRect_t fuelcritpulse;
  GLRect_t shcritpulse;
  GLRect_t hullcritpulse;
  
  GLRect_t magfac;                 /* mag factor */

  /* iconhud decals */

  /* decal1 */
  GLRect_t decal1;              /* decal 1 location */
  GLRect_t d1shg;               /* shield gauge */
  GLRect_t d1shcharge;          /* shield charge status */
  GLRect_t d1shn;               /* shield value (number) */
  GLRect_t d1damg;              /* damage */
  GLRect_t d1damn;           
  GLRect_t d1icon;              /* the ship icon area */
  GLRect_t d1torps;             /* location of the torp pip area */
  GLRect_t d1torppips[MAXTORPS]; /* the torp pips */
  GLRect_t d1phcharge;          /* phaser charge status */
  GLRect_t d1atarg;             /* ship causing alert */

  /* decal2 */
  GLRect_t decal2;              /* decal 2 location */
  GLRect_t d2fuelg;             /* fuel gauge */
  GLRect_t d2fueln;             /* fuel value (number) */
  GLRect_t d2engtg;             /* etemp */
  GLRect_t d2engtn;
  GLRect_t d2weptg;             /* wtemp */
  GLRect_t d2weptn;
  GLRect_t d2allocg;            /* alloc */
  GLRect_t d2allocn;
  GLRect_t d2killb;             /* location of kills box */

} o = {};

/* a useful 'mapping' macros for the decals */
#define MAPAREAX(_decalw, _decalh, _rect) \
        ( tx + (((_rect)->x / (_decalw) ) * tw) )

#define MAPAREAY(_decalw, _decalh, _rect) \
        ( decaly + (((_rect)->y / (_decalh)) * th) )

#define MAPAREAW(_decalw, _decalh, _rect) \
        ( (((_rect)->w / (_decalw)) * tw) )

#define MAPAREAH(_decalw, _decalh, _rect) \
        ( (((_rect)->h / (_decalh)) * th) )

#define MAPAREA(_decalptr, _taptr, _optr) \
        {                                 \
          (_optr)->x = MAPAREAX((_decalptr)->w, (_decalptr)->h, (_taptr)); \
          (_optr)->y = MAPAREAY((_decalptr)->w, (_decalptr)->h, (_taptr)); \
          (_optr)->w = MAPAREAW((_decalptr)->w, (_decalptr)->h, (_taptr)); \
          (_optr)->h = MAPAREAH((_decalptr)->w, (_decalptr)->h, (_taptr)); \
        }

/* update icon hud geometry if stuff changes (resize).  */
void updateIconHudGeo(int snum)
{				/* assumes context is current*/
  GLfloat tx, ty, th, tw, decaly;
  int i;
  static int steam = -1;
  /* these two rects will just be used to store the texture size (in
     hw pixels) of each of the 2 decal areas (in w and h).  */
  static GLRect_t decal1_sz, decal2_sz;
  /* area pointers, decal1 */
  static cqiTextureAreaPtr_t d1shg = NULL;  /* d1 sh gauge */
  static cqiTextureAreaPtr_t d1shchrg = NULL;  /* d1 sh charge gauge */
  static cqiTextureAreaPtr_t d1shn = NULL;  /* d1 sh number */
  static cqiTextureAreaPtr_t d1damg = NULL; /* d1 damage gauge */
  static cqiTextureAreaPtr_t d1damn = NULL; /* d1 damage number */
  static cqiTextureAreaPtr_t d1torps = NULL; /* d1 torp icon area */
  static cqiTextureAreaPtr_t d1phaserchrg = NULL; /* d1 phaser charge */
  static cqiTextureAreaPtr_t d1icon = NULL; /* d1 ship icon area */

  /* area pointers, decal2 */
  static cqiTextureAreaPtr_t d2fuelg = NULL; /* d2 fuel */
  static cqiTextureAreaPtr_t d2fueln = NULL; /* d2 number */
  static cqiTextureAreaPtr_t d2engtg = NULL; /* d2 engine temp */
  static cqiTextureAreaPtr_t d2engtn = NULL; /* d2 engine number */
  static cqiTextureAreaPtr_t d2weptg = NULL; /* d2 weapon temp */
  static cqiTextureAreaPtr_t d2weptn = NULL; /* d2 weapon number */
  static cqiTextureAreaPtr_t d2allocg = NULL; /* d2 alloc */
  static cqiTextureAreaPtr_t d2allocn = NULL; /* d2 alloc number */
  static cqiTextureAreaPtr_t d2killb = NULL; /* d2 kills box */
  /* default texarea if we couldn't find the right one (1 pixel size) */
  static cqiTextureAreaRec_t defaultTA = { "NULL", 0.0, 0.0, 1.0, 1.0 };
  char buffer[CQI_NAMELEN];

  if (Ships[snum].team != steam)
    {                           /* we switched teams, reload/remap the
                                   decal texareas */
      int ndx;
      steam = Ships[snum].team;

      /* decal 1 */
      snprintf(buffer, CQI_NAMELEN - 1, "ship%c-ico-decal1", 
               Teams[steam].name[0]);

      /* get decal1 size */
      memset((void*)&decal1_sz, 0, sizeof(GLRect_t));
      if ((ndx = findGLTexture(buffer)) >= 0 )
        {                       /* this is the decal's texture size
                                   in pixels */
          decal1_sz.w = (GLfloat)GLTextures[ndx].w;
          decal1_sz.h = (GLfloat)GLTextures[ndx].h;
        } 
        
      /* look up the texareas, and clamp to the texture size */
      d1shg     = cqiFindTexArea(buffer, "shieldg", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1shg);
      d1shchrg  = cqiFindTexArea(buffer, "shchargeg", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1shchrg);
      d1shn     = cqiFindTexArea(buffer, "shieldn", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1shn);
      d1damg    = cqiFindTexArea(buffer, "damageg", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1damg);
      d1damn    = cqiFindTexArea(buffer, "damagen", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1damn);
      d1icon    = cqiFindTexArea(buffer, "icon", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1icon);
      d1torps    = cqiFindTexArea(buffer, "torps", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1torps);
      d1phaserchrg = cqiFindTexArea(buffer, "phaserchrg", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1phaserchrg);

      /* decal 2 */
      snprintf(buffer, CQI_NAMELEN - 1, "ship%c-ico-decal2", 
               Teams[steam].name[0]);

      /* get decal2 size */
      memset((void*)&decal2_sz, 0, sizeof(GLRect_t));
      if ((ndx = findGLTexture(buffer)) >= 0 )
        {
          decal2_sz.w = (GLfloat)GLTextures[ndx].w;
          decal2_sz.h = (GLfloat)GLTextures[ndx].h;
        } 
        
      d2fuelg   = cqiFindTexArea(buffer, "fuelg", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2fuelg);
      d2fueln   = cqiFindTexArea(buffer, "fueln", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2fueln);
      d2engtg   = cqiFindTexArea(buffer, "etempg", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2engtg);
      d2engtn   = cqiFindTexArea(buffer, "etempn", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2engtn);
      d2weptg   = cqiFindTexArea(buffer, "wtempg", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2weptg);
      d2weptn   = cqiFindTexArea(buffer, "wtempn", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2weptn);
      d2allocg  = cqiFindTexArea(buffer, "allocg", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2allocg);
      d2allocn  = cqiFindTexArea(buffer, "allocn", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2allocn);
      d2killb   = cqiFindTexArea(buffer, "killsbox", &defaultTA);
      CLAMPRECT(decal2_sz.w, decal2_sz.h, d2killb);
    }

  o.x = dConf.wX;
  o.y = dConf.wY;
  o.w = dConf.wW;
  o.h = dConf.wH;

  /* the width of the entire hud info drawing area */
  o.xstatw = dConf.vX - (dConf.borderW * 2.0);

  tx = dConf.vX;
  ty = dConf.vY + dConf.vH;
  /* icon height */
  o.sb_ih = (dConf.wH - (dConf.borderW * 2.0) - ty) / 4.0;

  /* alert border */
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
   * The decal texture sizes and the texareas specified in their
   * texture definitions are used to compute the right x/y/w/h values
   * for items that are going to be mapped into them (like a fuel
   * gauge).  The actual texture pixel values were determined by
   * loading the tex into gimp, positioning the cursor on the area of
   * interest, and recording the texture pixel start X, Y, width, and
   * height of the area of interest (like the fuel gauge in decal2).
   * These coordinates are specified as 'texareas' in the decal's
   * texture definition.  No more hardcoding! :)
   */

  /* heading tex and label 
   * we divide the section horizontally into 6 parts
   * the heading icon occupies 2 parts, and the warp 3, then
   * centered within the 6-part area.
   */

  /* we should probably re-do heading/warp as a single texture
     (decal0?) with texarea's describing the right location to draw
     things (like decal1 and 2 are handled). */

  o.head.x = tx + 2.0;
  o.head.y = ty;
  o.head.w = ((o.xstatw / 6.0) * 2.0);
  o.head.h = (dConf.vH / 10.0) * 1.5;

  o.headl.x = o.head.x;
  o.headl.y = o.head.y + o.head.h;
  o.headl.w = ((o.xstatw / 6.0) * 1.5);
  o.headl.h = (o.head.h / 10.0) * 2.0;

  /* warp tex and label, used for warp backgraound as well */
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
  tw = o.xstatw;

  /* decal 1 */
  o.decal1.x = tx;
  o.decal1.y = ty + ((dConf.vH / 10.0) * 2.0);
  o.decal1.w = tw;
  o.decal1.h = (dConf.vH / 10.0) * 4.0;

  decaly = o.decal1.y;          /* save this for mapping macros */

  /* shield gauge */
  MAPAREA(&decal1_sz, d1shg, &o.d1shg);

  /* shield number (value) */
  MAPAREA(&decal1_sz, d1shn, &o.d1shn);

  /* shield charge level (this is just a line) */
  MAPAREA(&decal1_sz, d1shchrg, &o.d1shcharge);

  /* damage */
  MAPAREA(&decal1_sz, d1damg, &o.d1damg);

  MAPAREA(&decal1_sz, d1damn, &o.d1damn);

  /* position the ship icon area within decal 1 */
  MAPAREA(&decal1_sz, d1icon, &o.d1icon);

  /* torp pips */
  MAPAREA(&decal1_sz, d1torps, &o.d1torps);
  for (i=0; i < MAXTORPS; i++)
    {
      o.d1torppips[i].x = o.d1torps.x;
      o.d1torppips[i].y = o.d1torps.y + 
        (o.d1torps.h / (real)MAXTORPS * (real)i);
      o.d1torppips[i].w = o.d1torps.w;
      o.d1torppips[i].h = o.d1torps.h / (real)MAXTORPS;
    } 

  /* phaser recharge status */
  MAPAREA(&decal1_sz, d1phaserchrg, &o.d1phcharge);

  /* decal 2 */
  o.decal2.x = tx;
  o.decal2.y = o.decal1.y + o.decal1.h;
  o.decal2.w = tw;
  o.decal2.h = (dConf.vH / 10.0) * 4.0;

  decaly = o.decal2.y;          /* save this for mapping */

  /* fuel */
  MAPAREA(&decal2_sz, d2fuelg, &o.d2fuelg);

  MAPAREA(&decal2_sz, d2fueln, &o.d2fueln);
  
  /* etemp */
  MAPAREA(&decal2_sz, d2engtg, &o.d2engtg);

  MAPAREA(&decal2_sz, d2engtn, &o.d2engtn);

  /* wtemp */
  MAPAREA(&decal2_sz, d2weptg, &o.d2weptg);

  MAPAREA(&decal2_sz, d2weptn, &o.d2weptn);

  /* allocations */
  MAPAREA(&decal2_sz, d2allocg, &o.d2allocg);

  MAPAREA(&decal2_sz, d2allocn, &o.d2allocn);

  /* kills box */
  MAPAREA(&decal2_sz, d2killb, &o.d2killb);

  /* END of the DECALS! */

  /* Cat 'gridscale'.  Now, magfac. */
  o.magfac.x = dConf.vX + ((dConf.vW / 180.0) * 1.0);
  o.magfac.y = dConf.vY + (dConf.vH - ((dConf.vH / 20.0) * 1.35));
  o.magfac.w = (dConf.vW / 12.0) * 1.0;
  o.magfac.h = (dConf.vH / 38.0) * 1.0;

  /*  For tow, armies, and wep/eng pulses, we divide the viewer into 8
   *  horizontal, and 20 vertical segments and confine these items to
   *  one of those segments.
   */

  /* tow is located in at the bottom left of the viewer area. */
  o.tow.x = dConf.vX + ((dConf.vW / 8.0) * 1.0);
  o.tow.y = dConf.vY + (dConf.vH - ((dConf.vH / 20.0) * 2.0));
  o.tow.w = (dConf.vW / 8.0) * 1.0;
  o.tow.h = (dConf.vH / 20.0) * 1.0;

  /* armies in lower right of the viewer area */
  o.arm.x = dConf.vX + ((dConf.vW / 8.0) * 6.0);
  o.arm.y = o.tow.y;
  o.arm.w = o.tow.w;
  o.arm.h = o.tow.h;

  /* for the eng/wep/fuel failure/crit 'pulse' messages (centered) */
  o.fuelcritpulse.x = dConf.vX + ((dConf.vW / 8.0) * 1.5);
  o.fuelcritpulse.y = dConf.vY + (dConf.vH - ((dConf.vH / 20.0) * 19.0));
  o.fuelcritpulse.w = ((dConf.vW / 8.0) * 5.0);
  o.fuelcritpulse.h = (dConf.vH / 20.0) * 2.0;

  o.wepfailpulse.x = dConf.vX + ((dConf.vW / 8.0) * 1.5);
  o.wepfailpulse.y = dConf.vY + (dConf.vH - ((dConf.vH / 20.0) * 16.0));
  o.wepfailpulse.w = ((dConf.vW / 8.0) * 5.0);
  o.wepfailpulse.h = (dConf.vH / 20.0) * 2.0;

  o.hullcritpulse.x = dConf.vX + ((dConf.vW / 8.0) * 1.5);
  o.hullcritpulse.y = dConf.vY + (dConf.vH - ((dConf.vH / 20.0) * 13.0));
  o.hullcritpulse.w = ((dConf.vW / 8.0) * 5.0);
  o.hullcritpulse.h = (dConf.vH / 20.0) * 2.0;

  o.shcritpulse.x = dConf.vX + ((dConf.vW / 8.0) * 1.5);
  o.shcritpulse.y = dConf.vY + (dConf.vH - ((dConf.vH / 20.0) * 8.0));
  o.shcritpulse.w = ((dConf.vW / 8.0) * 5.0);
  o.shcritpulse.h = (dConf.vH / 20.0) * 2.0;

  o.engfailpulse.x = dConf.vX + ((dConf.vW / 8.0) * 1.5);
  o.engfailpulse.y = dConf.vY + (dConf.vH - ((dConf.vH / 20.0) * 5.0));
  o.engfailpulse.w = ((dConf.vW / 8.0) * 5.0);
  o.engfailpulse.h = (dConf.vH / 20.0) * 2.0;

  /* alert target - bottom of viewer */
  o.d1atarg.x = dConf.vX + ((dConf.vW / 8.0) * 1.5);
  o.d1atarg.y = dConf.vY + (dConf.vH - ((dConf.vH / 20.0) * 1.0));
  o.d1atarg.w = ((dConf.vW / 8.0) * 5.0);
  o.d1atarg.h = (dConf.vH / 20.0) * 1.0;

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
                    TexFont *lfont, int val, int col, int boxcol)
{
  static char buf32[32];
  static int oldval = -1;

  if (val != oldval)
    {
      snprintf(buf32, 32 - 1, "%3d", val);
      oldval = val;
    }

  /* a scale value (number) */
  glfRender(x, y, 0.0, w, h, lfont, buf32, col, NULL,
            TRUE, FALSE, TRUE);
  return;
}


/* draw overload/critical  messages using a 'pulse' effect
   in the viewer.  Use a slower pulse for 'critical' levels, except
   for hull critical. */
void renderPulseMsgs(void)
{
  static animStateRec_t engfail = {}; /* animdef states  */
  static animStateRec_t wepfail = {}; 
  static animStateRec_t wepcrit = {}; 
  static animStateRec_t engcrit = {}; 
  static animStateRec_t fuelcrit = {}; 
  static animStateRec_t shcrit = {}; 
  static animStateRec_t hullcrit = {}; 
  static int firsttime = TRUE;
  static const int testlamps = 0; /* set to non-zero to see them all
                                     at once (for testing) */
  scrNode_t *curnode = getTopNode();
  int drawing = FALSE;
  
  if (firsttime)
    {
      if (!curnode->animQue)
        return;                 /* mmaybe we'll get one next time */

      firsttime = FALSE;

      /* init the anims */
      if (animInitState("overload-pulse", &engfail, NULL))
        {
          animQueAdd(curnode->animQue, &engfail);
        }

      if (animInitState("overload-pulse", &wepfail, NULL))
        {
          animQueAdd(curnode->animQue, &wepfail);
        }

      if (animInitState("critical-pulse", &engcrit, NULL))
        {
          animQueAdd(curnode->animQue, &engcrit);
        }

      if (animInitState("critical-pulse", &wepcrit, NULL))
        {
          animQueAdd(curnode->animQue, &wepcrit);
        }

      if (animInitState("overload-pulse", &fuelcrit, NULL))
        {
          animQueAdd(curnode->animQue, &fuelcrit);
        }

      if (animInitState("overload-pulse", &shcrit, NULL))
        {
          animQueAdd(curnode->animQue, &shcrit);
        }

      if (animInitState("overload-pulse", &hullcrit, NULL))
        {
          animQueAdd(curnode->animQue, &hullcrit);
        }
    }

  if (testlamps || dData.alloc.ealloc <= 0 || dData.alloc.walloc <= 0 ||
      dData.etemp.etemp > E_CRIT  || dData.wtemp.wtemp > W_CRIT ||
      dData.fuel.fuel < F_CRIT || 
      (dData.sh.shields <= SH_CRIT && 
       SSHUP(Context.snum) && !SREPAIR(Context.snum)) ||
      dData.dam.damage >= HULL_CRIT)
    drawing = TRUE;

  if (!drawing)
    return;

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
 
  if (testlamps || dData.fuel.fuel < F_CRIT)
    {
      if (ANIM_EXPIRED(&fuelcrit))
        {
          animResetState(&fuelcrit, frameTime);
          animQueAdd(curnode->animQue, &fuelcrit);
        }
      
      glfRender(o.fuelcritpulse.x, o.fuelcritpulse.y, 0.0, 
                o.fuelcritpulse.w, o.fuelcritpulse.h, 
                fontLargeTxf, "Fuel Critical", 
                0, &fuelcrit.state.col, 
                TRUE, FALSE, TRUE);
      
    }      

  if (testlamps ||dData.alloc.ealloc <= 0)
    {
      if (ANIM_EXPIRED(&engfail))
        {
          animResetState(&engfail, frameTime);
          animQueAdd(curnode->animQue, &engfail);
        }
      
      glfRender(o.engfailpulse.x, o.engfailpulse.y, 0.0, 
                o.engfailpulse.w, o.engfailpulse.h, 
                fontLargeTxf, "Engines Overloaded", 
                0, &engfail.state.col, 
                TRUE, FALSE, TRUE);
    }      
  else if (dData.etemp.etemp > E_CRIT)
    {
      if (ANIM_EXPIRED(&engcrit))
        {
          animResetState(&engcrit, frameTime);
          animQueAdd(curnode->animQue, &engcrit);
        }
      
      glfRender(o.engfailpulse.x, o.engfailpulse.y, 0.0, 
                o.engfailpulse.w, o.engfailpulse.h, 
                fontLargeTxf, "Engines Critical", 
                0, &engcrit.state.col, 
                TRUE, FALSE, TRUE);
    }      
  

  if (testlamps ||dData.alloc.walloc <= 0)
    {
      if (ANIM_EXPIRED(&wepfail))
        {
          animResetState(&wepfail, frameTime);
          animQueAdd(curnode->animQue, &wepfail);
        }

      glfRender(o.wepfailpulse.x, o.wepfailpulse.y, 0.0, 
                o.wepfailpulse.w, o.wepfailpulse.h, 
                fontLargeTxf, "Weapons Overloaded", 
                0, &wepfail.state.col, 
                TRUE, FALSE, TRUE);

    }      
  else if (dData.wtemp.wtemp > W_CRIT)
    {
      if (ANIM_EXPIRED(&wepcrit))
        {
          animResetState(&wepcrit, frameTime);
          animQueAdd(curnode->animQue, &wepcrit);
        }
      
      glfRender(o.wepfailpulse.x, o.wepfailpulse.y, 0.0, 
                o.wepfailpulse.w, o.wepfailpulse.h, 
                fontLargeTxf, "Weapons Critical", 
                0, &wepcrit.state.col, 
                TRUE, FALSE, TRUE);
      
    }      

  if (testlamps || (dData.sh.shields < SH_CRIT && SSHUP(Context.snum) && 
       !SREPAIR(Context.snum)))
    {
      if (ANIM_EXPIRED(&shcrit))
        {
          animResetState(&shcrit, frameTime);
          animQueAdd(curnode->animQue, &shcrit);
        }

      glfRender(o.shcritpulse.x, o.shcritpulse.y, 0.0, 
                o.shcritpulse.w, o.shcritpulse.h, 
                fontLargeTxf, "Shields Critical", 
                0, &shcrit.state.col, 
                TRUE, FALSE, TRUE);

    }      

  if (testlamps || dData.dam.damage >= HULL_CRIT)
    {
      if (ANIM_EXPIRED(&hullcrit))
        {
          animResetState(&hullcrit, frameTime);
          animQueAdd(curnode->animQue, &hullcrit);
        }

      glfRender(o.hullcritpulse.x, o.hullcritpulse.y, 0.0, 
                o.hullcritpulse.w, o.hullcritpulse.h, 
                fontLargeTxf, "Hull Critical", 
                0, &hullcrit.state.col, 
                TRUE, FALSE, TRUE);

    }      

  glDisable(GL_BLEND);

  return;
}

/* render the shield's current strength when down */
void renderShieldCharge(void)
{
  real val;
  
  val = CLAMP(0.0, 100.0, Ships[Context.snum].shields);

  if (!val)
    return;
  
  uiPutColor(dData.sh.color);
  
  /* gauge */
  drawLine(o.d1shcharge.x, o.d1shcharge.y, 
           o.d1shcharge.w * (GLfloat)((GLfloat)val / 100.0),
           o.d1shcharge.h);

  return;
}


/* This is Cat's icon hud */
void renderHud(int dostats)
{				/* assumes context is current*/
  static char sbuf[128];
  static char ibuf[128];        /* fps, stats, etc */
  static char fbuf[128];
  int FPS = (int)getFPS();
  cqColor icl;
  real warp = Ships[Context.snum].warp;
  real maxwarp = ShipTypes[Ships[Context.snum].shiptype].warplim;
  int steam = Ships[Context.snum].team;
  static int oldteam = -1;
  static int rxtime = 0;
  static int oldrx = 0;
  static int rxdiff = 0;
  int i;
  static int ack_alert = 0;
  static struct {
    real oldFPS;
    Unsgn32 oldPingAvg;
    real oldRxdiff;
  } oldData = {};
  int magFac;
  extern int ncpLRMagFactor; /* from nCP */
  extern int ncpSRMagFactor;


  if (!GLTextures)
    return;                     /* don't render until we actually can */

  if ((frameTime - rxtime) > 1000)
    {
      rxdiff = pktRXBytes - oldrx;
      oldrx = pktRXBytes;
      rxtime = frameTime;
    }

  /* update stats data, if needed */
  if (FPS != oldData.oldFPS || 
      pingAvgMS != oldData.oldPingAvg || rxdiff != oldData.oldRxdiff)
    {
      oldData.oldFPS = FPS;
      oldData.oldPingAvg = pingAvgMS;
      oldData.oldRxdiff = rxdiff;

      snprintf(fbuf, 128 - 1, "FPS: %03d", FPS);

      snprintf(ibuf, 128 - 1, "%4dms %3.1fKB/s %s",  
               pingAvgMS, 
               ((float)rxdiff / 1000.0),
               fbuf);
    }

  if (o.x != dConf.wX || o.y != dConf.wY || o.w != dConf.wW ||
      o.h != dConf.wH || oldteam != steam)
    {
      updateIconHudGeo(Context.snum);
      oldteam = steam;
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
            NoColor, NULL, TRUE, FALSE, TRUE);

  /* warp background */
  /* kindof sucky to do this here, but since we are drawing a quad
     sandwiched between two textures... */
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  drawIconHUDDecal(o.warp.x, o.warp.y, o.warp.w, o.warp.h,
                   HUD_WARP2, 0xffffffff);

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);

  /* warp val */
  glfRender(o.warpl.x, o.warpl.y, 0.0, 
            o.warpl.w, o.warpl.h,
            fontLargeTxf, dData.warp.warp, InfoColor, NULL, TRUE, FALSE, TRUE);
  
  /* warp quad indicator color */
  
  GLCOLOR4F(GLShips[Ships[Context.snum].team][Ships[Context.snum].shiptype].warpq_col);

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
                  dData.sh.color, NoColor);

  /* shield charging status */
  if (!SSHUP(Context.snum) || SREPAIR(Context.snum))
    renderShieldCharge();
  
  /* damage gauge */
  renderScale(o.d1damg.x, o.d1damg.y, 
              o.d1damg.w, o.d1damg.h,
              0, 100, dData.dam.damage, dData.dam.color);

  /* damage num */
  renderScaleVal(o.d1damn.x, o.d1damn.y,
                 o.d1damn.w, o.d1damn.h,
                 fontFixedTxf, dData.dam.damage, dData.dam.color,
                 NoColor);

  /* fuel guage */
  renderScale(o.d2fuelg.x, o.d2fuelg.y, o.d2fuelg.w, o.d2fuelg.h,
              0, 999, dData.fuel.fuel, dData.fuel.color);
  
  /* fuel value */
  renderScaleVal(o.d2fueln.x, o.d2fueln.y,
                 o.d2fueln.w, o.d2fueln.h,
                 fontFixedTxf, dData.fuel.fuel, dData.fuel.color,
                 NoColor);

  /* etemp guage */
  renderScale(o.d2engtg.x, o.d2engtg.y, o.d2engtg.w, o.d2engtg.h,
              0, 100, dData.etemp.etemp, dData.etemp.color);

  /* etemp value */
  renderScaleVal(o.d2engtn.x, o.d2engtn.y,
                 o.d2engtn.w, o.d2engtn.h,
                 fontFixedTxf, dData.etemp.etemp, dData.etemp.color,
                 BlueColor);

  /* wtemp gauge */
  renderScale(o.d2weptg.x, o.d2weptg.y, o.d2weptg.w, o.d2weptg.h,
              0, 100, dData.wtemp.wtemp, dData.wtemp.color);

  /* wtemp value*/
  renderScaleVal(o.d2weptn.x, o.d2weptn.y,
                 o.d2weptn.w, o.d2weptn.h,
                 fontFixedTxf, dData.wtemp.wtemp, dData.wtemp.color,
                 BlueColor);

  /* alloc */
  renderAlloc(o.d2allocg.x, o.d2allocg.y, o.d2allocg.w, o.d2allocg.h,
              &dData.alloc, fontLargeTxf, fontFixedTxf, 
              TRUE);
  
  /* alloc value */
  glfRender(o.d2allocn.x, o.d2allocn.y, 0.0, o.d2allocn.w, o.d2allocn.h, 
            fontFixedTxf, dData.alloc.allocstr, InfoColor, 
            NULL, TRUE, FALSE, TRUE);
  
  /* BEGIN "stat" box -
     kills, towing/towed by, x armies, CLOAKED/destruct */

  /* kills */

  glfRender(o.d2killb.x, o.d2killb.y, 0.0, 
            o.d2killb.w, 
            o.d2killb.h, fontFixedTxf, dData.kills.kills, InfoColor, NULL,
            TRUE, TRUE, TRUE);

  magFac = (!SMAP(Context.snum)) ? ncpSRMagFactor : ncpLRMagFactor;
  /* towed-towing/armies/destruct/alert - blended text displayed in viewer */
  if (magFac || dData.tow.str[0] || dData.armies.str[0] || 
      dData.cloakdest.str[0] == 'D' || dData.aStat.alertStatus[0])
    {
      /* we want to use blending for these */

      glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
      glEnable(GL_BLEND);
      
      if (dData.tow.str[0])
        glfRender(o.tow.x, o.tow.y, 0.0, 
                  o.tow.w, o.tow.h, 
                  fontFixedTxf, dData.tow.str, 
                  MagentaColor | 0x50000000, 
                  NULL, TRUE, FALSE, TRUE);
      
      if (dData.armies.str[0])
        glfRender(o.arm.x, o.arm.y, 0.0, 
                  o.arm.w, o.arm.h, 
                  fontFixedTxf, 
                  dData.armies.str, 
                  InfoColor | 0x50000000, 
                  NULL, TRUE, FALSE, TRUE);
      

      /* Destruct msg, centered in the viewer */
      if (dData.cloakdest.str[0] == 'D') /* destructing */
        {
          glfRender(o.cloakdest.x, o.cloakdest.y, 0.0, 
                    o.cloakdest.w, o.cloakdest.h, 
                    fontMsgTxf, 
                    dData.cloakdest.str, 
                    (GL_BLINK_ONESEC) ? 
                    dData.cloakdest.color | CQC_A_BOLD | 0x50000000: 
                    (dData.cloakdest.color | 0x50000000) & ~CQC_A_BOLD, 
                    NULL, TRUE, FALSE, TRUE);
        }

      /* alert target */
      if (dData.aStat.alertStatus[0])
        {
          switch(dData.aStat.alertStatus[0])  /* define alert decal */
            {                       /* need to blink these */
            case 'R':               /* red alert (not Alert) */
            case 'Y':               /* yellow alert (not Prox) */
              icl = (GL_BLINK_HALFSEC) ? dData.aStat.color & ~CQC_A_BOLD : 
                dData.aStat.color | CQC_A_BOLD;
              break;
            default:
              icl = dData.aStat.color;
              break;
            }
          
          glfRender(o.d1atarg.x, o.d1atarg.y, 0.0, 
                    o.d1atarg.w, o.d1atarg.h, 
                    fontLargeTxf, dData.aStat.alertStatus, 
                    icl | 0x50000000, NULL, TRUE, FALSE, TRUE);
        }

      /* magnification factor */
      if (magFac)
        {
          if (magFac > 0)
            sprintf(sbuf, "MAG +%1d", magFac);
          else 
            sprintf(sbuf, "MAG %1d", magFac);
          
          glfRender(o.magfac.x, o.magfac.y, 0.0,
                    o.magfac.w, o.magfac.h,
                    fontFixedTxf, sbuf,
                    SpecialColor | 0x50000000,
                    NULL, TRUE, FALSE, TRUE);
        }

      glDisable(GL_BLEND);
    }

  /*
    Cataboligne - sound code to handle red alert klaxon
    
    logic - ack_alert inits 0
    when in red range - if handle is INV meaning sound is off -
    if ack is OFF play snd, set ack ON
    if ack is ON - klaxon was turned off with ESC - set ack to ACK
    when out of red range -
    set ack to OFF
    stop sound if playing
    
    another idea - play sound whenever near a cloaked ship and
    (CLOAKED) displays if info gotten
  */
  if (dData.aStat.alertStatus[0] == 'R' || dData.aStat.alertStatus[0] == 'A')
    {                   /* alert condition red - check klaxon state */
      if (alertHandle == CQS_INVHANDLE) /* not playing now */
        {
          if (ack_alert == ALERT_OFF) /* was off */
            {
              ack_alert = ALERT_ON;
              cqsEffectPlayTracked(teamEffects[Ships[Context.snum].team].alert,
                                   &alertHandle, 0.0, 0.0, 0.0);
            }
          else if (ack_alert == ALERT_ON) /* was on - turned off */
            {
              ack_alert = ALERT_ACK;  /* been ack'ed in nCP.c with <ESC> */
            }
        }
    }
  else
    {
      /* Cataboligne
         - idea time out the alert if going beyond the red range?
         (or just plain stop it here...hmm) */
      
      if (alertHandle != CQS_INVHANDLE)
        {
          cqsEffectStop(alertHandle, FALSE);
          alertHandle = CQS_INVHANDLE;
        }
      ack_alert = ALERT_OFF; 
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
  
  /* hacky.. for sure. */
  if (dData.cloakdest.str[1] == 'C') 
    drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                     HUD_ICLOAK, icl);
  else
    if (SREPAIR(Context.snum)) 
      drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h,
                       HUD_IREPAIR, icl);
  
  /*
    Cataboligne + display critical alerts decals
    
    critical levels - display decal
    
    shields  < 20
    eng temp > 80 or overloaded
    wep temp > 80 or overloaded
    hull dmg >= 70
  */

  /* draw warp and decals */
  drawIconHUDDecal(o.warp.x, o.warp.y, o.warp.w, o.warp.h, 
                   HUD_WARP, icl);
  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   HUD_DECAL1, icl);
  drawIconHUDDecal(o.decal2.x, o.decal2.y, o.decal2.w, o.decal2.h, 
                   HUD_DECAL2, icl);


  /* torp pips */
  if (Context.snum > 0 && Context.snum <= MAXSHIPS)
    {
      glBindTexture(GL_TEXTURE_2D, 
                    GLShips[Ships[Context.snum].team][Ships[Context.snum].shiptype].ico_torp);
      
      for (i=0; i < MAXTORPS; i++)
        {
          switch (Ships[Context.snum].torps[i].status)
            {
            case TS_OFF:
              uiPutColor(GreenColor);
              break;
            case TS_FIREBALL:
              uiPutColor(RedColor);
              break;
            default:
              uiPutColor(NoColor);
              break;
            }
          
          drawTexQuad(o.d1torppips[i].x, 
                      o.d1torppips[i].y, 
                      o.d1torppips[i].w, 
                      o.d1torppips[i].h, 
                      0.0);
        }
    }

  /* GL */
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);

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
                  NULL, TRUE, TRUE, TRUE);
      
      if (dostats)
        {
          glfRender(o.rectime.x, o.rectime.y, 0.0, 
                    o.rectime.w, o.rectime.h,
                    fontFixedTxf, ibuf, NoColor | CQC_A_DIM, 
                    NULL, TRUE, TRUE, TRUE);
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
                NULL, TRUE, TRUE, TRUE);
      
      glfRender(o.rectime.x, o.rectime.y, 0.0, 
                o.rectime.w, o.rectime.h,
                fontFixedTxf, dData.recTime.str, NoColor, 
                NULL, TRUE, TRUE, TRUE);
    }


  /* the 3 prompt/msg lines  */

  /* MSG_LIN1 */
  if (dData.p1.str[0])
    glfRender(o.msg1.x, o.msg1.y, 0.0, 
              o.msg1.w, o.msg1.h,
              fontFixedTxf, dData.p1.str, InfoColor, 
              NULL, TRUE, TRUE, TRUE);

  /* MSG_LIN2 */
  if (dData.p2.str[0])
    glfRender(o.msg2.x, o.msg2.y, 0.0,
              o.msg2.w, o.msg2.h,
              fontFixedTxf, dData.p2.str, InfoColor, 
              NULL, TRUE, TRUE, TRUE);

  /* MSG_MSG */
  if (dData.msg.str[0])
    glfRender(o.msgmsg.x, o.msgmsg.y, 0.0, 
              o.msgmsg.w, o.msgmsg.h,
              fontMsgTxf, dData.msg.str, InfoColor, 
              NULL, TRUE, TRUE, TRUE);

  /* critical/overload indicators */
  renderPulseMsgs();

  /* phaser recharge status */
  if (Context.snum > 0 && Context.snum <= MAXSHIPS)
    {
      if (Ships[Context.snum].pfuse <= 0)
        uiPutColor(GreenColor);
      else
        uiPutColor(RedColor);
        
      drawQuad(o.d1phcharge.x, o.d1phcharge.y, o.d1phcharge.w, 
               (Ships[Context.snum].pfuse <= 0) ? o.d1phcharge.h :
               (o.d1phcharge.h - (o.d1phcharge.h / 10.0) * 
                (real)Ships[Context.snum].pfuse), 0.0);
    }

  return;
}

void renderViewer(int dovbg, int dobomb)
{
  /* setup the proper viewport and projection matrix for the viewer */
  glViewport(dConf.vX, 
             dConf.vY + (dConf.wH - dConf.vH - (dConf.borderW * 2.0)), 
             dConf.vW, 
             dConf.vH);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(dConf.vmat);
  glMatrixMode(GL_MODELVIEW);
  
  drawViewerBG(Context.snum, dovbg);
  drawNEB(Context.snum);

  display( Context.snum, FALSE );

  /* if we're faking it, (nCP.c), do it */
  if (dobomb)
    drawBombing(Context.snum);

#if 0                           /* TEST GRID */
  {
    int i;
    static const int nlines = 10; /* 30 lines, each side of 0 */
    GLfloat gx, gy;
    
    uiPutColor(InfoColor);
    for (i = 0; i < nlines; i++)
      {
        if (!i)
          gx = gy = 0.0;
        else
          GLcvtcoords(0.0, 0.0,
                      i * 1000.0, i * 1000.0, 
                      (SMAP(Context.snum) ? MAP_FAC : SCALE_FAC),
                      &gx, &gy);

        //        if (i == (nlines - 1))
        //          clog("nlines: %d gx = %f gx = %f\n", nlines, gx, gy);
        
        glBegin(GL_LINES);

        /* x */
        glVertex3f(gx, -VIEWANGLE, TRANZ);  /* left */
        glVertex3f(gx, VIEWANGLE, TRANZ);   /* right */
        
        glVertex3f(-gx, -VIEWANGLE, TRANZ); /* left */
        glVertex3f(-gx, VIEWANGLE, TRANZ);  /* right */
        
        /* y */
        glVertex3f(-VIEWANGLE, gy, TRANZ);  /* top */
        glVertex3f(VIEWANGLE, gy, TRANZ);   /* bottom */

        glVertex3f(-VIEWANGLE, -gy, TRANZ); /* top */
        glVertex3f(VIEWANGLE, -gy, TRANZ);  /* bottom */

        glEnd();
      }
  }
#endif


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
  val = CLAMP(min, max, val);
  
  uiPutColor(scalecolor);
  
  /* gauge */
  drawQuad(x, y, 
           w * (GLfloat)((GLfloat)val / ((GLfloat)max - (GLfloat)min)),
           h, 0.0);
  
  return;
}

/* like renderScale, but more specific */
void renderAlloc(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
               struct _alloc *a,
               TexFont *lfont, TexFont *vfont, int HUD)
{
  int walloc = 0;
  
  /* bg */
  if (a->ealloc > 0)
    uiPutColor(BlueColor);
  else
    uiPutColor(RedLevelColor);

  drawQuad(x, y, w, h, 0.0);

  /* fg */
  
  if (!a->ealloc)
    walloc = a->walloc;         /* e overload */
  else
    walloc = 100 - a->ealloc;

  if (a->walloc > 0)
    uiPutColor(NoColor);
  else
    uiPutColor(RedLevelColor);

  drawQuad(x, y, 
           w * (GLfloat)((GLfloat)walloc / 100.0),
           h, 0.0);

  return;
}

