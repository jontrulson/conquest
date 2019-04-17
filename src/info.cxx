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
#include "fmt/format.h"

#include "global.h"
#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "conf.h"
#include "color.h"
#include "rndlb.h"

#include "ui.h"
#include "hud.h"

#include "conqutil.h"
#include "conqlb.h"
#include "cprintf.h"

#include "info.h"

//  info - do an info command
void info( const std::string& inbuf, char ch, bool doOutput )
{
    // default to \r
    static char lastInfoTermChar = TERM_NORMAL;

    // an snum < 0 indicates godlike (conqoper)
    int snum = Context.snum;

    if ( ch == TERM_ABORT )
    {
        uiPutMsg("", MSG_LIN1);
        hudSetInfoTarget(-1, false);
        return;
    }

    // special case: ch == 0, means to use the last used term char.
    // This is used by the TAD display to "automate" info commands.
    if (ch == 0)
        ch = lastInfoTermChar;
    else
        lastInfoTermChar = ch;

    int extra = ( ch == TERM_EXTRA );

    std::string tmpBuf = inbuf;

    /* Default to what we did last time. */
    if ( tmpBuf.empty() )
    {
        tmpBuf = Context.lastInfoStr;
        if ( tmpBuf.empty() )
	{
            uiPutMsg("", MSG_LIN1);
            hudSetInfoTarget(-1, false);
            return;
	}
    }
    else
        Context.lastInfoStr = tmpBuf;

    int count;
    InfoNear what;
    InfoSpecial token;
    int j, sorpnum, xsorpnum;
    if ( utIsSpecial( tmpBuf, &what, &token, &count ) )
    {
        if ( ! clbFindSpecial( snum, token, count, &sorpnum, &xsorpnum ) )
            what = NEAR_NONE;
        else if ( extra )
	{
            if ( xsorpnum == -1 )
                what = NEAR_NONE;
            else
                sorpnum = xsorpnum;
	}

        if ( what == NEAR_SHIP )
            infoShip( sorpnum, snum, doOutput );
        else if ( what == NEAR_PLANET )
            infoPlanet( "", sorpnum, snum, doOutput );
        else
        {
            if (doOutput)
                uiPutMsg( "Not found.", MSG_LIN2 );
            hudSetInfoTarget(-1, false);
        }
    }
    else if ( tmpBuf.size() > 1 && tmpBuf[0] == 's' && ::isdigit(tmpBuf[1]) )
    {
        utSafeCToI( &j, tmpBuf, 1 );		/* ignore status */
        infoShip( j, snum, doOutput );
    }
    else if (utIsDigits(tmpBuf))
    {
        utSafeCToI( &j, tmpBuf);		/* ignore status */
        infoShip( j, snum, doOutput );
    }
    else if ( clbPlanetMatch( tmpBuf, &j, ((snum < 0) ? true : false) ) )
        infoPlanet( "", j, snum, doOutput );
    else
    {
        if (doOutput)
            uiPutMsg( "I don't understand.", MSG_LIN2 );
        hudSetInfoTarget(-1, false);
        return;
    }
}


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
        buf += ", ";
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

