#include "c_defs.h"

/************************************************************************
 *
 * server specific proc routines (from client)
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "conqdef.h"
#include "conqcom.h"
#include "conqlb.h"
#include "conqutil.h"
#include "conqunix.h"
#include "conf.h"
#include "server.h"
#include "serverpkt.h"
#include "servercmd.h"
#include "context.h"
#include "record.h"
#include "conqlb.h"
#include "rndlb.h"

#include "sproc.h"

/* disptach init */
int procDispatchInit(uint16_t vers, packetEnt_t *pktList, int numpkts)
{
    /* nothing since we only allow the server to deal with the latest proto */
    return TRUE;
}

void procSetName(char *buf)
{
    cpSetName_t *cpsetn = (cpSetName_t *)buf;

    if (!pktIsValid(CP_SETNAME, cpsetn))
        return;

    cpsetn->alias[MAXUSERPNAME - 1] = 0;

    strncpy(Users[Context.unum].alias, (char *)cpsetn->alias, MAXUSERPNAME);

    if (Context.snum >= 0 && Context.snum < MAXSHIPS)
        strncpy(Ships[Context.snum].alias, (char *)cpsetn->alias, MAXUSERPNAME);

    return;
}

void procSetCourse(char *buf)
{
    cpSetCourse_t *csc = (cpSetCourse_t *)buf;
    courseLock_t lock;
    uint16_t lockDetail = 0;
    real dir;

    if (!pktIsValid(CP_SETCOURSE, csc))
        return;

    lock = (courseLock_t)csc->lock;
    lockDetail = ntohs(csc->lockDetail);
    // if an invalid enum
    if ((int)lock >= LOCK_MAX)
    {
        utLog("%s: invalid courseLock enum %d(%d), ignoring", __FUNCTION__,
              (int)lock, (int)lockDetail);
        lock = LOCK_NONE;
        lockDetail = 0;
    }

    dir = (real)((real)ntohs(csc->head) / 100.0);

#if defined(DEBUG_SERVERPROC)
    utLog("PROC SETCOURSE: lock = %d, head = %f",
          lock, dir);
#endif

    /* now we need to do a few checks to make sure it's legal ;-) */

    if (dir < 0.0)
        dir = 0.0;

    if (dir > 359.9)
        dir = 359.9;

    if (lock != LOCK_NONE && !(lock == LOCK_PLANET && lockDetail < MAXPLANETS))
    {
        // could lock onto ships someday...
        //  but not today.
        utLog("%s: unsupported courseLock %d(%d), ignoring", __FUNCTION__,
              (int)lock, (int)lockDetail);
        lock = LOCK_NONE;
        lockDetail = 0;
    }
    /* always applies to our own ship */
    if ( Ships[Context.snum].warp < 0.0 ) /* if orbiting */
        Ships[Context.snum].warp = 0.0;   /* break orbit */

    Ships[Context.snum].dhead = dir;	// set direction first to
                                        // avoid a race in display()
    Ships[Context.snum].lock = lock;
    Ships[Context.snum].lockDetail = lockDetail;

    return;
}

void procSetWarp(cpCommand_t *swarp)
{
    int snum = Context.snum;		/* we always use our own ship */
    char cbuf[BUFFER_SIZE_256];
    real warp;
    real mw;

    if (!pktIsValid(CP_COMMAND, swarp))
        return;

    if (swarp->cmd != CPCMD_SETWARP)
        return;

    warp = (real)ntohs(swarp->detail);

#if defined(DEBUG_SERVERPROC)
    utLog("PROC SETWARP: warp = %f", warp);
#endif

    if ( Ships[snum].dwarp == 0.0 && warp != 0.0 )
    {
        /* See if engines are working. */
        if ( Ships[snum].efuse > 0 )
	{
            sendFeedback("Engines are currently overloaded.");
            return;
	}
        /* No charge if already warp 0. */
        if ( clbUseFuel( snum, ENGINES_ON_FUEL, FALSE, TRUE ) == FALSE)
	{
            sendFeedback("We don't have enough fuel.");
            return;
	}

        /* Don't stop repairing if changing to warp 0. */
        SFCLR(snum, SHIP_F_REPAIR);
    }

    /* If orbitting, break orbit. */
    if ( Ships[snum].warp < 0.0 )
    {
        Ships[snum].warp = 0.0;
        Ships[snum].lock = LOCK_NONE;
        Ships[snum].lockDetail = 0;
        Ships[snum].dhead = Ships[snum].head;
    }

    /* Handle ship limitations. */
    Ships[snum].dwarp = min( warp, ShipTypes[Ships[snum].shiptype].warplim );

    /* Warn about damage limitations. */
    mw = maxwarp( snum );
    if ( around( Ships[snum].dwarp ) > mw )
    {
        sprintf(cbuf,
                "(Due to damage, warp is currently limited to %.1f.)", mw);
        sendFeedback(cbuf);
    }

    return;
}

void procSetShields(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    int shup;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_SETSHIELDS)
        return;

    shup = (((int)ntohs(cmd->detail)) ? TRUE : FALSE);

#if defined(DEBUG_SERVERPROC)
    utLog("PROC SETSHIELDS: sh = %d", shup);
#endif

    if (shup)
    {
        SFSET(snum, SHIP_F_SHUP);
        SFCLR(snum, SHIP_F_REPAIR);
    }
    else
        SFCLR(snum, SHIP_F_SHUP);

    return;
}

void procAlloc(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    int alloc;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_ALLOC)
        return;

    /* we only deal with weapons alloc.  we compute engalloc based on this */
    alloc = (int)ntohs(cmd->detail);

    if (alloc < 30)
        alloc = 30;

    if (alloc > 70)
        alloc = 70;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC ALLOC: (w)alloc = %d", alloc);
