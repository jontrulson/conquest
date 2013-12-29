/* 
 * higher level rendering for the CP
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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
#include "blinker.h"
#include "hud.h"

/*
 Cataboligne - 11.20.6
 ack alert klaxon with <ESC>
*/
#include "cqsound.h"

#define ALERT_OFF 0
#define ALERT_ON 1
#define ALERT_ACK 2

cqsHandle alertHandle = CQS_INVHANDLE;

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
  GLRect_t destruct;

  GLRect_t pbitem;              /* planet/ship we are watching */
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
  GLRect_t d1icon_fa;           /* the ship icon firing angle indicator */
  GLRect_t d1icon_tad;          /* last target, angle, dist indicator */
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

/* FIXME - re-generate this... Pass the main rect (decal) as a rect
   and pass decal1/2 as rects.. 

   ie: pass the proper decal_sz, and the 'hw' decal representation as
   args instead of 'global' tx,ty,tw,th - make it a friggin rect!
*/

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
  static cqiTextureAreaPtr_t d1icon_fa = NULL;  /* d1 ship icon firing ang */
  static cqiTextureAreaPtr_t d1icon_tad = NULL; /* d1 ship icon target data */

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
      d1icon_fa = cqiFindTexArea(buffer, "icon-fangle", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1icon_fa);
      d1icon_tad = cqiFindTexArea(buffer, "icon-tad", &defaultTA);
      CLAMPRECT(decal1_sz.w, decal1_sz.h, d1icon_tad);
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
  o.xstatw = dConf.vX - (dConf.wBorderW * 2.0);

  tx = dConf.vX;
  ty = dConf.vY + dConf.vH;
  /* icon height */
  o.sb_ih = (dConf.wH - (dConf.wBorderW * 2.0) - ty) / 4.0;

  /* alert border */
  o.alertb.x = dConf.vX - 3.0;
  o.alertb.y = dConf.vY - 3.0;
  o.alertb.w = dConf.vW + (3.0 * 2.0);
  o.alertb.h = dConf.vH + (3.0 * 2.0);

  tx = dConf.wBorderW;
  ty = dConf.wBorderW;

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

  /* position the 'firing angle' and 'target, ang, distance' indicators */
  MAPAREA(&decal1_sz, d1icon_fa, &o.d1icon_fa);
  MAPAREA(&decal1_sz, d1icon_tad, &o.d1icon_tad);

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

  /* tow is located at the bottom left of the viewer area. */
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
  o.destruct.x = dConf.vX + ((dConf.vW / 6.0) * 1.0);
  o.destruct.y = dConf.vY + (dConf.vH / 2.0) - ((dConf.vH / 6.0) / 2.0);
  o.destruct.w = ((dConf.vW / 6.0) * 4.0);
  o.destruct.h = (dConf.vH / 6.0);

  /* playback item (ship/planet, name, team, etc) */
  tx = dConf.vX;
  ty = dConf.vY + dConf.vH;

  o.pbitem.x = tx;
  o.pbitem.y = ty + (o.sb_ih * 0.2);
  o.pbitem.w = ((dConf.vX + dConf.vW) - tx);
  o.pbitem.h =  (o.sb_ih * 0.7);

  /* rectime or stats info */
  tx = dConf.wBorderW;

  o.rectime.x = tx;
  o.rectime.y = (ty + (o.sb_ih * 0.2));
  o.rectime.w =  o.xstatw;
  o.rectime.h =  (o.sb_ih * 0.7);

  /* the 3 prompt/msg lines  */
  o.msg1.x = tx;
  o.msg1.y = ty + (o.sb_ih * 1.0);
  o.msg1.w = (dConf.wW  - (dConf.wBorderW * 2.0));
  o.msg1.h = o.sb_ih;
  
  o.msg2.x = tx;
  o.msg2.y = ty + (o.sb_ih * 2.0);
  o.msg2.w = (dConf.wW  - (dConf.wBorderW * 2.0));
  o.msg2.h = o.sb_ih;
  
  o.msgmsg.x = tx;
  o.msgmsg.y = ty + (o.sb_ih * 3.0);
  o.msgmsg.w = (dConf.wW  - (dConf.wBorderW * 2.0));
  o.msgmsg.h = o.sb_ih;

  return;

}

