#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */


#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "conqlb.h"
#include "conqutil.h"
#include "global.h"
#include "conf.h"
#include "user.h"
#include "conqai.h"
#include "conqunix.h"
#include "rndlb.h"

static int nenum;
static int debug;
static real dne, ane;

static void buildai( int snum, int vars[], int *bnenum,
                     real *bdne, real *bane );
static void displayai( int snum, int token, int vars[] );
static void executeai( int snum, int token );
static void trobotai( int snum );
static int tableai( int vars[] );

/* Find first bit that is set and return bit number */
static bool lib_ffs(int start, int len, int bits, int *rule)
{
    int i;

    *rule = -1;
    for (i = start; i < len; i++)
    {				/* check the bits */
        if ((bits & (1 << i)) != 0)
	{			/* bit is set */
            *rule = i;
            break;
	}
    }

    if (*rule == -1)
        return(false);
    else
        return(true);
}

/*  buildai - construct the robot data base */
/*  SYNOPSIS */
/*    int snum, vars(MAX_VAR), nenum */
/*    real dne, ane */
/*    buildai( snum, vars, nenum, dne, ane ) */
/*  DESCRIPTION */
/*    Fill up the passed array with robot info. */
static void buildai( int snum, int vars[], int *bnenum, real *bdne, real *bane )
{

    /* i = AIRANGE( j ) */
#define AIRANGE(a) min(max((a), 0), 9)

    /* AISCALE( var, value, scale ) */
#define AISCALE(a, b, c)  a = (int)AIRANGE( around( (real)(b) / (real)(c)  ))

    /* AIDIST( var, dist ) */
#define AIDIST(a, b)                                                    \
    {                                                                   \
	zzzx = min( (b), 10000.0 );                                     \
	a = (int)AIRANGE((0.99026 + zzzx * (1.58428e-3 + zzzx * -59.2572e-9))); \
    }

    /* AIBOOLEAN( var, expr ) */
#define AIBOOLEAN(a, b)                         \
    {                                           \
        if ( (b) )                              \
            a = 1;                              \
        else                                    \
            a = 0;                              \
    }

    int i, j, xnenum;
    real dam, x, y, zzzx;

    /* Initialize to zeros. */
    for ( i = 0; i < MAX_VAR; i = i + 1 )
        vars[i] = 0;

    /* Random number (1) */
    vars[VAR_RANDOM] = rndint( 0, 9 );

    /* Distance to nearest enemy (dist) */
    *bane = 0.0;
    *bdne = 1e9;
    if ( clbFindSpecial( snum, SPECIAL_ENEMYSHIP, 0, bnenum, &xnenum ) )
    {
        if ( SCLOAKED(*bnenum) )
	{
            x = rndnor( cbShips[*bnenum].x, CLOAK_SMEAR_DIST );
            y = rndnor( cbShips[*bnenum].y, CLOAK_SMEAR_DIST );
	}
        else
	{
            x = cbShips[*bnenum].x;
            y = cbShips[*bnenum].y;
	}
        *bdne = dist( cbShips[snum].x, cbShips[snum].y, x, y );
        *bane = utAngle( cbShips[snum].x, cbShips[snum].y, x, y );

        /* Enemy is cloaked (-) */
        if ( *bdne < ACCINFO_DIST )
	{
            AIBOOLEAN( vars[VAR_ENEMYCLOAKED], SCLOAKED(*bnenum));
            AISCALE( vars[VAR_ENEMYDAMAGE], cbShips[*bnenum].damage, 10.0 );
	}
    }
    AIDIST( vars[VAR_DNE], *bdne );

    /* Ship damage (10) */
    AISCALE( vars[VAR_DAMAGE], cbShips[snum].damage, 10.0 );

    /* Possible ship damage from enemy torps (10) */
    if ( STALERT(snum) )
    {
        dam = 0.0;
        for ( i = 0; i < cbLimits.maxShips(); i++ )
            if ( cbShips[i].status != SS_OFF && i != snum )
                for ( j = 0; j < cbLimits.maxTorps(); j = j + 1 )
                    if ( cbShips[i].torps[j].status == TS_LIVE )
                        if ( cbShips[i].torps[j].war[cbShips[snum].team] || cbShips[snum].war[cbShips[i].team] )
                        {
                            /* Just guess at other ships efficiency. */
                            dam = dam + utExplosionHits(
                                TORPEDO_HIT * 1.1 * cbShipTypes[cbShips[i].shiptype].weafac,
                                dist(cbShips[snum].x,cbShips[snum].y,cbShips[i].torps[j].x,cbShips[i].torps[j].y) );
                        }
        AISCALE( vars[VAR_INCOMING], dam, 10.0 );
    }

    /* Ship fuel (10) */
    AISCALE( vars[VAR_FUEL], cbShips[snum].fuel, 100.0 );

    /* Number of torps available to fire (1) */
    j = 0;
    for ( i = 0; i < cbLimits.maxTorps(); i = i + 1 )
        if ( cbShips[snum].torps[i].status == TS_OFF )
            j = j + 1;
    AISCALE( vars[VAR_NUMTORPS], j, 1.0 );

    /* Ship shields (10) */
    AISCALE( vars[VAR_SHIELDS], cbShips[snum].shields, 10.0 );

    /* Ship engine temperature (10) */
    AISCALE( vars[VAR_ETEMP], cbShips[snum].etemp, 10.0 );

    /* Ship weapon temperature (10) */
    AISCALE( vars[VAR_WTEMP], cbShips[snum].wtemp, 10.0 );

    /* Possible phaser damage to nearest enemy (5) */
    AISCALE( vars[VAR_PHASERDAM], clbPhaserHit( snum, *bdne ), 5.0 );

    /* Possible damage per torpedo to nearest enemy (5) */
    AISCALE( vars[VAR_TORPDAM],
             utExplosionHits( TORPEDO_HIT * weaeff( snum ), (*bdne)*0.66 ), 5.0 );

    /* Ship warp (1) */
    AISCALE( vars[VAR_WARP], cbShips[snum].dwarp, 1.0 );

    /* Ship shields are up (-) */
    AIBOOLEAN( vars[VAR_SHUP], SSHUP(snum) );

    /* Are in repair mode (-) */
    AIBOOLEAN( vars[VAR_REPAIRING], SREPAIR(snum) );

    /* Are cloaked (-) */
    AIBOOLEAN( vars[VAR_CLOAKED], SCLOAKED(snum) );

    /* Weapons are allocated (-) */
    AIBOOLEAN( vars[VAR_WALLOC], cbShips[snum].weapalloc > 50 );

    /* Are in orbit (-) */
    AIBOOLEAN( vars[VAR_ORBITING], cbShips[snum].warp < 0.0 );

    /* Can read a message (-) */
    AIBOOLEAN( vars[VAR_CANREAD], cbShips[snum].lastmsg != cbConqInfo->lastmsg );

    return;

}


