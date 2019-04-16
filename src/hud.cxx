//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "c_defs.h"

#include <string>
#include <algorithm>
#include "fmt/format.h"

#include "conqdef.h"
#include "context.h"
#include "global.h"
#include "color.h"
#include "cb.h"
#include "conqlb.h"
#include "conqutil.h"
#include "conqai.h"

#define NOEXTERN_HUD
#include "hud.h"
#undef  NOEXTERN_HUD

/* init all of the hud data to 'default' values.  This should ensure a
 *  forced update when any of the items are set.
 */
void hudInitData(void)
{
    hudData.warp.warp        = 92.0;     /* "Warp 92 Mr. Sulu." */
    hudData.warp.color       = InfoColor;
    hudData.warp.str.clear();

    hudData.heading.head     = 999;
    hudData.heading.color    = NoColor;
    hudData.heading.str.clear();

    hudData.kills.kills      = -20.0;
    hudData.kills.color      = InfoColor;
    hudData.kills.str.clear();

    hudData.aStat.alertLevel = GREEN_ALERT;
    hudData.aStat.aShip      = -1;
    hudData.aStat.color      = GreenLevelColor;
    hudData.aStat.str.clear();

    hudData.sh.shields       = -9;
    hudData.sh.color         = GreenLevelColor;
    hudData.sh.str.clear();

    hudData.dam.damage       = 999.0;
    hudData.dam.color        = GreenLevelColor;
    hudData.dam.str.clear();

    hudData.fuel.fuel        = -99;
    hudData.fuel.color       = GreenLevelColor;
    hudData.fuel.str.clear();

    hudData.alloc.walloc     = -1;
    hudData.alloc.ealloc     = -1;
    hudData.alloc.color      = InfoColor;
    hudData.alloc.str.clear();

    hudData.etemp.temp       = -1.0;
    hudData.etemp.color      = GreenLevelColor;
    hudData.etemp.overl      = false;
    hudData.etemp.str.clear();

    hudData.wtemp.temp       = -1.0;
    hudData.wtemp.color      = GreenLevelColor;
    hudData.wtemp.overl      = false;
    hudData.wtemp.str.clear();

    hudData.tow.towstat      = false;
    hudData.tow.color        = MagentaColor;
    hudData.tow.str.clear();

    hudData.armies.armies    = -1701;
    hudData.armies.color     = InfoColor;
    hudData.armies.str.clear();

    hudData.raction.action   = 0;
    hudData.raction.color    = InfoColor;
    hudData.raction.str.clear();

    hudData.destruct.fuse    = -1;
    hudData.destruct.color   = RedLevelColor;
    hudData.destruct.str.clear();

    hudData.p1.str.clear();
    hudData.p2.str.clear();
    hudData.msg.str.clear();

    hudData.info.lastblast   = -1;
    hudData.info.lastang     = -1;
    hudData.info.lastdist    = -1;
    hudData.info.lasttarget  = -1;

    hudData.info.lastblaststr.clear();
    hudData.info.lasttargetstr.clear();
    hudData.info.lasttadstr.clear();

    hudSetInfoFiringAngle(0);

    return;
}

/* These routines assume that snum is always valid, so make sure it is. */

// set hud warp data to current, if it's changed
void hudSetWarp(int snum)
{
    if (snum < 0)
        return;

    if ( cbShips[snum].warp != hudData.warp.warp )
    {
        hudData.warp.warp = cbShips[snum].warp;
        if (hudData.warp.warp >= 0)
            hudData.warp.str = fmt::format("{:2.1f}", hudData.warp.warp);
        else
            hudData.warp.str = "Orbiting";
    }

    return;
}


void hudSetHeading(int snum)
{
    if (snum < 0)
        return;

    int i = iround( cbShips[snum].head );

    if ( i != hudData.heading.head)
    {
        if ( cbShips[snum].lock == LOCK_PLANET
             && cbShips[snum].lockDetail < cbLimits.maxPlanets())
        {                       /* just the first 3 characters if locked */
            std::string tmp = cbPlanets[cbShips[snum].lockDetail].name;
            hudData.heading.str = tmp.substr(0, 3);
        }
        else
        {
            hudData.heading.str = fmt::format("{:3d}", i);
        }

        hudData.heading.head = i;
    }

    return;
}

