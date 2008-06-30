/************************************************************************
 *
 * $Id$
 *
 * Copyright 2008 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "c_defs.h"

#include "conqdef.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "conqcom.h"
#include "conqlb.h"

extern void setWarp(real warp); /* FIXME, GL.c */

#define NOEXTERN_HUD
#include "hud.h"
#undef  NOEXTERN_HUD

/* init all of the hud data to 'default' values.  This should ensure a
 *  forced update when any of the items are set.
 */
void hudInitData(void)
{
  memset(&hudData, 0, sizeof(hudData_t));

  hudData.warp.warp        = 92.0;     /* "Warp 92 Mr. Sulu." */
  hudData.warp.color       = InfoColor;

  hudData.heading.head     = 999;
  hudData.heading.color    = NoColor;

  hudData.kills.kills      = -20.0;
  hudData.kills.color      = InfoColor;

  hudData.aStat.alertLevel = GREEN_ALERT;
  hudData.aStat.aShip      = 0;
  hudData.aStat.color      = GreenLevelColor;

  hudData.sh.shields       = -9;
  hudData.sh.color         = GreenLevelColor;

  hudData.dam.damage       = 999.0;
  hudData.dam.color        = GreenLevelColor;

  hudData.fuel.fuel        = -99;
  hudData.fuel.color       = GreenLevelColor;

  hudData.alloc.walloc     = -1;
  hudData.alloc.ealloc     = -1;
  hudData.alloc.color      = InfoColor;

  hudData.etemp.temp       = -1.0;
  hudData.etemp.color      = GreenLevelColor;
  hudData.etemp.overl      = FALSE;

  hudData.wtemp.temp       = -1.0;
  hudData.wtemp.color      = GreenLevelColor;
  hudData.wtemp.overl      = FALSE;

  return;
}

/* These routines assume that snum is always valid, so make sure it is. */

/* set hud warp data to current, if it's changed */
void hudSetWarp(int snum)
{
  if ( Ships[snum].warp != hudData.warp.warp )
    {
      hudData.warp.warp = Ships[snum].warp;
      if (hudData.warp.warp >= 0)
        snprintf(hudData.warp.warpstr, HUD_STR_SZ - 1, "%2.1f", 
                 hudData.warp.warp );
      else
        strncpy(hudData.warp.warpstr, "Orbiting", HUD_STR_SZ - 1);

      hudData.warp.warpstr[HUD_STR_SZ] = 0;

      /* set the right sound effects */
      setWarp(hudData.warp.warp);
    }

  return;
}


void hudSetHeading(int snum)
{
  int i = Ships[snum].lock;

  if ( i >= 0 || Ships[snum].warp < 0.0)
    i = round( Ships[snum].head );
  if ( i != hudData.heading.head)
    {
      if ( -i > 0 && -i <= NUMPLANETS)
        {                       /* just the first 3 characters if locked */
          hudData.heading.headstr[0] = Planets[-i].name[0];
          hudData.heading.headstr[1] = Planets[-i].name[1];
          hudData.heading.headstr[2] = Planets[-i].name[2];
          hudData.heading.headstr[3] = 0;
        }
      else
        {
          snprintf( hudData.heading.headstr, HUD_STR_SZ - 1, "%3d", i );
          hudData.heading.headstr[HUD_STR_SZ] = 0;
        }

      hudData.heading.head = i;
    }

  return;
}

void hudSetAlertStatus(int snum, int asnum, alertLevel_t astatus)
{

  if (snum <= 0)
    {
      if (hudData.aStat.alertLevel != GREEN_ALERT) /* a special */
        {
          hudData.aStat.alertLevel = GREEN_ALERT;
          hudData.aStat.color = GreenLevelColor;
          hudData.aStat.alertstr[0] = 0;
        }
      
      return;
    }

  if (hudData.aStat.alertLevel != astatus ||
      hudData.aStat.aShip != asnum)
    {
      switch (astatus)
        {
        case PHASER_ALERT:
          {
            hudData.aStat.color = RedLevelColor;
            strcpy(hudData.aStat.alertstr, "RED ALERT ");
          }
          break;

        case RED_ALERT:
          {
            hudData.aStat.color = RedLevelColor;
            strcpy(hudData.aStat.alertstr, "Alert ");
          }
          break;

        case TORP_ALERT:
          {
            hudData.aStat.color = YellowLevelColor;
            strcpy(hudData.aStat.alertstr, "Torp Alert ");
          }
          break;

        case YELLOW_ALERT:
          {
            hudData.aStat.color = YellowLevelColor;
            strcpy(hudData.aStat.alertstr, "Yellow Alert ");
          }
          break;

        case PROXIMITY_ALERT:
          {
            hudData.aStat.color = YellowLevelColor;
            strcpy(hudData.aStat.alertstr, "Proximity Alert ");
          }
          break;

        case GREEN_ALERT:
        default:
          {
            hudData.aStat.color = GreenLevelColor;
            hudData.aStat.alertstr[0] = 0;
          }
          break;
        }

      if (asnum)
        appship(asnum, hudData.aStat.alertstr);

      if (SCLOAKED(asnum))
        appstr( " (CLOAKED)", hudData.aStat.alertstr );

      hudData.aStat.alertstr[HUD_STR_SZ] = 0;

      hudData.aStat.alertLevel = astatus;
      hudData.aStat.aShip      = asnum;
    }

  return;

}