void infoShip( int snum, int scanner, bool doOutput )
{
    int status;
    real x, y, dis, kills, appx, appy;
    bool godlike = false, canscan = false;
    std::string cbuf;

    godlike = (scanner < 0 || scanner >= cbLimits.maxShips());

    if (doOutput)
        uiPutMsg("", MSG_LIN1);
    if ( snum < 0 || snum >= cbLimits.maxShips() )
    {
        if (doOutput)
            uiPutMsg( "No such ship.", MSG_LIN1 );
        hudSetInfoTarget(-1, false);
        return;
    }
    status = cbShips[snum].status;
    if ( ! godlike && status != SS_LIVE )
    {
        if (doOutput)
            uiPutMsg( "Not found.", MSG_LIN1 );
        hudSetInfoTarget(-1, false);
        return;
    }

    cbuf.clear();
    Context.lastInfoTarget.clear();
    utAppendShip(cbuf, snum) ;
    /* save for hudInfo */
    Context.lastInfoTarget = cbuf;
    hudSetInfoTarget(snum, true);

    if ( snum == scanner )
    {
        /* Silly Captain... */
        cbuf += ": That's us, silly!";
        if (doOutput)
            uiPutMsg( cbuf, MSG_LIN1 );
        hudSetInfoTarget(-1, false);
        return;
    }
    /* Scan another ship. */
    if ( godlike )
    {
        x = 0.0;
        y = 0.0;
    }
    else
    {
        x = cbShips[scanner].x;
        y = cbShips[scanner].y;
    }

    if ( SCLOAKED(snum) )
    {
        if (godlike)
	{
            appx = rndnor(cbShips[snum].x, CLOAK_SMEAR_DIST);
            appy = rndnor(cbShips[snum].y, CLOAK_SMEAR_DIST);
	}
        else			/* client */
	{			/* for clients, these have already been
				   smeared */
            appx = cbShips[snum].x;
            appy = cbShips[snum].y;
	}
    }
    else
    {
        appx = cbShips[snum].x;
        appy = cbShips[snum].y;
    }
    dis = dist( x, y, appx, appy );
    if ( godlike )
        canscan = true;
    else
    {
        /* Decide if we can do an acurate scan. */
        canscan = ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) ||
                    ( (cbShips[snum].scanned[ cbShips[scanner].team] > 0)
                      && ! selfwar(scanner) ) );
    }

    cbuf += ": ";
    if ( cbShips[snum].alias[0] != 0 )
    {
        cbuf += cbShips[snum].alias;
        cbuf += ", ";
    }
    kills = (cbShips[snum].kills + cbShips[snum].strkills);
    if ( kills == 0.0 )
        cbuf += "no";
    else
        cbuf += fmt::format("{:.1f}", kills);

    cbuf += " kill";
    if ( kills != 1.0 )
        cbuf += 's';

    if ( SCLOAKED(snum) && ( godlike || SSCANDIST(snum) ) )
        cbuf += " (CLOAKED) ";
    else
        cbuf += ", ";

    cbuf += "a ";
    cbuf += cbShipTypes[cbShips[snum].shiptype].name;
    cbuf += ", ";

    if ( godlike )
    {
        utAppendShipStatus(cbuf , status) ;
        cbuf += '.';
    }
    else
    {
        if ( cbShips[snum].war[cbShips[scanner].team] )
            cbuf += "at WAR.";
        else
            cbuf += "at peace.";
    }

    if (doOutput)
        uiPutMsg( cbuf, MSG_LIN1 );

    if ( ! SCLOAKED(snum) || cbShips[snum].warp != 0.0 )
    {
        Context.lasttdist = iround( dis ); /* save these puppies for hud info */
        Context.lasttang = iround( utAngle( x, y, appx, appy ) );

        cbuf = fmt::format("Range {0}, direction {1}",
                           Context.lasttdist, Context.lasttang);

        if (UserConf.DoETAStats && !godlike)
	{
            /* ETA related vars */
            real pwarp, diffdis, close_rate;
            time_t difftime, curtime;
            static time_t oldtime = 0;
            static int oldsnum = 0;
            static real avgclose_rate, olddis = 0.0, oldclose_rate = 0.0;

            if (cbShips[scanner].warp > 0.0 || cbShips[snum].warp > 0.0)
	    {
                curtime = time(0);

                if (snum == oldsnum)
		{		/* then we can get better eta
				   by calculating closure rate and
				   extrapolate from there the apparent warp
				   giving a better estimate. */
                    difftime = curtime - oldtime;

                    /* we still need to compute diffdis  */
                    diffdis = olddis - dis;
                    olddis = dis;

                    if (difftime <= 0)
		    {		/* not enough time passed for a guess
				   use last closerate, and don't set
				   oldtime so it will eventually work */
                        close_rate = oldclose_rate;
		    }
                    else
		    {		/* we can make an estimate of closure rate in
				   MM's per second */
                        oldtime = curtime;

                        close_rate = diffdis / (real) difftime;
		    }

                    /* give a 'smoother' est. by avg'ing with
                       last close rate.*/
                    avgclose_rate = (close_rate + oldclose_rate) / 2.0;
                    oldclose_rate = close_rate;

#ifdef DEBUG_ETA
                    utLog("_infoship: close_rate(%.1f) = diffdis(%.1f) / difftime(%d), avgclose_rate = %.1f",
                          close_rate,
                          diffdis,
                          difftime,
                          avgclose_rate);
#endif

                    if (avgclose_rate <= 0.0)
		    {		/* dist is increasing or no change,
				   - can't ever catchup = ETA never */
                        cbuf += ", ETA ";
                        cbuf += clbETAStr(0.0, dis);
		    }
                    else
		    {		/* we are indeed closing... */

			/* calc psuedo-warp */
                        pwarp = (avgclose_rate / (real) MM_PER_SEC_PER_WARP);

#ifdef DEBUG_ETA
                        utLog("_infoship:\tdis(%.1f) pwarp(%.1f) = (close_rate(%.1f) / MM_PER_SEC_PER_WARP(%.1f)", dis, pwarp, close_rate, MM_PER_SEC_PER_WARP);
#endif

                        cbuf += ", ETA ";
                        cbuf += clbETAStr(pwarp, dis);
		    }
		}
                else
		{
                    /* scanning a new ship - assume ships heading
                     * directly at each other */

                    /* init old* vars */
                    oldtime = curtime;
                    oldsnum = snum;
                    olddis = dis;

                    pwarp =
                        (((cbShips[scanner].warp > 0.0) ?
                          cbShips[scanner].warp :
                          0.0) +
                         ((cbShips[snum].warp > 0.0) ?
                          cbShips[snum].warp
                          : 0.0));

                    cbuf += ", ETA ";
                    cbuf += clbETAStr(pwarp, dis);
		}
	    }
	} /* if do ETA stats */
    }
    else				/* else cloaked and at w0 */
    {
        Context.lasttdist = Context.lasttang = 0;
        cbuf.clear();
    }

    if ( canscan )
    {
        if ( !cbuf.empty() )
            cbuf += ", ";
        cbuf += "shields ";
        if ( SSHUP(snum) && ! SREPAIR(snum) )
            cbuf += std::to_string(iround( cbShips[snum].shields ));
        else
            cbuf += "DOWN";

        int i = iround( cbShips[snum].damage );
        if ( i > 0 )
	{
            if ( !cbuf.empty() )
                cbuf += ", ";
            cbuf += fmt::format("damage {}", i );
	}
        i = cbShips[snum].armies;
        if ( i > 0 )
	{
            cbuf += fmt::format(", with {} arm", i);
            if ( i == 1 )
	    {
                cbuf += 'y';
	    }
            else
                cbuf += "ies";
	}
    }
    if ( !cbuf.empty() )
    {
        cbuf += '.';
        if (doOutput)
            uiPutMsg( cbuf, MSG_LIN2 );
    }

    return;
}

