#include "c_defs.h"

/************************************************************************
 * conquest curses using stuff
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/


#include "conqdef.h"
#include "cb.h"
#include "conqlb.h"
#include "rndlb.h"
#include "conqutil.h"
#include "context.h"

#include "conf.h"
#include "global.h"
#include "color.h"
#include "cd2lb.h"
#include "iolb.h"
#include "cumisc.h"
#include "ui.h"

static char cbuf[BUFFER_SIZE_1024]; /* general purpose buffer */

/*  histlist - display the last usage list */
/*  SYNOPSIS */
/*    int godlike */
/*    mcuHistList( godlike ) */
void mcuHistList( int godlike )
{
    int i, j, lin, col, fline, lline, thistptr = 0;
    int ch;
    static const char *hd0="C O N Q U E S T   U S E R   H I S T O R Y";
    char connecttm[BUFFER_SIZE_256];
    char histentrytm[MAXDATESIZE + 1];
    char puname[MAXUSERNAME];

    /* Do some screen setup. */
    cdclear();
    fline = 1;
    lline = MSG_LIN1 - 1;
    cprintf(fline,0,ALIGN_CENTER,"#%d#%s",LabelColor, hd0);
    fline = fline + 2;

    while (TRUE) /* repeat */
    {
        if ( ! godlike )
            if ( ! clbStillAlive( Context.snum ) )
                break;

        thistptr = cbConqInfo->histptr;
        lin = fline;
        col = 1;
        cdclrl( fline, lline - fline + 1 );

        i = thistptr + 1;
        for ( j = 0; j < MAXHISTLOG; j++ )
	{
            bool isResigned = false;

            // adjust based on histptr being the next available slot.
            i = utModPlusOne( i - 1, MAXHISTLOG );

            // No username, no entry
            if (!cbHistory[i].username[0])
                continue;

            if (cbHistory[i].unum < 0) // resigned
                isResigned = true;

            utStrncpy(puname, cbHistory[i].username, MAXUSERNAME);

            /* entry time */
            utFormatTime( histentrytm, cbHistory[i].enterTime);

            /* now elapsed time */
            utFormatSeconds((int) cbHistory[i].elapsed, connecttm);
            /* strip off seconds, or for long times, anything after 7 bytes */
            connecttm[7] = '\0';

            cprintf( lin, col, ALIGN_NONE,
                     "#%d#%-10.10s #%d#%16s#%d#-#%d#%7s",
                     (isResigned) ? CyanColor : YellowLevelColor,
                     puname,
                     GreenLevelColor,
                     histentrytm,
                     NoColor,
                     RedLevelColor,
                     connecttm);

            lin++;
            if ( lin > lline )
	    {
                col = 40;
                lin = fline;
	    }
	}

        mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
        cdrefresh();
        if ( iogtimed( &ch, 1.0 ) )
            break;				/* exit loop if we got one */
    }

    return;

}



/*  puthing - put an object on the display */
/*  SYNOPSIS */
/*    int what, lin, col */
/*    mcuPutThing( what, lin, col ) */
void mcuPutThing( int what, int lin, int col )
{
    int i, j, tlin, tcol;
    char buf[3][7];

    switch ( what )
    {
    case PLANET_SUN:
        strcpy(buf[0] , " \\|/ ") ;
        strcpy(buf[1] , "-- --") ;
        strcpy(buf[2] , " /|\\ ") ;
        break;
    case PLANET_CLASSM:
    case PLANET_CLASSA:
    case PLANET_CLASSO:
    case PLANET_CLASSZ:
    case PLANET_DEAD:
    case PLANET_GHOST:
        strcpy(buf[0], " .-. ") ;
        strcpy(buf[1], "(   )");
        strcpy(buf[2], " `-' ") ;
        break;
    case PLANET_MOON:
        strcpy(buf[0], "     ") ;
        strcpy(buf[1], " ( ) ");
        strcpy(buf[2], "     ") ;
        break;
    case THING_EXPLOSION:
        strcpy(buf[0], " %%% ") ;
        strcpy(buf[1], "%%%%%") ;
        strcpy(buf[2], " %%% ") ;
        break;
    default:
        strcpy(buf[0], " ??? ") ;
        strcpy(buf[1], "?????") ;
        strcpy(buf[2], " ??? ") ;
        break;
    }

    for ( j=0; j<3; j++ )
    {
        tlin = lin + j - 1;
        /*	tlin = lin + j - 2; */
        if ( tlin >= 0 && tlin <= DISPLAY_LINS )
            for ( i = 0; i < 6; i = i + 1 )
            {
                tcol = col + i - 1;
                /*	      tcol = col + i - 3;*/
                if ( tcol > STAT_COLS && tcol <= Context.maxcol - 1 )
                    if (buf[j][i] != '\0')
                        cdput( buf[j][i], tlin, tcol );
            }
    }

    return;

}


/*  readmsg - display a message */
/*  SYNOPSIS */
/*    int snum, msgnum */
/*    mcuReadMsg( snum, msgnum ) */
int mcuReadMsg( int msgnum, int dsplin )
{
    char buf[MSGMAXLINE];
    unsigned int attrib = 0;


    buf[0] = '\0';

    if (Context.hascolor)
    {				/* set up the attrib so msg's are cyan */
        attrib = CyanColor;
    }

    clbFmtMsg(cbMsgs[msgnum].from, cbMsgs[msgnum].fromDetail,
              cbMsgs[msgnum].to, cbMsgs[msgnum].toDetail, buf);
    strcat(buf , ": ") ;
    strcat(buf , cbMsgs[msgnum].msgbuf) ;

    uiPutColor(attrib);
    mcuPutMsg( buf, dsplin );
    uiPutColor(0);
    /* clear second line if sending to MSG_LIN1 */
    if (dsplin == MSG_LIN1)
    {
        cdclrl( MSG_LIN2, 1 );
    }

    return(TRUE);

}

/* convert a KP key into an angle */
int mcuKPAngle(int ch, real *angle)
{
    int rv = FALSE;

    switch (ch)
    {
    case KEY_HOME:
    case KEY_A1:		/* KP upper left */
        *angle = 135.0;
        rv = TRUE;
        break;
    case KEY_PPAGE:
    case KEY_A3:		/* KP upper right */
        *angle = 45.0;
        rv = TRUE;
        break;
    case KEY_END:
    case KEY_C1:		/* KP lower left */
        *angle = 225.0;
        rv = TRUE;
        break;
    case KEY_NPAGE:
    case KEY_C3:		/* KP lower right */
        *angle = 315.0;
        rv = TRUE;
        break;
    case KEY_UP:		/* up arrow */
        *angle = 90.0;
        rv = TRUE;
        break;
    case KEY_DOWN:		/* down arrow */
        *angle = 270.0;
        rv = TRUE;
        break;
    case KEY_LEFT:		/* left arrow */
        *angle = 180.0;
        rv = TRUE;
        break;
    case KEY_RIGHT:		/* right arrow */
        *angle = 0.0;
        rv = TRUE;
        break;
    default:
        rv = FALSE;
        break;
    }

    return(rv);
}

/* convert a KP key into a 'dir' key */
int mcuKP2DirKey(int *ch)
{
    int rv;
    char cch;

    switch (*ch)
    {
    case KEY_HOME:
    case KEY_A1:		/* KP upper left */
        cch = 'q';
        rv = TRUE;
        break;
    case KEY_PPAGE:
    case KEY_A3:		/* KP upper right */
        cch = 'e';
        rv = TRUE;
        break;
    case KEY_END:
    case KEY_C1:		/* KP lower left */
        cch = 'z';
        rv = TRUE;
        break;
    case KEY_NPAGE:
    case KEY_C3:		/* KP lower right */
        cch = 'c';
        rv = TRUE;
        break;
    case KEY_UP:		/* up arrow */
        cch = 'w';
        rv = TRUE;
        break;
    case KEY_DOWN:		/* down arrow */
        cch = 'x';
        rv = TRUE;
        break;
    case KEY_LEFT:		/* left arrow */
        cch = 'a';
        rv = TRUE;
        break;
    case KEY_RIGHT:		/* right arrow */
        cch = 'd';
        rv = TRUE;
        break;
    default:
        cch = (char)0;
        rv = FALSE;
        break;
    }

    if ((int)cch != 0)
        *ch = (char)cch;

    return(rv);
}


