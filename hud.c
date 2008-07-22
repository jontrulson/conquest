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

  hudData.tow.towstat      = 0;
  hudData.tow.color        = MagentaColor;

  hudData.armies.armies    = -1701;
  hudData.armies.color     = InfoColor;

  hudData.raction.action   = 0;
  hudData.raction.color    = InfoColor;

  hudData.destruct.fuse    = -1;
  hudData.destruct.color   = RedLevelColor;

  hudData.info.lastblast   = -1;
  hudData.info.lastang     = -1;
  hudData.info.lastdist    = -1;
  hudData.info.lasttarget  = 0;
  hudSetInfoFiringAngle(0);

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
        snprintf(hudData.warp.str, HUD_STR_SZ - 1, "%2.1f", 
                 hudData.warp.warp );
      else
        strncpy(hudData.warp.str, "Orbiting", HUD_STR_SZ - 1);

      hudData.warp.str[HUD_STR_SZ - 1] = 0;

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
          hudData.heading.str[0] = Planets[-i].name[0];
          hudData.heading.str[1] = Planets[-i].name[1];
          hudData.heading.str[2] = Planets[-i].name[2];
          hudData.heading.str[3] = 0;
        }
      else
        {
          snprintf( hudData.heading.str, HUD_STR_SZ - 1, "%3d", i );
          hudData.heading.str[HUD_STR_SZ - 1] = 0;
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
          hudData.aStat.str[0] = 0;
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
            strcpy(hudData.aStat.str, "RED ALERT ");
          }
          break;

        case RED_ALERT:
          {
            hudData.aStat.color = RedLevelColor;
            strcpy(hudData.aStat.str, "Alert ");
          }
          break;

        case TORP_ALERT:
          {
            hudData.aStat.color = YellowLevelColor;
            strcpy(hudData.aStat.str, "Torp Alert ");
          }
          break;

        case YELLOW_ALERT:
          {
            hudData.aStat.color = YellowLevelColor;
            strcpy(hudData.aStat.str, "Yellow Alert ");
          }
          break;

        case PROXIMITY_ALERT:
          {
            hudData.aStat.color = YellowLevelColor;
            strcpy(hudData.aStat.str, "Proximity Alert ");
          }
          break;

        case GREEN_ALERT:
        default:
          {
            hudData.aStat.color = GreenLevelColor;
            hudData.aStat.str[0] = 0;
          }
          break;
        }

      if (asnum)
        appship(asnum, hudData.aStat.str);

      if (SCLOAKED(asnum))
        appstr( " (CLOAKED)", hudData.aStat.str );

      hudData.aStat.str[HUD_STR_SZ - 1] = 0;

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
      snprintf( hudData.kills.str, HUD_STR_SZ - 1, "%0.1f", oneplace(x) );
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

      snprintf(hudData.sh.str, HUD_STR_SZ - 1, "%3d", (i < 0) ? 0 : i);
      hudData.sh.str[HUD_STR_SZ - 1] = 0;
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

      snprintf( hudData.dam.str, HUD_STR_SZ - 1, "%3d",
                (i < 0) ? 0 : i );
      hudData.dam.str[HUD_STR_SZ - 1] = 0;

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
      
      snprintf( hudData.fuel.str, HUD_STR_SZ - 1, "%3d",
                (i < 0) ? 0 : i );
      hudData.fuel.str[HUD_STR_SZ - 1] = 0;

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
      hudData.alloc.str[0] = 0;

      if ( i == 0 )
        appstr( "**", hudData.alloc.str );
      else
        appint( i, hudData.alloc.str );

      appchr( '/', hudData.alloc.str );

      if ( j == 0 )
        appstr( "**", hudData.alloc.str );
      else
        appint( j, hudData.alloc.str );

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

      snprintf( hudData.etemp.str, HUD_STR_SZ - 1, "%3d",
                (i < 0) ? 0 : i );
      hudData.etemp.str[HUD_STR_SZ - 1] = 0;

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

      snprintf( hudData.wtemp.str, HUD_STR_SZ - 1, "%3d",
                (i < 0) ? 0 : i );
      hudData.wtemp.str[HUD_STR_SZ - 1] = 0;

      hudData.wtemp.temp  = wtemp;
      hudData.wtemp.overl = wOverl;

    }

  return;
}

void hudSetTow(int snum)
{
  int i = Ships[snum].towedby;

  if ( i == 0 )
    i = -Ships[snum].towing;

  if (i != hudData.tow.towstat)
    {
      if ( i == 0 )
        {
          hudData.tow.str[0] = 0;
        }
      else if ( i < 0 )
        {
          strcpy(hudData.tow.str, "towing ");
          appship( -i, hudData.tow.str );
        }
      else if ( i > 0 )
        {
          strcpy(hudData.tow.str, "towed by ");
          appship( i, hudData.tow.str );
        }

      hudData.tow.towstat = i;
    }

  return;
}