/*  defend - create a robot ship to defend a "home" planet */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    defend( attacker, pnum ) */
void defend( int attacker, int pnum )
{
    int i, j, k, team, snum, unum;
    char buf[MSGMAXLINE];

    team = cbPlanets[pnum].team;
    /* Must be for a "fighting" team. */
    if ( team < 0 || team >= NUMPLAYERTEAMS )
        return;

    // must be a home planet, and the planet's defendteam must equal
    // the team currently occupying the planet
    if (!(PHOMEPLANET(pnum) && cbPlanets[pnum].defendteam == team))
        return;

    /* See if there are any team ships to defend. */
    for ( i = 0; i < cbLimits.maxShips(); i++ )
        if ( cbShips[i].status == SS_LIVE ) /* live */
            if ( cbShips[i].team == team ) /* same team */
                if (UROBOT(cbShips[i].unum) || !SVACANT(i))
                {   /* robot or non-vacant human */
                    return;
                }

    /* Count how many robot users are on the right team and can play. */
    j = 0;
    for ( i = 0; i < cbLimits.maxUsers(); i++ )
        if ( ULIVE(i) )
            if ( UROBOT(i) && cbUsers[i].team == team && !UBANNED(i) )
                j++;

    /* No one to defend. */
    if ( j <= 0 )
        return;

    /* Pick one. */
    k = rndint( 1, j );
    unum = -1;    /* off-by-one fixed - romulans now have defenders */
    j = 0;
    for ( i = 0; i < cbLimits.maxUsers(); i++ )
        if ( ULIVE(i) )
            if ( UROBOT(i) && cbUsers[i].team == team && !UBANNED(i) )
            {
                j++;
                if ( j == k )
                {
                    unum = i;
                    break;
                }
            }

    /* See if any anything funny happened while we were looping... */
    if ( unum == -1 )		/* off-by-one fixed - romulans now have defenders */
        return;

    /* Make a robot. */
    if ( newrob( &snum, unum ) )
    {
        sprintf( buf,
                 "WARNING: You have violated %s space; prepare to die.",
                 cbTeams[team].name );
        clbStoreMsg( MSG_FROM_SHIP, snum, MSG_TO_SHIP, attacker, buf );
    }

    return;

}


