#include "c_defs.h"

/************************************************************************
 *
 * client specific stuff
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#include "global.h"
#include "conf.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"
#include "client.h"
#include "clientlb.h"
#include "conqcom.h"
#include "context.h"
#include "record.h"

void clntPseudo( int unum, int snum )
{
  char ch, buf[MSGMAXLINE];
  
  buf[0] = EOS;

  cdclrl( MSG_LIN1, 2 );
  c_strcpy( "Old pseudonym: ", buf );
  if ( snum > 0 && snum <= MAXSHIPS )
    appstr( Ships[snum].alias, buf );
  else
    appstr( Users[unum].alias, buf );
  cdputc( buf, MSG_LIN1 );
  ch = getcx( "Enter a new pseudonym: ",
	     MSG_LIN2, -4, TERMS, buf, MAXUSERPNAME );
  if ( ch == TERM_ABORT || buf[0] == EOS)
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
void clntDoWar( int snum )
{
  int i, entertime, now; 
  Unsgn8 war;
  int twar[NUMPLAYERTEAMS], dowait;
  int ch;
  const int POffset = 47, WOffset = 61;
  
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    twar[i] = Ships[snum].war[i];
  
  cdclrl( MSG_LIN1, 2 );
  
  cdputs(
	 "Press TAB when done, ESCAPE to abort:  Peace: # # # #  War: # # # #", 
	 MSG_LIN1, 1 );
  
  while ( stillalive( Context.snum ) )
    {
      for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	if ( twar[i] )
	  {
	    cdput( ' ', MSG_LIN1, POffset + (i*2) );
	    if ( Ships[snum].rwar[i] )
	      ch = Teams[i].teamchar;
	    else
	      ch = (char)tolower(Teams[i].teamchar);
	    cdput( ch, MSG_LIN1, WOffset + (i*2) );
	  }
	else
	  {
	    cdput( (char)tolower(Teams[i].teamchar), MSG_LIN1, POffset + (i*2) );
	    cdput( ' ', MSG_LIN1, WOffset+(i*2) );
	  }
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
	  
	  sendCommand(CPCMD_SETWAR, (Unsgn16)war);

	  /* Only check for computer delay when flying. */
	  if ( Ships[snum].status != SS_RESERVED && dowait )
	    {
	      /* We've set war with at least one team, stall a little. */
	      c_putmsg(
		       "Reprogramming the battle computer, please stand by...",
		       MSG_LIN2 );
	      cdrefresh();
	      grand( &entertime );
	      while ( dgrand( entertime, &now ) < REARM_GRAND )
		{
		  /* See if we're still alive. */
		  if ( ! stillalive( Context.snum ) )
		    return;
		  
		  /* Sleep */
 		  c_sleep( ITER_SECONDS );
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

/* put a recieved message into the clinet's copy of the message buffer
   (Msgs[]).  Pretty much a copy of stormsg() without the locking */
void clntStoreMessage(spMessage_t *msg)
{
  int nlastmsg;

  if (!msg)
    return;

  nlastmsg = modp1( ConqInfo->lastmsg + 1, MAXMESSAGES );
  strncpy(Msgs[nlastmsg].msgbuf, msg->msg, MESSAGE_SIZE);
  Msgs[nlastmsg].msgfrom = (int)((Sgn16)ntohs(msg->from));
  Msgs[nlastmsg].msgto = (int)((Sgn16)ntohs(msg->to));
  Msgs[nlastmsg].flags = msg->flags;
  ConqInfo->lastmsg = nlastmsg;

  /* Remove allowable last message restrictions. */
  Ships[Context.snum].alastmsg = LMSG_READALL;

  return;
}


/* compose and send a message. If remote is true, we will use sendMessage,
   else stormsg (for local cb updates) */
void clntSendMsg( int from, int terse, int remote )
{
  int i, j; 
  char buf[MSGMAXLINE] = "", msg[MESSAGE_SIZE] = "";
  int ch;
  int editing; 
  string mto="Message to: ";
  string nf="Not found.";
  string huh="I don't understand.";

  int append_flg;  /* when user types to the limit. */
  int do_append_flg;
  
  static int to = MSG_NOONE;
  
  /* First, find out who we're sending to. */
  cdclrl( MSG_LIN1, 2 );
  buf[0] = EOS;
  ch = cdgetx( mto, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE, TRUE );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  /* TAB or RETURN means use the target from the last message. */
  editing = ( (ch == TERM_EXTRA || ch == TERM_NORMAL) && buf[0] == EOS );
  if ( editing )
    {
      /* Make up a default string using the last target. */
      if ( to > 0 && to <= MAXSHIPS )
	sprintf( buf, "%d", to );
      else if ( -to >= 0 && -to < NUMPLAYERTEAMS )
	c_strcpy( Teams[-to].name, buf );
      else switch ( to )
	{
	case MSG_ALL:
	  c_strcpy( "All", buf );
	  break;
	case MSG_GOD:
	  c_strcpy( "GOD", buf );
	  break;
	case MSG_IMPLEMENTORS:
	  c_strcpy( "Implementors", buf );
	  break;
	case MSG_FRIENDLY:
	  c_strcpy( "Friend", buf );
	  break;
	default:
	  buf[0] = EOS;
	  break;
	}
      
    }
  
  /* Got a target, parse it. */
  delblanks( buf );
  upper( buf );
  if ( alldig( buf ) == TRUE )
    {
      /* All digits means a ship number. */
      i = 0;
      safectoi( &j, buf, i );		/* ignore status */
      if ( j < 1 || j > MAXSHIPS )
	{
	  c_putmsg( "No such ship.", MSG_LIN2 );
	  return;
	}
      if ( Ships[j].status != SS_LIVE )
	{
	  c_putmsg( nf, MSG_LIN2 );
	  return;
	}
      to = j;
    }
  else switch ( buf[0] )
    {
    case 'A':
    case 'a':
      to = MSG_ALL;
      break;
    case 'G':
    case 'g':
      to = MSG_GOD;
      break;
    case 'I':
    case 'i':
      to = MSG_IMPLEMENTORS;
      break;
    default:
      /* check for 'Friend' */
      if (buf[0] == 'F' && buf[1] == 'R')
	{			/* to friendlies */
	  to = MSG_FRIENDLY;
	}
      else
	{
	  /* Check for a team character. */
	  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	    if ( buf[0] == Teams[i].teamchar || buf[0] == (char)tolower(Teams[i].teamchar) )
	      break;
	  if ( i >= NUMPLAYERTEAMS )
	    {
	      c_putmsg( huh, MSG_LIN2 );
	      return;
	    }
	  to = -i;
	};
      break;
    }
  
  /* Now, construct a header for the selected target. */
  c_strcpy( "Message to ", buf );
  if ( to > 0 && to <= MAXSHIPS )
    {
      if ( Ships[to].status != SS_LIVE )
	{
	  c_putmsg( nf, MSG_LIN2 );
	  return;
	}
      appship( to, buf );
      appchr( ':', buf );
    }
  else if ( -to >= 0 && -to < NUMPLAYERTEAMS )
    {
      appstr( Teams[-to].name, buf );
      appstr( "s:", buf );
    }
  else switch ( to ) 
    {
    case MSG_ALL:
      appstr( "everyone:", buf );
      break;
    case MSG_GOD:
      appstr( "GOD:", buf );
      break;
    case MSG_IMPLEMENTORS:
      appstr( "The Implementors:", buf );
      break;
    case MSG_FRIENDLY:
      appstr( "Friend:", buf );
      break;
    default:
      c_putmsg( huh, MSG_LIN2 );
      return;
      break;
    }
  
  if ( ! terse )
    appstr( " (ESCAPE to abort)", buf );
  
  c_putmsg( buf, MSG_LIN1 );
  cdclrl( MSG_LIN2, 1 );
  
  if ( ! editing )
    msg[0] = EOS;
  
  if ( to == MSG_IMPLEMENTORS )
    i = MSGMAXLINE;
  else
    i = MESSAGE_SIZE;
  
  append_flg = TRUE;
  while (append_flg == TRUE) {
  append_flg = FALSE;
  do_append_flg = TRUE;
  msg[0] = EOS;
  if ( cdgetp( ">", MSG_LIN2, 1, TERMS, msg, i, 
	       &append_flg, do_append_flg, TRUE ) != TERM_ABORT )
    {
      if ( to != MSG_IMPLEMENTORS )
	{
	  if (remote)		/* remotes don't send 'from' */
	    sendMessage(to, msg);
	  else
	    stormsg( from, to, msg ); /* conqoper */
	}
      else
	{
	  /* Handle a message to the Implementors. */
	  c_strcpy( "Communique from ", buf );
	  if ( from > 0 && from <= MAXSHIPS )
	    {
	      appstr( Ships[from].alias, buf );
	      appstr( " on board ", buf );
	      appship( from, buf );
	    }
	  else if ( from == MSG_GOD )
	    appstr( "GOD", buf );
	  else
	    {
	      appchr( '(', buf );
	      appint( from, buf );
	      appchr( ')', buf );
	    }
	  if (remote)           /* remotes don't send 'from' */
	    sendMessage(MSG_IMPLEMENTORS, msg);
	  else
	    stormsg( from, MSG_IMPLEMENTORS, msg ); /* GOD == IMP (conqoper) */
	  /* log it to the logfile too */
	  clog("MSG: MESSAGE TO IMPLEMENTORS: %s: %s", buf, msg);
	}
    }
  } /* end while loop */ 
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}

/* feedback messages are sent by the server using spMessage_t's like
   normal messages.  However, these messages are displayed immediately,
   as well as being displayed on MSG_LIN1 */
void clntDisplayFeedback(char *msg)
{
  if (!msg)
    return;

  c_putmsg(msg, MSG_LIN1);

  return;
}

/* return a static string containing the server's stringified  flags */
char *clntServerFlagsStr(spServerStat_t *sstat)
{
  static char serverflags[256];

  if (sstat->flags == SPSSTAT_FLAGS_NONE)
    strcpy(serverflags, "None");
  else
    strcpy(serverflags, "");

  if (sstat->flags & SPSSTAT_FLAGS_REFIT)
    strcat(serverflags, "Refit ");
  
  if (sstat->flags & SPSSTAT_FLAGS_VACANT)
    strcat(serverflags, "Vacant ");
  
  if (sstat->flags & SPSSTAT_FLAGS_SLINGSHOT)
    strcat(serverflags, "SlingShot ");
  
  if (sstat->flags & SPSSTAT_FLAGS_NODOOMSDAY)
    strcat(serverflags, "NoDoomsday ");
  
  if (sstat->flags & SPSSTAT_FLAGS_KILLBOTS)
    strcat(serverflags, "Killbots ");
  
  if (sstat->flags & SPSSTAT_FLAGS_SWITCHTEAM)
    strcat(serverflags, "SwitchTeam ");

  return serverflags;
}
  


