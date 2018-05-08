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
#include "format.h"

#include "global.h"
#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "conf.h"
#include "color.h"

#include "ui.h"
#include "hud.h"

#include "conqutil.h"
#include "conqlb.h"
#include "cprintf.h"

#include "info.h"


// get info on a planet and output it if doOutput is true
void infoPlanet( const std::string& str, int pnum, int snum, bool doOutput )
{
    int godlike = false, canscan = false;
    std::string buf;
    real x, y;

    /* Check range of the passed planet number. */
    if ( pnum < 0 || pnum >= cbLimits.maxPlanets() )
    {
        if (doOutput)
        {
            uiPutMsg( "No such planet.", MSG_LIN1 );
            uiPutMsg( "", MSG_LIN2);
            hudSetInfoTarget(-1, false);
        }
        utLog("_infoplanet: Called with invalid pnum (%d).",
              pnum );
        return;
    }

    if (snum < 0 || snum >= cbLimits.maxShips())
        godlike = true;

    /* In some cases, report hostilities. */
    std::string buf2;

    if ( ! godlike )
        if ( cbPlanets[pnum].scanned[cbShips[snum].team]
             && clbSPWar( snum, pnum ) )
            buf2 = " (hostile)";

    if ( godlike )
    {
        x = 0.0;
        y = 0.0;
    }
    else
    {
        x = cbShips[snum].x;
        y = cbShips[snum].y;
    }

    Context.lasttdist =
        iround(dist( x, y, cbPlanets[pnum].x, cbPlanets[pnum].y));
    Context.lasttang =
        iround(utAngle( x, y, cbPlanets[pnum].x, cbPlanets[pnum].y ));

    std::string tmpstr;
    if (UserConf.DoETAStats && !godlike)
    {
        if (cbShips[snum].warp > 0.0)
            tmpstr = fmt::format(", ETA {}",
                                 clbETAStr(cbShips[snum].warp,
                                           Context.lasttdist));
    }

    buf = fmt::format("{}{}, a {}{}, range {}, direction {}{}",
                      str,
                      cbPlanets[pnum].name,
                      cbConqInfo->ptname[cbPlanets[pnum].type],
                      buf2,
                      Context.lasttdist,
                      Context.lasttang,
                      tmpstr);

    /* save for the hudInfo, only first 3 characters */
    Context.lastInfoTarget = std::string(cbPlanets[pnum].name).substr(0, 3);
    hudSetInfoTarget(pnum, false);

    if ( godlike )
        canscan = true;
    else
        canscan = cbPlanets[pnum].scanned[cbShips[snum].team];

    buf2.clear();
    if ( cbPlanets[pnum].type != PLANET_SUN
         && cbPlanets[pnum].type != PLANET_MOON )
    {
        if ( ! canscan )
            buf2 = "with unknown occupational forces";
        else
	{
            int i = cbPlanets[pnum].armies;

            if ( i == 0 )
	    {
                if ( cbPlanets[pnum].uninhabtime > 0 )
                    buf2 = fmt::format("uninhabitable for {} more minutes",
                                       cbPlanets[pnum].uninhabtime);
                else
                    buf2 = "with NO armies";
	    }
            else
	    {
                buf2 = fmt::format("with {} {} arm", i,
                                   cbTeams[cbPlanets[pnum].team].name);
                if ( i == 1 )
                    buf2 += "y";
                else
                    buf2 += "ies";
	    }
	}

        /* Now see if we can tell about coup time. */
        if ( godlike )
            canscan = false;	/* GOD can use teaminfo instead */
        else
            canscan = ( pnum == cbTeams[cbShips[snum].team].homeplanet &&
                        TEAM_COUPINFO(cbShips[snum].team) );
        if ( canscan )
	{
            int j = cbTeams[cbShips[snum].team].couptime;
            if ( j > 0 )
	    {
                if ( !buf2.empty() )
                    buf2 += ", ";
                buf += std::to_string(j);
                buf2 += " minutes until coup time";
	    }
	}
    }

    if ( buf2.empty() )
    {
        buf += '.';
    }
    else
    {
        buf += ',';
        buf2 += '.';
    }

    if (doOutput)
    {
        /* Now output the info. Break the stuff in buf across two lines */
        /*  (if necessary) and force the stuff in buf2 (the number of */
        /*  armies for planets) to be all on the second line. */
        int lim = 75;             /* desired maximum length */
        if ( buf.size() <= lim )
        {
            /* The first part is small enough. */
            uiPutMsg( buf, MSG_LIN1 );
            uiPutMsg( buf2, MSG_LIN2 );
        }
        else
        {
            // we need to slit the extra stuff off of buf, and prepend
            // it to buf2.

            // a space is guaranteed to be present somewhere...
            size_t loc = buf.rfind(' ', lim);
            std::string remaining = buf.substr(loc + 1);

            // now erase from buf from loc to end
            buf.erase(loc);

            buf2 = fmt::format("{}{}", remaining, buf2);
            uiPutMsg( buf, MSG_LIN1 );
            uiPutMsg( buf2, MSG_LIN2 );
        }
    }

    return;
}