/*  infoplanet - write out information about a planet */
/*  SYNOPSIS */
/*    char str() */
/*    int pnum, snum */
/*    mcuInfoPlanet( str, pnum, snum ) */
void mcuInfoPlanet( const char *str, int pnum, int snum )
{
    int i, j;
    int godlike, canscan;
    char buf[MSGMAXLINE*2], junk[MSGMAXLINE];
    real x, y;

    /* Check range of the passed planet number. */
    if ( pnum < 0 || pnum >= MAXPLANETS )
    {
        mcuPutMsg( "No such planet.", MSG_LIN1 );
        cdclrl( MSG_LIN2, 1 );
        cdmove( MSG_LIN1, 1 );
        utLog("infoplanet: Called with invalid pnum (%d).", pnum );
        return;
    }

    /* GOD is too clever. */
    godlike = ( snum < 0 || snum >= MAXSHIPS );

    /* In some cases, report hostilities. */
    junk[0] = 0;
    if ( cbPlanets[pnum].type == PLANET_CLASSM || cbPlanets[pnum].type == PLANET_DEAD )
        if ( ! godlike )
            if ( cbPlanets[pnum].scanned[cbShips[snum].team] && clbSPWar( snum, pnum ) )
                strcat(junk, " (hostile)");

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

    Context.lasttdist = round(dist( x, y, cbPlanets[pnum].x, cbPlanets[pnum].y));
    Context.lasttang = round(utAngle( x, y, cbPlanets[pnum].x, cbPlanets[pnum].y ));

    if (UserConf.DoETAStats)
    {
        static char tmpstr[64];

        if (cbShips[snum].warp > 0.0)
	{
            sprintf(tmpstr, ", ETA %s",
                    clbETAStr(cbShips[snum].warp,
                              Context.lasttdist));
	}
        else
            tmpstr[0] = '\0';

        sprintf( buf, "%s%s, a %s%s, range %d, direction %d%s",
                 str,
                 cbPlanets[pnum].name,
                 cbConqInfo->ptname[cbPlanets[pnum].type],
                 junk,
                 Context.lasttdist,
                 Context.lasttang,
                 tmpstr);


    }
    else
        sprintf( buf, "%s%s, a %s%s, range %d, direction %d",
                 str,
                 cbPlanets[pnum].name,
                 cbConqInfo->ptname[cbPlanets[pnum].type],
                 junk,
                 Context.lasttdist,
                 Context.lasttang);

    /* save for hudInfo (only first 3 chars) */
    utStrncpy(Context.lasttarg, cbPlanets[pnum].name, 4);

    if ( godlike )
        canscan = TRUE;
    else
        canscan = cbPlanets[pnum].scanned[cbShips[snum].team];

    junk[0] = 0;
    if ( cbPlanets[pnum].type != PLANET_SUN && cbPlanets[pnum].type != PLANET_MOON )
    {
        if ( ! canscan )
            strcpy(junk , "with unknown occupational forces") ;
        else
	{
            i = cbPlanets[pnum].armies;
            if ( i == 0 )
	    {
                j = cbPlanets[pnum].uninhabtime;
                if ( j > 0 )
                    sprintf( junk, "uninhabitable for %d more minutes", j );
                else
                    strcpy(junk , "with NO armies") ;
	    }
            else
	    {
                sprintf( junk, "with %d %s arm", i,
                         cbTeams[cbPlanets[pnum].team].name );
                if ( i == 1 )
                    strcat(junk , "y") ;
                else
                    strcat(junk , "ies") ;
	    }
	}

        /* Now see if we can tell about coup time. */
        if ( godlike )
            canscan = FALSE;	/* GOD can use teaminfo instead */
        else
            canscan = ( pnum == cbTeams[cbShips[snum].team].homeplanet &&
                        cbTeams[cbShips[snum].team].coupinfo );
        if ( canscan )
	{
            j = cbTeams[cbShips[snum].team].couptime;
            if ( j > 0 )
	    {
                if ( junk[0] != 0 )
                    strcat(junk, ", ");
                utAppendInt(junk , j) ;
                strcat(junk , " minutes until coup time") ;
	    }
	}
    }

    if ( junk[0] == 0 )
    {
        utAppendChar(buf , '.') ;
    }
    else
    {
        utAppendChar(buf, ',');
        utAppendChar(junk , '.') ;
    }

    /* Now output the info. Break the stuff in buf across two lines */
    /*  (if necessary) and force the stuff in junk (the number of */
    /*  armies for planets) to be all on the second line. */
    i = strlen( buf );				/* strlen of first part */
    j = 69;					/* desired maximum length */
    if ( i <= j )
    {
        /* The first part is small enough. */
        mcuPutMsg( buf, MSG_LIN1 );
        if ( junk[0] != 0 )
            mcuPutMsg( junk, MSG_LIN2 );
        else
            cdclrl( MSG_LIN2, 1 );
    }
    else
    {
        /* Break it into two lines. */
        i = j + 1;
        while ( buf[i] != ' ' && i > 1 )
            i = i - 1;
        utAppendChar(buf , ' ') ;
        strcat(buf , junk) ;
        buf[i] = 0;				/* terminate at blank */
        mcuPutMsg( buf, MSG_LIN1 );
        mcuPutMsg( &buf[i+1], MSG_LIN2 );
    }

    cdmove( MSG_LIN1, 1 );
    return;

}

