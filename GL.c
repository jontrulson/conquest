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
#include "color.h"
#include "conqcom.h"
#include "ibuf.h"
#include "display.h"
#include <stdlib.h>
#include <stdio.h>
#include <Xm/Xm.h>    
#include <Xm/Form.h> 
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/Scale.h>
#include <Xm/Separator.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>  /* For XA_RGB_DEFAULT_MAP. */
#include <X11/Xmu/StdCmap.h>  /* For XmuLookupStandardColormap. */
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/GLwMDrawA.h>  /* Motif OpenGL drawing area. */

#define NOEXTERN
#include "conquest.h"

#define VIEWRANGE 100.0

static int snglBuf[] = {GLX_RGBA, GLX_DEPTH_SIZE, 12,
			GLX_RED_SIZE, 1, None};
static int dblBuf[] = {GLX_RGBA, GLX_DEPTH_SIZE, 12,
		       GLX_DOUBLEBUFFER, GLX_RED_SIZE, 1, 
		       None};
static String fallbackResources[] = {
  "*glxarea*width: 450", 
  "*glxarea*height: 450",
  "*frame*x: 10", 
  "*frame*y: 10",
  "*frame*topOffset: 10", 
  "*frame*rightOffset: 5", 
  "*frame*leftOffset: 5",
  "*bframe*topOffset: 5",
  "*ilabel*rightOffset: 5", 
  "*ilabel*leftOffset: 5",
  "*frame*shadowType: SHADOW_IN", 
  NULL
};

#define ConquestGLNwhitePixel           "whitePixel"
#define ConquestGLCwhitePixel           "WhitePixel"
#define ConquestGLNblackPixel           "blackPixel"
#define ConquestGLCblackPixel           "BlackPixel"
#define ConquestGLNredPixel             "redPixel"
#define ConquestGLCredPixel             "RedPixel"
#define ConquestGLNyellowPixel          "yellowPixel"
#define ConquestGLCyellowPixel          "yellowPixel"
#define ConquestGLNgreenPixel           "greenPixel"
#define ConquestGLCgreenPixel           "GreenPixel"
#define ConquestGLNbluePixel            "bluePixel"
#define ConquestGLCbluePixel            "BluePixel"
#define ConquestGLNcyanPixel            "cyanPixel"
#define ConquestGLCcyanPixel            "CyanPixel"
#define ConquestGLNmagentaPixel         "magentaPixel"
#define ConquestGLCmagentaPixel         "MagentaPixel"

#define ConquestGLNdefaultPixel         "defaultPixel"
#define ConquestGLCdefaultPixel         "DefaultPixel"
#define ConquestGLNinfoPixel            "infoPixel"
#define ConquestGLCinfoPixel            "InfoPixel"
#define ConquestGLNspecialPixel         "specialPixel"
#define ConquestGLCspecialPixel         "SpecialPixel"

#define ConquestGLNstatsepPixel         "statsepPixel"
#define ConquestGLCstatsepPixel         "StatsepPixel"


static XtResource resources[] = {
  {
    ConquestGLNblackPixel,
    ConquestGLCblackPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, blackPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNwhitePixel,
    ConquestGLCwhitePixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, whitePixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNredPixel,
    ConquestGLCredPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, redPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNyellowPixel,
    ConquestGLCyellowPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, yellowPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNgreenPixel,
    ConquestGLCgreenPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, greenPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNbluePixel,
    ConquestGLCbluePixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, bluePixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNcyanPixel,
    ConquestGLCcyanPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, cyanPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNmagentaPixel,
    ConquestGLCmagentaPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, magentaPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNdefaultPixel,
    ConquestGLCdefaultPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, defaultPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNinfoPixel,
    ConquestGLCinfoPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, infoPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNspecialPixel,
    ConquestGLCspecialPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, specialPixel),
    XtRImmediate,
    (XtPointer) 0,
  },
  {
    ConquestGLNstatsepPixel,
    ConquestGLCstatsepPixel,
    XtRPixel,
    sizeof(Pixel),
    XtOffsetOf(ConquestDataRec, statsepPixel),
    XtRImmediate,
    (XtPointer) 0,
  }
};


const int delay = 40;

/*Display *dpy;
  XtAppContext app;
  XtIntervalId timerId = 0;*/

XmStringTag inforend = "inforend";
XmStringTag datarend = "datarend";

Widget form, sform, bform, frame, glxarea, framelabel;
Widget bframe, sframe;

Widget shscale, shscroller, shtitle;
Widget damscale, damscroller, damtitle;
Widget etempscale, etempscroller, etemptitle;
Widget wtempscale, wtempscroller, wtemptitle;

Widget promptlabel1, promptlabel2;/*JET*/
Widget msglabel;
Widget sep1, sep2, sep3, sep4;

Widget fuelscale, fuelscroller, fueltitle;

Widget killlabel, killlabelval;
Widget warplabel, warplabelval;
Widget headlabel, headlabelval;

Widget ilabel;

Widget alloclabel, alloclabelval;
Widget towlabel;
Widget armieslabel, armieslabelval;
Widget cloaklabel;

XVisualInfo *visinfo;
GLXContext glxcontext;

Colormap cmap;

Bool doubleBuffer = True, drawing = True;
Bool spindir = True;

XFontStruct *font;
GLuint base;

