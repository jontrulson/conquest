/* 
 * cockpit node
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "context.h"
#include "global.h"
#include "datatypes.h"
#include "color.h"
#include "conf.h"
#include "conqcom.h"
#include "conqlb.h"
#include "gldisplay.h"
#include "node.h"
#include "client.h"
#include "clientlb.h"
#include "record.h"
#include "ibuf.h"
#include "prm.h"
#include "cqkeys.h"

#include "nDead.h"
#include "nCPHelp.h"
#include "nShipl.h"
#include "nPlanetl.h"
#include "nUserl.h"
#include "nHistl.h"
#include "nTeaml.h"
#include "nOptions.h"

#include <assert.h>

#include "glmisc.h"
#include "glfont.h"
#include "render.h"

#include "nCP.h"

#define S_NONE         0
#define S_COURSE       1        /* setting course */
#define S_DOINFO       2
#define S_TARGET       3        /* targeting info for torp/phase */
#define S_CLOAK        4
#define S_ALLOC        5
#define S_DISTRESS     6
#define S_REFIT        7
#define S_REFITING     8
#define S_COUP         9
#define S_TOW          10
#define S_MSGTO        11       /* getting msg target */
#define S_MSG          12       /* getting msg  */
#define S_DOAUTOPILOT  13
#define S_AUTOPILOT    14
#define S_REVIEW       15       /* review messages */
#define S_DEAD         16       /* the abyss */
#define S_DESTRUCT     17
#define S_DESTRUCTING  18
#define S_BOMB         19
#define S_BOMBING      20
#define S_BEAM         21
#define S_BEAMDIR      22
#define S_BEAMNUM      23
#define S_BEAMING      24
#define S_WAR          25
#define S_WARRING      26
#define S_PSEUDO       27       /* name change */
static int state;

#define T_PHASER       0        /* S_TARGET */
#define T_BURST        1
#define T_TORP         2
static int desttarg;

/* from conquestgl */
extern Unsgn8 clientFlags; 
extern int lastServerError;

/* timer vars */
static int entertime;

/* glut timer vars */
static int rftime;              /* last recording frame */

/* refit vars */
static int refitst = 0;         /* currently selected shiptype */

/* msg vars */
static int msgto = 0;

/* review msg vars */
static int lstmsg;              /* saved last msg */
static int lastone, msg;        /* saved for scrolling */

/* beaming vars */
static int dirup, upmax, downmax, beamax;

/* war vars */
static int twar[NUMPLAYERTEAMS];

/* the current prompt */
static prm_t prm;
static int prompting = FALSE;

/* save lastblast and always use a local copy for both torp and phasering */
static real lastblast;
static real lastphase;

/* misc buffers */
static char cbuf[MID_BUFFER_SIZE];
static char pbuf[MID_BUFFER_SIZE];

static char *abt = "...aborted...";

static int dostats = FALSE;     /* whether to display rendering stats */

extern dspData_t dData;

/* Ping status */
static Unsgn32 pingStart = 0;
static int pingPending = FALSE;

#define cp_putmsg(str, lin)  setPrompt(lin, NULL, NoColor, str, NoColor)

static int nCPDisplay(dspConfig_t *);
static int nCPIdle(void);
static int nCPInput(int ch);

static scrNode_t nCPNode = {
  nCPDisplay,               /* display */
  nCPIdle,                  /* idle */
  nCPInput,                  /* input */
  NULL
};

/* convert a KP key into an angle */
static int _KPAngle(int ch, real *angle)
{
  int rv;

  switch (ch)
    {
    case CQ_KEY_HOME:                /* KP upper left */
      *angle = 135.0;
      rv = TRUE;
      break;
    case CQ_KEY_PAGE_UP:                /* KP upper right */
      *angle = 45.0;
      rv = TRUE;
      break;
    case CQ_KEY_END:                /* KP lower left */
      *angle = 225.0;
      rv = TRUE;
      break;
    case CQ_KEY_PAGE_DOWN:                /* KP lower right */
      *angle = 315.0;
      rv = TRUE;
      break;
    case CQ_KEY_UP:                /* up arrow */
      *angle = 90.0;
      rv = TRUE;
      break;
    case CQ_KEY_DOWN:              /* down arrow */
      *angle = 270.0;
      rv = TRUE;
      break;
    case CQ_KEY_LEFT:              /* left arrow */
      *angle = 180.0;
      rv = TRUE;
      break;
    case CQ_KEY_RIGHT:             /* right arrow */
      *angle = 0.0;
      rv = TRUE;
      break;
    default:
      rv = FALSE;
      break;
    }

  return(rv);
}



static void _infoship( int snum, int scanner )
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
  
  clrPrompt(MSG_LIN1);
  if ( snum < 1 || snum > MAXSHIPS )
    {
      cp_putmsg( "No such ship.", MSG_LIN1 );
      return;
    }
  status = Ships[snum].status;
  if ( ! godlike && status != SS_LIVE )
    {
      cp_putmsg( "Not found.", MSG_LIN1 );
      return;
    }

  cbuf[0] = Context.lasttarg[0] = EOS;
  appship( snum, cbuf );
  strcpy(Context.lasttarg, cbuf); /* save for alt hud */

  if ( snum == scanner )
    {
      /* Silly Captain... */
      appstr( ": That's us, silly!", cbuf );
      cp_putmsg( cbuf, MSG_LIN1 );
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
  
  cp_putmsg( cbuf, MSG_LIN1 );
  
  if ( ! SCLOAKED(snum) || Ships[snum].warp != 0.0 )
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
		  clog("_infoship: close_rate(%.1f) = diffdis(%.1f) / difftime(%d), avgclose_rate = %.1f",
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
clog("_infoship:\tdis(%.1f) pwarp(%.1f) = (close_rate(%.1f) / MM_PER_SEC_PER_WARP(%.1f)", dis, pwarp, close_rate, MM_PER_SEC_PER_WARP);
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
      cp_putmsg( cbuf, MSG_LIN2 );
    }
  
  return;
  
}



