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
#include <vector>
#include "format.h"

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
#include "cprintf.h"

/*  histlist - display the last usage list */
void mcuHistList()
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

    while (true) /* repeat */
    {
        thistptr = cbConqInfo->histptr;
        lin = fline;
        col = 1;
        cdclrl( fline, lline - fline + 1 );

        i = thistptr + 1;
        for ( j = 0; j < cbLimits.maxHist(); j++ )
	{
            bool isResigned = false;

            // adjust based on histptr being the next available slot.
            i--;
            if (i < 0)
                i = cbLimits.maxHist() - 1;

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
    std::string buf;
    unsigned int attrib = 0;

    attrib = CyanColor;

    clbFmtMsg(cbMsgs[msgnum].from, cbMsgs[msgnum].fromDetail,
              cbMsgs[msgnum].to, cbMsgs[msgnum].toDetail, buf);
    buf += ": ";
    buf += cbMsgs[msgnum].msgbuf;

    uiPutColor(attrib);
    uiPutMsg( buf.c_str(), dsplin );
    uiPutColor(0);
    /* clear second line if sending to MSG_LIN1 */
    if (dsplin == MSG_LIN1)
    {
        cdclrl( MSG_LIN2, 1 );
    }

    return(true);

}

/*  planlist - list planets */
void mcuPlanetList()
{
    int i, lin, col, olin, pnum;
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
    static bool FirstTime = true;
    int PlanetOffset;		/* offset into cbLimits.maxPlanets() for this page */
    int PlanetIdx = 0;
    int Done;

    if (FirstTime)
    {
        FirstTime = false;

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
    }

    /* sort the planets */
    std::vector<int> sv;
    for ( i = 0; i < cbLimits.maxPlanets(); i++ )
        sv.push_back(i);
    clbSortPlanets( sv );

    utStrncpy( hd3, hd2, sizeof(hd3) );
    for ( i = 0; hd3[i] != 0; i++ )
        if ( hd3[i] != ' ' )
            hd3[i] = '-';

    PlanetOffset = 0;
    cdclear();
    Done = false;
    do
    {

        cdclra(0, 0, MSG_LIN1 + 2, Context.maxcol - 1);
        lin = 1;

        cprintf(lin, column_h, ALIGN_NONE, "%s", pd0);

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
        col2 = false;

        PlanetIdx = 0;

        if (PlanetOffset < cbLimits.maxPlanets())
	{
            while ((PlanetOffset + PlanetIdx) < cbLimits.maxPlanets())
	    {
                i = PlanetOffset + PlanetIdx;
                PlanetIdx++;
                pnum = sv[i];

                /* Don't display unless it's real. */
                if ( ! PVISIBLE(pnum) )
                    continue;

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

                /* I want everything if it's real */

                /* Figure out who owns it and count armies. */
                ch =  cbTeams[cbPlanets[pnum].team].teamchar;
                sprintf( junk, "%d", cbPlanets[pnum].armies );

                /* Suns and moons are displayed as unowned. */
                if ( cbPlanets[pnum].type == PLANET_SUN
                     || cbPlanets[pnum].type == PLANET_MOON )
                    ch = ' ';

                /* Moons aren't supposed to have armies. */
                if ( cbPlanets[pnum].type == PLANET_MOON )
		{
                    if ( cbPlanets[pnum].armies == 0 )
                        junk[0] = 0;
		}

                char coreflag = ' ';
                char homeflag = ' ';

                /* flag planets that are required for a conq */

                // check for homeplanet
                if (PHOMEPLANET(pnum))
                    homeflag = cbTeams[cbPlanets[pnum].defendteam].teamchar;

                // core planet - required for conquer
                if (PCORE(pnum))
                    coreflag = '+';

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
                        col2 = true;
		    }
		}

                if (!col2)
                    col = column_1;
                else
                    col = column_2;

	    } /* while */

            if ((PlanetOffset + PlanetIdx) >= cbLimits.maxPlanets())
                mcuPutPrompt( MTXT_DONE, MSG_LIN2 );
            else
                mcuPutPrompt( MTXT_MORE, MSG_LIN2 );

            cdrefresh();

            if (iogtimed( &cmd, 1.0 ))
	    {			/* got a char */
                if (cmd == 'q' || cmd == 'Q' || cmd == TERM_ABORT)
		{		/* quit */
                    Done = true;
		}
                else
		{		/* some other key... */
				/* setup for new page */
                    PlanetOffset += PlanetIdx;
                    if (PlanetOffset >= cbLimits.maxPlanets())
		    {		/* pointless to continue */
                        Done = true;
		    }

		}
	    }

	} /* if PlanetOffset <= cbLimits.maxPlanets() */
        else
            Done = true; /* else PlanetOffset > cbLimits.maxPlanets() */

    } while(Done != true); /* do */

    return;

}