#endif

    Ships[snum].weapalloc = alloc;
    Ships[snum].engalloc = (100 - alloc);

    return;
}


void procCloak(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    static char *nofuel="Not enough fuel to engage cloaking device.";

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_CLOAK)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC CLOAK");
#endif


    if ( SCLOAKED(snum) )
    {
        SFCLR(snum, SHIP_F_CLOAKED);
        sendFeedback("Cloaking device disengaged.");
        return;
    }
    if ( Ships[snum].efuse > 0 )
    {
        sendFeedback("Engines are currently overloaded.");
        return;
    }
    if ( Ships[snum].fuel < CLOAK_ON_FUEL )
    {
        sendFeedback(nofuel);
        return;
    }

    SFCLR(snum, SHIP_F_REPAIR);
    if ( ! clbUseFuel( snum, CLOAK_ON_FUEL, FALSE, TRUE ) )
    {
        sendFeedback(nofuel);
        return;
    }

    SFSET(snum, SHIP_F_CLOAKED);
    sendFeedback("Cloaking device engaged.");

    return;

}

void procDetSelf(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    int j;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_DETSELF)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC DETSELF");
#endif

    for ( j = 0; j < MAXTORPS; j++ )
        clbDetonate( snum, j );

    return;
}

void procDetEnemy(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_DETENEMY)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC DETENEMY");
#endif

    clbEnemyDet( snum );

    return;
}

void procDistress(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    char buf[128], cbuf[128];
    real x;
    int i, isorb = FALSE;
    int tofriendly;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_DISTRESS)
        return;

    tofriendly = (int)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
    utLog("PROC DISTRESS: tofriendly = %d", tofriendly);
#endif

    sprintf( cbuf,
             "sh=%d %c, dam=%d, fuel=%d, temp=",
             round(Ships[snum].shields),
             (SSHUP(snum)) ? 'U' : 'D',
             round(Ships[snum].damage),
             round(Ships[snum].fuel) );

    i = round(Ships[snum].wtemp);
    if ( i < 100 )
        utAppendInt(cbuf , i) ;
    else
        strcat(cbuf , "**") ;
    utAppendChar(cbuf , '/') ;
    i = round(Ships[snum].etemp);
    if ( i < 100 )
        utAppendInt(cbuf , i) ;
    else
        strcat(cbuf , "**") ;
    i = Ships[snum].armies;

    if ( i > 0 )
    {
        strcat(cbuf,  ", arm=");
        utAppendInt(cbuf , i) ;
    }

    if ( Ships[snum].wfuse > 0 )
        strcat(cbuf, ", -weap");

    if ( Ships[snum].efuse > 0 )
        strcat(cbuf, ", -eng");

    /* warp */
    x = Ships[snum].warp;
    if ( x >= 0.0 )
    {
        sprintf( buf, ", warp=%.1f", x );
        strcat(cbuf, buf) ;
        isorb = FALSE;
    }
    else
    {
        sprintf( buf, ", orbiting %.3s",
                 Planets[Ships[snum].lockDetail].name );
        strcat(cbuf, buf) ;
        isorb = TRUE;
    }

    /* heading */

    if (isorb == FALSE)
    {
        // locked onto a planet we haven't reached yet
        if (Ships[snum].lock == LOCK_PLANET
            && Ships[snum].lockDetail < MAXPLANETS)
        {
            sprintf( buf, ", head=%.3s", Planets[Ships[snum].lockDetail].name );
        }
        else
        {
            // just the heading ma'am
            sprintf( buf, ", head=%d", round( Ships[snum].head ));
        }

        strcat(cbuf, buf) ;
    }

    if (tofriendly)
        clbStoreMsg( MSG_FROM_SHIP, (uint16_t)snum, MSG_TO_FRIENDLY, 0,
                     cbuf );
    else
        clbStoreMsg( MSG_FROM_SHIP, (uint16_t)snum,
                     MSG_TO_TEAM, Ships[snum].team, cbuf );

    return;

}

void procFirePhaser(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    real dir;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_FIREPHASER)
        return;

    dir = (real)ntohs(cmd->detail) / 100.0;

    if (dir < 0)
        dir = 0.0;

    if (dir > 359.9)
        dir = 359.9;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC FIREPHASER: dir = %f", dir);
#endif

    if ( SCLOAKED(snum) )
    {
        sendFeedback("The cloaking device is using all available power.");
        return;
    }

    if ( Ships[snum].wfuse > 0 )
    {
        sendFeedback("Weapons are currently overloaded.");
        return;
    }

    if ( Ships[snum].fuel < PHASER_FUEL )
    {
        sendFeedback("Not enough fuel to fire phasers.");
        return;
    }

    if ( !clbPhaser( snum, dir ) )
        sendFeedback(">PHASERS DRAINED<");

    return;
}

void procOrbit(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    int pnum;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_ORBIT)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC ORBIT");
#endif

    if ( ( Ships[snum].warp == ORBIT_CW ) || ( Ships[snum].warp == ORBIT_CCW ) )
        return;

    if ( ! clbFindOrbit( snum, &pnum ) )
        return;

    if ( Ships[snum].warp > MAX_ORBIT_WARP )
        return;

    clbOrbit( snum, pnum );

    return;
}

void procRepair(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_REPAIR)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC REPAIR");
#endif

    if ( ! SCLOAKED(snum) )
    {
        SFSET(snum, SHIP_F_REPAIR);
        Ships[snum].dwarp = 0.0;
    }
    else
        sendFeedback("The cloaking device is using all available power.");

    return;
}