static void _infoplanet( char *str, int pnum, int snum )
{
  int i, j; 
  int godlike, canscan; 
  char buf[MSGMAXLINE*2], junk[MSGMAXLINE];
  real x, y;
  
  /* Check range of the passed planet number. */
  if ( pnum <= 0 || pnum > NUMPLANETS )
    {
      cp_putmsg( "No such planet.", MSG_LIN1 );
      clrPrompt(MSG_LIN2);
      cerror("_infoplanet: Called with invalid pnum (%d).",
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
      cp_putmsg( buf, MSG_LIN1 );
      if ( junk[0] != EOS )
	cp_putmsg(junk, MSG_LIN2);
      else
	clrPrompt(MSG_LIN2);
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
      cp_putmsg( buf, MSG_LIN1 );
      cp_putmsg( &buf[i+1], MSG_LIN2 );
    }
  
  return;
  
}


static void _dowarp( int snum, real warp )
{
  clrPrompt(MSG_LIN2);
  
  /* Handle ship limitations. */
  
  warp = min( warp, ShipTypes[Ships[snum].shiptype].warplim );
  if (!sendCommand(CPCMD_SETWARP, (Unsgn16)warp))
    return;
  
  sprintf( cbuf, "Warp %d.", (int) warp );
  cp_putmsg( cbuf, MSG_LIN1 );
  
  return;
  
}

/* get a target */
static int _gettarget(char *buf, real cdefault, real *dir, char ch)
{
  int i, j; 
  
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

static int _chktorp(void)
{
  int snum = Context.snum;
  
  if ( SCLOAKED(snum) )
    {
      cp_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return FALSE;
    }
  if ( Ships[snum].wfuse > 0 )
    {
      cp_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return FALSE;
    }
  if ( Ships[snum].fuel < TORPEDO_FUEL )
    {
      cp_putmsg( "Not enough fuel to launch a torpedo.", MSG_LIN1 );
      return FALSE;
    }

  return TRUE;
}

static int _chkphase(void)
{
  int snum = Context.snum;

  if ( SCLOAKED(snum) )
    {
      cp_putmsg( "The cloaking device is using all available power.",
	       MSG_LIN1 );
      return FALSE;
    }
  if ( Ships[snum].wfuse > 0 )
    {
      cp_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return FALSE;
    }
  if ( Ships[snum].fuel < PHASER_FUEL )
    {
      cp_putmsg( "Not enough fuel to fire phasers.", MSG_LIN1 );
      return FALSE;
    }

  return TRUE;
}

static void _dophase( real dir )
{
  int snum = Context.snum;
  
  if ( Ships[snum].wfuse > 0 )
    {
      cp_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
      return;
    }
  if ( Ships[snum].fuel < PHASER_FUEL )
    {
      cp_putmsg( "Not enough fuel to fire phasers.", MSG_LIN2 );
      return;
    }

  if ( Ships[snum].pfuse == 0 && clbUseFuel( snum, PHASER_FUEL, 
                                          TRUE, FALSE ) )
    {			/* a local approximation of course */
      cp_putmsg( "Firing phasers...", MSG_LIN2 );
      Ships[snum].pfuse = -1;   /* fake it out until next upd */
      sendCommand(CPCMD_FIREPHASER, (Unsgn16)(dir * 100.0));
    }
  else
    cp_putmsg( ">PHASERS DRAINED<", MSG_LIN2 );

  return;
  
}

static void _dotorp(real dir, int num)
{
  int snum = Context.snum;

  if ( ! clbCheckLaunch( snum, num ) )
    cp_putmsg( ">TUBES EMPTY<", MSG_LIN2 );
  else
    {			/* a local approx */
      sendFireTorps(num, dir);
      clrPrompt(MSG_LIN1);
    }

  return;
}


/*  doinfo - do an info command */
/*  SYNOPSIS */
/*    int snum */
/*    doinfo( snum ) */
static void _doinfo( char *buf, char ch )
{
  int snum = Context.snum;
  int i, j, what, sorpnum, xsorpnum, count, token, now[NOWSIZE]; 
  int extra; 
  
  if ( ch == TERM_ABORT )
    {
      clrPrompt(MSG_LIN1);
      return;
    }
  extra = ( ch == TERM_EXTRA );
  
  /* Default to what we did last time. */
  delblanks( buf );
  fold( buf );
  if ( buf[0] == EOS )
    {
      c_strcpy( Context.lastinfostr, buf );
      if ( buf[0] == EOS )
	{
          clrPrompt(MSG_LIN1);
	  return;
	}
    }
  else
    c_strcpy( buf, Context.lastinfostr );
  
  if ( special( buf, &what, &token, &count ) )
    {
      if ( ! clbFindSpecial( snum, token, count, &sorpnum, &xsorpnum ) )
	what = NEAR_NONE;
      else if ( extra )
	{
	  if ( xsorpnum == 0 )
	    what = NEAR_NONE;
	  else
	    sorpnum = xsorpnum;
	}
      
      if ( what == NEAR_SHIP )
	_infoship( sorpnum, snum );
      else if ( what == NEAR_PLANET )
	_infoplanet( "", sorpnum, snum );
      else
	cp_putmsg( "Not found.", MSG_LIN2 );
    }
  else if ( buf[0] == 's' && alldig( &buf[1] ) == TRUE )
    {
      i = 1;
      safectoi( &j, buf, i );		/* ignore status */
      _infoship( j, snum );
    }
  else if ( alldig( buf ) == TRUE )
    {
      i = 0;
      safectoi( &j, buf, i );		/* ignore status */
      _infoship( j, snum );
    }
  else if ( clbPlanetMatch( buf, &j, FALSE ) )
    _infoplanet( "", j, snum );
  else if ( stmatch( buf, "time", FALSE ) )
    {
      getnow( now, 0 );
      c_strcpy( "It's ", buf );
      appnumtim( now, buf );
      appchr( '.', buf );
      cp_putmsg( buf, MSG_LIN1 );
    }
  else
    {
      cp_putmsg( "I don't understand.", MSG_LIN2 );
    }
  
  return;
  
}


/* 'read' a msg */
static void rmesg(int snum, int msgnum, int lin)
{
  char buf[MSGMAXLINE];
  
  clbFmtMsg(Msgs[msgnum].msgto, Msgs[msgnum].msgfrom, buf);
  appstr( ": ", buf );
  appstr( Msgs[msgnum].msgbuf, buf );

  setPrompt(lin, NULL, NoColor, buf, CyanColor);

  return;
}

static void _domydet(void)
{
  clrPrompt(MSG_LIN2);
  
  sendCommand(CPCMD_DETSELF, 0);

  cp_putmsg( "Detonating...", MSG_LIN1 );

  return;
  
}

static void _doshields( int snum, int up )
{

  if (!sendCommand(CPCMD_SETSHIELDS, (Unsgn16)up))
    return;

  if ( up )
    {
      SFCLR(snum, SHIP_F_REPAIR);
      cp_putmsg( "Shields raised.", MSG_LIN1 );
    }
  else
    cp_putmsg( "Shields lowered.", MSG_LIN1 );

  clrPrompt(MSG_LIN2);
  
  return;
  
}

static void _doorbit( int snum )
{
  int pnum;
  
  if ( ( Ships[snum].warp == ORBIT_CW ) || ( Ships[snum].warp == ORBIT_CCW ) )
    _infoplanet( "But we are already orbiting ", -Ships[snum].lock, snum );
  else if ( ! clbFindOrbit( snum, &pnum ) )
    {
      sprintf( cbuf, "We are not close enough to orbit, %s.",
	     Ships[snum].alias );
      cp_putmsg( cbuf, MSG_LIN1 );
      clrPrompt(MSG_LIN2);
    }
  else if ( Ships[snum].warp > MAX_ORBIT_WARP )
    {
      sprintf( cbuf, "We are going too fast to orbit, %s.",
	     Ships[snum].alias );
      cp_putmsg( cbuf, MSG_LIN1 );
      sprintf( cbuf, "Maximum orbital insertion velocity is warp %.1f.",
	     oneplace(MAX_ORBIT_WARP) );
      cp_putmsg( cbuf, MSG_LIN2 );
    }
  else
    {
      sendCommand(CPCMD_ORBIT, 0);
      _infoplanet( "Coming into orbit around ", pnum, snum );
    }
  
  return;
  
}

static void _doalloc(char *buf, char ch)
{
  int snum = Context.snum;
  int i, alloc;
  int dwalloc = 0;
  
  switch (ch)
    {
    case TERM_EXTRA:
      dwalloc = Ships[snum].engalloc;
      break;

    case TERM_NORMAL:
      i = 0;
      safectoi( &alloc, buf, i );			/* ignore status */
      if ( alloc != 0 )
	{
	  if ( alloc < 30 )
	    alloc = 30;
	  else if ( alloc > 70 )
	    alloc = 70;
	  dwalloc = alloc;
	}
      else
	{
          clrPrompt(MSG_LIN1);
	  return;
	}
     
      break;

    default:
      return;
    }

  sendCommand(CPCMD_ALLOC, (Unsgn16)dwalloc);

  clrPrompt(MSG_LIN1);
  
  return;
  
}

static void _dodet( void )
{
  int snum = Context.snum;

  clrPrompt(MSG_LIN1);
  
  if ( Ships[snum].wfuse > 0 )
    cp_putmsg( "Weapons are currently overloaded.", MSG_LIN1 );
  else if ( clbUseFuel( snum, DETONATE_FUEL, TRUE, FALSE ) )
    {				/* we don't really use fuel here on the
				   client*/
      cp_putmsg( "detonating...", MSG_LIN1 );
      sendCommand(CPCMD_DETENEMY, 0);
    }
  else
    cp_putmsg( "Not enough fuel to fire detonators.", MSG_LIN1 );
  
  return;
  
}

static void _dodistress(char *buf, char ch)
{
  clrPrompt(MSG_LIN1);
  clrPrompt(MSG_LIN2);

  if (ch == TERM_EXTRA)
    sendCommand(CPCMD_DISTRESS, (Unsgn16)UserConf.DistressToFriendly);
  
  clrPrompt(MSG_LIN1);
  
  return;
  
}

static int _chkrefit(void)
{
  int snum = Context.snum;
  int pnum;
  static string ntp="We must be orbiting a team owned planet to refit.";
  static string nek="You must have at least one kill to refit.";
  static string cararm="You cannot refit while carrying armies";
  
  clrPrompt(MSG_LIN2);
  
  /* Check for allowability. */
  if ( oneplace( Ships[snum].kills ) < MIN_REFIT_KILLS )
    {
      cp_putmsg( nek, MSG_LIN1 );
      return FALSE;
    }

  pnum = -Ships[snum].lock;

  if (Planets[pnum].team != Ships[snum].team || Ships[snum].warp >= 0.0)
    {
      cp_putmsg( ntp, MSG_LIN1 );
      return FALSE;
    }

  if (Ships[snum].armies != 0)
    {
      cp_putmsg( cararm, MSG_LIN1 );
      return FALSE;
    }

  return TRUE;
}

static int _chkcoup(void)
{
  int snum = Context.snum;
  int i, pnum;
  string nhp="We must be orbiting our home planet to attempt a coup.";
  
  clrPrompt(MSG_LIN2);
  
  /* some checks we will do locally, the rest will have to be handled by
     the server */
  /* Check for allowability. */
  if ( oneplace( Ships[snum].kills ) < MIN_COUP_KILLS )
    {
      cp_putmsg(
	       "Fleet orders require three kills before a coup can be attempted.",
	       MSG_LIN1 );
      return FALSE;
    }
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    if ( Planets[i].team == Ships[snum].team && Planets[i].armies > 0 )
      {
	cp_putmsg( "We don't need to coup, we still have armies left!",
		 MSG_LIN1 );
	return FALSE;
      }
  if ( Ships[snum].warp >= 0.0 )
    {
      cp_putmsg( nhp, MSG_LIN1 );
      return FALSE;
    }
  pnum = -Ships[snum].lock;
  if ( pnum != Teams[Ships[snum].team].homeplanet )
    {
      cp_putmsg( nhp, MSG_LIN1 );
      return FALSE;
    }
  if ( Planets[pnum].armies > MAX_COUP_ENEMY_ARMIES )
    {
      cp_putmsg( "The enemy is still too strong to attempt a coup.",
	       MSG_LIN1 );
      return FALSE;
    }
  i = Planets[pnum].uninhabtime;
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minutes.",
	     i );
      cp_putmsg( cbuf, MSG_LIN1 );
      return FALSE;
    }

  return TRUE;
}  