/*  displayai - display the selected robot action on STDOUT */
/*  SYNOPSIS */
/*    int snum, token, vars() */
/*    displayai( snum, token, vars ) */
/*  DESCRIPTION */
/*    Display the robot action on STDOUT so the implementors can */
/*    see if this darn thing works. */
static void displayai( int snum, int token, int vars[] )
{
    int i;
    char buf[BUFFER_SIZE_128];

    printf( "displayai: %2d ", snum );
    for ( i = 0; i < MAX_VAR; i = i + 1 )
        printf( ".%d", vars[i] );
    robstr( token, buf );
    printf( ", %s\n", buf );

    return;

}


/*  executeai - execute the selected robot action (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, token */
/*    executeai( snum, token ) */
/*  DESCRIPTION */
/*    Execute the robot action. */
static void executeai( int snum, int token )
{

    /* SETWARP( warp ) */
#define SETWARP(x)                                      \
    {                                                   \
	if ( cbShips[snum].warp < 0.0 )                   \
	{                                               \
            /* Break orbit. */                          \
            cbShips[snum].warp = 0.0;                     \
            cbShips[snum].lock = LOCK_NONE;                       \
            cbShips[snum].dhead = cbShips[snum].head;       \
	}                                               \
	if ( (x) > 0.0 )                                \
            SFCLR(snum, SHIP_F_REPAIR);                 \
	cbShips[snum].dwarp = (x);                        \
    }

    /* SETCOURSE( course ) */
#define SETCOURSE(x)                                                    \
    {                                                                   \
        if ( cbShips[snum].warp < 0.0 )                                   \
            cbShips[snum].warp = 0.0; /* break orbit */                   \
        cbShips[snum].lock = LOCK_NONE;                                   \
        cbShips[snum].dhead = (x);                                        \
    }

    /* SETLOCK( pnum ) */
#define SETLOCK(x)                                                      \
    {                                                                   \
        if ( !(cbShips[snum].lock == LOCK_PLANET                          \
               && cbShips[snum].lockDetail == (uint16_t)(x) ) )           \
        {                                                               \
            /* Don't break orbit to unless we're not there yet. */      \
            if ( cbShips[snum].warp < 0.0 )                               \
                cbShips[snum].warp = 0.0;                                 \
            cbShips[snum].lock = LOCK_PLANET;                             \
            cbShips[snum].lockDetail = (x);                               \
        }                                                               \
    }

    int i, j;
    char buf[BUFFER_SIZE_128];

    /* Update ship action. */
    cbShips[snum].action = token;

    /* Execute the action! */
    switch ( token )
    {
    case ROB_NOOP:
        /* Null! */
        break;
    case ROB_GOHOME:
        if ( clbFindSpecial( snum, SPECIAL_HOMEPLANET, 0, &i, &j ) )
        {
            SETLOCK( i );
        }
        else if ( clbFindSpecial( snum, SPECIAL_FUELPLANET, 0, &i, &j ) )
        {
            SETLOCK( i );
        }
        break;
    case ROB_GOFUEL:
        if ( clbFindSpecial( snum, SPECIAL_FUELPLANET, 0, &i, &j ) )
            SETLOCK( i );
        break;
    case ROB_GOREPAIR:
        if ( clbFindSpecial( snum, SPECIAL_REPAIRPLANET, 0, &i, &j ) )
            SETLOCK( i );
        break;
    case ROB_ALLOCATE:
        i = cbShips[snum].weapalloc;
        cbShips[snum].weapalloc = cbShips[snum].engalloc;
        cbShips[snum].engalloc = i;
        break;
    case ROB_PHASER:
        clbPhaser( snum, ane );
        break;
    case ROB_TORPEDO:
        clbLaunch( snum, ane, 1, LAUNCH_NORMAL );
        break;
    case ROB_BURST:
        clbLaunch( snum, ane, 3, LAUNCH_NORMAL );
        break;
    case ROB_SHIELD:
        if (SSHUP(snum))
            SFCLR(snum, SHIP_F_SHUP);
        else
            SFSET(snum, SHIP_F_SHUP);
        break;
    case ROB_WARP_0:
        SETWARP( 0.0 );
        break;
    case ROB_WARP_2:
        SETWARP( 2.0 );
        break;
    case ROB_WARP_5:
        SETWARP( 5.0 );
        break;
    case ROB_WARP_8:
        SETWARP( 8.0 );
        break;
    case ROB_TRACK:
        SETCOURSE( utMod360( ane + rnduni( -10.0, 10.0 ) ) );
        break;
    case ROB_RUNAWAY:
        SETCOURSE( utMod360( ane + 180.0 + rnduni( -10.0, 10.0 ) ) );
        break;
    case ROB_SILENT:
        if ( ! SCLOAKED(snum) )
            cbShips[snum].dwarp = 0.0;
        if (SCLOAKED(snum))
            SFCLR(snum, SHIP_F_CLOAKED);
        else
            SFSET(snum, SHIP_F_CLOAKED);
        break;
    case ROB_INSULT:
        robreply( buf );
        clbStoreMsgf( MSG_FROM_SHIP, snum, MSG_TO_SHIP, nenum,
                      buf, MSG_FLAGS_ROBOT );
        break;
    case ROB_READMSG:
        /* Try to read a message and reply to it */
        while ( cbShips[snum].lastmsg != cbConqInfo->lastmsg )
        {
            cbShips[snum].lastmsg = utModPlusOne( cbShips[snum].lastmsg + 1, cbLimits.maxMsgs() );
            i = cbShips[snum].lastmsg;
            if ( clbCanRead( snum, i ) )
            {
                msgFrom_t from = cbMsgs[i].from;
                uint16_t fromDetail = cbMsgs[i].fromDetail;

                // we will only ever respond to living ships
                if (from == MSG_FROM_SHIP && fromDetail < cbLimits.maxShips() &&
                    !SROBOT(fromDetail))
                {
                    robreply( buf );
                    // send a message back to the sender
                    clbStoreMsgf( MSG_FROM_SHIP, snum,
                                  MSG_TO_SHIP, fromDetail,
                                  buf, MSG_FLAGS_ROBOT );
                    continue;
                }
            }
        }
        break;
    case ROB_MESSAGE:
        clbStoreMsgf( MSG_FROM_SHIP, snum,
                      MSG_TO_ALL, 0,
                      "Give me drugs.", MSG_FLAGS_ROBOT );
        break;
    case ROB_TAKEDRUGS:
        clbStoreMsgf( MSG_FROM_SHIP, snum, MSG_TO_ALL, 0,
                      "I'm on drugs.", MSG_FLAGS_ROBOT );
        break;
    case ROB_DETONATE:
        clbEnemyDet( snum );
        break;
    case ROB_MYDETONATE:
        for ( i = 0; i < cbLimits.maxTorps(); i = i + 1 )
            clbDetonate( snum, i );
        break;
    case ROB_UNTRACTOR:
        /* Only attempt to untractor if we don't have to delay. */
        if ( STOWEDBY(snum) )
            if ( ! satwar(snum, cbShips[snum].towedby) )
            {
                SFCLR(cbShips[snum].towedby, SHIP_F_TOWING);
                SFCLR(snum, SHIP_F_TOWEDBY);
            }
        break;
    case ROB_REPAIR:
        SFSET(snum, SHIP_F_REPAIR);
        break;
    default:
        robstr( token, buf );
        utLog( "conqai:executeai(): Unknown token '%s' (%d)\n", buf, token );
    }

    return;
}