void procCoup(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    int i, pnum, now, entertime;
    real failprob;
    char *nhp="We must be orbiting our home planet to attempt a coup.";
    char cbuf[128];

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_COUP)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC COUP");
#endif

    /* Check for allowability. */
    if ( oneplace( Ships[snum].kills ) < MIN_COUP_KILLS )
    {
        sendFeedback("Fleet orders require three kills before a coup can be attempted.");
        return;
    }
    for ( i = 0; i < MAXPLANETS; i++ )
        if ( PVISIBLE(i) && (Planets[i].team == Ships[snum].team) &&
             (Planets[i].armies > 0) )
        {
            sendFeedback("We don't need to coup, we still have armies left!");
            return;
        }

    if ( Ships[snum].warp >= 0.0 )
    {
        sendFeedback(nhp);
        return;
    }

    // the assumption if that if you are orbiting (warp < 0) you are
    // locked onto the planet you are orbiting...
    pnum = Ships[snum].lockDetail;
    if ( pnum != Teams[Ships[snum].team].homeplanet )
    {
        sendFeedback(nhp);
        return;
    }
    if ( Planets[pnum].armies > MAX_COUP_ENEMY_ARMIES )
    {
        sendFeedback("The enemy is still too strong to attempt a coup.");
        return;
    }
    i = Planets[pnum].uninhabtime;
    if ( i > 0 )
    {
        sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
                 i );
        sendFeedback(cbuf);
        return;
    }

    /* Now our team can tell coup time for free. */
    Teams[Ships[snum].team].coupinfo = TRUE;

    i = Teams[Ships[snum].team].couptime;
    if ( i > 0 )
    {
        sprintf( cbuf, "Our forces need %d more minutes to organize.", i );
        sendFeedback(cbuf);
        return;
    }

    /* Now wait it out... */
    sendFeedback("Attempting coup...");
    utGrand( &entertime );
    while ( utDeltaGrand( entertime, &now ) < COUP_GRAND )
    {
        /* See if we're still alive. */
        if ( ! clbStillAlive( Context.snum ) )
            return;

        /* Sleep */
        utSleep( ITER_SECONDS );
    }

    PVLOCK(&ConqInfo->lockword);
    if ( Planets[pnum].team == Ships[snum].team )
    {
        PVUNLOCK(&ConqInfo->lockword);
        sendFeedback("Sensors show hostile forces eliminated from the planet.");
        return;
    }

    failprob = Planets[pnum].armies / MAX_COUP_ENEMY_ARMIES * 0.5 + 0.5;

    if ( rnd() < failprob )
    {
        /* Failed; setup new reorganization time. */
        Teams[Ships[snum].team].couptime = rndint( 5, 10 );
        PVUNLOCK(&ConqInfo->lockword);
        sendFeedback("Coup unsuccessful.");
        return;
    }

    clbTakePlanet( pnum, snum );

    /* Make the planet not scanned. */
    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
        Planets[pnum].scanned[i] = FALSE;

    /* ...except by us */
    Planets[pnum].scanned[Ships[snum].team] = TRUE;

    Planets[pnum].armies = rndint( 10, 20 );	/* create token coup force */
    Users[Ships[snum].unum].stats[USTAT_COUPS] += 1;
    Teams[Ships[snum].team].stats[TSTAT_COUPS] += 1;
    PVUNLOCK(&ConqInfo->lockword);

    sendFeedback("Coup successful!");

    /* force a team update for this ship */
    sendTeam(sInfo.sock, Ships[snum].team, TRUE);

    return;
}

void procFireTorps(char *buf)
{
    int snum = Context.snum;		/* we always use our own ship */
    cpFireTorps_t *cftorp = (cpFireTorps_t *)buf;
    real dir;
    int num;

    if (!pktIsValid(CP_FIRETORPS, cftorp))
        return;

    dir = (real)((real)ntohs(cftorp->dir) / 100.0);
    num = (int)cftorp->num;

    if (dir < 0)
        dir = 0.0;

    if (dir > 359.9)
        dir = 359.9;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC FIRETORPS: dir = %f, num = %d", dir, num);
#endif

    if ( SCLOAKED(snum) )
    {
        sendFeedback("The cloaking device is using all available power.");
        return;
    }

    if ( Ships[snum].wfuse > 0 )
    {
        sendFeedback("Weapons are currently overloaded.");
        return;
    }

    if ( Ships[snum].fuel < TORPEDO_FUEL )
    {
        sendFeedback("Not enough fuel to launch a torpedo.");
        return;
    }

    if ( ! clbLaunch( snum, dir, num, LAUNCH_NORMAL ) )
    {
        sendFeedback(">TUBES EMPTY<");
    }

    return;

}

void procMessage(char *buf)
{
    uint16_t snum = (uint16_t)Context.snum; /* we always use our own ship */
    cpMessage_t *cmsg = (cpMessage_t *)buf;
    msgTo_t to;
    uint16_t toDetail;

    if (sInfo.state != SVR_STATE_PLAY)
        return;

    if (!pktIsValid(CP_MESSAGE, cmsg))
        return;

    // bogus 'to' enum?
    if ((int)cmsg->to >= MSG_TO_MAX)
        return;

    to = (msgTo_t)cmsg->to;
    toDetail = ntohs(cmsg->toDetail);
    cmsg->msg[MESSAGE_SIZE - 1] = 0;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC MESSAGE: to %d(%d)", to, (int)toDetail);
#endif

    clbStoreMsg(MSG_FROM_SHIP, snum, to, toDetail, (char *)cmsg->msg);
    checkOperExec(MSG_FROM_SHIP, snum, to, toDetail, (char *)cmsg->msg);

    return;
}