static int _chktow(void)
{
  int snum = Context.snum;
  
  clrPrompt(MSG_LIN1);
  clrPrompt(MSG_LIN2);

  if ( Ships[snum].towedby != 0 )
    {
      c_strcpy( "But we are being towed by ", cbuf );
      appship( Ships[snum].towedby, cbuf );
      appchr( '!', cbuf );
      cp_putmsg( cbuf, MSG_LIN2 );
      return FALSE;
    }
  if ( Ships[snum].towing != 0 )
    {
      c_strcpy( "But we're already towing ", cbuf );
      appship( Ships[snum].towing, cbuf );
      appchr( '.', cbuf );
      cp_putmsg( cbuf, MSG_LIN2 );
      return FALSE;
    }

  return TRUE;
}

static void _dotow(char *buf, int ch)
{
  int i, other;
  if (ch == TERM_ABORT)
    return;
  i = 0;
  safectoi( &other, cbuf, i );		/* ignore status */
  
  sendCommand(CPCMD_TOW, (Unsgn16)other);

  return;
}

/* modifies state */
static void _domsgto(char *buf, int ch, int terse)
{
  int i, j; 
  static char tbuf[MESSAGE_SIZE];
  string nf="Not found.";
  string huh="I don't understand.";
  int editing;
  static int to = MSG_NOONE;
  
  /* First, find out who we're sending to. */
  clrPrompt(MSG_LIN1);
  clrPrompt(MSG_LIN2);

  if ( ch == TERM_ABORT )
    {
      clrPrompt(MSG_LIN1);
      state = S_NONE;
      prompting = FALSE;

      return;
    }

  c_strcpy( buf, tbuf);  

  /* TAB or RETURN means use the target from the last message. */
  editing = ( (ch == TERM_EXTRA || ch == TERM_NORMAL) && buf[0] == EOS );
  if ( editing )
    {
      /* Make up a default string using the last target. */
      if ( to > 0 && to <= MAXSHIPS )
	sprintf( tbuf, "%d", to );
      else if ( -to >= 0 && -to < NUMPLAYERTEAMS )
	c_strcpy( Teams[-to].name, tbuf );
      else switch ( to )
	{
	case MSG_ALL:
	  c_strcpy( "All", tbuf );
	  break;
	case MSG_GOD:
	  c_strcpy( "GOD", tbuf );
	  break;
	case MSG_IMPLEMENTORS:
	  c_strcpy( "Implementors", tbuf );
	  break;
	case MSG_FRIENDLY:
	  c_strcpy( "Friend", tbuf );
	  break;
	default:
	  tbuf[0] = EOS;
	  break;
	}
    }

  /* Got a target, parse it. */
  delblanks( tbuf );
  upper( tbuf );
  if ( alldig( tbuf ) == TRUE )
    {
      /* All digits means a ship number. */
      i = 0;
      safectoi( &j, tbuf, i );		/* ignore status */
      if ( j < 1 || j > MAXSHIPS )
	{
	  cp_putmsg( "No such ship.", MSG_LIN2 );
          clrPrompt(MSG_LIN1);
          state = S_NONE;
          prompting = FALSE;
	  return;
	}
      if ( Ships[j].status != SS_LIVE )
	{
	  cp_putmsg( nf, MSG_LIN2 );
          clrPrompt(MSG_LIN1);
          state = S_NONE;
          prompting = FALSE;
	  return;
	}
      to = j;
    }
  else switch ( tbuf[0] )
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
      if (tbuf[0] == 'F' && tbuf[1] == 'R')
	{			/* to friendlies */
	  to = MSG_FRIENDLY;
	}
      else
	{
	  /* Check for a team character. */
	  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
	    if ( tbuf[0] == Teams[i].teamchar || tbuf[0] == (char)tolower(Teams[i].teamchar) )
	      break;
	  if ( i >= NUMPLAYERTEAMS )
	    {
	      cp_putmsg( huh, MSG_LIN2 );
              clrPrompt(MSG_LIN1);
              state = S_NONE;
              prompting = FALSE;
	      return;
	    }
	  to = -i;
	};
      break;
    }
  
  /* Now, construct a header for the selected target. */
  c_strcpy( "Message to ", tbuf );
  if ( to > 0 && to <= MAXSHIPS )
    {
      if ( Ships[to].status != SS_LIVE )
	{
	  cp_putmsg( nf, MSG_LIN2 );
          clrPrompt(MSG_LIN1);
          state = S_NONE;
          prompting = FALSE;
	  return;
	}
      appship( to, tbuf );
      appchr( ':', tbuf );
    }
  else if ( -to >= 0 && -to < NUMPLAYERTEAMS )
    {
      appstr( Teams[-to].name, tbuf );
      appstr( "s:", tbuf );
    }
  else switch ( to ) 
    {
    case MSG_ALL:
      appstr( "everyone:", tbuf );
      break;
    case MSG_GOD:
      appstr( "GOD:", tbuf );
      break;
    case MSG_IMPLEMENTORS:
      appstr( "The Implementors:", tbuf );
      break;
    case MSG_FRIENDLY:
      appstr( "Friend:", tbuf );
      break;
    default:
      cp_putmsg( huh, MSG_LIN2 );
      return;
      break;
    }
  
  if (!terse)
    appstr( " (ESCAPE to abort)", tbuf );
  
  cp_putmsg( tbuf, MSG_LIN1 );
  clrPrompt(MSG_LIN2);
  
  msgto = to;                   /* set global */

  state = S_MSG;
  prm.preinit = False;
  prm.buf = cbuf;
  prm.buflen = MESSAGE_SIZE;
  strcpy(pbuf, "> ");
  prm.pbuf = pbuf;
  prm.terms = TERMS;
  prm.index = MSG_LIN2;
  prm.buf[0] = EOS;
  setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
  prompting = TRUE;
  
  return;
}

/* modifies state */
static void _domsg(char *msg, int ch, int irv)
{
  static char mbuf[MSGMAXLINE];
  char *cptr;
  int len = strlen(msg);

  /* if maxlen reached */
  if (irv == PRM_MAXLEN)
    {                           /* then we need to send what we have
                                   and continue */
      mbuf[0] = EOS;
      cptr = &msg[len - 1];

      while ((cptr > msg) && *cptr != ' ')
        cptr--;

      if (cptr > msg)
        {
          *cptr = EOS;
          sprintf(mbuf, "%s -", msg);

          cptr++;
          strcpy(prm.pbuf, "- ");
          sprintf(prm.buf, "%s%c", cptr, ch);
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          sendMessage(msgto, mbuf);
        }
      else
        {

          strcpy(mbuf, msg);

          strcpy(prm.pbuf, "- ");
          prm.buf[0] = ch;
          prm.buf[1] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);
          sendMessage(msgto, mbuf);
        }

      return;
    }
  else
    {                           /* ready or abort */
      if (ch != TERM_ABORT)
        sendMessage(msgto, msg); /* just send it */

      state = S_NONE;
      prompting = FALSE;
      clrPrompt(MSG_LIN1);
      clrPrompt(MSG_LIN2);
    }

  return;
}

static int _xlateFKey(int ch)
{
  int fkey = -1;

  switch (ch & CQ_FKEY_MASK)
    {
    case CQ_KEY_F1:
      fkey = 1;
      break;
    case CQ_KEY_F2:
      fkey = 2;
      break;
    case CQ_KEY_F3:
      fkey = 3;
      break;
    case CQ_KEY_F4:
      fkey = 4;
      break;
    case CQ_KEY_F5:
      fkey = 5;
      break;
    case CQ_KEY_F6:
      fkey = 6;
      break;
    case CQ_KEY_F7:
      fkey = 7;
      break;
    case CQ_KEY_F8:
      fkey = 8;
      break;
    case CQ_KEY_F9:
      fkey = 9;
      break;
    case CQ_KEY_F10:
      fkey = 10;
      break;
    case CQ_KEY_F11:
      fkey = 11;
      break;
    case CQ_KEY_F12:
      fkey = 12;
      break;
    default:
      fkey = -1;
    }

  if (fkey == -1)
    return fkey;

  if (CQ_MODIFIER(ch) & CQ_KEY_MOD_SHIFT) 
    fkey += 12;
  else if (CQ_MODIFIER(ch) & CQ_KEY_MOD_CTRL)
    fkey += 24;
  else if (CQ_MODIFIER(ch) & CQ_KEY_MOD_ALT)
    fkey += 36;

  fkey--;

  return fkey;
}


