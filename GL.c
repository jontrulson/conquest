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
#include "ibuf.h"
#define NOEXTERN_DCONF
#include "gldisplay.h"
#undef NOEXTERN_DCONF
#include "node.h"
#include "conf.h"
#include "cqkeys.h"
#include "record.h"

extern void conqend(void);

#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>

#include <assert.h>

#include "ui.h"

#include "glmisc.h"
#include "glfont.h"

#define NOEXTERN
#include "conquest.h"

dspData_t dData;

int frame=0, gtime, timebase=0;
static float FPS = 0.0;

Bool drawing = True;

/* textures... */
typedef struct              
{
  GLubyte	*imageData; 
  GLuint	bpp;        
  GLuint	width;      
  GLuint	height;     
  GLuint	texID;      
} textureImage;             

struct _texinfo {
  char *filename;
  unsigned char alpha;
};

#define TEX_SUN 0
#define TEX_CLASSM 1
#define TEX_CLASSD 2
#define TEX_EXPLODE 3

#define TEX_SHIP_FSC 4
#define TEX_SHIP_FDE 5
#define TEX_SHIP_FCR 6

#define TEX_SHIP_KSC 7
#define TEX_SHIP_KDE 8
#define TEX_SHIP_KCR 9

#define TEX_SHIP_RSC 10
#define TEX_SHIP_RDE 11
#define TEX_SHIP_RCR 12

#define TEX_SHIP_OSC 13
#define TEX_SHIP_ODE 14
#define TEX_SHIP_OCR 15

#define TEX_DOOMSDAY 16

#define TEX_VBG      17

#define TEX_TORP     18

#define NUM_TEX 19

struct _texinfo TexInfo[NUM_TEX] = { /* need to correlate with defines above */
  { "img/star.tga",      255 },

  { "img/classm.tga",    255 },
  { "img/classd.tga",    255 },

  { "img/explode.tga",   255 },

  { "img/shipfsc.tga",   255 },
  { "img/shipfde.tga",   255 },
  { "img/shipfcr.tga",   255 },

  { "img/shipksc.tga",   255 },
  { "img/shipkde.tga",   255 },
  { "img/shipkcr.tga",   255 },

  { "img/shiprsc.tga",   255 },
  { "img/shiprde.tga",   255 },
  { "img/shiprcr.tga",   255 },

  { "img/shiposc.tga",   255 },
  { "img/shipode.tga",   255 },
  { "img/shipocr.tga",   255 },

  { "img/doomsday.tga",  255 },

  { "img/vbg.tga",   255 },

  { "img/torp.tga",  255 },
};

GLuint  textures[NUM_TEX];       /* texture storage */

static void resize(int w, int h);
static void charInput(unsigned char key, int x, int y);
static void input(int key, int x, int y);
static int LoadGLTextures(void);
static void renderFrame(void);
static int renderNode(void);

float getFPS(void)
{
  return FPS;
}

/* get the 'real' strlen of a string, skipping past any embedded colors */
int uiCStrlen(char *buf)
{
  register char *p;
  register int l;

  l = 0;
  p = buf;
  while (*p)
    {
      if (*p == '#')
        {                       /* a color sequence */
          p++;
          while (*p && isdigit(*p))
            p++;

          if (*p == '#')
            p++;
        }
      else
        {
          p++;
          l++;
        }
    }

  return l;
}


/* return a 'space' buffer for padding */
char *padstr(int l)
{
  static char padding[256 + 1];

  if (l > 256)
    l = 256;
  
  memset(padding, ' ', l);
  padding[l] = 0;

  return padding;
}