void procChangePassword(char *buf)
{
    char salt[3];
    int unum = Context.unum;
    cpAuthenticate_t *cauth = (cpAuthenticate_t *)buf;

    if (!pktIsValid(CP_AUTHENTICATE, buf))
        return;

    cauth->pw[MAXUSERNAME - 1] = 0;

    /* we are just interested in the pw */

#if defined(DEBUG_SERVERPROC)
    utLog("PROC ChangePassword");
#endif

    salt[0] = (Users[unum].username[0] != 0) ? Users[unum].username[0] :
        'J';
    salt[1] = (Users[unum].username[1] != 0) ? Users[unum].username[1] :
        'T';
    salt[2] = 0;

    strncpy(Users[unum].pw, (char *)crypt((char *)cauth->pw, salt),
            MAXUSERNAME - 2);
    Users[unum].pw[MAXUSERNAME - 1] = 0;

    return;
}


void procSetWar(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    int unum = Context.unum;
    int dowait = FALSE, entertime, now, i;
    uint8_t war;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_SETWAR)
        return;

    war = (uint8_t)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
    utLog("PROC SETWAR war = 0x%02x", war);
#endif

    for (i=0; i<NUMPLAYERTEAMS; i++)
    {
        if (war & (1 << i))
	{
            if (!Ships[Context.snum].war[i]) /* if not at war, we will delay */
                dowait = TRUE;

            Ships[snum].war[i] = TRUE;
	}
        else
	{
            Ships[snum].war[i] = FALSE;
	}

        Users[unum].war[i] = Ships[snum].war[i];
    }

    /* now we 'sleep' for awhile, if dowait is set */
    /* any packets the client tries to send will have to wait ;-) */
    if (dowait && Ships[Context.snum].status != SS_RESERVED)
    {
        utGrand( &entertime );
        while ( utDeltaGrand( entertime, &now ) < REARM_GRAND )
	{
            /* See if we're still alive. */
            if ( ! clbStillAlive( Context.snum ) )
                return;

            /* Sleep */
            utSleep( ITER_SECONDS );
	}
    }

    return;
}

void procRefit(cpCommand_t *cmd)
{
    int snum = Context.snum;		/* we always use our own ship */
    int entertime, now;
    int stype;
    int pnum;
    static char *wmbio = "We must be orbiting a team owned planet to refit.";

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_REFIT)
        return;

    stype = (int)ntohs(cmd->detail);

    if (stype < 0 || stype >= MAXNUMSHIPTYPES)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC REFIT: snum - %d, stype = %d", snum, stype);
#endif

    /* Check for allowability. */
    if ( oneplace( Ships[snum].kills ) < MIN_REFIT_KILLS )
    {
        sendFeedback("You must have at least one kill to refit.");
        return;
    }

    // make sure we are in orbit
    if (Ships[snum].warp >= 0.0)
    {
        sendFeedback(wmbio);
        return;
    }

    // assumption here is that we are in orbit, and therefore locked
    // onto the planet we are orbiting...
    pnum = Ships[snum].lockDetail;

    if (Planets[pnum].team != Ships[snum].team)
    {
        sendFeedback(wmbio);
        return;
    }

    if (Ships[snum].armies != 0)
    {
        sendFeedback("You cannot refit while carrying armies");
        return;
    }

    /* now we wait for a bit. */
    utGrand( &entertime );
    while ( utDeltaGrand( entertime, &now ) < REFIT_GRAND )
    {
        /* See if we're still alive. */
        if ( ! clbStillAlive( snum ) )
            return;

        /* Sleep */
        utSleep( ITER_SECONDS );
    }

    /* make it so... */
    Ships[snum].shiptype = stype;

    return;
}

void procSetRate(cpCommand_t *cmd)
{
#if defined(DEBUG_SERVERPROC)
    int snum = Context.snum;
#endif
    int rate;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_SETRATE)
        return;

    rate = (int)ntohs(cmd->detail);

    if (rate < 1 || rate > 10)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC SETRATE: snum = %d, rate = %d", snum, rate);
#endif

    Context.updsec = rate;

    return;
}

void procTow(cpCommand_t *cmd)
{
    int snum = Context.snum;
    int other;
    char cbuf[BUFFER_SIZE_256];

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_TOW)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC TOW: snum = %d", snum);
#endif

    other = (int)ntohs(cmd->detail);

    if ( Ships[snum].towedby != 0 )
    {
        strcpy(cbuf , "But we are being towed by ") ;
        utAppendShip(cbuf , Ships[snum].towedby) ;
        utAppendChar(cbuf , '!') ;
        sendFeedback(cbuf);
        return;
    }
    if ( Ships[snum].towing != 0 )
    {
        strcpy(cbuf , "But we're already towing ") ;
        utAppendShip(cbuf , Ships[snum].towing) ;
        utAppendChar(cbuf , '.') ;
        sendFeedback(cbuf);
        return;
    }
    cbuf[0] = 0;
    PVLOCK(&ConqInfo->lockword);
    if ( other < 0 || other >= MAXSHIPS )
        strcpy(cbuf , "No such ship.") ;
    else if ( Ships[other].status != SS_LIVE )
        strcpy(cbuf , "Not found.") ;
    else if ( other == snum )
        strcpy(cbuf , "We can't tow ourselves!") ;
    else if ( dist( Ships[snum].x, Ships[snum].y, Ships[other].x, Ships[other].y ) > TRACTOR_DIST )
        strcpy(cbuf , "That ship is out of tractor range.") ;
    else if ( Ships[other].warp < 0.0 )
        strcpy(cbuf , "You can't tow a ship out of orbit.") ;
    else if ( sqrt( pow(( (real) (Ships[snum].dx - Ships[other].dx) ), (real) 2.0) +
                    pow( (real) ( Ships[snum].dy - Ships[other].dy ), (real) 2.0 ) ) /
              ( MM_PER_SEC_PER_WARP * ITER_SECONDS ) > MAX_TRACTOR_WARP )
        sprintf( cbuf, "That ships relative velocity is higher than %2.1f.",
                 MAX_TRACTOR_WARP );
    else if ( Ships[other].towing != 0 || Ships[other].towedby != 0 )
        strcpy(cbuf,
               "There seems to be some interference with the tractor beams...");
    else
    {
        Ships[other].towedby = snum;
        Ships[snum].towing = other;
        strcpy(cbuf, "Tractor beams engaged.") ;
    }
    PVUNLOCK(&ConqInfo->lockword);

    sendFeedback(cbuf);

    return;

}