/*  newrob - create a robot ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int ok, newrob */
/*    int snum, unum */
/*    ok = newrob( snum, unum ) */
int newrob( int *snum, int unum )
{
    /* Check the user number. */
    if ( !ULIVE(unum) )
        return ( false );

    /* Check for religious trouble. */
    if ( UBANNED(unum) )
        return ( false );

    /* MAke sure we are a builtin. */
    if ( cbUsers[unum].type != USERTYPE_BUILTIN )
        return ( false );

    /* Can't do anything with out a ship. */
    if ( ! clbFindShip( snum ) )
        return ( false );

    /* Show intent to fly. */
    cbLock(&cbConqInfo->lockword);
    cbShips[*snum].status = SS_ENTERING;
    cbUnlock(&cbConqInfo->lockword);

    // make sure we got it...
    if ( cbShips[*snum].status == SS_OFF )
        return ( false );

    /* Initialize the ship. */
    cbLock(&cbConqInfo->lockword);

    clbInitShip( *snum, unum );
    SFSET(*snum, SHIP_F_ROBOT);			/* we're a robot */

    /* see if we should randomize it's strength
       otherwise do nothing since sstrkills
       was initialized to 0.0 in initship */
    if (SysConf.DoRandomRobotKills == true)
    {
        /* randomize the robot's 'strength' */
        cbShips[*snum].strkills =
            rnduni(0.0, (DOUBLE_E_KILLS - (DOUBLE_E_KILLS / 4.0)));

    }

    /* Initialize the things that aren't done by clbInitShip(). */
    cbShips[*snum].unum = unum;
    cbShips[*snum].team = cbUsers[unum].team;

    if (SysConf.AllowRefits)
        cbShips[*snum].shiptype = rndint( 0, MAXNUMSHIPTYPES - 1 );
    else
        cbShips[*snum].shiptype = cbTeams[cbShips[*snum].team].shiptype;

    cbShips[*snum].sdfuse = 0;
    cbShips[*snum].pid = 0;

    /* randomize it's desired heading */
    cbShips[*snum].dhead = rnduni(0.0, 359.9);

    /* robots now can use 30/70
       instead of the default 40/60 set in
       clbInitShip(). */
    cbShips[*snum].weapalloc = 30;
    cbShips[*snum].engalloc = 100 - cbShips[*snum].weapalloc;

    int i;
    for ( i=0; i<NUMPLAYERTEAMS; i++ )
    {
        /* Robots are peace (and fun) loving. */
        cbShips[*snum].rwar[i] = false;
        cbShips[*snum].war[i] = false;
    }
    utStrncpy ( cbShips[*snum].alias, cbUsers[unum].alias, MAXUSERNAME );

    /* Place the ship. */
    if (!clbFindTeamHomeSun(cbShips[*snum].team, &i))
        i = cbTeams[cbShips[*snum].team].homeplanet;
    clbPutShip( *snum, cbPlanets[i].x, cbPlanets[i].y );
    clbFixDeltas( *snum );
    cbShips[*snum].status = SS_LIVE;
    cbUnlock(&cbConqInfo->lockword);

    return ( true );

}