static void mouse(int b, int state, int x, int y)
{
  /*  clog("MOUSE CLICK: b = %d, state = %d x = %d, y = %d", b, state, x, y);*/
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

void drawRect(GLfloat x, GLfloat y, GLfloat w, GLfloat h)
{				/* draw a square centered on x,y
				   USE BETWEEN glBegin/End pair! */
  const GLfloat z = TRANZ;
  GLfloat rx, ry;

#if 0
  clog("%s: x = %f, y = %f, w = %f h = %f\n", __FUNCTION__, x, y,
       w, h);
#endif

  rx = x - (w / 2.0);
  ry = y - (h / 2.0);

  glVertex3f(rx, ry, z); /* ll */
  glVertex3f(rx + w, ry, z); /* lr */
  glVertex3f(rx + w, ry + h, z); /* ur */
  glVertex3f(rx, ry + h, z); /* ul */

  return;
}

void drawBox(GLfloat x, GLfloat y, GLfloat size)
{				/* draw a square centered on x,y
				   USE BETWEEN glBegin/End pair! */
  const GLfloat z = 0.0;
  GLfloat rx, ry;

#if 0
  clog("%s: x = %f, y = %f\n", __FUNCTION__, x, y);
#endif

  rx = x - (size / 2);
  ry = y - (size / 2);

  glVertex3f(rx, ry, z); /* ll */
  glVertex3f(rx + size, ry, z); /* lr */
  glVertex3f(rx + size, ry + size, z); /* ur */
  glVertex3f(rx, ry + size, z); /* ul */

  return;
}

void drawQuad(GLfloat x, GLfloat y, GLfloat w, GLfloat h)
{
  glBegin(GL_POLYGON);
  glVertex3f(x, y, 0.0); /* ll */
  glVertex3f(x + w, y, 0.0); /* lr */
  glVertex3f(x + w, y + h, 0.0); /* ur */
  glVertex3f(x, y + h, 0.0); /* ul */
  glEnd();

  return;
}


void drawTexBox(GLfloat x, GLfloat y, GLfloat size)
{				/* draw textured square centered on x,y
				   USE BETWEEN glBegin/End pair! */
  const GLfloat z = -5.0;	/* we want to drop these down a bit
				   so that they don't occlude anything */
  GLfloat rx, ry;

#ifdef DEBUG
  clog("%s: x = %f, y = %f\n", __FUNCTION__, x, y);
#endif

  rx = x - (size / 2);
  ry = y - (size / 2);

  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(rx, ry, z); /* ll */

  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(rx + size, ry, z); /* lr */

  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(rx + size, ry + size, z); /* ur */

  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(rx, ry + size, z); /* ul */

  return;
}

void drawExplosion(GLfloat x, GLfloat y)
{
  const GLfloat z = 5.0;

  glPushMatrix();
  glLoadIdentity();

  /* translate to correct position, */
  glTranslatef(x , y , TRANZ);
  /* THEN rotate ;-) */
  glRotatef(rnduni( 0.0, 360.0 ), 0.0, 0.0, z);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, textures[TEX_EXPLODE]);
  
  glColor3f(1.0, 0.5, 0.0);	/* orange. */

  glBegin(GL_POLYGON);
  drawTexBox(0.0, 0.0, 7.5);
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
  GLfloat size = 10.0;
  char buf32[32];
  char torpchar;
  char planame[BUFFER_SIZE];
  int showpnams = UserConf.ShowPlanNames;

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

  what = Planets[pnum].type;

  glPushMatrix();
  glLoadIdentity();
  
  
  if (what == PLANET_SUN)
    {
      glEnable(GL_TEXTURE_2D); 
      glBindTexture(GL_TEXTURE_2D, textures[TEX_SUN]);
    }
  else if (what == PLANET_CLASSM)
    {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, textures[TEX_CLASSM]);
    }
  else if (what == PLANET_DEAD || what == PLANET_MOON)
    {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, textures[TEX_CLASSD]);
    }
  
  glTranslatef(0.0, 0.0, TRANZ);
  
  GLError();

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);
  
  glBegin(GL_POLYGON);
  
  switch ( what )
    {
    case PLANET_SUN:
      /* choose a good color for the main suns */
      switch(pnum)
        {
        case PNUM_SOL:		/* yellow */
        case PNUM_KEJELA:
          glColor3f(0.8, 0.8, 0.0);	
          break;
        case PNUM_SIRIUS:	/* red */
        case PNUM_SYRINX:
          glColor3f(0.8, 0.0, 0.0);
          break;
        case PNUM_BETELGEUSE:	/* blue */
        case PNUM_MURISAK:
          glColor3f(0.0, 0.0, 0.8);
          break;
          
        default:		/* red */
          glColor3f(0.8, 0.0, 0.0);
          break;
        }
      
      size = 50.0;		/* 'magnify' the texture */
      break;
    case PLANET_CLASSM:
      glColor3f(0.9, 0.9, 0.9);
      size = 10.0;
      break;
    case PLANET_CLASSA:
    case PLANET_CLASSO:
    case PLANET_CLASSZ:
    case PLANET_DEAD:
      glColor3f(0.7, 0.7, 0.7);	/* ? */
      size = 10.0;
      break;
    case PLANET_GHOST:
      glColor3f(0.0, 1.0, 0.0);	/* green */
      size = 10.0;
      break;
    case PLANET_MOON:
      glColor3f(0.5, 0.5, 0.5);	/* grey */
      size = 5.0;
      break;
    default:
      break;
    }

  if (scale == MAP_FAC)
    size /= 4.0;
  
  drawTexBox(x, y, size);
  
  glEnd();
  
  glDisable(GL_TEXTURE_2D); 
  glDisable(GL_BLEND); 

  /*  text data... */
  if (scale == SCALE_FAC)
    {
      if (showpnams)
        {
          snprintf(buf32, 32 - 1, "%s", Planets[pnum].name);
          drawString(x, y - ((GLfloat)size / 2.0), -5.0, buf32, vFontDL, textcolor);
        }
    }
  else
    {                           /* MAP_FAC */
      if (Planets[pnum].type == PLANET_SUN || 
          !Planets[pnum].scanned[Ships[Context.snum].team] )
        torpchar = ' ';
      else
        if ( Planets[pnum].armies <= 0 || Planets[pnum].team < 0 || 
             Planets[pnum].team >= NUMPLAYERTEAMS )
          torpchar = '-';
        else
          torpchar = Teams[Planets[pnum].team].torpchar;

      if (showpnams)
        {
          strcpy(planame, Planets[pnum].name);
          planame[3] = EOS;    /* just want first 3 chars */
        }
      else
        planame[0] = EOS;

      if (UserConf.DoNumMap && (torpchar != ' '))
        snprintf(buf32, 32 - 1, "%c%d%c%s", 
                 torpchar,
                 Planets[pnum].armies,
                 torpchar,
                 planame);
      else
        snprintf(buf32, 32 - 1, "%c%c%c%s", 
                 torpchar,
                 ConqInfo->chrplanets[Planets[pnum].type],
                 torpchar,
                 planame);

      
      drawString(x, 
                 y - ((GLfloat)size / 2.0),
                 -5.0, buf32, vFontDL, textcolor);

    }

  glPopMatrix();

  return;
  
}


