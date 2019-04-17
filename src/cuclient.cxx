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
#include "ui.h"

void cucPseudo( int unum, int snum )
{
    char ch, buf[MSGMAXLINE];

    buf[0] = 0;

    cdclrl( MSG_LIN1, 2 );
    strcpy(buf , "Old pseudonym: ") ;
    if ( snum >= 0 && snum < cbLimits.maxShips() )
        strcat(buf , cbShips[snum].alias) ;
    else
        strcat(buf , cbUsers[unum].alias) ;
    cdputc( buf, MSG_LIN1 );
    ch = mcuGetCX( "Enter a new pseudonym: ",
                   MSG_LIN2, -4, TERMS, buf, MAX_USERNAME );
    if ( ch == TERM_ABORT || buf[0] == 0)
    {
        cdclrl( MSG_LIN1, 2 );
        return;
    }

    sendSetName(buf);

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
    static const char *mto="Message to: ";
    static const char *nf="Not found.";
    static const char *huh="I don't understand.";

    int append_flg;  /* when user types to the limit. */
    int do_append_flg;

    static msgTo_t to = MSG_TO_NOONE;
    static uint16_t toDetail = 0;

    /* First, find out who we're sending to. */
    cdclrl( MSG_LIN1, 2 );
    buf[0] = 0;
    ch = cdgetx( mto, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, true );
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
        if ( to == MSG_TO_SHIP && toDetail < cbLimits.maxShips() )
        {
            sprintf( buf, "%d", toDetail );
        }
        else if ( to == MSG_TO_TEAM && toDetail < NUMPLAYERTEAMS )
        {
            strcpy(buf , cbTeams[toDetail].name) ;
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
        utSafeCToI( &j, buf, 0 );		/* ignore status */
        if ( j < 0 || j >= cbLimits.maxShips() )
	{
            uiPutMsg( "No such ship.", MSG_LIN2 );
            return;
	}
        if ( cbShips[j].status != SS_LIVE )
	{
            uiPutMsg( nf, MSG_LIN2 );
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
                    if ( buf[0] == cbTeams[i].teamchar
                         || buf[0] == (char)tolower(cbTeams[i].teamchar) )
                        break;

                if ( i >= NUMPLAYERTEAMS )
                {
                    uiPutMsg( huh, MSG_LIN2 );
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
    if ( to == MSG_TO_SHIP && toDetail < cbLimits.maxShips() )
    {
        if ( cbShips[toDetail].status != SS_LIVE )
	{
            uiPutMsg( nf, MSG_LIN2 );
            return;
	}
        utAppendShip(buf, (int)toDetail) ;
        utAppendChar(buf, ':') ;
    }
    else if ( to == MSG_TO_TEAM && toDetail < NUMPLAYERTEAMS )
    {
        strcat(buf , cbTeams[toDetail].name) ;
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
             uiPutMsg( huh, MSG_LIN2 );
             return;
             break;
         }

    if ( ! terse )
        strcat(buf, " ([ESC] to abort)");

    uiPutMsg( buf, MSG_LIN1 );
    cdclrl( MSG_LIN2, 1 );

    if ( ! editing )
        msg[0] = 0;

    i = MESSAGE_SIZE;

    append_flg = true;
    while (append_flg == true) {
        append_flg = false;
        do_append_flg = true;
        msg[0] = 0;
        if ( cdgetp( ">", MSG_LIN2, 1, TERMS, msg, i,
                     &append_flg, do_append_flg, true ) != TERM_ABORT )
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
                if ( from == MSG_FROM_SHIP && fromDetail < cbLimits.maxShips() )
                {
                    strcat(buf , cbShips[fromDetail].alias) ;
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