/*  _docourse - set course */
/*  SYNOPSIS */
/*    int snum */
/*    _docourse( snum ) */
static void _docourse( char *buf, char ch)
{
  int i, j, what, sorpnum, xsorpnum, newlock, token, count;
  real dir, appx, appy; 
  int snum = Context.snum;

  clrPrompt(MSG_LIN1);
  clrPrompt(MSG_LIN2);

  delblanks( buf );
  if ( ch == TERM_ABORT || buf[0] == EOS )
    {
      clrPrompt(MSG_LIN1);
      return;
    }
  
  newlock = 0;				/* default to no lock */
  fold( buf );
  
  what = NEAR_ERROR;
  if ( alldig( buf ) == TRUE )
    {
      /* Raw angle. */
      clrPrompt( MSG_LIN1 );
      i = 0;
      if ( safectoi( &j, buf, i ) )
	{
	  what = NEAR_DIRECTION;
	  dir = (real)mod360( (real)( j ) );
	}
    }
  else if ( buf[0] == 's' && alldig( &buf[1] ) == TRUE )
    {
      /* Ship. */

      i = 1;
      if ( safectoi( &sorpnum, buf, i ) )
	what = NEAR_SHIP;
    }
  else if ( arrows( buf, &dir ) )
    what = NEAR_DIRECTION;
  else if ( special( buf, &i, &token, &count ) )
    {
      if ( clbFindSpecial( snum, token, count, &sorpnum, &xsorpnum ) )
	what = i;
    }
  else if ( clbPlanetMatch( buf, &sorpnum, FALSE ) )
    what = NEAR_PLANET;
  
  switch ( what )
    {
    case NEAR_SHIP:
      if ( sorpnum < 1 || sorpnum > MAXSHIPS )
	{
	  cp_putmsg( "No such ship.", MSG_LIN2 );
	  return;
	}
      if ( sorpnum == snum )
	{
          clrPrompt(MSG_LIN1);
	  return;
	}
      if ( Ships[sorpnum].status != SS_LIVE )
	{
	  cp_putmsg( "Not found.", MSG_LIN2 );
	  return;
	}

      if ( SCLOAKED(sorpnum) )
	{
	  if ( Ships[sorpnum].warp <= 0.0 )
	    {
	      cp_putmsg( "Sensors are unable to lock on.", MSG_LIN2 );
	      return;
	    }
	}

      appx = Ships[sorpnum].x;
      appy = Ships[sorpnum].y;

      dir = (real)angle( Ships[snum].x, Ships[snum].y, appx, appy );
      
      /* Give info if he used TAB. */
      if ( ch == TERM_EXTRA )
	_infoship( sorpnum, snum );
      else
        clrPrompt(MSG_LIN1);
      break;
    case NEAR_PLANET:
      dir = angle( Ships[snum].x, Ships[snum].y, Planets[sorpnum].x, Planets[sorpnum].y );
      if ( ch == TERM_EXTRA )
	{
	  newlock = -sorpnum;
	  _infoplanet( "Now locked on to ", sorpnum, snum );
	}
      else
	_infoplanet( "Setting course for ", sorpnum, snum );
      break;
    case NEAR_DIRECTION:
      clrPrompt(MSG_LIN1);
      break;
    case NEAR_NONE:
      cp_putmsg( "Not found.", MSG_LIN2 );
      return;
      break;
    default:
      /* This includes NEAR_ERROR. */
      cp_putmsg( "I don't understand.", MSG_LIN2 );
      return;
      break;
    }
  
  sendSetCourse(cInfo.sock, newlock, dir);

  return;
  
}

/* will decloak if cloaked */
static int _chkcloak(void)
{
  int snum = Context.snum;
  string nofuel="Not enough fuel to engage cloaking device.";
  
  clrPrompt(MSG_LIN1);
  clrPrompt(MSG_LIN2);
  
  if ( SCLOAKED(snum) )
    {
      sendCommand(CPCMD_CLOAK, 0);
      cp_putmsg( "Cloaking device disengaged.", MSG_LIN1 );
      SFCLR(snum, SHIP_F_CLOAKED); /* do it locally too */
      return FALSE;
    }
  if ( Ships[snum].efuse > 0 )
    {
      cp_putmsg( "Engines are currently overloaded.", MSG_LIN1 );
      return FALSE;
    }
  if ( Ships[snum].fuel < CLOAK_ON_FUEL )
    {
      cp_putmsg( nofuel, MSG_LIN1 );
      return FALSE;
    }

  return TRUE;
}

static void _docloak( char *buf, char ch)
{
  int snum = Context.snum;
  string nofuel="Not enough fuel to engage cloaking device.";
  
  clrPrompt(MSG_LIN1);

  if (ch == TERM_EXTRA)
    {
      if ( ! clbUseFuel( snum, CLOAK_ON_FUEL, FALSE, TRUE ) )
	{			/* an approximation of course... */
	  cp_putmsg( nofuel, MSG_LIN2 );
	  return;
	}

      sendCommand(CPCMD_CLOAK, 0);
      SFSET(snum, SHIP_F_CLOAKED); /* do it locally */
      cp_putmsg( "Cloaking device engaged.", MSG_LIN2 );
    }

  clrPrompt(MSG_LIN1);

  return;
  
}

static int _review(void)
{
  int snum = Context.snum;
  
  if ( clbCanRead( snum, msg ))
    {
      rmesg( snum, msg, MSG_LIN1 );
    }
  else
    {
      msg = modp1( msg - 1, MAXMESSAGES );
      if (msg == lastone)
        {
          state = S_NONE;
          return FALSE;
        }
    }

  return TRUE;
  
}


static void _doreview(void)
{
  int snum = Context.snum;
  int i;

  lstmsg = Ships[snum].lastmsg;	/* don't want lstmsg changing while
                                   reading old ones. */

  lastone = modp1( ConqInfo->lastmsg+1, MAXMESSAGES );
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      if ( Ships[snum].lastmsg == LMSG_NEEDINIT )
        {
          cp_putmsg( "There are no old messages.", MSG_LIN1 );
          return;               /* none to read */
        }
      i = Ships[snum].alastmsg;
      if ( i != LMSG_READALL )
        lastone = i;
    }

  msg = lstmsg;
  
  state = S_REVIEW;
  
  clrPrompt(MSG_LIN1);
  if (!_review())
    {
      cp_putmsg( "There are no old messages.", MSG_LIN1 );
      return;               /* none to read */
    }
      
          
  cp_putmsg("--- [SPACE] for more, arrows to scroll, any key to quit ---",
                    MSG_LIN2);

  return;
  
}

static void _dobomb(void)
{
  int snum = Context.snum;
  int pnum;
  
  SFCLR(snum, SHIP_F_REPAIR);;

  clrPrompt(MSG_LIN1);  
  clrPrompt(MSG_LIN2);  
  
  /* Check for allowability. */
  if ( Ships[snum].warp >= 0.0 )
    {
      cp_putmsg( "We must be orbiting a planet to bombard it.", MSG_LIN1 );
      return;
    }
  pnum = -Ships[snum].lock;
  if ( Planets[pnum].type == PLANET_SUN || Planets[pnum].type == PLANET_MOON ||
      Planets[pnum].team == TEAM_NOTEAM || Planets[pnum].armies == 0 )
    {
      cp_putmsg( "There is no one there to bombard.", MSG_LIN1 );
      return;
    }
  if ( Planets[pnum].team == Ships[snum].team )
    {
      cp_putmsg( "We can't bomb our own armies!", MSG_LIN1 );
      return;
    }

  if ( Planets[pnum].team != TEAM_SELFRULED && Planets[pnum].team != TEAM_GOD )
    if ( ! Ships[snum].war[Planets[pnum].team] )
      {
	cp_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
	return;
      }
  
  /* set up the state, and proceed. */

  sprintf( pbuf, "Press TAB to bombard %s, %d armies:",
           Planets[pnum].name, Planets[pnum].armies );

  state = S_BOMB;
  prm.preinit = False;
  prm.buf = cbuf;
  prm.buf[0] = EOS;
  prm.buflen = MSGMAXLINE;
  prm.pbuf = pbuf;
  prm.terms = TERMS;
  prm.index = MSG_LIN1;
  setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
  prompting = TRUE;

  lastServerError = 0;          /* so the server can tell us to stop */

  return;
}