/*  puthing - put an object on the display */
/*  SYNOPSIS */
/*    int what, lin, col */
/*    cumPutThing( what, lin, col ) */
void glPutThing( int what, GLfloat x, GLfloat y, int id)
{
#ifdef DEBUG
  clog("glputthing: what = %d, lin = %.1f, col = %.1f\n",
       what, x, y);
#endif

  /* first things first */
  if (what == THING_EXPLOSION)
    {
      drawExplosion(x, y);
      return;
    }

  /* first things first */
  if (what == THING_DEATHSTAR)
    {
      /* nothing here yet. */
      return;
    }

  return;
  
}

void GLcvtcoords(real cenx, real ceny, real x, real y, real scale,
		 GLfloat *rx, GLfloat *ry )
{
  GLfloat rscale;

  /* 21 = lines in viewer in curses client. */
  rscale = (21.0 * (float)scale / (VIEWANGLE * 2));

  *rx = (((VIEWANGLE * dConf.vAspect) - (x-cenx)) / rscale) * -1;
  *ry = ((VIEWANGLE - (y-ceny)) / rscale) * -1;

#ifdef DEBUG
  clog("GLCVTCOORDS: cx = %.2f, cy = %.2f, \n\tx = %.2f, y = %.2f, glx = %.2f,"
       " gly = %.2f \n",
       cenx, ceny, x, y, *rx, *ry);
#endif

 return; 
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

void setWarp(char *warp)
{
  int l = sizeof(dData.warp.warp);

  strncpy(dData.warp.warp, warp, 
          l - 1);
  dData.warp.warp[l - 1] = 0;

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

  strncpy(dData.alloc.allocstr, alloc, l - 1);
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


static void dspInitData(void)
{
  memset((void *)&dConf, 0, sizeof(dspConfig_t));

  dConf.wX = dConf.wY = 0;
  dConf.wW = 800;
  dConf.wH = 600;
  dConf.viewerwmapped = FALSE;

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

  return;
}
  
int uiGLInit(int *argc, char **argv)
{
#ifdef DEBUG_GL
  clog("uiGLInit: ENTER");
#endif
  memset(&ConqData, 0, sizeof(ConqData));

  dConf.inited = False;

  dspInitData();

  glutInit(argc, argv);
  glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_ALPHA);

  glutInitWindowPosition(0,0);
  glutInitWindowSize(dConf.wW, dConf.wH);
  
  dConf.mainw = glutCreateWindow(CONQUESTGL_NAME);

  glutKeyboardFunc       (charInput);
  glutSpecialFunc        (input);
  glutMouseFunc          (mouse);
  glutPassiveMotionFunc  (NULL);
  glutMotionFunc         (NULL);
  glutDisplayFunc        (renderFrame);
  glutIdleFunc           (renderFrame);
  glutReshapeFunc        (resize);
  glutEntryFunc          (NULL);

  /* will be resized/positioned later */
  dConf.viewerw = glutCreateSubWindow(dConf.mainw, 0, 0, 4, 4);

  glutKeyboardFunc       (charInput);
  glutSpecialFunc        (input);
  glutMouseFunc          (mouse);
  glutPassiveMotionFunc  (NULL);
  glutMotionFunc         (NULL);
  glutDisplayFunc        (renderFrame);
  glutIdleFunc           (renderFrame);
  glutReshapeFunc        (resize);
  glutEntryFunc          (NULL);

  glutHideWindow();

  return 0;             
}

void graphicsInit(void)
{
  glClearDepth(1.0);
  glClearColor(0.0, 0.0, 0.0, 0.0);  /* clear to black */

  glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  
  glMatrixMode(GL_PROJECTION);
  gluPerspective(VIEWANGLE, 1.0, 10.0, 200.0);
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(0.0, 0.0, TRANZ);

  if (!LoadGLTextures())
    clog("ERROR: LoadTextures() failed\n");

}

void
resize(int w, int h)
{
  static int vinit = FALSE;
  static int minit = FALSE;

  if ((glutGetWindow() == dConf.viewerw) && !vinit)
    {
      vinit = TRUE;
      graphicsInit();
      initFonts();
    }

  if ((glutGetWindow() == dConf.mainw) && !minit)
    {
      minit = TRUE;
      initTexFonts();
    }

#ifdef DEBUG_GL
  clog("GL: RESIZE: WIN = %d w = %d h = %d, \n"
       "    vX = %f, vY = %f, vW = %f, vH = %f",
       glutGetWindow(), w, h, 
       dConf.vX, dConf.vY, dConf.vW, dConf.vH);
#endif

  if (glutGetWindow() == dConf.mainw)
    {
      dConf.wW = (GLfloat)w;
      dConf.wH = (GLfloat)h;
      dConf.mAspect = (GLfloat)w/(GLfloat)h;

      /* calculate the border width */
      dConf.borderW = ((dConf.wW * 0.01) + (dConf.wH * 0.01)) / 2.0;
      
      dConf.vX = dConf.wX + (dConf.wW * 0.30); /* x + 30% */
      dConf.vY = dConf.wY + dConf.borderW;
      dConf.vW = (dConf.wW - dConf.vX) - dConf.borderW;
      dConf.vH = (dConf.wH - (dConf.wH * 0.20)); /* y + 20% */

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

      glMatrixMode(GL_MODELVIEW);
      
      glutPostRedisplay();

      /* viewer */
      glutSetWindow(dConf.viewerw);
      glutPositionWindow((int)dConf.vX, (int)dConf.vY);
      glutReshapeWindow((int)dConf.vW, (int)dConf.vH);

      glViewport(0, 0, w, h);
      
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluPerspective(VIEWANGLE, dConf.vW / dConf.vH, 
                     0.1, 1000.0);
      glMatrixMode(GL_MODELVIEW);
      glutPostRedisplay();
    }
  else                          /* viewer */
    {
      glutPositionWindow(dConf.vX, dConf.vY);
      glutReshapeWindow(dConf.vW, dConf.vH);

      glViewport(0, 0, w, h);
      
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      dConf.vAspect = (GLfloat)w/(GLfloat)h;
      gluPerspective(VIEWANGLE, dConf.vAspect, 
                     0.1, 1000.0);

      glMatrixMode(GL_MODELVIEW);

      glutPostRedisplay();
    }

  dConf.inited = True;

  return;
}

static int renderNode(void)
{
  scrNode_t *node = getTopNode();
  int rv;

  if (node)
    {
      if (node->display)
        {
          glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

          rv = (*node->display)(&dConf);

          glutSwapBuffers();

          if (dConf.viewerwmapped)
            {
              glutSetWindow(dConf.viewerw);
              glutSwapBuffers();
            }
      
          if (rv == NODE_EXIT)
            return rv;
        }

      if (node->idle)
        rv = (*node->idle)();
      
      if (rv == NODE_EXIT)
        return rv;
    }

  return NODE_OK;
}

static void renderFrame(void)
{				/* assumes context is current*/
  int rv;

  /* don't render anything until we are ready */
  if (!dConf.inited)
    return;

  /* get FPS */
  frame++;
  gtime = glutGet(GLUT_ELAPSED_TIME);
  if (gtime - timebase > 1000) 
    {
      FPS = (frame*1000.0/(gtime-timebase));
      timebase = gtime;
      frame = 0;
    }

  if (getTopNode())
    {
      glutSetWindow(dConf.mainw);
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
  if (Context.recmode == RECMODE_PLAYING)
    c_sleep(framedelay);
  else
    {
      if (FPS > 75.0)               /* a little throttling... */
        c_sleep(0.01);
    }

  return;

}


void uiPrintFixed(GLfloat x, GLfloat y, GLfloat w, GLfloat h, char *str)
{                               /* this works for dConf.mainw only */
  glfRender(x, y, w, h, fontFixedTxf, str, NoColor, TRUE, TRUE);

  return;
}


void drawTorp(GLfloat x, GLfloat y, char torpchar, int torpcolor, 
              int scale)
{
  const GLfloat z = -5.0;
  GLfloat size = 3.0;
  GLfloat sizeh;

  if (scale == MAP_FAC)
    size = size / 2.0;

  sizeh = (size / 2.0);

  glPushMatrix();
  glLoadIdentity();

  glTranslatef(x , y , TRANZ);
  glRotatef(rnduni( 0.0, 360.0 ), 0.0, 0.0, z);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);

  glEnable(GL_TEXTURE_2D); 
  glBindTexture(GL_TEXTURE_2D, textures[TEX_TORP]); 

  glBegin(GL_POLYGON);		

  uiPutColor(torpcolor |CQC_A_BOLD);

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
  glDisable(GL_BLEND);

  glPopMatrix();

  return;
}

void
drawShip(GLfloat x, GLfloat y, GLfloat angle, char ch, int i, int color,
	 GLfloat scale)
{
  char buf[16];
  GLfloat alpha = 1.0;
  const GLfloat z = 1.0;
  GLfloat size = 7.0;
  GLfloat sizeh;
  GLint texsel;
  const GLfloat viewrange = VIEWANGLE * 2; 
  const GLfloat phaseradius = (PHASER_DIST / ((21.0 * SCALE_FAC) / viewrange));


  if (scale == MAP_FAC)
    size = size / 2.0;

  sizeh = size / 2.0;

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);

  /* we draw this before the ship */
  if (((scale == SCALE_FAC) && Ships[i].pfuse > 0)) /* phaser action */
    {
      glPushMatrix();
      glLoadIdentity();
      /* translate to correct position, */
      glTranslatef(x , y , TRANZ);
      /* THEN rotate ;-) */
      glRotatef(Ships[i].lastphase, 0.0, 0.0, z);

      glLineWidth(2.0);

      glBegin(GL_LINES);
      glColor4f(1.0, 1.0, 1.0, .9);
      glVertex3f(0.0, 0.0, z + 1.0);
      glColor4f(1.0, 0.0, 0.0, 0.25);
      glVertex3f(phaseradius, 0.0, z + 1.0);
      glEnd();
      
      glPopMatrix();
    }

  sprintf(buf, "%c%d", ch, i);

  /* set a lower alpha if we are cloaked. */
  if (ch == '~')
    alpha = 0.4;		/* semi-transparent */

  GLError();
  
#ifdef DEBUG
  clog("DRAWSHIP(%s) x = %.1f, y = %.1f, ang = %.1f\n", buf, x, y, angle);
#endif

  glPushMatrix();
  glLoadIdentity();

  glEnable(GL_TEXTURE_2D); 

  texsel = TEX_SHIP_FSC;	/* default - fed scout */
  switch(Ships[i].team)
    {
    case TEAM_FEDERATION:
      switch(Ships[i].shiptype)
	{
	case ST_SCOUT:
	  texsel = TEX_SHIP_FSC;
	  break;
	case ST_DESTROYER:
	  texsel = TEX_SHIP_FDE;
	  break;
	case ST_CRUISER:
	  texsel = TEX_SHIP_FCR;
	  break;
	}
      break;
    case TEAM_KLINGON:
      switch(Ships[i].shiptype)
	{
	case ST_SCOUT:
	  texsel = TEX_SHIP_KSC;
	  break;
	case ST_DESTROYER:
	  texsel = TEX_SHIP_KDE;
	  break;
	case ST_CRUISER:
	  texsel = TEX_SHIP_KCR;
	  break;
	}
      break;
    case TEAM_ROMULAN:
      switch(Ships[i].shiptype)
	{
	case ST_SCOUT:
	  texsel = TEX_SHIP_RSC;
	  break;
	case ST_DESTROYER:
	  texsel = TEX_SHIP_RDE;
	  break;
	case ST_CRUISER:
	  texsel = TEX_SHIP_RCR;
	  break;
	}
      break;
    case TEAM_ORION:
      switch(Ships[i].shiptype)
	{
	case ST_SCOUT:
	  texsel = TEX_SHIP_OSC;
	  break;
	case ST_DESTROYER:
	  texsel = TEX_SHIP_ODE;
	  break;
	case ST_CRUISER:
	  texsel = TEX_SHIP_OCR;
	  break;
	}
      break;
    }
  

  glBindTexture(GL_TEXTURE_2D, textures[texsel]);
  
  /* translate to correct position, */
  glTranslatef(x , y , TRANZ);
  /* THEN rotate ;-) */
  glRotatef(angle, 0.0, 0.0, z);

  glColor4f(1.0, 1.0, 1.0, alpha);	

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
  
  /* highlight enemy ships... */
  if (UserConf.EnemyShipBox)
    if (color == RedLevelColor || color == RedColor)
      drawLineBox(-sizeh, -sizeh, size, size, RedColor, 1.0);

  glPopMatrix();

  glDisable(GL_BLEND);

  drawString(x, ((scale == SCALE_FAC) ? y - 5.0 : y - 4.0), z, buf, 
             vFontDL, color);

  return;
}

