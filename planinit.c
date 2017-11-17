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
    int homeplan[NUMPLAYERTEAMS][3];             /* ids of home planets */
    int F, K, R, O;               /* homeplanet count per team */

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

    memset((void *)homeplan, 0, sizeof(int) * NUMPLAYERTEAMS * 3);

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

    /* now setup the team-based homeplanets, homeplanet, and homesun */

    /* get all the homeplanets */
    F = K = R = O = 0;
    for (i=0; i<cqiGlobal->maxplanets; i++)
    {
        if (cqiPlanets[i].homeplanet)
        {
            switch (cqiPlanets[i].pteam)
            {
            case TEAM_FEDERATION:
                if (F < 3)
                    homeplan[cqiPlanets[i].pteam][F++] = i;
                break;
            case TEAM_ROMULAN:
                if (R < 3)
                    homeplan[cqiPlanets[i].pteam][R++] = i;
                break;
            case TEAM_KLINGON:
                if (K < 3)
                    homeplan[cqiPlanets[i].pteam][K++] = i;
                break;
            case TEAM_ORION:
                if (O < 3)
                    homeplan[cqiPlanets[i].pteam][O++] = i;
                break;

            default:
                break;
            }
        }
    }

    /* this should not happen */
    if (F != 3 || R != 3 || K != 3 || O != 3)
    {
        utLog("ERROR: cqiInitPlanets: homeplanet count for each team is not == 3! (f=%d o=%d k=%d r=%d)",
              F, O, K, R);
        /* continue, but homeplanets will be screwed up */
    }
    else
    {
        // set up the team homeplanets 
        for (i=0; i < NUMPLAYERTEAMS; i++)
        {
            Teams[i].teamhplanets[0] =
                homeplan[i][0];
            Teams[i].teamhplanets[1] =
                homeplan[i][1];
            Teams[i].teamhplanets[2] =
                homeplan[i][2];

            /* primary homeplanet - 1st one */
            Teams[i].homeplanet = homeplan[i][0];

            /* setup homesun - primary of 1st one */
            Teams[i].homesun = Planets[Teams[i].teamhplanets[0]].primary;
        }
    }

    /* Place the planets in their proper orbits. */
    /* careful about planets that orbit themselves (planet == primary)  */
    for ( i = MAXPLANETS - 1; i >= 0; i-- )
    {
        if ( Planets[i].primary != 0 && Planets[i].primary != i )
        {
            Planets[i].x = Planets[Planets[i].primary].x +
                Planets[i].orbrad * cosd(Planets[i].orbang);
            Planets[i].y = Planets[Planets[i].primary].y +
                Planets[i].orbrad * sind(Planets[i].orbang);
        }
    }

    /* now setup the intial scan status */
    for ( j = 0; j < NUMPLAYERTEAMS; j++ )
    {
        /* Each team has scanned its own planets. */
        for ( k = 0; k < 3; k++ )
            Planets[Teams[j].teamhplanets[k]].scanned[j] = TRUE;
    }

    /* Un-twiddle the lockword. */
    PVUNLOCK(&ConqInfo->lockword);

    /* Protect against a system crash here! */
    upchuck();

    return;

}
