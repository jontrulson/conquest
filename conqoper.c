#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/*                               C O N Q O P E R */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"

static char *conqoperId = "$Id$";

/*##  conqoper - main program */
main(int argc, char *argv[])
{
  int i;
  int l;
  char name[MAXUSERNAME];
  string cpr=COPYRIGHT;

  extern char *optarg;
  extern int optind;

  glname( name );
  if ( ! isagod(NULL) )
    {
      printf( "Poor cretins such as yourself lack the");
      error( " skills necessary to use this program.\n" );
    }
  
  while ((i = getopt(argc, argv, "C")) != EOF)    /* get command args */
    switch (i)
      {
      case 'C': 
	GetSysConf(TRUE);		/* init defaults... */
	if (MakeSysConf() == ERR)
	  exit(1);
	else
	  exit(0);
	
        break;
	
      case '?':
      default: 
	fprintf(stderr, "usage: %s [-C]\n", argv[0]);
	fprintf(stderr, "       -C\trebuild systemwide conquestrc file\n");
	exit(1);
	break;
      }

  if ((ConquestUID = GetConquestUID()) == ERR)
    {
      fprintf(stderr, "conqoper: GetConquestUID() failed\n");
      exit(1);
    }
  
  if ((ConquestGID = GetConquestGID()) == ERR)
    {
      fprintf(stderr, "conqoper: GetConquestGID() failed\n");
      exit(1);
    }
  
  if (GetSysConf(FALSE) == ERR)
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
      /* */
    }
  
  if (setgid(ConquestGID) == -1)
    {
      clog("conqoper: setgid(%d): %s",
	   ConquestGID,
	   sys_errlist[errno]);
      fprintf(stderr, "conqoper: setgid(): failed\n");
      exit(1);
    }
  
#ifdef USE_SEMS
  if (GetSem() == ERR)
    {
      fprintf(stderr, "GetSem() failed to get semaphores. exiting.\n");
      exit(1);
    }
#endif
  
#ifdef SET_PRIORITY
  /* Increase our priority a bit */
  
  if (nice(CONQUEST_PRI) == -1)
    {
      clog("conqoper: main(): nice(CONQUEST_PRI (%d)): failed: %s",
	   CONQUEST_PRI,
	   sys_errlist[errno]);
    }
  else
    clog("conqoper: main(): nice(CONQUEST_PRI (%d)): succeeded.",
	 CONQUEST_PRI);
#endif
  
  
  map_common();		/* Map the conquest universe common block */
  
  rndini( 0, 0 );				/* initialize random numbers */
  cdinit();					/* initialize display environment */
  
  cunum = MSG_GOD;				/* stow user number */
  csnum = ERR;		/* don't display in cdgetp - JET */
  cmaxlin = cdlins( 0 );			/* number of lines */
  
  cmaxcol = cdcols( 0 );			/* number of columns */
  
  operate();
  
  cdend();
  
  exit(0);
  
}


/*##  bigbang - fire a torp from every ship */
/*  SYNOPSIS */
/*    bigbang */
void bigbang(void)
{
  int i, snum, cnt;
  real dir;
  
  dir = 0.0;
  cnt = 0;
  for ( snum = 1; snum <= MAXSHIPS; snum = snum + 1 )
    if ( sstatus[snum] == SS_LIVE )
      for ( i = 0; i < MAXTORPS; i = i + 1 )
	if ( ! launch( snum, dir, 1 ) )
	  break;
	else
	  {
	    dir = mod360( dir + 40.0 );
	    cnt = cnt + 1;
	  }
  cerror("bigbang: Fired %d torpedos, hoo hah won't they be surprised!", cnt );
  
  return;
  
}