/* sets state */
static void _initbeam()
{
  int snum = Context.snum;
  int pnum, capacity, i;
  real rkills;
  string lastfew="Fleet orders prohibit removing the last three armies.";
  
  clrPrompt(MSG_LIN1);
  clrPrompt(MSG_LIN2);

  /* all of these checks are performed server-side as well, but we also
     check here for the obvious problems so we can save time without
     bothering the server for something it will refuse anyway. */
  
  /* at least the basic checks could be split into a seperate func that
     could be used by both client and server */

  /* Check for allowability. */
  if ( Ships[snum].warp >= 0.0 )
    {
      cp_putmsg( "We must be orbiting a planet to use the transporter.",
	       MSG_LIN1 );
      return;
    }
  pnum = -Ships[snum].lock;
  if ( Ships[snum].armies > 0 )
    {
      if ( Planets[pnum].type == PLANET_SUN )
	{
	  cp_putmsg( "Idiot!  Our armies will fry down there!", MSG_LIN1 );
	  return;
	}
      else if ( Planets[pnum].type == PLANET_MOON )
	{
	  cp_putmsg( "Fool!  Our armies will suffocate down there!",
		   MSG_LIN1 );
	  return;
	}
      else if ( Planets[pnum].team == TEAM_GOD )
	{
	  cp_putmsg(
		   "GOD->you: YOUR ARMIES AREN'T GOOD ENOUGH FOR THIS PLANET.",
		   MSG_LIN1 );
	  return;
	}
    }
  
  i = Planets[pnum].uninhabtime;
  if ( i > 0 )
    {
      sprintf( cbuf, "This planet is uninhabitable for %d more minute",
	     i );
      if ( i != 1 )
	appchr( 's', cbuf );
      appchr( '.', cbuf );
      cp_putmsg( cbuf, MSG_LIN1 );
      return;
    }
  
  /* can take empty planets */
  if ( Planets[pnum].team != Ships[snum].team &&
      Planets[pnum].team != TEAM_SELFRULED &&
      Planets[pnum].team != TEAM_NOTEAM )
    if ( ! Ships[snum].war[Planets[pnum].team] && Planets[pnum].armies != 0) 
      {
	cp_putmsg( "But we are not at war with this planet!", MSG_LIN1 );
	return;
      }
  
  if ( Ships[snum].armies == 0 &&
      Planets[pnum].team == Ships[snum].team && Planets[pnum].armies <= MIN_BEAM_ARMIES )
    {
      cp_putmsg( lastfew, MSG_LIN1 );
      return;
    }
  
  rkills = Ships[snum].kills;

  if ( rkills < (real)1.0 )
    {
      cp_putmsg(
	       "Fleet orders prohibit beaming armies until you have a kill.",
	       MSG_LIN1 );
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
      capacity = min( ifix( rkills ) * 2, ShipTypes[Ships[snum].shiptype].armylim );
      upmax = min( Planets[pnum].armies - MIN_BEAM_ARMIES, 
		   capacity - Ships[snum].armies );
    }
  
  /* If there are armies to beam but we're selfwar... */
  if ( upmax > 0 && selfwar(snum) && Ships[snum].team == Planets[pnum].team )
    {
      if ( downmax <= 0 )
	{
	  c_strcpy( "The arm", cbuf );
	  if ( upmax == 1 )
	    appstr( "y is", cbuf );
	  else
	    appstr( "ies are", cbuf );
	  appstr( " reluctant to beam aboard a pirate vessel.", cbuf );
	  cp_putmsg( cbuf, MSG_LIN1 );
	  return;
	}
      upmax = 0;
    }
  
  /* Figure out which direction to beam. */
  if ( upmax <= 0 && downmax <= 0 )
    {
      cp_putmsg( "There is no one to beam.", MSG_LIN1 );
      return;
    }

  if ( upmax <= 0 )
    dirup = FALSE;
  else if ( downmax <= 0 )
    dirup = TRUE;
  else
    {                           /* need to ask beam dir... */
      state = S_BEAMDIR;
      prm.preinit = False;
      prm.buf = cbuf;
      prm.buflen = 10;
      prm.pbuf = "Beam [up or down] ";
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prm.buf[0] = EOS;
      setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
      prompting = TRUE;

      return;
    }

  /* else, get the number to beam */

  if ( dirup )
    beamax = upmax;
  else
    beamax = downmax;

  /* Figure out how many armies should be beamed. */
  sprintf( pbuf, "Beam %s [1-%d] ", (dirup) ? "up" : "down", beamax );

  state = S_BEAMNUM;
  prm.preinit = False;
  prm.buf = cbuf;
  prm.buflen = 10;
  prm.pbuf = pbuf;
  prm.terms = TERMS;
  prm.index = MSG_LIN1;
  prm.buf[0] = EOS;
  setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
  prompting = TRUE;
  
  return;
}

static void _dobeam(char *buf, int ch)
{
  int num, i;

  if ( ch == TERM_ABORT )
    {
      state = S_NONE;
      prompting = FALSE;
      cp_putmsg( abt, MSG_LIN1 );
      return;
    }
  else if ( ch == TERM_EXTRA && buf[0] == EOS )
    num = beamax;
  else
    {
      delblanks( buf );
      if ( alldig( buf ) != TRUE )
	{
          state = S_NONE;
          prompting = FALSE;
	  cp_putmsg( abt, MSG_LIN1 );
	  return;
	}
      i = 0;
      safectoi( &num, buf, i );			/* ignore status */
      if ( num < 1 || num > beamax )
	{
	  cp_putmsg( abt, MSG_LIN1 );
          state = S_NONE;
          prompting = FALSE;
	  return;
	}
    }

  /* now we start the phun. */

  lastServerError = 0;

  /* detail is (armies & 0x00ff), 0x8000 set if beaming down */

  sendCommand(CPCMD_BEAM, 
	      (dirup) ? (Unsgn16)(num & 0x00ff): 
	      (Unsgn16)((num & 0x00ff) | 0x8000));

  state = S_BEAMING;
  prompting = FALSE;

  return;
  
}


