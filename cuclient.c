#include "c_defs.h"

/************************************************************************
 *
 * client specific stuff
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#include "global.h"
#include "conf.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"
#include "client.h"
#include "clientlb.h"
#include "conqutil.h"
#include "cd2lb.h"
#include "iolb.h"
#include "cumisc.h"
#include "cb.h"
#include "conqlb.h"
#include "context.h"
#include "record.h"
#include "disputil.h"

void cucPseudo( int unum, int snum )
{
    char ch, buf[MSGMAXLINE];

    buf[0] = 0;

    cdclrl( MSG_LIN1, 2 );
    strcpy(buf , "Old pseudonym: ") ;
    if ( snum >= 0 && snum < MAXSHIPS )
        strcat(buf , Ships[snum].alias) ;
    else
        strcat(buf , Users[unum].alias) ;
    cdputc( buf, MSG_LIN1 );
    ch = mcuGetCX( "Enter a new pseudonym: ",
                   MSG_LIN2, -4, TERMS, buf, MAXUSERNAME );
    if ( ch == TERM_ABORT || buf[0] == 0)
    {
        cdclrl( MSG_LIN1, 2 );
        return;
    }

    sendSetName(buf);

    cdclrl( MSG_LIN1, 2 );

    return;

}

/*  dowar - declare war or peace */
/*  SYNOPSIS */
/*    int snum */
/*    dowar( snum ) */
void cucDoWar( int snum )
{
    int i, entertime, now;
    uint8_t war;
    int twar[NUMPLAYERTEAMS], dowait;
    int ch;

    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
        twar[i] = Ships[snum].war[i];

    cdclrl( MSG_LIN1, 2 );

    while ( clbStillAlive( Context.snum ) )
    {
        cdputs(clbWarPrompt(Context.snum, twar), MSG_LIN1, 1);

        cdrefresh();
        if ( iogtimed( &ch, 1.0 ) == FALSE )
	{
            continue; /* next; */
	}

        ch = (char)tolower( ch );
        if ( ch == TERM_ABORT )
            break;
        if ( ch == TERM_EXTRA )
	{
            /* Now update the war settings. */
            dowait = FALSE;
            war = 0;
            for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	    {
                if ( twar[i] && ! Ships[snum].war[i] )
                    dowait = TRUE;

                if (twar[i])
                    war |= (1 << i);

                /* we'll let it happen locally as well... */
                Users[Ships[snum].unum].war[i] = twar[i];
                Ships[snum].war[i] = twar[i];
	    }

            sendCommand(CPCMD_SETWAR, (uint16_t)war);

            /* Only check for computer delay when flying. */
            if ( Ships[snum].status != SS_RESERVED && dowait )
	    {
                /* We've set war with at least one team, stall a little. */
                mcuPutMsg(
                    "Reprogramming the battle computer, please stand by...",
                    MSG_LIN2 );
                cdrefresh();
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
            break;
	}

        for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
            if ( ch == (char)tolower( Teams[i].teamchar ) )
            {
                if ( ! twar[i] || ! Ships[snum].rwar[i] )
                {
                    twar[i] = ! twar[i];
                    goto ccont1;	/* next 2  */
                }
                break;
            }
        cdbeep();

    ccont1:				/* goto  */
        ;
    }

    cdclrl( MSG_LIN1, 2 );

    return;

}

/* compose and send a message. If remote is true, we will use sendMessage,
   else stormsg (for local cb updates) */
void cucSendMsg( msgFrom_t from, uint16_t fromDetail, int terse, int remote )
{
    int i, j;
    char buf[MSGMAXLINE] = "", msg[MESSAGE_SIZE] = "";
    int ch;
    int editing;
    char *mto="Message to: ";
    char *nf="Not found.";
    char *huh="I don't understand.";

    int append_flg;  /* when user types to the limit. */
    int do_append_flg;

    static msgTo_t to = MSG_TO_NOONE;
    static uint16_t toDetail = 0;

    /* First, find out who we're sending to. */
    cdclrl( MSG_LIN1, 2 );
    buf[0] = 0;
    ch = cdgetx( mto, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, TRUE );
    if ( ch == TERM_ABORT )
    {
        cdclrl( MSG_LIN1, 1 );
        return;
    }

    /* TAB or ENTER means use the target from the last message. */
    editing = ( (ch == TERM_EXTRA || ch == TERM_NORMAL) && buf[0] == 0 );
    if ( editing )
    {
        /* Make up a default string using the last target. */
        if ( to == MSG_TO_SHIP && toDetail < MAXSHIPS )
        {
            sprintf( buf, "%d", toDetail );
        }
        else if ( to == MSG_TO_TEAM && toDetail >= 0
                  && toDetail < NUMPLAYERTEAMS )
        {
            strcpy(buf , Teams[toDetail].name) ;
        }
        else
        {
            switch ( to )
            {
            case MSG_TO_ALL:
                strcpy(buf , "All") ;
                break;
            case MSG_TO_GOD:
                strcpy(buf , "GOD") ;
                break;
            case MSG_TO_IMPLEMENTORS:
                strcpy(buf , "Implementors") ;
                break;
            case MSG_TO_FRIENDLY:
                strcpy(buf , "Friend") ;
                break;
            default:
                to = MSG_TO_NOONE;
                toDetail = 0;
                buf[0] = 0;
                break;
             }
        }

    }

    /* Got a target, parse it. */
    utDeleteBlanks( buf );
    utToUpperCase( buf );
    if (utIsDigits(buf))
    {
        /* All digits means a ship number. */
        i = 0;
        utSafeCToI( &j, buf, i );		/* ignore status */
        if ( j < 0 || j >= MAXSHIPS )
	{
            mcuPutMsg( "No such ship.", MSG_LIN2 );
            return;
	}
        if ( Ships[j].status != SS_LIVE )
	{
            mcuPutMsg( nf, MSG_LIN2 );
            return;
	}
        to = MSG_TO_SHIP;
        toDetail = (uint16_t)j;
    }
    else
    {
        switch ( buf[0] )
        {
        case 'A':
        case 'a':
            to = MSG_TO_ALL;
            toDetail = 0;
            break;
        case 'G':
        case 'g':
            to = MSG_TO_GOD;
            toDetail = 0;
            break;
        case 'I':
        case 'i':
            to = MSG_TO_IMPLEMENTORS;
            toDetail = 0;
            break;
        default:
            /* check for 'Friend' */
            if (buf[0] == 'F' && buf[1] == 'R')
            {			/* to friendlies */
                to = MSG_TO_FRIENDLY;
                toDetail = 0;
            }
            else
            {
                /* Check for a team character. */
                for ( i = 0; i < NUMPLAYERTEAMS; i++ )
                    if ( buf[0] == Teams[i].teamchar
                         || buf[0] == (char)tolower(Teams[i].teamchar) )
                        break;

                if ( i >= NUMPLAYERTEAMS )
                {
                    mcuPutMsg( huh, MSG_LIN2 );
                    return;
                }
                to = MSG_TO_TEAM;
                toDetail = (uint16_t)i;
            }
            break;
        }
    }

    /* Now, construct a header for the selected target. */
    strcpy(buf , "Message to ") ;
    if ( to == MSG_TO_SHIP && toDetail < MAXSHIPS )
    {
        if ( Ships[toDetail].status != SS_LIVE )
	{
            mcuPutMsg( nf, MSG_LIN2 );
            return;
	}
        utAppendShip(buf, (int)toDetail) ;
        utAppendChar(buf, ':') ;
    }
    else if ( to == MSG_TO_TEAM && toDetail < NUMPLAYERTEAMS )
    {
        strcat(buf , Teams[toDetail].name) ;
        strcat(buf , "s:") ;
    }
    else switch ( to )
         {
         case MSG_TO_ALL:
             strcat(buf , "everyone:") ;
             break;
         case MSG_TO_GOD:
             strcat(buf , "GOD:") ;
             break;
         case MSG_TO_IMPLEMENTORS:
             strcat(buf , "The Implementors:") ;
             break;
         case MSG_TO_FRIENDLY:
             strcat(buf , "Friend:") ;
             break;
         default:
             mcuPutMsg( huh, MSG_LIN2 );
             return;
             break;
         }

    if ( ! terse )
        strcat(buf, " ([ESC] to abort)");

    mcuPutMsg( buf, MSG_LIN1 );
    cdclrl( MSG_LIN2, 1 );

    if ( ! editing )
        msg[0] = 0;

    i = MESSAGE_SIZE;

    append_flg = TRUE;
    while (append_flg == TRUE) {
        append_flg = FALSE;
        do_append_flg = TRUE;
        msg[0] = 0;
        if ( cdgetp( ">", MSG_LIN2, 1, TERMS, msg, i,
                     &append_flg, do_append_flg, TRUE ) != TERM_ABORT )
        {
            if ( to != MSG_TO_IMPLEMENTORS )
            {
                if (remote)		/* remotes don't send 'from' */
                    sendMessage(to, toDetail, msg);
                else /* conqoper */
                    clbStoreMsg( from, fromDetail, to, toDetail, msg );
            }
            else
            {
                /* Handle a message to the Implementors. */
                strcpy(buf , "Communique from ") ;
                if ( from == MSG_FROM_SHIP && fromDetail < MAXSHIPS )
                {
                    strcat(buf , Ships[fromDetail].alias) ;
                    strcat(buf , " on board ") ;
                    utAppendShip(buf , (int)fromDetail) ;
                }
                else if ( from == MSG_FROM_GOD )
                    strcat(buf , "GOD") ;
                else
                {
                    utAppendChar(buf , '(') ;
                    utAppendInt(buf , fromDetail) ;
                    utAppendChar(buf, ')');
                }
                if (remote)           /* remotes don't send 'from' */
                    sendMessage(MSG_TO_IMPLEMENTORS, 0, msg);
                else // GOD == IMP (conqoper) 
                    clbStoreMsg( from, fromDetail,
                                 MSG_TO_IMPLEMENTORS, 0, msg );

                /* log it to the logfile too */
                utLog("MSG: MESSAGE TO IMPLEMENTORS: %s: %s", buf, msg);
            }
        }
    } /* end while loop */
    cdclrl( MSG_LIN1, 2 );

    return;

}