static void renderScale(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
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
static void renderAlloc(GLfloat x, GLfloat y, GLfloat w, GLfloat h,
               struct _alloc *a)
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

/* draw overload/critical  messages using a 'pulse' effect
   in the viewer.  Use a slower pulse for 'critical' levels, except
   for hull critical. */
static void renderPulseMsgs(void)
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

  if (testlamps || hudData.alloc.ealloc <= 0 || hudData.alloc.walloc <= 0 ||
      hudData.etemp.temp > HUD_E_CRIT  || hudData.wtemp.temp > HUD_W_CRIT ||
      hudData.fuel.fuel < HUD_F_CRIT || 
      (hudData.sh.shields <= HUD_SH_CRIT && 
       SSHUP(Context.snum) && !SREPAIR(Context.snum)) ||
      hudData.dam.damage >= HUD_HULL_CRIT)
    drawing = TRUE;

  if (!drawing)
    return;

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
 
  if (testlamps || hudData.fuel.fuel < HUD_F_CRIT)
    {
      if (ANIM_EXPIRED(&fuelcrit))
        {
          animResetState(&fuelcrit, frameTime);
          animQueAdd(curnode->animQue, &fuelcrit);
        }
      
      glfRenderFont(o.fuelcritpulse.x, o.fuelcritpulse.y, 0.0, 
                    o.fuelcritpulse.w, o.fuelcritpulse.h, 
                    glfFontLarge, "Fuel Critical", 
                    0, &fuelcrit.state.col, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
      
    }      

  if (testlamps ||hudData.alloc.ealloc <= 0)
    {
      if (ANIM_EXPIRED(&engfail))
        {
          animResetState(&engfail, frameTime);
          animQueAdd(curnode->animQue, &engfail);
        }
      
      glfRenderFont(o.engfailpulse.x, o.engfailpulse.y, 0.0, 
                    o.engfailpulse.w, o.engfailpulse.h, 
                    glfFontLarge, "Engines Overloaded", 
                    0, &engfail.state.col, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
    }      
  else if (hudData.etemp.temp > HUD_E_CRIT)
    {
      if (ANIM_EXPIRED(&engcrit))
        {
          animResetState(&engcrit, frameTime);
          animQueAdd(curnode->animQue, &engcrit);
        }
      
      glfRenderFont(o.engfailpulse.x, o.engfailpulse.y, 0.0, 
                    o.engfailpulse.w, o.engfailpulse.h, 
                    glfFontLarge, "Engines Critical", 
                    0, &engcrit.state.col, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
    }      
  

  if (testlamps ||hudData.alloc.walloc <= 0)
    {
      if (ANIM_EXPIRED(&wepfail))
        {
          animResetState(&wepfail, frameTime);
          animQueAdd(curnode->animQue, &wepfail);
        }

      glfRenderFont(o.wepfailpulse.x, o.wepfailpulse.y, 0.0, 
                    o.wepfailpulse.w, o.wepfailpulse.h, 
                    glfFontLarge, "Weapons Overloaded", 
                    0, &wepfail.state.col, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

    }      
  else if (hudData.wtemp.temp > HUD_W_CRIT)
    {
      if (ANIM_EXPIRED(&wepcrit))
        {
          animResetState(&wepcrit, frameTime);
          animQueAdd(curnode->animQue, &wepcrit);
        }
      
      glfRenderFont(o.wepfailpulse.x, o.wepfailpulse.y, 0.0, 
                    o.wepfailpulse.w, o.wepfailpulse.h, 
                    glfFontLarge, "Weapons Critical", 
                    0, &wepcrit.state.col, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
      
    }      

  if (testlamps || (hudData.sh.shields < HUD_SH_CRIT && SSHUP(Context.snum) && 
       !SREPAIR(Context.snum)))
    {
      if (ANIM_EXPIRED(&shcrit))
        {
          animResetState(&shcrit, frameTime);
          animQueAdd(curnode->animQue, &shcrit);
        }

      glfRenderFont(o.shcritpulse.x, o.shcritpulse.y, 0.0, 
                    o.shcritpulse.w, o.shcritpulse.h, 
                    glfFontLarge, "Shields Critical", 
                    0, &shcrit.state.col, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

    }      

  if (testlamps || hudData.dam.damage >= HUD_HULL_CRIT)
    {
      if (ANIM_EXPIRED(&hullcrit))
        {
          animResetState(&hullcrit, frameTime);
          animQueAdd(curnode->animQue, &hullcrit);
        }

      glfRenderFont(o.hullcritpulse.x, o.hullcritpulse.y, 0.0, 
                    o.hullcritpulse.w, o.hullcritpulse.h, 
                    glfFontLarge, "Hull Critical", 
                    0, &hullcrit.state.col, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

    }      

  glDisable(GL_BLEND);

  return;
}

/* render the shield's current strength when down */
static void renderShieldCharge(void)
{
  real val;
  
  val = CLAMP(0.0, 100.0, Ships[Context.snum].shields);

  if (!val)
    return;
  
  uiPutColor(hudData.sh.color);
  
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
  int stype = Ships[Context.snum].shiptype;
  int snum = Context.snum;
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
      rxdiff = pktStats.rxBytes - oldrx;
      oldrx = pktStats.rxBytes;
      rxtime = frameTime;
    }

  /* update stats data, if needed */
  if (FPS != oldData.oldFPS || 
      pktStats.pingAvg != oldData.oldPingAvg || rxdiff != oldData.oldRxdiff)
    {
      oldData.oldFPS = FPS;
      oldData.oldPingAvg = pktStats.pingAvg;
      oldData.oldRxdiff = rxdiff;

      snprintf(fbuf, 128 - 1, "FPS: %03d", FPS);

      snprintf(ibuf, 128 - 1, "%4dms %3.1fKB/s %s",  
               pktStats.pingAvg, 
               ((float)rxdiff / 1000.0),
               fbuf);
    }

  if (o.x != dConf.wX || o.y != dConf.wY || o.w != dConf.wW ||
      o.h != dConf.wH || oldteam != steam)
    {
      updateIconHudGeo(snum);
      oldteam = steam;
    }

  /* draw alert border */
  drawLineBox(o.alertb.x, o.alertb.y, 0.0,
              o.alertb.w,
              o.alertb.h,
              hudData.aStat.color,
              2.0);


  /* The Icon Hud (TM) in all it's glory... */

  /* heading val */
  glfRenderFont(o.headl.x, o.headl.y, 0.0, o.headl.w, 
                o.headl.h, glfFontLarge, hudData.heading.str, 
                hudData.heading.color, NULL, 
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

  /* warp background */
  /* kindof sucky to do this here, but since we are drawing a quad
     sandwiched between two textures... */
  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  drawIconHUDDecal(o.warp.x, o.warp.y, o.warp.w, o.warp.h,
                   TEX_HUD_WARP2, 0xffffffff);

  glDisable(GL_BLEND);
  glDisable(GL_TEXTURE_2D);

  /* warp val */
  glfRenderFont(o.warpl.x, o.warpl.y, 0.0, 
                o.warpl.w, o.warpl.h,
                glfFontLarge, hudData.warp.str, 
                hudData.warp.color, NULL, 
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
  
  /* warp quad indicator color */
  
  glColor4fv(GLTEX_COLOR(GLShips[steam][stype].warpq_col).vec);

  /* warp indicator quad */
  if (warp >= 0.1)
    drawQuad(o.warp.x, 
             o.warp.y, (o.warp.w * (warp / maxwarp)), 
             o.warp.h * 0.79 /*empirical. ick.*/, 0.0);

  /* shields gauge */
  if (hudData.sh.shields > 0.0)
    renderScale(o.d1shg.x, o.d1shg.y, o.d1shg.w, o.d1shg.h,
                0, 100, hudData.sh.shields, hudData.sh.color);
  
  /* shields num */
  glfRenderFont(o.d1shn.x, o.d1shn.y, 0.0, o.d1shn.w, o.d1shn.h, 
                glfFontFixed, 
                hudData.sh.str, hudData.sh.color, 
                NULL, 
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

  /* shield charging status */
  if (!SSHUP(snum) || SREPAIR(snum))
    renderShieldCharge();
  
  /* damage gauge */
  renderScale(o.d1damg.x, o.d1damg.y, 
              o.d1damg.w, o.d1damg.h,
              0, 100, hudData.dam.damage, hudData.dam.color);

  /* damage num */
  glfRenderFont(o.d1damn.x, o.d1damn.y, 0.0, o.d1damn.w, o.d1damn.h, 
                glfFontFixed, 
                hudData.dam.str, hudData.dam.color, 
                NULL, 
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

  /* fuel guage */
  renderScale(o.d2fuelg.x, o.d2fuelg.y, o.d2fuelg.w, o.d2fuelg.h,
              0, 999, hudData.fuel.fuel, hudData.fuel.color);
  
  /* fuel value */
  glfRenderFont(o.d2fueln.x, o.d2fueln.y, 0.0, o.d2fueln.w, o.d2fueln.h, 
                glfFontFixed, 
                hudData.fuel.str, hudData.fuel.color, 
                NULL, 
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

  /* etemp guage */
  renderScale(o.d2engtg.x, o.d2engtg.y, o.d2engtg.w, o.d2engtg.h,
              0, 100, hudData.etemp.temp, hudData.etemp.color);

  /* etemp value */
  glfRenderFont(o.d2engtn.x, o.d2engtn.y, 0.0, o.d2engtn.w, o.d2engtn.h, 
                glfFontFixed, 
                hudData.etemp.str, hudData.etemp.color, 
                NULL, 
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

  /* wtemp gauge */
  renderScale(o.d2weptg.x, o.d2weptg.y, o.d2weptg.w, o.d2weptg.h,
              0, 100, hudData.wtemp.temp, hudData.wtemp.color);

  /* wtemp value*/
  glfRenderFont(o.d2weptn.x, o.d2weptn.y, 0.0, o.d2weptn.w, o.d2weptn.h, 
                glfFontFixed, 
                hudData.wtemp.str, hudData.wtemp.color, 
                NULL, 
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

  /* alloc */
  renderAlloc(o.d2allocg.x, o.d2allocg.y, o.d2allocg.w, o.d2allocg.h,
              &hudData.alloc);
  
  /* alloc value */
  glfRenderFont(o.d2allocn.x, o.d2allocn.y, 0.0, o.d2allocn.w, o.d2allocn.h, 
                glfFontFixed, hudData.alloc.str, hudData.alloc.color, 
                NULL, 
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
  
  /* BEGIN "stat" box -
     kills, towing/towed by, x armies, CLOAKED/destruct */

  /* kills */

  glfRenderFont(o.d2killb.x, o.d2killb.y, 0.0, 
                o.d2killb.w, 
                o.d2killb.h, glfFontFixed, 
                hudData.kills.str, 
                hudData.kills.color, NULL,
                GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);

  magFac = (!SMAP(snum)) ? ncpSRMagFactor : ncpLRMagFactor;
  /* towed-towing/armies/destruct/alert - blended text displayed in viewer */
  if (magFac || hudData.tow.towstat || hudData.armies.armies || 
      hudData.destruct.fuse || hudData.aStat.alertLevel)
    {
      /* we want to use blending for these */

      glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_ONE);
      glEnable(GL_BLEND);
      
      if (hudData.tow.towstat)
        glfRenderFont(o.tow.x, o.tow.y, 0.0, 
                      o.tow.w, o.tow.h, 
                      glfFontFixed, hudData.tow.str, 
                      hudData.tow.color | 0x50000000, 
                      NULL, 
                      GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
      
      /* army count or robot action - we position the robot action
       *  at the same location as the army count...
       */
      if (SROBOT(snum))
        glfRenderFont(o.arm.x, o.arm.y, 0.0, 
                      o.arm.w, o.arm.h, 
                      glfFontFixed, 
                      hudData.raction.str, 
                      hudData.raction.color | 0x50000000, 
                      NULL, 
                      GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
      else if (hudData.armies.armies)
        glfRenderFont(o.arm.x, o.arm.y, 0.0, 
                      o.arm.w, o.arm.h, 
                      glfFontFixed, 
                      hudData.armies.str, 
                      hudData.armies.color | 0x50000000, 
                      NULL, 
                      GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
        
      /* Destruct msg, centered in the viewer */
      if (hudData.destruct.fuse) /* destructing */
        {
          glfRenderFont(o.destruct.x, o.destruct.y, 0.0, 
                        o.destruct.w, o.destruct.h, 
                        glfFontMsg, 
                        hudData.destruct.str, 
                        (BLINK_ONESEC) ? 
                        hudData.destruct.color | CQC_A_BOLD | 0x50000000: 
                        (hudData.destruct.color | 0x50000000) & ~CQC_A_BOLD, 
                        NULL, 
                        GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
        }

      /* alert target */
      if (hudData.aStat.alertLevel != GREEN_ALERT)
        {
          switch(hudData.aStat.alertLevel)  /* define alert decal */
            {                               /* need to blink these */
            case PHASER_ALERT:               /* red alert (not Alert) */
            case YELLOW_ALERT:               /* yellow alert (not Prox) */
              icl = (BLINK_HALFSEC) ? hudData.aStat.color & ~CQC_A_BOLD : 
                hudData.aStat.color | CQC_A_BOLD;
              break;
            default:
              icl = hudData.aStat.color;
              break;
            }
          
          glfRenderFont(o.d1atarg.x, o.d1atarg.y, 0.0, 
                        o.d1atarg.w, o.d1atarg.h, 
                        glfFontLarge, hudData.aStat.str, 
                        icl | 0x50000000, NULL, 
                        GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
        }

      /* magnification factor */
      if (magFac)
        {
          if (magFac > 0)
            sprintf(sbuf, "MAG +%1d", magFac);
          else 
            sprintf(sbuf, "MAG %1d", magFac);
          
          glfRenderFont(o.magfac.x, o.magfac.y, 0.0,
                        o.magfac.w, o.magfac.h,
                        glfFontFixed, sbuf,
                        SpecialColor | 0x50000000,
                        NULL, 
                        GLF_FONT_F_SCALEX | GLF_FONT_F_ORTHO);
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
  if (hudData.aStat.alertLevel == PHASER_ALERT || 
      hudData.aStat.alertLevel == RED_ALERT)
    {                   /* alert condition red - check klaxon state */
      if (alertHandle == CQS_INVHANDLE) /* not playing now */
        {
          if (ack_alert == ALERT_OFF) /* was off */
            {
              ack_alert = ALERT_ON;
              cqsEffectPlay(cqsTeamEffects[steam].alert,
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
  
  /* icon shield decal */
  if (hudData.sh.shields > 0.0)
    {
      if (SCLOAKED(snum)) 
        icl = hudData.sh.color | 0x80000000; /* set some alpha if cloaked */
      else 
        icl = hudData.sh.color;
      
      drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h, 
                       TEX_HUD_SHI, icl);
    }

  
  /* ship icon decal */
  if (SCLOAKED(snum)) 
    icl = hudData.dam.color | 0x80000000; /* set some alpha if cloaked */
  else 
    icl = hudData.dam.color;

  drawIconHUDDecal(o.d1icon.x, o.d1icon.y, o.d1icon.w, o.d1icon.h, 
                   TEX_HUD_ICO, icl);

  icl = 0xFFFFFFFF;
  
  /* heading decal */
  drawIconHUDDecal(o.head.x, o.head.y, o.head.w, o.head.h,
                   TEX_HUD_HEAD, icl);
  
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  /* draw the heading pointer (dialp) */
  /* GL */
  glPushMatrix();
  glTranslatef(o.head.x + (o.head.w / 2.0), 
               o.head.y + (o.head.h / 2.0), 0.0);
  glRotatef(Ships[snum].head, 0.0, 0.0, -1.0);
  drawIconHUDDecal(-(o.head.w / 2.0), -(o.head.h / 2.0), 
                   o.head.w, o.head.h,
                   TEX_HUD_HDP, icl);
  glPopMatrix();

  /* regular conq stuff - tractor beam, armies, cloak */

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
                   TEX_HUD_WARP, icl);
  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1, icl);
  drawIconHUDDecal(o.decal2.x, o.decal2.y, o.decal2.w, o.decal2.h, 
                   TEX_HUD_DECAL2, icl);

  /* ico lamps */

  /* shields 
   *  during repair, if your shields are up, this will blink green.  Just so
   *  you know :)
   */
  icl = 0;
  if (SSHUP(snum))
    {
      if (hudData.sh.shields <= HUD_SH_CRIT)
        icl = (BLINK_QTRSEC) ? hudData.sh.color : 0;
      else
        icl = hudData.sh.color;
    }

  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1_LAMP_SH, icl);

  /* hull */
  icl = 0;
  if (hudData.dam.damage > HUD_HULL_ALRT)
    {
      if (hudData.dam.damage > HUD_HULL_CRIT)
        icl = (BLINK_QTRSEC) ? hudData.dam.color : 0;
      else
        icl = hudData.dam.color;
    }        

  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1_LAMP_HULL, icl);

  /* fuel */
  icl = 0;
  if (hudData.fuel.fuel < HUD_F_ALRT)
    {
      if (hudData.fuel.fuel < HUD_F_CRIT)
        icl = (BLINK_QTRSEC) ? hudData.fuel.color : 0;
      else
        icl = hudData.fuel.color;
    }

  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1_LAMP_FUEL, icl);

  /* engines */
  icl = 0;

  if (hudData.etemp.overl || hudData.etemp.temp > HUD_E_ALRT)
    {
      if (hudData.etemp.overl)
        icl = (BLINK_QTRSEC) ? hudData.etemp.color : 0;
      else if (hudData.etemp.temp > HUD_E_CRIT)
        icl = (BLINK_HALFSEC) ? hudData.etemp.color : 0;
      else
        icl = hudData.etemp.color;
    }        

  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1_LAMP_ENG, icl);

  /* weapons */
  icl = 0;

  if (hudData.wtemp.overl || hudData.wtemp.temp > HUD_W_ALRT)
    {
      if (hudData.wtemp.overl)
        icl = (BLINK_QTRSEC) ? hudData.wtemp.color : 0;
      else if (hudData.wtemp.temp > HUD_W_CRIT)
        icl = (BLINK_HALFSEC) ? hudData.wtemp.color : 0;
      else
        icl = hudData.wtemp.color;
    }        

  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1_LAMP_WEP, icl);

  /* cloaking */
  icl = 0;

  if (SCLOAKED(snum)) 
    icl = (BLINK_ONESEC) ? MagentaColor : MagentaColor | CQC_A_DIM;

  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1_LAMP_CLOAK, icl);

  /* repairing */
  icl = 0;

  if (SREPAIR(snum))
    icl = (BLINK_ONESEC) ? CyanColor : CyanColor | CQC_A_DIM;

  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1_LAMP_REP, icl);

  /* towing/towedby */
  icl = 0;

  if (hudData.tow.towstat)
    icl = (BLINK_ONESEC) ? CyanColor : CyanColor | CQC_A_DIM;

  drawIconHUDDecal(o.decal1.x, o.decal1.y, o.decal1.w, o.decal1.h, 
                   TEX_HUD_DECAL1_LAMP_TOW, icl);

  /* torp pips */
  if (snum > 0 && snum <= MAXSHIPS)
    {
      glBindTexture(GL_TEXTURE_2D, 
                    GLTEX_ID(GLShips[steam][stype].ico_torp));
      
      for (i=0; i < MAXTORPS; i++)
        {
          switch (Ships[snum].torps[i].status)
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
                      0.0,
                      o.d1torppips[i].w, 
                      o.d1torppips[i].h, 
                      TRUE, FALSE);
        }
    }

  /* phaser recharge status */
  if (snum > 0 && snum <= MAXSHIPS)
    {
      GLfloat phasH;

      /* draw the ship's phaser */
      glBindTexture(GL_TEXTURE_2D, 
                    GLTEX_ID(GLShips[steam][stype].phas));

      glColor4fv(GLTEX_COLOR(GLShips[steam][stype].phas).vec);


      phasH = (Ships[snum].pfuse <= 0) ? o.d1phcharge.h :
        (o.d1phcharge.h - (o.d1phcharge.h / 10.0) * 
         (real)Ships[snum].pfuse);
      
      drawTexQuad(o.d1phcharge.x, o.d1phcharge.y, 0.0, o.d1phcharge.w, phasH,
                  TRUE, TRUE);
    }

  /* GL */
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_BLEND);

  /* if phasers are recharging, draw a box around the recharge indicator */

  if (snum > 0 && snum <= MAXSHIPS)
    {
      if (Ships[snum].pfuse > 0)
        drawLineBox(o.d1phcharge.x, o.d1phcharge.y, 0.0,
                    o.d1phcharge.w,
                    o.d1phcharge.h,
                    RedColor,
                    1.0);
    }

  /* END stat box */
  
  /* last firing angle, target angle and distance icon indicators */

  /* first update the data if neccessary */
  if (snum > 0)
    hudSetInfoFiringAngle(Ships[snum].lastblast);
  hudSetInfoTargetAngle(Context.lasttang);
  hudSetInfoTargetDist(Context.lasttdist);

  if (UserConf.hudInfo)
    {
      /* render fa */
      glfRenderFont(o.d1icon_fa.x, o.d1icon_fa.y, 
                    0.0, 
                    o.d1icon_fa.w, o.d1icon_fa.h,
                    glfFontFixed, hudData.info.lastblaststr, NoColor, 
                    NULL, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);
      
      /* render tad */
      if (hudData.info.lasttadstr[0])
        glfRenderFont(o.d1icon_tad.x, o.d1icon_tad.y, 
                      0.0, 
                      o.d1icon_tad.w, o.d1icon_tad.h,
                      glfFontFixed, hudData.info.lasttadstr, NoColor, 
                      NULL, 
                      GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);
    }

  if ((Context.recmode != RECMODE_PLAYING) &&
      (Context.recmode != RECMODE_PAUSED))
    {
      if (dostats)
        {
          glfRenderFont(o.rectime.x, o.rectime.y, 0.0, 
                        o.rectime.w, o.rectime.h,
                        glfFontFixed, ibuf, NoColor | CQC_A_DIM, 
                        NULL, 
                        GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);
        }          
    }
  else
    {
      /* for playback, the ship/item we are watching */
      glfRenderFont(o.pbitem.x, o.pbitem.y, 
                    0.0, 
                    o.pbitem.w, o.pbitem.h,
                    glfFontFixed, hudData.recId.str, 
                    MagentaColor | CQC_A_BOLD, 
                    NULL, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);
      
      glfRenderFont(o.rectime.x, o.rectime.y, 0.0, 
                    o.rectime.w, o.rectime.h,
                    glfFontFixed, hudData.recTime.str, NoColor, 
                    NULL, 
                    GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);
    }


  /* the 3 prompt/msg lines  */

  /* MSG_LIN1 */
  if (hudData.p1.str[0])
    glfRenderFont(o.msg1.x, o.msg1.y, 0.0, 
                  o.msg1.w, o.msg1.h,
                  glfFontFixed, hudData.p1.str, InfoColor, 
                  NULL, 
                  GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);

  /* MSG_LIN2 */
  if (hudData.p2.str[0])
    glfRenderFont(o.msg2.x, o.msg2.y, 0.0,
                  o.msg2.w, o.msg2.h,
                  glfFontFixed, hudData.p2.str, InfoColor, 
                  NULL, 
                  GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);

  /* MSG_MSG */
  if (hudData.msg.str[0])
    glfRenderFont(o.msgmsg.x, o.msgmsg.y, 0.0, 
                  o.msgmsg.w, o.msgmsg.h,
                  glfFontMsg, hudData.msg.str, InfoColor, 
                  NULL, 
                  GLF_FONT_F_SCALEX | GLF_FONT_F_DOCOLOR | GLF_FONT_F_ORTHO);

  /* critical/overload indicators */
  renderPulseMsgs();

  return;
}

void renderViewer(int dovbg, int dobomb)
{
  /* setup the proper viewport and projection matrix for the viewer */
  glViewport(dConf.vX, 
             dConf.vY + (dConf.wH - dConf.vH - (dConf.wBorderW * 2.0)), 
             dConf.vW, 
             dConf.vH);
  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(dConf.viewerProjection);
  glMatrixMode(GL_MODELVIEW);
  
  drawViewerBG(Context.snum, dovbg);
  drawNEB(Context.snum);

  display( Context.snum );

  /* if we're faking it, (nCP.c), do it */
  if (dobomb)
    drawBombing(Context.snum, (SMAP(Context.snum) ? MAP_FAC : SCALE_FAC));

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
        //          utLog("nlines: %d gx = %f gx = %f\n", nlines, gx, gy);
        
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
  glLoadMatrixf(dConf.hudProjection);
  glMatrixMode(GL_MODELVIEW);

  return;
}