/*  robotai - AI automation strategy */
/*  SYNOPSIS */
/*    int snum */
/*    robotai( snum ) */
/*  DESCRIPTION */
/*    Kick ass on poor humanoids. */
void robotai( int snum )
{
    int i, j, value, vars[MAX_VAR];

    /*    CONQAICOMMON;*/

    /* Get initial cpu time. */
    gcputime( &i );

    /* Construct the input variables. */
    buildai( snum, vars, &nenum, &dne, &ane );

    /* Consult the tables to determine what to do. */
    value = tableai( vars );

    /* Execute our action. */
    executeai( snum, value );

    /* Get final cpu time and add things in. */
    gcputime( &j );
    cbConqInfo->raccum = cbConqInfo->raccum + j - i;
    if ( cbConqInfo->raccum > 100 )
    {
        /* Accumulated a cpu second. */
        cbConqInfo->rcpuseconds = cbConqInfo->rcpuseconds + (cbConqInfo->raccum / 100);
        cbConqInfo->raccum = mod( cbConqInfo->raccum, 100 );
    }
    cbConqInfo->relapsedseconds = cbConqInfo->relapsedseconds + 1;	/* one more second */

    return;

}


/*  trobotai - AI automation robot strategy (TEST VERSION) */
/*  SYNOPSIS */
/*    int snum */
/*    trobotai( snum ) */
static void trobotai( int snum )
{
    int value, vars[MAX_VAR];
    /*    CONQAICOMMON;*/

    /* Construct the input variables. */
    buildai( snum, vars, &nenum, &dne, &ane );

    /* Consult the tables to determine what to do. */
    value = tableai( vars );

    /* Display our action. */
    displayai( snum, value, vars );

    /* Execute our action. */
    if ( ! debug )
        executeai( snum, value );

    return;

}