/*  infoship - write out information about a ship */
/*  SYNOPSIS */
/*    int snum, scanner */
/*    mcuInfoShip( snum, scanner ) */
void mcuInfoShip( int snum, int scanner )
{
    int i, status;
    char junk[MSGMAXLINE];
    real x, y, dis, kills, appx, appy;
    int godlike, canscan;
    static char tmpstr[BUFFER_SIZE_256];
    real pwarp, diffdis, close_rate;
    time_t difftime, curtime;
    static time_t oldtime = 0;
    static real avgclose_rate, olddis = 0.0, oldclose_rate = 0.0;
    static int oldsnum = 0;

    godlike = ( scanner < 0 || scanner >= MAXSHIPS );

    cdclrl( MSG_LIN1, 2 );
    if ( snum < 0 || snum >= MAXSHIPS )
    {
        mcuPutMsg( "No such ship.", MSG_LIN1 );
        cdmove( MSG_LIN1, 1 );
        return;
    }
    status = cbShips[snum].status;
    if ( ! godlike && status != SS_LIVE )
    {
        mcuPutMsg( "Not found.", MSG_LIN1 );
        cdmove( MSG_LIN1, 1 );
        return;
    }

    cbuf[0] = Context.lasttarg[0] = 0;
    utAppendShip(cbuf , snum) ;
    strcpy(Context.lasttarg, cbuf); /* save for hudInfo */

    if ( snum == scanner )
    {
        /* Silly Captain... */
        strcat(cbuf, ": That's us, silly!");
        mcuPutMsg( cbuf, MSG_LIN1 );
        cdmove( MSG_LIN1, 1 );
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
        canscan = TRUE;
    else
    {

        /* Decide if we can do an acurate scan. */
        canscan = ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) ||
                    ( (cbShips[snum].scanned[ cbShips[scanner].team] > 0) && ! selfwar(scanner) ) );
    }

    strcat(cbuf , ": ") ;
    if ( cbShips[snum].alias[0] != 0 )
    {
        strcat(cbuf , cbShips[snum].alias) ;
        strcat(cbuf, ", ");
    }
    kills = (cbShips[snum].kills + cbShips[snum].strkills);
    if ( kills == 0.0 )
        strcat(cbuf , "no") ;
    else
    {
        sprintf( junk, "%.1f", kills );
        strcat(cbuf , junk) ;
    }
    strcat(cbuf , " kill") ;
    if ( kills != 1.0 )
        utAppendChar(cbuf , 's') ;
    if ( SCLOAKED(snum) && ( godlike || SSCANDIST(snum) ) )
        strcat(cbuf, " (CLOAKED) ");
    else
        strcat(cbuf,  ", ");

    strcat(cbuf, "a ") ;
    strcat(cbuf, cbShipTypes[cbShips[snum].shiptype].name) ;
    strcat(cbuf, ", ");

    if ( godlike )
    {
        utAppendShipStatus(cbuf , status) ;
        utAppendChar(cbuf , '.') ;
    }
    else
    {
        if ( cbShips[snum].war[cbShips[scanner].team] )
            strcat(cbuf , "at WAR.") ;
        else
            strcat(cbuf , "at peace.") ;
    }

    mcuPutMsg( cbuf, MSG_LIN1 );

    if ( ! SCLOAKED(snum) || cbShips[snum].warp != 0.0 )
    {
        Context.lasttdist = round( dis ); /* save these puppies for hudInfo */
        Context.lasttang = round( utAngle( x, y, appx, appy ) );
        sprintf( cbuf, "Range %d, direction %d",
                 Context.lasttdist, Context.lasttang );


        if (UserConf.DoETAStats)
	{
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
                    utLog("infoship: close_rate(%.1f) = diffdis(%.1f) / difftime(%d), avgclose_rate = %.1f",
                          close_rate,
                          diffdis,
                          difftime,
                          avgclose_rate);
#endif

                    if (avgclose_rate <= 0.0)
		    {		/* dist is increasing or no change,
				   - can't ever catchup = ETA never */
                        sprintf(tmpstr, ", ETA %s",
                                clbETAStr(0.0, dis));
                        strcat(cbuf, tmpstr) ;
		    }
                    else
		    {		/* we are indeed closing... */

				/* calc psuedo-warp */
                        /* pwarp = dis / (avgclose_rate (in MM/sec) /
                           MM_PER_SEC_PER_WARP(18)) */
                        pwarp = (avgclose_rate / (real) MM_PER_SEC_PER_WARP);

#ifdef DEBUG_ETA
                        utLog("infoship:\tdis(%.1f) pwarp(%.1f) = (close_rate(%.1f) / MM_PER_SEC_PER_WARP(%.1f)", dis, pwarp, close_rate, MM_PER_SEC_PER_WARP);
#endif

                        sprintf(tmpstr, ", ETA %s",
                                clbETAStr(pwarp, dis));
                        strcat(cbuf, tmpstr) ;
		    }
		}
                else
		{		/* scanning a new ship - assume ships
				   heading directly at each other */

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

                    sprintf(tmpstr, ", ETA %s",
                            clbETAStr(pwarp, dis));
                    strcat(cbuf, tmpstr) ;
		}
	    }
	} /* if do ETA stats */
    }
    else				/* else cloaked and at w0 */
    {
        Context.lasttdist = Context.lasttang = 0;
        cbuf[0] = 0;
    }

    if ( canscan )
    {
        if ( cbuf[0] != 0 )
            strcat(cbuf,  ", ");
        strcat(cbuf , "shields ") ;
        if ( SSHUP(snum) && ! SREPAIR(snum) )
            utAppendInt(cbuf, round( cbShips[snum].shields ));
        else
            strcat(cbuf , "DOWN") ;
        i = round( cbShips[snum].damage );
        if ( i > 0 )
	{
            if ( cbuf[0] != 0 )
                strcat(cbuf, ", ");
            sprintf( junk, "damage %d", i );
            strcat(cbuf , junk) ;
	}
        i = cbShips[snum].armies;
        if ( i > 0 )
	{
            sprintf( junk, ", with %d arm", i );
            strcat(cbuf , junk) ;
            if ( i == 1 )
	    {
                utAppendChar(cbuf , 'y') ;
	    }
            else
                strcat(cbuf , "ies") ;
	}
    }
    if ( cbuf[0] != 0 )
    {
        cbuf[0] = (char)toupper( cbuf[0] );
        utAppendChar(cbuf , '.') ;
        mcuPutMsg( cbuf, MSG_LIN2 );
    }

    cdmove( MSG_LIN1, 1 );
    return;

}

/*  planlist - list planets */
/*  SYNOPSIS */
/*    int team */
/*    mcuPlanetList( team ) */
void mcuPlanetList( int team, int snum )
{
    int i, lin, col, olin, pnum;
    static int sv[MAXPLANETS];
    int cmd;
    char ch, junk[10];
    static const char *hd0="P L A N E T   L I S T   ";
    static const char *hd1="' = must take to conquer the Universe)";
    static const char *hd2="planet      type team armies          planet      type team armies";
    char hd3[BUFFER_SIZE_256];
    int outattr;
    int col2;
    int column_h = 7;
    int column_1 = 3;
    int column_2 = 41;
    char xbuf[BUFFER_SIZE_256];
    static char pd0[BUFFER_SIZE_1024];
    static int FirstTime = TRUE;
    int PlanetOffset;		/* offset into MAXPLANETS for this page */
    int PlanetIdx = 0;
    int Done;

    if (FirstTime == TRUE)
    {
        FirstTime = FALSE;

        /* build header fmt char **/
        sprintf(pd0,
                "#%d#%s#%d#%s#%d#%s#%d#%s" ,
                LabelColor,
                hd0,
                InfoColor,
                "('",
                SpecialColor,
                "+",
                InfoColor,
                hd1);

        /* sort the planets */
        for ( i = 0; i < MAXPLANETS; i++ )
            sv[i] = i;
        clbSortPlanets( sv );

    }

    strcpy( hd3, hd2 );
    for ( i = 0; hd3[i] != 0; i++ )
        if ( hd3[i] != ' ' )
            hd3[i] = '-';

    PlanetOffset = 1;
    cdclear();
    Done = FALSE;
    do
    {

        cdclra(0, 0, MSG_LIN1 + 2, Context.maxcol - 1);
        lin = 1;

        cprintf(lin, column_h, ALIGN_NONE, pd0);

        /* display column headings */
        lin += 2;
        uiPutColor(LabelColor);
        cdputc( hd2, lin );
        lin++;
        cdputc( hd3, lin );
        uiPutColor(0);
        lin++;
        olin = lin;
        col = column_1;
        col2 = FALSE;

        PlanetIdx = 0;

        if (PlanetOffset < MAXPLANETS)
	{
            while ((PlanetOffset + PlanetIdx) < MAXPLANETS)
	    {
                i = PlanetOffset + PlanetIdx;
                PlanetIdx++;
                pnum = sv[i];

                /* colorize - dwp */
                if ( snum >= 0 && snum < MAXSHIPS)
		{	/* if user has a valid ship */
                    if ( cbPlanets[pnum].team == cbShips[snum].team && !selfwar(snum) )
                        outattr = GreenLevelColor;
                    else if ( (clbSPWar(snum,pnum) && cbPlanets[pnum].scanned[cbShips[snum].team] ) ||
                              cbPlanets[pnum].type == PLANET_SUN )
                        outattr = RedLevelColor;
                    else
                        outattr = YellowLevelColor;
		}
                else
		{			/* else, user doesn't have a ship yet */
                    if (team == TEAM_NOTEAM)
		    {			/* via conqoper */
                        switch(cbPlanets[pnum].type)
			{
			case PLANET_SUN:
                            outattr = RedLevelColor;
                            break;
			case PLANET_CLASSM:
                            outattr = GreenLevelColor;
                            break;
			case PLANET_DEAD:
                            outattr = YellowLevelColor;
                            break;
			case PLANET_CLASSA:
			case PLANET_CLASSO:
			case PLANET_CLASSZ:
                            outattr = CQC_A_BOLD;
                            break;
			case PLANET_GHOST:
                            outattr = NoColor;
                            break;
			default:
                            outattr = SpecialColor;
                            break;
			}
		    }
                    else
		    {			/* via menu() */
                        if ( cbPlanets[pnum].team == cbUsers[Context.unum].team &&
                             !(cbUsers[Context.unum].war[cbUsers[Context.unum].team]))
			{
                            outattr = GreenLevelColor;
			}
                        else if ( cbPlanets[pnum].type == PLANET_SUN ||
                                  (cbPlanets[pnum].team < NUMPLAYERTEAMS &&
                                   cbUsers[Context.unum].war[cbPlanets[pnum].team] &&
                                   cbPlanets[pnum].scanned[cbUsers[Context.unum].team]) )
			{
                            outattr = RedLevelColor;
			}
                        else
			{
                            outattr = YellowLevelColor;
			}
		    }
		}

                /* Don't display unless it's real. */
                if ( ! PVISIBLE(pnum) )
                    continue;

                /* I want everything if it's real */

                /* Figure out who owns it and count armies. */
                ch =  cbTeams[cbPlanets[pnum].team].teamchar;
                sprintf( junk, "%d", cbPlanets[pnum].armies );

                /* Then modify based on scan information. */

                if ( team != TEAM_NOTEAM )
                    if ( ! cbPlanets[pnum].scanned[team] )
                    {
                        ch = '?';
                        strcpy(junk , "?") ;
                    }

                /* Suns and moons are displayed as unowned. */
                if ( cbPlanets[pnum].type == PLANET_SUN || cbPlanets[pnum].type == PLANET_MOON )
                    ch = ' ';

                /* Don't display armies for suns unless we're special. */
                if ( cbPlanets[pnum].type == PLANET_SUN )
                    if ( team != TEAM_NOTEAM )
                        junk[0] = 0;

                /* Moons aren't supposed to have armies. */
                if ( cbPlanets[pnum].type == PLANET_MOON )
		{
                    if ( team != TEAM_NOTEAM )
                        junk[0] = 0;
                    else if ( cbPlanets[pnum].armies == 0 )
                        junk[0] = 0;
		}

                char coreflag = ' ';
                char homeflag = ' ';

                /* flag planets that are required for a conq */
                if (cbPlanets[pnum].type == PLANET_CLASSM
                    || cbPlanets[pnum].type == PLANET_DEAD)
		{
                    // check for homeplanet
                    if (PHOMEPLANET(pnum))
                        homeflag = cbTeams[cbPlanets[pnum].defendteam].name[0];

                    // core planet - required for conquer
                    if (PCORE(pnum))
                        coreflag = '+';
		}

                sprintf(xbuf,"%c %c ", homeflag, coreflag);  // home/core flags
                uiPutColor(SpecialColor);
                cdputs( xbuf, lin, col );

                col+=(strlen(xbuf));
                sprintf(xbuf,"%-11s ",cbPlanets[pnum].name);  /* cbPlanets[pnum].name */
                uiPutColor(outattr);
                cdputs( xbuf, lin, col );

                col+=(strlen(xbuf));
                sprintf( xbuf, "%-4c %-3c  ",
                         cbConqInfo->chrplanets[cbPlanets[pnum].type], ch);
                cdputs( xbuf, lin, col );

                col+=(strlen(xbuf));
                sprintf(xbuf,"%4s",junk);
                if (junk[0] == '?')
                    uiPutColor(YellowLevelColor);
                else
                    uiPutColor(outattr);
                cdputs( xbuf, lin, col );
                uiPutColor(0);

                lin++;;
                if ( lin == MSG_LIN1 )
		{
                    if (col2)	/* need a new page... */
		    {
                        break; /* out of while */
		    }
                    else
		    {
                        lin = olin;
                        col2 = TRUE;
		    }
		}

                if (!col2)
                    col = column_1;
                else
                    col = column_2;

	    } /* while */

            if ((PlanetOffset + PlanetIdx) >= MAXPLANETS)
                mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
            else
                mcuPutPrompt( MTXT_MORE, MSG_LIN2 );

            cdrefresh();

            if (iogtimed( &cmd, 1.0 ))
	    {			/* got a char */
                if (cmd == 'q' || cmd == 'Q' || cmd == TERM_ABORT)
		{		/* quit */
                    Done = TRUE;
		}
                else
		{		/* some other key... */
				/* setup for new page */
                    PlanetOffset += PlanetIdx;
                    if (PlanetOffset >= MAXPLANETS)
		    {		/* pointless to continue */
                        Done = TRUE;
		    }

		}
	    }

            /* didn't get a char, update */
            if (snum >= 0 && snum < MAXSHIPS)
                if (!clbStillAlive(snum))
                    Done = TRUE;

	} /* if PlanetOffset <= MAXPLANETS */
        else
            Done = TRUE;		/* else PlanetOffset > MAXPLANETS */

    } while(Done != TRUE); /* do */

    return;

}