void procUnTow(cpCommand_t *cmd)
{
    int snum = Context.snum;
    char cbuf[BUFFER_SIZE_256];
    int entertime, now;
    int warsome;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_UNTOW)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC UNTOW: snum = %d", snum);
#endif

    if ( Ships[snum].towedby != 0 )
    {
        /* If we're at war with him or he's at war with us, make it */
        /*  hard to break free. */
        warsome = ( satwar( snum, Ships[snum].towedby) );
        if ( warsome )
	{
            utGrand( &entertime );
            while ( utDeltaGrand( entertime, &now ) < BREAKAWAY_GRAND )
	    {
                if ( ! clbStillAlive( Context.snum ) )
                    return;

                utSleep( ITER_SECONDS );

	    }
	}
        if ( warsome && ( rnd() > BREAKAWAY_PROB ) )
            sendFeedback("Attempt to break free failed.");
        else
	{
            strcpy(cbuf , "Breaking free from ship ") ;
            utAppendShip(cbuf , Ships[snum].towedby) ;
            PVLOCK(&ConqInfo->lockword);
            if ( Ships[snum].towedby != 0 )
	    {
                /* Coast to a stop. */
                Ships[snum].head = Ships[Ships[snum].towedby].head;

                if (!SysConf.AllowSlingShot)
                {               /* only set warp if valid JET - 9/15/97 */
                    if (Ships[Ships[snum].towedby].warp >= 0.0)
                        Ships[snum].warp = Ships[Ships[snum].towedby].warp;
                    else
                        Ships[snum].warp = 2.0;
                }
                else
                    Ships[snum].warp = Ships[Ships[snum].towedby].warp;

                /* Release the tow. */
                if ( Ships[Ships[snum].towedby].towing != 0 )
                    Ships[Ships[snum].towedby].towing = 0;
                Ships[snum].towedby = 0;
	    }
            PVUNLOCK(&ConqInfo->lockword);
            utAppendChar(cbuf , '.') ;
            sendFeedback(cbuf);
	}
    }
    else if ( Ships[snum].towing != 0 )
    {
        strcpy(cbuf , "Tow released from ship ") ;
        utAppendShip(cbuf , Ships[snum].towing) ;
        PVLOCK(&ConqInfo->lockword);
        if ( Ships[snum].towing != 0 )
	{
            /* Set other ship coasting. */
            Ships[Ships[snum].towing].head = Ships[snum].head;


            if (!SysConf.AllowSlingShot)
            {               /* only set warp if valid JET - 9/15/97 */
                if (Ships[snum].warp >= 0.0)
                    Ships[Ships[snum].towing].warp = Ships[snum].warp;
                else
                    Ships[Ships[snum].towing].warp = 2.0;
            }
            else
                Ships[Ships[snum].towing].warp = Ships[snum].warp;

            /* Release the tow. */
            if ( Ships[Ships[snum].towing].towedby != 0 )
                Ships[Ships[snum].towing].towedby = 0;
            Ships[snum].towing = 0;
	}
        PVUNLOCK(&ConqInfo->lockword);
        utAppendChar(cbuf , '.') ;
        sendFeedback(cbuf);
    }
    else
        sendFeedback("No tractor beam activity detected.");

    return;

}

void procBomb(cpCommand_t *cmd)
{
    int snum = Context.snum;
    int bomb;
    int pnum, now, entertime, total, ototal, oparmies;
    real x, killprob;
    int oldsshup;
    char buf[MSGMAXLINE];
    char cbuf[BUFFER_SIZE_256];
    char *lastfew="The last few armies are eluding us.";


    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_BOMB)
        return;

    bomb = (int)ntohs(cmd->detail);

#if defined(DEBUG_SERVERPROC)
    utLog("PROC BOMB: snum = %d, start bombing = %d", snum, bomb);