/*  playlist - list ships */
void mcuPlayList( bool doall )
{
    // FIXME - get rid of this
    std::string cbuf;

    int unum, status, lin, col;
    int fline, lline, fship;
    int ch;

    /* Do some screen setup. */
    cdclear();
    uiPutColor(LabelColor);  /* dwp */

    clbShipline(SHIPLINE_HDR_OPER, true, cbuf);

    col = (int)(Context.maxcol - cbuf.size() ) / (int)2;
    lin = 2;
    cdputs( cbuf.c_str(), lin, col );

    for ( int i=0; i < cbuf.size(); i++ )
        if ( cbuf[i] != ' ' )
            cbuf[i] = '-';
    lin = lin + 1;
    cdputs( cbuf.c_str(), lin, col );
    uiPutColor(0);          /* dwp */

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */
    fship = 0;					/* first user in uvec */

    while(true) /* repeat- while */
    {
        int i = fship;
        cdclrl( fline, lline - fline + 1 );
        lin = fline;
        while ( i < cbLimits.maxShips() && lin <= lline )
	{
            cbuf.clear();
            status = cbShips[i].status;

            killedBy_t kb = cbShips[i].killedBy;
            uint16_t detail = cbShips[i].killedByDetail;

            if ( status == SS_LIVE ||
                 ( doall && ( status != SS_OFF || kb != KB_NONE ) ) )
	    {
                clbShipline(i, true, cbuf);

                if ( doall && kb != KB_NONE )
		{
                    cbuf += "  ";
                    utAppendKilledBy(cbuf, kb, detail) ;
		}

                uiPutColor(YellowLevelColor);

                cdputs( cbuf.c_str(), lin, col );
                uiPutColor(0);
                if ( doall && status != SS_LIVE )
		{
                    cbuf.clear();
                    utAppendShipStatus(cbuf, status) ;

                    uiPutColor(YellowLevelColor);
                    cdputs( cbuf.c_str(), lin, col - 2 - cbuf.size() );
                    uiPutColor(0);
		}
                lin++;
	    }
            i++; // next ship
	} // while ( i < cbLimits.maxShips() && lin <= lline )

        if ( i >= cbLimits.maxShips() )
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
int mcuReviewMsgs( int slm )
{
    int ch, Done, msg, tmsg, lastone;
    int didany;

    didany = false;
    Done = false;

    lastone = mod( cbConqInfo->lastmsg+1, cbLimits.maxMsgs() );
    cdclrl( MSG_LIN1, 1 );

    msg = slm;

    do
    {
        if ( clbCanRead( -1, msg ))
	{
            mcuReadMsg( msg, MSG_LIN1 );
            didany = true;
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
                tmsg = msg - 1;
                if (tmsg < 0)
                    tmsg = cbLimits.maxMsgs() - 1;
                while(!clbCanRead( -1, tmsg ) && tmsg != lastone)
		{
                    tmsg--;
                    if (tmsg < 0)
                        tmsg = cbLimits.maxMsgs() - 1;
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
                tmsg =  mod( msg + 1, cbLimits.maxMsgs() );
                while(!clbCanRead( -1, tmsg ) && tmsg != slm + 1 )
		{
                    tmsg = mod( tmsg + 1, cbLimits.maxMsgs() );
		}
                if (tmsg == (slm + 1))
		{
                    cdbeep();
		}
                else
                    msg = tmsg;

                break;
	    default:
                Done = true;
                break;
	    }
	}
        else
	{
            msg--;
            if (msg < 0)
                msg = cbLimits.maxMsgs() - 1;
            if (msg == lastone)
                Done = true;
	}

    } while (Done == false);

    cdclrl( MSG_LIN1, 2 );

    return ( didany );

}


/*  teamlist - list team statistics */
/*  SYNOPSIS */
/*    int team */
/*    mcuTeamList( team ) */
void mcuTeamList()
{
    int i, j, lin, col = 0, ctime, etime;
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
    static bool FirstTime = true;	/* Only necc if the colors aren't
                                           going to change at runtime */

    if (FirstTime)
    {
        FirstTime = false;
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
    cprintf(lin,col, ALIGN_NONE, sfmt2, " ",
            cbTeams[0].name, cbTeams[1].name, cbTeams[2].name, cbTeams[3].name, "Totals" );

    lin++;
    for ( i = 0; buf[i] != 0; i++ )
        if ( buf[i] != ' ' )
            buf[i] = '-';
    uiPutColor(LabelColor);
    cdputs( buf, lin, col );
    uiPutColor(0);

    lin++;
    cprintf(lin,col, ALIGN_NONE, dfmt2, "Conquers",
            cbTeams[0].stats[TSTAT_CONQUERS], cbTeams[1].stats[TSTAT_CONQUERS],
            cbTeams[2].stats[TSTAT_CONQUERS], cbTeams[3].stats[TSTAT_CONQUERS],
            cbTeams[0].stats[TSTAT_CONQUERS] + cbTeams[1].stats[TSTAT_CONQUERS] +
            cbTeams[2].stats[TSTAT_CONQUERS] + cbTeams[3].stats[TSTAT_CONQUERS] );

    lin++;
    cprintf(lin,col, ALIGN_NONE, dfmt2, "Wins",
            cbTeams[0].stats[TSTAT_WINS], cbTeams[1].stats[TSTAT_WINS],
            cbTeams[2].stats[TSTAT_WINS], cbTeams[3].stats[TSTAT_WINS],
            cbTeams[0].stats[TSTAT_WINS] + cbTeams[1].stats[TSTAT_WINS] +
            cbTeams[2].stats[TSTAT_WINS] + cbTeams[3].stats[TSTAT_WINS] );

    lin++;
    cprintf(lin,col, ALIGN_NONE, dfmt2, "Losses",
            cbTeams[0].stats[TSTAT_LOSSES], cbTeams[1].stats[TSTAT_LOSSES],
            cbTeams[2].stats[TSTAT_LOSSES], cbTeams[3].stats[TSTAT_LOSSES],
            cbTeams[0].stats[TSTAT_LOSSES] + cbTeams[1].stats[TSTAT_LOSSES] +
            cbTeams[2].stats[TSTAT_LOSSES] + cbTeams[3].stats[TSTAT_LOSSES] );

    lin++;
    cprintf(lin,col, ALIGN_NONE, dfmt2, "Ships",
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
    cprintf(lin,col, ALIGN_NONE, sfmt3, "Time",
            timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

    lin++;
    ctime = cbTeams[0].stats[TSTAT_CPUSECONDS] + cbTeams[1].stats[TSTAT_CPUSECONDS] +
        cbTeams[2].stats[TSTAT_CPUSECONDS] + cbTeams[3].stats[TSTAT_CPUSECONDS];
    utFormatSeconds( cbTeams[0].stats[TSTAT_CPUSECONDS], timbuf[0] );
    utFormatSeconds( cbTeams[1].stats[TSTAT_CPUSECONDS], timbuf[1] );
    utFormatSeconds( cbTeams[2].stats[TSTAT_CPUSECONDS], timbuf[2] );
    utFormatSeconds( cbTeams[3].stats[TSTAT_CPUSECONDS], timbuf[3] );
    utFormatSeconds( ctime, timbuf[4] );
    cprintf( lin,col, ALIGN_NONE, sfmt3, "Cpu time",
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
    cprintf( lin,col, ALIGN_NONE, pfmt2, "Cpu usage",
             x[0], x[1], x[2], x[3], x[4] );

    lin++;
    cprintf( lin,col, ALIGN_NONE, dfmt2, "Phaser shots",
             cbTeams[0].stats[TSTAT_PHASERS], cbTeams[1].stats[TSTAT_PHASERS],
             cbTeams[2].stats[TSTAT_PHASERS], cbTeams[3].stats[TSTAT_PHASERS],
             cbTeams[0].stats[TSTAT_PHASERS] + cbTeams[1].stats[TSTAT_PHASERS] +
             cbTeams[2].stats[TSTAT_PHASERS] + cbTeams[3].stats[TSTAT_PHASERS] );

    lin++;
    cprintf( lin,col, ALIGN_NONE, dfmt2, "Torps fired",
             cbTeams[0].stats[TSTAT_TORPS], cbTeams[1].stats[TSTAT_TORPS],
             cbTeams[2].stats[TSTAT_TORPS], cbTeams[3].stats[TSTAT_TORPS],
             cbTeams[0].stats[TSTAT_TORPS] + cbTeams[1].stats[TSTAT_TORPS] +
             cbTeams[2].stats[TSTAT_TORPS] + cbTeams[3].stats[TSTAT_TORPS] );

    lin++;
    cprintf( lin,col, ALIGN_NONE, dfmt2, "Armies bombed",
             cbTeams[0].stats[TSTAT_ARMBOMB], cbTeams[1].stats[TSTAT_ARMBOMB],
             cbTeams[2].stats[TSTAT_ARMBOMB], cbTeams[3].stats[TSTAT_ARMBOMB],
             cbTeams[0].stats[TSTAT_ARMBOMB] + cbTeams[1].stats[TSTAT_ARMBOMB] +
             cbTeams[2].stats[TSTAT_ARMBOMB] + cbTeams[3].stats[TSTAT_ARMBOMB] );

    lin++;
    cprintf( lin,col, ALIGN_NONE, dfmt2, "Armies captured",
             cbTeams[0].stats[TSTAT_ARMSHIP], cbTeams[1].stats[TSTAT_ARMSHIP],
             cbTeams[2].stats[TSTAT_ARMSHIP], cbTeams[3].stats[TSTAT_ARMSHIP],
             cbTeams[0].stats[TSTAT_ARMSHIP] + cbTeams[1].stats[TSTAT_ARMSHIP] +
             cbTeams[2].stats[TSTAT_ARMSHIP] + cbTeams[3].stats[TSTAT_ARMSHIP] );

    lin++;
    cprintf( lin,col, ALIGN_NONE, dfmt2, "Planets taken",
             cbTeams[0].stats[TSTAT_CONQPLANETS], cbTeams[1].stats[TSTAT_CONQPLANETS],
             cbTeams[2].stats[TSTAT_CONQPLANETS], cbTeams[3].stats[TSTAT_CONQPLANETS],
             cbTeams[0].stats[TSTAT_CONQPLANETS] + cbTeams[1].stats[TSTAT_CONQPLANETS] +
             cbTeams[2].stats[TSTAT_CONQPLANETS] + cbTeams[3].stats[TSTAT_CONQPLANETS] );

    lin++;
    cprintf( lin,col, ALIGN_NONE, dfmt2, "Coups",
             cbTeams[0].stats[TSTAT_COUPS], cbTeams[1].stats[TSTAT_COUPS],
             cbTeams[2].stats[TSTAT_COUPS], cbTeams[3].stats[TSTAT_COUPS],
             cbTeams[0].stats[TSTAT_COUPS] + cbTeams[1].stats[TSTAT_COUPS] +
             cbTeams[2].stats[TSTAT_COUPS] + cbTeams[3].stats[TSTAT_COUPS] );

    lin++;
    cprintf( lin,col, ALIGN_NONE, dfmt2, "Genocides",
             cbTeams[0].stats[TSTAT_GENOCIDE], cbTeams[1].stats[TSTAT_GENOCIDE],
             cbTeams[2].stats[TSTAT_GENOCIDE], cbTeams[3].stats[TSTAT_GENOCIDE],
             cbTeams[0].stats[TSTAT_GENOCIDE] + cbTeams[1].stats[TSTAT_GENOCIDE] +
             cbTeams[2].stats[TSTAT_GENOCIDE] + cbTeams[3].stats[TSTAT_GENOCIDE] );

    for ( i = 0; i < 4; i++ )
        if ( cbTeams[i].couptime == 0 )
            timbuf[i][0] = 0;
        else
            sprintf( timbuf[i], "%d", cbTeams[i].couptime );

    timbuf[4][0] = 0;

    lin++;
    cprintf( lin,col, ALIGN_NONE, sfmt3, "Coup time",
             timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

    uiPutColor(0);

    return;

}


/*  userlist - display the user list */
/*  SYNOPSIS */
/*    mcuUserList( godlike ) */
void mcuUserList()
{
    int i, j, unum, fuser, fline, lline, lin;
    std::vector<int> uvec;
    int ch;
    static const std::string hd1="U S E R   L I S T";
    std::string cbuf;

    /* Do some screen setup. */
    cdclear();
    lin = 0;
    uiPutColor(LabelColor);
    cdputc( hd1.c_str(), lin );

    lin = lin + 3;
    clbUserline( -1, -1, cbuf, false, false );
    cdputs( cbuf.c_str(), lin, 1 );

    for ( j = 0; cbuf[j] != 0; j++ )
        if ( cbuf[j] != ' ' )
            cbuf[j] = '-';
    lin = lin + 1;
    cdputs( cbuf.c_str(), lin, 1 );
    uiPutColor(0);

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */
    fuser = 0;					/* first user in uvec */

    while (true)
    {
        /* sort the (living) user list */
        uvec.clear();
        for ( unum = 0; unum < cbLimits.maxUsers(); unum++)
            if ( ULIVE(unum) )
            {
                uvec.push_back(unum);
            }
        clbSortUsers(uvec);

        i = fuser;
        cdclrl( fline, lline - fline + 1 );
        lin = fline;
        while ( i < uvec.size() && lin <= lline )
	{
            clbUserline( uvec[i], -1, cbuf, true, false );

            uiPutColor(YellowLevelColor); /* bland view */

            cdputs( cbuf.c_str(), lin, 1 );
            uiPutColor(0);
            i++;
            lin++;
	}
        if ( i >= uvec.size() )
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
void mcuUserStats()
{
    int i, j, unum, fuser, fline, lline, lin;
    std::vector<int> uvec;
    int ch;
    static const std::string hd1 = "M O R E   U S E R   S T A T S";
    std::string cbuf;

    /* Do some screen setup. */
    cdclear();
    lin = 1;
    uiPutColor(LabelColor);  /* dwp */
    cdputc( hd1.c_str(), lin );


    lin = lin + 2;
    clbStatline(STATLINE_HDR1, cbuf);
    cdputs( cbuf.c_str(), lin, 33 );

    // get the header
    clbStatline(STATLINE_HDR2, cbuf);
    lin++;
    cdputs( cbuf.c_str(), lin, 0 );

    for ( j = 0; cbuf[j] != 0; j = j + 1 )
        if ( cbuf[j] != ' ' )
            cbuf[j] = '-';

    lin = lin + 1;
    cdputs( cbuf.c_str(), lin, 1 );
    uiPutColor(0);

    fline = lin + 1;				/* first line to use */
    lline = MSG_LIN1;				/* last line to use */
    fuser = 0;					/* first user in uvec */

    while (true)
    {
        /* sort the (living) user list */
        uvec.clear();
        for ( unum = 0; unum < cbLimits.maxUsers(); unum++)
            if ( ULIVE(unum) )
            {
                uvec.push_back(unum);
            }
        clbSortUsers(uvec);

        i = fuser;
        cdclrl( fline, lline - fline + 1 );
        lin = fline;
        while ( i < uvec.size() && lin <= lline )
	{
            clbStatline( uvec[i], cbuf );

            /* determine color */
            uiPutColor(YellowLevelColor); /* bland view */

            cdputs( cbuf.c_str(), lin, 1 );
            uiPutColor(0);
            i++;
            lin++;
	}
        if ( i >= uvec.size() )
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
bool mcuConfirm(void)
{
    static const char *cprompt = "Are you sure? ";
    int scol = ((Context.maxcol - strlen(cprompt)) / 2);

    if (mcuAskYN("Are you sure? ", MSG_LIN2, scol))
        return(true);
    else
        return (false);
}

/*  askyn - ask the user a yes/no question - return true if yes */
int mcuAskYN(const char *question, int lin, int col)
{
    char ch, buf[MSGMAXLINE];

    cdclrl( MSG_LIN2, 1 );
    uiPutColor(InfoColor);
    buf[0] = 0;
    ch = cdgetx( question, lin, col, TERMS, buf, MSGMAXLINE - 1, true);
    uiPutColor(0);
    cdclrl( lin, 1 );
    cdrefresh();
    if ( ch == TERM_ABORT )
        return ( false );
    if ( buf[0] == 'y' || buf[0] == 'Y' )
        return ( true );

    return ( false );

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
    return ( cdgetx( pmt, lin, i, terms, buf, len, true ) );

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


/*  putmsg - display a message on the bottom of the user's screen */
/*  SYNOPSIS */
/*    char msg() */
/*    int line */
/*    uiPutMsg( msg, line ) */
void uiPutMsg( const char *msg, int line )
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

/* planet updating (textures, etc) */
/* for the GL client, this is defined in GL.c, for the curses client
   this will be a noop defined in cumisc.c */

int uiUpdatePlanet(int pnum)
{
    return true;                  /* NOOP */
}