/*##  debugdisplay - verbose and ugly version of display */
/*  SYNOPSIS */
/*    int snum */
/*    debugdisplay( snum ) */
void debugdisplay( int snum )
{
  
  /* The following aren't displayed by this routine... */
  /* int ssrpwar(snum,NUMPLANETS)	# self-ruled planets s/he is at war */
  /* int slastmsg(snum)		# last message seen */
  /* int salastmsg(snum)		# last message allowed to be seen */
  /* int smap(snum)			# strategic map or not */
  /* int spfuse(snum)			# tenths until phasers can be fired */
  /* int sscanned(snum,NUMTEAMS)	# fuse for which ships have been */
  /* int stalert(snum)			# torp alert! */
  /* int sctime(snum)			# cpu hundreths at last check */
  /* int setime(snum)			# elapsed hundreths at last check */
  /* int scacc(snum)			# accumulated cpu time */
  /* int seacc(snum)			# accumulated elapsed time */
  
  int i, j, unum, lin, col, tcol, dcol;
  real x;
  char buf[MSGMAXLINE];
  char *torpstr;
  
#define TOFF "OFF"
#define TLAUNCHING "LAUNCHING"
#define TLIVE "LIVE"
#define TDETONATE "DETONATE"
#define TFIREBALL "EXPLODING"
  
  
  cdclrl( 1, MSG_LIN1 - 1 );		/* don't clear the message lines */
  unum = suser[snum];
  
  lin = 1;
  tcol = 1;
  dcol = tcol + 10;
  cdputs( "    ship:", lin, tcol );
  buf[0] = EOS;
  appship( snum, buf );
  if ( srobot[snum] )
    appstr( " (ROBOT)", buf );
  cdputs( buf, lin, dcol );
  lin = lin + 1;
  cdputs( "      sx:", lin, tcol );
  cdputr( oneplace(sx[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "      sy:", lin, tcol );
  cdputr( oneplace(sy[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "     sdx:", lin, tcol );
  cdputr( oneplace(sdx[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "     sdy:", lin, tcol );
  cdputr( oneplace(sdy[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "  skills:", lin, tcol );
  cdputr( oneplace(skills[snum] + sstrkills[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "   swarp:", lin, tcol );
  x = oneplace(swarp[snum]);
  if ( x == ORBIT_CW )
    cdputs( "ORBIT_CW", lin, dcol );
  else if ( x == ORBIT_CCW )
    cdputs( "ORBIT_CCW", lin, dcol );
  else
    cdputr( x, 0, lin, dcol );
  lin = lin + 1;
  cdputs( "  sdwarp:", lin, tcol );
  cdputr( oneplace(sdwarp[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "   shead:", lin, tcol );
  cdputn( round(shead[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "  sdhead:", lin, tcol );
  cdputn( round(sdhead[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( " sarmies:", lin, tcol );
  cdputn( sarmies[snum], 0, lin, dcol );
  
  lin = 1;
  tcol = 23;
  dcol = tcol + 12;
  cdputs( "      name:", lin, tcol );
  if ( spname[snum] != EOS )
    cdputs( spname[snum], lin, dcol ); /* -[] */
  lin = lin + 1;
  cdputs( "  username:", lin, tcol );
  buf[0] = EOS;
  if ( unum >= 0 && unum < MAXUSERS )
    {
      c_strcpy( cuname[unum], buf );
      if ( buf[0] != EOS )
	appchr( ' ', buf );
    }
  appchr( '(', buf );
  appint( unum, buf );
  appchr( ')', buf );
  cdputs( buf, lin, dcol );
  lin = lin + 1;
  cdputs( "     slock:", lin, tcol );
  cdputs( "       dtt:", lin + 1, tcol );
  i = slock[snum];
  if ( -i >= 1 && -i <= NUMPLANETS )
    {
      cdputs( pname[-i], lin, dcol );	/* -[] */
      cdputn( round( dist( sx[snum], sy[snum], px[-i], py[-i] ) ),
	     0, lin + 1, dcol );
    }
  else if ( i != 0 )
    cdputn( i, 0, lin, dcol );
  lin = lin + 2;
  cdputs( "     sfuel:", lin, tcol );
  cdputn( round(sfuel[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "       w/e:", lin, tcol );
  sprintf( buf, "%d/%d", sweapons[snum], sengines[snum] );
  if ( swfuse[snum] > 0 || sefuse[snum] > 0 )
    {
      appstr( " (", buf );
      appint( swfuse[snum], buf );
      appchr( '/', buf );
      appint( sefuse[snum], buf );
      appchr( ')', buf );
    }
  cdputs( buf, lin, dcol );
  lin = lin + 1;
  cdputs( "      temp:", lin, tcol );
  sprintf( buf, "%d/%d", round(swtemp[snum]), round(setemp[snum]) );
  cdputs( buf, lin, dcol );
  lin = lin + 1;
  cdputs( "   ssdfuse:", lin, tcol );
  i = ssdfuse[snum];
  buf[0] = EOS;
  if ( i != 0 )
    {
      sprintf( buf, "%d ", i );
    }
  if ( scloaked[snum] )
    appstr( "(CLOAKED)", buf );
  cdputs( buf, lin, dcol );
  lin = lin + 1;
  cdputs( "      spid:", lin, tcol );
  i = spid[snum];
  if ( i != 0 )
    {
      sprintf( buf, "%d", i );
      cdputs( buf, lin, dcol );
    }
  lin = lin + 1;
  cdputs( "slastblast:", lin, tcol );
  cdputr( oneplace(slastblast[snum]), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "slastphase:", lin, tcol );
  cdputr( oneplace(slastphase[snum]), 0, lin, dcol );
  
  lin = 1;
  tcol = 57;
  dcol = tcol + 12;
  cdputs( "   sstatus:", lin, tcol );
  buf[0] = EOS;
  appsstatus( sstatus[snum], buf );
  cdputs( buf, lin, dcol );
  lin = lin + 1;
  cdputs( " skilledby:", lin, tcol );
  i = skilledby[snum];
  if ( i != 0 )
    {
      buf[0] = EOS;
      appkb( skilledby[snum], buf );
      cdputs( buf, lin, dcol );
    }
  lin = lin + 1;
  cdputs( "   shields:", lin, tcol );
  cdputn( round(sshields[snum]), 0, lin, dcol );
  if ( ! sshup[snum] )
    cdput( 'D', lin, dcol+5 );
  lin = lin + 1;
  cdputs( "   sdamage:", lin, tcol );
  cdputn( round(sdamage[snum]), 0, lin, dcol );
  if ( srmode[snum] )
    cdput( 'R', lin, dcol+5 );
  lin = lin + 1;
  cdputs( "  stowedby:", lin, tcol );
  i = stowedby[snum];
  if ( i != 0 )
    {
      buf[0] = EOS;
      appship( i, buf );
      cdputs( buf, lin, dcol );
    }
  lin = lin + 1;
  cdputs( "   stowing:", lin, tcol );
  i = stowing[snum];
  if ( i != 0 )
    {
      buf[0] = EOS;
      appship( i, buf );
      cdputs( buf, lin, dcol );
    }
  lin = lin + 1;
  cdputs( "      swar:", lin, tcol );
  buf[0] = '(';
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    if ( swar[snum][i] )
      buf[i+1] = chrteams[i];
    else
      buf[i+1] = '-';
  buf[NUMTEAMS+1] = ')';
  buf[NUMTEAMS+2] = EOS;
  fold( buf );
  cdputs( buf, lin, dcol );
  
  lin = lin + 1;
  cdputs( "     srwar:", lin, tcol );
  buf[0] = '(';
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    if ( srwar[snum][i] )
      buf[i+1] = chrteams[i];
    else
      buf[i+1] = '-';
  buf[NUMTEAMS+1] = ')';
  buf[NUMTEAMS+2] = EOS;
  cdputs( buf, lin, dcol );
  
  lin = lin + 1;
  cdputs( "   soption:", lin, tcol );
  c_strcpy( "(gpainte)", buf );
  for ( i = 0; i < MAXOPTIONS; i = i + 1 )
    if ( soption[snum][i] )
      buf[i+1] = cupper(buf[i+1]);
  cdputs( buf, lin, dcol );
  
  lin = lin + 1;
  cdputs( "   uoption:", lin, tcol );
  if ( unum >= 0 && unum < MAXUSERS )
    {
      c_strcpy( "(gpainte)", buf );
      for ( i = 0; i < MAXOPTIONS; i = i + 1 )
	if ( uoption[unum,i] )
	  buf[i+1] = cupper(buf[i+1]);
      cdputs( buf, lin, dcol );
    }
  
  lin = lin + 1;
  cdputs( "   saction:", lin, tcol );
  i = saction[snum];
  if ( i != 0 )
    {
      robstr( i, buf );
      cdputs( buf, lin, dcol );
    }
  
  lin = cmaxlin - MAXTORPS - 2;
  cdputs(
	 "tstatus    tfuse    tmult       tx       ty      tdx      tdy     twar",
	 lin, 3 );
  for ( i = 0; i < MAXTORPS; i = i + 1 )
    {
      lin = lin + 1;
      switch(tstatus[snum][i])
	{
	case TS_OFF:
	  torpstr = TOFF;
	  break;
	case TS_LAUNCHING:
	  torpstr = TLAUNCHING;
	  break;
	case TS_LIVE:
	  torpstr = TLIVE;
	  break;
	case TS_DETONATE:
	  torpstr = TDETONATE;
	  break;
	case TS_FIREBALL:
	  torpstr = TFIREBALL;
	  break;
	}
      
      cdputs(torpstr, lin, 3);
      if ( tstatus[snum][i] != TS_OFF )
	{
	  cdputn( tfuse[snum][i], 6, lin, 13 );
	  cdputr( oneplace(tmult[snum][i]), 9, lin, 19 );
	  cdputr( oneplace(tx[snum][i]), 9, lin, 28 );
	  cdputr( oneplace(ty[snum][i]), 9, lin, 37 );
	  cdputr( oneplace(tdx[snum][i]), 9, lin, 46 );
	  cdputr( oneplace(tdy[snum][i]), 9, lin, 55 );
	  buf[0] = '(';
	  for ( j = 0; j < NUMTEAMS; j++ )
	    if ( twar[snum][i][j] )
	      buf[j+1] = chrteams[j];
	    else
	      buf[j+1] = '-';
	  buf[NUMTEAMS+1] = ')';
	  buf[NUMTEAMS+2] = EOS;
	  cdputs( buf, lin, 67 );
	}
    }
  
  cdmove( 1, 1 );
  cdrefresh( TRUE );
  
  return;
  
}


/*##  debugplan - debugging planet list */
/*  SYNOPSIS */
/*    debugplan */
void debugplan(void)
{
  
  int i, j, k, lin, col, olin;
  int l;
  static int init = FALSE, sv[NUMPLANETS + 1];
  char buf[MSGMAXLINE], junk[10], uninhab[20];
  
  string hd="planet        C T arm uih scan        planet        C T arm uih scan";
  
  if ( init == FALSE )
    {
      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
	sv[i] = i;
      init = TRUE;
      sortplanets( sv );
    }
  
  cdclear();
  c_strcpy( hd, buf );
  lin = 1;
  cdputc( buf, lin );
  for ( i = 0; buf[i] != EOS; i = i + 1 )
    if ( buf[i] != ' ' )
      buf[i] = '-';
  lin = lin + 1;
  cdputc( buf, lin );
  lin = lin + 1;
  olin = lin;
  col = 6;
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    {
      k = sv[i];
      
      for ( j = 0; j < NUMTEAMS; j = j + 1 )
	if ( pscanned[k][j] && (j >= 0 && j < NUMTEAMS) )
	  junk[j] = chrteams[j];
	else
	  junk[j] = '-';
      junk[j] = EOS;
      j = puninhabtime[k];
      if ( j != 0 )
	sprintf( uninhab, "%d", j );
      else
	uninhab[0] = EOS;
      sprintf( buf, "%-13s %c %c %3d %3s %4s",
	     pname[k], chrplanets[ptype[k]], chrteams[pteam[k]],
	     parmies[k], uninhab, junk );
      
      cdputs( buf, lin, col );
      if ( ! preal[k] )
	cdput( '-', lin, col - 1 );
      
      lin = lin + 1;
      if ( lin == MSG_LIN1 )
	{
	  lin = olin;
	  col = 44;
	}
    }
  
  while (!more( "-- press SPACE to continue --" ))
    ;
  
  return;
  
}


/*##  doomdisplay - watch the doomsday machine */
/*  SYNOPSIS */
/*    doomdisplay */
void doomdisplay(void)
{
  
  int i, lin, col, dcol;
  char buf[MSGMAXLINE];
  
  cdclear();
  
  lin = 1;
  col = 1;
  c_strcpy( dname, buf );
  if ( *dtype != 0 )
    {
      appchr( '(', buf );
      appint( *dtype, buf );
      appchr( ')', buf );
    }
  cdputc( buf, 1 );
  
  lin = lin + 2;
  dcol = col + 11;
  cdputs( "  dstatus:", lin, col );
  cdputn( *dstatus, 0, lin, dcol );
  lin = lin + 1;
  cdputs( "       dx:", lin, col );
  cdputr( oneplace(*dx), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "       dy:", lin, col );
  cdputr( oneplace(*dy), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "      ddx:", lin, col );
  cdputr( oneplace(*ddx), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "      ddy:", lin, col );
  cdputr( oneplace(*ddy), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "    dhead:", lin, col );
  cdputn( round(*dhead), 0, lin, dcol );
  lin = lin + 1;
  cdputs( "    dlock:", lin, col );
  cdputs( "      ddt:", lin+1, col );
  i = *dlock;
  if ( -i > 0 && -i <= NUMPLANETS )
    {
      cdputs( pname[-i], lin, dcol );	/* -[] */
      cdputn( round( dist( *dx, *dy, px[-i], py[-i] ) ),
	     0, lin + 1, dcol );
    }
  else if ( i > 0 && i <= MAXSHIPS )
    {
      buf[0] = EOS;
      appship( i, buf );
      cdputs( buf, lin, dcol );
      cdputn( round( dist( *dx, *dy, sx[i], sy[i] ) ),
	     0, lin + 1, dcol );
    }
  else
    cdputn( i, 0, lin + 1, dcol );
  
  lin = lin + 2;
  
  cdmove( 1, 1 );
  
  return;
  
}


/*##  gplanmatch - GOD's check if a string matches a planet name */
/*  SYNOPSIS */
/*    int gplanmatch, pnum */
/*    char str() */
/*    int status */
/*    status = gplanmatch( str, pnum ) */
int gplanmatch( char str[], int *pnum )
{
  int i;
  
  if ( alldig( str ) == YES )
    {
      i = 0;
      if ( ! safectoi( pnum, str, i ) )
	return ( FALSE );
      if ( *pnum < 1 || *pnum > NUMPLANETS )
	return ( FALSE );
    }
  else
    return ( planmatch( str, pnum, TRUE ) );
  
  return ( TRUE );
  
}


/*##  kiss - give the kiss of death */
/*  SYNOPSIS */
/*    kiss */
void kiss(void)
{
  
  int i, snum, unum;
  char ch, buf[MSGMAXLINE];
  int l, didany;
  
  /* Find out what to kill. */
  cdclrl( MSG_LIN1, 2 );
  ch = cdgetx( "Kill what (<cr> for driver)? ", MSG_LIN1, 1,
	      TERMS, buf, MSGMAXLINE );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      cdmove( 1, 1 );
      return;
    }
  delblanks( buf );
  
  /* Kill the driver? */
  if ( buf[0] == EOS )
    {
      if ( confirm( 0 ) )
	if ( *drivstat == DRS_RUNNING )
	  *drivstat = DRS_KAMIKAZE;
      cdclrl( MSG_LIN1, 2 );
      cdmove( 1, 1 );
      return;
    }
  
  /* Kill a ship? */
  if ( alldig( buf ) == YES )
    {
      i = 0;
      l = safectoi( &snum, buf, i );		/* ignore status */
      if ( snum < 1 || snum > MAXSHIPS )
	cdputs( "No such ship.", MSG_LIN2, 1 );
      else if ( sstatus[snum] != SS_LIVE )
	cdputs( "You can't kill that ship.", MSG_LIN2, 1 );
      else if ( confirm( 0 ) )
	{
	  killship( snum, KB_GOD );
	  cdclrl( MSG_LIN1, 2 );
	}
      cdmove( 1, 1 );
      return;
    }
  
  /* Kill EVERYBODY? */
  if ( stmatch( buf, "all", FALSE ) )
    {
      didany = FALSE;
      for ( snum = 1; snum <= MAXSHIPS; snum = snum + 1 )
	if ( sstatus[snum] == SS_LIVE )
	  {
	    didany = TRUE;
	    cdclrl( MSG_LIN1, 1 );
	    c_strcpy( "Kill ship ", buf );
	    appship( snum, buf );
	    cdputs( buf, MSG_LIN1, 1 );
	    if ( confirm( 0 ) )
	      killship( snum, KB_GOD );
	  }
      if ( didany )
	cdclrl( MSG_LIN1, 2 );
      else
	cdputs( "Nobody here but us GODs.", MSG_LIN2, 1 );
      cdmove( 1, 1 );
      return;
    }
  
  /* Kill a user? */
  if ( ! gunum( &unum, buf ) )
    {
      cdputs( "No such user.", MSG_LIN2, 1 );
      cdmove( 0, 0 );
      return;
    }
  
  /* Yes. */
  didany = FALSE;
  for ( snum = 1; snum <= MAXSHIPS; snum = snum + 1 )
    if ( sstatus[snum] == SS_LIVE )
      if ( suser[snum] == unum )
	{
	  didany = TRUE;
	  cdclrl( MSG_LIN1, 1 );
	  c_strcpy( "Kill ship ", buf );
	  appship( snum, buf );
	  cdputs( buf, MSG_LIN1, 1 );
	  if ( confirm( 0 ) )
	    killship( snum, KB_GOD );
	}
  if ( ! didany )
    cdputs( "That user isn't flying right now.", MSG_LIN2, 1 );
  else
    cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*##  opback - put up the background for the operator program */
/*  SYNOPSIS */
/*    int lastrev, savelin */
/*    opback( lastrev, savelin ) */
void opback( int lastrev, int *savelin )
{
  int i, lin, col;
#include "conqcom2.h"
  
  cdclear();
  
  lin = 2;
  if ( lastrev == COMMONSTAMP )
    cdputc( "CONQUEST OPERATOR PROGRAM", lin );
  else
    {
      sprintf( cbuf, "CONQUEST COMMON BLOCK MISMATCH %d != %d",
	     lastrev, COMMONSTAMP );
      cdputc( cbuf, lin );
    }
  
  lin = lin + 2;
  *savelin = lin;
  lin = lin + 3;
  
  cdputc( "Options:", lin );
  lin = lin + 2;
  i = lin;
  
  col = 5;
  cdputs( "(f) - flip the open/closed flag", lin, col );
  lin = lin + 1;
  cdputs( "(d) - flip the doomsday machine!", lin, col );
  lin = lin + 1;
  cdputs( "(h) - hold the driver", lin, col );
  lin = lin + 1;
  cdputs( "(I) - initialize", lin, col );
  lin = lin + 1;
  cdputs( "(b) - big bang", lin, col );
  lin = lin + 1;
  cdputs( "(H) - user history", lin, col );
  lin = lin + 1;
  cdputs( "(/) - player list", lin, col );
  lin = lin + 1;
  cdputs( "(\\) - full player list", lin, col );
  lin = lin + 1;
  cdputs( "(?) - planet list", lin, col );
  lin = lin + 1;
  cdputs( "($) - debugging planet list", lin, col );
  lin = lin + 1;
  cdputs( "(p) - edit a planet", lin, col );
  lin = lin + 1;
  cdputs( "(w) - watch a ship", lin, col );
  lin = lin + 1;
  cdputs( "(i) - info", lin, col );
  
  lin = i;
  col = 45;
  cdputs( "(r) - create a robot ship", lin, col );
  lin = lin + 1;
  cdputs( "(L) - review messages", lin, col );
  lin = lin + 1;
  cdputs( "(m) - message from GOD", lin, col );
  lin = lin + 1;
  cdputs( "(T) - team stats", lin, col );
  lin = lin + 1;
  cdputs( "(U) - user stats", lin, col );
  lin = lin + 1;
  cdputs( "(S) - more user stats", lin, col );
  lin = lin + 1;
  cdputs( "(s) - special stats page", lin, col );
  lin = lin + 1;
  cdputs( "(a) - add a user", lin, col );
  lin = lin + 1;
  cdputs( "(e) - edit a user", lin, col );
  lin = lin + 1;
  cdputs( "(R) - resign a user", lin, col );
  lin = lin + 1;
  cdputs( "(k) - kiss of death", lin, col );
  lin = lin + 1;
  cdputs( "(q) - exit", lin, col );
  
  return;
  
}


/*##  operate - main part of the conquest operator program */
/*  SYNOPSIS */
/*    operate */
void operate(void)
{
  
  int i, lin, savelin, cntlockword, cntlockmesg;
  int redraw, now, readone;
  int lastrev, msgrand;
  char buf[MSGMAXLINE], junk[MSGMAXLINE];
  int ch;
  int l;
  
  *glastmsg = *lastmsg;
  glname( buf );
  if ( gunum( &i, buf ) )
    uooption[i][MAXOOPTIONS] = TRUE;
  cntlockword = 0;
  cntlockmesg = 0;
  lastrev = *commonrev;
  grand( &msgrand );
  
  redraw = TRUE;
  while (TRUE)      /* repeat */
    {
      if ( redraw || lastrev != *commonrev )
	{
	  lastrev = *commonrev;
	  opback( lastrev, &savelin );
	  redraw = FALSE;
	}
      /* Line 1. */
      c_strcpy( "game ", buf );
      if ( *closed )
	appstr( "CLOSED", buf );
      else
	appstr( "open", buf );
      appstr( ", driver ", buf );
      switch ( *drivstat )
	{
	case DRS_OFF:
	  appstr( "OFF", buf );
	  break;
	case DRS_RESTART:
	  appstr( "RESTART", buf );
	  break;
	case DRS_STARTING:
	  appstr( "STARTING", buf );
	  break;
	case DRS_HOLDING:
	  appstr( "HOLDING", buf );
	  break;
	case DRS_RUNNING:
	  appstr( "on", buf );
	  break;
	case DRS_KAMIKAZE:
	  appstr( "KAMIKAZE", buf );
	  break;
	default:
	  appstr( "???", buf );
	}
      appstr( ", eater ", buf );
      i = *dstatus;
      if ( i == DS_OFF )
	appstr( "off", buf );
      else if ( i == DS_LIVE )
	{
	  appstr( "ON (", buf );
	  i = *dlock;
	  if ( -i > 0 && -i <= NUMPLANETS )
	    appstr( pname[-i], buf );
	  else
	    appship( i, buf );		/* this will handle funny numbers */
	  appchr( ')', buf );
	}
      else
	appstr( "???", buf );
      lin = savelin;
      cdclrl( lin, 1 );
      cdputc( buf, lin );
      
      /* Line 2. */
#ifdef USE_SEMS
      strcpy(buf, GetSemVal(0));
#else
      c_strcpy( "lockword", buf );
      if ( *lockword == 0 )
	cntlockword = 0;
      else if ( cntlockword < 5 )
	cntlockword = cntlockword + 1;
      else
	upper( buf );
      c_strcpy( buf, junk );
      appstr( " = %d, ", junk );
      
      c_strcpy( "lockmesg", buf );
      if ( *lockmesg == 0 )
	cntlockmesg = 0;
      else if ( cntlockmesg < 5 )
	cntlockmesg = cntlockmesg + 1;
      else
	upper( buf );
      appstr( buf, junk );
      appstr( " = %d", junk );
      sprintf( buf, junk, *lockword, *lockmesg );
#endif
      
      lin = lin + 1;
      cdclrl( lin, 1 );
      cdputc( buf, lin );
      
      /* Display a new message, if any. */
      readone = FALSE;
      if ( dgrand( msgrand, &now ) >= NEWMSG_GRAND )
	if ( getamsg( MSG_GOD, glastmsg ) )
	  {
	    readmsg( MSG_GOD, *glastmsg, RMsg_Line );

#if defined(OPER_MSG_BEEP)
	    if (msgfrom[slastmsg[MSG_GOD]] != MSG_GOD)
	      cdbeep();
#endif
	    readone = TRUE;
	    msgrand = now;
	  }
      cdmove( 1, 1 );
      cdrefresh( TRUE );
      /* Un-read message, if there's a chance it got garbaged. */
      if ( readone )
	if ( iochav( 0 ) )
	  *glastmsg = modp1( *glastmsg - 1, MAXMESSAGES );
      
      /* Get a char with timeout. */
      if ( ! iogtimed( &ch, 1 ) )
	continue;		/* next */
      switch ( ch )
	{
	case 'a':
	  opuadd();
	  redraw = TRUE;
	  break;
	case 'b':
	  if ( confirm( 0 ) )
	    bigbang();
	  break;
	case 'd':
	  if ( *dstatus == DS_LIVE )
	    *dstatus = DS_OFF;
	  else
	    doomsday();
	  break;
	case 'e':
	  opuedit();
	  redraw = TRUE;
	  break;
	case 'f':
	  if ( *closed )
	    {
	      *closed = FALSE;
	      /* Unlock the lockwords (just in case...) */
	      PVUNLOCK(lockword);
	      PVUNLOCK(lockmesg);
	      *drivstat = DRS_OFF;
	      *drivpid = 0;
	      drivowner[0] = EOS;
	    }
	  else if ( confirm( 0 ) )
	    *closed = TRUE;
	  break;
	case 'h':
	  if ( *drivstat == DRS_HOLDING )
	    *drivstat = DRS_RUNNING;
	  else
	    *drivstat = DRS_HOLDING;
	  break;
	case 'H':
	  histlist( TRUE );
	  redraw = TRUE;
	  break;
	case 'i':
	  opinfo( MSG_GOD );
	  break;
	case 'I':
	  opinit();
	  redraw = TRUE;
	  break;
	case 'k':
	  kiss();
	  break;
	case 'L':
	  l = review( MSG_GOD, *glastmsg );
	  break;
	case 'm':
	  sendmsg( MSG_GOD, TRUE );
	  break;
	case 'p':
	  oppedit();
	  redraw = TRUE;
	  break;
	case 'q':
	case 'Q':
	  return;
	  break;
	case 'r':
	  oprobot();
	  break;
	case 'R':
	  opresign();
	  redraw = TRUE;
	  break;
	case 's':
	  opstats();
	  redraw = TRUE;
	  break;
	case 'S':
	  userstats( TRUE );
	  redraw = TRUE;
	  break;
	case 'T':
	  opteamlist();
	  redraw = TRUE;
	  break;
	case 'U':
	  userlist( TRUE );
	  redraw = TRUE;
	  break;
	case 'w':
	  watch();
	  redraw = TRUE;
	  break;
	case '/':
	  playlist( TRUE, FALSE );
	  redraw = TRUE;
	  break;
	case '\\':
	  playlist( TRUE, TRUE );
	  redraw = TRUE;
	  break;
	case '?':
	  opplanlist();
	  redraw = TRUE;
	  break;
	case '$':
	  debugplan();
	  redraw = TRUE;
	  break;
	case 0x0c:
	  cdredo();
	  redraw = TRUE;
	  break;
	case ' ':
	case TERM_NORMAL:
	case EOS:
	  /* do nothing */
	default:
	  cdbeep();
	}
      /* Disable messages for awhile. */
      grand( &msgrand );
    } /* repeat */
  
  /* NOTREACHED */
  
}


/*##  opinfo - do an operator info command */
/*  SYNOPSIS */
/*    int snum */
/*    opinfo( snum ) */
void opinfo( int snum )
{
  int i, j, now[8];
  char ch;
  int l, extra;
  string pmt="Information on: ";
  string huh="I don't understand.";
  string nf="Not found.";
  
  cdclrl( MSG_LIN1, 2 );
  
  ch = cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  extra = ( ch == TERM_EXTRA );
  
  delblanks( cbuf );
  fold( cbuf );
  if ( cbuf[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  if ( cbuf[0] == 's' && alldig( &cbuf[1] ) == YES )
    {
      i = 0;
      l = safectoi( &j, &cbuf[1], i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( alldig( cbuf ) == YES )
    {
      i = 0;
      l = safectoi( &j, cbuf, i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( gplanmatch( cbuf, &j ) )
    infoplanet( "", j, snum );
  else if ( stmatch( cbuf, "time", FALSE ) )
    {
      getnow( now );
      c_strcpy( "It's ", cbuf );
      appnumtim( now, cbuf );
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
    }
  else
    {
      cdmove( MSG_LIN2, 1 );
      c_putmsg( huh, MSG_LIN2 );
    }
  
  return;
  
}


/*##  opinit - handle the various kinds of initialization */
/*  SYNOPSIS */
/*    opinit */
void opinit(void)
{
  
  int i, lin, col, icol;
  char ch, buf[MSGMAXLINE];
  string pmt="Initialize what: ";
  
  
  cdclear();
  cdredo();
  
  lin = 2;
  icol = 11;
  cdputc( "Conquest Initialization", lin );
  
  lin = lin + 3;
  col = icol - 2;
  c_strcpy( "(r)obots", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  cdputs( "<-", lin, col );
  col = col + 4;
  c_strcpy( "(e)verything", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  cdputs( "->", lin, col );
  col = col + 4;
  c_strcpy( "(z)ero everything", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  
  col = icol + 20;
  lin = lin + 3;
  cdput( '|', lin, col );
  lin = lin + 1;
  cdput( 'v', lin, col );
  lin = lin + 3;
  
  col = icol;
  c_strcpy( "(s)hips", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  cdputs( "<-", lin, col );
  col = col + 4;
  c_strcpy( "(u)niverse", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  cdputs( "->", lin, col );
  col = col + 4;
  c_strcpy( "(g)ame", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  cdputs( "->", lin, col );
  col = col + 4;
  c_strcpy( "(p)lanets", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  
  col = icol + 20;
  lin = lin + 3;
  cdput( '|', lin, col );
  lin = lin + 1;
  cdput( 'v', lin, col );
  lin = lin + 3;
  
  col = icol + 15;
  c_strcpy( "(m)essages", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 8;
  c_strcpy( "(l)ockwords", buf );
  i = strlen( buf );
  cdputs( buf, lin, col+1 );
  cdbox( lin-1, col, lin+1, col+i+1 );
  
  while (TRUE)  /*repeat */
    {
      lin = MSG_LIN1;
      col = 30;
      cdclrl( lin, 1 );
      ch = cdgetx( pmt, lin, col, TERMS, buf, MSGMAXLINE );
      cdclrl( lin, 1 );
      cdputs( pmt, lin, col );
      col = col + strlen( pmt );
      if ( ch == TERM_ABORT || buf[0] == EOS )
	break;
      switch ( buf[0] )
	{
	case 'e':
	  cdputs( "everything", lin, col );
	  if ( confirm( 0 ) )
	    {
	      initeverything();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'z':
	  cdputs( "zero everything", lin, col );
	  if ( confirm( 0 ) )
	    zeroeverything();
	  break;
	case 'u':
	  cdputs( "universe", lin, col );
	  if ( confirm( 0 ) )
	    {
	      inituniverse();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'g':
	  cdputs( "game", lin, col );
	  if ( confirm( 0 ) )
	    {
	      initgame();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'p':
	  cdputs( "planets", lin, col );
	  if ( confirm( 0 ) )
	    {
	      initplanets();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 's':
	  cdputs( "ships", lin, col );
	  if ( confirm( 0 ) )
	    {
	      clearships();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'm':
	  cdputs( "messages", lin, col );
	  if ( confirm( 0 ) )
	    {
	      initmsgs();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'l':
	  cdputs( "lockwords", lin, col );
	  if ( confirm( 0 ) )
	    {
	      PVUNLOCK(lockword);
	      PVUNLOCK(lockmesg);
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'r':
	  cdputs( "robots", lin, col );
	  if ( confirm( 0 ) )
	    {
	      initrobots();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	default:
	  cdbeep();
	}
    }
  
  return;
  
}


/*##  oppedit - edit a planet's characteristics */
/*  SYNOPSIS */
/*    oppedit */
void oppedit(void)
{
  
  int i, j, lin, col, datacol;
  static int pnum = PNUM_JANUS;
  real x;
  int ch;
  char buf[MSGMAXLINE];
  /*    data pnum / PNUM_JANUS /;
	common / conqopp / pnum;*/
  
  col = 4;
  datacol = col + 28;
  
  cdredo();
  cdclear();
  while (TRUE) /*repeat*/
    {
      
      /* Display the planet. */
      i = 20;			/* i = 10 */
      j = 57;
      puthing(ptype[pnum], i, j );
      cdput( chrplanets[ptype[pnum]], i, j + 1);
      sprintf(buf, "%s\n", pname[pnum]);
      cdputs( buf, i + 1, j + 2 ); /* -[] */
      
      /* Display info about the planet. */
      lin = 4;
      i = pnum;
      cdputs( "(p) - Planet:\n", lin, col );
      sprintf( buf, "%s (%d)", pname[pnum], pnum );
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      i = pprimary[pnum];
      if ( i == 0 )
	{
	  lin = lin + 1;
	  
	  x = porbvel[pnum];
	  if ( x == 0.0 )
	    cdputs( "(v) - Stationary\n", lin, col );
	  else
	    {
	      cdputs( "(v) - Velocity:\n", lin, col );
	      sprintf( buf, "Warp %f", oneplace(x) );
	      cdputs( buf, lin, datacol );
	    }
	}
      else
	{
	  cdputs( "(o) - Orbiting:\n", lin, col );
	  sprintf( buf, "%s (%d)", pname[i], i );
	  cdputs( buf, lin, datacol );
	  
	  lin = lin + 1;
	  cdputs( "(v) - Orbit velocity:\n", lin, col );
	  sprintf( buf, "%.1f degrees/minute", porbvel[pnum] );
	  cdputs( buf, lin, datacol );
	}
      
      lin = lin + 1;
      cdputs( "(r) - Radius:\n", lin, col );
      sprintf( buf, "%.1f", porbrad[pnum] );
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      cdputs( "(a) - Angle:\n", lin, col );
      sprintf( buf, "%.1f", porbang[pnum] );
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      i = ptype[pnum];
      cdputs( "(t) - Type:\n", lin, col );
      sprintf( buf, "%s (%d)", ptname[i], i );
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      i = pteam[pnum];
      if ( pnum <= NUMCONPLANETS )
	cdputs( "      Owner team:\n", lin, col );
      else
	cdputs( "(T) - Owner team:\n", lin, col );
      sprintf( buf, "%s (%d)", tname[i], i );
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      cdputs( "(x,y) Position:\n", lin, col );
      sprintf( buf, "%.1f, %.1f",px[pnum], py[pnum] );
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      cdputs( "(A)   Armies:\n", lin, col );
      sprintf( buf, "%d", parmies[pnum] );
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      cdputs( "(s)   Scanned by:\n", lin, col );
      buf[0] = '(';
      for ( i = 1; i <= NUMTEAMS; i = i + 1 )
	if ( pscanned[pnum][i - 1] )
	  buf[i] = chrteams[i - 1];
	else
	  buf[i] = '-';
      buf[NUMTEAMS+1] = ')';
      buf[NUMTEAMS+2] = '\0';
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      cdputs( "(u)   Uninhabitable time:\n", lin, col );
      sprintf( buf, "%d", puninhabtime[pnum] );
      cdputs( buf, lin, datacol );
      
      lin = lin + 1;
      if ( preal[pnum] )
	cdputs( "(-) - Visible\n", lin, col );
      else
	cdputs( "(+) - Hidden\n", lin, col );
      
      lin = lin + 1;
      cdputs( "(n) - Change planet name\n", lin, col );
      
      lin = lin + 1;
      cdputs( "<TAB> increment planet number\n", lin, col );
      
      cdclra(MSG_LIN1, 0, MSG_LIN1 + 2, cdcols(0) - 1);
      
      cdmove( 0, 0 );
      cdrefresh( TRUE );
      
      if ( ! iogtimed( &ch, 1 ) )
	continue;		/* next */
      switch ( ch )
	{
	case 'a':
	  /* Angle. */
	  ch = getcx( "New angle? ", MSG_LIN1, 0,
		     TERMS, buf, MSGMAXLINE );
	  if ( ch == TERM_ABORT || buf[0] == EOS )
	    continue;	/* next */
	  delblanks( buf );
	  i = 0;
	  if ( ! safectoi( &j, buf, i ) )
	    continue;	/* next */

	  x = ctor( buf);
	  if ( x < 0.0 || x > 360.0 )
	    continue;	/* next */
	  porbang[pnum] = x;
	  break;
	case 'A':
	  /* Armies. */
	  ch = getcx( "New number of armies? ",
		     MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
	  if ( ch == TERM_ABORT | buf[0] == EOS )
	    continue;
	  delblanks( buf );
	  i = 0;
	  if ( ! safectoi( &j, buf, i ) )
	    continue;
	  parmies[pnum] = j;
	  break;
	case 'n':
	  /* New planet name. */
	  ch = getcx( "New name for this planet? ",
		     MSG_LIN1, 0, TERMS, buf, MAXPLANETNAME );
	  if ( ch != TERM_ABORT && ( ch == TERM_EXTRA || buf[0] != EOS ) )
	    stcpn( buf, pname[pnum], MAXPLANETNAME );
	  break;
	case 'o':
	  /* New primary. */
	  ch = getcx( "New planet to orbit? ",
		     MSG_LIN1, 0, TERMS, buf, MAXPLANETNAME );
	  if ( ch == TERM_ABORT || buf[0] == EOS )
	    continue;	/* next */
	  if ( buf[1] == '0' && buf[2] == EOS )
	    pprimary[pnum] = 0;
	  else if ( gplanmatch( buf, &i ) )
	    pprimary[pnum] = i;
	  break;
	case 'v':
	  /* Velocity. */
	  ch = getcx( "New velocity? ",
		     MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
	  if ( ch == TERM_ABORT || buf[0] == EOS )
	    continue;	/* next */
	  delblanks( buf );
	  i = 0;
	  if ( ! safectoi( &j, buf, i ) )
	    continue;	/* next */

	  porbvel[pnum] = ctor( buf );
	  break;
	case 'T':
	  /* Rotate owner team. */
	  if ( pnum > NUMCONPLANETS )
	    {
	      pteam[pnum] = modp1( pteam[pnum] + 1, NUMALLTEAMS );
	    }
	  else
	    cdbeep();
	  break;
	case 't':
	  /* Rotate planet type. */
	  ptype[pnum] = modp1( ptype[pnum] + 1, MAXPLANETTYPES );
	  break;
	case 'x':
	  /* X coordinate. */
	  ch = getcx( "New X coordinate? ",
		     MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
	  if ( ch == TERM_ABORT || buf[0] == EOS )
	    continue;	/* next */
	  delblanks( buf );
	  i = 0;
	  if ( ! safectoi( &j, buf, i ) )
	    continue;	/* next */

	  px[pnum] = ctor( buf );
	  break;
	case 'y':
	  /* Y coordinate. */
	  ch = getcx( "New Y coordinate? ",
		     MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
	  if ( ch == TERM_ABORT || buf[0] == EOS )
	    continue;	/* next */
	  delblanks( buf );
	  i = 0;
	  if ( ! safectoi( &j, buf, i ) )
	    continue;	/* next */

	  py[pnum] = ctor( buf );
	  break;
	case 's':
	  /* Scanned. */
	  cdputs( "Toggle which team? ", MSG_LIN1, 1 );
	  cdmove( MSG_LIN1, 20 );
	  cdrefresh( TRUE );
	  ch = cupper( iogchar( ch ) );
	  for ( i = 0; i < NUMTEAMS; i = i + 1 )
	    if ( ch == chrteams[i] )
	      {
		pscanned[pnum][i] = ! pscanned[pnum][i];
		break;
	      }
	  break;
	  /* Uninhabitable minutes */
	case 'u':
	  ch = getcx( "New uninhabitable minutes? ",
		     MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
	  if ( ch == TERM_ABORT | buf[0] == EOS )
	    continue;
	  delblanks( buf );
	  i = 0;
	  if ( ! safectoi( &j, buf, i ) )
	    continue;
	  puninhabtime[pnum] = j;
	  break;
	case 'p':
	  ch = getcx( "New planet to edit? ",
		     MSG_LIN1, 0, TERMS, buf, MAXPLANETNAME );
	  if ( ch == TERM_ABORT || buf[0] == EOS )
	    continue;	/* next */
	  if ( gplanmatch( buf, &i ) )
	    pnum = i;
	  break;
	case 'r':
	  /* Radius. */
	  ch = getcx( "New radius? ",
		     MSG_LIN1, 0, TERMS, buf, MSGMAXLINE );
	  if ( ch == TERM_ABORT || buf[0] == EOS )
	    continue;	/* next */
	  delblanks( buf );
	  i = 0;
	  if ( ! safectoi( &j, buf, i ) )
	    continue;	/* next */

	  porbrad[pnum] = ctor( buf );
	  break;
	case '+':
	  /* Now you see it... */
	  preal[pnum] = TRUE;
	  break;
	case '-':
	  /* Now you don't */
	  preal[pnum] = FALSE;
	  break;
	case TERM_EXTRA:
	  /* Rotate planet number. */
	  
	  /*		pnum++;
			i = mod( pnum , NUMPLANETS );
			pnum = i;
			*/
	  
	  /*		pnum = mod( pnum + 1, NUMPLANETS );*/
	  
	  pnum = mod( pnum + 1, NUMPLANETS );
	  pnum = (pnum == 0) ? NUMPLANETS : pnum;
	  break;
	case ' ':
	  /* do no-thing */
	  break;
	case '\x0c':
	  cdredo();
	  break;
	case TERM_NORMAL:
	case TERM_ABORT:
	case 'q':
	case 'Q':
	  return;
	  break;
	default:
	  cdbeep();
	}
    }/* while (TRUE) */
  
  /* NOTREACHED */
  
}


/*##  opplanlist - display the planet list for an operator */
/*  SYNOPSIS */
/*    opplanlist */
void opplanlist(void)
{
  
  int ch;
  
  cdclear();
  do
    {
      planlist( TEAM_NOTEAM );		/* we get extra info */
      putpmt( "--- press space when done ---", MSG_LIN2 );
      cdrefresh( TRUE );
    } while (!iogtimed(&ch, 1));
}


/*##  opresign - resign a user */
/*  SYNOPSIS */
/*    opresign */
void opresign(void)
{
  
  int unum;
  char ch, buf[MSGMAXLINE];
  
  cdclrl( MSG_LIN1, 2 );
  ch = cdgetx( "Resign user: ", MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  delblanks( buf );
  if ( ! gunum( &unum, buf ) )
    {
      cdputs( "No such user.", MSG_LIN2, 1 );
      cdmove( 1, 1 );
      cdrefresh( FALSE );
      c_sleep( 1.0 );
    }
  else if ( confirm( 0 ) )
    resign( unum );
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*##  oprobot - handle gratuitous robot creation */
/*  SYNOPSIS */
/*    oprobot */
void oprobot(void)
{
  
  int i, j, snum, unum, num, anum;
  char ch, buf[MSGMAXLINE];
  int l, warlike;
  
  cdclrl( MSG_LIN1, 2 );
  ch = cdgetx( "Enter username for new robot (Orion, Federation, etc): ",
	      MSG_LIN1, 1, TERMS, buf, MAXUSERNAME );
  if ( ch == TERM_ABORT || buf[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  delblanks( buf );
  if ( ! gunum( &unum, buf ) )
    {
      cdputs( "No such user.", MSG_LIN2, 1 );
      return;
    }
  
  /* Defaults. */
  num = 1;
  warlike = FALSE;
  
  if ( ch == TERM_EXTRA )
    {
      ch = cdgetx( "Enter number desired (TAB for warlike): ",
		  MSG_LIN2, 1, TERMS, buf, MAXUSERNAME );
      if ( ch == TERM_ABORT )
	{
	  cdclrl( MSG_LIN1, 2 );
	  return;
	}
      warlike = ( ch == TERM_EXTRA );
      delblanks( buf );
      i = 0;
      l = safectoi( &num, buf, i );
      if ( num <= 0 )
	num = 1;
    }
  
  anum = 0;
  for ( i = 1; i <= num; i = i + 1 )
    {
      if ( ! newrob( &snum, unum ) )
	{
	  c_putmsg( "Failed to create robot ship.", MSG_LIN1 );
	  break;
	}
      
      anum = anum + 1;
      
      /* If requested, make the robot war-like. */
      if ( warlike )
	{
	  for ( j = 0; j < NUMTEAMS; j = j + 1 )
	    swar[snum][j] = TRUE;
	  swar[snum][steam[snum]] = FALSE;
	}
    }
  
  /* Report the good news. */
  sprintf( buf, "Automation %s (%s) is now flying ",
	 upname[unum], cuname[unum] );
  if ( anum == 1 )
    appship( snum, buf );
  else
    {
      appint( anum, buf );
      appstr( " new ships.", buf );
    }
  cdclrl( MSG_LIN2, 1 );
  i = MSG_LIN1;
  if ( anum != num )
    i = i + 1;
  c_putmsg( buf, i );
  
  return;
  
}


/*##  opstats - display operator statistics */
/*  SYNOPSIS */
/*    opstats */
void opstats(void)
{
  
  int i, lin, col;
  unsigned long size;
  char buf[MSGMAXLINE], junk[MSGMAXLINE], timbuf[32];
  int ch;
  real x;
  string sfmt="%32s %12s\n";
  string tfmt="%32s %20s\n";
  string pfmt="%32s %11.1f%%\n";
  
  col = 8;
  cdclear();
  do /*repeat*/
    {
      lin = 2;
      fmtseconds( *ccpuseconds, timbuf );
      sprintf( buf, sfmt, "Conquest cpu time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      i = *celapsedseconds;
      fmtseconds( i, timbuf );
      sprintf( buf, sfmt, "Conquest elapsed time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      if ( i == 0 )
	x = 0.0;
      else
	x = oneplace( 100.0 * float(*ccpuseconds) / float(i) );
      sprintf( buf, pfmt, "Conquest cpu usage:", x );
      cdputs( buf, lin, col );
      
      lin = lin + 2;
      fmtseconds( *dcpuseconds, timbuf );
      sprintf( buf, sfmt, "Conqdriv cpu time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      i = *delapsedseconds;
      fmtseconds( i, timbuf );
      sprintf( buf, sfmt, "Conqdriv elapsed time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      if ( i == 0 )
	x = 0.0;
      else
	x = oneplace( 100.0 * float(*dcpuseconds) / float(i) );
      sprintf( buf, pfmt, "Conqdriv cpu usage:", x );
      cdputs( buf, lin, col );
      
      lin = lin + 2;
      fmtseconds( *rcpuseconds, timbuf );
      sprintf( buf, sfmt, "Robot cpu time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      i = *relapsedseconds;
      fmtseconds( i, timbuf );
      sprintf( buf, sfmt, "Robot elapsed time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      if ( i == 0 )
	x = 0.0;
      else
	x = ( 100.0 * float(*rcpuseconds) / float(i) );
      sprintf( buf, pfmt, "Robot cpu usage:", x );
      cdputs( buf, lin, col );
      
      lin = lin + 2;
      sprintf( buf, tfmt, "Last initialize:", inittime );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      sprintf( buf, tfmt, "Last conquer:", conqtime );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      fmtseconds( *playtime, timbuf );
      sprintf( buf, sfmt, "Driver time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      fmtseconds( *drivtime, timbuf );
      sprintf( buf, sfmt, "Play time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      sprintf( buf, tfmt, "Last upchuck:", lastupchuck );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      getdandt( timbuf );
      sprintf( buf, tfmt, "Current time:", timbuf );
      cdputs( buf, lin, col );
      
      lin = lin + 2;
      if ( drivowner[0] != EOS )
	sprintf( junk, "%d (%s), ", *drivpid, drivowner );
      else if ( *drivpid != 0 )
	sprintf( junk, "%d, ", *drivpid );
      else
	junk[0] = EOS;
      sprintf( buf, "%sdrivsecs = %03d, drivcnt = %d\n",
	     junk, *drivsecs, *drivcnt);
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      comsize( &size );
      sprintf( buf, "%u bytes (out of %d) in the common block.\n",
	     size, SIZEOF_COMMONBLOCK );
      cdputs( buf, lin, col );
      
      lin = lin + 1;
      sprintf( buf, "Common ident is %d", *commonrev );
      if ( *commonrev != COMMONSTAMP )
	{
	  sprintf( junk, " (binary ident is %d)\n", COMMONSTAMP );
	  appstr( junk, buf );
	}
      cdputs( buf, lin, col );
      
      cdmove( 1, 1 );
      cdrefresh( TRUE );
    }
  while ( !iogtimed( &ch, 1 ) ); /* until */
  
  return;
  
}


/*##  opteamlist - display the team list for an operator */
/*  SYNOPSIS */
/*    opteamlist */
void opteamlist(void)
{
  
  int ch;
  
  cdclear();
  do /* repeat*/
    {
      teamlist( -1 );
      putpmt( "--- press space when done ---", MSG_LIN2 );
      cdrefresh( TRUE );
    }
  while ( !iogtimed( &ch, 1 ) ); /* until */
  
}


/*##  opuadd - add a user */
/*  SYNOPSIS */
/*    opuadd */
void opuadd(void)
{
  
  int i, unum, team;
  char ch;
  char buf[MSGMAXLINE], junk[MSGMAXLINE], name[MSGMAXLINE];
  
  cdclrl( MSG_LIN1, 2 );
  ch = cdgetx( "Add user: ", MSG_LIN1, 1, TERMS, name, MAXUSERNAME );
  delblanks( name );
  if ( ch == TERM_ABORT || name[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  if ( gunum( &unum, name ) )
    {
      cdputs( "That user is already enrolled.", MSG_LIN2, 1 );
      cdmove( 1, 1 );
      cdrefresh( FALSE );
      c_sleep( 1.0 );
      cdclrl( MSG_LIN1, 2 );
      return;
    }
  for ( team = -1; team == -1; )
    {
      sprintf(junk, "Select a team (%c%c%c%c): ",
	     chrteams[TEAM_FEDERATION],
	     chrteams[TEAM_ROMULAN],
	     chrteams[TEAM_KLINGON],
	     chrteams[TEAM_ORION]);
      
      cdclrl( MSG_LIN1, 1 );
      ch = cdgetx( junk, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
      if ( ch == TERM_ABORT )
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
      else if ( ch == TERM_EXTRA && buf[0] == EOS )
	team = rndint( 0, NUMTEAMS - 1);
      else
	{
	  ch = cupper( buf[0] );
	  for ( i = 0; i < NUMTEAMS; i = i + 1 )
	    if ( chrteams[i] == ch )
	      {
		team = i;
		break;
	      }
	}
    }
  
  
  buf[0] = EOS;
  apptitle( team, buf );
  appchr( ' ', buf );
  i = strlen( buf );
  appstr( name, buf );
  buf[i] = cupper( buf[i] );
  buf[MAXUSERPNAME] = EOS;
  if ( ! c_register( name, buf, team, &unum ) )
    {
      cdputs( "Error adding new user.", MSG_LIN2, 1 );
      cdmove( 0, 0 );
      cdrefresh( FALSE );
      c_sleep( 1.0 );
    }
  cdclrl( MSG_LIN1, 2 );
  
  return;

}


/*##  opuedit - edit a user */
/*  SYNOPSIS */
/*    opuedit */
void opuedit(void)
{
  
#define MAXUEDITROWS (MAXOPTIONS+2) 
  int i, unum, row = 1, lin, col, olin, tcol, dcol, lcol, rcol;
  char buf[MSGMAXLINE];
  int l, ch, left = TRUE;
  
  cdclrl( MSG_LIN1, 2 );
  ch = getcx( "Edit which user: ", MSG_LIN1, 0, TERMS, buf, MAXUSERNAME );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 2 );
      return;
    }
  delblanks( buf );
  if ( ! gunum( &unum, buf ) )
    {
      cdclrl( MSG_LIN1, 2 );
      cdputs( "Unknown user.", MSG_LIN1, 1 );
      cdmove( 1, 1 );
      cdrefresh( FALSE );
      c_sleep( 1.0 );
      return;
    }
  cdclear();
  cdclrl( MSG_LIN1, 2 );
  
  while (TRUE) /* repeat */
    {
      /* Do the right side first. */
      lin = 1;
      tcol = 43;
      dcol = 62;
      rcol = dcol - 1;
      
      cdputs( "         Username:", lin, tcol );
      cdputs( cuname[unum], lin, dcol ); /* -[] */
      
      lin = lin + 1;
      cdputs( "   Multiple count:", lin, tcol );
      cdputn( umultiple[unum], 0, lin, dcol );
      
      lin = lin + 1;
      for ( i = 0; i < MAXOPTIONS; i = i + 1 )
	{
	  sprintf( buf, "%17d:", i );
	  cdputs( buf, lin+i, tcol );
	  if ( uoption[unum][i] )
	    cdput( 'T', lin+i, dcol );
	  else
	    cdput( 'F', lin+i, dcol );
	}
      cdputs( "  Phaser graphics:", lin+OPT_PHASERGRAPHICS, tcol );
      cdputs( "     Planet names:", lin+OPT_PLANETNAMES, tcol );
      cdputs( "       Alarm bell:", lin+OPT_ALARMBELL, tcol );
      cdputs( "  Intruder alerts:", lin+OPT_INTRUDERALERT, tcol );
      cdputs( "      Numeric map:", lin+OPT_NUMERICMAP, tcol );
      cdputs( "            Terse:", lin+OPT_TERSE, tcol );
      cdputs( "       Explosions:", lin+OPT_EXPLOSIONS, tcol );
      
      lin = lin + MAXOPTIONS + 1;
      cdputs( "          Urating:", lin, tcol );
      cdputr( oneplace(urating[unum]), 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( "             Uwar:", lin, tcol );
      buf[0] = '(';
      for ( i = 0; i < NUMTEAMS; i = i + 1 )
	if ( uwar[unum][i] )
	  buf[i+1] = chrteams[i];
	else
	  buf[i+1] = '-';
      buf[NUMTEAMS+1] = ')';
      buf[NUMTEAMS+2] = EOS;
      cdputs( buf, lin, dcol );
      
      lin = lin + 1;
      cdputs( "           Urobot:", lin, tcol );
      if ( urobot[unum] )
	cdput( 'T', lin, dcol );
      else
	cdput( 'F', lin, dcol );
      
      /* Now the left side. */
      lin = 1;
      tcol = 3;
      dcol = 22;
      lcol = dcol - 1;
      
      cdputs( "             Name:", lin, tcol );
      cdputs( upname[unum], lin, dcol ); /* -[] */
      
      lin = lin + 1;
      cdputs( "             Team:", lin, tcol );
      i = uteam[unum];
      if ( i < 0 || i >= NUMTEAMS )
	cdputn( i, 0, lin, dcol );
      else
	cdputs( tname[i], lin, dcol ); /* -[] */
      
      lin = lin + 1;
      for ( i = 0; i < MAXOOPTIONS; i = i + 1 )
	{
	  sprintf( buf, "%17d:", i );
	  cdputs( buf, lin+i, tcol );
	  if ( uooption[unum][i] )
	    cdput( 'T', lin+i, dcol );
	  else
	    cdput( 'F', lin+i, dcol );
	}
      cdputs( "         Multiple:", lin+OOPT_MULTIPLE, tcol );
      cdputs( "     Switch teams:", lin+OOPT_SWITCHTEAMS, tcol );
      cdputs( " Play when closed:", lin+OOPT_PLAYWHENCLOSED, tcol );
      cdputs( "          Disable:", lin+OOPT_SHITLIST, tcol );
      cdputs( "     GOD messages:", lin+OOPT_GODMSG, tcol );
      cdputs( "             Lose:", lin+OOPT_LOSE, tcol );
      cdputs( "        Autopilot:", lin+OOPT_AUTOPILOT, tcol );
      
      lin = lin + MAXOOPTIONS + 1;
      cdputs( "       Last entry:", lin, tcol );
      cdputs( ulastentry[unum], lin, dcol ); /* -[] */
      
      lin = lin + 1;
      cdputs( "  Elapsed seconds:", lin, tcol );
      fmtseconds( ustats[unum][TSTAT_SECONDS], buf );
      i = dcol + 11 - strlen( buf );
      cdputs( buf, lin, i );
      
      lin = lin + 1;
      cdputs( "      Cpu seconds:", lin, tcol );
      fmtseconds( ustats[unum][TSTAT_CPUSECONDS], buf );
      i = dcol + 11 - strlen ( buf );
      cdputs( buf, lin, i );
      
      lin = lin + 1;
      
      /* Do column 4 of the bottom stuff. */
      olin = lin;
      tcol = 62;
      dcol = 72;
      cdputs( "Maxkills:", lin, tcol );
      cdputn( ustats[unum][USTAT_MAXKILLS], 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( "Torpedos:", lin, tcol );
      cdputn( ustats[unum][USTAT_TORPS], 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( " Phasers:", lin, tcol );
      cdputn( ustats[unum][USTAT_PHASERS], 0, lin, dcol );
      
      /* Do column 3 of the bottom stuff. */
      lin = olin;
      tcol = 35;
      dcol = 51;
      cdputs( " Planets taken:", lin, tcol );
      cdputn( ustats[unum][USTAT_CONQPLANETS], 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( " Armies bombed:", lin, tcol );
      cdputn( ustats[unum][USTAT_ARMBOMB], 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( "   Ship armies:", lin, tcol );
      cdputn( ustats[unum][USTAT_ARMSHIP], 0, lin, dcol );
      
      /* Do column 2 of the bottom stuff. */
      lin = olin;
      tcol = 18;
      dcol = 29;
      cdputs( " Conquers:", lin, tcol );
      cdputn( ustats[unum][USTAT_CONQUERS], 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( "    Coups:", lin, tcol );
      cdputn( ustats[unum][USTAT_COUPS], 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( "Genocides:", lin, tcol );
      cdputn( ustats[unum][USTAT_GENOCIDE], 0, lin, dcol );
      
      /* Do column 1 of the bottom stuff. */
      lin = olin;
      tcol = 1;
      dcol = 10;
      cdputs( "   Wins:", lin, tcol );
      cdputn( ustats[unum][USTAT_WINS], 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( " Losses:", lin, tcol );
      cdputn( ustats[unum][USTAT_LOSSES], 0, lin, dcol );
      
      lin = lin + 1;
      cdputs( "Entries:", lin, tcol );
      cdputn( ustats[unum][USTAT_ENTRIES], 0, lin, dcol );
      
      /* Display the stuff */
      cdputc( "Use arrow keys to position, SPACE to modify, q to quit.", MSG_LIN2 );
      if ( left )
	i = lcol;
      else
	i = rcol;
      cdput( '+', row, i );
      cdmove( row, i );
      cdrefresh( TRUE );
      
      /* Now, get a char and process it. */
      if ( ! iogtimed( &ch, 1 ) )
	continue;		/* next */
      switch ( ch )
	{
	case 'a': 
	case 'A': 
	case 'h': 
	case 'H':
	case KEY_LEFT:
	  /* Left. */
	  left = TRUE;
	  break;
	case 'd': 
	case 'D': 
	case 'l': 
	case 'L':
	case KEY_RIGHT:
	  /* Right. */
	  left = FALSE;
	  break;
	case 'w': 
	case 'W': 
	case 'k':
	case 'K':
	case KEY_UP:
	  /* Up. */
	  row = max( row - 1, 1 );
	  break;
	case 'x':
	case  'X':
	case 'j':
	case  'J':
	case KEY_DOWN:
	  /* Down. */
	  row = min( row + 1, MAXUEDITROWS );
	  break;
	case 'q': 
	case 'Q':
	  return;
	  break;
	case 'y':
	case 'Y':
	case KEY_A1:
	case KEY_HOME:
	  /* Up and left. */
	  left = TRUE;
	  row = max( row - 1, 1 );
	  break;
	case 'e': 
	case 'E':
	case 'u':
	case 'U':
	case KEY_PPAGE:
	case KEY_A3:
	  /* Up and right. */
	  left = FALSE;
	  row = max( row - 1, 1 );
	  break;
	case 'z':
	case 'Z':
	case 'b':
	case 'B':
	case KEY_END:
	case KEY_C1:
	  /* Down and left. */
	  left = TRUE;
	  row = min( row + 1, MAXUEDITROWS );
	  break;
	case 'c':
	case 'C':
	case 'n':
	case 'N':
	case KEY_NPAGE:
	case KEY_C3:
	  /* Down and right. */
	  left = FALSE;
	  row = min( row + 1, MAXUEDITROWS );
	  break;
	case ' ':
	  /* Modify the current entry. */
	  if ( left && row == 1 )
	    {
	      /* Pseudonym. */
	      cdclrl( MSG_LIN2, 1 );
	      ch = getcx( "Enter a new pseudonym: ",
			 MSG_LIN2, 0, TERMS, buf, MAXUSERPNAME );
	      if ( ch != TERM_ABORT &&
		  ( buf[0] != EOS || ch == TERM_EXTRA ) )
		stcpn( buf, upname[unum], MAXUSERPNAME ); /* -[] */
	    }
	  else if ( ! left && row == 1 )
	    {
	      /* Username. */
	      cdclrl( MSG_LIN2, 1 );
	      ch = getcx( "Enter a new username: ",
			 MSG_LIN2, 0, TERMS, buf, MAXUSERNAME );
	      if ( ch != TERM_ABORT &&
		  ( buf[0] != EOS || ch == TERM_EXTRA ) );
	      {
		delblanks( buf );
		if ( ! gunum( &i, buf ) )
		  stcpn( buf, cuname[unum], MAXUSERNAME ); /* -[] */
		else
		  {
		    cdclrl( MSG_LIN1, 2 );
		    cdputc( "That username is already in use.",
			   MSG_LIN2 );
		    cdmove( 1, 1 );
		    cdrefresh( FALSE );
		    c_sleep( 1.0 );
		  }
	      }
	    }
	  else if ( left && row == 2 )
	    {
	      /* Team. */
	      uteam[unum] = modp1( uteam[unum] + 1, NUMTEAMS );
	    }
	  else if ( ! left && row == 2 )
	    {
	      /* Multiple count. */
	      cdclrl( MSG_LIN2, 1 );
	      ch = getcx( "Enter new multiple count: ",
			 MSG_LIN2, 0, TERMS, buf, MSGMAXLINE );
	      if ( ch != TERM_ABORT && buf[0] != EOS )
		{
		  delblanks( buf );
		  i = 0;
		  l = safectoi( &umultiple[unum], buf, i );
		}
	    }
	  else
	    {
	      i = row - 3;
	      if ( left )
		if ( i >= 0 && i < MAXOOPTIONS )
		  uooption[unum][i] = ! uooption[unum][i];
		else
		  cdbeep();
	      else
		if ( i >= 0 && i < MAXOPTIONS )
		  uoption[unum][i] = ! uoption[unum][i];
		else
		  cdbeep();
	    }
	  break;
	case TERM_NORMAL:
	case TERM_EXTRA:
	case TERM_ABORT:
	  break;
	case '\x0c':
	  cdredo();
	  break;
	default:
	  cdbeep();
	}
    }
  
  /* NOTREACHED */
  
}


/*##  watch - peer over someone's shoulder */
/*  SYNOPSIS */
/*    watch */
void watch(void)
{
  
  int i, snum;
  int ch, tch, l, normal;
  int msgrand, readone, now;
  char buf[MSGMAXLINE];
  string pmt="Watch which ship (<cr> for doomsday)? ";
  string nss="No such ship.";
  string nf="Not found.";
  string help1="M - message from GOD, L - review messages,";
  string help2="K - kill this ship, H - this message";
  
  cdclrl( MSG_LIN1, 2 );
  tch = cdgetx( pmt, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
  if ( tch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  if ( strlen( buf ) == 0 )
    {
      /* Doomsday. */
      cdclear();
      cdredo();
      do			/* repeat */
	{
	  doomdisplay();
	  cdrefresh( TRUE );
	}
      while ( !iogtimed( &ch, 1 ) ); /* until */
    }
  else
    {
      normal = ( tch != TERM_EXTRA );		/* line feed means debugging */
      delblanks( buf );
      if ( alldig( buf ) != YES )
	{
	  cdputs( nss, MSG_LIN2, 1 );
	  cdmove( 1, 1 );
	  cdrefresh( FALSE );
	  c_sleep( 1.0 );
	  return;
	}
      i = 0;
      l = safectoi( &snum, buf, i );		/* ignore status */
      if ( snum < 1 || snum > MAXSHIPS )
	{
	  cdputs( nss, MSG_LIN2, 1 );
	  cdmove( 1, 1 );
	  cdrefresh( FALSE );
	  c_sleep( 1.0 );
	}
      else
	{
	  credraw = TRUE;
	  cdclear();
	  cdredo();
	  grand( &msgrand );
	  while (TRUE)	/* repeat */
	    {
	      /* Try to display a new message. */
	      readone = FALSE;
	      if ( dgrand( msgrand, &now ) >= NEWMSG_GRAND )
		if ( getamsg( MSG_GOD, glastmsg ) )
		  {
		    readmsg( MSG_GOD, *glastmsg, RMsg_Line );
		    if (msgfrom[slastmsg[MSG_GOD]] != MSG_GOD)
		      cdbeep();
		    msgrand = now;
		    readone = TRUE;
		  }
	      
	      /* Drive the display. */
	      if ( normal )
		display( snum );
	      else
		debugdisplay( snum );
	      
	      /* Un-read message, if there's a chance it got garbaged. */
	      if ( readone )
		if ( iochav( 0 ) )
		  *glastmsg = modp1( *glastmsg - 1, MAXMESSAGES );
	      
	      /* Get a char with timeout. */
	      if ( ! iogtimed( &ch, 1 ) )
		continue; /* next */
	      cdclrl( MSG_LIN1, 2 );
	      switch ( ch )
		{
		case 'h':
		case 'H':
		  cdputc( help1, MSG_LIN1 );
		  cdputc( help2, MSG_LIN2 );
		  break;
		case 'i':
		  opinfo( MSG_GOD );
		  break;
		case 'k':
		  if ( confirm( 0 ) )
		    killship( snum, KB_GOD );
		  break;
		case 'm':
		  sendmsg( MSG_GOD, TRUE );
		  break;
		case 'L':
		  l = review( MSG_GOD, *glastmsg );
		  break;
		case '\x0c':
		  cdredo();
		  credraw = TRUE;
		  break;
		case 'q':
		case 'Q':
		  return;
		default:
		  break;
		}
	      /* Disable messages for awhile. */
	      grand( &msgrand );
	    }
	  
	}
    }
  
  return;
  
}