/*  playlist - list ships */
/*  SYNOPSIS */
/*    int godlike, doall */
/*    mcuPlayList( godlike, doall ) */
void mcuPlayList( int godlike, int doall, int snum )
{
    int i, unum, status, lin, col;
    int fline, lline, fship;
    char sbuf[20];
    char kbuf[20];
    char pidbuf[20];
    char ubuf[MAXUSERNAME + 2];
    int ch;
    static const char *hd1="ship  name          pseudonym              kills      pid";
    static const char *hd2="ship  name          pseudonym              kills     type";
    killedBy_t kb;
    uint16_t detail;

    /* Do some screen setup. */
    cdclear();
    uiPutColor(LabelColor);  /* dwp */

    if (godlike)
        strcpy(cbuf , hd1) ;
    else
        strcpy(cbuf , hd2) ;

    col = (int)(Context.maxcol - strlen( cbuf )) / (int)2;
    lin = 2;
    cdputs( cbuf, lin, col );

    for ( i = 0; cbuf[i] != 0; i = i + 1 )
        if ( cbuf[i] != ' ' )
            cbuf[i] = '-';
    lin = lin + 1;
    cdputs( cbuf, lin, col );
    uiPutColor(0);          /* dwp */

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */
    fship = 0;					/* first user in uvec */

    while(TRUE) /* repeat- while */
    {
        if ( ! godlike )
            if ( ! clbStillAlive( Context.snum ) )
                break;
        i = fship;
        cdclrl( fline, lline - fline + 1 );
        lin = fline;
        while ( i < MAXSHIPS && lin <= lline )
	{
            status = cbShips[i].status;

            kb = cbShips[i].killedBy;
            detail = cbShips[i].killedByDetail;
            if ( status == SS_LIVE ||
                 ( doall && ( status != SS_OFF || kb != KB_NONE ) ) )
	    {
                sbuf[0] = 0;
                utAppendShip(sbuf , i) ;
                strcat(sbuf, " ") ;
                utAppendChar(sbuf, cbShipTypes[cbShips[i].shiptype].name[0]) ;

                unum = cbShips[i].unum;
                if ( unum >= 0 && unum < MAXUSERS )
		{
                    if (SROBOT(i)) /* robot */
                        strcpy(pidbuf, " ROBOT");
                    else if (SVACANT(i))
                        strcpy(pidbuf, "VACANT");
                    else
		    {
                        if (godlike)
                            sprintf(pidbuf, "%6d", cbShips[i].pid);
                        else
                            strcpy(pidbuf, "  LIVE");
		    }

                    strcpy(ubuf, cbUsers[unum].username);

                    sprintf(kbuf, "%6.1f", (cbShips[i].kills + cbShips[i].strkills));
                    sprintf( cbuf, "%-5s %-13.13s %-21.21s %-8s %6s",
                             sbuf, ubuf, cbShips[i].alias,
                             kbuf, pidbuf );
		}
                else
                    sprintf( cbuf, "%-5s %13s %21s %8s %6s", sbuf,
                             " ", " ", " ", " " );
                if ( doall && kb != KB_NONE )
		{
                    strcat(cbuf, "  ") ;
                    utAppendKilledBy(cbuf , kb, detail) ;
		}

		if (snum >= 0 && snum < MAXSHIPS )
                {		/* a normal ship view */
		    if ( i == snum )    /* it's ours */
                        uiPutColor(CQC_A_BOLD);
		    else if (satwar(i, snum)) /* we're at war with it */
                        uiPutColor(RedLevelColor);
		    else if (cbShips[i].team == cbShips[snum].team && !selfwar(snum))
                        uiPutColor(GreenLevelColor); /* it's a team ship */
		    else
                        uiPutColor(YellowLevelColor);
                }
		else if (godlike) /* conqoper */
                {
		    uiPutColor(YellowLevelColor);
                }
		else
                { /* not conqoper, and not a valid ship (main menu) */
		    if (cbUsers[Context.unum].war[cbShips[i].team])  /* we're at war with ships's
                                                                    team */
                        uiPutColor(RedLevelColor);
		    else if (cbUsers[Context.unum].team == cbShips[i].team)
                        uiPutColor(GreenLevelColor); /* it's a team ship */
		    else
                        uiPutColor(YellowLevelColor);
                }

                cdputs( cbuf, lin, col );
                uiPutColor(0);
                if ( doall && status != SS_LIVE )
		{
                    cbuf[0] = 0;
                    utAppendShipStatus(cbuf , status) ;

                    uiPutColor(YellowLevelColor);
                    cdputs( cbuf, lin, col - 2 - strlen( cbuf ) );
                    uiPutColor(0);
		}
	    }
            i = i + 1;
            lin = lin + 1;
	}
        if ( i >= MAXSHIPS )
	{
            /* We're displaying the last page. */
            mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
            cdrefresh();
            if ( iogtimed( &ch, 1.0 ) )
	    {
                if ( ch == TERM_EXTRA )
                    fship = 0;			/* move to first page */
                else
                    break;
	    }
	}
        else
	{
            /* There are ships left to display. */
            mcuPutPrompt( MTXT_MORE, MSG_LIN2 );
            cdrefresh();
            if ( iogtimed( &ch, 1.0 ) )
	    {
                if ( ch == TERM_EXTRA )
                    fship = 0;			/* move to first page */
                else if ( ch == ' ' )
                    fship = i;			/* move to next page */
                else
                    break;
	    }
	}
    }

    return;

}