void hudSetAlertStatus(int snum, int asnum, alertLevel_t astatus)
{
    if (snum < 0)
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
                hudData.aStat.str = "RED ALERT ";
            }
            break;

            case RED_ALERT:
            {
                hudData.aStat.color = RedLevelColor;
                hudData.aStat.str = "Alert ";
            }
            break;

            case TORP_ALERT:
            {
                hudData.aStat.color = YellowLevelColor;
                hudData.aStat.str = "Torp Alert ";
            }
            break;

            case YELLOW_ALERT:
            {
                hudData.aStat.color = YellowLevelColor;
                hudData.aStat.str = "Yellow Alert ";
            }
            break;

            case PROXIMITY_ALERT:
            {
                hudData.aStat.color = YellowLevelColor;
                hudData.aStat.str = "Proximity Alert ";
            }
            break;

            case GREEN_ALERT:
            default:
            {
                hudData.aStat.color = GreenLevelColor;
                hudData.aStat.str.clear();
            }
            break;
        }

        if (asnum >= 0)
        {
            utAppendShip(hudData.aStat.str, asnum) ;

            if (SCLOAKED(asnum))
                hudData.aStat.str += " (CLOAKED)";
        }

        hudData.aStat.alertLevel = astatus;
        hudData.aStat.aShip      = asnum;
    }

    return;

}

void hudSetKills(int snum)
{
    if (snum < 0)
        return;

    real x = (cbShips[snum].kills + cbShips[snum].strkills);

    if ( x != hudData.kills.kills )
    {
        hudData.kills.str = fmt::format("{:0.1f}", x );
        hudData.kills.kills = x;
    }

    return;
}


void hudSetShields(int snum, bool *dobeep)
{
    if (snum < 0)
        return;

    int i, k;

    if (cbShips[snum].shields < hudData.sh.shields)
        *dobeep = true;

    i = k = iround( cbShips[snum].shields );

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

        hudData.sh.str = fmt::format("{:3d}", ((i < 0) ? 0 : i));
    }

    return;
}

void hudSetDamage(int snum, real *lastdamage)
{
    if (snum < 0)
        return;

    real r = cbShips[snum].damage;
    int i;

    *lastdamage = hudData.dam.damage;

    if ( r != hudData.dam.damage )
    {
        i = iround(r);
        if (i <= HUD_HULL_ALRT)
            hudData.dam.color = GreenLevelColor;
        else if (i <= HUD_HULL_WARN)
            hudData.dam.color = YellowLevelColor;
        else
            hudData.dam.color = RedLevelColor;

        hudData.dam.str = fmt::format("{:3d}", ((i < 0) ? 0 : i) );

        hudData.dam.damage = r;
    }

    return;
}

void hudSetFuel(int snum)
{
    if (snum < 0)
        return;

    real r = cbShips[snum].fuel;
    int i;

    if ( r != hudData.fuel.fuel )
    {
        i = iround( r );

        if (i >= HUD_F_ALRT)
            hudData.fuel.color = GreenLevelColor;
        else if (i >= HUD_F_WARN)
            hudData.fuel.color = YellowLevelColor;
        else
            hudData.fuel.color = RedLevelColor;

        hudData.fuel.str = fmt::format("{:3d}", ((i < 0) ? 0 : i) );

        hudData.fuel.fuel = r;
    }


    return;
}

void hudSetAlloc(int snum)
{
    if (snum < 0)
        return;

    int i = cbShips[snum].weapalloc;
    int j = cbShips[snum].engalloc;

    if ( cbShips[snum].wfuse > 0 )
        i = 0;
    if ( cbShips[snum].efuse > 0 )
        j = 0;

    if ( i != hudData.alloc.walloc || j != hudData.alloc.ealloc )
    {
        hudData.alloc.str.clear();

        if ( i == 0 )
            hudData.alloc.str += "**";
        else
            hudData.alloc.str += std::to_string(i);

        hudData.alloc.str += '/';

        if ( j == 0 )
            hudData.alloc.str += "**";
        else
            hudData.alloc.str += std::to_string(j);

        hudData.alloc.walloc  = i;
        hudData.alloc.ealloc  = j;
    }

    return;
}

void hudSetTemps(int snum)
{
    if (snum < 0)
        return;

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
        i = iround( etemp );

        if (i >= HUD_E_WARN)
            hudData.etemp.color = RedLevelColor;
        else if (i >= HUD_E_ALRT)
            hudData.etemp.color = YellowLevelColor;
        else
            hudData.etemp.color = GreenLevelColor;

        hudData.etemp.str = fmt::format("{:3d}", ((i < 0) ? 0 : i ));

        hudData.etemp.temp  = etemp;
        hudData.etemp.overl = eOverl;
    }

    if (wtemp != hudData.wtemp.temp || wOverl != hudData.wtemp.overl)
    {
        i = iround( wtemp );

        if (i >= HUD_W_WARN)
            hudData.wtemp.color = RedLevelColor;
        else if (i >= HUD_W_ALRT)
            hudData.wtemp.color = YellowLevelColor;
        else
            hudData.wtemp.color = GreenLevelColor;

        hudData.wtemp.str = fmt::format("{:3d}", ((i < 0) ? 0 : i ));

        hudData.wtemp.temp  = wtemp;
        hudData.wtemp.overl = wOverl;
    }

    return;
}