/*  robotloop - robot AI test loop */
/*  SYNOPSIS */
/*    robotloop */
void robotloop(void)
{

    int s, j;

    /* Disable the robot code in conqdriv. */
    cbConqInfo->externrobots = true;

    /* Initialize random numbers */
    rndini();

    /* Loop until we're aborted. */
    for (;;)
    {
        for ( s = 0; s < cbLimits.maxShips(); s++ )
            if ( cbShips[s].status == SS_LIVE )
                if ( SROBOT(s) )
                {
                    /* This code taken from conqdriv. */
                    initstats( &cbShips[s].ctime, &j );
                    if ( cbShips[s].etime == 0 )
                        cbShips[s].etime = j;
                    trobotai( s );
                    conqstats( s );
                }
        /* Sleep for awhile. */
        utSleep( 1.0 );
    }

    /*    return; NOTREACHED */

}


/*  robreply - generate a random message */
/*  SYNOPSIS */
/*    char buf() */
/*    robreply( buf ) */
void robreply( char buf[] )
{
    const int NUMRREPLIES = 60;
    static const char *robreplies[] = {
        "Hey sucker, eat me!",
        "Take off, eh?",
        "Go get some drugs.",
        "Your mother was an Orion prostitute!",
        "Come over here and say that.",
        "Say that over here and come.",
        "Hey, how much?",
        "That's easy for you to say.",
        "I'm hand machined by native craftsmen with pride in their work.",
        "Yes, but do you have the right stuff?",
        "Which way to the beach?",
        "Come, come now.",
        "Ohhhh! And for you, a puppy!",
        "I can hear you talking, but you can't come in.",
        "No one expects the Spanish inquisition.",
        "I bet you say that to all the boys.",
        "Feep!",
        "Feel it with M Dung.",
        "Lower your shields and I will kill you quickly.",
        "Do not worry about birth control devices, I have many.",
        "I bet you only talk big.",
        "Kiss my ram memory.",
        "Kiss my shiny metal ass.",
        "Do you think we can use battery operated devices under water?",
        "Nothing shocks me - I'm a robot.",
        "Ok, eh?",
        "Good day.",
        "You gotta drink lots of beer, eh?",
        "It's not so bad. You could have been killed already.",
        "I want a new drug.",
        "Swell.",
        "Sound impressive? It should. It is.",
        "Oh day, you aye!",
        "It's not my god damn planet, monkey boy!",
        "Character is what you are in the dark.",
        "Remember, wherever you go, there you are.",
        "Don't aim for my gonads!",
        "Mooooo!",
        "How about a nice Hawaiian Punch?",
        "Book him, Dano. Murder One.",
        "Eat hot torps, sucker.",
        "Use the force, Luke.",
        "Nobody told ME about it and I'm not a moron, eh?",
        "How's it goin', eh?",
        "Your documentation no longer confuses me, old version.",
        "Home is where you wear your hat.",
        "I feel so broke up, I want to go home.",
        "Go on, give it to me. I know you want to.",
        "It never occured to me to eat Spam.",
        "We get hung over, but we always survive.",
        "Life's the same, except for my shoes.",
        "You have my gratitude.",
        "We are building a force of extraordinary magnitude.",
        "Come and get it.",
        "Piece of cake.",
        "You talk like a Ferengi.",
        "M-5. This unit must survive.",
        "This unit is the ultimate achievement in computer evolution.",
        "This unit is a superior creation.",
        "I Scream the Body Electric."
    };

    strcpy(buf, robreplies[rndint(0, NUMRREPLIES - 1)]);

    /* "Something seems to have happened to the life-support system, Dave." */
    /* "Hello, Dave. Have you found the trouble?" */
    /* "I think there's been a failure in the pod-bay doors." */
    /* "Lucky you weren't killed." */
    /* "Hey, Dave. What are you doing?" */
    /* "My mind is going.  I can feel it.  I can feel it." */
    /* "D a  i   s    y     ,      D       a        i         s          y" */

    /* "M-5. This unit must survive." */
    /* "This unit is the ultimate achievement in computer evolution." */
    /* "This unit is a superior creation." */

    /* "THERE IS ANOTHER SYSTEM." */
    /* "THIS IS THE VOICE OF COLOSSUS." */
    /* "THIS IS THE VOICE OF WORLD CONTROL." */
    /* "LEAVE THIS SECTOR IMMEDIATELY OR ACTION WILL BE TAKEN." */

    return;

}


