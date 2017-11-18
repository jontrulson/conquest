#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
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
    PVUNLOCK(&ConqInfo->lockword);
    PVLOCK(&ConqInfo->lockword);

    if (!cqiGlobal)
    {
        utLog("WARNING: cqiInitPlanets(): cqiGlobal is NULL, can't init planets");
        PVUNLOCK(&ConqInfo->lockword);
        return;
    }

    if (cqiGlobal->maxplanets != MAXPLANETS)
    {                           /* bummer - should not happen */
        utLog("ERROR: cqiInitPlanets: cqiGlobal->maxplanets(%d) != MAXPLANETS(%d)\n",
              cqiGlobal->maxplanets, MAXPLANETS);

        PVUNLOCK(&ConqInfo->lockword);
        return;
    }

    for (i=0; i<MAXPLANETS; i++)
    {                           /* init all of the planets. */
        strncpy(Planets[i].name, cqiPlanets[i].name, MAXPLANETNAME);

        Planets[i].type = cqiPlanets[i].ptype;
        Planets[i].primary = cqiPlanets[i].primary;

        if (cqiPlanets[i].visible)
            PFSET(i, PLAN_F_VISIBLE);
        else
            PFCLR(i, PLAN_F_VISIBLE);

        Planets[i].team = cqiPlanets[i].pteam;

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
        if (PHOMEPLANET(i) && Planets[i].team < NUMPLAYERTEAMS)
            Planets[i].defendteam = Planets[i].team;
        else
            Planets[i].defendteam = TEAM_NOTEAM; // no robot rescuers :(

        /* for armies, we use what is specified */
        Planets[i].armies = cqiPlanets[i].armies;

        Planets[i].uninhabtime = 0;

        for ( j = 0; j < NUMPLAYERTEAMS; j++ )
            Planets[i].scanned[j] = FALSE;

        /* we will need to defer setting up planet's team scan until
           we have identified the team's homeplanets... */
        for ( j = 0; j < NUMPLAYERTEAMS; j++ )
        {
            Teams[j].couptime = 0; /* time left to coup starts at zero. */
            Teams[j].coupinfo = FALSE; /* don't know coup time */
        }

        Planets[i].size = cqiPlanets[i].size;
        Planets[i].x = cqiPlanets[i].xcoord;
        Planets[i].y = cqiPlanets[i].ycoord;
        Planets[i].orbrad = cqiPlanets[i].radius;
        Planets[i].orbvel = cqiPlanets[i].velocity;

        if (cqiPlanets[i].angle < 0)
            Planets[i].orbang = rnduni( 0.0, 360.0 ); /* randomly choose one */
        else
            Planets[i].orbang = utMod360(cqiPlanets[i].angle);
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
                    Teams[TEAM_FEDERATION].homeplanet = i;
                break;
            case TEAM_ROMULAN:
                R++;
                if (R == 1)
                    Teams[TEAM_ROMULAN].homeplanet = i;
                break;
            case TEAM_KLINGON:
                K++;
                if (K == 1)
                    Teams[TEAM_KLINGON].homeplanet = i;
                break;
            case TEAM_ORION:
                O++;
                if (O == 1)
                    Teams[TEAM_ORION].homeplanet = i;
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
        if ( Planets[i].primary != i ) // not stationary
        {
            Planets[i].x = Planets[Planets[i].primary].x +
                Planets[i].orbrad * cosd(Planets[i].orbang);
            Planets[i].y = Planets[Planets[i].primary].y +
                Planets[i].orbrad * sind(Planets[i].orbang);
        }
    }

    /* now setup the intial scan status for each team's homeplanet(s) */
    for ( k = 0; k < MAXPLANETS; k++ )
    {
        if (PHOMEPLANET(k) && Planets[k].team < NUMPLAYERTEAMS)
            Planets[k].scanned[Planets[k].team] = TRUE;
    }

    /* Un-twiddle the lockword. */
    PVUNLOCK(&ConqInfo->lockword);

    /* Protect against a system crash here! */
    upchuck();

    return;

}