void
drawDoomsday(GLfloat x, GLfloat y, GLfloat angle, GLfloat scale)
{
  GLfloat alpha = 1.0;
  const GLfloat z = 1.0;
  GLfloat size = 30.0;
  GLfloat sizeh;

  if (scale == MAP_FAC)
    size = size / 3.0;

  sizeh = size / 2.0;

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);

  GLError();
    
#ifdef DEBUG
  clog("DRAWDOOMSDAY(%s) x = %.1f, y = %.1f, ang = %.1f\n", buf, x, y, angle);
#endif

  glPushMatrix();
  glLoadIdentity();

  glEnable(GL_TEXTURE_2D); 

  glBindTexture(GL_TEXTURE_2D, textures[TEX_DOOMSDAY]);
  
  /* translate to correct position, */
  glTranslatef(x , y , TRANZ);
  /* THEN rotate ;-) */
  glRotatef(angle, 0.0, 0.0, z);

  glColor4f(1.0, 1.0, 1.0, alpha);	

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

void drawViewerBG()
{
  const GLfloat z = -5.0;
  const GLfloat size = VIEWANGLE / 10.0;
  const GLfloat sizeh = size / 2.0;

  glPushMatrix();
  glLoadIdentity();

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, textures[TEX_VBG]);
  
  glColor3f(1.0, 1.0, 1.0);	

  glBegin(GL_POLYGON);

  glTexCoord2f(0.75f, 0.25f);
  glVertex3f(-sizeh, -sizeh, z); /* ll */

  glTexCoord2f(0.75f, 0.75f);
  glVertex3f(sizeh, -sizeh, z); /* lr */

  glTexCoord2f(0.25f, 0.75f);
  glVertex3f(sizeh, sizeh, z); /* ur */

  glTexCoord2f(0.25f, 0.25f);
  glVertex3f(-sizeh, sizeh, z); /* ul */

