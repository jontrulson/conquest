#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

/* gamlb - game restriction library */

/* ver  date   who remarks */
/* --- ------- --- ------------------------------------------------------ */
/* 03c 10sep85 jp  .Added feature to disallow specified groups of */
/*                  terminals.  Default is to disallow nty*. */
/*                 .Removed gamcronfile() and added the cronfile argument */
/*                  to gamlinit(). */
/* 03b 27May85 cal .Reformatted. Added simple truename feature. Modified */
/*                  to use vmserror(). Fixed bugs in gamcmp(). */
/* 03a 11Jun84 cal .Added gamcronfile() and removed the cronfile argument */
/*                  to gamlinit(). Cleaned up some system calls. */
/* 02e 19Apr84 cal .Modified to use getnow() and wkday(). Added code to */
/*                  set up an exit handler to insure restoring the */
/*                  process priority. */
/* 02d 15Jul83 cal .Ratfix, added NOIMPLICITs. */
/* 02c 24Apr83 cal .Added a lower level of initialization. */
/* 02b 31jan82 cal .Added code to protect system int names from */
/*                  group or process names. */
/* 02a 01jan82 cal .Modifed to use formatted file to determine legal */
/*                  game hours. Modified gamnumtim to bias day of week */
/*                  with Monday == 1 (just like unix.) */
/* 01b 12nov82 NJD .Changed game-restriction hours. */
/* 01a 02nov82 JP  .First written. */


/*##  gamcheck - check game cron entry (internal) */
/*  SYNOPSIS */
/*    int truth, gamcheck */
/*    truth = gamcheck( 0 ) */
/* Returns YES if the current game cron entry fits the current time and */
/* date, else NO. There are 6 fields in a valid entry in the game cron file. */
/* Fields are seperated with COLONs; the fields are minute (0-59), hour */
/* (0-23), day of month (1-31), month of year (1-12), day of week (1-7 */
/* where 1 == Monday), and finally a random message. Each entry specifies */
/* one case when it is allowable to play a game. */
/* A range may be specified by using a MINUS. A list is seperated with */
/* COMMAs. A STAR matches any time. Also, the HASH char can be used */
/* for comment lines. */
int gamcheck( int dummy )
{
  
  /* JET - always return true - for now */
  return(TRUE);
}


/*##  gamcmp - check game time field (internal) */
/*  SYNOPSIS */
/*    int truth, gamcmp */
/*    int num */
/*    char buf() */
/*    truth = gamcmp( num, buf ) */
int gamcmp( int num, char buf[] )
{
  
  /* JET always return TRUE - for now */
  return(TRUE);
  
}


/*##  gamcronfile - return the pathname of the cron file (internal) */
/*  SYNOPSIS */
/*    char file() */
/*    gamcronfile( file ) */
void gamcronfile( char file[] )
{
  
  c_strcpy( "/misc/lib/gamcron", file );
  
  return;
  
}


/*##  gamdialup - check whether the user is on a dialup */
/*  SYNOPSIS */
/*    int truth, gamdialup */
/*    truth = gamdialup( 0 ) */
/*  DESCRIPTION */
/*    If a terminal has the attributes remote and modem, it is */
/*    considered to be a dialup. */
int gamdialup( int dummy )
{
  
  /* JET always FALSE - for now */
  return(FALSE);
}


/*##  gamend - terminate a game */
/*  SYNOPSIS */
/*    gamend */
void gamend(void)
{
  return;
  
}


/*##  gaminit - initialize for a game */
/*  SYNOPSIS */
/*    char truename() */
/*    gaminit( truename ) */
/* This is the default routine used to initialize games. Dialup and network */
/* use are not allowed, priority altering is desired, checking of the game */
/* cron file is desired, the priority is lowered to the default, and "guest" */
/* is not allowed to play. */
void gaminit( char truename[] )
{
  char cronfile[FILENAMESIZE];
  
  gamcronfile( cronfile );
  gamlinit( FALSE, TRUE, TRUE, 0, "", "", truename,
	   cronfile );
  
  return;
  
}


/*##  gamlinit - initialize for a game, low level */
/*  SYNOPSIS */
/*    int fdial, fprio, fcron */
/*    int despri */
/*    char badlist(), badttylist(), truename() */
/*    gamlinit( fdial, fprio, fcron, despri, badlist, badttylist, */
/*        truename, cronfile ) */
void gamlinit( int fdial, int fprio, int fcron, int despri, char *badlist, 
	      char *badttylist, char *truename, char *cronfile )
{
  int i, status;
  int newpri, state;
  int desblk[4];
  char buf[MAXLINE], gamcron[FILENAMESIZE], image[FILENAMESIZE];
  char ntruename[FILENAMESIZE];
  
  /* No games for cretins. */
  if ( badlist[0] != EOS )
    {
      glname( buf );
      if ( gamtname( buf, badlist, FALSE ) )
	error( "You are NOT allowed to play this game." );
    }
  
  /* No games on specified terminals. */
  if ( badttylist[0] != EOS )
    {
      termin( buf );
      fold( buf );
      if ( gamtname( buf, badttylist, TRUE ) )
	error( "Games are not allowed on this terminal." );
    }
  
  /* Check dialup usage. */
  if ( ! fdial )
    if ( gamdialup( 0 ) )
      error( "Games are not allowed over dial-up lines." );
  
  return;
  
}


/*##  gamimage - get the pathname this image was executed from */
/*  SYNOPSIS */
/*    char image() */
/*    gamimage( image ) */
void gamimage( char image[] )
{
  /* JET - nothing done here */
  image[0] = '\0';
  return;
  
}


/*##  gamtname - test for a name (internal) */
/*  SYNOPSIS */
/*    int gamtname, flag, leadingmatch */
/*    char name(), list() */
/*    flag = gamtname( name, list, leadingmatch ) */
/* The names in list are seperated with COLONs.  The leadingmatch boolean */
/* says that the names in list only have to match a leading substring of */
/* name - e.g. a list of "nty:ttb" would match all terminals beginning with */
/* nty or ttb. */
int gamtname( char name[], char list[], int leadingmatch )
{
  char tmpstr[16];
  
  sprintf(tmpstr, ":%s:", name);
  
  if (strstr(list, tmpstr) == NULL)
    return(FALSE);
  else
    return(TRUE);
}