void hudSetKills(int snum)
{
  real x = (Ships[snum].kills + Ships[snum].strkills);

  if ( x != hudData.kills.kills )
    {
      snprintf( hudData.kills.killstr, HUD_STR_SZ - 1, "%0.1f", oneplace(x) );
      hudData.kills.kills = x;
    }

  return;
}


void hudSetShields(int snum, int *dobeep)
{
  int i, k;

  if (Ships[snum].shields < hudData.sh.shields)
    *dobeep = TRUE;

  i = k = round( Ships[snum].shields );

  if ( ! SSHUP(snum) || SREPAIR(snum) )
    i = -1;

  /* update the color */
  if (k >= 0 && k <= 50)
    hudData.sh.color = RedLevelColor;
  else if (k >= 51 && k <=80)
    hudData.sh.color = YellowLevelColor;
  else if (k >= 81)
    hudData.sh.color = GreenLevelColor;

  /* update the text */
  if ( i != hudData.sh.shields)
    {

      hudData.sh.shields = i;

      snprintf(hudData.sh.shieldstr, HUD_STR_SZ - 1, "%3d", (i < 0) ? 0 : i);
      hudData.sh.shieldstr[HUD_STR_SZ] = 0;
    }

  return;
}

void hudSetDamage(int snum, real *lastdamage)
{
  real r = Ships[snum].damage;
  int i;

  *lastdamage = hudData.dam.damage;

  if ( r != hudData.dam.damage )
    {
      i = round(r);
      if (i >= 0 && i <= 10)
        hudData.dam.color = GreenLevelColor;
      else if (i >= 11 && i <= 65)
        hudData.dam.color = YellowLevelColor;
      else if (i >= 66)
        hudData.dam.color = RedLevelColor;

      snprintf( hudData.dam.damagestr, HUD_STR_SZ - 1, "%3d",
                (i < 0) ? 0 : i );
      hudData.dam.damagestr[HUD_STR_SZ] = 0;

      hudData.dam.damage = r;
    }
  
  return;
}

void hudSetFuel(int snum)
{
  real r = Ships[snum].fuel;
  int i;

  if ( r != hudData.fuel.fuel )
    {
      i = round( r );
      if (i >= 0 && i <= 200)
        hudData.fuel.color = RedLevelColor;
      else if (i >= 201 && i <= 500)
        hudData.fuel.color = YellowLevelColor;
      else if (i >= 501)
        hudData.fuel.color = GreenLevelColor;
      
      snprintf( hudData.fuel.fuelstr, HUD_STR_SZ - 1, "%3d",
                (i < 0) ? 0 : i );
      hudData.fuel.fuelstr[HUD_STR_SZ] = 0;

      hudData.fuel.fuel = r;
    }


  return;
}

void hudSetAlloc(int snum)
{
  int i = Ships[snum].weapalloc;
  int j = Ships[snum].engalloc;

  if ( Ships[snum].wfuse > 0 )
    i = 0;
  if ( Ships[snum].efuse > 0 )
    j = 0;

  if ( i != hudData.alloc.walloc || j != hudData.alloc.ealloc )
    {
      hudData.alloc.allocstr[0] = 0;

      if ( i == 0 )
        appstr( "**", hudData.alloc.allocstr );
      else
        appint( i, hudData.alloc.allocstr );

      appchr( '/', hudData.alloc.allocstr );

      if ( j == 0 )
        appstr( "**", hudData.alloc.allocstr );
      else
        appint( j, hudData.alloc.allocstr );

      hudData.alloc.walloc  = i;
      hudData.alloc.ealloc  = j;
    }
  
  return;
}

void hudSetTemps(int snum)
{
  real etemp = Ships[snum].etemp;
  real wtemp = Ships[snum].wtemp;
  int eOverl, wOverl;
  int i;

  if (etemp > 100.0)
    etemp = 100.0;
  if (wtemp > 100.0)
    wtemp = 100.0;

  if (Ships[snum].efuse > 0)
    eOverl = TRUE;
  else
    eOverl = FALSE;

  if (Ships[snum].wfuse > 0)
    wOverl = TRUE;
  else
    wOverl = FALSE;
  
  if (etemp != hudData.etemp.temp || eOverl != hudData.etemp.overl)
    {
      i = round( etemp );

      if (i >= 0 && i <= 50)
        hudData.etemp.color = GreenLevelColor;
      else if (i >=51 && i <=80)
        hudData.etemp.color = YellowLevelColor;
      else if (i >= 81)
        hudData.etemp.color = RedLevelColor;

      snprintf( hudData.etemp.tempstr, HUD_STR_SZ - 1, "%3d",
                (i < 0) ? 0 : i );
      hudData.etemp.tempstr[HUD_STR_SZ] = 0;

      hudData.etemp.temp  = etemp;
      hudData.etemp.overl = eOverl;
    }

  if (wtemp != hudData.wtemp.temp || wOverl != hudData.wtemp.overl)
    {
      i = round( wtemp );

      if (i >= 0 && i <= 50)
        hudData.wtemp.color = GreenLevelColor;
      else if (i >=51 && i <=80)
        hudData.wtemp.color = YellowLevelColor;
      else if (i >= 81)
        hudData.wtemp.color = RedLevelColor;

      snprintf( hudData.wtemp.tempstr, HUD_STR_SZ - 1, "%3d",
                (i < 0) ? 0 : i );
      hudData.wtemp.tempstr[HUD_STR_SZ] = 0;

      hudData.wtemp.temp  = wtemp;
      hudData.wtemp.overl = wOverl;

    }

  return;
}