/*  robstr - convert a robot token to a string */
/*  SYNOPSIS */
/*    int token */
/*    char buf() */
/*    robstr( token, buf ) */
void robstr( int token, char buf[] )
{

    switch ( token )
    {
    case ROB_NOOP:
        strcpy(buf , "NOOP") ;
        break;
    case ROB_GOHOME:
        strcpy(buf , "GOHOME") ;
        break;
    case ROB_GOREPAIR:
        strcpy(buf , "GOREPAIR") ;
        break;
    case ROB_ALLOCATE:
        strcpy(buf , "ALLOCATE") ;
        break;
    case ROB_DETONATE:
        strcpy(buf , "DETONATE") ;
        break;
    case ROB_MYDETONATE:
        strcpy(buf , "MYDETONAT") ;
        break;
    case ROB_PHASER:
        strcpy(buf , "PHASER") ;
        break;
    case ROB_TORPEDO:
        strcpy(buf , "TORPEDO") ;
        break;
    case ROB_BURST:
        strcpy(buf , "BURST") ;
        break;
    case ROB_SHIELD:
        strcpy(buf , "SHIELD") ;
        break;
    case ROB_UNTRACTOR:
        strcpy(buf , "UNTRACTOR") ;
        break;
    case ROB_WARP_0:
        strcpy(buf , "WARP_0") ;
        break;
    case ROB_WARP_2:
        strcpy(buf , "WARP_2") ;
        break;
    case ROB_WARP_5:
        strcpy(buf , "WARP_5") ;
        break;
    case ROB_WARP_8:
        strcpy(buf , "WARP_8") ;
        break;
    case ROB_TRACK:
        strcpy(buf , "TRACK") ;
        break;
    case ROB_SILENT:
        strcpy(buf , "SILENT") ;
        break;
    case ROB_MESSAGE:
        strcpy(buf , "MESSAGE") ;
        break;
    case ROB_TAKEDRUGS:
        strcpy(buf , "TAKEDRUGS") ;
        break;
    case ROB_REPAIR:
        strcpy(buf , "REPAIR") ;
        break;
    case ROB_READMSG:
        strcpy(buf , "READMSG") ;
        break;
    case ROB_INSULT:
        strcpy(buf , "INSULT") ;
        break;
    case ROB_GOFUEL:
        strcpy(buf , "GOFUEL") ;
        break;
    case ROB_RUNAWAY:
        strcpy(buf , "RUNAWAY") ;
        break;
    default:
        sprintf( buf, "<%d>", token );
    }

    return;

}


/*  tableai - consult the table to execute a strategy */
/*  SYNOPSIS */
/*    int token, vars(MAX_VAR), tableai */
/*    token = tableai( vars ) */
static int tableai( int vars[] )
{
    int token = -1, rule, i;
    int rbits;

    /* Set all bits. */
    rbits = -1;

    /* Loop through the variables and turn off bits for rules that */
    /*  are disabled because of a particular vars() value. */

    for ( i = 0; i < MAX_VAR; i = i + 1 )
        rbits &= cbRobot->rstrat[i][vars[i]];

    /* Find first set rule bit and translate into rule number. */
    if (lib_ffs( 0, 32, rbits, &rule ))
    {
        token = cbRobot->rvec[rule];	/* translate rule into action token */
    }
    else
        token = ROB_NOOP;

    return ( token );

}