/* textures... */
typedef struct {
    int width;
    int height;
    unsigned char *data;
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

#define NUM_TEX 17

struct _texinfo TexInfo[NUM_TEX] = { /* need to correlate with defines above */
  { "img/star.bmp",      255 },

  { "img/classm.bmp",    255 },
  { "img/classd.bmp",    255 },

  { "img/explode.bmp",   255 },

  { "img/shipfsc.bmp",   255 },
  { "img/shipfde.bmp",   255 },
  { "img/shipfcr.bmp",   255 },

  { "img/shipksc.bmp",   255 },
  { "img/shipkde.bmp",   255 },
  { "img/shipkcr.bmp",   255 },

  { "img/shiprsc.bmp",   255 },
  { "img/shiprde.bmp",   255 },
  { "img/shiprcr.bmp",   255 },

  { "img/shiposc.bmp",   255 },
  { "img/shipode.bmp",   255 },
  { "img/shipocr.bmp",   255 },

  { "img/doomsday.bmp",  255 }
};

GLuint  textures[NUM_TEX];       /* texture storage */

void drawGL(void);
void mapStateChanged(Widget w, XtPointer clientData, XEvent * event, 
		     Boolean * cont);
Colormap getShareableColormap(XVisualInfo * vi);
void graphicsInit(Widget w, XtPointer clientData, XtPointer call);
void expose(Widget w, XtPointer clientData, XtPointer call);
void resize(Widget w, XtPointer clientData, XtPointer call);
void input(Widget w, XtPointer clientData, XtPointer callData);

#define FONT_SMALL_BASE 2000

void _GLError(char *funcname, int line)
{
  int i;

  while ((i = glGetError()) != GL_NO_ERROR)
    clog("GL ERROR: %s@%d: %s\n",
	 funcname, line, gluErrorString(i));

  return;
}
#define GLError() _GLError(__FUNCTION__, __LINE__)

int InitFonts(void)
{
  base = glGenLists(96);

  if ((font = XLoadQueryFont(ConqData.dpy, "fixed")) == NULL)
    {
      clog("%s: XLoadQueryFont(fixed) failed\n", __FUNCTION__);
      return FALSE;
    }

  glXMakeCurrent(ConqData.dpy, XtWindow(glxarea), glxcontext);
  glXUseXFont(font->fid, 32, 96, base /*FONT_SMALL_BASE + 32*/); /* skip non-printables */

  //  XFreeFont(ConqData.dpy, font);

  return TRUE;
}

void renderFrame(XtPointer closure, XtIntervalId *intervalId)
{
  extern void display(int, int);
  /* NO MOPRE  extern void clntASTService(int); */
  /* call display with the current ship */

  //    clog("renderFrame: rendering = %d\n", ConqData.rendering);
    //display(Context.snum, FALSE);
#warning "need a new ast for the GL version"
  /* JET clntASTService(0); */
  //  clog("renderFrame: AFTER astservice()\n");
  
  //  if (ConqData.rendering)
  //  return False;
  //else
  //  return True;
  if (ConqData.rendering)
    ConqData.timerId = XtAppAddTimeOut(ConqData.app, delay, renderFrame, NULL);

  return;
}

void procEvent()
{
  XEvent event;

  //  clog("procEvent ONE SERIES\n");

  if (ConqData.app)
    {
      while(XtAppPending(ConqData.app))
	{
	  clog("PROCEVENT: event pending, calling NextEvent\n");
	  XtAppNextEvent(ConqData.app, &event);
	  clog("PROCEVENT: got event, dispatching.\n");
	  XtDispatchEvent(&event);
	  clog("PROCEVENT: DISPATCHED:\n");
	}
    }

  return;
}

void procEvents()
{
  XEvent event;

  //  clog("procEvents\n");

  if (ConqData.app)
    {
#if 0
      while(XPending(ConqData.dpy))
	{
	  XNextEvent(ConqData.dpy, &event);
	  XtDispatchEvent(&event);
	}
#endif
      while(ConqData.rendering)
	{
	  XtAppNextEvent(ConqData.app, &event);
	  XtDispatchEvent(&event);
	}
    }

  return;
}
	  

/* start rendering */
void startRendering(void)
{
  clog("START RENDERING\n");
  ConqData.rendering = True;
  ConqData.timerId = XtAppAddTimeOut(ConqData.app, delay, renderFrame, NULL);
  //  ConqData.workId = XtAppAddWorkProc(ConqData.app, renderFrame, NULL);
  //  while (ConqData.rendering)
  // {
  //   procEvents();
  //}

  return;
}

void stopRendering(void)
{
  clog("STOP RENDERING\n");
  if (ConqData.timerId)
    XtRemoveTimeOut(ConqData.timerId);

    //    XtRemoveWorkProc(ConqData.workId);
  ConqData.rendering = False;
  ConqData.timerId = 0;
  return;
}


void drawString(GLfloat x, GLfloat y, char *str, int color)
{
  
  if (!str)
    return;
#ifdef DEBUG
  clog("%s: x = %.1f, y = %.1f, str = '%s'\n", __FUNCTION__,
       x, y, str);
#endif

  glPushMatrix();
  glLoadIdentity();

  glTranslatef(0.0, 0.0, -100.0 /*-75.0*/);

  if (color == 0 || color == A_BOLD)
    glColor3f(1.0, 1.0, 1.0);
  else if (color == RedLevelColor)
    glColor3f(1.0, 0.0, 0.0);
  else if (color == GreenLevelColor)
    glColor3f(0.0, 1.0, 0.0);
  else if (color == YellowLevelColor)
    glColor3f(1.0, 1.0, 0.0);
  else if (color == InfoColor)
    glColor3f(0.0, 1.0, 1.0);
  else
    glColor3f(0.5, 0.5, 0.5);

  GLError();

  glRasterPos2f(x, y);
  glPushAttrib(GL_LIST_BIT);

  /*glListBase(FONT_SMALL_BASE);*/
  glListBase(base - 32);
  glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);

  glPopAttrib();

  glPopMatrix();

  GLError();

  return;
}

void GLSetTimer(void)
{
  return;			/* not doing much anyway... */

#if 0
  if (!ConqData.timerId)
    ConqData.timerId = XtAppAddTimeOut(ConqData.app, delay, spin, NULL);
#endif
  return;
}

void GLStopTimer(void)
{

  return;			/* not doing much anyway... */

  if (ConqData.timerId)
    XtRemoveTimeOut(ConqData.timerId);
  ConqData.timerId = 0;
}

void drawBox(GLfloat x, GLfloat y, GLfloat size)
{				/* draw a square centered on x,y
				   USE BETWEEN glBegin/End pair! */
  const GLfloat z = 0.0;
  GLfloat rx, ry;

#ifdef DEBUG
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
  glTranslatef(x , y , -100.0);
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

/*  puthing - put an object on the display */
/*  SYNOPSIS */
/*    int what, lin, col */
/*    puthing( what, lin, col ) */
void glputhing( int what, GLfloat x, GLfloat y, int id)
{
  GLfloat size = 10.0;
  int texon = FALSE;
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

  glPushMatrix();
  glLoadIdentity();


  if (what == PLANET_SUN)
    {				/* lets enable texture mapping for suns */
      glEnable(GL_TEXTURE_2D); 
      glBindTexture(GL_TEXTURE_2D, textures[TEX_SUN]);
      texon = TRUE;
    }
  else if (what == PLANET_CLASSM)
    {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, textures[TEX_CLASSM]);
      texon = TRUE;
    }
  else if (what == PLANET_DEAD || what == PLANET_MOON)
    {
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, textures[TEX_CLASSD]);
      texon = TRUE;
    }

  glTranslatef(0.0, 0.0, -100.0);

  GLError();

  glBegin(GL_POLYGON);

  switch ( what )
    {
    case PLANET_SUN:
      /* choose a good color for the main suns */
      switch(id)
	{
	case PNUM_SOL:		/* yellow */
	case PNUM_KEJELA:
	  glColor3f(1.0, 1.0, 0.0);	
	  break;
	case PNUM_SIRIUS:	/* red */
	case PNUM_SYRINX:
	  glColor3f(1.0, 0.0, 0.0);
	  break;
	case PNUM_BETELGEUSE:	/* blue */
	case PNUM_MURISAK:
	  glColor3f(0.0, 0.0, 1.0);
	  break;
	  
	default:		/* red */
	  glColor3f(1.0, 0.0, 0.0);
	  break;
	}
	  
      size = 40.0;		/* 'magnify' the texture */
      break;
    case PLANET_CLASSM:
      glColor3f(0.5, 1.0, 0.5); /* ? */
      size = 10.0;
      break;
    case PLANET_CLASSA:
    case PLANET_CLASSO:
    case PLANET_CLASSZ:
    case PLANET_DEAD:
      glColor3f(0.8, 0.5, 0.5);	/* ? */
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
    case THING_DEATHSTAR:
      break;
    default:
      break;
    }
  
  if (texon)
    {				
      drawTexBox(x, y, size);
    }
  else
    drawBox(x, y, size);
  
  glEnd();

  if (texon)
    {				
      glDisable(GL_TEXTURE_2D); 
      glDisable(GL_BLEND); 
    }

  glPopMatrix();

  return;
  
}

void GLcvtcoords(real cenx, real ceny, real x, real y, real scale,
		 GLfloat *rx, GLfloat *ry )
{
  const GLfloat srscanradius = (20.0 * SCALE_FAC);
  const GLfloat lrscanradius = (20.0 * MAP_FAC);
  const GLfloat viewrange = VIEWRANGE; /* need to figure out how to compute this */
  GLfloat rscale;

  if (scale == SCALE_FAC)
    rscale = srscanradius / viewrange;
  else				/* MAP_FAC */
    rscale = lrscanradius / viewrange;

  *rx = (((viewrange / 2) - (x-cenx)) / rscale) * -1;
  *ry = (((viewrange / 2) - (y-ceny)) / rscale) * -1;

#ifdef DEBUG
  clog("GLCVTCOORDS: cx = %.2f, cy = %.2f, \n\tx = %.2f, y = %.2f, glx = %.2f,"
       " gly = %.2f \n",
       cenx, ceny, x, y, *rx, *ry);
#endif

 return; 
}


Pixel getXColor(unsigned int color)
{
  /*  if (color & A_BOLD)
      color &= ~A_BOLD;*/

  if (color == 0)
    {
      //      clog("WHITE");
      return ConqData.whitePixel;
    }
  else if (color == RedLevelColor)
    {
      //clog("RED");
      return ConqData.redPixel;
    }
  else if (color == GreenLevelColor)
    {
      //clog("GREEN");
      return ConqData.greenPixel;
    }
  else if (color == YellowLevelColor)
    {
      //clog("YELLOW");
      return ConqData.yellowPixel;
    }
  else if (color == InfoColor)
    {
      //clog("CYAN");
      return ConqData.cyanPixel;
    }
  else if (color == LabelColor)
    {
      //clog("BLUE");
      return ConqData.bluePixel;
    }
  else
    {
      //clog("DEFAULT WHITE");
      return ConqData.defaultPixel;
    }
}