/*  review - review old messages */
/*  SYNOPSIS */
/*    int flag, review */
/*    int snum, slm */
/*    flag = mcuReviewMsgs( snum, slm ) */
int mcuReviewMsgs( int snum, int slm )
{
    int ch, Done, i, msg, tmsg, lastone;
    int didany;

    didany = FALSE;
    Done = FALSE;

    lastone = utModPlusOne( cbConqInfo->lastmsg+1, MAXMESSAGES );
    if ( snum >= 0 && snum < MAXSHIPS )
    {
        if ( cbShips[snum].lastmsg == LMSG_NEEDINIT )
            return ( FALSE );				/* none to read */
        i = cbShips[snum].alastmsg;
        if ( i != LMSG_READALL )
            lastone = i;
    }

    cdclrl( MSG_LIN1, 1 );

    /*  for ( msg = slm; msg != lastone; msg = utModPlusOne( msg-1, MAXMESSAGES ) )*/

    msg = slm;

    do
    {
        if ( clbCanRead( snum, msg ))
	{
            mcuReadMsg( msg, MSG_LIN1 );
            didany = TRUE;
            mcuPutPrompt( "--- [SPACE] for more, arrows to scroll, any key to quit ---",
                          MSG_LIN2 );
            cdrefresh();
            ch = iogchar();
            switch(ch)
	    {
	    case ' ':
	    case '<':
	    case KEY_UP:
	    case KEY_LEFT:
                tmsg = utModPlusOne( msg - 1, MAXMESSAGES );
                while(!clbCanRead( snum, tmsg ) && tmsg != lastone)
		{
                    tmsg = utModPlusOne( tmsg - 1, MAXMESSAGES );
		}
                if (tmsg == lastone)
		{
                    cdbeep();
		}
                else
                    msg = tmsg;

                break;
	    case '>':
	    case KEY_DOWN:
	    case KEY_RIGHT:
                tmsg =  utModPlusOne( msg + 1, MAXMESSAGES );
                while(!clbCanRead( snum, tmsg ) && tmsg != slm + 1 )
		{
                    tmsg = utModPlusOne( tmsg + 1, MAXMESSAGES );
		}
                if (tmsg == (slm + 1))
		{
                    cdbeep();
		}
                else
                    msg = tmsg;

                break;
	    default:
                Done = TRUE;
                break;
	    }
	}
        else
	{
            msg = utModPlusOne( msg - 1, MAXMESSAGES );
            if (msg == lastone)
                Done = TRUE;
	}

    } while (Done == FALSE);

    cdclrl( MSG_LIN1, 2 );

    return ( didany );

}


