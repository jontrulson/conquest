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

#include "cqsound.h"

#include "hud.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>


/* Global to this module */

static alertLevel_t AlertLevel = GREEN_ALERT;
extern real LastPhasDist;	/* defined in conqlb.c */

/*  display - do one update of a ships screen */
void display( int snum )
{
  int i, j, minenemy, minsenemy;
  char ch;
  int dobeep, lsmap;
  int palertcol;
  real scale, cenx, ceny, dis, mindis, minsdis;
  real prevdam;
  static int talertfx = -1;
  int color;
  GLfloat glx, gly;
  
  if (talertfx == -1)
      talertfx = cqsFindEffect("torp-alert");

  /* if we need to reset cached state... */
  if (Context.redraw)
    {
      AlertLevel = GREEN_ALERT;
      hudInitData();
      Context.redraw = FALSE;
    }

  hudSetAlertStatus(snum, 0, AlertLevel);

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
  
  /* adjust the ships orbital position if neccessary */
  clbAdjOrbitalPosition(snum);

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
	continue;
      if ( !GLcvtcoords( cenx, ceny, Planets[i].x, Planets[i].y, -scale, 
                         &glx, &gly ))
	continue;

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

      drawPlanet(glx, gly, i, scale, palertcol);
    }
  
  /* Display the planet eater. */
  if ( Doomsday->status == DS_LIVE )
    {
      if (GLcvtcoords( cenx, ceny, Doomsday->x, Doomsday->y, -scale, 
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
	    /* Display the torps on a LR scan if it's a friend (or you). */
	    if (lsmap)
	      {
		if (snum > 0 && ((snum == i) || 
                                 (Ships[snum].war[Ships[i].team] == FALSE &&
                                  Ships[i].war[Ships[snum].team] == FALSE)) )
		  {
		    for ( j = 0; j < MAXTORPS; j = j + 1 )
		      if ( Ships[i].torps[j].status == TS_LIVE 
			  || Ships[i].torps[j].status == TS_DETONATE )
			if ( GLcvtcoords( cenx, ceny, Ships[i].torps[j].x, 
					 Ships[i].torps[j].y, -scale, 
					 &glx, &gly ) )
			  {
			    drawTorp(glx, gly, scale, i, j);			    
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

                /* if the ship is in orbit, adjust it's position for
                 *  smoother motion on fast moving planets.  Our own
                 *  ship's position will have already been adjusted
                 *  above.
                 */

                if (snum != i)
                  clbAdjOrbitalPosition(i);

	      }	/* if a ship view (snum > 0) */

	    /* There is potential for un-cloaked ships and ourselves. */
	    if ( ! SCLOAKED(i) || i == snum )
	      {
		/* ... especially if he's in the bounds of our current */
		/*  display (either tactical or strategic map) */
		if (GLcvtcoords( cenx, ceny, Ships[i].x, Ships[i].y, 
				 -scale, &glx, &gly ))
		  {
		    /* He's on the screen. */
		    /* We can see him if one of the following is true: */

		    /*  - We are not looking at our strategic map and
                          the ship is within scanning distance */
		    /*  - We're mutually at peace */
		    /*  - Our team has scanned him and we're not self-war */
		    /*  - He's within accurate scanning range */
		    
		    if ( ( ! lsmap && SSCANDIST(i)) ||
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
                        if (SBOMBING(i))
                          drawBombing(i, scale);
		      }
		  }
	      }
	  } /* it's alive */
      } /* for each ship */
  

  /* now the torps for each ship.  we do this after the ships are drawn so
     blending of explosions works on the ships. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    {
      /* explosions first */
      if ( snum < 0 || (snum > 0 && UserConf.DoExplode) ) /* dwp */
        for ( j = 0; j < MAXTORPS; j = j + 1 )
          if ( Ships[i].torps[j].status == TS_FIREBALL )
            {
              /* First display exploding torps. */
              if ( GLcvtcoords( cenx, ceny, Ships[i].torps[j].x, 
                                Ships[i].torps[j].y, -scale, &glx, &gly ))
                { 
                  drawExplosion(glx, gly, i, j, scale);
                }
            }

      if ( ! lsmap ) 
	{
	  /* Now display the live torps. */
	  for ( j = 0; j < MAXTORPS; j = j + 1 )
	    if ( Ships[i].status != SS_DYING && Ships[i].status != SS_DEAD && 
		 (Ships[i].torps[j].status == TS_LIVE || Ships[i].torps[j].status == TS_DETONATE) )
	      if (GLcvtcoords( cenx, ceny, Ships[i].torps[j].x, 
                               Ships[i].torps[j].y, -scale, &glx, &gly))
		{
		  drawTorp(glx, gly, scale, i, j);
		}
	}
    }
  
  
  /* Figure out the ship's current alert status, and the ship causing the
   *  alert, if needed */
  if (snum > 0)
    {				/* if a ship view */
      if ( minenemy != 0 || STALERT(snum) )
	{
	  if ( mindis <= PHASER_DIST )
	    {
	      /* Nearest enemy is very close. */
	      AlertLevel = PHASER_ALERT;
	      dobeep = TRUE;
	    }
	  else if ( mindis < ALERT_DIST )
	    {
	      /* Nearest enemy is close. */
	      AlertLevel = RED_ALERT;
	      dobeep = TRUE;
	    }
	  else if ( STALERT(snum) )
	    {
	      /* Nearby torpedos. */
	      minenemy = 0;			/* disable nearby enemy code */
              cqsEffectPlay(talertfx, NULL, 0, 0, 0);

	      AlertLevel = TORP_ALERT;
	      dobeep = TRUE;
	    }
	  else if ( mindis < YELLOW_DIST )
	    {
	      /* Near an enemy. */
	      AlertLevel = YELLOW_ALERT;
	    }
	  else if ( minsenemy != 0 )
	    {
	      /* An enemy near one of our ships or planets. */
	      minenemy = minsenemy;		/* for cloaking code below */
	      AlertLevel = PROXIMITY_ALERT;
	    }
	  else
	    {
	      AlertLevel = GREEN_ALERT;
	      minenemy = 0;
	    }

	}
      else
	AlertLevel = GREEN_ALERT;
    }
  else /* if snum < 0 */
    AlertLevel = GREEN_ALERT;	/* for a special */

  hudSetAlertStatus(snum, minenemy, AlertLevel);
  
  /* Build and display the status info as necessary. */
  if (snum > 0) 
    {                           /* we're watching a ship - dwp */
      /* Shields. 
       * this will set dobeep if the shields dropped in power since last
       *  update 
       */
      hudSetShields(snum, &dobeep);

      /* Kills. */
      hudSetKills(snum);

      /* Warp. */
      hudSetWarp(snum);
      
      /* Heading. */
      hudSetHeading(snum);
      
      /* Fuel. */
      hudSetFuel(snum);

      /* Allocation. */

      hudSetAlloc(snum);

      /* Temperature. */
      hudSetTemps(snum);

      /* Damage/repair. */
      hudSetDamage(snum, &prevdam);

      if ( Ships[snum].damage > prevdam )
        {
          if ( (Ships[snum].damage - prevdam) > 5.0 )
            cqsEffectPlay(cqsTeamEffects[Ships[snum].team].hit, NULL, 0, 0, 0);
          
          dobeep = TRUE;
        }
      
      /* Armies/Robot action. */
      if (SROBOT(snum))
        hudSetRobotAction(snum);
      else
        hudSetArmies(snum);

      /* Tractor beams. */
      hudSetTow(snum);

      /* self destruct fuse */
      hudSetDestruct(snum);

      if ( UserConf.DoAlarms && dobeep )
        mglBeep(MGL_BEEP_ALERT);
      
    } /* end of ship stats display */
  
  return;
  
}

void displayFeedback(char *msg, int lin)
{
  if (!msg)
    return;

  hudSetPrompt(MSG_LIN1, NULL, NoColor, msg, NoColor);

  return;
}