void showXtraInfo(int ifang)
{
  static int FirstTime = True;
  static XmString colon, slash, fa, tda;
  static XmString fang, targ, targa, targd;
  static int fireangle = 0, targetangle = 0, targetdist = 0;
  static char targetname[16] = "";
  XmString output;
  char ifa[16], tname[16], tang[16], tdist[32];

  if (FirstTime)
    {
      FirstTime = False;

      colon = XmStringGenerate(":", NULL, XmCHARSET_TEXT, inforend);
      slash = XmStringGenerate("/", NULL, XmCHARSET_TEXT, inforend);
      fa = XmStringGenerate("FA:", NULL, XmCHARSET_TEXT, inforend);
      tda = XmStringGenerate(" TA/D:", NULL, XmCHARSET_TEXT, inforend);
    }

  if (ifang != fireangle || strcmp(Context.lasttarg, targetname) ||
      Context.lasttang != targetangle ||
      Context.lasttdist != targetdist)
    {
      sprintf(ifa, "%3d", ifang);
      sprintf(tname, "%3s", Context.lasttarg);
      sprintf(tang, "%3d", Context.lasttang);
      sprintf(tdist, "%d", Context.lasttdist);

      fang = XmStringGenerate(ifa, NULL, XmCHARSET_TEXT, datarend);
      targ = XmStringGenerate(tname, NULL, XmCHARSET_TEXT, datarend);
      targa = XmStringGenerate(tang, NULL, XmCHARSET_TEXT, datarend);
      targd = XmStringGenerate(tdist, NULL, XmCHARSET_TEXT, datarend);
      
      output = XmStringCopy(fa);
      output = XmStringConcat(output, fang);
      output = XmStringConcat(output, tda);
      output = XmStringConcat(output, targ);
      output = XmStringConcat(output, colon);
      output = XmStringConcat(output, targa);
      output = XmStringConcat(output, slash);
      output = XmStringConcat(output, targd);
      
      XtVaSetValues(ilabel, 
		    XmNlabelString, output,
		    NULL);

      XmStringFree(fang);
      XmStringFree(targ);
      XmStringFree(targa);
      XmStringFree(targd);
      XmStringFree(output);

      fireangle = ifang;
      strcpy(targetname, Context.lasttarg);
      targetangle = Context.lasttang;
      targetdist = Context.lasttdist;

    }


  return;

}
  


void showHeading(char *heading)
{
  XmString s;

  XtVaSetValues(headlabelval, 
		XmNlabelString, s = XmStringCreateLocalized(heading),
		NULL);
  XmStringFree(s);

  return;
}

void showWarp(char *warp)
{
  XmString s;

  XtVaSetValues(warplabelval, 
		XmNlabelString, s = XmStringCreateLocalized(warp),
		NULL);
  XmStringFree(s);
  
  return;
}

void showKills(char *kills)
{
  XmString s;

  XtVaSetValues(killlabelval, 
		XmNlabelString, s = XmStringCreateLocalized(kills),
		NULL);
  XmStringFree(s);
  
  return;
}

void showFuel(int fuel, int color)
{
  XtVaSetValues(fuelscale,
		XmNvalue, fuel,
		XmNforeground, getXColor(color),
		XmNbottomShadowColor, getXColor(color),
		XmNtopShadowColor, getXColor(color),
		NULL);

  XtVaSetValues(fuelscroller,
		XmNforeground, getXColor(color),
		NULL);
  
  return;
}

void showAlertLabel(char *buf, int color)
{
  XmString s;
  static char tempstr[BUFFER_SIZE] = "";

  if (strlen(buf))
    {
      if (strcmp(tempstr, buf))
	{
	  XtMapWidget(framelabel);
	  XtVaSetValues(framelabel, 
			XmNforeground, getXColor(color),
			XmNlabelString, s = XmStringCreateLocalized(buf),
			NULL);
	  XmStringFree(s);
	  strcpy(tempstr, buf);
	}
    }
  else
    {
      XtUnmapWidget(framelabel);
      tempstr[0] = EOS;
    }

  return;
}

void showShields(int shields, unsigned int color)
{
  static int sshields = -2;
  static int scolor = -1;
  XmString s;

  //  clog("showShields(int shields, int color) shields = %d, color = %d\n",
  //   shields, color);

  if (shields != sshields || color != scolor)
    {
      if (shields == -1)	/* down */
	{
	  XtVaSetValues(shscale,
			XmNvalue, 0,
			XmNforeground, getXColor(color),
			XmNbottomShadowColor, getXColor(color),
			XmNtopShadowColor, getXColor(color),
			NULL);

	  XtVaSetValues(shtitle,
			XmNforeground, getXColor(color),
			XmNlabelString, s = XmStringCreateLocalized("Shields DOWN"),
			NULL);
	  
	  XtVaSetValues(shscroller,
			XmNforeground, getXColor(color),
			NULL);
	}
      else
	{			/* up */
	  XtVaSetValues(shscale,
			XmNvalue, shields,
			XmNforeground, getXColor(color),
			XmNbottomShadowColor, getXColor(color),
			XmNtopShadowColor, getXColor(color),
			NULL);
	  

	  XtVaSetValues(shscroller,
			XmNforeground, getXColor(color),
			NULL);
	  XtVaSetValues(shtitle,
			XmNforeground, getXColor(color),
			XmNlabelString, s = XmStringCreateLocalized("Shields UP"),
			NULL);
	}
      
      XmStringFree(s);
      sshields = shields;
      scolor = color;
    }

  return;
}

void showAlloc(char *alloc)
{
  static char salloc[8] = "";
  XmString s;

  if (strncmp(salloc, alloc, 8))
    {
      strncpy(salloc, alloc, 8 - 1);
      XtVaSetValues(alloclabelval, 
		    XmNlabelString, s = XmStringCreateLocalized(alloc),
		    NULL);
      XmStringFree(s);
    }
      
  return;
}

void showTemp(int etemp, int ecolor, int wtemp, int wcolor, int efuse, int wfuse)
{
  XmString s;
  static Bool seover = False, swover = False;
  Bool eover = False, wover = False;

  if (etemp > 100)
    etemp = 100;
  if (wtemp > 100)
    wtemp = 100;

  if (efuse > 0)
    eover = True;

  if (wfuse > 0)
    wover = True;
  
  /*etemp */
  XtVaSetValues(etempscale,
		XmNvalue, etemp,
		XmNforeground, getXColor(ecolor),
		XmNbottomShadowColor, getXColor(ecolor),
		XmNtopShadowColor, getXColor(ecolor),
		NULL);
  XtVaSetValues(etempscroller,
		XmNforeground, getXColor(ecolor),
		NULL);

  if (eover != seover)
    {
      if (eover)
	{
	  XtVaSetValues(etemptitle,
			XmNlabelString, s = XmStringCreateLocalized("Engines Overloaded"),
			XmNforeground, ConqData.redPixel,
			NULL);
	}
      else
	{
	  XtVaSetValues(etemptitle,
			XmNlabelString, s = XmStringCreateLocalized("Engine Temperature"),
			XmNforeground, ConqData.whitePixel,
			NULL);
	}
      XmStringFree(s);

      seover = eover;
    }

  XtVaSetValues(etempscroller,
		XmNforeground, getXColor(ecolor),
		NULL);

  /*wtemp */
  XtVaSetValues(wtempscale,
		XmNvalue, wtemp,
		XmNforeground, getXColor(wcolor),
		XmNbottomShadowColor, getXColor(wcolor),
		XmNtopShadowColor, getXColor(wcolor),
		NULL);

  XtVaSetValues(wtempscroller,
		XmNforeground, getXColor(wcolor),
		NULL);

  if (swover != wover)
    {
      if (wover)
	{
	  XtVaSetValues(wtemptitle,
			XmNlabelString, s = XmStringCreateLocalized("Weapons Overloaded"),
			XmNforeground, ConqData.redPixel,
			NULL);
	}
      else
	{
	  XtVaSetValues(wtemptitle,
			XmNlabelString, s = XmStringCreateLocalized("Weapon Temperature"),
			XmNforeground, ConqData.whitePixel,
			NULL);
	}
      XmStringFree(s);

      swover = wover;
    }

  return;
}