#endif

    if (!bomb)			/* the bombing has stopped. yaay. */
        return;

    SFCLR(snum, SHIP_F_REPAIR);

    /* Check for allowability. */
    if ( Ships[snum].warp >= 0.0 )
    {
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback("We must be orbiting a planet to bombard it.");
        return;
    }

    // as we are in orbit, we are locked onto the planet
    pnum = Ships[snum].lockDetail;
    if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON ||
         Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
    {
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback("There is no one there to bombard.");
        return;
    }
    if ( Planets[pnum].team == Ships[snum].team )
    {
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback("We can't bomb our own armies!");
        return;
    }
    if ( Planets[pnum].team != TEAM_SELFRULED && Planets[pnum].team != TEAM_GOD )
        if ( ! Ships[snum].war[Planets[pnum].team] )
        {
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            sendFeedback("But we are not at war with this planet!");
            return;
        }

    /* Handle war logic. */
    Ships[snum].srpwar[pnum] = TRUE;
    if ( Planets[pnum].team >= 0 && Planets[pnum].team < NUMPLAYERTEAMS )
    {
        /* For a team planet make the war sticky and send an intruder alert. */
        Ships[snum].rwar[Planets[pnum].team] = TRUE;
        clbIntrude( snum, pnum );
    }
    /* Planets owned by GOD have a special defense system. */
    if ( Planets[pnum].team == TEAM_GOD )
    {
        sprintf( cbuf, "That was a bad idea, %s...", Ships[snum].alias );
        clbDamage( snum,  rnduni( 50.0, 100.0 ), KB_LIGHTNING, 0 );
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback(cbuf);
        return;
    }

    /* Lower shields. */
    oldsshup = SSHUP(snum);
    SFCLR(snum, SHIP_F_SHUP);

    /* Bombard. */
    total = 0;
    ototal = -1;			/* force an update the first time */
    oparmies = -1;
    utGrand( &entertime );		/* get start time */
    SFSET(snum, SHIP_F_BOMBING);
    while(TRUE)
    {
        if ( ! clbStillAlive( Context.snum ) )
        {
            SFCLR(snum, SHIP_F_BOMBING);
            return;
        }

        if ( pktReadPacketReady() )
        {
            SFCLR(snum, SHIP_F_BOMBING);
            break;
        }

        /* See if it's time to bomb yet. */
        while ((int) fabs ((real)utDeltaGrand( (int)entertime, (int *)&now )) >= BOMBARD_GRAND )
	{
            if ( Ships[snum].wfuse > 0 )
	    {
                pktSendAck(PSEV_INFO, PERR_CANCELED,
                           NULL);
                sendFeedback("Weapons are currently overloaded.");
                goto cbrk22; /* break 2;*/
	    }
            x = BOMBARD_FUEL * (real)(BOMBARD_GRAND / 1000.0);
            if ( ! clbUseFuel( snum, x, TRUE, TRUE ) )
	    {
                pktSendAck(PSEV_INFO, PERR_CANCELED,
                           NULL);
                sendFeedback("Not enough fuel to bombard.");
                goto cbrk22; /* break 2;*/
	    }
            /*  entertime = mod( entertime + BOMBARD_GRAND, 24*60*60*1000 );*/
            utGrand(&entertime);
            killprob = (real)((BOMBARD_PROB *
                               ((real) weaeff( snum ) *
                                (real)((real)Planets[pnum].armies/100.0))) + 0.5);

            if ( rnd() < killprob )
	    {
                PVLOCK(&ConqInfo->lockword);
                if ( Planets[pnum].armies <= MIN_BOMB_ARMIES )
		{
                    /* No more armies left to bomb. */
                    PVUNLOCK(&ConqInfo->lockword);
                    pktSendAck(PSEV_INFO, PERR_CANCELED,
                               NULL);
                    sendFeedback(lastfew);
                    goto cbrk22; /* break 2;*/
		}
                Planets[pnum].armies = Planets[pnum].armies - 1;

                Ships[snum].kills = Ships[snum].kills + BOMBARD_KILLS;
                Users[Ships[snum].unum].stats[USTAT_ARMBOMB] += 1;
                Teams[Ships[snum].team].stats[TSTAT_ARMBOMB] += 1;
                PVUNLOCK(&ConqInfo->lockword);
                total = total + 1;
	    }
	} /* while */

        if ( Planets[pnum].armies <= MIN_BOMB_ARMIES )
	{
            /* No more armies left to bomb. */
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            sendFeedback(lastfew);
            break;
	}

        if ( Planets[pnum].armies != oparmies || ototal != total )
	{
            /* Either our bomb run total or the population changed. */
            oparmies = Planets[pnum].armies;
            if ( total == 1 )
                strcpy(buf , "y") ;
            else
                strcpy(buf , "ies") ;
            sprintf( cbuf, "Bombing %s, %d arm%s killed, %d left.",
                     Planets[pnum].name, total, buf, oparmies );
            sendFeedback(cbuf);

            ototal = total;
	}

        utSleep( ITER_SECONDS );

    } /* while */
cbrk22:
    ;

    /* no longer bombing */
    SFCLR(snum, SHIP_F_BOMBING);

    /* Restore shields. */
    if (oldsshup)
        SFSET(snum, SHIP_F_SHUP);

    return;

}


void procBeam(cpCommand_t *cmd)
{
    int snum = Context.snum;
    int beam, beamup;
    int pnum, total, num, upmax, downmax, capacity, beamax, i;
    int ototal, entertime, now;
    int oldsshup, dirup, zeroed, conqed;
    char cbuf[BUFFER_SIZE_256];
    real rkills;
    char *lastfew="Fleet orders prohibit removing the last three armies.";

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_BEAM)
        return;

    /* detail is (armies & 0x00ff), 0x8000 set if beaming down */

    beam = (int)(ntohs(cmd->detail) & 0x00ff);

    if (beam == 0)
        return;			/* stop beaming */

    if (ntohs(cmd->detail) & 0x8000)
        beamup = FALSE;		/* beaming down */
    else
        beamup = TRUE;		/* beaming up */

#if defined(DEBUG_SERVERPROC)
    utLog("PROC BEAM: snum = %d, detail = 0x%04x, beamup = %d, beam = %d",
          snum, cmd->detail, beamup, beam);
