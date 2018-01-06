/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#include "c_defs.h"

#include "conqdef.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"
#include "conqai.h"

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
    hudData.etemp.overl      = false;

    hudData.wtemp.temp       = -1.0;
    hudData.wtemp.color      = GreenLevelColor;
    hudData.wtemp.overl      = false;

    hudData.tow.towstat      = false;
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
    hudData.info.lasttarget  = -1;
    hudData.info.lasttargetstr[0] = 0;
    hudSetInfoFiringAngle(0);

    return;
}

/* These routines assume that snum is always valid, so make sure it is. */

/* set hud warp data to current, if it's changed */
void hudSetWarp(int snum)
{
    if ( cbShips[snum].warp != hudData.warp.warp )
    {
        hudData.warp.warp = cbShips[snum].warp;
        if (hudData.warp.warp >= 0)
            snprintf(hudData.warp.str, HUD_STR_SZ, "%2.1f",
                     hudData.warp.warp );
        else
            utStrncpy(hudData.warp.str, "Orbiting", HUD_STR_SZ);

        /* set the right sound effects */
        setWarp(hudData.warp.warp);
    }

    return;
}


void hudSetHeading(int snum)
{
    int i = round( cbShips[snum].head );

    if ( i != hudData.heading.head)
    {
        if ( cbShips[snum].lock == LOCK_PLANET
             && cbShips[snum].lockDetail < cbLimits.maxPlanets())
        {                       /* just the first 3 characters if locked */
            hudData.heading.str[0] = cbPlanets[cbShips[snum].lockDetail].name[0];
            hudData.heading.str[1] = cbPlanets[cbShips[snum].lockDetail].name[1];
            hudData.heading.str[2] = cbPlanets[cbShips[snum].lockDetail].name[2];
            hudData.heading.str[3] = 0;
        }
        else
        {
            snprintf( hudData.heading.str, HUD_STR_SZ, "%3d", i );
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
            utAppendShip(hudData.aStat.str, asnum) ;

        if (SCLOAKED(asnum))
            strcat(hudData.aStat.str, " (CLOAKED)");

        hudData.aStat.str[HUD_STR_SZ - 1] = 0;

        hudData.aStat.alertLevel = astatus;
        hudData.aStat.aShip      = asnum;
    }

    return;

}

void hudSetKills(int snum)
{
    real x = (cbShips[snum].kills + cbShips[snum].strkills);

    if ( x != hudData.kills.kills )
    {
        snprintf( hudData.kills.str, HUD_STR_SZ, "%0.1f", oneplace(x) );
        hudData.kills.kills = x;
    }

    return;
}


void hudSetShields(int snum, int *dobeep)
{
    int i, k;

    if (cbShips[snum].shields < hudData.sh.shields)
        *dobeep = true;

    i = k = round( cbShips[snum].shields );

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

        snprintf(hudData.sh.str, HUD_STR_SZ, "%3d", (i < 0) ? 0 : i);
    }

    return;
}

void hudSetDamage(int snum, real *lastdamage)
{
    real r = cbShips[snum].damage;
    int i;

    *lastdamage = hudData.dam.damage;

    if ( r != hudData.dam.damage )
    {
        i = round(r);
        if (i <= HUD_HULL_ALRT)
            hudData.dam.color = GreenLevelColor;
        else if (i <= HUD_HULL_WARN)
            hudData.dam.color = YellowLevelColor;
        else
            hudData.dam.color = RedLevelColor;

        snprintf( hudData.dam.str, HUD_STR_SZ, "%3d",
                  (i < 0) ? 0 : i );

        hudData.dam.damage = r;
    }

    return;
}

void hudSetFuel(int snum)
{
    real r = cbShips[snum].fuel;
    int i;

    if ( r != hudData.fuel.fuel )
    {
        i = round( r );

        if (i >= HUD_F_ALRT)
            hudData.fuel.color = GreenLevelColor;
        else if (i >= HUD_F_WARN)
            hudData.fuel.color = YellowLevelColor;
        else
            hudData.fuel.color = RedLevelColor;

        snprintf( hudData.fuel.str, HUD_STR_SZ, "%3d",
                  (i < 0) ? 0 : i );

        hudData.fuel.fuel = r;
    }


    return;
}

void hudSetAlloc(int snum)
{
    int i = cbShips[snum].weapalloc;
    int j = cbShips[snum].engalloc;

    if ( cbShips[snum].wfuse > 0 )
        i = 0;
    if ( cbShips[snum].efuse > 0 )
        j = 0;

    if ( i != hudData.alloc.walloc || j != hudData.alloc.ealloc )
    {
        hudData.alloc.str[0] = 0;

        if ( i == 0 )
            strcat(hudData.alloc.str , "**") ;
        else
            utAppendInt(hudData.alloc.str , i) ;

        utAppendChar(hudData.alloc.str , '/') ;

        if ( j == 0 )
            strcat(hudData.alloc.str , "**") ;
        else
            utAppendInt(hudData.alloc.str , j) ;

        hudData.alloc.walloc  = i;
        hudData.alloc.ealloc  = j;
    }

    return;
}