/*  command - execute a user's command  ( COMMAND ) */
/*  SYNOPSIS */
/*    char ch */
/*    command( ch ) */
static void command( int ch )
{
  int i;
  real x;
  int snum = Context.snum;

  if (_KPAngle(ch, &x))         /* hit a keypad/arrow key */
    {				/* alter course Mr. Sulu. */
      clrPrompt(MSG_LIN1);
      clrPrompt(MSG_LIN2);
      
      sendSetCourse(cInfo.sock, 0, x);
      return;
    }

  switch ( ch )
    {
    case '0':           /* - '9', '=' :set warp factor */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '=':
      if ( ch == '=' )
	x = 10.0;
      else
	{
	  i = ch - '0';
	  x = (real) (i); 
	}
      _dowarp( Context.snum, x );
      break;
    case 'a':				/* autopilot */
      if ( Users[Ships[Context.snum].unum].ooptions[ OOPT_AUTOPILOT] )
	{
          state = S_DOAUTOPILOT;
          prm.preinit = False;
          prm.buf = cbuf;
          prm.buflen = MSGMAXLINE;
          prm.pbuf = "Press TAB to engage autopilot: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prm.buf[0] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
          prompting = TRUE;
	}
      else
	{
          mglBeep();
          cp_putmsg( "Type h for help.", MSG_LIN2 );
	}
      break;
    case 'A':				/* change allocation */
      state = S_ALLOC;
      prm.preinit = False;
      prm.buf = cbuf;
      prm.buflen = MSGMAXLINE;
      prm.pbuf = "New weapons allocation: (30-70) ";
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prm.buf[0] = EOS;
      setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
      prompting = TRUE;
      break;
    case 'b':				/* beam armies */
      _initbeam();
      break;
    case 'B':				/* bombard a planet */
      _dobomb();  /* will set state */
      break;
    case 'C':				/* cloak control */
      if (_chkcloak())
        {
          state = S_CLOAK;
          prm.preinit = False;
          prm.buf = cbuf;
          prm.buflen = MSGMAXLINE;
          prm.pbuf = "Press TAB to engage cloaking device: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prm.buf[0] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
          prompting = TRUE;
        }
      break;
    case 'd':				/* detonate enemy torps */
    case '*':
      _dodet();
      break;
    case 'D':				/* detonate own torps */
      _domydet();
      break;
    case 'E':				/* emergency distress call */
      state = S_DISTRESS;
      prm.preinit = False;
      prm.buf = cbuf;
      prm.buflen = MSGMAXLINE;
      prm.pbuf = "Press TAB to send an emergency distress call: ";
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prm.buf[0] = EOS;
      setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);

      prompting = TRUE;
      break;
    case 'f':				/* phasers */
      if (_chkphase())
        {
          state = S_TARGET;
          desttarg = T_PHASER;
          prm.preinit = False;
          prm.buf = cbuf;
          prm.buflen = MSGMAXLINE;
          prm.pbuf = "Fire phasers: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prm.buf[0] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
          prompting = TRUE;
        }
      break;
    case 'F':				/* phasers, same direction */
      _dophase(lastphase);
      break;
    case 'h':
      setONode(nCPHelpInit(FALSE));
      break;
    case 'H':
      setONode(nHistlInit(DSP_NODE_CP, FALSE));
      break;
    case 'i':				/* information */
      state = S_DOINFO;
      prm.preinit = False;
      prm.buf = cbuf;
      prm.buflen = MSGMAXLINE;
      prm.pbuf = "Information on: ";
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prm.buf[0] = EOS;
      setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);

      prompting = TRUE;
      break;
    case 'k':				/* set course */
      state = S_COURSE;
      prm.preinit = False;
      prm.buf = cbuf;
      prm.buflen = MSGMAXLINE;
      prm.pbuf = "Come to course: ";
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prm.buf[0] = EOS;
      setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
      prompting = TRUE;
      break;
    case 'K':				/* coup */
      if (_chkcoup())
        {
          state = S_COUP;
          prm.preinit = False;
          prm.buf = cbuf;
          prm.buflen = MSGMAXLINE;
          prm.pbuf = "Press TAB to try it: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prm.buf[0] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
          prompting = TRUE;
        }
      break;
    case 'L':                   /* review old messages */
      _doreview();              /* will set state */
      break;
    case 'm':				/* send a message */
      state = S_MSGTO;
      prm.preinit = False;
      prm.buf = cbuf;
      prm.buflen = MSGMAXLINE;
      strcpy(pbuf, "Message to: ");
      prm.pbuf = pbuf;
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prm.buf[0] = EOS;
      setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
      prompting = TRUE;
      break;
    case 'M':				/* strategic/tactical map */
      if (SMAP(Context.snum))
	SFCLR(Context.snum, SHIP_F_MAP);
      else
	SFSET(Context.snum, SHIP_F_MAP);
      break;
    case 'N':				/* change pseudonym */
      c_strcpy( "Old pseudonym: ", pbuf );
      appstr( Ships[Context.snum].alias, pbuf );
      cp_putmsg(pbuf, MSG_LIN1);
      state = S_PSEUDO;
      prm.preinit = False;
      prm.buf = cbuf;
      prm.buflen = MAXUSERPNAME;
      prm.pbuf = "Enter a new pseudonym: ";
      prm.terms = TERMS;
      prm.index = MSG_LIN2;
      prm.buf[0] = EOS;
      setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
      prompting = TRUE;

      break;

    case 'O':
      setONode(nOptionsInit(NOPT_USER, FALSE, DSP_NODE_CP));
      break;

    case 'o':				/* orbit nearby planet */
      _doorbit( Context.snum );
      break;
    case 'P':				/* photon torpedo burst */
      if (_chktorp())
        {
          state = S_TARGET;
          desttarg = T_BURST;
          prm.preinit = False;
          prm.buf = cbuf;
          prm.buflen = MSGMAXLINE;
          prm.pbuf = "Torpedo burst: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prm.buf[0] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
          prompting = TRUE;
        }
      break;
    case 'p':				/* photon torpedoes */
      if (_chktorp())
        {
          state = S_TARGET;
          desttarg = T_TORP;
          prm.preinit = False;
          prm.buf = cbuf;
          prm.buflen = MSGMAXLINE;
          prm.pbuf = "Launch torpedo: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prm.buf[0] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
          prompting = TRUE;
        }
      break;
    case 'Q':				/* self destruct */
      if ( SCLOAKED(Context.snum) )
        {
          cp_putmsg( "The cloaking device is using all available power.",
                     MSG_LIN1 );
        }
      else
        {
          state = S_DESTRUCT;
          prm.preinit = False;
          prm.buf = cbuf;
          prm.buflen = MSGMAXLINE;
          prm.pbuf = "Press TAB to initiate self-destruct sequence: ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prm.buf[0] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
          prompting = TRUE;
        }
      break;
    case 'r':				/* refit */
      if (sStat.flags & SPSSTAT_FLAGS_REFIT)
        {
          if (_chkrefit())
            {
              state = S_REFIT;
              prm.preinit = False;
              prm.buf = cbuf;
              prm.buflen = MSGMAXLINE;
              refitst = Ships[Context.snum].shiptype;
              sprintf(pbuf, "Refit ship type: %s", ShipTypes[refitst].name);
              prm.pbuf = pbuf;
              prm.terms = TERMS;
              prm.index = MSG_LIN1;
              prm.buf[0] = EOS;
              setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
              cp_putmsg("Press TAB to change, ENTER to accept: ", MSG_LIN2);
              prompting = TRUE;
            }
        }
      else
        mglBeep();
      break;
    case 'R':				/* repair mode */
      if ( ! SCLOAKED(Context.snum) )
	{
          clrPrompt(MSG_LIN1);
	  sendCommand(CPCMD_REPAIR, 0);
	}
      else
	{
          clrPrompt(MSG_LIN2);
	  cp_putmsg(
		   "You cannot repair while the cloaking device is engaged.",
		   MSG_LIN1 );
	}
      break;
    case 't':
      if (_chktow())
        {
          state = S_TOW;
          prm.preinit = False;
          prm.buf = cbuf;
          prm.buflen = MSGMAXLINE;
          prm.pbuf = "Tow which ship? ";
          prm.terms = TERMS;
          prm.index = MSG_LIN1;
          prm.buf[0] = EOS;
          setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
          prompting = TRUE;
        }
      break;
    case 'S':				/* more user stats */
      setONode(nUserlInit(DSP_NODE_CP, FALSE, Context.snum, FALSE, TRUE));
      break;
    case 'T':				/* team list */
      setONode(nTeamlInit(DSP_NODE_CP, FALSE, Ships[Context.snum].team));
      break;
    case 'u':				/* un-tractor */
      sendCommand(CPCMD_UNTOW, 0);
      break;
    case 'U':				/* user stats */
      setONode(nUserlInit(DSP_NODE_CP, FALSE, Context.snum, FALSE, FALSE));
      break;
    case 'W':				/* war and peace */
      for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
        twar[i] = Ships[Context.snum].war[i];

      state = S_WAR;
      prompting = TRUE;
      prm.preinit = False;
      prm.buf = cbuf;
      prm.buflen = 5;
      prm.pbuf = clbWarPrompt(Context.snum, twar);
      prm.terms = TERMS;
      prm.index = MSG_LIN1;
      prm.buf[0] = EOS;
      setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
      prompting = TRUE;

      break;
    case '-':				/* shields down */
      _doshields( Context.snum, FALSE );
      break;
    case '+':				/* shields up */
      _doshields( Context.snum, TRUE );
      break;
    case '/':				/* player list */
      setONode(nShiplInit(DSP_NODE_CP, FALSE));  /* shipl node */
      break;
    case '?':				/* planet list */
      if (Context.snum > 0 && Context.snum <= MAXSHIPS)
        setONode(nPlanetlInit(DSP_NODE_CP, FALSE, Context.snum, Ships[Context.snum].team));
      else          /* then use user team if user doen't have a ship yet */
        setONode(nPlanetlInit(DSP_NODE_CP, FALSE, Context.snum, Users[Context.unum].team));
      break;
    case TERM_REDRAW:			/* clear all the prompts */
      clrPrompt(MSG_LIN1);
      clrPrompt(MSG_LIN2);
      clrPrompt(MSG_MSG);
      break;
      
    case TERM_NORMAL:		/* Have [RETURN] act like 'I[RETURN]'  */
    case KEY_ENTER:
    case '\n':
      cbuf[0] = EOS;
      _doinfo(cbuf, TERM_NORMAL);
      break;

    case ' ':
      if (SMAP(snum))
        UserConf.DoLocalLRScan = !UserConf.DoLocalLRScan;
      break;

    case TERM_EXTRA:		/* Have [TAB] act like 'i\t' */
      cbuf[0] = EOS;
      _doinfo(cbuf, TERM_EXTRA);
      break;

    case TERM_RELOAD:		/* have server resend current universe */
      sendCommand(CPCMD_RELOAD, 0);
      clog("client: sent CPCMD_RELOAD");
      break;
      
    case -1:			/* really nothing, move along */
      break;

      /* nothing. */
    default:
      mglBeep();
      cp_putmsg( "Type h for help.", MSG_LIN2 );
    }
  
  return;
  
}


void nCPInit(void)
{
  prompting = FALSE;
  state = S_NONE;
  clientFlags = 0;

  /* init timers */
  rftime = glutGet(GLUT_ELAPSED_TIME);
  lastblast = Ships[Context.snum].lastblast;
  lastphase = Ships[Context.snum].lastphase;
  pingPending = FALSE;
  pingStart = 0;

  setNode(&nCPNode);

  return;
}


static int nCPDisplay(dspConfig_t *dsp)
{
  /* Viewer */
  renderViewer(UserConf.doVBG);

  /* Main/Hud */
  renderHud(dostats);

  /* draw the overlay bg if active */
  mglOverlayQuad();

  return NODE_OK;
}  
  
