#include "c_defs.h"

/************************************************************************
 * conquest curses using stuff
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/


#include "conqdef.h"
#include "conqcom.h"
#include "conqlb.h"
#include "context.h"

#include "conf.h"
#include "global.h"
#include "color.h"
#include "cd2lb.h"
#include "iolb.h"
#include "cumisc.h"
#include "ui.h"

static char cbuf[MID_BUFFER_SIZE]; /* general purpose buffer */

/*  histlist - display the last usage list */
/*  SYNOPSIS */
/*    int godlike */
/*    cumHistList( godlike ) */
void cumHistList( int godlike )
{
  int i, j, unum, lin, col, fline, lline, thistptr = 0;
  int ch;
  char *hd0="C O N Q U E S T   U S E R   H I S T O R Y";
  char puname[MAXUSERNAME + 2]; /* for '\0' and '@' */
  char connecttm[BUFFER_SIZE];
  char histentrytm[DATESIZE + 1];

				/* Do some screen setup. */
  cdclear();
  fline = 1;
  lline = MSG_LIN1 - 1;
  cprintf(fline,0,ALIGN_CENTER,"#%d#%s",LabelColor, hd0);
  fline = fline + 2;
  
  thistptr = -1;		/* force an update the first time */
  while (TRUE) /* repeat */
    {
      if ( ! godlike )
	if ( ! clbStillAlive( Context.snum ) )
	  break;
      
      thistptr = ConqInfo->histptr;
      lin = fline;
      col = 1;
      cdclrl( fline, lline - fline + 1 );
      
      i = thistptr + 1;
      for ( j = 0; j < MAXHISTLOG; j++ )
	{
	  i = modp1( i - 1, MAXHISTLOG );
	  unum = History[i].histunum;
	  
	  if ( unum < 0 || unum >= MAXUSERS )
	    continue; 
	  if ( ! Users[unum].live )
	    continue; 

	  strcpy(puname, Users[unum].username);
	  
				/* entry time */
	  getdandt( histentrytm, History[i].histlog);
	  
	  
				/* now elapsed time */
	  fmtseconds((int) History[i].elapsed, connecttm);
	  /* strip off seconds, or for long times, anything after 7 bytes */
	  connecttm[7] = '\0';
	  
	  cprintf( lin, col, ALIGN_NONE, 
		   "#%d#%-10.10s #%d#%16s#%d#-#%d#%7s", 
		   YellowLevelColor,
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
      
      cumPutPrompt( MTXT_DONE, MSG_LIN2 );
      cdrefresh();
      if ( iogtimed( &ch, 1.0 ) )
	break;				/* exit loop if we got one */
    }
  
  return;
  
}



/*  puthing - put an object on the display */
/*  SYNOPSIS */
/*    int what, lin, col */
/*    cumPutThing( what, lin, col ) */
void cumPutThing( int what, int lin, int col )
{
  int i, j, tlin, tcol;
  char buf[3][7];
  
  switch ( what )
    {
    case PLANET_SUN:
      c_strcpy( " \\|/ ", buf[0] );
      c_strcpy( "-- --", buf[1] );
      c_strcpy( " /|\\ ", buf[2] );
      break;
    case PLANET_CLASSM:
    case PLANET_CLASSA:
    case PLANET_CLASSO:
    case PLANET_CLASSZ:
    case PLANET_DEAD:
    case PLANET_GHOST:
      c_strcpy( " .-. ", buf[0] );
      c_strcpy( "(   )", buf[1] );
      c_strcpy( " `-' ", buf[2] );
      break;
    case PLANET_MOON:
      c_strcpy( "     ", buf[0] );
      c_strcpy( " ( ) ", buf[1] );
      c_strcpy( "     ", buf[2] );
      break;
    case THING_EXPLOSION:
      c_strcpy( " %%% ", buf[0] );
      c_strcpy( "%%%%%", buf[1] );
      c_strcpy( " %%% ", buf[2] );
      break;
    case THING_DEATHSTAR:
      c_strcpy( "/===\\", buf[0] );
      c_strcpy( "===O=", buf[1] );
      c_strcpy( "\\===/", buf[2] );
      break;
    default:
      c_strcpy( " ??? ", buf[0] );
      c_strcpy( "?????", buf[1] );
      c_strcpy( " ??? ", buf[2] );
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
/*    cumReadMsg( snum, msgnum ) */
int cumReadMsg( int snum, int msgnum, int dsplin )
{
  char buf[MSGMAXLINE];
  unsigned int attrib = 0;
  
  
  buf[0] = '\0';
  
  if (Context.hascolor)
    {				/* set up the attrib so msg's are cyan */
      attrib = CyanColor;
    }

  clbFmtMsg(Msgs[msgnum].msgto, Msgs[msgnum].msgfrom, buf);
  appstr( ": ", buf );
  appstr( Msgs[msgnum].msgbuf, buf );

  uiPutColor(attrib);
  cumPutMsg( buf, dsplin );
  uiPutColor(0);
				/* clear second line if sending to MSG_LIN1 */
  if (dsplin == MSG_LIN1)
    {
      cdclrl( MSG_LIN2, 1 );
    }
  
  return(TRUE);
  
}

				/* convert a KP key into an angle */
int cumKPAngle(int ch, real *angle)
{
  int rv;
  
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
int cumKP2DirKey(int *ch)
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


/* display the conquest logo. returns last line number following */
int cumConqLogo(void)
{
  int col, lin, lenc1;
  string c1=" CCC    OOO   N   N   QQQ   U   U  EEEEE   SSSS  TTTTT";
  string c2="C   C  O   O  NN  N  Q   Q  U   U  E      S        T";
  string c3="C      O   O  N N N  Q   Q  U   U  EEE     SSS     T";
  string c4="C   C  O   O  N  NN  Q  Q   U   U  E          S    T";
  string c5=" CCC    OOO   N   N   QQ Q   UUU   EEEEE  SSSS     T";
  
  /* First clear the display. */
  cdclear();
  
  /* Display the logo. */
  lenc1 = strlen( c1 );
  col = (Context.maxcol-lenc1) / 2;
  lin = 2;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | CQC_A_BOLD, c1);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | CQC_A_BOLD, c2);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | CQC_A_BOLD, c3);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | CQC_A_BOLD, c4);
  lin++;
  cprintf( lin,col,ALIGN_NONE,"#%d#%s", RedColor | CQC_A_BOLD, c5);

  /* Draw a box around the logo. */
  lin++;
  uiPutColor(CQC_A_BOLD);
  cdbox( 1, col-2, lin, col+lenc1+1 );
  uiPutColor(0);
  
  lin++;

  return lin;
  }


/*  infoplanet - write out information about a planet */
/*  SYNOPSIS */
/*    char str() */
/*    int pnum, snum */
/*    cumInfoPlanet( str, pnum, snum ) */
void cumInfoPlanet( char *str, int pnum, int snum )
{
  int i, j; 
  int godlike, canscan; 
  char buf[MSGMAXLINE*2], junk[MSGMAXLINE];
  real x, y;
  
  /* Check range of the passed planet number. */
  if ( pnum <= 0 || pnum > NUMPLANETS )
    {
      cumPutMsg( "No such planet.", MSG_LIN1 );
      cdclrl( MSG_LIN2, 1 );
      cdmove( MSG_LIN1, 1 );
      cerror("infoplanet: Called with invalid pnum (%d).",
	     pnum );
      return;
    }
  
  /* GOD is too clever. */
  godlike = ( snum < 1 || snum > MAXSHIPS );
  
  /* In some cases, report hostilities. */
  junk[0] = EOS;
  if ( Planets[pnum].type == PLANET_CLASSM || Planets[pnum].type == PLANET_DEAD )
    if ( ! godlike )
      if ( Planets[pnum].scanned[Ships[snum].team] && clbSPWar( snum, pnum ) )
	appstr( " (hostile)", junk );
  
  /* Things that orbit things that orbit have phases. */
  switch ( clbPhoon( pnum ) )
    {
    case PHOON_FIRST:
      appstr( " (first quarter)", junk );
      break;
    case PHOON_FULL:
      appstr( " (full)", junk );
      break;
    case PHOON_LAST:
      appstr( " (last quarter)", junk );
      break;
    case PHOON_NEW:
      appstr( " (new)", junk );
      break;
    case PHOON_NO:
      /* Do no-thing. */;
      break;
    default:
      appstr( " (weird)", junk );
      break;
    }
  
  if ( godlike )
    {
      x = 0.0;
      y = 0.0;
    }
  else
    {
      x = Ships[snum].x;
      y = Ships[snum].y;
    }
  
  Context.lasttdist = round(dist( x, y, Planets[pnum].x, Planets[pnum].y));
  Context.lasttang = round(angle( x, y, Planets[pnum].x, Planets[pnum].y ));

  if (UserConf.DoETAStats)
    {
      static char tmpstr[64];
      
      if (Ships[snum].warp > 0.0)
	{
	  sprintf(tmpstr, ", ETA %s",
		  clbETAStr(Ships[snum].warp, 	
			 Context.lasttdist));
	}
      else
	tmpstr[0] = '\0';

      sprintf( buf, "%s%s, a %s%s, range %d, direction %d%s",
	       str,
	     Planets[pnum].name,
	     ConqInfo->ptname[Planets[pnum].type],
	     junk,
	     Context.lasttdist,
	     Context.lasttang,
	     tmpstr);


    }
  else
    sprintf( buf, "%s%s, a %s%s, range %d, direction %d",
	   str,
	   Planets[pnum].name,
	   ConqInfo->ptname[Planets[pnum].type],
	   junk,
	   Context.lasttdist,
	   Context.lasttang);
  
  /* save for the alt hud */
  strncpy(Context.lasttarg, Planets[pnum].name, 3);
  Context.lasttarg[3] = EOS;

  if ( godlike )
    canscan = TRUE;
  else
    canscan = Planets[pnum].scanned[Ships[snum].team];
  
  junk[0] = EOS;
  if ( Planets[pnum].type != PLANET_SUN && Planets[pnum].type != PLANET_MOON )
    {
      if ( ! canscan )
	c_strcpy( "with unknown occupational forces", junk );
      else
	{
	  i = Planets[pnum].armies;
	  if ( i == 0 )
	    {
	      j = Planets[pnum].uninhabtime;
	      if ( j > 0 )
		sprintf( junk, "uninhabitable for %d more minutes", j );
	      else
		c_strcpy( "with NO armies", junk );
	    }
	  else
	    {
	      sprintf( junk, "with %d %s arm", i, 
		       Teams[Planets[pnum].team].name );
	      if ( i == 1 )
		appstr( "y", junk );
	      else
		appstr( "ies", junk );
	    }
	}
      
      /* Now see if we can tell about coup time. */
      if ( godlike )
	canscan = FALSE;	/* GOD can use teaminfo instead */
      else
	canscan = ( pnum == Teams[Ships[snum].team].homeplanet &&
		   Teams[Ships[snum].team].coupinfo );
      if ( canscan )
	{
	  j = Teams[Ships[snum].team].couptime;
	  if ( j > 0 )
	    {
	      if ( junk[0] != EOS )
		appstr( ", ", junk );
	      appint( j, junk );
	      appstr( " minutes until coup time", junk );
	    }
	}
    }
  
  if ( junk[0] == EOS )
    {
      appchr( '.', buf );
    }
  else
    {
      appchr( ',', buf );
      appchr( '.', junk );
    }
  
  /* Now output the info. Break the stuff in buf across two lines */
  /*  (if necessary) and force the stuff in junk (the number of */
  /*  armies for planets) to be all on the second line. */
  i = strlen( buf );				/* strlen of first part */
  j = 69;					/* desired maximum length */
  if ( i <= j )
    {
      /* The first part is small enough. */
      cumPutMsg( buf, MSG_LIN1 );
      if ( junk[0] != EOS )
	cumPutMsg( junk, MSG_LIN2 );
      else
	cdclrl( MSG_LIN2, 1 );
    }
  else
    {
      /* Break it into two lines. */
      i = j + 1;
      while ( buf[i] != ' ' && i > 1 )
	i = i - 1;
      appchr( ' ', buf );
      appstr( junk, buf );
      buf[i] = EOS;				/* terminate at blank */
      cumPutMsg( buf, MSG_LIN1 );
      cumPutMsg( &buf[i+1], MSG_LIN2 );
    }
  
  cdmove( MSG_LIN1, 1 );
  return;
  
}

/*  infoship - write out information about a ship */
/*  SYNOPSIS */
/*    int snum, scanner */
/*    cumInfoShip( snum, scanner ) */
void cumInfoShip( int snum, int scanner )
{
  int i, status;
  char junk[MSGMAXLINE];
  real x, y, dis, kills, appx, appy;
  int godlike, canscan;
  static char tmpstr[BUFFER_SIZE];

#define BETTER_ETA		/* we'll try this out for a release */
#undef DEBUG_ETA		/* define for debugging */

#if defined(BETTER_ETA)
  real pwarp, diffdis, close_rate;
  time_t difftime, curtime;
  static time_t oldtime = 0;
  static real avgclose_rate, olddis = 0.0, oldclose_rate = 0.0;
  static int oldsnum = 0;
#endif /* BETTER_ETA */

  godlike = ( scanner < 1 || scanner > MAXSHIPS );
  
  cdclrl( MSG_LIN1, 2 );
  if ( snum < 1 || snum > MAXSHIPS )
    {
      cumPutMsg( "No such ship.", MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
      return;
    }
  status = Ships[snum].status;
  if ( ! godlike && status != SS_LIVE )
    {
      cumPutMsg( "Not found.", MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
      return;
    }

  cbuf[0] = Context.lasttarg[0] = EOS;
  appship( snum, cbuf );
  strcpy(Context.lasttarg, cbuf); /* save for alt hud */

  if ( snum == scanner )
    {
      /* Silly Captain... */
      appstr( ": That's us, silly!", cbuf );
      cumPutMsg( cbuf, MSG_LIN1 );
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
      x = Ships[scanner].x;
      y = Ships[scanner].y;
    }
  if ( SCLOAKED(snum) )
    {
      if (godlike)
	{
	  appx = rndnor(Ships[snum].x, CLOAK_SMEAR_DIST);
	  appy = rndnor(Ships[snum].y, CLOAK_SMEAR_DIST);
	}
      else			/* client */
	{			/* for clients, these have already been
				   smeared */
	  appx = Ships[snum].x;
	  appy = Ships[snum].y;
	}
    }
  else
    {
      appx = Ships[snum].x;
      appy = Ships[snum].y;
    }
  dis = dist( x, y, appx, appy );
  if ( godlike )
    canscan = TRUE;
  else
    {
      
      /* Decide if we can do an acurate scan. */
      canscan = ( (dis < ACCINFO_DIST && ! SCLOAKED(snum)) ||
		 ( (Ships[snum].scanned[ Ships[scanner].team] > 0) && ! selfwar(scanner) ) );
    }
  
  appstr( ": ", cbuf );
  if ( Ships[snum].alias[0] != EOS )
    {
      appstr( Ships[snum].alias, cbuf );
      appstr( ", ", cbuf );
    }
  kills = (Ships[snum].kills + Ships[snum].strkills);
  if ( kills == 0.0 )
    appstr( "no", cbuf );
  else
    {
      sprintf( junk, "%.1f", kills );
      appstr( junk, cbuf );
    }
  appstr( " kill", cbuf );
  if ( kills != 1.0 )
    appchr( 's', cbuf );
  if ( SCLOAKED(snum) && ( godlike || SSCANDIST(snum) ) )
    appstr( " (CLOAKED) ", cbuf );
  else
    appstr( ", ", cbuf );

  appstr("a ", cbuf);
  appstr(ShipTypes[Ships[snum].shiptype].name, cbuf);
  appstr(", ", cbuf);

  if ( godlike )
    {
      appsstatus( status, cbuf );
      appchr( '.', cbuf );
    }
  else 
    {
      if ( Ships[snum].war[Ships[scanner].team] )
	appstr( "at WAR.", cbuf );
      else
	appstr( "at peace.", cbuf );
    }
  
  cumPutMsg( cbuf, MSG_LIN1 );
  
  if ( ! SCLOAKED(snum) || Ships[snum].warp > 0.0 )
    {
      Context.lasttdist = round( dis ); /* save these puppies for alt hud */
      Context.lasttang = round( angle( x, y, appx, appy ) );
      sprintf( cbuf, "Range %d, direction %d",
	     Context.lasttdist, Context.lasttang );


#if defined(BETTER_ETA)
      if (UserConf.DoETAStats)
	{
	  if (Ships[scanner].warp > 0.0 || Ships[snum].warp > 0.0)
	    {
	      curtime = getnow(NULL, 0);

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
		  clog("infoship: close_rate(%.1f) = diffdis(%.1f) / difftime(%d), avgclose_rate = %.1f",
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
		      appstr(tmpstr, cbuf);
		    }
		  else
		    {		/* we are indeed closing... */

				/* calc psuedo-warp */
		      /* pwarp = dis / (avgclose_rate (in MM/sec) / 
			                MM_PER_SEC_PER_WARP(18)) */
		      pwarp = (avgclose_rate / (real) MM_PER_SEC_PER_WARP);

#ifdef DEBUG_ETA
clog("infoship:\tdis(%.1f) pwarp(%.1f) = (close_rate(%.1f) / MM_PER_SEC_PER_WARP(%.1f)", dis, pwarp, close_rate, MM_PER_SEC_PER_WARP);
#endif

		      sprintf(tmpstr, ", ETA %s",
			      clbETAStr(pwarp, dis));
		      appstr(tmpstr, cbuf);
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
		    (((Ships[scanner].warp > 0.0) ? 
		      Ships[scanner].warp : 
		      0.0) +
		     ((Ships[snum].warp > 0.0) ? 
		      Ships[snum].warp 
		      : 0.0));

		  sprintf(tmpstr, ", ETA %s",
			  clbETAStr(pwarp, dis));
		  appstr(tmpstr, cbuf);
		}
	    }
	} /* if do ETA stats */
#else /* not a BETTER_ETA */

      if (UserConf.DoETAStats)
	{
	  if (Ships[scanner].warp > 0.0 || Ships[snum].warp > 0.0)
	    {
				/* take other ships velocity into account */
	      cumwarp = 
		(((Ships[scanner].warp > 0.0) ? Ships[scanner].warp : 0.0) + 
		((Ships[snum].warp > 0.0) ? Ships[snum].warp : 0.0));

	      sprintf(tmpstr, ", ETA %s",
		      clbETAStr(cumwarp, dis));
	      appstr(tmpstr, cbuf);
	    }
	} /* if do ETA stats */
#endif /* !BETTER_ETA */

    }
  else				/* else cloaked and at w0 */
    {
      Context.lasttdist = Context.lasttang = 0;
      cbuf[0] = EOS;
    }
  
  if ( canscan )
    {
      if ( cbuf[0] != EOS )
	appstr( ", ", cbuf );
      appstr( "shields ", cbuf );
      if ( SSHUP(snum) && ! SREPAIR(snum) )
	appint( round( Ships[snum].shields ), cbuf );
      else
	appstr( "DOWN", cbuf );
      i = round( Ships[snum].damage );
      if ( i > 0 )
	{
	  if ( cbuf[0] != EOS )
	    appstr( ", ", cbuf );
	  sprintf( junk, "damage %d", i );
	  appstr( junk, cbuf );
	}
      i = Ships[snum].armies;
      if ( i > 0 )
	{
	  sprintf( junk, ", with %d arm", i );
	  appstr( junk, cbuf );
	  if ( i == 1 )
	    {
	      appchr( 'y', cbuf );
	    }
	  else
	    appstr( "ies", cbuf );
	}
    }
  if ( cbuf[0] != EOS )
    {
      cbuf[0] = (char)toupper( cbuf[0] );
      appchr( '.', cbuf );
      cumPutMsg( cbuf, MSG_LIN2 );
    }
  
  cdmove( MSG_LIN1, 1 );
  return;
  
}

/*  planlist - list planets */
/*  SYNOPSIS */
/*    int team */
/*    cumPlanetList( team ) */
void cumPlanetList( int team, int snum )
{
  int i, lin, col, olin, pnum;
  static int sv[NUMPLANETS + 1];
  int cmd;
  char ch, junk[10], coreflag;
  char *hd0="P L A N E T   L I S T   ";
  char *hd1="' = must take to conquer the Universe)";
  string hd2="planet      type team armies          planet      type team armies";
  char hd3[BUFFER_SIZE];
  int outattr;
  int col2;
  int column_h = 7;
  int column_1 = 5;
  int column_2 = 43;
  char xbuf[BUFFER_SIZE];
  static char pd0[MID_BUFFER_SIZE];
  static int FirstTime = TRUE;
  int PlanetOffset;		/* offset into NUMPLANETS for this page */
  int PlanetIdx = 0;
  int Done;

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;

				/* build header fmt string */
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
      for ( i = 1; i <= NUMPLANETS; i++ )
	sv[i] = i;
      clbSortPlanets( sv );
      
    }

  strcpy( hd3, hd2 );
  for ( i = 0; hd3[i] != EOS; i++ )
    if ( hd3[i] != ' ' )
      hd3[i] = '-';
  
  PlanetIdx = 0;

  PlanetOffset = 1;
  cdclear();
  Done = FALSE;
  do
    {

      cdclra(0, 0, MSG_LIN1 + 2, Context.maxcol - 1);
      PlanetIdx = 0;
      lin = 1;
      col = column_h;
      
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
      
      if (PlanetOffset <= NUMPLANETS)
	{
	  while ((PlanetOffset + PlanetIdx) <= NUMPLANETS)
	    {
	      i = PlanetOffset + PlanetIdx;
	      PlanetIdx++;
	      pnum = sv[i];
	      
	      /* colorize - dwp */    
	      if ( snum > 0 && snum <= MAXSHIPS)
		{	/* if user has a valid ship */
		  if ( Planets[pnum].team == Ships[snum].team && !selfwar(snum) )
		    outattr = GreenLevelColor;
		  else if ( (clbSPWar(snum,pnum) && Planets[pnum].scanned[Ships[snum].team] ) ||
			    Planets[pnum].type == PLANET_SUN )
		    outattr = RedLevelColor;
		  else 
		    outattr = YellowLevelColor;
		}
	      else
		{			/* else, user doesn't have a ship yet */
		  if (team == TEAM_NOTEAM)
		    {			/* via conqoper */
		      switch(Planets[pnum].type)
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
		      if ( Planets[pnum].team == Users[Context.unum].team && 
			   !(Users[Context.unum].war[Users[Context.unum].team]))
			{
			  outattr = GreenLevelColor;
			}
		      else if ( Planets[pnum].type == PLANET_SUN ||
				(Planets[pnum].team < NUMPLAYERTEAMS && 
				 Users[Context.unum].war[Planets[pnum].team] &&
				 Planets[pnum].scanned[Users[Context.unum].team]) )
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
	      if ( ! Planets[pnum].real )
		continue; 
	      
	      /* I want everything if it's real */
	      
	      /* Figure out who owns it and count armies. */
	      ch =  Teams[Planets[pnum].team].teamchar;
	      sprintf( junk, "%d", Planets[pnum].armies );
	      
	      /* Then modify based on scan information. */
	      
	      if ( team != TEAM_NOTEAM )
		if ( ! Planets[pnum].scanned[team] )
		  {
		    ch = '?';
		    c_strcpy( "?", junk );
		  }
	      
	      /* Suns and moons are displayed as unowned. */
	      if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON )
		ch = ' ';
	      
	      /* Don't display armies for suns unless we're special. */
	      if ( Planets[pnum].type == PLANET_SUN )
		if ( team != TEAM_NOTEAM )
		  junk[0] = EOS;
	      
	      /* Moons aren't supposed to have armies. */
	      if ( Planets[pnum].type == PLANET_MOON )
		{
		  if ( team != TEAM_NOTEAM )
		    junk[0] = EOS;
		  else if ( Planets[pnum].armies == 0 )
		    junk[0] = EOS;
		}
	      
	      coreflag = ' ';
	      
	      /* flag planets that are required for a conq */
	      if (Planets[pnum].type == PLANET_CLASSM || Planets[pnum].type == PLANET_DEAD)
		{
		  if (pnum > NUMCONPLANETS)
		    coreflag = ' ';
		  else
		    coreflag = '+';
		}
	      
	      sprintf(xbuf,"%c ",coreflag);  /* coreflag */
	      uiPutColor(SpecialColor);
	      cdputs( xbuf, lin, col );
	      
	      col+=(strlen(xbuf));
	      sprintf(xbuf,"%-11s ",Planets[pnum].name);  /* Planets[pnum].name */
	      uiPutColor(outattr);
	      cdputs( xbuf, lin, col );
	      
	      col+=(strlen(xbuf));
	      sprintf( xbuf, "%-4c %-3c  ", 
		       ConqInfo->chrplanets[Planets[pnum].type], ch);
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

	  if ((PlanetOffset + PlanetIdx) > NUMPLANETS)
	    cumPutPrompt( MTXT_DONE, MSG_LIN2 );
	  else
	    cumPutPrompt( MTXT_MORE, MSG_LIN2 );

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
		  if (PlanetOffset > NUMPLANETS)
		    {		/* pointless to continue */
		      Done = TRUE;
		    }

		}
	    }

				/* didn't get a char, update */
	  if (snum > 0 && snum <= MAXSHIPS)
	    if (!clbStillAlive(snum))
	      Done = TRUE;

	} /* if PlanetOffset <= NUMPLANETS */
      else
	Done = TRUE;		/* else PlanetOffset > NUMPLANETS */
      
    } while(Done != TRUE); /* do */
  
  return;
  
}


/*  playlist - list ships */
/*  SYNOPSIS */
/*    int godlike, doall */
/*    cumPlayList( godlike, doall ) */
void cumPlayList( int godlike, int doall, int snum )
{
  int i, unum, status, kb, lin, col;
  int fline, lline, fship;
  char sbuf[20];
  char kbuf[20];
  char pidbuf[20];
  char ubuf[MAXUSERNAME + 2];
  int ch;
  char *hd1="ship  name          pseudonym              kills      pid";
  char *hd2="ship  name          pseudonym              kills     type";
  
  /* Do some screen setup. */
  cdclear();
  uiPutColor(LabelColor);  /* dwp */

  if (godlike)
    c_strcpy( hd1, cbuf );
  else
    c_strcpy( hd2, cbuf );

  col = (int)(Context.maxcol - strlen( cbuf )) / (int)2;
  lin = 2;
  cdputs( cbuf, lin, col );
  
  for ( i = 0; cbuf[i] != EOS; i = i + 1 )
    if ( cbuf[i] != ' ' )
      cbuf[i] = '-';
  lin = lin + 1;
  cdputs( cbuf, lin, col );
  uiPutColor(0);          /* dwp */
  
  fline = lin + 1;				/* first line to use */
  lline = MSG_LIN1;				/* last line to use */
  fship = 1;					/* first user in uvec */
  
  while(TRUE) /* repeat- while */
    {
      if ( ! godlike )
	if ( ! clbStillAlive( Context.snum ) )
	  break;
      i = fship;
      cdclrl( fline, lline - fline + 1 );
      lin = fline;
      while ( i <= MAXSHIPS && lin <= lline )
	{
	  status = Ships[i].status;

	  kb = Ships[i].killedby;
	  if ( status == SS_LIVE ||
	      ( doall && ( status != SS_OFF || kb != 0 ) ) )
	    {
	      sbuf[0] = EOS;
	      appship( i, sbuf );
	      appstr(" ", sbuf);
	      appchr(ShipTypes[Ships[i].shiptype].name[0], sbuf);

	      unum = Ships[i].unum;
	      if ( unum >= 0 && unum < MAXUSERS )
		{
		  if (SROBOT(i)) /* robot */
		    strcpy(pidbuf, " ROBOT");
		  else if (SVACANT(i)) 
		    strcpy(pidbuf, "VACANT");
		  else
		    {
		      if (godlike)
			sprintf(pidbuf, "%6d", Ships[i].pid);
		      else
			strcpy(pidbuf, "  LIVE");
		    }
		
		  strcpy(ubuf, Users[unum].username);

		  sprintf(kbuf, "%6.1f", (Ships[i].kills + Ships[i].strkills));
		  sprintf( cbuf, "%-5s %-13.13s %-21.21s %-8s %6s",
			   sbuf, ubuf, Ships[i].alias, 
			   kbuf, pidbuf );
		}
	      else
		sprintf( cbuf, "%-5s %13s %21s %8s %6s", sbuf,
		       " ", " ", " ", " " );
	      if ( doall && kb != 0 )
		{
		  appstr( "  ", cbuf);
		  appkb( kb, cbuf );
		}

		if (snum > 0 && snum <= MAXSHIPS )
		  {		/* a normal ship view */
		    if ( i == snum )    /* it's ours */
		      uiPutColor(CQC_A_BOLD);
		    else if (satwar(i, snum)) /* we're at war with it */
		      uiPutColor(RedLevelColor);
		    else if (Ships[i].team == Ships[snum].team && !selfwar(snum))
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
		    if (Users[Context.unum].war[Ships[i].team])  /* we're at war with ships's
						   team */
		      uiPutColor(RedLevelColor);
		    else if (Users[Context.unum].team == Ships[i].team)
		      uiPutColor(GreenLevelColor); /* it's a team ship */
		    else
		      uiPutColor(YellowLevelColor);
		  }

	      cdputs( cbuf, lin, col );
	      uiPutColor(0);
	      if ( doall && status != SS_LIVE )
		{
		  cbuf[0] = EOS;
		  appsstatus( status, cbuf );
		  
		  uiPutColor(YellowLevelColor);  
		  cdputs( cbuf, lin, col - 2 - strlen( cbuf ) );
		  uiPutColor(0); 
		}
	    }
	  i = i + 1;
	  lin = lin + 1;
	}
      if ( i > MAXSHIPS )
	{
	  /* We're displaying the last page. */
	  cumPutPrompt( MTXT_DONE, MSG_LIN2 );
	  cdrefresh();
	  if ( iogtimed( &ch, 1.0 ) )
	    {
	      if ( ch == TERM_EXTRA )
		fship = 1;			/* move to first page */
	      else
		break;
	    }
	}
      else
	{
	  /* There are ships left to display. */
	  cumPutPrompt( MTXT_MORE, MSG_LIN2 );
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
/*    flag = cumReviewMsgs( snum, slm ) */
int cumReviewMsgs( int snum, int slm )
{
  int ch, Done, i, msg, tmsg, lastone; 
  int didany;
  
  didany = FALSE;
  Done = FALSE;

  lastone = modp1( ConqInfo->lastmsg+1, MAXMESSAGES );
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      if ( Ships[snum].lastmsg == LMSG_NEEDINIT )
	return ( FALSE );				/* none to read */
      i = Ships[snum].alastmsg;
      if ( i != LMSG_READALL )
	lastone = i;
    }
  
  cdclrl( MSG_LIN1, 1 );
  
  /*  for ( msg = slm; msg != lastone; msg = modp1( msg-1, MAXMESSAGES ) )*/

  msg = slm;

  do
    {
      if ( clbCanRead( snum, msg ))
	{
	  cumReadMsg( snum, msg, MSG_LIN1 );
	  didany = TRUE;
	  cumPutPrompt( "--- [SPACE] for more, arrows to scroll, any key to quit ---", 
		  MSG_LIN2 );
	  cdrefresh();
	  ch = iogchar();
	  switch(ch)
	    {
	    case ' ':
	    case '<':
	    case KEY_UP:
	    case KEY_LEFT:
	      tmsg = modp1( msg - 1, MAXMESSAGES );
	      while(!clbCanRead( snum, tmsg ) && tmsg != lastone)
		{
		  tmsg = modp1( tmsg - 1, MAXMESSAGES );
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
	      tmsg =  modp1( msg + 1, MAXMESSAGES );
	      while(!clbCanRead( snum, tmsg ) && tmsg != slm + 1 )
		{
		  tmsg = modp1( tmsg + 1, MAXMESSAGES );
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
	  msg = modp1( msg - 1, MAXMESSAGES );
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
/*    cumTeamList( team ) */
void cumTeamList( int team )
{
  int i, j, lin, col, ctime, etime;
  int godlike;
  char buf[MSGMAXLINE], timbuf[5][DATESIZE];
  real x[5];
  string sfmt="%15s %11s %11s %11s %11s %11s";
  char *stats="Statistics since: ";
  char *last_conquered="Universe last conquered at: ";

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
  col = 0; /*1*/
  
  lin = 1;
  /* team stats and last date conquered */
  sprintf(tmpfmt,"#%d#%%s#%d#%%s",LabelColor,InfoColor);
  cprintf(lin,0,ALIGN_CENTER, tmpfmt, stats, ConqInfo->inittime);
  lin++;

  /* last conquered */
  cprintf(lin, 0, ALIGN_CENTER, tmpfmt, last_conquered, 
	  ConqInfo->conqtime);
  lin++;

  /* last conqueror and conqteam */
  sprintf(tmpfmt,"#%d#by #%d#%%s #%d#for the #%d#%%s #%d#team",
		LabelColor,(int)CQC_A_BOLD,LabelColor,(int)CQC_A_BOLD,LabelColor);
  cprintf(lin,0,ALIGN_CENTER, tmpfmt, ConqInfo->conqueror, 
	  ConqInfo->conqteam);
  
  col=0;  /* put col back to 0 for rest of display */
  lin = lin + 1;
  cdclrl( lin, 1 );
  if ( ConqInfo->lastwords[0] != EOS )
    {
      sprintf(tmpfmt, "#%d#%%c%%s%%c", YellowLevelColor);
      cprintf(lin, 0, ALIGN_CENTER, tmpfmt, '"', ConqInfo->lastwords, '"' );
    }
  
  lin+=2;
  sprintf( buf, sfmt, " ",
	 Teams[0].name, Teams[1].name, Teams[2].name, Teams[3].name, "Totals" );
  cprintf(lin,col,0, sfmt2, " ",
	 Teams[0].name, Teams[1].name, Teams[2].name, Teams[3].name, "Totals" );
  
  lin++;
  for ( i = 0; buf[i] != EOS; i++ )
    if ( buf[i] != ' ' )
      buf[i] = '-';
  uiPutColor(LabelColor);
  cdputs( buf, lin, col );
  uiPutColor(0);
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Conquers",
	 Teams[0].stats[TSTAT_CONQUERS], Teams[1].stats[TSTAT_CONQUERS],
	 Teams[2].stats[TSTAT_CONQUERS], Teams[3].stats[TSTAT_CONQUERS],
	 Teams[0].stats[TSTAT_CONQUERS] + Teams[1].stats[TSTAT_CONQUERS] +
	 Teams[2].stats[TSTAT_CONQUERS] + Teams[3].stats[TSTAT_CONQUERS] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Wins",
	 Teams[0].stats[TSTAT_WINS], Teams[1].stats[TSTAT_WINS],
	 Teams[2].stats[TSTAT_WINS], Teams[3].stats[TSTAT_WINS],
	 Teams[0].stats[TSTAT_WINS] + Teams[1].stats[TSTAT_WINS] +
	 Teams[2].stats[TSTAT_WINS] + Teams[3].stats[TSTAT_WINS] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Losses",
	 Teams[0].stats[TSTAT_LOSSES], Teams[1].stats[TSTAT_LOSSES],
	 Teams[2].stats[TSTAT_LOSSES], Teams[3].stats[TSTAT_LOSSES],
	 Teams[0].stats[TSTAT_LOSSES] + Teams[1].stats[TSTAT_LOSSES] +
	 Teams[2].stats[TSTAT_LOSSES] + Teams[3].stats[TSTAT_LOSSES] );
  
  lin++;
  cprintf(lin,col,0, dfmt2, "Ships",
	 Teams[0].stats[TSTAT_ENTRIES], Teams[1].stats[TSTAT_ENTRIES],
	 Teams[2].stats[TSTAT_ENTRIES], Teams[3].stats[TSTAT_ENTRIES],
	 Teams[0].stats[TSTAT_ENTRIES] + Teams[1].stats[TSTAT_ENTRIES] +
	 Teams[2].stats[TSTAT_ENTRIES] + Teams[3].stats[TSTAT_ENTRIES] );
  
  lin++;
  etime = Teams[0].stats[TSTAT_SECONDS] + Teams[1].stats[TSTAT_SECONDS] +
    Teams[2].stats[TSTAT_SECONDS] + Teams[3].stats[TSTAT_SECONDS];
  fmtseconds( Teams[0].stats[TSTAT_SECONDS], timbuf[0] );
  fmtseconds( Teams[1].stats[TSTAT_SECONDS], timbuf[1] );
  fmtseconds( Teams[2].stats[TSTAT_SECONDS], timbuf[2] );
  fmtseconds( Teams[3].stats[TSTAT_SECONDS], timbuf[3] );
  fmtseconds( etime, timbuf[4] );
  cprintf(lin,col,0, sfmt3, "Time",
	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  
  lin++;
  ctime = Teams[0].stats[TSTAT_CPUSECONDS] + Teams[1].stats[TSTAT_CPUSECONDS] +
    Teams[2].stats[TSTAT_CPUSECONDS] + Teams[3].stats[TSTAT_CPUSECONDS];
  fmtseconds( Teams[0].stats[TSTAT_CPUSECONDS], timbuf[0] );
  fmtseconds( Teams[1].stats[TSTAT_CPUSECONDS], timbuf[1] );
  fmtseconds( Teams[2].stats[TSTAT_CPUSECONDS], timbuf[2] );
  fmtseconds( Teams[3].stats[TSTAT_CPUSECONDS], timbuf[3] );
  fmtseconds( ctime, timbuf[4] );
  cprintf( lin,col,0, sfmt3, "Cpu time",
	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  
  lin++;
  for ( i = 0; i < 4; i++ )
    {
      j = Teams[i].stats[TSTAT_SECONDS];
      if ( j <= 0 )
	x[i] = 0.0;
      else
	x[i] = 100.0 * ((real) Teams[i].stats[TSTAT_CPUSECONDS] / (real) j);
    }
  if ( etime <= 0 )
    x[4] = 0.0;
  else
    x[4] = 100.0 * (real) ctime / (real)etime;
  cprintf( lin,col,0, pfmt2, "Cpu usage", x[0], x[1], x[2], x[3], x[4] );

  lin++;
  cprintf( lin,col,0, dfmt2, "Phaser shots",
	 Teams[0].stats[TSTAT_PHASERS], Teams[1].stats[TSTAT_PHASERS],
	 Teams[2].stats[TSTAT_PHASERS], Teams[3].stats[TSTAT_PHASERS],
	 Teams[0].stats[TSTAT_PHASERS] + Teams[1].stats[TSTAT_PHASERS] +
	 Teams[2].stats[TSTAT_PHASERS] + Teams[3].stats[TSTAT_PHASERS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Torps fired",
	 Teams[0].stats[TSTAT_TORPS], Teams[1].stats[TSTAT_TORPS],
	 Teams[2].stats[TSTAT_TORPS], Teams[3].stats[TSTAT_TORPS],
	 Teams[0].stats[TSTAT_TORPS] + Teams[1].stats[TSTAT_TORPS] +
	 Teams[2].stats[TSTAT_TORPS] + Teams[3].stats[TSTAT_TORPS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Armies bombed",
	 Teams[0].stats[TSTAT_ARMBOMB], Teams[1].stats[TSTAT_ARMBOMB],
	 Teams[2].stats[TSTAT_ARMBOMB], Teams[3].stats[TSTAT_ARMBOMB],
	 Teams[0].stats[TSTAT_ARMBOMB] + Teams[1].stats[TSTAT_ARMBOMB] +
	 Teams[2].stats[TSTAT_ARMBOMB] + Teams[3].stats[TSTAT_ARMBOMB] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Armies captured",
	 Teams[0].stats[TSTAT_ARMSHIP], Teams[1].stats[TSTAT_ARMSHIP],
	 Teams[2].stats[TSTAT_ARMSHIP], Teams[3].stats[TSTAT_ARMSHIP],
	 Teams[0].stats[TSTAT_ARMSHIP] + Teams[1].stats[TSTAT_ARMSHIP] +
	 Teams[2].stats[TSTAT_ARMSHIP] + Teams[3].stats[TSTAT_ARMSHIP] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Planets taken",
	 Teams[0].stats[TSTAT_CONQPLANETS], Teams[1].stats[TSTAT_CONQPLANETS],
	 Teams[2].stats[TSTAT_CONQPLANETS], Teams[3].stats[TSTAT_CONQPLANETS],
	 Teams[0].stats[TSTAT_CONQPLANETS] + Teams[1].stats[TSTAT_CONQPLANETS] +
	 Teams[2].stats[TSTAT_CONQPLANETS] + Teams[3].stats[TSTAT_CONQPLANETS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Coups",
	 Teams[0].stats[TSTAT_COUPS], Teams[1].stats[TSTAT_COUPS],
	 Teams[2].stats[TSTAT_COUPS], Teams[3].stats[TSTAT_COUPS],
	 Teams[0].stats[TSTAT_COUPS] + Teams[1].stats[TSTAT_COUPS] +
	 Teams[2].stats[TSTAT_COUPS] + Teams[3].stats[TSTAT_COUPS] );
  
  lin++;
  cprintf( lin,col,0, dfmt2, "Genocides",
	 Teams[0].stats[TSTAT_GENOCIDE], Teams[1].stats[TSTAT_GENOCIDE],
	 Teams[2].stats[TSTAT_GENOCIDE], Teams[3].stats[TSTAT_GENOCIDE],
	 Teams[0].stats[TSTAT_GENOCIDE] + Teams[1].stats[TSTAT_GENOCIDE] +
	 Teams[2].stats[TSTAT_GENOCIDE] + Teams[3].stats[TSTAT_GENOCIDE] );
  
  for ( i = 0; i < 4; i++ )
    if ( Teams[i].couptime == 0 )
      timbuf[i][0] = EOS;
    else
      sprintf( timbuf[i], "%d", Teams[i].couptime );
  
  if ( ! godlike )
    {
      for ( i = 0; i < 4; i++ )
	if ( team != i )
	  c_strcpy( "-", timbuf[i] );
	else if ( ! Teams[i].coupinfo && timbuf[i][0] != EOS )
	  c_strcpy( "?", timbuf[i] );
    }
  
  timbuf[4][0] = EOS;
  
  lin++;
  cprintf( lin,col,0, sfmt3, "Coup time",
 	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );

  uiPutColor(0);
  
  return;
  
}


/*  userlist - display the user list */
/*  SYNOPSIS */
/*    cumUserList( godlike ) */
void cumUserList( int godlike, int snum )
{
  int i, j, unum, nu, fuser, fline, lline, lin;
  static int uvec[MAXUSERS];
  int ch;
  char *hd1="U S E R   L I S T";

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
  
  for ( j = 0; cbuf[j] != EOS; j = j + 1 )
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
	if ( Users[unum].live)
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
	  if ( snum > 0 && snum <= MAXSHIPS ) /* we're a valid ship */
	    {
		if ( strcmp(Users[uvec[i]].username,
			    Users[Ships[snum].unum].username) == 0 &&
		     Users[uvec[i]].type == Users[Ships[snum].unum].type)
		  uiPutColor(CQC_A_BOLD);    /* it's ours */
		else if (Ships[snum].war[Users[uvec[i]].team]) /* we're at war with it */
		  uiPutColor(RedLevelColor);
		else if (Ships[snum].team == Users[uvec[i]].team && !selfwar(snum))
		  uiPutColor(GreenLevelColor); /* it's a team ship */
		else
		  uiPutColor(YellowLevelColor);
	    }
	  else if (godlike)/* we are running conqoper */
	    uiPutColor(YellowLevelColor); /* bland view */
	  else			/* we don't have a ship yet */
	    {
	      if ( strcmp(Users[uvec[i]].username,
			  Users[Context.unum].username) == 0 &&
		   Users[uvec[i]].type == Users[Context.unum].type)
		uiPutColor(CQC_A_BOLD);    /* it's ours */
	      else if (Users[Context.unum].war[Users[uvec[i]].team]) /* we're war with them */
		uiPutColor(RedLevelColor);	            /* (might be selfwar) */
	      else if (Users[Context.unum].team == Users[uvec[i]].team) /* team ship */
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
	  cumPutPrompt( MTXT_DONE, MSG_LIN2 );
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
	  cumPutPrompt( MTXT_MORE, MSG_LIN2 );
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
/*    cumUserStats( godlike, snum ) */
void cumUserStats( int godlike , int snum )
{
  int i, j, unum, nu, fuser, fline, lline, lin;
  static int uvec[MAXUSERS];
  int ch;
  char *hd1="M O R E   U S E R   S T A T S";
  char *hd2="name         cpu  conq coup geno  taken bombed/shot  shots  fired   last entry";
  char *hd3="planets  armies    phaser  torps";

  for (i=0; i<MAXUSERS; i++)
    uvec[i] = i;
  
  /* Do some screen setup. */
  cdclear();
  lin = 1;
  uiPutColor(LabelColor);  /* dwp */
  cdputc( hd1, lin );
  
  lin = lin + 2;
  cdputs( hd3, lin, 34 );
  
  c_strcpy( hd2, cbuf );
  lin = lin + 1;
  cdputs( cbuf, lin, 1 );
  
  for ( j = 0; cbuf[j] != EOS; j = j + 1 )
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
	if ( Users[unum].live)
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
	  if ( snum > 0 && snum <= MAXSHIPS ) /* we're a valid ship */
	  {
	    if ( strcmp(Users[uvec[i]].username, 
			Users[Ships[snum].unum].username) == 0 &&
		 Users[uvec[i]].type == Users[Ships[snum].unum].type )
	      uiPutColor(CQC_A_BOLD);	        /* it's ours */
	    else if (Ships[snum].war[Users[uvec[i]].team]) 
	      uiPutColor(RedLevelColor);   /* we're at war with it */
	    else if (Ships[snum].team == Users[uvec[i]].team && !selfwar(snum))
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
	      if ( strcmp(Users[uvec[i]].username,
			  Users[Context.unum].username) == 0  &&
		   Users[uvec[i]].type == Users[Context.unum].type )
		uiPutColor(CQC_A_BOLD);	/* it's ours */
	      else if (Users[Context.unum].war[Users[uvec[i]].team]) 
		uiPutColor(RedLevelColor);  /* we're war with them (poss selfwar) */
	      else if (Users[Context.unum].team == Users[uvec[i]].team) 
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
	  cumPutPrompt( MTXT_DONE, MSG_LIN2 );
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
	  cumPutPrompt( MTXT_MORE, MSG_LIN2 );
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
/*    ok = cumConfirm() */
int cumConfirm(void)
{
  static char *cprompt = "Are you sure? ";
  int scol = ((Context.maxcol - strlen(cprompt)) / 2);

  if (cumAskYN("Are you sure? ", MSG_LIN2, scol))
    return(TRUE);
  else
    return (FALSE);
  
}

/*  askyn - ask the user a yes/no question - return TRUE if yes */
int cumAskYN(char *question, int lin, int col)
{
  char ch, buf[MSGMAXLINE];
  
  cdclrl( MSG_LIN2, 1 );
  uiPutColor(InfoColor);
  buf[0] = EOS;
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
/*    tch = cumGetCX( pmt, lin, offset, terms, buf, len ) */
char cumGetCX( char *pmt, int lin, int offset, char *terms, char *buf, int len )
{
  int i;
  
  i = (int)( Context.maxcol - strlen( pmt ) ) / (int)2 + offset;
  if ( i <= 0 )
    i = 1;
  move(lin, 0);
  clrtoeol();
  buf[0] = EOS;
  return ( cdgetx( pmt, lin, i, terms, buf, len, TRUE ) );
  
}



/*  gettarget - get a target angle from the user */
/*  SYNOPSIS */
/*    char pmt() */
/*    int lin, col */
/*    real dir */
/*    int flag, gettarget */
/*    flag = cumGetTarget( pmt, lin, col, dir ) */
int cumGetTarget( char *pmt, int lin, int col, real *dir, real cdefault )
{
  int i, j; 
  char ch, buf[MSGMAXLINE];
  
  cdclrl( lin, 1 );
  buf[0] = EOS;
  ch = (char)cdgetx( pmt, lin, col, TERMS, buf, MSGMAXLINE, TRUE );
  if ( ch == TERM_ABORT )
    return ( FALSE );
  
  delblanks( buf );
  fold( buf );
  if ( buf[0] == EOS )
    {
      /* Default. */
      *dir = cdefault;
      return ( TRUE );
    }
  if ( alldig( buf ) == TRUE )
    {
      i = 0;
      if ( ! safectoi( &j, buf, i ) )
	return ( FALSE );
      *dir = mod360( (real) j );
      return ( TRUE );
    }
  if ( arrows( buf, dir ) )
    return ( TRUE );
  
  return ( FALSE );
  
}

/*  more - wait for the user to type a space */
/*  SYNOPSIS */
/*    char pmt() */
/*    int spacetyped, more */
/*    spacetyped = cumMore( pmt ) */
int cumMore( char *pmt )
{
  int ch = 0; 
  string pml=MTXT_MORE;
  
  if ( pmt[0] != EOS )
    cumPutPrompt( pmt, MSG_LIN2 );
  else
    cumPutPrompt( pml, MSG_LIN2 );
  
  cdrefresh();
  ch = iogchar();
  return ( ch == ' ' );
  
}


/*  pagefile - page through a file */
/*  SYNOPSIS */
/*    char file(), errmsg() */
/*    int ignorecontroll, eatblanklines */
/*    cumPageFile( file, errmsg, ignorecontroll, eatblanklines ) */
void cumPageFile( char *file, char *errmsg )
{
  
  int plins = 1;
  FILE *pfd;
  string sdone="--- press any key to return ---";
  char buffer[BUFFER_SIZE];
  int buflen;
  
  if ((pfd = fopen(file, "r")) == NULL)
    {
      clog("cumPageFile(): fopen(%s) failed: %s",
	   file,
	   strerror(errno));
      
      cdclear();
      cdredo();
      cdputc( errmsg, MSG_LIN2/2 );
      cumMore( sdone );
      
      return;
    }
  
  cdclear();
  cdrefresh();
  cdmove(0, 0);
  
  plins = 0;
  
  while (fgets(buffer, BUFFER_SIZE - 1, pfd) != NULL)
    { 
      /* get one at a time */
      buflen = strlen(buffer);
      
      buffer[buflen - 1] = EOS; /* remove trailing LF */
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
	  if (!cumMore(MTXT_MORE))
	    break;		/* bail if space not hit */
	  
	  cdclear();
	  plins = 1;
	}
    }
  
  fclose(pfd);
  
  cumMore(sdone);
  
  return;
  
}


/*  putmsg - display a message on the bottom of the user's screen */
/*  SYNOPSIS */
/*    char msg() */
/*    int line */
/*    cumPutMsg( msg, line ) */
void cumPutMsg( char *msg, int line )
{
  cdclrl( line, 1 );
  cdputs( msg, line, 1 );

  return;
  
}


/*  putpmt - display a prompt */
/*  SYNOPSIS */
/*    char pmt() */
/*    int line */
/*    cumPutPrompt( pmt, line ) */
void cumPutPrompt( char *pmt, int line )
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
void cumHelpLesson(void)
{
  
  char buf[MSGMAXLINE];
  char helpfile[BUFFER_SIZE];
  
  sprintf(helpfile, "%s/%s", CONQSHARE, C_CONQ_HELPFILE);
  sprintf( buf, "%s: Can't open.", helpfile );
  cumPageFile( helpfile, buf);
  
  return;
  
}

/*  news - list current happenings */
/*  SYNOPSIS */
/*    news */
void cumNews(void)
{
  char newsfile[BUFFER_SIZE];
  
  sprintf(newsfile, "%s/%s", CONQSHARE, C_CONQ_NEWSFILE);
  
  cumPageFile( newsfile, "No news is good news.");
  
  return;
  
}