#endif

    SFCLR(snum, SHIP_F_REPAIR);

    /* Check for allowability. */
    if ( Ships[snum].warp >= 0.0 )
    {
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback("We must be orbiting a planet to use the transporter.");
        return;
    }

    // if we are orbiting, then we are locked onto a planet
    pnum = Ships[snum].lockDetail;
    if ( Ships[snum].armies > 0 )
    {
        if ( Planets[pnum].type == PLANET_SUN )
	{
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            sendFeedback("Idiot!  Our armies will fry down there!");
            return;
	}
        else if ( Planets[pnum].type == PLANET_MOON )
	{
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            sendFeedback("Fool!  Our armies will suffocate down there!");
            return;
	}
        else if ( Planets[pnum].team == TEAM_GOD )
	{
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            sendFeedback("GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.");
            return;
	}
    }

    i = Planets[pnum].uninhabtime;
    if ( i > 0 )
    {
        sprintf( cbuf, "This planet is uninhabitable for %d more minute",
                 i );
        if ( i != 1 )
            utAppendChar(cbuf , 's') ;
        utAppendChar(cbuf , '.') ;
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback(cbuf);
        return;
    }

    if ( Planets[pnum].team != Ships[snum].team &&
         Planets[pnum].team != TEAM_SELFRULED &&
         Planets[pnum].team != TEAM_NOTEAM )
        if ( ! Ships[snum].war[Planets[pnum].team] && Planets[pnum].armies != 0) /* can take empty planets */
        {
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            sendFeedback("But we are not at war with this planet!");
            return;
        }

    if ( Ships[snum].armies == 0 &&
         Planets[pnum].team == Ships[snum].team && Planets[pnum].armies <= MIN_BEAM_ARMIES )
    {
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback(lastfew);
        return;
    }

    rkills = Ships[snum].kills;

    if ( rkills < (real)1.0 )
    {
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback("Fleet orders prohibit beaming armies until you have a kill.");
        return;
    }

    /* Figure out what can be beamed. */
    downmax = Ships[snum].armies;
    if ( clbSPWar(snum,pnum) ||
         Planets[pnum].team == TEAM_SELFRULED ||
         Planets[pnum].team == TEAM_NOTEAM ||
         Planets[pnum].team == TEAM_GOD ||
         Planets[pnum].armies == 0 )
    {
        upmax = 0;
    }
    else
    {
        capacity = min( (int)rkills * 2,
                        ShipTypes[Ships[snum].shiptype].armylim );
        upmax = min( Planets[pnum].armies - MIN_BEAM_ARMIES,
                     capacity - Ships[snum].armies );
    }

    /* If there are armies to beam but we're selfwar... */
    if ( upmax > 0 && selfwar(snum) && Ships[snum].team == Planets[pnum].team )
    {
        if ( downmax <= 0 )
	{
            strcpy(cbuf , "The arm") ;
            if ( upmax == 1 )
                strcat(cbuf , "y is") ;
            else
                strcat(cbuf , "ies are") ;
            strcat(cbuf , " reluctant to beam aboard a pirate vessel.") ;
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            sendFeedback(cbuf);
            return;
	}
        upmax = 0;
    }

    /* Figure out which direction to beam. */
    if ( upmax <= 0 && downmax <= 0 )
    {
        pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
        sendFeedback("There is no one to beam.");
        return;
    }
    if ( upmax <= 0 )
        dirup = FALSE;
    else if ( downmax <= 0 )
        dirup = TRUE;
    else
        dirup = beamup;

    if ( dirup )
        beamax = upmax;
    else
        beamax = downmax;

    if (beam > beamax)
        beam = beamax;

    num = beam;

    /* Now we are ready! */
    if ( Planets[pnum].team >= NUMPLAYERTEAMS )
    {
        /* If the planet is not race owned, make it war with us. */
        Ships[snum].srpwar[pnum] = TRUE;
    }
    else if ( Planets[pnum].team != Ships[snum].team )
    {
        /* For a team planet make the war sticky and send an intruder alert. */
        Ships[snum].rwar[Planets[pnum].team] = TRUE;

        /* Chance to create a robot here. */
        clbIntrude( snum, pnum );
    }

    /* Lower shields. */
    oldsshup = SSHUP(snum);
    SFCLR(snum, SHIP_F_SHUP);

    /* Beam. */
    total = 0;
    ototal = -1;				/* force an update the first time */
    zeroed = FALSE;
    conqed = FALSE;

    utGrand( &entertime );
    while(TRUE)
    {
        if ( ! clbStillAlive( Context.snum ) )
            return;
        if ( pktReadPacketReady() )
            break;

        /* See if it's time to beam again. */
        while ( utDeltaGrand( entertime, &now ) >= BEAM_GRAND )
	{
            utGrand(&entertime);
            PVLOCK(&ConqInfo->lockword);
            if ( dirup )
	    {
                /* Beam up. */
                if ( Planets[pnum].armies <= MIN_BEAM_ARMIES )
		{
                    PVUNLOCK(&ConqInfo->lockword);
                    pktSendAck(PSEV_INFO, PERR_CANCELED,
                               NULL);
                    sendFeedback(lastfew);
                    break;
		}
                Ships[snum].armies = Ships[snum].armies + 1;
                Planets[pnum].armies = Planets[pnum].armies - 1;
	    }
            else
	    {
                /* Beam down. */
                Ships[snum].armies = Ships[snum].armies - 1;
                if ( Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
		{
                    clbTakePlanet( pnum, snum );
                    conqed = TRUE;
		}
                else if ( Planets[pnum].team != Ships[snum].team )
		{
                    Planets[pnum].armies = Planets[pnum].armies - 1;
                    if ( Planets[pnum].armies == 0 )
		    {
                        clbZeroPlanet( pnum, snum );
                        zeroed = TRUE;
		    }
		}
                else
                    Planets[pnum].armies = Planets[pnum].armies + 1;
	    }
            PVUNLOCK(&ConqInfo->lockword);
            total = total + 1;

            if ( total >= num )
	    {
                /* Done. */
                pktSendAck(PSEV_INFO, PERR_DONE,
                           NULL);
                sendFeedback("");
                goto cbrk21;
	    }
	}

        if ( ototal != total )
	{
            strcpy(cbuf , "Beaming ") ;
            if ( dirup )
                strcat(cbuf , "up from ") ;
            else
                strcat(cbuf , "down to ") ;
            strcat(cbuf , Planets[pnum].name) ;
            strcat(cbuf,  ", ");
            if ( total == 0 )
                strcat(cbuf , "no") ;
            else
                utAppendInt(cbuf , total) ;
            strcat(cbuf , " arm") ;
            if ( total == 1 )
	    {
                utAppendChar(cbuf , 'y') ;
	    }
            else
	    {
                strcat(cbuf , "ies") ;
	    }
            strcat(cbuf, " transported, ");
            utAppendInt(cbuf , num - total) ;
            strcat(cbuf , " to go.") ;
            sendFeedback(cbuf);
            ototal = total;
	}

        if ( dirup && Planets[pnum].armies <= MIN_BEAM_ARMIES )
	{
            sendFeedback(lastfew);
            break;
	}

        utSleep( ITER_SECONDS );

    }
cbrk21:

    /* Restore shields. */
    if (oldsshup)
        SFSET(snum, SHIP_F_SHUP);

    if ( conqed )
    {
        sprintf( cbuf, "You have conquered %s.", Planets[pnum].name );
        sendFeedback(cbuf);
    }
    else if ( zeroed )
        sendFeedback("Sensors show hostile forces eliminated from the planet.");

    return;

}