static int nCPIdle(void)
{
  spAck_t *sack;
  int pkttype;
  int now;
  int gnow = glutGet(GLUT_ELAPSED_TIME);
  Unsgn8 buf[PKT_MAXSIZE];
  int difftime = dgrand( Context.msgrand, &now );
  int sockl[2] = {cInfo.sock, cInfo.usock};
  static Unsgn32 iterstart = 0;
  static Unsgn32 pingtime = 0;
  Unsgn32 iternow = clbGetMillis();
  const Unsgn32 iterwait = 50; /* ms */
  const Unsgn32 pingwait = 2000; /* ms (2 seconds) */
  real tdelta = (real)iternow - (real)iterstart;


  if (state == S_DEAD)
    {                           /* transfer to the dead node */
      nDeadInit();
      return NODE_OK;
    }

  while ((pkttype = waitForPacket(PKT_FROMSERVER, sockl, PKT_ANYPKT,
                                  buf, PKT_MAXSIZE, 0, NULL)) > 0)
    {
        switch (pkttype)
          {
            case SP_ACK: 
              sack = (spAck_t *)buf;
                                /* see if it's a ping resp */
              if (sack->code == PERR_PINGRESP)
                {
                  pingPending = FALSE;
                  pingAvgMS = (pingAvgMS + (iternow - pingStart)) / 2;
                  pingStart = 0;
                  continue;
                }
              else
                processPacket(buf);

              break;
          default:
            processPacket(buf);
            break;
          }
    }

  if (pkttype < 0)          /* some error */
    {
      clog("nCPIdle: waitForPacket returned %d", pkttype);
      Ships[Context.snum].status = SS_OFF;
      return NODE_EXIT;
    }

  /* send a ping if it's time */
  if (!pingPending && ((iternow - pingtime) > pingwait))
    {                           /* send a ping request */
      /* only send this if we aren't doing things that this packet would end
         up cancelling... */
      if (state != S_REFITING && state != S_BOMBING && 
          state != S_BEAMING  && state != S_DESTRUCTING &&
          state != S_WARRING && state != S_AUTOPILOT)
        {

          pingtime = iternow;
          pingStart = iternow;
          pingPending = TRUE;
          sendCommand(CPCMD_PING, 0);
        }
    }

  /* drive the local universe */
  if (tdelta > iterwait) 
    {
      clbPlanetDrive(tdelta / 1000.0);
      clbTorpDrive(tdelta / 1000.0);
      iterstart = iternow;
      recordGenTorpLoc();
    }

  if (clientFlags & SPCLNTSTAT_FLAG_KILLED)
    {                           /* we died.  set the state and deal with
                                   it on the next frame */
      state = S_DEAD;
      prompting = FALSE;        /* doesn't really matter */
      return NODE_OK;
    }

  if (state == S_BOMBING && lastServerError)
    {                           /* the server stopped bombing for us */
      sendCommand(CPCMD_BOMB, 0); /* to be sure */
      state = S_NONE;
      prompting = FALSE;
      clrPrompt(MSG_LIN2);
      return NODE_OK;           /* next iter will process the char */
    }

  if (state == S_BEAMING && lastServerError)
    {                           /* the server stopped beaming for us */
      sendCommand(CPCMD_BEAM, 0); /* to be sure */
      state = S_NONE;
      prompting = FALSE;
      clrPrompt(MSG_LIN2);
      return NODE_OK;           /* next iter will process the char */
    }

  if (state == S_REFITING)
    {                           /* done refiting? */
      if (dgrand( entertime, &now ) >= REFIT_GRAND)
        {
          clrPrompt(MSG_LIN1);
          clrPrompt(MSG_LIN2);
          state = S_NONE;
        }
      else
        return NODE_OK;
    }

  if (state == S_WARRING)
    {                           
      if (dgrand( entertime, &now ) >= REARM_GRAND)
        {
          clrPrompt(MSG_LIN1);
          clrPrompt(MSG_LIN2);
          state = S_NONE;
        }
      else
        return NODE_OK;
    }

  nCPInput(0);                   /* handle any queued chars */

  /* check for messages */
  if (Context.msgok)
    {
      if (difftime >= NEWMSG_GRAND)
        if ( getamsg(Context.snum, &Ships[Context.snum].lastmsg))
          {
            rmesg(Context.snum, Ships[Context.snum].lastmsg, MSG_MSG);
            if (Msgs[Ships[Context.snum].lastmsg].msgfrom !=
                Context.snum)
              if (UserConf.MessageBell)
                mglBeep();
            
            Context.msgrand = now;
          }
    }

  if ((gnow - rftime) > (int)((1.0 / (real)Context.updsec) * 1000.0))
    {                           /* record a frame */
      recordUpdateFrame();
      rftime = gnow;
    }

  return NODE_OK;
}