void hudSetArmies(int snum)
{
  int i = Ships[snum].armies;
  
  if (i != hudData.armies.armies)
    {
      if (i == 0)
        hudData.armies.str[0] = 0;
      else
        {
          snprintf( hudData.armies.str, HUD_STR_SZ - 1, "%2d armies", i );
          hudData.armies.str[HUD_STR_SZ - 1] = 0;
        }

      hudData.armies.armies = i;
    }

  return;
}

void hudSetRobotAction(int snum)
{
  int i = Ships[snum].action;
  
  if (i != hudData.raction.action)
    {
      robstr( i, hudData.raction.str );

      hudData.raction.action = i;
    }

  return;
}

void hudSetDestruct(int snum)
{
  int i = max( 0, Ships[snum].sdfuse );

  if (i != hudData.destruct.fuse)
    {
      if (!i)
        hudData.destruct.str[0] = 0;
      else
        {
          snprintf( hudData.destruct.str, HUD_STR_SZ - 1, 
                    "DESTRUCT MINUS %2d", i );
          hudData.destruct.str[HUD_STR_SZ - 1] = 0;
        }

      hudData.destruct.fuse = i;
    }

  return;
}


/* return a buffer of spaces for padding */
static char *_padstr(int l)
{
  static char padding[HUD_PROMPT_SZ];

  if (l >= HUD_PROMPT_SZ)
    l = HUD_PROMPT_SZ - 1;

  if (l < 0)
    l = 0;
  
  if (l > 0)
    memset(padding, ' ', l);

  padding[l - 1] = 0;

  return padding;
}


void hudSetPrompt(int line, char *prompt, int pcolor, 
                  char *buf, int color)
{
  char *str;
  char *pstr;
  int pl;
  char *bstr;
  int bl;
  const int maxwidth = 80;

  switch(line)
    {
    case MSG_LIN1:
      str = hudData.p1.str;
      break;

    case MSG_LIN2:
      str = hudData.p2.str;
      break;

    case MSG_MSG:
    default:
      color = InfoColor;
      str = hudData.msg.str;
      break;
    }

  if (!buf && !prompt)
    {
      str[0] = 0;
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

  snprintf(str, HUD_PROMPT_SZ - 1,
           "#%d#%s#%d#%s%s",
           pcolor, pstr, color, bstr, _padstr(maxwidth - (pl + bl)));
  str[HUD_PROMPT_SZ - 1] = 0; 

  return;
}

/* a shortcut */
void hudClearPrompt(int line)
{
  hudSetPrompt(line, NULL, NoColor, NULL, NoColor);

  return;
}


void hudSetInfoFiringAngle(real blastang)
{
  int i = (int)blastang;

  if (blastang != hudData.info.lastblast)
    { 
      snprintf(hudData.info.lastblaststr, HUD_INFO_STR_SZ - 1,
               "#%d#FA:#%d#%3d",
               MagentaColor,
               InfoColor,
               (i >= 0) ? i : 0);
      
      hudData.info.lastblaststr[HUD_INFO_STR_SZ - 1] = 0;

      hudData.info.lastblast = blastang;
    }
  
  return;
}

/* utility - based on cached data, build the proper target info string in
 *  the lasttadstr string.
 */
static void _updateTargetInfoString(void)
{
  if (!hudData.info.lasttarget)
    hudData.info.lasttadstr[0] = 0;
  else
    snprintf(hudData.info.lasttadstr, HUD_INFO_STR_SZ - 1,
             "#%d#TA/D:#2%d#%3s#%d#:#%d#%3d#%d#/#%d#%5d",
             MagentaColor,
             SpecialColor,
             hudData.info.lasttargetstr,
             MagentaColor,
             InfoColor,
             hudData.info.lastang,
             MagentaColor,
             InfoColor,
             hudData.info.lastdist);
  
  return;
}

void hudSetInfoTarget(int tnum)
{
  /* < 0 = planet number, > 0 ship number, 0 = no target */
  if (tnum != hudData.info.lasttarget)
    {
      hudData.info.lasttargetstr[0] = 0;
      if (tnum)
        {
          if (tnum > 0)
            appship( tnum, hudData.info.lasttargetstr );
          else if (tnum < 0)
            {                   /* planet, just need 3 chars */
              hudData.info.lasttargetstr[0] = Planets[-tnum].name[0];
              hudData.info.lasttargetstr[1] = Planets[-tnum].name[1];
              hudData.info.lasttargetstr[2] = Planets[-tnum].name[2];
              hudData.info.lasttargetstr[3] = 0;
            }

          _updateTargetInfoString();
        }

      hudData.info.lasttarget = tnum;
    }

  return;
}

void hudSetInfoTargetAngle(int ang)
{
  if (ang != hudData.info.lastang)
    {
      hudData.info.lastang = ang;

      _updateTargetInfoString();
    }

  return;
}
      
void hudSetInfoTargetDist(int tdist)
{
  if (tdist != hudData.info.lastdist)
    {
      hudData.info.lastdist = tdist;

      _updateTargetInfoString();
    }

  return;
}
      