void procDestruct(cpCommand_t *cmd)
{
    int snum = Context.snum;
    int entertime, now;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_DESTRUCT)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC DESTRUCT: snum = %d, detail = 0x%04x",
          snum, cmd->detail);
#endif

    if (!ntohs(cmd->detail))
        return;			/* canceled a self destruct */

    /* time to breach the core */

    if ( SCLOAKED(snum) )
    {
        pktSendAck(PSEV_INFO, PERR_CANCELED,
                   NULL);
        sendFeedback("The cloaking device is using all available power.");
        return;
    }

    /* Set up the destruct fuse. */
    Ships[Context.snum].sdfuse = SELFDESTRUCT_FUSE;

    utGetSecs( &entertime );


    Context.msgok = TRUE;			/* messages are ok in the beginning */
    while ( Ships[Context.snum].sdfuse > 0 )
    {
        Ships[Context.snum].sdfuse = SELFDESTRUCT_FUSE - utDeltaSecs ( entertime, &now );

        /* Display new messages until T-minus 3 seconds. */

        if ( Ships[Context.snum].sdfuse < 3 )
            Context.msgok = FALSE;

        if ( ! clbStillAlive( Context.snum ) )
	{
            /* Died in the process. */
            Ships[Context.snum].sdfuse = 0;
            return;
	}

        if ( pktReadPacketReady() )
	{
            Ships[Context.snum].sdfuse = 0;
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            return;
	}

        utSleep( ITER_SECONDS );

    } /* end while */

    Context.msgok = FALSE;			/* turn off messages */

    if ( Doomsday->status == DS_LIVE )
    {
        if ( dist(Ships[Context.snum].x, Ships[Context.snum].y,
                  Doomsday->x, Doomsday->y) <= DOOMSDAY_KILL_DIST )
	{
            Doomsday->status = DS_OFF;
            clbStoreMsg( MSG_FROM_DOOM, 0, MSG_TO_ALL, 0, "AIEEEEEEEE!" );
            clbKillShip( Context.snum, KB_GOTDOOMSDAY, 0 );
	}
        else
            if (clbStillAlive(Context.snum))	/* if we're not dead yet... */
                clbKillShip( Context.snum, KB_SELF, 0 );
    }
    else
    {
        if (clbStillAlive(Context.snum))	/* if we're not dead yet... */
            clbKillShip( Context.snum, KB_SELF, 0 );
    }

    pktSendAck(PSEV_INFO, PERR_DONE,
               NULL);
    return;

}

void procAutoPilot(cpCommand_t *cmd)
{
    int snum = Context.snum;
    int laststat, now;

    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_AUTOPILOT)
        return;

#if defined(DEBUG_SERVERPROC)
    utLog("PROC AUTOPILOT: snum = %d, detail = 0x%04x",
          snum, ntohs(cmd->detail));
#endif

    if (!ntohs(cmd->detail))
        return;			/* cancelled autopilot */

    /* allowed? */
    if (!Users[Ships[snum].unum].ooptions[OOPT_AUTOPILOT])
    {
        utLog("PROC AUTOPILOT: unum = %d, snum = %d: NOT ALLOWED",
              Ships[snum].unum, snum);
        return;
    }

    sendFeedback("Autopilot activated.");
    SFSET(snum, SHIP_F_ROBOT);

    utGetSecs( &laststat );			/* initialize stat timer */
    while ( clbStillAlive( Context.snum ) )
    {
        /* Make sure we still control our ship. */
        if ( Ships[snum].pid != Context.pid )
            break;

        /* See if it's time to update the statistics. */
        if ( utDeltaSecs( laststat, &now ) >= 15 )
	{
            conqstats( Context.snum );
            laststat = now;
	}

        if ( pktReadPacketReady() )
        {
            pktSendAck(PSEV_INFO, PERR_CANCELED, NULL);
            break;
        }

        utSleep(ITER_SECONDS);
    }

    SFCLR(snum, SHIP_F_ROBOT);
    Ships[snum].action = 0;

    pktSendAck(PSEV_INFO, PERR_DONE,
               NULL);

    return;
}

void procReload(cpCommand_t *cmd)
{
    if (!pktIsValid(CP_COMMAND, cmd))
        return;

    if (cmd->cmd != CPCMD_RELOAD)
        return;

    /* reset the packet history */
    spktInitPkt();

    return;
}