static int nCPInput(int ch)
{
  int cf = ch;                      /* backup of ch for domacros() */
  char c;
  int irv, tmsg, i;
  real tdir;
  real x;
  int snum = Context.snum;

  if ((CQ_CHAR(ch) == 'B' || CQ_CHAR(ch) == 'b') && 
      CQ_MODIFIER(ch) & CQ_KEY_MOD_ALT)
    {
      UserConf.doVBG = !UserConf.doVBG;
      return NODE_OK;
    }

  /* display render/comm stats */
  if ((CQ_CHAR(ch) == 'S' || CQ_CHAR(ch) == 's') && 
      CQ_MODIFIER(ch) & CQ_KEY_MOD_ALT)
    {
      dostats = !dostats;
      return NODE_OK;
    }

  ch = CQ_CHAR(ch) | CQ_FKEY(ch);

  if (ch == 0x1c)
    return NODE_EXIT;                  /* Control-/ (INSTA-QUIT (tm)) */

  if (state == S_REFITING && ch) /* if refitting, just que all chars */
    {
      iBufPutc(ch);
      return NODE_OK;
    }

  if (state == S_WARRING && ch) 
    {
      iBufPutc(ch);
      return NODE_OK;
    }

  if (state == S_BOMBING && ch)
    {                           /* aborting */
      iBufPutc(ch);             /* just que it */
      sendCommand(CPCMD_BOMB, 0);
      state = S_NONE;
      prompting = FALSE;
      cp_putmsg( abt, MSG_LIN1 );
      clrPrompt(MSG_LIN2);
      return NODE_OK;           /* next iter will process the char */
    }

  if (state == S_BEAMING && ch)
    {                           /* aborting */
      iBufPutc(ch);             /* just que it */
      sendCommand(CPCMD_BEAM, 0);
      state = S_NONE;
      prompting = FALSE;
      cp_putmsg( abt, MSG_LIN1 );
      clrPrompt(MSG_LIN2);
      return NODE_OK;           /* next iter will process the char */
    }

  if (ch == 0)
    {                           /* check for queued chars */
      if (iBufCount())
        ch = iBufGetCh();
      else
        return NODE_OK;
    }
  else
    {
      if (iBufCount())
        {
          iBufPutc(ch);                 /* que char */
          ch = iBufGetCh();
        }
    }

  c = CQ_CHAR(ch);              /* strip off everything but the character */

  if (prompting)
    {
      irv = prmProcInput(&prm, ch);

      switch (state)
        {
        case S_COURSE:
          if (irv > 0)
            {
              _docourse(prm.buf, ch);
              prompting = FALSE;
              state = S_NONE;
            }
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;

        case S_DOINFO:
          if (irv > 0)
            {
              _doinfo(prm.buf, ch);
              prompting = FALSE;
              state = S_NONE;
            }
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;

        case S_TARGET:
          if (irv > 0)
            {
                if (_gettarget(prm.buf, 
                               (desttarg == T_PHASER) ? lastphase : lastblast,
                               &tdir, ch))
                  {
                    lastblast = tdir; /* we set both of them */
                    lastphase = tdir;
                    switch (desttarg)
                      {
                      case T_PHASER:
                        _dophase(tdir);
                        break;
                      case T_TORP:
                        _dotorp(tdir, 1);
                        break;
                      case T_BURST:
                        _dotorp(tdir, 3);
                        break;
                      }
                  }
                else
                  cp_putmsg( "Invalid targeting information.", MSG_LIN1 );

                prompting = FALSE;
                state = S_NONE;
            }
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;

        case  S_CLOAK:
          if (irv > 0)
            {
              _docloak(prm.buf, ch);
              prompting = FALSE;
              state = S_NONE;
            }

          break;

        case  S_ALLOC:
          if (irv > 0)
            {
              _doalloc(prm.buf, ch);
              prompting = FALSE;
              state = S_NONE;
            }
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;

        case  S_DISTRESS:
          if (irv > 0)
            {
              _dodistress(prm.buf, ch);
              prompting = FALSE;
              state = S_NONE;
            }

          break;

        case  S_COUP:
          if (irv > 0)
            {
              if (ch == TERM_EXTRA)
                {
                  cp_putmsg( "Attempting coup...", MSG_LIN1 );
                  sendCommand(CPCMD_COUP, 0);
                }
              else
                cp_putmsg( abt, MSG_LIN1 );

              prompting = FALSE;
              state = S_NONE;
            }

          break;

        case  S_TOW:
          if (irv > 0)
            {
              _dotow(prm.buf, ch);
              prompting = FALSE;
              state = S_NONE;
            }
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;

        case  S_DOAUTOPILOT:
          if (irv > 0)
            {
              if (ch == TERM_EXTRA)
                {
                  sendCommand(CPCMD_AUTOPILOT, 1);
                  state = S_AUTOPILOT;
                }
              else
                {
                  clrPrompt(MSG_LIN1);
                  prompting = FALSE;
                  state = S_NONE;
                }
            }

          break;

        case S_AUTOPILOT:
          if (ch == TERM_ABORT)
            {
              sendCommand(CPCMD_AUTOPILOT, 0);
              clrPrompt(MSG_LIN1);
              prompting = FALSE;
              state = S_NONE;
            }
          else
            cp_putmsg("Press ESCAPE to abort autopilot.", MSG_LIN1);

          break;

        case  S_MSGTO:
          if (irv > 0)
            _domsgto(prm.buf, ch,  UserConf.Terse); /* will set state appropriately */
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;

        case  S_MSG:
          if (irv != 0)
            _domsg(prm.buf, ch, irv); /* will set state appropriately */
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;

        case S_DESTRUCT:
          if (ch == TERM_EXTRA)
            {
              sendCommand(CPCMD_DESTRUCT, 1); /* blow yourself up */
              state = S_DESTRUCTING;
            }
          else
            {                   /* chicken */
              state = S_NONE;
              prompting = FALSE;
              clrPrompt(MSG_LIN1);
              clrPrompt(MSG_LIN2);
            }

          break;

        case S_DESTRUCTING:
          if (ch == TERM_ABORT)
            {
              sendCommand(CPCMD_DESTRUCT, 0); /* just kidding */
              state = S_NONE;
              prompting = FALSE;
              clrPrompt(MSG_LIN1);
              clrPrompt(MSG_LIN2);
              cp_putmsg( "Self destruct has been cancelled.", MSG_LIN1 );
            }
          else
            {                   /* chicken */
              clrPrompt(MSG_LIN1);
              clrPrompt(MSG_LIN2);
              prm.buf[0] = EOS;
	      cp_putmsg( "Press ESCAPE to abort self destruct.", MSG_LIN1 );
	      mglBeep();
            }
          
          break;

        case S_BOMB:
          if (ch == TERM_EXTRA)
            {
              sendCommand(CPCMD_BOMB, 1);   /* start the bombing */
              state = S_BOMBING;
              prompting = FALSE;
              clrPrompt(MSG_LIN1);
            }
          else
            {                   /* weak human */
              state = S_NONE;
              prompting = FALSE;
              cp_putmsg( abt, MSG_LIN1 );
              clrPrompt(MSG_LIN2);
            }

          break;

        case S_BEAMDIR:
          i = 0;
          switch (ch)
            {
            case 'u':
            case 'U':
              dirup = TRUE;
              i = TRUE;
              break;
            case 'd':
            case 'D':
            case TERM_EXTRA:
              dirup = FALSE;
              i = TRUE;
              break;
            default:
              cp_putmsg( abt, MSG_LIN1 );
              state = S_NONE;
              prompting = FALSE;
              return NODE_OK;
            }

          if (i)
            {                   /* time to ask how many */
              if ( dirup )
                beamax = upmax;
              else
                beamax = downmax;

              sprintf( pbuf, "Beam %s [1-%d] ", (dirup) ? "up" : "down", 
                       beamax );
              state = S_BEAMNUM;
              prm.preinit = False;
              prm.buf = cbuf;
              prm.buflen = 10;
              prm.pbuf = pbuf;
              prm.terms = TERMS;
              prm.index = MSG_LIN1;
              prm.buf[0] = EOS;
              setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
              prompting = TRUE;
            }

          break;


        case S_BEAMNUM:
          if (irv > 0)
            _dobeam(prm.buf, ch); /* will set state appropriately */
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;
          

        case S_WAR:
          if (irv > 0)
            {
              if (ch == TERM_ABORT || ch == TERM_NORMAL)
                {
                  state = S_NONE;
                  prompting = FALSE;
                  clrPrompt(MSG_LIN1);
                  clrPrompt(MSG_LIN2);
                  return NODE_OK;
                }

              if (ch == TERM_EXTRA) /* accepted */
                {
                  int dowait = FALSE;
                  Unsgn16 cwar; 

                  cwar = 0;
                  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                    {
                      if ( twar[i] && ! Ships[snum].war[i] )
                        dowait = TRUE;

                      if (twar[i])
                        cwar |= (1 << i);

                      /* we'll let it happen locally as well... */
                      Users[Ships[Context.snum].unum].war[i] = twar[i];
                      Ships[Context.snum].war[i] = twar[i];
                    }

                  clrPrompt(MSG_LIN1);
                  clrPrompt(MSG_LIN2);

                  if (dowait)
                    {
                      state = S_WARRING;
                      prompting = FALSE;
                      cp_putmsg(
                                "Reprogramming the battle computer, please stand by...",
                                MSG_LIN2 );
                      
                      grand( &entertime ); /* gotta wait */
                    }
                  else
                    {
                      state = S_NONE;
                      prompting = FALSE;
                    }

                  sendCommand(CPCMD_SETWAR, (Unsgn16)cwar);
                  return NODE_OK;
                }

            }
          else
            {
              prm.buf[0] = EOS;
              for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
                if ( ch == (char)tolower( Teams[i].teamchar ) )
                  {
                    if ( ! twar[i] || ! Ships[Context.snum].rwar[i] )
                      twar[i] = ! twar[i];
                    prm.pbuf = clbWarPrompt(Context.snum, twar);
                    setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
                  }
            }

          break;

        case S_PSEUDO:
          if (irv > 0)
            {
              if (ch != TERM_ABORT && prm.buf[0] != EOS)
                sendSetName(prm.buf);
              prompting = FALSE;
              state = S_NONE;
              clrPrompt(MSG_LIN1);
              clrPrompt(MSG_LIN2);
            }
          else
            setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, CyanColor);

          break;

        case  S_REFIT:
          if (irv > 0)
            {
              switch (ch)
                {
                case TERM_ABORT: /* cancelled */
                  state = S_NONE;
                  prompting = FALSE;
                  clrPrompt(MSG_LIN1);
                  clrPrompt(MSG_LIN2);

                  break;

                case TERM_NORMAL:
                  clrPrompt(MSG_LIN1);
                  clrPrompt(MSG_LIN2);
                  prm.pbuf = "Refitting ship...";
                  setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);
                  sendCommand(CPCMD_REFIT, (Unsgn16)refitst);
                  prompting = FALSE;
                  grand( &entertime );
                  state = S_REFITING;

                  break;
                  
                case TERM_EXTRA:
                  refitst = modp1( refitst + 1, MAXNUMSHIPTYPES );
                  sprintf(pbuf, "Refit ship type: %s", 
                          ShipTypes[refitst].name);
                  prm.buf[0] = EOS;
                  setPrompt(prm.index, prm.pbuf, NoColor, prm.buf, NoColor);

                  break;

                }

              break;

            }
        }
    }
  else
    {                           /* !prompting */

      if (state == S_REVIEW)
        {                       /* reviewing messages */
          switch (ch)
            {
	    case ' ':
	    case '<':
	    case CQ_KEY_UP:
	    case CQ_KEY_LEFT:
	      tmsg = modp1( msg - 1, MAXMESSAGES );
	      while(!clbCanRead( snum, tmsg ) && tmsg != lastone)
		{
		  tmsg = modp1( tmsg - 1, MAXMESSAGES );
		}
	      if (tmsg == lastone)
		{
		  mglBeep();
		}
	      else
		msg = tmsg;
	      break;
	    case '>':
	    case CQ_KEY_DOWN:
	    case CQ_KEY_RIGHT:
	      tmsg =  modp1( msg + 1, MAXMESSAGES );
	      while(!clbCanRead( snum, tmsg ) && tmsg != lstmsg + 1 )
		{
		  tmsg = modp1( tmsg + 1, MAXMESSAGES );
		}
	      if (tmsg == (lstmsg + 1))
		{
		  mglBeep();
		}
	      else
		msg = tmsg;
	      
	      break;
	    default:
              clrPrompt(MSG_LIN1);
              clrPrompt(MSG_LIN2);
              state = S_NONE;
              return NODE_OK;
	      break;
	    }

          if (!_review())
            {
              state = S_NONE;
              return NODE_OK;
            }
        }
      else
        {
          clrPrompt(MSG_LIN1);
          clrPrompt(MSG_LIN2);

          if (CQ_FKEY(cf) && !_KPAngle(cf, &x))
            {                           /* handle macros */
              if (DoMacro(_xlateFKey(cf)))
                {
                  while (iBufCount())
                    nCPInput(0); /* recursion warning */
                  return NODE_OK;
                }
              else
                return NODE_OK;
            }
          else
            command(ch);
       }
    }

  return NODE_OK;
}