void showDamage(int dam, int color)
{
  XmString s;

  if (dam > 100)
    dam = 100;

  XtVaSetValues(damscale,
		XmNvalue, dam,
		XmNforeground, getXColor(color),
		XmNbottomShadowColor, getXColor(color),
		XmNtopShadowColor, getXColor(color),
		NULL);

  XtVaSetValues(damscroller,
		XmNforeground, getXColor(color),
		NULL);

  return;
}

void showDamageLabel(char *buf, int color)
{
  XmString s;

  XtVaSetValues(damtitle,
		XmNforeground, getXColor(color),
		XmNlabelString, s = XmStringCreateLocalized(buf),
		NULL);
  XmStringFree(s);

  return;
}

void showArmies(char *labelbuf, char *buf)
{				/* this also displays robot actions... */
  XmString s;

  if (labelbuf)
    {
      XtVaSetValues(armieslabel, 
		    XmNlabelString, s = XmStringCreateLocalized(labelbuf),
		    XmNforeground, ConqData.whitePixel,
		    NULL);
      XmStringFree(s);
      
      XtVaSetValues(armieslabelval, 
		    XmNlabelString, s = XmStringCreateLocalized(buf),
		    XmNforeground, ConqData.cyanPixel,
		    NULL);
      XmStringFree(s);
    }
  else
    {
      XtVaSetValues(armieslabel, 
		    XmNforeground, ConqData.blackPixel,
		    NULL);
      
      XtVaSetValues(armieslabelval, 
		    XmNforeground, ConqData.blackPixel,
		    NULL);
    }
  
  return;
}  

void showTow(char *buf)
{
  XmString s;

  if (buf)
    {
      XtVaSetValues(towlabel, 
		    XmNlabelString, s = XmStringCreateLocalized(buf),
		    XmNbackground, ConqData.cyanPixel,
		    NULL);
      
      XmStringFree(s);
    }
  else
    {
      XtVaSetValues(towlabel, 
		    XmNbackground, ConqData.blackPixel,
		    NULL);
    }

  return;
}

void showCloakDestruct(char *buf)
{
  XmString s;

  if (buf)
    {
      XtVaSetValues(cloaklabel, 
		    XmNlabelString, s = XmStringCreateLocalized(buf),
		    XmNbackground, ConqData.redPixel,
		    NULL);
      
      XmStringFree(s);
    }
  else
    {
      XtVaSetValues(cloaklabel, 
		    XmNbackground, ConqData.blackPixel,
		    NULL);
    }

  return;
}
void setAlertBorder(int color)
{
  XtVaSetValues(frame, 
		XmNbottomShadowColor, getXColor(color),
		XmNtopShadowColor, getXColor(color),
		NULL);

  return;
}

void showMessage(int line, char *buf)
{
  XmString s;
  Widget w;
  Pixel color;

  switch(line)
    {
    case MSG_LIN1:
      w = promptlabel1;
      color = ConqData.whitePixel;
      showMessage(MSG_LIN2, NULL); /* clear second line, NO RECURSION ;-) */
      break;
    case MSG_LIN2:
      w = promptlabel2;
      color = ConqData.whitePixel;
      break;
    default:
      w = msglabel;
      color = ConqData.cyanPixel;
      break;
    }

  if (buf)
    XtVaSetValues(w, 
		  XmNlabelString, s = XmStringCreateLocalized(buf),
		  XmNforeground, color,
		  NULL);
  else
    XtVaSetValues(w, 
		  XmNforeground, ConqData.blackPixel,
		  NULL);
    
  return;
}

