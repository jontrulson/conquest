#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"

#include "gldisplay.h"
#include "glmisc.h"
#include "conqlb.h"

#include "node.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>



#define GREEN_ALERT 0
#define YELLOW_ALERT 1
#define RED_ALERT 2

#define DS_LIVE_STR "DS_LIVE"
#define DS_OFF_STR  "DS_OFF"

/* Global to this module */

static int AlertLevel = GREEN_ALERT;
extern real LastPhasDist;	/* defined in conqlb.c */

static int alertcolor(int alert)
{
  int theattrib = 0;
  
  switch (alert)
    {
    case GREEN_ALERT:
      theattrib = GreenLevelColor;
      break;
    case YELLOW_ALERT:
      theattrib = YellowLevelColor;
      break;
    case RED_ALERT:
      theattrib = RedLevelColor;
      break;
    default:
      clog("alertcolor(): invalid alert level: %d", alert);
      break;
    }

  return(theattrib);
}


/*  display - do one update of a ships screen */
/*  SYNOPSIS */
/*    int snum, display_info */
/*    display( snum, display_info ) */
void display( int snum, int display_info )
{
  int i, j, minenemy, minsenemy;
  int outattr = 0;
  static int OldAlert = 0;
  char ch, buf[MSGMAXLINE];
  int dobeep, lsmap;
  int palertcol;
  real x, scale, cenx, ceny, dis, mindis, minsdis;
  static real zzskills = -20.0, 
    zzswarp = 92.0; /* "Warp 92 Mr. Sulu." */
  static char zzbuf[MSGMAXLINE] = "";
  static int zzsshields = -9, zzshead = 999, 
    zzsfuel = -99;
  static int zzsweapons = -1, zzsengines = -1, zzsdamage = -1, 
    zzsarmies = -1701;
  static int zzsetemp = 0, zzswtemp = 0, 
    zzssdfuse = -9;
  static real prevsh = 0.0 , prevdam = 100.0 ;
  static int ShieldAttrib = 0;
  static int FuelAttrib = 0;
  static int WeapAttrib = 0;
  static int EngAttrib = 0;
  static int DamageAttrib = 0;
  int color;
  GLfloat glx, gly;

  setXtraInfo();
  setAlertBorder(alertcolor(AlertLevel));

  dobeep = FALSE;
  mindis = 1.0e6;
  minsdis = 1.0e6;
  minenemy = 0;
  minsenemy = 0;

  dis = 0;

  if (snum > 0)
    lsmap = SMAP(snum);
  else
    lsmap = FALSE;
  
  if ( lsmap )
    {
      scale = MAP_FAC;
      
      if (UserConf.DoLocalLRScan)
	{
	  cenx = Ships[snum].x;
	  ceny = Ships[snum].y;
	}
      else
	{
	  cenx = 0.0;
	  ceny = 0.0;
	}
    }
  else
    {
      scale = SCALE_FAC;
      if (snum == DISPLAY_DOOMSDAY) { /* dwp */
	cenx = Doomsday->x;
	ceny = Doomsday->y;
      }
      else {
	cenx = Ships[snum].x;
	ceny = Ships[snum].y;
      }
    }
  
  /* Display the planets and suns. */
  for ( i = NUMPLANETS; i > 0; i = i - 1 )
    {
      if ( ! Planets[i].real )
	continue; /*next;*/
      if ( !GLcvtcoords( cenx, ceny, Planets[i].x, Planets[i].y, scale, 
                         &glx, &gly ))
	continue; /* next;*/

      palertcol = 0;
				/* determine alertlevel for object */
      if (snum > 0 && clbSPWar( snum, i ) && Planets[i].scanned[Ships[snum].team])
	{
	  palertcol = RedLevelColor;
	}
      else if (snum > 0 && Planets[i].team == Ships[snum].team && !selfwar(snum))
	{
	  palertcol = GreenLevelColor;
	}
      else if ( snum > 0 && Planets[i].team == TEAM_SELFRULED &&
                Planets[i].scanned[Ships[snum].team])
        palertcol = CyanColor;
      else
        {
          if (Planets[i].scanned[Ships[snum].team])
            palertcol = YellowLevelColor;
          else
            palertcol = NoColor | CQC_A_DIM;
        }
				/* suns are always yellow level material */
      if (Planets[i].type == PLANET_SUN)
	palertcol = YellowLevelColor;

      if ( lsmap )
	{
	  /* Strategic map. */
	  /* Can't see moons. */
	  if ( Planets[i].type == PLANET_MOON )
	    continue; 

          uiDrawPlanet(glx, gly, i, scale, palertcol, FALSE);
        }
      else
	{
	  /* Tactical map. */
          uiDrawPlanet(glx, gly, i, scale, palertcol, 
                       ((snum > 0) ? 
                        Planets[i].scanned[Ships[snum].team] : FALSE));
	}
    }
  
  /* Display the planet eater. */
  if ( Doomsday->status == DS_LIVE )
    {
      if (GLcvtcoords( cenx, ceny, Doomsday->x, Doomsday->y, scale, 
		       &glx, &gly ))
	{
	  drawDoomsday(glx, gly, (GLfloat)Doomsday->heading, scale);
	}
    }

  /* Display the ships. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status != SS_OFF )
      {
	if (UserConf.DoLRTorpScan)
	  {
	    /* Display the torps on a LR scan if it's a friend. */
	    if (lsmap)
	      {
		if (snum > 0 && Ships[snum].war[Ships[i].team] == FALSE &&
		    Ships[i].war[Ships[snum].team] == FALSE)
		  {
                    
                    if (i == snum) /* if it's your torps you're a ship */
                      color = 0;
                    else if (i != snum && satwar(i, snum))
                      color = RedLevelColor;
                    else if (Ships[i].team == Ships[snum].team)
                      color = GreenLevelColor;
                    else
                      color = YellowLevelColor;
                    
		    for ( j = 0; j < MAXTORPS; j = j + 1 )
		      if ( Ships[i].torps[j].status == TS_LIVE 
			  || Ships[i].torps[j].status == TS_DETONATE )
			if ( GLcvtcoords( cenx, ceny, Ships[i].torps[j].x, 
					 Ships[i].torps[j].y, scale, 
					 &glx, &gly ) )
			  {
			    drawTorp(glx, gly, Teams[Ships[i].team].torpchar, 
				     color, scale, i, j);			    
			  }
		  }
	      }
	  }
	
	/* Display the ships. */
	if ( Ships[i].status == SS_LIVE )
	  {
	    /* It's alive. */
	    if ( snum > 0)	/* it's a ship view */
	      {
		dis = (real) dist(Ships[snum].x, Ships[snum].y, Ships[i].x, Ships[i].y );
	    
		/* Here's where ship to ship accurate information is
                   "gathered". */
		/* Check for nearest enemy and nearest scanned enemy. */
		if ( satwar( i, snum ) )
		  if ( i != snum )
		    {
		  
#ifdef WARP0CLOAK
		      /* 1/6/94 */
		      /* we want any cloaked ship at warp 0.0 */
		      /* to be invisible. */
		      if (SCLOAKED(i) && Ships[i].warp == 0.0)
			{
			  /* skip to next ship. this one isn't here */
			  /* ;-} */
			  continue;	/* RESTART FOR */
			}
#endif /* WARP0CLOAK */
		  
		      if ( dis < mindis )
			{
			  /* New nearest enemy. */
			  mindis = dis;
			  minenemy = i;
			}
		      if ( dis < minsdis )
			if ( ! selfwar( snum ) )
			  if ( Ships[i].scanned[Ships[snum].team] > 0 )
			    {
			      /* New nearest scanned enemy. */
			      minsdis = dis;
			      minsenemy = i;
			    }
		      
		    }
	      }	/* if a ship view (snum > 0) */

	    /* There is potential for un-cloaked ships and ourselves. */
	    if ( ! SCLOAKED(i) || i == snum )
	      {
		/* ... especially if he's in the bounds of our current */
		/*  display (either tactical or strategic map) */
		if (GLcvtcoords( cenx, ceny, Ships[i].x, Ships[i].y, 
				 scale, &glx, &gly ))
		  {
		    /* He's on the screen. */
		    /* We can see him if one of the following is true: */

		    /*  - We are not looking at our strategic map */
		    /*  - We're mutually at peace */
		    /*  - Our team has scanned him and we're not self-war */
		    /*  - He's within accurate scanning range */
		    
		    if ( ( ! lsmap ) ||
			( snum > 0 && !satwar(i, snum) ) ||
			( snum > 0 && Ships[i].scanned[Ships[snum].team] && 
			  !selfwar(snum) ) ||
			( dis <= ACCINFO_DIST ) )
		      {
			if ( snum > 0 && ( i == snum ) && SCLOAKED(snum) )
			  ch = CHAR_CLOAKED;
			else
			  ch = Teams[Ships[i].team].teamchar;
			
				/* determine color */
			if (snum > 0)
			  {
			    if (i == snum)    /* it's ours */
			      color = CQC_A_BOLD;
			    else if (satwar(i, snum)) /* we're at war with it */
			      color = RedLevelColor;
			    else if (Ships[i].team == Ships[snum].team && !selfwar(snum))
			      color = GreenLevelColor; /* it's a team ship */
			    else
			      color = YellowLevelColor;
			  }
			else
			  color = YellowLevelColor; /* special view */

			drawShip(glx, gly, Ships[i].head, ch, i, color, scale);
		      }
		  }
	      }
	  } /* it's alive */
      } /* for each ship */
  

  /* now the torps for each ship.  we do this after the ships are drawn so
     blending of explosions works on the ships. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    {
      if ( ! lsmap )
	{
	  /* First display exploding torps. */
	  if ( snum < 0 || (snum > 0 && UserConf.DoExplode) ) /* dwp */
	    for ( j = 0; j < MAXTORPS; j = j + 1 )
	      if ( Ships[i].torps[j].status == TS_FIREBALL )
		if ( GLcvtcoords( cenx, ceny, Ships[i].torps[j].x, 
                                  Ships[i].torps[j].y, scale, &glx, &gly ))
		  { 
		    drawExplosion(glx, gly, i, j);
		  }
	  /* Now display the live torps. */
	  
	  if (snum > 0)
	    {			/* a ship */

              if (i == snum) /* if it's your torps you're a ship */
                color = 0;
              else if (i != snum && satwar(i, snum))
                color = RedLevelColor;
              else if (Ships[i].team == Ships[snum].team)
                color = GreenLevelColor;
              else
                color = YellowLevelColor;
	    }
	  else		/* a special */
	    color = YellowLevelColor;
	  
	  for ( j = 0; j < MAXTORPS; j = j + 1 )
	    if ( Ships[i].status != SS_DYING && Ships[i].status != SS_DEAD && 
		 (Ships[i].torps[j].status == TS_LIVE || Ships[i].torps[j].status == TS_DETONATE) )
	      if (GLcvtcoords( cenx, ceny, Ships[i].torps[j].x, 
                               Ships[i].torps[j].y, scale, &glx, &gly))
		{
		  drawTorp(glx, gly, Teams[Ships[i].team].torpchar, color,
                           scale, i, j);
		}
	}
    }
  
  
  /* Construct alert status line. */
  buf[0] = EOS;

  if (snum > 0)
    {				/* if a ship view */
      if ( minenemy != 0 || STALERT(snum) )
	{
	  if ( mindis <= PHASER_DIST )
	    {
	      /* Nearest enemy is very close. */
	      outattr = RedLevelColor;
	      AlertLevel = RED_ALERT;
	      c_strcpy( "RED ALERT ", buf );
	      dobeep = TRUE;
	    }
	  else if ( mindis < ALERT_DIST )
	    {
	      /* Nearest enemy is close. */
	      outattr = RedLevelColor;
	      AlertLevel = RED_ALERT;
	      c_strcpy( "Alert ", buf );
	      dobeep = TRUE;
	    }
	  else if ( STALERT(snum) )
	    {
	      /* Nearby torpedos. */
	      outattr = YellowLevelColor;
	      AlertLevel = YELLOW_ALERT;
	      c_strcpy( "Torp alert", buf );
	      minenemy = 0;			/* disable nearby enemy code */
	      dobeep = TRUE;
	    }
	  else if ( mindis < YELLOW_DIST )
	    {
	      /* Near an enemy. */
	      outattr = YellowLevelColor;
	      AlertLevel = YELLOW_ALERT;
	      c_strcpy( "Yellow alert ", buf );
	    }
	  else if ( minsenemy != 0 )
	    {
	      /* An enemy near one of our ships or planets. */
	      outattr = YellowLevelColor;
	      minenemy = minsenemy;		/* for cloaking code below */
	      AlertLevel = YELLOW_ALERT;
	      c_strcpy( "Proximity Alert ", buf );
	    }
	  else
	    {
	      outattr = GreenLevelColor;
	      AlertLevel = GREEN_ALERT;
	      minenemy = 0;
	    }
	  
	  if ( minenemy != 0 )
	    {
	      appship( minenemy, buf );
	      if ( SCLOAKED(minenemy) )
		appstr( " (CLOAKED)", buf );
	    }
	}
      else
	AlertLevel = GREEN_ALERT;
    }
  else /* if snum < 0 */
    AlertLevel = GREEN_ALERT;	/* for a special */

  if (OldAlert != AlertLevel)
    {
      setXtraInfo();
      setAlertBorder(alertcolor(AlertLevel));
      OldAlert = AlertLevel;
    }
  
  if ((strcmp( buf, zzbuf ) != 0) || UserConf.AltHUD)
    {
      setXtraInfo();
      setAlertLabel(buf, outattr);
      c_strcpy( buf, zzbuf );
    }
  
  /* Build and display the status info as necessary. */
  if (snum > 0) 
    { /* we're watching a ship */  /* dwp */
    
    /* Shields. */
    if ( Ships[snum].shields < prevsh )
      dobeep = TRUE;
    prevsh = Ships[snum].shields;
    
    i = round( Ships[snum].shields );
    if ( ! SSHUP(snum) || SREPAIR(snum) )
      i = -1;
    if ( i != zzsshields || i == -1)
      {
	if ( i == -1 )
	  {
	    if (AlertLevel == YELLOW_ALERT) 
	      color = YellowLevelColor;
	    else if (AlertLevel == RED_ALERT)
	      color = RedLevelColor ;
	    else
	      color = GreenLevelColor;
	    
	    setShields(i, color);

	  }
	else
	  {
	    if (i >= 0 && i <= 50)
	      ShieldAttrib = RedLevelColor;
	    else if (i >=51 && i <=80)
	      ShieldAttrib = YellowLevelColor;
	    else if (i >= 81)
	      ShieldAttrib = GreenLevelColor;
	    
	    setShields(i, ShieldAttrib);
	  }
	zzsshields = i;
      }
    
    /* Kills. */
    x = (Ships[snum].kills + Ships[snum].strkills);
    if ( x != zzskills )
      {
	sprintf( buf, "%0.1f", oneplace(x) );
	
	setKills(buf);
	
	zzskills = x;
      }
    
    /* Warp. */
    x = Ships[snum].warp;
    if ( x != zzswarp )
      {
	if ( x >= 0.0 )
	  {
	    sprintf( buf, "%2.1f", x );
	    setWarp(buf);
	  }
	else
	  {
	    setWarp(" o ");
	  }
	
	zzswarp = x;
      }
    
    /* Heading. */
    i = Ships[snum].lock;
    if ( i >= 0 || Ships[snum].warp < 0.0)
      i = round( Ships[snum].head );
    if ( i != zzshead)
      {
	if ( -i > 0 && -i <= NUMPLANETS)
	  sprintf( buf, "%.3s", Planets[-i].name );
	else
	  sprintf( buf, "%3d", i );
	setHeading(buf);
	zzshead = i;
      }
    
    /* Fuel. */
    i = round( Ships[snum].fuel );
    if ( i != zzsfuel )
      {
	if (i >= 0 && i <= 200)
	  FuelAttrib = RedLevelColor;
	else if (i >=201 && i <=500)
	  FuelAttrib = YellowLevelColor;
	else if (i >= 501)
	  FuelAttrib = GreenLevelColor;
	
	setFuel(i, FuelAttrib);
	zzsfuel = i;
      }
    
    /* Allocation. */
    i = Ships[snum].weapalloc;
    j = Ships[snum].engalloc;
    if ( Ships[snum].wfuse > 0 )
      i = 0;
    if ( Ships[snum].efuse > 0 )
      j = 0;
    if ( i != zzsweapons || j != zzsengines )
      {
	buf[0] = EOS;
	if ( i == 0 )
	  appstr( "**", buf );
	else
	  appint( i, buf );
	appchr( '/', buf );
	if ( j == 0 )
	  appstr( "**", buf );
	else
	  appint( j, buf );
	setAlloc(i, j, buf);
	zzsweapons = i;
      }
    
    /* Temperature. */
    i = round( Ships[snum].wtemp );
    j = round( Ships[snum].etemp );
    if ( i > 100 )
      i = 100;
    if ( j > 100 )
      j = 100;
    if ( i != zzswtemp || j != zzsetemp )
      {
	if ( i != 0 || j != 0 )
	  {
	    buf[0] = EOS;
	    
	    if (i >= 0 && i <= 50)
	      WeapAttrib = GreenLevelColor;
	    else if (i >=51 && i <=75)
	      WeapAttrib = YellowLevelColor;
	    else if (i >= 76)
	      WeapAttrib = RedLevelColor;
	    
	    if (j >= 0 && j <= 50)
	      EngAttrib = GreenLevelColor;
	    else if (j >=51 && j <=80)
	      EngAttrib = YellowLevelColor;
	    else if (j >= 81)
	      EngAttrib = RedLevelColor;
	    
	    setTemp(j, EngAttrib, i, WeapAttrib, Ships[snum].efuse,
		     Ships[snum].wfuse);
	    
	  }
	else
	  setTemp(0, GreenLevelColor, 0, GreenLevelColor, 0, 0);
	    
	zzswtemp = i;
	zzsetemp = j;
      }
    
    /* Damage/repair. */
    if ( Ships[snum].damage > prevdam )
      dobeep = TRUE;
    prevdam = Ships[snum].damage;
    
    i = round( Ships[snum].damage );
    if ( i != zzsdamage )
      {
	if ( i > 0 )
	  {
	    sprintf( buf, "%d", i );
	    if (i >= 0 && i <= 10)
	      DamageAttrib = GreenLevelColor;
	    else if (i >=11 && i <=65)
	      DamageAttrib = YellowLevelColor;
	    else if (i >= 66)
	      DamageAttrib = RedLevelColor;
	  
	    setDamage(i, DamageAttrib);

	  }
	else
	  setDamage(0, GreenLevelColor);
	zzsdamage = i;
      }

    if ( SREPAIR(snum) )
      j = 'r';
    else if ( i >= 50 )
      j = 'D';
    else if ( i > 0 )
      j = 'd';
    else
      j = ' ';

    if ( j == 'r' )
      setDamageLabel("REPAIR", GreenLevelColor);
    else if ( j == 'd' )
      setDamageLabel("damage", YellowLevelColor);
    else if ( j == 'D' )
      setDamageLabel("DAMAGE", RedLevelColor);
    else 
      setDamageLabel("No Damage", GreenLevelColor);

    /* Armies. */
    i = Ships[snum].armies;
    if ( i == 0 )
      i = -Ships[snum].action;
    if ( i != zzsarmies )
      {
	if ( i > 0 )
	  {
	    sprintf( buf, "%d", i );
	    setArmies("Armies = ", buf);
	  }
	else if ( i < 0 )
	  {
	    robstr( -i, buf );
	    setArmies("AI: ", buf);
	  }
	else
	  setArmies("", "");

	zzsarmies = i;
      }
  
    /* Tractor beams. */
    i = Ships[snum].towedby;
    if ( i == 0 )
      i = -Ships[snum].towing;
    
    if ( i == 0 )
      {
        buf[0] = EOS;
      }
    else if ( i < 0 )
      {
        c_strcpy( "towing ", buf );
        appship( -i, buf );
      }
    else if ( i > 0 )
      {
        c_strcpy( "towed by ", buf );
        appship( i, buf );
      }
    setTow(buf);
  
    /* Self destruct fuse. */
    if ( SCLOAKED(snum) )
      i = -1;
    else
      i = max( 0, Ships[snum].sdfuse );
    if ( i != zzssdfuse )
      {
	if ( i > 0 )
	  {
	    sprintf( buf, "DESTRUCT MINUS %-3d", i );
	    setCloakDestruct(buf, RedLevelColor);
	  }
	else if ( i == -1 )
	  {
	    setCloakDestruct(" CLOAKED ", MagentaColor);
	  }
	else 
	  {
	    setCloakDestruct("", NoColor);
	  }
	zzssdfuse = i;
      }
  
    if ( dobeep )
      if ( UserConf.DoAlarms )
	mglBeep();
  
  } /* end of ship stats display */
  
  return;
  
}

void displayFeedback(char *msg, int lin)
{
  if (!msg)
    return;

  setPrompt(MSG_LIN1, NULL, NoColor, msg, NoColor);

  return;
}