void hudSetTemps(int snum)
{
    real etemp = cbShips[snum].etemp;
    real wtemp = cbShips[snum].wtemp;
    int eOverl, wOverl;
    int i;

    if (etemp > 100.0)
        etemp = 100.0;
    if (wtemp > 100.0)
        wtemp = 100.0;

    if (cbShips[snum].efuse > 0)
        eOverl = true;
    else
        eOverl = false;

    if (cbShips[snum].wfuse > 0)
        wOverl = true;
    else
        wOverl = false;

    if (etemp != hudData.etemp.temp || eOverl != hudData.etemp.overl)
    {
        i = round( etemp );

        if (i >= HUD_E_WARN)
            hudData.etemp.color = RedLevelColor;
        else if (i >= HUD_E_ALRT)
            hudData.etemp.color = YellowLevelColor;
        else
            hudData.etemp.color = GreenLevelColor;

        snprintf( hudData.etemp.str, HUD_STR_SZ, "%3d",
                  (i < 0) ? 0 : i );

        hudData.etemp.temp  = etemp;
        hudData.etemp.overl = eOverl;
    }

    if (wtemp != hudData.wtemp.temp || wOverl != hudData.wtemp.overl)
    {
        i = round( wtemp );

        if (i >= HUD_W_WARN)
            hudData.wtemp.color = RedLevelColor;
        else if (i >= HUD_W_ALRT)
            hudData.wtemp.color = YellowLevelColor;
        else
            hudData.wtemp.color = GreenLevelColor;

        snprintf( hudData.wtemp.str, HUD_STR_SZ, "%3d",
                  (i < 0) ? 0 : i );

        hudData.wtemp.temp  = wtemp;
        hudData.wtemp.overl = wOverl;

    }

    return;
}

void hudSetTow(int snum)
{
    if (STOWING(snum) || STOWEDBY(snum))
    {
        hudData.tow.str[0] = 0;

        if (STOWING(snum))
        {
            utStrncpy(hudData.tow.str, "towing ", HUD_STR_SZ);
            utAppendShip(hudData.tow.str, cbShips[snum].towing);
        }

        if (STOWEDBY(snum))
        {
            if (STOWING(snum))
                utStrncat(hudData.tow.str, ", ", HUD_STR_SZ);

            utStrncat(hudData.tow.str, "towedby ", HUD_STR_SZ);
            utAppendShip(hudData.tow.str, cbShips[snum].towedby);
        }

        hudData.tow.towstat = true;
    }
    else
    {
        hudData.tow.towstat = false;
    }

    return;
}


void hudSetArmies(int snum)
{
    int i = cbShips[snum].armies;

    if (i != hudData.armies.armies)
    {
        if (i == 0)
            hudData.armies.str[0] = 0;
        else
            snprintf( hudData.armies.str, HUD_STR_SZ, "%2d armies", i );

        hudData.armies.armies = i;
    }

    return;
}

void hudSetRobotAction(int snum)
{
    int i = cbShips[snum].action;

    if (i != hudData.raction.action)
    {
        robstr( i, hudData.raction.str );

        hudData.raction.action = i;
    }

    return;
}

void hudSetDestruct(int snum)
{
    int i = max( 0, cbShips[snum].sdfuse );

    if (i != hudData.destruct.fuse)
    {
        if (!i)
            hudData.destruct.str[0] = 0;
        else
            snprintf( hudData.destruct.str, HUD_STR_SZ,
                      "DESTRUCT MINUS %2d", i );

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


void hudSetPrompt(int line, const char *prompt, int pcolor,
                  const char *buf, int color)
{
    char *str;
    const char *pstr;
    int pl;
    const char *bstr;
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

    snprintf(str, HUD_PROMPT_SZ,
             "#%d#%s#%d#%s%s",
             pcolor, pstr, color, bstr, _padstr(maxwidth - (pl + bl)));

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
        snprintf(hudData.info.lastblaststr, HUD_INFO_STR_SZ,
                 "#%d#FA:#%d#%3d",
                 MagentaColor,
                 InfoColor,
                 (i >= 0) ? i : 0);

        hudData.info.lastblast = blastang;
    }

    return;
}

/* utility - based on cached data, build the proper target info string in
 *  the lasttadstr string.
 */
static void _updateTargetInfoString(void)
{
    if (hudData.info.lasttarget < 0)
        hudData.info.lasttadstr[0] = 0;
    else
        snprintf(hudData.info.lasttadstr, HUD_INFO_STR_SZ,
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

void hudSetInfoTarget(int tnum, bool isShip)
{
    if (tnum != hudData.info.lasttarget)
    {
        hudData.info.lasttargetstr[0] = 0;
        if (tnum >= 0)
        {
            if (isShip && tnum < MAXSHIPS)
                utAppendShip(hudData.info.lasttargetstr , tnum) ;
            else if (!isShip && tnum < cbLimits.maxPlanets())
            {                   /* planet, just need 3 chars */
                hudData.info.lasttargetstr[0] = cbPlanets[tnum].name[0];
                hudData.info.lasttargetstr[1] = cbPlanets[tnum].name[1];
                hudData.info.lasttargetstr[2] = cbPlanets[tnum].name[2];
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

/* ship currently being viewed during playback */
void hudSetRecId(char *str)
{
    if (str)
        strcpy(hudData.recId.str, str);
    else
        hudData.recId.str[0] = 0;

    return;
}

/* ship currently being viewed during playback */
void hudSetRecTime(char *str)
{
    if (str)
        strcpy(hudData.recTime.str, str);
    else
        hudData.recTime.str[0] = 0;

    return;
}