void hudSetTow(int snum)
{
    if (snum < 0)
        return;

    if (STOWING(snum) || STOWEDBY(snum))
    {
        hudData.tow.str.clear();

        if (STOWING(snum))
        {
            hudData.tow.str = "towing ";
            utAppendShip(hudData.tow.str, cbShips[snum].towing);
        }

        if (STOWEDBY(snum))
        {
            if (STOWING(snum))
                hudData.tow.str += ", ";

            hudData.tow.str += "towedby ";
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
    if (snum < 0)
        return;

    int i = cbShips[snum].armies;

    if (i != hudData.armies.armies)
    {
        if (i == 0)
            hudData.armies.str.clear();
        else
            hudData.armies.str = fmt::format("{:2d} armies", i );

        hudData.armies.armies = i;
    }

    return;
}

void hudSetRobotAction(int snum)
{
    if (snum < 0)
        return;

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
    if (snum < 0)
        return;

    int i = std::max( 0, cbShips[snum].sdfuse );

    if (i != hudData.destruct.fuse)
    {
        if (!i)
            hudData.destruct.str.clear();
        else
            hudData.destruct.str = fmt::format("DESTRUCT MINUS {:02d}", i );

        hudData.destruct.fuse = i;
    }

    return;
}

void hudSetPrompt(int line, const std::string& prompt, int pcolor,
                  const std::string& buf, int color)
{
    std::string str;
    std::string pstr;
    int pl;
    std::string bstr;
    int bl;
    bool clearString = false;

    if (buf.empty() && prompt.empty())
        clearString = true;

    if (buf.empty())
    {
        bl = 0;
        bstr = "";
    }
    else
    {
        bl = buf.size();
        bstr = buf;
    }

    if (prompt.empty())
    {
        pl = 0;
        pstr = "";
    }
    else
    {
        pl = prompt.size();
        pstr = prompt;
    }

    if (clearString)
        str.clear();
    else
    {
        if (line != MSG_LIN1 && line != MSG_LIN2)
            color = InfoColor;

        str = fmt::format("#{:d}#{:s}#{:d}#{:s}{:s}",
                          pcolor, pstr, color, bstr,
                          std::string((Context.maxcol - (pl + bl)), ' '));
    }

    switch(line)
    {
    case MSG_LIN1:
        hudData.p1.str = str;
        break;

    case MSG_LIN2:
        hudData.p2.str = str;
        break;

    case MSG_MSG:
    default:
        hudData.msg.str = str;
        break;
    }

    return;
}

/* a shortcut */
void hudClearPrompt(int line)
{
    hudSetPrompt(line, "", NoColor, "", NoColor);

    return;
}


void hudSetInfoFiringAngle(real blastang)
{
    int i = (int)blastang;

    if (blastang != hudData.info.lastblast)
    {
        hudData.info.lastblaststr = fmt::format("#{:d}#FA:#{:d}#{:3d}",
                                                MagentaColor,
                                                InfoColor,
                                                ((i >= 0) ? i : 0));

        hudData.info.lastblast = blastang;
    }

    return;
}

/* utility - based on cached data, build the proper target info string in
 *  the lasttadstr string.
 */
static void _updateTargetInfoString(void)
{
    if ( !(hudData.info.lasttarget >= 0 && !Context.lastInfoStr.empty()) )
        hudData.info.lasttadstr.clear();
    else
        hudData.info.lasttadstr =
            fmt::format(
                "#{:d}#TA/D:#{:d}#{:>3s} #{:d}#{:3d}#{:d}#/#{:d}#{:5d}",
                MagentaColor,
                NoColor,
                hudData.info.lasttargetstr,
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
        hudData.info.lasttargetstr.clear();
        if (tnum >= 0)
        {
            if (isShip && tnum < cbLimits.maxShips())
                utAppendShip(hudData.info.lasttargetstr, tnum) ;
            else if (!isShip && tnum < cbLimits.maxPlanets())
            {                   /* planet, just need 3 chars */
                std::string tmp = cbPlanets[tnum].name;
                hudData.info.lasttargetstr = tmp.substr(0, 3);
            }
        }

        hudData.info.lasttarget = tnum;
        _updateTargetInfoString();
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
void hudSetRecId(const std::string& str)
{
    hudData.recId.str = str;

    return;
}

/* ship currently being viewed during playback */
void hudSetRecTime(const std::string& str)
{
    hudData.recTime.str = str;

    return;
}
