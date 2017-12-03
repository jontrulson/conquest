#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "conqlb.h"
#include "conqutil.h"
#include "rndlb.h"
#include "conqunix.h"

#include "conf.h"
#include "global.h"

#include "conqinit.h"


/*  initplanets - initialize the planets based on cqiPlanets[] data */
void cqiInitPlanets(void)
{
    int i, j, k;

    /* Hump the lockword. */
    cbUnlock(&cbConqInfo->lockword);
    cbLock(&cbConqInfo->lockword);

    if (!cqiGlobal)
    {
        utLog("WARNING: cqiInitPlanets(): cqiGlobal is NULL, can't init planets");
        cbUnlock(&cbConqInfo->lockword);
        return;
    }

    if (cqiGlobal->maxplanets != MAXPLANETS)
    {                           /* bummer - should not happen */
        utLog("ERROR: cqiInitPlanets: cqiGlobal->maxplanets(%d) != MAXPLANETS(%d)\n",
              cqiGlobal->maxplanets, MAXPLANETS);

        cbUnlock(&cbConqInfo->lockword);
        return;
    }

    for (i=0; i<MAXPLANETS; i++)
    {                           /* init all of the planets. */
        utStrncpy(cbPlanets[i].name, cqiPlanets[i].name, MAXPLANETNAME);

        cbPlanets[i].type = cqiPlanets[i].ptype;
        cbPlanets[i].primary = cqiPlanets[i].primary;

        if (cqiPlanets[i].visible)
            PFSET(i, PLAN_F_VISIBLE);
        else
            PFCLR(i, PLAN_F_VISIBLE);

        cbPlanets[i].team = cqiPlanets[i].pteam;

        // set core and homeplanet flags
        if (cqiPlanets[i].core)
            PFSET(i, PLAN_F_CORE);
        else
            PFCLR(i, PLAN_F_CORE);

        if (cqiPlanets[i].homeplanet)
            PFSET(i, PLAN_F_HOMEPLANET);
        else
            PFCLR(i, PLAN_F_HOMEPLANET);

        // set the defendteam if a homeplanet
        if (PHOMEPLANET(i) && cbPlanets[i].team < NUMPLAYERTEAMS)
            cbPlanets[i].defendteam = cbPlanets[i].team;
        else
            cbPlanets[i].defendteam = TEAM_NOTEAM; // no robot rescuers :(

        /* for armies, we use what is specified */
        cbPlanets[i].armies = cqiPlanets[i].armies;

        cbPlanets[i].uninhabtime = 0;

        for ( j = 0; j < NUMPLAYERTEAMS; j++ )
            cbPlanets[i].scanned[j] = FALSE;

        /* we will need to defer setting up planet's team scan until
           we have identified the team's homeplanets... */
        for ( j = 0; j < NUMPLAYERTEAMS; j++ )
        {
            cbTeams[j].couptime = 0; /* time left to coup starts at zero. */
            cbTeams[j].coupinfo = FALSE; /* don't know coup time */
        }

        cbPlanets[i].size = cqiPlanets[i].size;
        cbPlanets[i].x = cqiPlanets[i].xcoord;
        cbPlanets[i].y = cqiPlanets[i].ycoord;
        cbPlanets[i].orbrad = cqiPlanets[i].radius;
        cbPlanets[i].orbvel = cqiPlanets[i].velocity;

        if (cqiPlanets[i].angle < 0)
            cbPlanets[i].orbang = rnduni( 0.0, 360.0 ); /* randomly choose one */
        else
            cbPlanets[i].orbang = utMod360(cqiPlanets[i].angle);
    } /* for */

    // get all the homeplanets.  We require that each team have at
    // least 1 homeplanet.  We use the first one found for a team to
    // initialize the team's homeplanet member.
    int F = 0, K = 0, R = 0, O = 0; /* homeplanet count per team */

    for (i=0; i<MAXPLANETS; i++)
    {
        if (cqiPlanets[i].homeplanet)
        {
            switch (cqiPlanets[i].pteam)
            {
            case TEAM_FEDERATION:
                F++;
                if (F == 1)
                    cbTeams[TEAM_FEDERATION].homeplanet = i;
                break;
            case TEAM_ROMULAN:
                R++;
                if (R == 1)
                    cbTeams[TEAM_ROMULAN].homeplanet = i;
                break;
            case TEAM_KLINGON:
                K++;
                if (K == 1)
                    cbTeams[TEAM_KLINGON].homeplanet = i;
                break;
            case TEAM_ORION:
                O++;
                if (O == 1)
                    cbTeams[TEAM_ORION].homeplanet = i;
                break;

            default:
                break;
            }
        }
    }

    // this should never happen -- each team MUST have at least one
    // homeplanet. cqiValidatePlanets() should have failed before this
    // code is ever run.  But just in case, leave an error...
    if (F == 0 || R == 0 || K ==0 || O == 0)
    {
        utLog("ERROR: cqiInitPlanets: homeplanet count for each team is "
              "not > 0! (f=%d o=%d k=%d r=%d)",
              F, O, K, R);
        // continue, but homeplanets will be screwed up, and entering
        // systems will fail, robot defenses will fail, coups won't
        // work, etc, etc...
    }

    /* Place the planets in their proper orbits. */
    /* careful about planets that orbit themselves (planet == primary)  */
    for ( i = MAXPLANETS - 1; i >= 0; i-- )
    {
        if ( cbPlanets[i].primary != i ) // not stationary
        {
            cbPlanets[i].x = cbPlanets[cbPlanets[i].primary].x +
                cbPlanets[i].orbrad * cosd(cbPlanets[i].orbang);
            cbPlanets[i].y = cbPlanets[cbPlanets[i].primary].y +
                cbPlanets[i].orbrad * sind(cbPlanets[i].orbang);
        }
    }

    /* now setup the intial scan status for each team's homeplanet(s) */
    for ( k = 0; k < MAXPLANETS; k++ )
    {
        if (PHOMEPLANET(k) && cbPlanets[k].team < NUMPLAYERTEAMS)
            cbPlanets[k].scanned[cbPlanets[k].team] = TRUE;
    }

    /* Un-twiddle the lockword. */
    cbUnlock(&cbConqInfo->lockword);

    /* Protect against a system crash here! */
    upchuck();

    return;

}