int InitGL(int argc, char **argv)
{
  XmString s;

  memset(&ConqData, 0, sizeof(ConqData));
  ConqData.toplevel = XtAppInitialize(&ConqData.app, CONQUESTGL_NAME, 
				      NULL, 0, 
				      &argc, argv,
				      fallbackResources, 
				      NULL, 0);

  XtGetApplicationResources(ConqData.toplevel, &ConqData, 
			    resources, XtNumber(resources), 
			    NULL, 0);

  XtAddEventHandler(ConqData.toplevel, StructureNotifyMask,
		    False, mapStateChanged, NULL);

  ConqData.dpy = XtDisplay(ConqData.toplevel);

  visinfo = glXChooseVisual(ConqData.dpy, DefaultScreen(ConqData.dpy), dblBuf);

  if (visinfo == NULL) 
    {
      visinfo = glXChooseVisual(ConqData.dpy, DefaultScreen(ConqData.dpy), 
				snglBuf);
      if (visinfo == NULL)
	XtAppError(ConqData.app, "no good GL visual");
      doubleBuffer = False;
    }

  /* set up colors */
  cmap = getShareableColormap(visinfo);

  /* build the battle screen */

  /* form */
  form = 
    XtVaCreateManagedWidget("form", xmFormWidgetClass,
			    ConqData.toplevel,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.whitePixel,
			    XmNrubberPositioning, True,
			    XmNresizePolicy, XmRESIZE_GROW,
			    NULL);

  /* bframe */
  bframe = 
    XtVaCreateManagedWidget("bframe", xmFrameWidgetClass,
			    form,
			    XmNbottomAttachment, XmATTACH_FORM,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.whitePixel,
			    /*		    XmNtopAttachment, XmATTACH_WIDGET,
					    XmNtopWidget, frame,*/
			    XmNbottomAttachment, XmATTACH_FORM,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_FORM,
			    /*			    XmNallowOverlap, False,*/
			    XmNheight, 100,
			    NULL);

  /* bform */
  bform = 
    XtVaCreateManagedWidget("bform", xmFormWidgetClass,
			    bframe,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.blackPixel,
			    /*JET*/
			    NULL);


  promptlabel1 =
    XtVaCreateManagedWidget("promptlabel1", xmLabelWidgetClass,
			    bform,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.redPixel,
			    XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
			    XmNlabelString, s = XmStringCreateLocalized("It's full of stars"),
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_FORM,
			    XmNtopAttachment, XmATTACH_FORM,
			    XmNalignment, XmALIGNMENT_BEGINNING,
			    NULL);
  XmStringFree(s);

  promptlabel2 =
    XtVaCreateManagedWidget("promptlabel2", xmLabelWidgetClass,
			    bform,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.redPixel,
			    XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, promptlabel1,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_FORM,
			    XmNlabelString, s = XmStringCreateLocalized("It's full of stars, part 2"),
			    XmNalignment, XmALIGNMENT_BEGINNING,
			    NULL);
  XmStringFree(s);

  msglabel =
    XtVaCreateManagedWidget("msglabel", xmLabelWidgetClass,
			    bform,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.cyanPixel,
			    XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, promptlabel2,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_FORM,
			    XmNlabelString, s = XmStringCreateLocalized("O4: You die now!"),
			    XmNalignment, XmALIGNMENT_BEGINNING,
			    NULL);
  XmStringFree(s);

  /* sframe */
  sframe = 
    XtVaCreateManagedWidget("sframe", xmFrameWidgetClass,
			    form,
			    XmNbottomAttachment, XmATTACH_FORM,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.whitePixel,
			    XmNtopOffset, 10,
			    XmNtopAttachment, XmATTACH_FORM,
			    XmNbottomAttachment, XmATTACH_WIDGET,
			    XmNbottomWidget, bframe,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNbottomShadowColor, ConqData.blackPixel,
			    XmNtopShadowColor, ConqData.blackPixel,
			    NULL);

  /* sform */
  sform = 
    XtVaCreateManagedWidget("sform", xmFormWidgetClass,
			    sframe,
			    XmNtraversalOn, False,
			    XmNwidth, 250,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.whitePixel,
			    XmNresizePolicy, XmRESIZE_NONE,
			    NULL);


  /*JET*/

  ilabel =
    XtVaCreateManagedWidget("ilabel", xmLabelWidgetClass,
			    form,
			    /*			    XmNbackground, cyan,
						    XmNforeground, black,*/
			    XmNleftAttachment, XmATTACH_WIDGET,
			    XmNleftWidget, sframe,
			    XmNrightAttachment, XmATTACH_FORM,
			    XmNbottomAttachment, XmATTACH_WIDGET,
			    XmNbottomWidget, bframe,
			    XmNlabelString, s = XmStringCreateLocalized("INFORMATION PERTINENT TO YOUR SURVIVAL"),
			    XmNalignment, XmALIGNMENT_BEGINNING,
			    NULL);
  XmStringFree(s);

  /*JET*/
  /* frame */
  frame = 
    XtVaCreateManagedWidget("frame", xmFrameWidgetClass,
			    form,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.blackPixel,
			    XmNtopAttachment, XmATTACH_FORM,

			    XmNrightAttachment, XmATTACH_FORM,

			    XmNleftAttachment, XmATTACH_WIDGET,
			    XmNleftWidget, sframe,

			    XmNbottomAttachment, XmATTACH_WIDGET,
			    XmNbottomWidget, ilabel,
			    XmNallowOverlap, False,
			    XmNbottomShadowColor, ConqData.blackPixel,
			    XmNtopShadowColor, ConqData.blackPixel,
			    XmNresizable, True,
			    NULL);

  /* framelabel */
  /* initially mapped (for intial size) we then unap after
     toplevel is realized */
  framelabel = 
    XtVaCreateManagedWidget("framelabel", xmLabelWidgetClass,
			    frame,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.redPixel,
			    XmNchildHorizontalAlignment, XmALIGNMENT_CENTER,
			    XmNlabelString, s = XmStringCreateLocalized("FRAMELABEL"),
			    XmNframeChildType, XmFRAME_TITLE_CHILD,
			    XmNmappedWhenManaged, True,
			    NULL);
  XmStringFree(s);

  /* warp */
  warplabel = 
    XtVaCreateManagedWidget("warplabel", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("Warp ="),
			    XmNtopAttachment, XmATTACH_FORM,
			    XmNleftOffset, 10,
			    XmNleftAttachment, XmATTACH_FORM,
			    NULL);
  XmStringFree(s);

  warplabelval = 
    XtVaCreateManagedWidget("warplabelval", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("0.0"),
			    XmNtopAttachment, XmATTACH_FORM,
			    XmNleftOffset, 130,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNalignment, XmALIGNMENT_BEGINNING,
			    NULL);
  XmStringFree(s);

  /* head */
  headlabel = 
    XtVaCreateManagedWidget("headlabel",  xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("Head ="),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, warplabel,
			    XmNleftOffset, 10,
			    XmNleftAttachment, XmATTACH_FORM,
			    NULL);
  XmStringFree(s);

  headlabelval = 
    XtVaCreateManagedWidget("headlabelval", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("334"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, warplabelval,
			    XmNleftOffset, 130,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNalignment, XmALIGNMENT_BEGINNING,
			    NULL);
  XmStringFree(s);

  /* sep 4 */
  sep4 = 
    XtVaCreateManagedWidget("sep4", xmSeparatorWidgetClass,
			    sform,
			    XmNorientation, XmHORIZONTAL,
			    XmNseparatorType, XmSINGLE_LINE,
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, headlabel,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.statsepPixel,
			    NULL);
			    


  /* shields */
  shscale = 
    XtVaCreateManagedWidget("shscale", xmScaleWidgetClass,
			    sform,
			    XmNeditable, False,
			    XmNhighlightOnEnter, False,
			    XmNhighlightThickness, 0,
			    XmNbottomShadowColor, ConqData.greenPixel,
			    XmNtopShadowColor, ConqData.greenPixel,
			    XmNmaximum, 100,
			    XmNminimum, 0,
			    XmNorientation, XmHORIZONTAL,
			    XmNprocessingDirection, XmMAX_ON_RIGHT, 
			    XmNshowArrows, XmNONE,
			    XmNshowValue, XmNEAR_SLIDER,
			    XmNslidingMode, XmTHERMOMETER,
			    XmNsliderVisual, XmFOREGROUND_COLOR,
			    XmNtitleString, s = XmStringCreateLocalized("Shields"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, sep4,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.greenPixel,
			    XmNvalue, 50);
  XmStringFree(s);

  if ((shscroller = XtNameToWidget(shscale, "Scrollbar")) == NULL)
    {
      clog("JET: could not fine Scrollbar\n");
    }
  else
    {
      XtVaSetValues(shscroller,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.greenPixel,
		    NULL);
    }

  if ((shtitle = XtNameToWidget(shscale, "Title")) == NULL)
    {
      clog("JET: could not fine Title\n");
    }
  else
    {
      XtVaSetValues(shtitle,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.greenPixel,
		    NULL);
    }
				    
  /* dam */
  damscale = 
    XtVaCreateManagedWidget("damscale", xmScaleWidgetClass,
			    sform,
			    XmNeditable, False,
			    XmNhighlightOnEnter, False,
			    XmNhighlightThickness, 0,
			    XmNbottomShadowColor, ConqData.greenPixel,
			    XmNtopShadowColor, ConqData.greenPixel,
			    XmNmaximum, 100,
			    XmNminimum, 0,
			    XmNorientation, XmHORIZONTAL,
			    XmNprocessingDirection, XmMAX_ON_RIGHT, 
			    XmNshowArrows, XmNONE,
			    XmNshowValue, XmNEAR_SLIDER,
			    XmNslidingMode, XmTHERMOMETER,
			    XmNsliderVisual, XmFOREGROUND_COLOR,
			    XmNtitleString, s = XmStringCreateLocalized("No Damage"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, shscale,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNtraversalOn, False,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.greenPixel,
			    XmNvalue, 50);
  XmStringFree(s);

  if ((damscroller = XtNameToWidget(damscale, "Scrollbar")) == NULL)
    {
      clog("JET: could not fine Scrollbar\n");
    }
  else
    {
      XtVaSetValues(damscroller,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.greenPixel,
		    NULL);
    }

  if ((damtitle = XtNameToWidget(damscale, "Title")) == NULL)
    {
      clog("JET: could not fine Title\n");
    }
  else
    {
      XtVaSetValues(damtitle,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.greenPixel,
		    NULL);
    }

  /* sep 1 */
  sep1 = 
    XtVaCreateManagedWidget("sep1", xmSeparatorWidgetClass,
			    sform,
			    XmNorientation, XmHORIZONTAL,
			    XmNseparatorType, XmSINGLE_LINE,
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, damscale,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.statsepPixel,
			    NULL);
			    
  /* fuel */
  fuelscale = 
    XtVaCreateManagedWidget("fuelscale", xmScaleWidgetClass,
			    sform,
			    XmNeditable, False,
			    XmNhighlightOnEnter, False,
			    XmNhighlightThickness, 0,
			    XmNbottomShadowColor, ConqData.greenPixel,
			    XmNtopShadowColor, ConqData.greenPixel,
			    XmNmaximum, 999,
			    XmNminimum, 0,
			    XmNorientation, XmHORIZONTAL,
			    XmNprocessingDirection, XmMAX_ON_RIGHT, 
			    XmNshowArrows, XmNONE,
			    XmNshowValue, XmNEAR_SLIDER,
			    XmNslidingMode, XmTHERMOMETER,
			    XmNsliderVisual, XmFOREGROUND_COLOR,
			    XmNtitleString, s = XmStringCreateLocalized("Fuel"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, sep1, /*damscale,*/
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNtraversalOn, False,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.greenPixel,
			    XmNvalue, 999);
  XmStringFree(s);

  if ((fuelscroller = XtNameToWidget(fuelscale, "Scrollbar")) == NULL)
    {
      clog("JET: could not fine Scrollbar\n");
    }
  else
    {
      XtVaSetValues(fuelscroller,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.greenPixel,
		    NULL);
    }

  if ((fueltitle = XtNameToWidget(fuelscale, "Title")) == NULL)
    {
      clog("JET: could not fine Title\n");
    }
  else
    {
      XtVaSetValues(fueltitle,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.greenPixel,
		    NULL);
    }

  /* sep 2 */
  sep2 = 
    XtVaCreateManagedWidget("sep2", xmSeparatorWidgetClass,
			    sform,
			    XmNorientation, XmHORIZONTAL,
			    XmNseparatorType, XmSINGLE_LINE,
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, fuelscale,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.statsepPixel,
			    NULL);
			    

  /* alloc */
  alloclabel = 
    XtVaCreateManagedWidget("alloclabel", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("W/E ALLOC ="),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, sep2,
			    XmNleftOffset, 10,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.whitePixel,
			    NULL);
  XmStringFree(s);

  alloclabelval = 
    XtVaCreateManagedWidget("alloclabelval", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("30/70"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, sep2,
			    XmNleftOffset, 130,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.cyanPixel,
			    NULL);
  XmStringFree(s);



  /* etemp */
  etempscale = 
    XtVaCreateManagedWidget("etempscale", xmScaleWidgetClass,
			    sform,
			    XmNeditable, False,
			    XmNhighlightOnEnter, False,
			    XmNhighlightThickness, 0,
			    XmNbottomShadowColor, ConqData.greenPixel,
			    XmNtopShadowColor, ConqData.greenPixel,
			    XmNmaximum, 100,
			    XmNminimum, 0,
			    XmNorientation, XmHORIZONTAL,
			    XmNprocessingDirection, XmMAX_ON_RIGHT, 
			    XmNshowArrows, XmNONE,
			    XmNshowValue, XmNEAR_SLIDER,
			    XmNslidingMode, XmTHERMOMETER,
			    XmNsliderVisual, XmFOREGROUND_COLOR,
			    XmNtitleString, s = XmStringCreateLocalized("Engine Temperature"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, alloclabel,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNtraversalOn, False,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.greenPixel,
			    XmNvalue, 0);
  XmStringFree(s);

  if ((etempscroller = XtNameToWidget(etempscale, "Scrollbar")) == NULL)
    {
      clog("JET: could not fine Scrollbar\n");
    }
  else
    {
      XtVaSetValues(etempscroller,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.greenPixel,
		    NULL);
    }

  if ((etemptitle = XtNameToWidget(etempscale, "Title")) == NULL)
    {
      clog("JET: could not fine Title\n");
    }
  else
    {
      XtVaSetValues(etemptitle,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.whitePixel,
		    NULL);
    }

  /* wtemp */
  wtempscale = 
    XtVaCreateManagedWidget("wtempscale", xmScaleWidgetClass,
			    sform,
			    XmNeditable, False,
			    XmNhighlightOnEnter, False,
			    XmNhighlightThickness, 0,
			    XmNbottomShadowColor, ConqData.greenPixel,
			    XmNtopShadowColor, ConqData.greenPixel,
			    XmNmaximum, 100,
			    XmNminimum, 0,
			    XmNorientation, XmHORIZONTAL,
			    XmNprocessingDirection, XmMAX_ON_RIGHT, 
			    XmNshowArrows, XmNONE,
			    XmNshowValue, XmNEAR_SLIDER,
			    XmNslidingMode, XmTHERMOMETER,
			    XmNsliderVisual, XmFOREGROUND_COLOR,
			    XmNtitleString, s = XmStringCreateLocalized("Weapon Temperature"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, etempscale,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNtraversalOn, False,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.greenPixel,
			    XmNvalue, 0);
  XmStringFree(s);

  if ((wtempscroller = XtNameToWidget(wtempscale, "Scrollbar")) == NULL)
    {
      clog("JET: could not fine Scrollbar\n");
    }
  else
    {
      XtVaSetValues(wtempscroller,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.greenPixel,
		    NULL);
    }

  if ((wtemptitle = XtNameToWidget(wtempscale, "Title")) == NULL)
    {
      clog("JET: could not fine Title\n");
    }
  else
    {
      XtVaSetValues(wtemptitle,
		    XmNbackground, ConqData.blackPixel,
		    XmNforeground, ConqData.whitePixel,
		    NULL);
    }

  /* sep 3 */
  sep3 = 
    XtVaCreateManagedWidget("sep3", xmSeparatorWidgetClass,
			    sform,
			    XmNorientation, XmHORIZONTAL,
			    XmNseparatorType, XmSINGLE_LINE,
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, wtempscale,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.statsepPixel,
			    NULL);
			    
   /* kills */
  killlabel = 
    XtVaCreateManagedWidget("killlabel", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("Kills ="),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, sep3,
			    XmNleftOffset, 10,
			    XmNleftAttachment, XmATTACH_FORM,
			    NULL);
  XmStringFree(s);

  killlabelval = 
    XtVaCreateManagedWidget("killlabelval", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("0.0"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, sep3,
			    XmNleftOffset, 130,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNalignment, XmALIGNMENT_BEGINNING,
			    NULL);
  XmStringFree(s);

 /* armies */
  armieslabel = 
    XtVaCreateManagedWidget("armieslabel", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("ARM"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, killlabel,
			    XmNleftOffset, 10,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.whitePixel,
			    NULL);
  XmStringFree(s);

  armieslabelval = 
    XtVaCreateManagedWidget("armieslabelval", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("ARM"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, killlabelval,
			    XmNleftOffset, 130,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.blackPixel,
			    NULL);
  XmStringFree(s);

  /* towing */
  towlabel = 
    XtVaCreateManagedWidget("towlabel", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("AA TOWING"),
			    XmNtopAttachment, XmATTACH_WIDGET,
			    XmNtopWidget, armieslabel,
			    XmNleftOffset, 5,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNrightAttachment, XmATTACH_WIDGET,
			    XmNrightWidget, frame,
			    XmNalignment, XmALIGNMENT_BEGINNING,
			    XmNbackground, ConqData.blackPixel,
			    XmNforeground, ConqData.blackPixel,
			    NULL);
  XmStringFree(s);

  /* indicators? */
  /* cloak */
  cloaklabel = 
    XtVaCreateManagedWidget("cloaklabel", xmLabelWidgetClass,
			    sform,
			    XmNlabelString, s = XmStringCreateLocalized("CLOAKED"),
			    XmNbottomOffset, 5,
			    XmNbottomAttachment, XmATTACH_WIDGET,
			    XmNbottomWidget, bframe,
			    XmNleftOffset, 5,
			    XmNleftAttachment, XmATTACH_FORM,
			    XmNbackground, ConqData.redPixel,
			    XmNforeground, ConqData.blackPixel,
			    NULL);
  XmStringFree(s);

  /* opengl drawing area */
  glxarea = XtVaCreateManagedWidget("glxarea", glwMDrawingAreaWidgetClass, 
				    frame,
				    GLwNvisualInfo, visinfo,
				    XtNcolormap, cmap,
				    NULL);

  XtAddCallback(glxarea, GLwNginitCallback, graphicsInit, NULL);
  XtAddCallback(glxarea, GLwNexposeCallback, expose, NULL);
  XtAddCallback(glxarea, GLwNresizeCallback, resize, NULL);
  XtAddCallback(glxarea, GLwNinputCallback, input, NULL);


  XtRealizeWidget(ConqData.toplevel);

  /* now unmap framelabel */
  XtUnmapWidget(framelabel);

  /* empty message/prompt lines */
  showMessage(MSG_LIN1, NULL);
  showMessage(MSG_LIN2, NULL);
  showMessage(0, NULL);

  XStoreName(ConqData.dpy, XtWindow(ConqData.toplevel), CONQUESTGL_NAME);
  XSetIconName(ConqData.dpy, XtWindow(ConqData.toplevel), CONQUESTGL_NAME);

  XtSetKeyboardFocus(form, glxarea);

  /*  XtAppMainLoop(ConqData.app);*/

  //  procEvents();

  return 0;             
}

void
graphicsInit(Widget w, XtPointer clientData, XtPointer call)
{
  XVisualInfo *visinfo;

  /* Create OpenGL rendering context. */
  XtVaGetValues(w, GLwNvisualInfo, &visinfo, NULL);
  glxcontext = glXCreateContext(XtDisplay(w), visinfo,
				0,                  /* No sharing. */
				True);              /* Direct rendering if possible. */

  /* Setup OpenGL state. */

  glXMakeCurrent(XtDisplay(w), XtWindow(w), glxcontext);
  glClearDepth(1.0);
  glClearColor(0.0, 0.0, 0.0, 0.0);  /* clear to black */

  glShadeModel(GL_SMOOTH);
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
  
  glMatrixMode(GL_PROJECTION);
  gluPerspective(50.0/*40.0*/, 1.0, 10.0, 200.0);
  glMatrixMode(GL_MODELVIEW);
  glTranslatef(0.0, 0.0, -100.0);

  /*  Try something interesting...*/
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);
  glEnable(GL_COLOR_MATERIAL);

  if (!LoadGLTextures())
    clog("ERROR: LoadTextures() failed\n");

  InitFonts();
}

void
resize(Widget w,
  XtPointer clientData, XtPointer call)
{
  GLwDrawingAreaCallbackStruct *callData;

  callData = (GLwDrawingAreaCallbackStruct *) call;
  glXMakeCurrent(XtDisplay(w), XtWindow(w), glxcontext);
  glXWaitX();
  glViewport(0, 0, callData->width, callData->height);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(50.0, (GLfloat)callData->width/(GLfloat)callData->height, 
		 10.0, 200.0);
  glMatrixMode(GL_MODELVIEW);

  XtSetKeyboardFocus(form, glxarea);

}

void
expose(Widget w,
  XtPointer clientData, XtPointer call)
{
  startDraw();
  finishDraw();
  //renderFrame(0);
  //  drawGL();
}

void
drawstatic(void)
{				/* assumes context is current*/
  static GLfloat xtx = 0.0;

   return;			/* no need at the moment */

  glPushMatrix();
  glLoadIdentity();
  glTranslatef(xtx /*0.0*/, 0.0, -100.0 /*-75.0*/);

  glBegin(GL_LINES);
    glColor3f(1.0, 0.0, 0.0);	/* X */
    glVertex3f(20.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);	
    glVertex3f(-20.0, 0.0, 0.0);

    glColor3f(0.0, 1.0, 0.0);	/* Y */
    glVertex3f(0.0, 20.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);	
    glVertex3f(0.0, -20.0, 0.0);

    glColor3f(0.0, 0.0, 1.0);	/* Z */
    glVertex3f(0.0, 0.0, 20.0);
    glColor3f(1.0, 1.0, 1.0);	
    glVertex3f(0.0, 0.0, -20.0);

  glEnd();

  glPopMatrix();

  return;

}

void drawTorp(GLfloat x, GLfloat y, char torpchar, int torpcolor)
{
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(0.0, 0.0, -100.0 /*-75.0*/);

  glBlendFunc(GL_SRC_ALPHA, GL_ONE);
  glEnable(GL_BLEND);

  glEnable(GL_TEXTURE_2D); 
  glBindTexture(GL_TEXTURE_2D, textures[TEX_SUN]); /* mini suns */


  glBegin(GL_POLYGON);		/* draw a orange square */

  if (torpcolor == 0)
    glColor3f(1.0, 1.0, 1.0);
  else if (torpcolor == RedLevelColor)
    glColor3f(1.0, 0.0, 0.0);
  else if (torpcolor == GreenLevelColor)
    glColor3f(0.0, 1.0, 0.0);
  else if (torpcolor == YellowLevelColor)
    glColor3f(1.0, 1.0, 0.0);
  else
    glColor3f(0.5, 0.5, 0.5);

 drawTexBox(x, y, 3.0);
  glEnd();

  glDisable(GL_TEXTURE_2D); 
  glDisable(GL_BLEND);

  glPopMatrix();

  return;
}

/* start a frame */
void startDraw(void)
{
  glXMakeCurrent(ConqData.dpy, XtWindow(glxarea), glxcontext);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  return;
}

/* finish a frame */
void finishDraw(void)
{

  if (doubleBuffer)
    glXSwapBuffers(ConqData.dpy, XtWindow(glxarea));
  else
    glFlush();

  /* Avoid indirect rendering latency from queuing. */
  if (!glXIsDirect(ConqData.dpy, glxcontext))
    glFinish();

  //  XmUpdateDisplay(ConqData.toplevel);

  //  procEvent();
  //  glXWaitX();
  return;
}

void
drawGL(void)
{
  GLfloat x, y, z, ang;

  x = 0.0;
  y = 0.0;
  z = 1.0;
  ang = 2.5;


#if 0
  glXMakeCurrent(ConqData.dpy, XtWindow(glxarea), glxcontext);

  clog("DRAWGL ENTERING\n");

  glRotatef(ang, x, y, z);

  glBegin(GL_LINES);

  glColor3f(0.0, 1.0, 0.0);
  glVertex3f(-20.0, -20.0, 0.0);
  glColor3f(1.0, 1.0, 1.0);
  glVertex3f(-25.0, -25.0, 0.0); 
  
  glEnd();
#endif

  drawstatic();


  /*  finishDraw();*/

}

void
drawShip(GLfloat x, GLfloat y, GLfloat angle, char ch, int i, int color,
	 GLfloat scale)
{
  char buf[16];
  GLfloat alpha = 1.0;
  const GLfloat z = 1.0;
  GLfloat size = 7.0;
  GLfloat rx, ry;
  GLfloat sizeh;
  GLint texsel;
  Bool dophase = FALSE;
  const GLfloat viewrange = VIEWRANGE; /* need to figure out how to compute this */
  const GLfloat phaseradius = (PHASER_DIST / ((20.0 * SCALE_FAC) / viewrange));
  GLfloat rscale;

  dophase = ((scale == SCALE_FAC) && Ships[i].pfuse > 0);

  if (scale == MAP_FAC)
    size = size /= 2;

  sizeh = size/2;

  sprintf(buf, "%c%d", ch, i);

  /* set a lower alpha if we are cloaked. */
  if (ch == '~')
    {
      alpha = 0.5;		/* transparent */
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
    }
  else
    {
      glAlphaFunc(GL_GREATER, 0.000);
      glEnable(GL_ALPHA_TEST);
    }

    //  if (glIsEnabled(GL_ALPHA_TEST))
    //clog("ALPHA TEST ENABLED\n");
    //else
    //clog("ALPHA TEST NOT ENABLED\n");

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
  glTranslatef(x , y , -100.0);
  /* THEN rotate ;-) */
  glRotatef(angle, 0.0, 0.0, z);

  rx = x - (size / 2);
  ry = y - (size / 2);

  glColor4f(1.0, 1.0, 1.0, alpha);	
  //glColor3f(1.0, 1.0, 1.0);	

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

  if (ch == '~')
    glDisable(GL_BLEND);
  else
    glDisable(GL_ALPHA_TEST);

  if (dophase)			/* draw some phaser action */
    {
      glPushMatrix();
      glLoadIdentity();
      /* translate to correct position, */
      glTranslatef(x , y , -100.0);
      /* THEN rotate ;-) */
      glRotatef(Ships[i].lastphase, 0.0, 0.0, z);

      glLineWidth(2.0);

      glBegin(GL_LINES);
      glColor3f(1.0, 1.0, 1.0);
      glVertex3f(0.0, 0.0, 0.0);
      glColor3f(1.0, 0.0, 0.0);
      glVertex3f(phaseradius, 0.0, 0.0);
      glEnd();

      glLineWidth(1.0);
      glPopMatrix();
    }

  drawString(x, ((scale == SCALE_FAC) ? y - 5.0 : y - 4.0), buf, color);

}

void
drawDoomsday(GLfloat x, GLfloat y, GLfloat angle, GLfloat scale)
{
  GLfloat alpha = 1.0;
  const GLfloat z = 1.0;
  GLfloat size = 30.0;
  GLfloat sizeh;
  int dophase = FALSE;
  const GLfloat viewrange = VIEWRANGE; /* need to figure out how to compute this */
  GLfloat rscale;

  if (scale == MAP_FAC)
    size = size /= 2;

  sizeh = size/2;

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
  glTranslatef(x , y , -100.0);
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

void
input(Widget w, XtPointer clientData, XtPointer callData)
{
  XmDrawingAreaCallbackStruct *cd = (XmDrawingAreaCallbackStruct *) callData;
  char buffer[1];
  KeySym keysym;
  int numc;

  clog("INPUT\n");

  switch (cd->event->type) 
    {
    case KeyPress:
      if ((numc = XLookupString((XKeyEvent *) cd->event, buffer, 1, &keysym, NULL)) > 0) 
	{
	  clog("Got KeyPress, sym = 0x%x\n", keysym);
          clog("Got key 0x%x '%c'\n", *buffer, *buffer);
          iBufPutc(buffer[0]);

	  switch (keysym) 
	    {
	    
#if 0
	    case XK_space:   /* The spacebar. */
	      if (drawing) {
		GLStopTimer();
		drawing = False;
	      } else {
		GLSetTimer();
		drawing = True;
	      }
	      break;
	    case XK_Insert:
	      //      case XK_q:
	      ConqData.rendering = False;
	      XBell(ConqData.dpy, 100);
	      break;
	    case XK_Return:
	      spindir = !spindir;
	      break;
#endif
	    
	  }
	}
      else
	{
	  clog("Got sym 0x%x \n", keysym);
	  /*iBufPutc(keysym);*/
	}
      break;
    }
}

void
mapStateChanged(Widget w, XtPointer clientData,
  XEvent * event, Boolean * cont)
{
  switch (event->type) {
  case MapNotify:
    if (drawing && ConqData.timerId != 0)
      GLSetTimer();
    break;
  case UnmapNotify:
    if (drawing)
      GLStopTimer();
    break;
  }

  XtSetKeyboardFocus(form, glxarea);

  return;
}

Colormap
getShareableColormap(XVisualInfo * vi)
{
  Status status;
  XStandardColormap *standardCmaps;
  Colormap cmap;
  int i, numCmaps;

  /* Be lazy; using DirectColor too involved for this example. */
#if defined(_CH_) || defined(__cplusplus)
  if (vi->c_class != TrueColor)
#else
  if (vi->class != TrueColor)
#endif
    XtAppError(ConqData.app, "no support for non-TrueColor visual");
  /* If no standard colormap but TrueColor, just make an
     unshared one. */
  status = XmuLookupStandardColormap(ConqData.dpy, vi->screen, vi->visualid,
				     vi->depth, XA_RGB_DEFAULT_MAP,
				     False,              /* Replace. */
				     True);              /* Retain. */
  if (status == 1) 
    {
      status = XGetRGBColormaps(ConqData.dpy, RootWindow(ConqData.dpy, vi->screen),
				&standardCmaps, &numCmaps, XA_RGB_DEFAULT_MAP);
      if (status == 1)
	for (i = 0; i < numCmaps; i++)
	  if (standardCmaps[i].visualid == vi->visualid) 
	    {
	      cmap = standardCmaps[i].colormap;
	      XFree(standardCmaps);
	      return cmap;
	    }
    }
  cmap = XCreateColormap(ConqData.dpy, RootWindow(ConqData.dpy, vi->screen), vi->visual, AllocNone);
  return cmap;
}

/* simple loader for 24bit bitmaps (data is in rgb-format) */
int LoadBMP(char *filename, textureImage *texture, unsigned char alpha)
{
    FILE *file;
    unsigned short int bfType;
    long int bfOffBits;
    short int biPlanes;
    short int biBitCount;
    long int biSizeImage;
    long int biSizeImageA;
    unsigned char *readdata;
    int i,j;
    unsigned char temp;

    if ((file = fopen(filename, "rb")) == NULL)
    {
        clog("File not found : %s:%s\n", filename, strerror(errno));
        return FALSE;
    }
    if(!fread(&bfType, sizeof(short int), 1, file))
    {
        clog("Error reading file!\n");
        return FALSE;
    }
    /* check if file is a bitmap */
    if (bfType != 19778)
    {
        clog("Not a bitmap (*.bmp) file.\n");
        return FALSE;
    }        
    /* get the file size */
    /* skip file size and reserved fields of bitmap file header */
    fseek(file, 8, SEEK_CUR);
    /* get the position of the actual bitmap data */
    if (!fread(&bfOffBits, sizeof(long int), 1, file))
    {
        clog("Error reading file!\n");
        return FALSE;
    }

    /* skip size of bitmap info header */
    fseek(file, 4, SEEK_CUR);
    /* get the width of the bitmap */
    fread(&texture->width, sizeof(int), 1, file);

    /* get the height of the bitmap */
    fread(&texture->height, sizeof(int), 1, file);

    /* get the number of planes (must be set to 1) */
    fread(&biPlanes, sizeof(short int), 1, file);
    if (biPlanes != 1)
    {
        clog("Error: number of Planes not 1!\n");
        return FALSE;
    }
    /* get the number of bits per pixel */
    if (!fread(&biBitCount, sizeof(short int), 1, file))
    {
        clog("Error reading file: %s\n", strerror(errno));
        return FALSE;
    }
    if (biBitCount != 24)
    {
        clog("Bits per Pixel not 24\n");
        return FALSE;
    }

    biSizeImage = texture->width * texture->height * 3;
    biSizeImageA = texture->width * texture->height * 4;	/* + alpha */

    if ((readdata = malloc(biSizeImage)) == NULL)
    {
      clog("%s: malloc(%d) failed: %s\n", __FUNCTION__, biSizeImage, 
	   strerror(errno));
      return FALSE;
    }
    if ((texture->data = malloc(biSizeImageA)) == NULL)
    {
      clog("%s: malloc(%d) failed: %s\n", __FUNCTION__, biSizeImageA, 
	   strerror(errno));
      return FALSE;
    }
    /* seek to the actual data */
    fseek(file, bfOffBits, SEEK_SET);
    if (!fread(readdata, biSizeImage, 1, file))
    {
        clog("Error reading file: %s\n", strerror(errno));
        return FALSE;
    }
    /* swap red and blue (bgr -> rgb), add alpha channel */
    for (i=0, j=0; i < biSizeImage; i+=3, j+=4)
    {
      texture->data[j] = readdata[i+2];
      texture->data[j+1] = readdata[i+1];
      texture->data[j+2] = readdata[i];

      if (texture->data[j] == 0 && texture->data[j+1] == 0 &&
	  texture->data[j+2] == 0)
	texture->data[j+3] = 0;	/* set alpha 0 if black pixel */
      else
	texture->data[j+3] = alpha; 
    }
    if (readdata)
      free(readdata);

    return TRUE;
}

Bool LoadGLTextures()   /* Load Bitmaps And Convert To Textures */
{
    Bool status;
    int rv;
    textureImage *texti;
    int i;
    char filenm[512];

    status = False;

    for (i=0; i < NUM_TEX; i++)
      {
	texti = malloc(sizeof(textureImage));
	//	clog("LoadGLTextures: loading %s\n", TexInfo[i].filename);
	sprintf(filenm, "%s/%s", 
		CONQSHARE, TexInfo[i].filename);
	if ((rv = LoadBMP(filenm, texti, 
			  TexInfo[i].alpha)) == TRUE)
	  {
	    status = True;
	    glGenTextures(1, &textures[i]);   /* create the texture */
	    glBindTexture(GL_TEXTURE_2D, textures[i]);
	    /* actually generate the texture */
	    glTexImage2D(GL_TEXTURE_2D, 0, 4, texti->width, texti->height, 0,
			 GL_RGBA, GL_UNSIGNED_BYTE, texti->data);
	    /* use linear filtering */
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	  }
	
	GLError();

	/* free the ram we used in our texture generation process */
	if (texti)
	  {
	    if (texti->data && rv == TRUE)
	      free(texti->data);
	    free(texti);
	  }    
      }

    return status;
}