/*  teamlist - list team statistics */
/*  SYNOPSIS */
/*    int team */
/*    mcuTeamList( team ) */
void mcuTeamList( int team )
{
    int i, j, lin, col = 0, ctime, etime;
    int godlike;
    char buf[MSGMAXLINE], timbuf[5][MAXDATESIZE];
    real x[5];
    static const char *sfmt="%15s %11s %11s %11s %11s %11s";
    static const char *stats="Statistics since: ";
    static const char *last_conquered="Universe last conquered at: ";

    char tmpfmt[MSGMAXLINE * 2];
    static char sfmt2[MSGMAXLINE * 2];
    static char sfmt3[MSGMAXLINE * 2];
    static char dfmt2[MSGMAXLINE * 2];
    static char pfmt2[MSGMAXLINE * 2];
    static int FirstTime = TRUE;	/* Only necc if the colors aren't
                                           going to change at runtime */

    if (FirstTime == TRUE)
    {
        FirstTime = FALSE;
        sprintf(sfmt2,
                "#%d#%%16s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s",
                LabelColor,
                GreenLevelColor,
                YellowLevelColor,
                RedLevelColor,
                SpecialColor,
                InfoColor);

        sprintf(sfmt3,
                "#%d#%%15s #%d#%%12s #%d#%%11s #%d#%%11s #%d#%%11s #%d#%%11s",
                LabelColor,
                GreenLevelColor,
                YellowLevelColor,
                RedLevelColor,
                SpecialColor,
                InfoColor);

        sprintf(dfmt2,
                "#%d#%%15s #%d#%%12d #%d#%%11d #%d#%%11d #%d#%%11d #%d#%%11d",
                LabelColor,
                GreenLevelColor,
                YellowLevelColor,
                RedLevelColor,
                SpecialColor,
                InfoColor);

        sprintf(pfmt2,
                "#%d#%%15s #%d#%%11.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%% #%d#%%10.2f%%%%",
                LabelColor,
                GreenLevelColor,
                YellowLevelColor,
                RedLevelColor,
                SpecialColor,
                InfoColor);

    } /* FIRST_TIME */

    godlike = ( team < 0 || team >= NUMPLAYERTEAMS );

    lin = 1;
    /* team stats and last date conquered */
    sprintf(tmpfmt,"#%d#%%s#%d#%%s",LabelColor,InfoColor);
    cprintf(lin,0,ALIGN_CENTER, tmpfmt, stats, cbConqInfo->inittime);
    lin++;

    /* last conquered */
    cprintf(lin, 0, ALIGN_CENTER, tmpfmt, last_conquered,
            cbConqInfo->conqtime);
    lin++;

    /* last conqueror and conqteam */
    sprintf(tmpfmt,"#%d#by #%d#%%s #%d#for the #%d#%%s #%d#team",
            LabelColor,(int)CQC_A_BOLD,LabelColor,(int)CQC_A_BOLD,LabelColor);
    cprintf(lin,0,ALIGN_CENTER, tmpfmt, cbConqInfo->conqueror,
            cbConqInfo->conqteam);

    col = 0;  /* put col back to 0 for rest of display */
    lin = lin + 1;
    cdclrl( lin, 1 );
    if ( cbConqInfo->lastwords[0] != 0 )
    {
        sprintf(tmpfmt, "#%d#%%c%%s%%c", YellowLevelColor);
        cprintf(lin, 0, ALIGN_CENTER, tmpfmt, '"', cbConqInfo->lastwords, '"' );
    }

    lin+=2;
    sprintf( buf, sfmt, " ",
             cbTeams[0].name, cbTeams[1].name, cbTeams[2].name, cbTeams[3].name, "Totals" );
    cprintf(lin,col,0, sfmt2, " ",
            cbTeams[0].name, cbTeams[1].name, cbTeams[2].name, cbTeams[3].name, "Totals" );

    lin++;
    for ( i = 0; buf[i] != 0; i++ )
        if ( buf[i] != ' ' )
            buf[i] = '-';
    uiPutColor(LabelColor);
    cdputs( buf, lin, col );
    uiPutColor(0);

    lin++;
    cprintf(lin,col,0, dfmt2, "Conquers",
            cbTeams[0].stats[TSTAT_CONQUERS], cbTeams[1].stats[TSTAT_CONQUERS],
            cbTeams[2].stats[TSTAT_CONQUERS], cbTeams[3].stats[TSTAT_CONQUERS],
            cbTeams[0].stats[TSTAT_CONQUERS] + cbTeams[1].stats[TSTAT_CONQUERS] +
            cbTeams[2].stats[TSTAT_CONQUERS] + cbTeams[3].stats[TSTAT_CONQUERS] );

    lin++;
    cprintf(lin,col,0, dfmt2, "Wins",
            cbTeams[0].stats[TSTAT_WINS], cbTeams[1].stats[TSTAT_WINS],
            cbTeams[2].stats[TSTAT_WINS], cbTeams[3].stats[TSTAT_WINS],
            cbTeams[0].stats[TSTAT_WINS] + cbTeams[1].stats[TSTAT_WINS] +
            cbTeams[2].stats[TSTAT_WINS] + cbTeams[3].stats[TSTAT_WINS] );

    lin++;
    cprintf(lin,col,0, dfmt2, "Losses",
            cbTeams[0].stats[TSTAT_LOSSES], cbTeams[1].stats[TSTAT_LOSSES],
            cbTeams[2].stats[TSTAT_LOSSES], cbTeams[3].stats[TSTAT_LOSSES],
            cbTeams[0].stats[TSTAT_LOSSES] + cbTeams[1].stats[TSTAT_LOSSES] +
            cbTeams[2].stats[TSTAT_LOSSES] + cbTeams[3].stats[TSTAT_LOSSES] );

    lin++;
    cprintf(lin,col,0, dfmt2, "Ships",
            cbTeams[0].stats[TSTAT_ENTRIES], cbTeams[1].stats[TSTAT_ENTRIES],
            cbTeams[2].stats[TSTAT_ENTRIES], cbTeams[3].stats[TSTAT_ENTRIES],
            cbTeams[0].stats[TSTAT_ENTRIES] + cbTeams[1].stats[TSTAT_ENTRIES] +
            cbTeams[2].stats[TSTAT_ENTRIES] + cbTeams[3].stats[TSTAT_ENTRIES] );

    lin++;
    etime = cbTeams[0].stats[TSTAT_SECONDS] + cbTeams[1].stats[TSTAT_SECONDS] +
        cbTeams[2].stats[TSTAT_SECONDS] + cbTeams[3].stats[TSTAT_SECONDS];
    utFormatSeconds( cbTeams[0].stats[TSTAT_SECONDS], timbuf[0] );
    utFormatSeconds( cbTeams[1].stats[TSTAT_SECONDS], timbuf[1] );
    utFormatSeconds( cbTeams[2].stats[TSTAT_SECONDS], timbuf[2] );
    utFormatSeconds( cbTeams[3].stats[TSTAT_SECONDS], timbuf[3] );
    utFormatSeconds( etime, timbuf[4] );
    cprintf(lin,col,0, sfmt3, "Time",
            timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

    lin++;
    ctime = cbTeams[0].stats[TSTAT_CPUSECONDS] + cbTeams[1].stats[TSTAT_CPUSECONDS] +
        cbTeams[2].stats[TSTAT_CPUSECONDS] + cbTeams[3].stats[TSTAT_CPUSECONDS];
    utFormatSeconds( cbTeams[0].stats[TSTAT_CPUSECONDS], timbuf[0] );
    utFormatSeconds( cbTeams[1].stats[TSTAT_CPUSECONDS], timbuf[1] );
    utFormatSeconds( cbTeams[2].stats[TSTAT_CPUSECONDS], timbuf[2] );
    utFormatSeconds( cbTeams[3].stats[TSTAT_CPUSECONDS], timbuf[3] );
    utFormatSeconds( ctime, timbuf[4] );
    cprintf( lin,col,0, sfmt3, "Cpu time",
             timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

    lin++;
    for ( i = 0; i < 4; i++ )
    {
        j = cbTeams[i].stats[TSTAT_SECONDS];
        if ( j <= 0 )
            x[i] = 0.0;
        else
            x[i] = 100.0 * ((real) cbTeams[i].stats[TSTAT_CPUSECONDS] / (real) j);
    }
    if ( etime <= 0 )
        x[4] = 0.0;
    else
        x[4] = 100.0 * (real) ctime / (real)etime;
    cprintf( lin,col,0, pfmt2, "Cpu usage", x[0], x[1], x[2], x[3], x[4] );

    lin++;
    cprintf( lin,col,0, dfmt2, "Phaser shots",
             cbTeams[0].stats[TSTAT_PHASERS], cbTeams[1].stats[TSTAT_PHASERS],
             cbTeams[2].stats[TSTAT_PHASERS], cbTeams[3].stats[TSTAT_PHASERS],
             cbTeams[0].stats[TSTAT_PHASERS] + cbTeams[1].stats[TSTAT_PHASERS] +
             cbTeams[2].stats[TSTAT_PHASERS] + cbTeams[3].stats[TSTAT_PHASERS] );

    lin++;
    cprintf( lin,col,0, dfmt2, "Torps fired",
             cbTeams[0].stats[TSTAT_TORPS], cbTeams[1].stats[TSTAT_TORPS],
             cbTeams[2].stats[TSTAT_TORPS], cbTeams[3].stats[TSTAT_TORPS],
             cbTeams[0].stats[TSTAT_TORPS] + cbTeams[1].stats[TSTAT_TORPS] +
             cbTeams[2].stats[TSTAT_TORPS] + cbTeams[3].stats[TSTAT_TORPS] );

    lin++;
    cprintf( lin,col,0, dfmt2, "Armies bombed",
             cbTeams[0].stats[TSTAT_ARMBOMB], cbTeams[1].stats[TSTAT_ARMBOMB],
             cbTeams[2].stats[TSTAT_ARMBOMB], cbTeams[3].stats[TSTAT_ARMBOMB],
             cbTeams[0].stats[TSTAT_ARMBOMB] + cbTeams[1].stats[TSTAT_ARMBOMB] +
             cbTeams[2].stats[TSTAT_ARMBOMB] + cbTeams[3].stats[TSTAT_ARMBOMB] );

    lin++;
    cprintf( lin,col,0, dfmt2, "Armies captured",
             cbTeams[0].stats[TSTAT_ARMSHIP], cbTeams[1].stats[TSTAT_ARMSHIP],
             cbTeams[2].stats[TSTAT_ARMSHIP], cbTeams[3].stats[TSTAT_ARMSHIP],
             cbTeams[0].stats[TSTAT_ARMSHIP] + cbTeams[1].stats[TSTAT_ARMSHIP] +
             cbTeams[2].stats[TSTAT_ARMSHIP] + cbTeams[3].stats[TSTAT_ARMSHIP] );

    lin++;
    cprintf( lin,col,0, dfmt2, "Planets taken",
             cbTeams[0].stats[TSTAT_CONQPLANETS], cbTeams[1].stats[TSTAT_CONQPLANETS],
             cbTeams[2].stats[TSTAT_CONQPLANETS], cbTeams[3].stats[TSTAT_CONQPLANETS],
             cbTeams[0].stats[TSTAT_CONQPLANETS] + cbTeams[1].stats[TSTAT_CONQPLANETS] +
             cbTeams[2].stats[TSTAT_CONQPLANETS] + cbTeams[3].stats[TSTAT_CONQPLANETS] );

    lin++;
    cprintf( lin,col,0, dfmt2, "Coups",
             cbTeams[0].stats[TSTAT_COUPS], cbTeams[1].stats[TSTAT_COUPS],
             cbTeams[2].stats[TSTAT_COUPS], cbTeams[3].stats[TSTAT_COUPS],
             cbTeams[0].stats[TSTAT_COUPS] + cbTeams[1].stats[TSTAT_COUPS] +
             cbTeams[2].stats[TSTAT_COUPS] + cbTeams[3].stats[TSTAT_COUPS] );

    lin++;
    cprintf( lin,col,0, dfmt2, "Genocides",
             cbTeams[0].stats[TSTAT_GENOCIDE], cbTeams[1].stats[TSTAT_GENOCIDE],
             cbTeams[2].stats[TSTAT_GENOCIDE], cbTeams[3].stats[TSTAT_GENOCIDE],
             cbTeams[0].stats[TSTAT_GENOCIDE] + cbTeams[1].stats[TSTAT_GENOCIDE] +
             cbTeams[2].stats[TSTAT_GENOCIDE] + cbTeams[3].stats[TSTAT_GENOCIDE] );

    for ( i = 0; i < 4; i++ )
        if ( cbTeams[i].couptime == 0 )
            timbuf[i][0] = 0;
        else
            sprintf( timbuf[i], "%d", cbTeams[i].couptime );

    if ( ! godlike )
    {
        for ( i = 0; i < 4; i++ )
            if ( team != i )
                strcpy(timbuf[i] , "-") ;
            else if ( ! cbTeams[i].coupinfo && timbuf[i][0] != 0 )
                strcpy(timbuf[i] , "?") ;
    }

    timbuf[4][0] = 0;

    lin++;
    cprintf( lin,col,0, sfmt3, "Coup time",
             timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

    uiPutColor(0);

    return;

}


/*  userlist - display the user list */
/*  SYNOPSIS */
/*    mcuUserList( godlike ) */
void mcuUserList( int godlike, int snum )
{
    int i, j, unum, nu, fuser, fline, lline, lin;
    static int uvec[MAXUSERS];
    int ch;
    static const char *hd1="U S E R   L I S T";

    /* init the user vector */

    for (i=0; i<MAXUSERS; i++)
        uvec[i] = i;

    /* Do some screen setup. */
    cdclear();
    lin = 0;
    uiPutColor(LabelColor);
    cdputc( hd1, lin );

    lin = lin + 3;        /* FIXME - hardcoded??? - dwp */
    clbUserline( -1, -1, cbuf, FALSE, FALSE );
    cdputs( cbuf, lin, 1 );

    for ( j = 0; cbuf[j] != 0; j = j + 1 )
        if ( cbuf[j] != ' ' )
            cbuf[j] = '-';
    lin = lin + 1;
    cdputs( cbuf, lin, 1 );
    uiPutColor(0);

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */
    fuser = 0;					/* first user in uvec */

    while (TRUE) /* repeat-while */
    {

        if ( ! godlike )
            if ( ! clbStillAlive( Context.snum ) )
                break;

        /* sort the (living) user list */
        nu = 0;
        for ( unum = 0; unum < MAXUSERS; unum++)
            if ( ULIVE(unum) )
            {
                uvec[nu++] = unum;
            }
        clbSortUsers(uvec, nu);

        i = fuser;
        cdclrl( fline, lline - fline + 1 );
        lin = fline;
        while ( i < nu && lin <= lline )
	{
            clbUserline( uvec[i], -1, cbuf, godlike, FALSE );

            /* determine color */
            if ( snum >= 0 && snum < MAXSHIPS ) /* we're a valid ship */
	    {
		if ( strcmp(cbUsers[uvec[i]].username,
			    cbUsers[cbShips[snum].unum].username) == 0 &&
		     cbUsers[uvec[i]].type == cbUsers[cbShips[snum].unum].type)
                    uiPutColor(CQC_A_BOLD);    /* it's ours */
		else if (cbShips[snum].war[cbUsers[uvec[i]].team]) /* we're at war with it */
                    uiPutColor(RedLevelColor);
		else if (cbShips[snum].team == cbUsers[uvec[i]].team && !selfwar(snum))
                    uiPutColor(GreenLevelColor); /* it's a team ship */
		else
                    uiPutColor(YellowLevelColor);
	    }
            else if (godlike)/* we are running conqoper */
                uiPutColor(YellowLevelColor); /* bland view */
            else			/* we don't have a ship yet */
	    {
                if ( strcmp(cbUsers[uvec[i]].username,
                            cbUsers[Context.unum].username) == 0 &&
                     cbUsers[uvec[i]].type == cbUsers[Context.unum].type)
                    uiPutColor(CQC_A_BOLD);    /* it's ours */
                else if (cbUsers[Context.unum].war[cbUsers[uvec[i]].team]) /* we're war with them */
                    uiPutColor(RedLevelColor);	            /* (might be selfwar) */
                else if (cbUsers[Context.unum].team == cbUsers[uvec[i]].team) /* team ship */
                    uiPutColor(GreenLevelColor);
                else
                    uiPutColor(YellowLevelColor);
	    }

            cdputs( cbuf, lin, 1 );
            uiPutColor(0);
            i = i + 1;
            lin = lin + 1;
	}
        if ( i >= nu )
	{
            /* We're displaying the last page. */
            mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
            cdrefresh();
            if ( iogtimed( &ch, 1.0 ) )
	    {
                if ( ch == TERM_EXTRA )
                    fuser = 0;			/* move to first page */
                else
                    break;
	    }
	}
        else
	{
            /* There are users left to display. */
            mcuPutPrompt( MTXT_MORE, MSG_LIN2 );
            cdrefresh();
            if ( iogtimed( &ch, 1.0 ) )
	    {
                if ( ch == TERM_EXTRA )
                    fuser = 0;			/* move to first page */
                else if ( ch == ' ' )
                    fuser = i;			/* move to next page */
                else
                    break;
	    }
	}
    }

    return;

}


/*  userstats - display the user list */
/*  SYNOPSIS */
/*    mcuUserStats( godlike, snum ) */
void mcuUserStats( int godlike , int snum )
{
    int i, j, unum, nu, fuser, fline, lline, lin;
    static int uvec[MAXUSERS];
    int ch;
    static const char *hd1="M O R E   U S E R   S T A T S";
    static const char *hd2="name         cpu  conq coup geno  taken bombed/shot  shots  fired   last entry";
    static const char *hd3="planets  armies    phaser  torps";

    for (i=0; i<MAXUSERS; i++)
        uvec[i] = i;

    /* Do some screen setup. */
    cdclear();
    lin = 1;
    uiPutColor(LabelColor);  /* dwp */
    cdputc( hd1, lin );

    lin = lin + 2;
    cdputs( hd3, lin, 34 );

    strcpy(cbuf , hd2) ;
    lin = lin + 1;
    cdputs( cbuf, lin, 1 );

    for ( j = 0; cbuf[j] != 0; j = j + 1 )
        if ( cbuf[j] != ' ' )
            cbuf[j] = '-';
    lin = lin + 1;
    cdputs( cbuf, lin, 1 );
    uiPutColor(0);          /* dwp */

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */
    fuser = 0;					/* first user in uvec */

    while (TRUE) /* repeat-while */
    {
        if ( ! godlike )
            if ( ! clbStillAlive( Context.snum ) )
                break;

        /* sort the (living) user list */
        nu = 0;
        for ( unum = 0; unum < MAXUSERS; unum++)
            if ( ULIVE(unum) )
            {
                uvec[nu++] = unum;
            }
        clbSortUsers(uvec, nu);

        i = fuser;
        cdclrl( fline, lline - fline + 1 );
        lin = fline;
        while ( i < nu && lin <= lline )
	{
            clbStatline( uvec[i], cbuf );

            /* determine color */
            if ( snum >= 0 && snum < MAXSHIPS ) /* we're a valid ship */
            {
                if ( strcmp(cbUsers[uvec[i]].username,
                            cbUsers[cbShips[snum].unum].username) == 0 &&
                     cbUsers[uvec[i]].type == cbUsers[cbShips[snum].unum].type )
                    uiPutColor(CQC_A_BOLD);	        /* it's ours */
                else if (cbShips[snum].war[cbUsers[uvec[i]].team])
                    uiPutColor(RedLevelColor);   /* we're at war with it */
                else if (cbShips[snum].team == cbUsers[uvec[i]].team && !selfwar(snum))
                    uiPutColor(GreenLevelColor); /* it's a team ship */
                else
                    uiPutColor(YellowLevelColor);
            }
            else if (godlike)/* we are running conqoper */
	    {
                uiPutColor(YellowLevelColor); /* bland view */
	    }
            else
	    {
                if ( strcmp(cbUsers[uvec[i]].username,
                            cbUsers[Context.unum].username) == 0  &&
                     cbUsers[uvec[i]].type == cbUsers[Context.unum].type )
                    uiPutColor(CQC_A_BOLD);	/* it's ours */
                else if (cbUsers[Context.unum].war[cbUsers[uvec[i]].team])
                    uiPutColor(RedLevelColor);  /* we're war with them (poss selfwar) */
                else if (cbUsers[Context.unum].team == cbUsers[uvec[i]].team)
                    uiPutColor(GreenLevelColor);	/* team ship */
                else
                    uiPutColor(YellowLevelColor);
	    }

            cdputs( cbuf, lin, 1 );
            uiPutColor(0);
            i = i + 1;
            lin = lin + 1;
	}
        if ( i >= nu )
	{
            /* We're displaying the last page. */
            mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
            cdrefresh();
            if ( iogtimed( &ch, 1.0 ) )
	    {
                if ( ch == TERM_EXTRA )
                    fuser = 0;			/* move to first page */
                else
                    break;
	    }
	}
        else
	{
            /* There are users left to display. */
            mcuPutPrompt( MTXT_MORE, MSG_LIN2 );
            cdrefresh();
            if ( iogtimed( &ch, 1.0 ) )
	    {
                if ( ch == TERM_EXTRA )
                    fuser = 0;			/* move to first page */
                else if ( ch == ' ' )
                    fuser = i;			/* move to next page */
                else
                    break;
	    }
	}
    }

    return;

}

/*  confirm - ask the user to confirm a dangerous action */
/*  SYNOPSIS */
/*    int ok, confirm */
/*    ok = mcuConfirm() */
int mcuConfirm(void)
{
    static const char *cprompt = "Are you sure? ";
    int scol = ((Context.maxcol - strlen(cprompt)) / 2);

    if (mcuAskYN("Are you sure? ", MSG_LIN2, scol))
        return(TRUE);
    else
        return (FALSE);

}

/*  askyn - ask the user a yes/no question - return TRUE if yes */
int mcuAskYN(const char *question, int lin, int col)
{
    char ch, buf[MSGMAXLINE];

    cdclrl( MSG_LIN2, 1 );
    uiPutColor(InfoColor);
    buf[0] = 0;
    ch = cdgetx( question, lin, col, TERMS, buf, MSGMAXLINE - 1, TRUE);
    uiPutColor(0);
    cdclrl( lin, 1 );
    cdrefresh();
    if ( ch == TERM_ABORT )
        return ( FALSE );
    if ( buf[0] == 'y' || buf[0] == 'Y' )
        return ( TRUE );

    return ( FALSE );

}

/*  getcx - prompt for a string, centered */
/*  SYNOPSIS */
/*    char pmt(), */
/*    int lin, offset */
/*    char terms(), buf() */
/*    int len */
/*    tch = mcuGetCX( pmt, lin, offset, terms, buf, len ) */
char mcuGetCX( const char *pmt, int lin, int offset, const char *terms,
               char *buf, int len )
{
    int i;

    i = (int)( Context.maxcol - strlen( pmt ) ) / (int)2 + offset;
    if ( i <= 0 )
        i = 1;
    move(lin, 0);
    clrtoeol();
    buf[0] = 0;
    return ( cdgetx( pmt, lin, i, terms, buf, len, TRUE ) );

}



/*  gettarget - get a target angle from the user */
/*  SYNOPSIS */
/*    char pmt() */
/*    int lin, col */
/*    real dir */
/*    int flag, gettarget */
/*    flag = mcuGetTarget( pmt, lin, col, dir ) */
int mcuGetTarget( const char *pmt, int lin, int col, real *dir, real cdefault )
{
    int i, j;
    char ch, buf[MSGMAXLINE];

    cdclrl( lin, 1 );
    buf[0] = 0;
    ch = (char)cdgetx( pmt, lin, col, TERMS, buf, MSGMAXLINE, TRUE );
    if ( ch == TERM_ABORT )
        return ( FALSE );

    utDeleteBlanks( buf );
    if ( buf[0] == 0 )
    {
        /* Default. */
        *dir = cdefault;
        return ( TRUE );
    }
    if (utIsDigits(buf))
    {
        i = 0;
        if ( ! utSafeCToI( &j, buf, i ) )
            return ( FALSE );
        *dir = utMod360( (real) j );
        return ( TRUE );
    }
    if ( utArrowsToDir( buf, dir ) )
        return ( TRUE );

    return ( FALSE );

}

/*  more - wait for the user to type a space */
/*  SYNOPSIS */
/*    char pmt() */
/*    int spacetyped, more */
/*    spacetyped = mcuMore( pmt ) */
int mcuMore( const char *pmt )
{
    int ch = 0;

    if ( pmt[0] != 0 )
        mcuPutPrompt( pmt, MSG_LIN2 );
    else
        mcuPutPrompt( MTXT_MORE, MSG_LIN2 );

    cdrefresh();
    ch = iogchar();
    return ( ch == ' ' );

}


/*  pagefile - page through a file */
/*  SYNOPSIS */
/*    char file(), errmsg() */
/*    int ignorecontroll, eatblanklines */
/*    mcuPageFile( file, errmsg, ignorecontroll, eatblanklines ) */
void mcuPageFile( const char *file, const char *errmsg )
{

    int plins = 1;
    FILE *pfd;
    static const char *sdone="--- press any key to return ---";
    char buffer[BUFFER_SIZE_256];
    int buflen;

    if ((pfd = fopen(file, "r")) == NULL)
    {
        utLog("mcuPageFile(): fopen(%s) failed: %s",
              file,
              strerror(errno));

        cdclear();
        cdredo();
        cdputc( errmsg, MSG_LIN2/2 );
        mcuMore( sdone );

        return;
    }

    cdclear();
    cdrefresh();
    cdmove(0, 0);

    plins = 0;

    while (fgets(buffer, BUFFER_SIZE_256 - 1, pfd) != NULL)
    {
        /* get one at a time */
        buflen = strlen(buffer);

        buffer[buflen - 1] = 0; /* remove trailing LF */
        buflen--;

        if (buffer[0] == 0x0c)	/* formfeed */
	{
            plins = DISPLAY_LINS + 1; /* force new page */
	}
        else
	{
            cdputs(buffer, plins, 0);
            plins++;
	}

        if (plins >= DISPLAY_LINS)
	{
            if (!mcuMore(MTXT_MORE))
                break;		/* bail if space not hit */

            cdclear();
            plins = 1;
	}
    }

    fclose(pfd);

    mcuMore(sdone);

    return;

}


/*  putmsg - display a message on the bottom of the user's screen */
/*  SYNOPSIS */
/*    char msg() */
/*    int line */
/*    mcuPutMsg( msg, line ) */
void mcuPutMsg( const char *msg, int line )
{
    cdclrl( line, 1 );
    cdputs( msg, line, 1 );

    return;

}


/*  putpmt - display a prompt */
/*  SYNOPSIS */
/*    char pmt() */
/*    int line */
/*    mcuPutPrompt( pmt, line ) */
void mcuPutPrompt( const char *pmt, int line )
{
    int i, dcol, pcol;

    i = strlen( pmt );
    dcol = ( Context.maxcol - i ) / 2;
    pcol = dcol + i;
    cdclrl( line, 1 );
    cprintf( line, dcol,ALIGN_NONE,"#%d#%s", InfoColor, pmt);
    cdmove( line, pcol );

    return;

}

/*  helplesson - verbose help */
/*  SYNOPSIS */
/*    helplesson */
void mcuHelpLesson(void)
{

    char buf[MSGMAXLINE];
    char helpfile[BUFFER_SIZE_256];

    sprintf(helpfile, "%s/%s", utGetPath(CONQSHARE), C_CONQ_HELPFILE);
    sprintf( buf, "%s: Can't open.", helpfile );
    mcuPageFile( helpfile, buf);

    return;

}

/*  news - list current happenings */
/*  SYNOPSIS */
/*    news */
void mcuNews(void)
{
    char newsfile[BUFFER_SIZE_256];

    sprintf(newsfile, "%s/%s", utGetPath(CONQSHARE), C_CONQ_NEWSFILE);

    mcuPageFile( newsfile, "No news is good news.");

    return;

}

/* planet updating (textures, etc) */
/* for the GL client, this is defined in GL.c, for the curses client
   this will be a noop defined in cumisc.c */

int uiUpdatePlanet(int pnum)
{
    return TRUE;                  /* NOOP */
}
/* update the direction of the torp (GL only) */
int uiUpdateTorpDir(int snum, int tnum)
{
    return TRUE;
}