#if 0
  glTexCoord2f(1.0f, 0.0f);
  glVertex3f(-sizeh, -sizeh, z); /* ll */

  glTexCoord2f(1.0f, 1.0f);
  glVertex3f(sizeh, -sizeh, z); /* lr */

  glTexCoord2f(0.0f, 1.0f);
  glVertex3f(sizeh, sizeh, z); /* ur */

  glTexCoord2f(0.0f, 0.0f);
  glVertex3f(-sizeh, sizeh, z); /* ul */
#endif
  glEnd();

  glDisable(GL_TEXTURE_2D); 

  glPopMatrix();

  return;
}



/* glut's a little.. lacking when it comes to keyboards... */
static void procInput(int key, int x, int y)
{
  int rv;
  scrNode_t *node = getTopNode();

#if 0
  clog("GL: procInput: key = %d, x = %d y = %d",
       key, x, y);
#endif

  if (node)
    {
      if (node->input)
        rv = (*node->input)(key);

      if (rv == NODE_EXIT)
        {
          conqend();
          exit(1);
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

static void charInput(unsigned char key, int x, int y)
{
  procInput((int)key, x, y);
  return;
}
  

static int LoadTGA(char *filename, textureImage *texture)
{    
  GLubyte TGAheader[12]={0,0,2,0,0,0,0,0,0,0,0,0}; /* Uncompressed TGA Header */
  GLubyte TGAcompare[12]; /* Used To Compare TGA Header */
  GLubyte header[6]; /* First 6 Useful Bytes From The Header */
  GLuint bytesPerPixel; 
  GLuint imageSize; 
  GLuint temp;   
  FILE *file = fopen(filename, "rb"); 
  int i;

  if (!file)
    {
      clog("Error reading file: %s\n", strerror(errno));
      return FALSE;
    }

  if (fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) || 
      memcmp(TGAheader,TGAcompare,sizeof(TGAheader)) !=0 || 
      fread(header,1,sizeof(header),file) != sizeof(header))
    {
      clog("Invalid file: %s\n", filename);
      fclose(file);
      return FALSE;
    }
  
  texture->width = header[1] * 256 + header[0]; 
  texture->height = header[3] * 256 + header[2];
  
  if (texture->width <= 0 || texture->height <= 0 ||
     (header[4] !=24 && header[4] != 32))
    {
      clog("Invalid file format: %s\n", filename);
      fclose(file);
      return FALSE;
    }
  
  texture->bpp	= header[4];
  bytesPerPixel	= texture->bpp / 8;
  imageSize = texture->width * texture->height * bytesPerPixel;
  texture->imageData = (GLubyte *)malloc(imageSize);
  
  if (!texture->imageData)
    {
      clog("Texture alloc failed for %s\n", filename);
      fclose(file);
      return FALSE;
    }

  if (fread(texture->imageData, 1, imageSize, file) != imageSize)
    {
      if (texture->imageData)
        free(texture->imageData);
      
      clog("Image data read failed for %s\n", filename);
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
  
  fclose (file);
  
  return TRUE;	
}

static int LoadGLTextures()   
{
    Bool status;
    int rv;
    textureImage *texti;
    int i;
    char filenm[MID_BUFFER_SIZE];
    int type;
    int components;         /* for RGBA */

    status = FALSE;

    for (i=0; i < NUM_TEX; i++)
      {
	texti = malloc(sizeof(textureImage));
        if (!texti)
          {
            clog("LoadGLTextures(): memory allocation failed for %d bytes\n");
            return FALSE;
          }
	snprintf(filenm, MID_BUFFER_SIZE - 1, "%s/%s", 
		CONQSHARE, TexInfo[i].filename);
	if ((rv = LoadTGA(filenm, texti)) == TRUE)
	  {
	    status = TRUE;
	    glGenTextures(1, &textures[i]);   /* create the texture */
	    glBindTexture(GL_TEXTURE_2D, textures[i]);
	    /* actually generate the texture */
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

	    glTexImage2D(GL_TEXTURE_2D, 0, components, 
                         texti->width, texti->height, 0,
			 type, GL_UNSIGNED_BYTE, texti->imageData);
	  }
	
	GLError();

	/* free the ram we used in our texture generation process */
	if (texti)
	  {
	    if (texti->imageData && rv == TRUE)
	      free(texti->imageData);
	    free(texti);
	  }    
      }

    return status;
}

