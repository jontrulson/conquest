#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/*                                 C O N Q L B */
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

#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"
#include "color.h"

#define GREEN_ALERT 0
#define YELLOW_ALERT 1
#define RED_ALERT 2

/* Global to this module */

static int AlertLevel = GREEN_ALERT;
static real LastPhasDist = PHASER_DIST;

/*##  chalkup - perform kills accoutinng */
/*  SYNOPSIS */
/*    int snum */
/*    chalkup( snum ) */
/*  Note: This routines ASSUMES you have the common locked before you it. */
void chalkup( int snum )
{
  int i, unum, team;
  real x, w, l, m;
  
  unum = suser[snum];
  team = steam[snum];
  
  /* Update wins. */
  ustats[unum][USTAT_WINS] = ustats[unum][USTAT_WINS] + ifix(skills[snum]);
  tstats[team][TSTAT_WINS] = tstats[team][TSTAT_WINS] + ifix(skills[snum]);
  
  /* Update max kills. */
  i = ifix( skills[snum] );
  if ( i > ustats[unum][USTAT_MAXKILLS] )
    ustats[unum][USTAT_MAXKILLS] = i;
  
  /* Update rating. */
  l = ustats[unum][USTAT_LOSSES];
  if ( l == 0 )
    l = 1;
  w = ustats[unum][USTAT_WINS];
  m = ustats[unum][USTAT_MAXKILLS];
  urating[unum] = ( w / l ) + ( m / 4.0 );
  x = w - l;
  if ( x >= 0.0 )
    urating[unum] = urating[unum] + powf((real) x, (real) ( 1.0 / 3.0 ));
  else
    urating[unum] = urating[unum] - powf((real) -x, (real) ( 1.0 / 3.0 ));
  
  return;
  
}


/*##  cloak - attempt to engage the cloaking device */
/*  SYNOPSIS */
/*    int didit, cloak */
/*    int snum */
/*    didit = cloak( snum ) */
int cloak( int snum )
{
  srmode[snum] = FALSE;
  if ( ! usefuel( snum, CLOAK_ON_FUEL, FALSE ) )
    return ( FALSE );
  scloaked[snum] = TRUE;
  return ( TRUE );
  
}


/*##  damage - damage a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    real dam */
/*    damage( snum, dam, kb ) */
void damage( int snum, real dam, int kb )
{
  real mw;
  
  sdamage[snum] = sdamage[snum] + dam;
  if ( sdamage[snum] >= 100.0 )
    killship( snum, kb );
  else
    {
      mw = maxwarp( snum );
      sdwarp[snum] = min( sdwarp[snum], mw );
    }
  
  return;
  
}

void do_bottomborder(void)
{
  int lin;

  lin = DISPLAY_LINS + 1;

  cdline( lin, 1, lin, cmaxcol );
  mvaddch(lin - 1, STAT_COLS - 1, ACS_BTEE);
}

void do_border(void)
{
  int lin;
  
  lin = DISPLAY_LINS + 1;
  
  
  cdline( 1, STAT_COLS, lin, STAT_COLS );
  /*  cdline( lin, 1, lin, cmaxcol );
  mvaddch(lin - 1, STAT_COLS - 1, ACS_BTEE);
  */
  do_bottomborder();
  
  return;
}

int alertcolor(int alert)
{
  int theattrib;
  
  switch (alert)
    {
    case GREEN_ALERT:
      if (HAS_COLORS)
	theattrib = COLOR_PAIR(COL_GREENBLACK);
      else
	theattrib = 0;
      break;
    case YELLOW_ALERT:
      if (HAS_COLORS)
	theattrib = COLOR_PAIR(COL_YELLOWBLACK);
      else
	theattrib = A_BOLD;
      break;
    case RED_ALERT:
      if (HAS_COLORS)
	theattrib = COLOR_PAIR(COL_REDBLACK);
      else
	theattrib = A_REVERSE;
      break;
    default:
      clog("alertcolor(): invalid alert level: %d", alert);
      break;
    }

  return(theattrib);
}

void draw_alertborder(int alert)
{
  
  attrset(alertcolor(alert));
  do_border();
  attrset(0);
  
  /*  cdrefresh(TRUE); */
  
  return;
}



/*##  detonate - blow up a torpedo (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, tnum */
/*    detonate( snum, tnum ) */
void detonate( int snum, int tnum )
{
  
  PVLOCK(lockword);
  if ( tstatus[snum][tnum] == TS_LIVE )
    tstatus[snum][tnum] = TS_DETONATE;
  PVUNLOCK(lockword);
  
  return;
  
}


/*##  display - do one update of a ships screen */
/*  SYNOPSIS */
/*    int snum */
/*    display( snum ) */
void display( int snum )
{
  int i, j, k, l, m, idx, lin, col, datacol, minenemy, minsenemy;
  int linofs[8] = {0, -1, -1, -1, 0, 1, 1, 1};
  int colofs[8] = {1, 1, 0, -1, -1, -1, 0, 1};
  int outattr;
  static int OldAlert = 0;
  string dirch="-/|\\-/|\\";
  char ch, buf[MSGMAXLINE];
  int godlike;
  int dobeep, lsmap;
  int doalertb = FALSE;
  int palertcol;
  real x, scale, cenx, ceny, dis, mindis, minsdis, fl, cd, sd;
  static real zzskills, zzswarp;
  static char zzbuf[MSGMAXLINE];
  static int zzsshields, zzcshields, zzshead, zzsfuel, zzcfuel;
  static int zzsweapons, zzsengines, zzsdamage, zzcdamage, zzsarmies;
  static int zzsetemp, zzswtemp, zzctemp, zzstowedby, zzssdfuse;
  static real prevsh = 0.0 , prevdam = 100.0 ;
  
  static int ShieldAttrib = 0;
  static int ShieldLAttrib = 0;
  static int FuelAttrib = 0;
  static int WeapAttrib = 0;
  static int EngAttrib = 0;
  static int DamageAttrib = 0;
  
  static time_t savtime = 0;
  time_t thetime;
  
  if ( credraw )
    {
      cdclear();
      lin = DISPLAY_LINS + 1;
      draw_alertborder(AlertLevel);
    }
  else
    cdclra( 1, STAT_COLS  + 1, DISPLAY_LINS, cmaxcol + 1 );
  
  dobeep = FALSE;
  mindis = 1.0e6;
  minsdis = 1.0e6;
  minenemy = 0;
  minsenemy = 0;
  lsmap = smap[snum];
  
  godlike = ( snum < 1 || snum > MAXSHIPS );
  
  if ( lsmap )
    {
      scale = MAP_FAC;
      
      if (sysconf_DoLocalLRScan)
	{
	  cenx = sx[snum];
	  ceny = sy[snum];
	}
      else
	{
	  cenx = 0.0;
	  ceny = 0.0;
	}
    }
  else
    {
      scale = SCALE_FAC;
      cenx = sx[snum];
      ceny = sy[snum];
    }
  
  if ( *closed )
    cdputs( "CLOSED", 1, STAT_COLS+(cmaxcol-STAT_COLS-6)/2+1 );
  else if ( srobot[snum] )
    {
      if (*externrobots == TRUE)
	cdputs( "ROBOT (external)", 1, STAT_COLS+(cmaxcol-STAT_COLS-16)/2+1);
      else
	cdputs( "ROBOT", 1, STAT_COLS+(cmaxcol-STAT_COLS-5)/2+1);
    }
  
  /* Display the planets and suns. */
  for ( i = NUMPLANETS; i > 0; i = i - 1 )
    {
      if ( ! preal[i] )
	continue; /*next;*/
      if ( ! cvtcoords( cenx, ceny, px[i], py[i], scale, &lin, &col ) )
	continue; /* next;*/

      palertcol = 0;
				/* determine alertlevel for object */
      if (spwar( snum, i ) && pscanned[i][steam[snum]])
	{
	  palertcol = RedLevelColor;
	}
      else if (pteam[i] == steam[snum] && !selfwar(snum))
	{
	  palertcol = GreenLevelColor;
	}
      else
	palertcol = YellowLevelColor;

				/* suns are always yellow level material */
      if (ptype[i] == PLANET_SUN)
	palertcol = YellowLevelColor;

      if ( lsmap )
	{
	  /* Strategic map. */
	  /* Can't see moons. */
	  if ( ptype[i] == PLANET_MOON )
	    continue; /* next;*/
	  /* If it's a sun or we any planet we haven't scanned... */
	  if ( ptype[i] == PLANET_SUN || ! pscanned[i][steam[snum]] )
	    {
	      if (ptype[i] == PLANET_SUN)
		attrset(RedLevelColor); /* suns have a red core */
	      else
		attrset(palertcol);

	      cdput( chrplanets[ptype[i]], lin, col );
	      attrset(0);
	    }
	  else
	    {
	      /* Pick a planet owner character. */
	      if ( parmies[i] <= 0 || pteam[i] < 0 || pteam[i] >= NUMTEAMS )
		ch = '-';
	      else
		ch = chrtorps[pteam[i]];
	      
	      /* Display the planet; either it's numeric or it's not. */
	      if ( soption[snum][OPT_NUMERICMAP] )
		{
		  sprintf( buf, "%d", parmies[i]);
		  l = strlen(buf);
		  
		  m = (col + 2 - (l + 2));

		  if (m > STAT_COLS)
		    {
		      attrset(palertcol);
		      cdput( ch, lin, m++);
		      attrset(0);

		      attrset(InfoColor);
		      cdputs( buf, lin, m);
		      m += l;
		      attrset(0);

		      attrset(palertcol);
                      cdput( ch, lin, m);
		      attrset(0);
		    }

		}
	      else if ( pscanned[i][steam[snum]] )
		{
		  l = 3;		/* strlen */
		  m = (col + 2 - l);

		  if (m > STAT_COLS)
		    {
                      attrset(palertcol);
                      cdput( ch, lin, m++);
                      attrset(0);

                      attrset(InfoColor);
                      cdput( chrplanets[ptype[i]], lin, m++);
                      attrset(0);

                      attrset(palertcol);
                      cdput( ch, lin, m++);
		      attrset(0);
		    }
		}
	    }
	  
	  /* If necessary, display the planet name. */
	  if ( soption[snum][OPT_PLANETNAMES] )
	    {
	      sprintf(buf, "%c%c%c", pname[i][0], pname[i][1], pname[i][2]);
	      attrset(palertcol);
	      cdputs( buf, lin, col+2 );
	      attrset(0);
	    }
	}
      else
	{
	  /* Tactical map. */
	  attrset(palertcol);
	  puthing( ptype[i], lin, col );
	  attrset(0);

	  if (col - 3 >= STAT_COLS - 1)
	    {
	      if (lin <= DISPLAY_LINS && lin > 0 )
		{
		  if (! pscanned[i][steam[snum]])
		    attrset(palertcol);	/* default to yellow for unscanned */
		  else
		    attrset(InfoColor);	/* scanned (known) value */

		  if (ptype[i] == PLANET_SUN)
		    attrset(RedLevelColor); /* suns have a red core */

		  cdput( chrplanets[ptype[i]], lin, col + 1);
		  attrset(0);
		}
	      if ( soption[snum][OPT_PLANETNAMES] )
		if ( (lin + 1 <= DISPLAY_LINS) && col + 1< cdcols(0) )
		  {
		    attrset(palertcol);
		    cdputs( pname[i], lin + 1, col + 2 );
		    attrset(0);
		  }
	    }
	}
    }
  
  /* Display the planet eater. */
  if ( *dstatus == DS_LIVE )
    if ( ! lsmap )
      if ( cvtcoords( cenx, ceny, *dx, *dy, scale, &lin, &col ) )
	{
	  dobeep = TRUE;
	  sd = sind(*dhead);
	  cd = cosd(*dhead);
	  /* Draw the body. */
	  attrset(COLOR_PAIR(COL_BLUEBLACK));
	  for ( fl = -DOOMSDAY_LENGTH/2.0;
	       fl < DOOMSDAY_LENGTH/2.0;
	       fl = fl + 50.0 )
	    if ( cvtcoords( cenx, ceny, *dx+fl*cd, *dy+fl*sd, scale, &lin, &col ) )
	      cdput( '#', lin, col );
	  attrset(0);
	  /* Draw the head. */
	  if ( cvtcoords( cenx, ceny, *dx+DOOMSDAY_LENGTH/2.0*cd,
			 *dy+DOOMSDAY_LENGTH/2.0*sd,
			 scale, &lin, &col ) )
	    {
	      attrset(RedLevelColor);
	      cdput( '*', lin, col );
	      attrset(0);
	    }
	}
  
  /* Display phaser graphics. */
  if ( ! lsmap && spfuse[snum] > 0 )
    if ( soption[snum][ OPT_PHASERGRAPHICS] )
      {
	sd = sind(slastphase[snum]);
	cd = cosd(slastphase[snum]);
	ch = dirch[mod( (round( slastphase[snum] + 22.5 ) / 45), 7 )];
	attrset(InfoColor);
	for ( fl = 0; fl <= LastPhasDist; fl = fl + 50.0 )
	  if ( cvtcoords( cenx, ceny,
			 sx[snum]+fl*cd, sy[snum]+fl*sd,
			 scale, &lin, &col ) )
	    cdput( ch, lin, col );
	attrset(0);
      }
  
  /* Display the ships. */
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] != SS_OFF )
      {
	if (sysconf_DoLRTorpScan)
	  {
	    /* Display the torps on a LR scan if it's a friend. */
	    if (lsmap)
	      {
		if (swar[snum][steam[i]] == FALSE &&
		    swar[i][steam[snum]] == FALSE)
		  {
		    if (i == snum) /* if it's your torps - */
		      attrset(A_BOLD);
		    else
		      attrset(YellowLevelColor);

		    for ( j = 0; j < MAXTORPS; j = j + 1 )
		      if ( tstatus[i][j] == TS_LIVE 
			  || tstatus[i][j] == TS_DETONATE )
			if ( cvtcoords( cenx, ceny, tx[i][j], ty[i][j],
				       scale, &lin, &col ) )
			  cdput( chrtorps[steam[i]], lin, col );
		    attrset(0);
		  }
	      }
	  }
	
	if ( ! lsmap )
	  {
	    /* First display exploding torps. */
	    if ( soption[snum][ OPT_EXPLOSIONS] )
	      for ( j = 0; j < MAXTORPS; j = j + 1 )
		if ( tstatus[i][j] == TS_FIREBALL )
		  if ( cvtcoords( cenx, ceny, tx[i][j], ty[i][j],
				 scale, &lin, &col) )
		    puthing( THING_EXPLOSION, lin, col );
	    /* Now display the live torps. */

	    if (i == snum) /* if it's your torps - */
	      attrset(0);
	    else if (i != snum && satwar(i, snum))
	      attrset(RedLevelColor);
	    else
	      attrset(GreenLevelColor);
	    
	    for ( j = 0; j < MAXTORPS; j = j + 1 )
	      if ( tstatus[i][j] == TS_LIVE || tstatus[i][j] == TS_DETONATE )
		if ( cvtcoords( cenx, ceny, tx[i][j], ty[i][j],
			       scale, &lin, &col ) )
		  cdput( chrtorps[steam[i]], lin, col );
	    attrset(0);
	  }
	/* Display the ships. */
	if ( sstatus[i] == SS_LIVE )
	  {
	    /* It's alive. */
	    dis = (real) dist(sx[snum], sy[snum], sx[i], sy[i] );
	    
	    /* Here's where ship to ship accurate information is "gathered". */
	    if ( dis < ACCINFO_DIST && ! scloaked[i] && ! selfwar( snum ) )
	      sscanned[i][steam[snum]] = SCANNED_FUSE;
	    
	    /* Check for nearest enemy and nearest scanned enemy. */
	    if ( satwar( i, snum ) )
	      if ( i != snum )
		{
		  
#ifdef WARP0CLOAK
		  /* CLOAKHACK - JET - 1/6/94 */
		  /* we want any cloaked ship at warp 0.0 */
		  /* to be invisible. */
		  if (scloaked[i] && swarp[i] == 0.0)
		    {
		      /* skip to next ship. this one isn't here */
		      /* ;-} */
		      continue;	/* RESTART FOR */
		    }
#endif /* WARP0CLOAK */
		  
		  if ( dis < mindis )
		    {
		      /* New nearest enemy. */
		      mindis = dis;
		      minenemy = i;
		    }
		  if ( dis < minsdis )
		    if ( ! selfwar( snum ) )
		      if ( sscanned[i][steam[snum]] > 0 )
			{
			  /* New nearest scanned enemy. */
			  minsdis = dis;
			  minsenemy = i;
			}

		}
	    
	    /* There is potential for un-cloaked ships and ourselves. */
	    if ( ! scloaked[i] || i == snum )
	      {
		/* ... especially if he's in the bounds of our current */
		/*  display (either tactical or strategic map) */
		if ( cvtcoords( cenx, ceny, sx[i], sy[i],
			       scale, &lin, &col ) )
		  {
		    /* He's on the screen. */
		    /* We can see him if one of the following is true: */
#
		    /*  - We are not looking at our strategic map */
		    /*  - We're mutually at peace */
		    /*  - Our team has scanned him and we're not self-war */
		    /*  - He's within accurate scanning range */
		    
		    if ( ( ! lsmap ) ||
			( ! satwar(i, snum) ) ||
			( sscanned[i][steam[snum]] && ! selfwar(snum) ) ||
			( dis <= ACCINFO_DIST ) )
		      {
			if ( ( i == snum ) && scloaked[snum] )
			  ch = CHAR_CLOAKED;
			else
			  ch = chrteams[steam[i]];
			
				/* determine color */
			if (i == snum)    /* it's ours */
			  attrset(A_BOLD);
			else if (satwar(i, snum)) /* we're at war with it */
			  attrset(RedLevelColor);
			else if (steam[i] == steam[snum] && !selfwar(snum))
			  attrset(GreenLevelColor); /* it's a team ship */
			else
			  attrset(YellowLevelColor);
			    
				 
			cdput( ch, lin, col );
			attrset(0); attrset(A_BOLD);
			cdputn( i, 0, lin, col + 2 );
			attrset(0);

			/*			    idx = modp1( round( shead[i] + 22.5 ) / 45 + 1, 8 );
			 */
			/*			    idx = mod( round( (shead[i] + 22.5) / 45), 7 );*/
			
			idx = (int)mod( round((((real)shead[i] + 22.5) / 45.0) + 0.5) - 1, 8);
			/* JET 9/28/94 */
			/* Very strange -mod keep returning 8 = 8 % 8*/
			/* which aint right... mod seems to behave */
			/* itself elsewhere... anyway, a kludge: */
			idx = ((idx == 8) ? 0 : idx);
			
			/*			    cerror("idx = %d", idx);*/
			
			j = lin+linofs[idx];
			k = col+colofs[idx];

			attrset(InfoColor);
			if ( j >= 0 && j < DISPLAY_LINS && 
			    k > STAT_COLS && k < cmaxcol )
			  cdput( dirch[idx], j, k );
			attrset(0);
		      }
		  }
		if ( snum == i )
		  {
		    /* If it's our ship and we're off the screen, fake it. */
		    if ( lin < 1 )
		      lin = 1;
		    else
		      lin = min( lin, DISPLAY_LINS );
		    if ( col < STAT_COLS + 1 )
		      col = STAT_COLS + 1;
		    else
		      col = min( col, cmaxcol );
		    cdmove( lin, col );
		  }
	      }
	  } /* it's alive */
      } /* for each ship */
  
  /* Construct alert status line. */
  if ( credraw )
    zzbuf[0] = EOS;
  buf[0] = EOS;

  if ( minenemy != 0 || stalert[snum] )
    {
      if ( mindis <= PHASER_DIST )
	{
	  /* Nearest enemy is very close. */
	  outattr = COLOR_PAIR(COL_REDBLACK);
	  AlertLevel = RED_ALERT;
	  c_strcpy( "RED ALERT ", buf );
	  dobeep = TRUE;
	}
      else if ( mindis < ALERT_DIST )
	{
	  /* Nearest enemy is close. */
	  outattr = COLOR_PAIR(COL_REDBLACK);
	  AlertLevel = RED_ALERT;
	  c_strcpy( "Alert ", buf );
	  dobeep = TRUE;
	}
      else if ( stalert[snum] )
	{
	  /* Nearby torpedos. */
	  outattr = COLOR_PAIR(COL_YELLOWBLACK);
	  AlertLevel = YELLOW_ALERT;
	  c_strcpy( "Torp alert", buf );
	  minenemy = 0;			/* disable nearby enemy code */
	  dobeep = TRUE;
	}
      else if ( mindis < YELLOW_DIST )
	{
	  /* Near an enemy. */
	  outattr = COLOR_PAIR(COL_YELLOWBLACK);
	  AlertLevel = YELLOW_ALERT;
	  c_strcpy( "Yellow alert ", buf );
	}
      else if ( minsenemy != 0 )
	{
	  /* An enemy near one of our ships or planets. */
	  outattr = COLOR_PAIR(COL_YELLOWBLACK);
	  minenemy = minsenemy;		/* for cloaking code below */
	  AlertLevel = YELLOW_ALERT;
	  c_strcpy( "Proximity Alert ", buf );
	}
      else
	{
	  outattr = COLOR_PAIR(COL_GREENBLACK);
	  AlertLevel = GREEN_ALERT;
	  minenemy = 0;
	}
      
      if ( minenemy != 0 )
	{
	  appship( minenemy, buf );
	  if ( scloaked[minenemy] )
	    appstr( " (CLOAKED)", buf );
	}
    }
  else
    AlertLevel = GREEN_ALERT;
  
  if (OldAlert != AlertLevel)
    {
/*      doalertb = TRUE;*/
      draw_alertborder(AlertLevel);
      OldAlert = AlertLevel;
    }
  
  if ( strcmp( buf, zzbuf ) != 0 )
    {
      lin = DISPLAY_LINS + 1;
      attrset(alertcolor(AlertLevel));
      do_bottomborder();
      attrset(0);

      if ( buf[0] != EOS )
	{
	  col = (cmaxcol-STAT_COLS-strlen(buf))/2+STAT_COLS+1;
	  if (HAS_COLORS)
	    {
	      attrset(outattr);
	    }
	  else
	    {
	      if (outattr == COLOR_PAIR(COL_REDBLACK))
		attrset(A_BOLD | A_BLINK);
	      else if (outattr == COLOR_PAIR(COL_YELLOWBLACK))
		attrset(A_BOLD);
	    }
	  cdputs( buf, DISPLAY_LINS+1, col );
	  attrset(0);
	}
      c_strcpy( buf, zzbuf );
    }
  
  /* Build and display the status info as necessary. */
  lin = 1;
  col = 1;
  datacol = col + 14;
  
  /* Shields. */
  if ( sshields[snum] < prevsh )
    dobeep = TRUE;
  prevsh = sshields[snum];
  
  if ( credraw )
    {
      zzsshields = -9;
      zzcshields = ' ';
    }
  i = round( sshields[snum] );
  if ( ! sshup[snum] || srmode[snum] )
    i = -1;
  if ( i != zzsshields || i == -1)
    {
      cdclra( lin, datacol, lin, STAT_COLS-1 );
      if ( i == -1 )
	{
	  if (AlertLevel == YELLOW_ALERT) 
	    attrset(YellowLevelColor);
	  else if (AlertLevel == RED_ALERT)
	    attrset(RedLevelColor | A_BLINK);
	  else
	    attrset(GreenLevelColor);
	  
	  cdputs( "DOWN", lin, datacol );
	  attrset(0);
	}
      
      else
	{
	  if (i >= 0 && i <= 50)
	    ShieldAttrib = RedLevelColor;
	  else if (i >=51 && i <=80)
	    ShieldAttrib = YellowLevelColor;
	  else if (i >= 81)
	    ShieldAttrib = GreenLevelColor;
	  
	  sprintf( buf, "%d", i );
	  attrset(ShieldAttrib);
	  cdputs( buf, lin, datacol );
	  attrset(0);
	}
      zzsshields = i;
    }
  
  if ( i < 60 )
    j = 'S';
  else
    j = 's';
  if ( j != zzcshields )
    {
      attrset(LabelColor);
      if ( j == 'S' )
	cdputs( "SHIELDS =", lin, col );
      else
	cdputs( "shields =", lin, col );
      attrset(0);
      zzcshields = j;
    }
  
  /* Kills. */
  lin = lin + 2;
  if ( credraw )
    {
      attrset(LabelColor);
      cdputs( "kills =", lin, col );
      attrset(0);
      zzskills = -20.0;
    }
  x = (skills[snum] + sstrkills[snum]);
  if ( x != zzskills )
    {
      cdclra( lin, datacol, lin, STAT_COLS-1 );
      sprintf( buf, "%0.1f", oneplace(x) );
      
      attrset(InfoColor);
      cdputs( buf, lin, datacol );
      attrset(0);
      
      zzskills = x;
    }
  
  /* Warp. */
  lin = lin + 2;
  if ( credraw )
    {
      attrset(LabelColor);
      cdputs( "warp =", lin, col );
      attrset(0);
      zzswarp = 92.0;			/* "Warp 92 Mr. Sulu." */
    }
  x = swarp[snum];
  if ( x != zzswarp )
    {
      cdclra( lin, datacol, lin, STAT_COLS-1 );
      
      attrset(InfoColor);
      if ( x >= 0 )
	{
	  sprintf( buf, "%.1f", x );
	  cdputs( buf, lin, datacol );
	}
      else
	cdput( 'o', lin, datacol );
      attrset(0);
      
      zzswarp = x;
    }
  
  /* Heading. */
  lin = lin + 2;
  if ( credraw )
    {
      attrset(LabelColor);
      cdputs( "heading =", lin, col );
      attrset(0);
      zzshead = 999;
    }
  i = slock[snum];
  if ( i >= 0 || swarp[snum] < 0.0)
    i = round( shead[snum] );
  if ( -i != zzshead)
    {
      cdclra( lin, datacol, lin, STAT_COLS-1 );
      
      attrset(InfoColor);
      if ( -i > 0 && -i <= NUMPLANETS)
	sprintf( buf, "%.3s", pname[-i] );
      else
	sprintf( buf, "%d", i );
      cdputs( buf, lin, datacol );
      attrset(0);
      zzshead = i;
    }
  
  /* Fuel. */
  lin = lin + 2;
  if ( credraw )
    {
      zzsfuel = -99;
      zzcfuel = ' ';
    }
  i = round( sfuel[snum] );
  if ( i != zzsfuel )
    {
      if (i >= 0 && i <= 200)
	FuelAttrib = RedLevelColor;
      else if (i >=201 && i <=500)
	FuelAttrib = YellowLevelColor;
      else if (i >= 501)
	FuelAttrib = GreenLevelColor;
      
      cdclra( lin, datacol, lin, STAT_COLS-1 );
      sprintf( buf, "%d", i );
      
      attrset(FuelAttrib);
      cdputs( buf, lin, datacol );
      attrset(0);
      
      zzsfuel = i;
    }
  
  if ( i < 200 )
    j = 'F';
  else
    j = 'f';
  if ( j != zzcfuel )
    {
      attrset(LabelColor);
      if ( j == 'F' )
	cdputs( "FUEL =", lin, col );
      else if ( j == 'f' )
	cdputs( "fuel =", lin, col );
      attrset(0);
      
      zzcfuel = j;
    }
  
  /* Allocation. */
  lin = lin + 2;
  if ( credraw )
    {
      attrset(LabelColor);
      cdputs( "w/e =", lin, col );
      attrset(0);
      zzsweapons = -9;
      zzsengines = -9;
    }
  i = sweapons[snum];
  j = sengines[snum];
  if ( swfuse[snum] > 0 )
    i = 0;
  if ( sefuse[snum] > 0 )
    j = 0;
  if ( i != zzsweapons || j != zzsengines )
    {
      cdclra( lin, datacol, lin, STAT_COLS-1 );
      buf[0] = EOS;
      if ( i == 0 )
	appstr( "**", buf );
      else
	appint( i, buf );
      appchr( '/', buf );
      if ( j == 0 )
	appstr( "**", buf );
      else
	appint( j, buf );
      attrset(InfoColor);
      cdputs( buf, lin, datacol );
      attrset(0);
      zzsweapons = i;
    }
  
  /* Temperature. */
  lin = lin + 2;
  if ( credraw )
    {
      zzswtemp = 0;
      zzsetemp = 0;
      zzctemp = ' ';
    }
  i = round( swtemp[snum] );
  j = round( setemp[snum] );
  if ( i > 100 )
    i = 100;
  if ( j > 100 )
    j = 100;
  if ( i != zzswtemp || j != zzsetemp )
    {
      static char wtbuf[16];
      static char etbuf[16];
      
      wtbuf[0] = '\0';
      etbuf[0] = '\0';
      
      cdclra( lin, datacol, lin, STAT_COLS-1 );
      if ( i != 0 || j != 0 )
	{
	  buf[0] = EOS;
	  
	  if ( i >= 100 )
	    strcpy(wtbuf, "**");
	  else
	    sprintf(wtbuf, "%02d", i);
	  
	  if ( j >= 100 )
	    strcpy(etbuf, "**");
	  else
	    sprintf(etbuf, "%02d", j);
	  
	  if (i >= 0 && i <= 50)
	    WeapAttrib = GreenLevelColor;
	  else if (i >=51 && i <=75)
	    WeapAttrib = YellowLevelColor;
	  else if (i >= 76)
	    WeapAttrib = RedLevelColor;
	  
	  attrset(WeapAttrib);
	  cdputs(wtbuf, lin, datacol);
	  attrset(0);
	  
	  attrset(InfoColor);
	  cdputs("/", lin, datacol + 2);
	  attrset(0);
	  
	  if (j >= 0 && j <= 50)
	    EngAttrib = GreenLevelColor;
	  else if (j >=51 && j <=80)
	    EngAttrib = YellowLevelColor;
	  else if (j >= 81)
	    EngAttrib = RedLevelColor;
	  
	  attrset(EngAttrib);
	  cdputs(etbuf, lin, datacol + 3);
	  attrset(0);
	  
	}
      zzswtemp = i;
      zzsetemp = j;
    }
  
  if ( i == 0 && j == 0 )
    j = ' ';
  else if ( i >= 80 || j >= 80 )
    j = 'T';
  else
    j = 't';
  if ( j != zzctemp )
    {
      cdclra( lin, col, lin, datacol-1 );
      if ( j == 't' )
	{
	  attrset(LabelColor);
	  cdputs( "temp =", lin, col );
	  attrset(0);
	}
      else if ( j == 'T' )
	{
	  attrset(LabelColor);
	  cdputs( "TEMP =", lin, col );
	  attrset(0);
	}
      zzctemp = j;
    }
  
  /* Damage/repair. */
  if ( sdamage[snum] > prevdam )
    dobeep = TRUE;
  prevdam = sdamage[snum];
  
  lin = lin + 2;
  if ( credraw )
    {
      zzsdamage = -9;
      zzcdamage = ' ';
    }
  i = round( sdamage[snum] );
  if ( i != zzsdamage )
    {
      cdclra( lin, datacol, lin, STAT_COLS-1 );
      if ( i > 0 )
	{
	  sprintf( buf, "%d", i );
	  if (i >= 0 && i <= 10)
	    DamageAttrib = GreenLevelColor;
	  else if (i >=11 && i <=65)
	    DamageAttrib = YellowLevelColor;
	  else if (i >= 66)
	    DamageAttrib = RedLevelColor;
	  
	  attrset(DamageAttrib);
	  cdputs( buf, lin, datacol );
	  attrset(0);
	}
      zzsdamage = i;
    }
  
  if ( srmode[snum] )
    j = 'r';
  else if ( i >= 50 )
    j = 'D';
  else if ( i > 0 )
    j = 'd';
  else
    j = ' ';
  if ( j != zzcdamage )
    {
      cdclra( lin, col, lin, datacol-1 );
      
      if ( j == 'r' )
	{
	  attrset(GreenLevelColor);
	  cdputs( "REPAIR, dmg =", lin, col );
	}
      else if ( j == 'd' )
	{
	  attrset(YellowLevelColor);
	  cdputs( "damage =", lin, col );
	}
      else if ( j == 'D' )
	{
	  attrset(RedLevelColor);
	  cdputs( "DAMAGE =", lin, col );
	}
      attrset(0);
      zzcdamage = j;
    }
  
  /* Armies. */
  lin = lin + 2;
  if ( credraw )
    zzsarmies = -666;
  i = sarmies[snum];
  if ( i == 0 )
    i = -saction[snum];
  if ( i != zzsarmies )
    {
      cdclra( lin, col, lin, STAT_COLS-1 );
      if ( i > 0 )
	{
	  attrset(InfoColor);
	  cdputs( "armies =", lin, col );
	  sprintf( buf, "%d", i );
	  cdputs( buf, lin, datacol );
	  attrset(0);
	}
      else if ( i < 0 )
	{
	  attrset(InfoColor);
	  cdputs( "action =", lin, col );
	  robstr( -i, buf );
	  cdputs( buf, lin, datacol );
	  attrset(0);
	}
      zzsarmies = i;
    }
  
  /* Tractor beams. */
  lin = lin + 2;
  if ( credraw )
    zzstowedby = 0;
  i = stowedby[snum];
  if ( i == 0 )
    i = -stowing[snum];
  if ( i != zzstowedby )
    {
      cdclra( lin, col, lin, datacol-1 );
      if ( i == 0 )
	buf[0] = EOS;
      else if ( i < 0 )
	{
	  c_strcpy( "towing ", buf );
	  appship( -i, buf );
	}
      else if ( i > 0 )
	{
	  c_strcpy( "towed by ", buf );
	  appship( i, buf );
	}
      attrset(InfoColor);
      cdputs( buf, lin, col );
      attrset(0);
      zzstowedby = i;
    }
  
  /* Self destruct fuse. */
  lin = lin + 2;
  if ( credraw )
    zzssdfuse = -9;
  if ( scloaked[snum] )
    i = -1;
  else
    i = max( 0, ssdfuse[snum] );
  if ( i != zzssdfuse )
    {
      cdclra( lin, col, lin, STAT_COLS-1 );
      if ( i > 0 )
	{
	  sprintf( buf, "DESTRUCT MINUS %-3d", i );
	  attrset(RedLevelColor);
	  cdputs( buf, lin, col );
	  attrset(0);
	}
      else if ( i == -1 )
	{
	  attrset(RedLevelColor);
	  cdputs( "CLOAKED", lin, col );
	  attrset(0);
	}
      else 
	cdputs( "       ", lin, col );
      zzssdfuse = i;
    }
  
  if ( dobeep )
    if ( soption[snum][OPT_ALARMBELL] )
      cdbeep();
  
  
  cdrefresh( TRUE );

/*  if (doalertb == TRUE)
    {
      draw_alertborder(AlertLevel);
      cdrefresh( TRUE );
    }
*/
  credraw = FALSE;
  
  return;
  
}


/*##  enemydet - detonate enemy torpedos */
/*  SYNOPSIS */
/*    int didit, enemydet */
/*    int snum */
/*    didit = enemydet( snum ) */
int enemydet( int snum )
{
  int i, j;
  
  /* Stop repairing. */
  srmode[snum] = FALSE;
  
  if ( ! usefuel( snum, DETONATE_FUEL, TRUE ) )
    return ( FALSE );
  
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] != SS_OFF && i != snum )
      for ( j = 0; j < MAXTORPS; j = j + 1 )
	if ( tstatus[i][j] == TS_LIVE )
	  if ( twar[i][j][steam[snum]] || swar[snum][steam[i]] )
	    if ( dist( sx[snum], sy[snum], tx[i][j], ty[i][j] ) <=
		DETONATE_DIST )
	      detonate( i, j );
  
  return ( TRUE );
  
}


/*##  hit - hit a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    real ht */
/*    hit( snum, ht, kb ) */
void hit( int snum, real ht, int kb )
{
  
  if ( ht > 0.0 )
    if ( sshup[snum] && ! srmode[snum] )
      if ( ht > sshields[snum] )
	{
	  damage( snum, ht-sshields[snum], kb );
	  sshields[snum] = 0.0;
	}
      else
	sshields[snum] = sshields[snum] - ht;
    else
      damage( snum, ht, kb );
  
  return;
}


/*##  ikill - ikill a ship */
/*  SYNOPSIS */
/*    int snum, kb */
/*    ikill( snum, kb ) */
/*  Note: This routines ASSUMES you have the common locked before you it. */
void ikill( int snum, int kb )
{
  int i, unum, team, kunum, kteam;
  real tkills;
  
  /* Only procede if the ship is alive */
  if ( sstatus[snum] != SS_LIVE )
    return;
  
  /* The ship is alive; kill it. */
  skilledby[snum] = kb;
  sstatus[snum] = SS_DYING;
  
  unum = suser[snum];
  team = steam[snum];
  
  /* Detonate all torpedos. */
  for ( i = 0; i < MAXTORPS; i = i + 1 )
    if ( tstatus[snum][i] == TS_LIVE )
      tstatus[snum][i] = TS_DETONATE;
  
  /* Release any tows. */
  if ( stowing[snum] != 0 )
    stowedby[stowing[snum]] = 0;
  if ( stowedby[snum] != 0 )
    stowing[stowedby[snum]] = 0;
  
  /* Zero team scan fuses. */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    sscanned[snum][i] = 0;
  
  if ( kb == KB_CONQUER )
    skills[snum] = skills[snum] + CONQUER_KILLS;
  else if ( kb == KB_GOTDOOMSDAY )
    skills[snum] = skills[snum] + DOOMSDAY_KILLS;
  else if ( kb >= 0 )				/* if a ship did the killing */
    {
      kunum = suser[kb];
      kteam = steam[kb];
      tkills = 1.0 + ((skills[snum] + sstrkills[snum]) * KILLS_KILLS);
      if ( sarmies[snum] > 0 )
	{
	  /* Keep track of carried armies killed - they are special. */
	  tkills = tkills + sarmies[snum] * ARMY_KILLS;
	  ustats[kunum][USTAT_ARMSHIP] =
	    ustats[kunum][USTAT_ARMSHIP] + sarmies[snum];
	  tstats[kteam][TSTAT_ARMSHIP] =
	    tstats[kteam][TSTAT_ARMSHIP] + sarmies[snum];
	}
      
      /* Kills accounting. */
      if ( sstatus[kb] == SS_LIVE )
	skills[kb] = skills[kb] + tkills;
      else
	{
	  /* Have to do some hacking when our killer is dead. */
	  ustats[kunum][USTAT_WINS] =
	    ustats[kunum][USTAT_WINS] - ifix(skills[kb]);
	  tstats[kteam][TSTAT_WINS] =
	    tstats[kteam][TSTAT_WINS] - ifix(skills[kb]);
	  skills[kb] = skills[kb] + tkills;
	  chalkup( kb );
	}
      
      /* Sticky war logic. */
				/* should set sticky war whether or not your
				   at war with them. -JET */

      if ( ! swar[snum][kteam] )
	{
	  swar[kb][team] = TRUE;
	}
      if ( ! srwar[snum][kteam] )
	{
	  srwar[kb][team] = TRUE;
	}
    }
  
  /* Kills accounting. */
  chalkup( snum );
  if ( kb != KB_SELF && kb != KB_CONQUER && kb != KB_NEWGAME &&
      kb != KB_EVICT && kb != KB_SHIT && kb != KB_GOD )
    {
      /* Update losses. */
      ustats[unum][USTAT_LOSSES] = ustats[unum][USTAT_LOSSES] + 1;
      tstats[team][TSTAT_LOSSES] = tstats[team][TSTAT_LOSSES] + 1;
    }
  
  if ( ! srobot[snum] || spid[snum] != 0 )
    {
      sstatus[snum] = SS_DEAD;
      ssdfuse[snum] = -TIMEOUT_PLAYER;		/* setup dead timeout timer */
    }
  else
    {
      sstatus[snum] = SS_OFF;			/* turn robots off */
      /* We'd like to remove this next line so that you could */
      /* use conqoper to see what killed him, but then robots */
      /* show up on the debugging playlist... */
      skilledby[snum] = 0;
    }
  
  return;
  
}


/*##  infoplanet - write out information about a planet */
/*  SYNOPSIS */
/*    char str() */
/*    int pnum, snum */
/*    infoplanet( str, pnum, snum ) */
void infoplanet( char *str, int pnum, int snum )
{
  int i, j; 
  int godlike, canscan; 
  char buf[MSGMAXLINE*2], junk[MSGMAXLINE];
  real x, y;
  int doETA;
  
  /* Check range of the passed planet number. */
  if ( pnum <= 0 || pnum > NUMPLANETS )
    {
      c_putmsg( "No such planet.", MSG_LIN1 );
      cdclrl( MSG_LIN2, 1 );
      cdmove( MSG_LIN1, 1 );
      cerror("infoplanet: Called with invalid pnum (%d).",
	     pnum );
      return;
    }
  
  /* GOD is too clever. */
  godlike = ( snum < 1 || snum > MAXSHIPS );
  
  if (sysconf_DoETAStats)
    {
      if (godlike)
	doETA = TRUE;
      else
	doETA = FALSE;
    }
  
  
  /* In some cases, report hostilities. */
  junk[0] = EOS;
  if ( ptype[pnum] == PLANET_CLASSM || ptype[pnum] == PLANET_DEAD )
    if ( ! godlike )
      if ( pscanned[pnum][steam[snum]] && spwar( snum, pnum ) )
	appstr( " (hostile)", junk );
  
  /* Things that orbit things that orbit have phases. */
  switch ( phoon( pnum ) )
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
      x = sx[snum];
      y = sy[snum];
    }
  
  if (sysconf_DoETAStats)
    {
      static char tmpstr[64];
      
      if (swarp[snum] > 0.0)
	{
	  sprintf(tmpstr, ", ETA %s",
		  ETAstr(swarp[snum], 	
			 round( dist( x, y, px[pnum], py[pnum])) ));
	}
      else
	tmpstr[0] = '\0';
      
      sprintf( buf, "%s%s, a %s%s, range %d, direction %d%s",
	     str,
	     pname[pnum],
	     ptname[ptype[pnum]],
	     junk,
	     round( dist( x, y, px[pnum], py[pnum] ) ),
	     round( angle( x, y, px[pnum], py[pnum] ) ),
	     tmpstr);
    }
  else
    sprintf( buf, "%s%s, a %s%s, range %d, direction %d",
	   str,
	   pname[pnum],
	   ptname[ptype[pnum]],
	   junk,
	   round( dist( x, y, px[pnum], py[pnum] ) ),
	   round( angle( x, y, px[pnum], py[pnum] ) ));
  
  if ( godlike )
    canscan = TRUE;
  else
    canscan = pscanned[pnum][steam[snum]];
  
  junk[0] = EOS;
  if ( ptype[pnum] != PLANET_SUN && ptype[pnum] != PLANET_MOON )
    {
      if ( ! canscan )
	c_strcpy( "with unknown occupational forces", junk );
      else
	{
	  i = parmies[pnum];
	  if ( i == 0 )
	    {
	      j = puninhabtime[pnum];
	      if ( j > 0 )
		sprintf( junk, "uninhabitable for %d more minutes", j );
	      else
		c_strcpy( "with NO armies", junk );
	    }
	  else
	    {
	      sprintf( junk, "with %d %s arm", i, tname[pteam[pnum]] );
	      if ( i == 1 )
		appstr( "y", junk );
	      else
		appstr( "ies", junk );
	    }
	}
      
      /* Now see if we can tell about coup time. */
      if ( godlike )
	canscan = FALSE;			/* GOD can use teaminfo instead */
      else
	canscan = ( pnum == homeplanet[steam[snum]] &&
		   tcoupinfo[steam[snum]] );
      if ( canscan )
	{
	  j = couptime[steam[snum]];
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
      c_putmsg( buf, MSG_LIN1 );
      if ( junk[0] != EOS )
	c_putmsg( junk, MSG_LIN2 );
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
      c_putmsg( buf, MSG_LIN1 );
      c_putmsg( &buf[i+1], MSG_LIN2 );
    }
  
  cdmove( MSG_LIN1, 1 );
  return;
  
}

/* ETAstr - return a string indicating ETA to a target */
char *ETAstr(real warp, real distance)
{
  real secs;
  real mins;
  static char retstr[64];
  
  if (warp <= 0.0)
    {
      sprintf(retstr, "never");
      return(retstr);
    }
  
  mins = 0.0;
  secs = (real) (distance / (warp * MM_PER_SEC_PER_WARP));
  
  if (secs > 60.0)
    {
      mins = secs / 60.0;
      secs = 0.0;
    }
  
  if (mins != 0.0)
    sprintf(retstr, "%.1f minutes", mins);
  else
    sprintf(retstr, "%.1f seconds", secs);
  
  return(retstr);
}

/*##  infoship - write out information about a ship */
/*  SYNOPSIS */
/*    int snum, scanner */
/*    infoship( snum, scanner ) */
void infoship( int snum, int scanner )
{
  int i, status;
  char junk[MSGMAXLINE];
  real x, y, dis, kills, appx, appy;
  int godlike, canscan;
  int doETA;
  
  godlike = ( scanner < 1 || scanner > MAXSHIPS );
  
  if (sysconf_DoETAStats)
    {
      if (godlike)
	doETA = TRUE;
      else
	doETA = FALSE;
    }
  
  cdclrl( MSG_LIN2, 1 );
  if ( snum < 1 || snum > MAXSHIPS )
    {
      c_putmsg( "No such ship.", MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
      return;
    }
  status = sstatus[snum];
  if ( ! godlike && status != SS_LIVE )
    {
      c_putmsg( "Not found.", MSG_LIN1 );
      cdmove( MSG_LIN1, 1 );
      return;
    }
  cbuf[0] = EOS;
  appship( snum, cbuf );
  if ( snum == scanner )
    {
      /* Silly Captain... */
      appstr( ": That's us, silly!", cbuf );
      c_putmsg( cbuf, MSG_LIN1 );
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
      x = sx[scanner];
      y = sy[scanner];
    }
  if ( scloaked[snum] )
    {
      appx = rndnor(sx[snum], CLOAK_SMEAR_DIST);
      appy = rndnor(sy[snum], CLOAK_SMEAR_DIST);
    }
  else
    {
      appx = sx[snum];
      appy = sy[snum];
    }
  dis = dist( x, y, appx, appy );
  if ( godlike )
    canscan = TRUE;
  else
    {
      /* Help out the driver with this scan. */
      if ( (dis < ACCINFO_DIST && ! scloaked[snum]) && ! selfwar(scanner) )
	sscanned[snum][ steam[scanner]] = SCANNED_FUSE;
      
      /* Decide if we can do an acurate scan. */
      canscan = ( (dis < ACCINFO_DIST && ! scloaked[snum]) ||
		 ( (sscanned[snum][ steam[scanner]] > 0) && ! selfwar(scanner) ) );
    }
  
  appstr( ": ", cbuf );
  if ( spname[snum][0] != EOS )
    {
      appstr( spname[snum], cbuf );
      appstr( ", ", cbuf );
    }
  kills = (skills[snum] + sstrkills[snum]);
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
  if ( dis < ACCINFO_DIST && scloaked[snum] )
    appstr( " (CLOAKED) ", cbuf );
  else
    appstr( ", ", cbuf );
  if ( godlike )
    {
      appsstatus( status, cbuf );
      appchr( '.', cbuf );
    }
  else if ( swar[snum][steam[scanner]] )
    appstr( "at WAR.", cbuf );
  else
    appstr( "at peace.", cbuf );
  
  c_putmsg( cbuf, MSG_LIN1 );
  
  if ( ! scloaked[snum] || swarp[snum] > 0.0 )
    {
      sprintf( cbuf, "Range %d, direction %d",
	     round( dis ), round( angle( x, y, appx, appy ) ) );
      
      if (sysconf_DoETAStats)
	{
	  if (swarp[scanner] > 0.0)
	    {
	      static char tmpstr[32];
	      
	      sprintf(tmpstr, ", ETA %s",
		      ETAstr(swarp[scanner], dis));
	      appstr(tmpstr, cbuf);
	    }
	}
    }
  else
    cbuf[0] = EOS;
  
  if ( canscan )
    {
      if ( cbuf[0] != EOS )
	appstr( ", ", cbuf );
      appstr( "shields ", cbuf );
      if ( sshup[snum] && ! srmode[snum] )
	appint( round( sshields[snum] ), cbuf );
      else
	appstr( "DOWN", cbuf );
      i = round( sdamage[snum] );
      if ( i > 0 )
	{
	  if ( cbuf[0] != EOS )
	    appstr( ", ", cbuf );
	  sprintf( junk, "damage %d", i );
	  appstr( junk, cbuf );
	}
      i = sarmies[snum];
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
      cbuf[0] = cupper( cbuf[0] );
      appchr( '.', cbuf );
      c_putmsg( cbuf, MSG_LIN2 );
    }
  
  cdmove( MSG_LIN1, 1 );
  return;
  
}


/*##  kill - kill a ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum, kb */
/*    kill( snum, kb ) */
void killship( int snum, int kb )
{
  int sendmsg = FALSE;
  char msgbuf[128];

  
				/* internal routine. */
  PVLOCK(lockword);
  ikill( snum, kb );
  PVUNLOCK(lockword);

				/* send a msg to all... */
  sendmsg = FALSE;

  /* Figure out why we died. */
  switch ( kb )
    {
    case KB_SELF:
      sprintf(msgbuf, "%c%d (%s) has self-destructed.",
	      chrteams[uteam[suser[snum]]],
	      snum,
	      cuname[suser[snum]]);
      sendmsg = TRUE;
      
      break;
    case KB_NEGENB:
      sprintf(msgbuf, "%c%d (%s) was destroyed by the negative energy barrier.",
	      chrteams[uteam[suser[snum]]],
	      snum,
	      upname[suser[snum]]);
      sendmsg = TRUE;
      
      break;
      
    case KB_GOD:
      sprintf(msgbuf, "%c%d (%s) was killed by an act of GOD.",
	      chrteams[uteam[suser[snum]]],
	      snum,
	      upname[suser[snum]]);
      sendmsg = TRUE;
      
      break;
    case KB_DOOMSDAY:
      sprintf(msgbuf, "%c%d (%s) was eaten by the doomsday machine.",
	      chrteams[uteam[suser[snum]]],
	      snum,
	      upname[suser[snum]]);
      sendmsg = TRUE;
      
      break;
    case KB_DEATHSTAR:
      sprintf(msgbuf, "%c%d (%s) was vaporized by the Death Star.",
	      chrteams[uteam[suser[snum]]],
	      snum,
	      upname[suser[snum]]);
      sendmsg = TRUE;

      break;
    case KB_LIGHTNING:
      sprintf(msgbuf, "%c%d (%s) was destroyed by lightning bolt.",
	      chrteams[uteam[suser[snum]]],
	      snum,
	      upname[suser[snum]]);
      sendmsg = TRUE;

      break;
    default:
      
      if ( kb > 0 && kb <= MAXSHIPS )
	{
	  sprintf(msgbuf, "%c%d (%s) was kill %.1f for %c%d (%s).",
		  chrteams[uteam[suser[snum]]],
		  snum,
		  upname[suser[snum]],
		  skills[kb],
		  chrteams[uteam[suser[kb]]],
		  kb,
		  upname[suser[kb]]);
	  sendmsg = TRUE;

	}
      else if ( -kb > 0 && -kb <= NUMPLANETS )
	{
	  sprintf(msgbuf, "%c%d (%s) was destroyed by %s",
		  chrteams[uteam[suser[snum]]],
		  snum,
		  upname[suser[snum]],
		  pname[-kb]);

	  sendmsg = TRUE;
	  
	  if ( ptype[-kb] == PLANET_SUN )
	    {
	      appstr( "'s solar radiation.", msgbuf );
	    }
	  else
	    {
	      appstr( "'s planetary defenses.", msgbuf );
	    }
	}
    }

  if (sendmsg == TRUE)
    stormsg(MSG_COMP, MSG_ALL, msgbuf);

  return;
  
}


/*##  launch - create a new torpedo for a ship (DOES LOCKING) */
/*  SYNOPSIS */
/*    int launch, snum */
/*    int flag, launch */
/*    real dir */
/*    flag = launch( snum, dir ) */
int launch( int snum, real dir, int number )
{
  register int i, j;
  real speed, adir; 
  int tnum, numslots, numfired;
  static int tslot[MAXTORPS];
  
  /* Stop repairing. */
  srmode[snum] = FALSE;
  
  /* Remember this important direction. */
  slastblast[snum] = dir;
  
  /* Set up last fired phaser direction. */
  slastphase[snum] = dir;

  numslots = 0;
  numfired = 0;
  tnum = number;
  
  /* Find free torp(s). */
  PVLOCK(lockword);
  for ( i = 0; i < MAXTORPS && tnum != 0; i++ )
    if ( tstatus[snum][i] == TS_OFF )
      {
	/* Found one. */
	tstatus[snum][i] = TS_LAUNCHING;
	tslot[numslots++] = i;
	tnum--;
      }
  PVUNLOCK(lockword);
  
  if (numslots == 0)
    {				/* couldn't find even one */
      return(FALSE);
    }
  
  for (i=0; i<numslots; i++)
    {
      /* Use fuel. */
      if ( usefuel( snum, TORPEDO_FUEL, TRUE ) == FALSE)
	{
	  tstatus[snum][tslot[i]] = TS_OFF;
	  continue;
	}
      else
	{			/* fired successfully */
	  numfired++;
	}
      
      /* Initialize it. */
      tfuse[snum][tslot[i]] = TORPEDO_FUSE;
      tx[snum][tslot[i]] = rndnor( sx[snum], 100.0 );
      ty[snum][tslot[i]] = rndnor( sy[snum], 100.0 );
      speed = torpwarp[steam[snum]] * MM_PER_SEC_PER_WARP * ITER_SECONDS;
      adir = rndnor( dir, 2.0 );
      tdx[snum][tslot[i]] = (real) (speed * cosd(adir));
      tdy[snum][tslot[i]] = (real)(speed * sind(adir));
      tmult[snum][tslot[i]] = (real)weaeff( snum );
      for ( j = 0; j < NUMTEAMS; j = j + 1 )
	twar[snum][tslot[i]][j] = swar[snum][j];
      tstatus[snum][tslot[i]] = TS_LIVE;
    } 
  
  if (numfired == 0)
    {				/* couldn't fire any. bummer dude. */
      return(FALSE);
    }
  else
    {				/* torps away! */
      /* Update stats. */
      PVLOCK(lockword);
      ustats[suser[snum]][USTAT_TORPS] =
	ustats[suser[snum]][USTAT_TORPS] + numfired;
      tstats[steam[snum]][TSTAT_TORPS] =
	tstats[steam[snum]][TSTAT_TORPS] + numfired;
      PVUNLOCK(lockword);
      
      if (numfired == number)
	{			/* fired all requested */
	  return ( TRUE );
	}
      else
	{
	  /* fired some, but not all */
	  return(FALSE);
	}
    }
  
}


/*##  orbit - place a ship into orbit around a planet */
/*  SYNOPSIS */
/*    int snum, pnum */
/*    orbit( snum, pnum ) */
void orbit( int snum, int pnum )
{
  real beer; 
  
  slock[snum] = -pnum;
  sdwarp[snum] = 0.0;
  
  /* Find bearing to planet. */
  beer = angle( sx[snum], sy[snum], px[pnum], py[pnum] );
  if ( shead[snum] < ( beer - 180.0 ) )
    beer = beer - 360.0;
  
  /* Check beer head to determine orbit direction. */
  if ( beer <= shead[snum] )
    {
      swarp[snum] = ORBIT_CW;
      shead[snum] = mod360( beer + 90.0 );
    }
  else
    {
      swarp[snum] = ORBIT_CCW;
      shead[snum] = mod360( beer - 90.0 );
    }
  
  return;
  
}


/*##  phaser - fire phasers (bug fry!!) (DOES LOCKING) */
/*  SYNOPSIS */
/*    int didit, phaser */
/*    int snum */
/*    real dir */
/*    didit = phaser( snum, dir ) */
int phaser( int snum, real dir )
{
  int k;
  real dis, ang;
  
  /* Set up last weapon direction. */
  slastblast[snum] = dir;
  
  /* Stop repairing. */
  srmode[snum] = FALSE;
  
  /* See if ok to fire. */
  if ( spfuse[snum] > 0 )
    return ( FALSE );
  
  /* Try to use fuel for this shot. */
  if ( ! usefuel( snum, PHASER_FUEL, TRUE ) )
    return ( FALSE );
  
  /* Update stats. */
  PVLOCK(lockword);
  ustats[suser[snum]][USTAT_PHASERS] = ustats[suser[snum]][USTAT_PHASERS] + 1;
  tstats[steam[snum]][TSTAT_PHASERS] = tstats[steam[snum]][TSTAT_PHASERS] + 1;
  PVUNLOCK(lockword);
  
  /* Set up last fired direction. */
  slastphase[snum] = dir;
  
  /* Start phaser fuse. */
  spfuse[snum] = PHASER_TENTHS;
  
  /* See what we can hit. */
  for ( k = 1; k <= MAXSHIPS; k = k + 1 )
    if ( sstatus[k] == SS_LIVE && k != snum )
      if ( satwar(snum, k ) )
	{
	  dis = dist( sx[snum], sy[snum], sx[k], sy[k] );
	  if ( dis <= PHASER_DIST )
	    {
	      ang = angle( sx[snum], sy[snum], sx[k], sy[k] );
	      if ( ang > 315.0 )
		ang = ang - 360.0;
	      if ( abs( dir - ang ) <= PHASER_SPREAD )
		{
		  hit( k, phaserhit( snum, dis ), snum );
		  LastPhasDist = dis;
		}
	      else
		LastPhasDist = PHASER_DIST;
	    }
	  else
	    LastPhasDist = PHASER_DIST;
	}
  
  return ( TRUE );
  
}


/*##  phaserhit - determine phaser damage */
/*  SYNOPSIS */
/*    int snum */
/*    real hit, phaserhit, dis */
/*    hit = phaserhit( snum, dis ) */
real phaserhit( int snum, real dis )
{
  return (( - dis / PHASER_DIST + 1.0 ) * PHASER_HIT * weaeff( snum ));
  
}


/*##  planlist - list planets */
/*  SYNOPSIS */
/*    int team */
/*    planlist( team ) */
void planlist( int team )
{
  int i, j, lin, col, olin, pnum;
  static int init = FALSE, sv[NUMPLANETS + 1];
  char ch, junk[10], junk2[MAXPLANETNAME + 2], coreflag;
  string hd="planet      type team armies          planet      type team armies";
  
  if ( init == FALSE )
    {
      for ( i = 1; i <= NUMPLANETS; i++ )
	sv[i] = i;
      init = TRUE;
      sortplanets( sv );
    }
  
  lin = 1;
  cdputc( "P L A N E T   L I S T    ('+' = must take to conquer the Universe)", lin );
  c_strcpy( hd, cbuf );
  lin = lin + 2;
  cdputc( cbuf, lin );
  for ( i = 0; cbuf[i] != EOS; i = i + 1 )
    if ( cbuf[i] != ' ' )
      cbuf[i] = '-';
  lin = lin + 1;
  cdputc( cbuf, lin );
  lin = lin + 1;
  olin = lin;
  col = 5;
  
  /*    for ( j = 0; j < 3; j = j + 1 )
	{
	*/
  for ( i = 1; i <= NUMPLANETS; i = i + 1 )
    {
      pnum = sv[i];
      
      /* Don't display unless it's real. */
      if ( ! preal[pnum] )
	continue; /*next;*/
      
      /* I want everything if it's real */
      
      /* Figure out who owns it and count armies. */
      ch =  chrteams[pteam[pnum]];
      sprintf( junk, "%d", parmies[pnum] );
      
      /* Then modify based on scan information. */
      if ( team != TEAM_NOTEAM )
	if ( ! pscanned[pnum][team] )
	  {
	    ch = '?';
	    c_strcpy( "?", junk );
	  }
      
      /* Suns and moons are displayed as unowned. */
      if ( ptype[pnum] == PLANET_SUN || ptype[pnum] == PLANET_MOON )
	ch = ' ';
      
      /* Don't display armies for suns unless we're special. */
      if ( ptype[pnum] == PLANET_SUN )
	if ( team != TEAM_NOTEAM )
	  junk[0] = EOS;
      
      /* Moons aren't supposed to have armies. */
      if ( ptype[pnum] == PLANET_MOON )
	if ( team != TEAM_NOTEAM )
	  junk[0] = EOS;
	else if ( parmies[pnum] == 0 )
	  junk[0] = EOS;

      coreflag = ' ';

				/* flag planets that are required for a conq */
      if (ptype[pnum] == PLANET_CLASSM || ptype[pnum] == PLANET_DEAD)
	{
	  if (pnum > NUMCONPLANETS)
	    coreflag = ' ';
	  else
	    coreflag = '+';
	}
      
      sprintf(junk2, "%c %s", coreflag,  pname[pnum]);

      sprintf( cbuf, "%-13s %-4c %-3c  %4s",
	     junk2, chrplanets[ptype[pnum]], ch, junk );
      
      cdputs( cbuf, lin, col );
      
      lin = lin + 1;
      if ( lin == MSG_LIN1 )
	{
	  lin = olin;
	  col = 43;
	}
    }
  
  
  return;
  
}


/*##  playlist - list ships */
/*  SYNOPSIS */
/*    int godlike, doall */
/*    playlist( godlike, doall ) */
void playlist( int godlike, int doall )
{
  int i, unum, status, kb, lin, col;
  int fline, lline, fship;
  char sbuf[20];
  char kbuf[20];
  int ch;
  
  /* Do some screen setup. */
  cdclear();
  c_strcpy( "ship name          pseudonym              kills      pid", cbuf );


  col = ( cdcols(0) - strlen( cbuf ) ) / 2;
  lin = 2;
  cdputs( cbuf, lin, col );
  
  for ( i = 0; cbuf[i] != EOS; i = i + 1 )
    if ( cbuf[i] != ' ' )
      cbuf[i] = '-';
  lin = lin + 1;
  cdputs( cbuf, lin, col );
  
  fline = lin + 1;				/* first line to use */
  lline = MSG_LIN1;				/* last line to use */
  fship = 1;					/* first user in uvec */
  
  while(TRUE) /* repeat- while */
    {
      if ( ! godlike )
	if ( ! stillalive( csnum ) )
	  break;
      i = fship;
      cdclrl( fline, lline - fline + 1 );
      lin = fline;
      while ( i <= MAXSHIPS && lin <= lline )
	{
	  status = sstatus[i];

	  kb = skilledby[i];
	  if ( status == SS_LIVE ||
	      ( doall && ( status != SS_OFF || kb != 0 ) ) )
	    {
	      sbuf[0] = EOS;
	      appship( i, sbuf );
	      unum = suser[i];
	      if ( unum >= 0 && unum < MAXUSERS )
		{
		  sprintf(kbuf, "%6.1f", (skills[i] + sstrkills[i]));
		  sprintf( cbuf, "%-4s %-13.13s %-21.21s %-8s %6d",
			   sbuf, cuname[unum], spname[i], 
			   kbuf, spid[i] );
		}
	      else
		sprintf( cbuf, "%-4s %13s %21s %8s %6s", sbuf,
		       " ", " ", " ", " " );
	      if ( doall && kb != 0 )
		{
		  appstr( "  ", cbuf);
		  appkb( kb, cbuf );
		}
	      cdputs( cbuf, lin, col );
	      if ( doall && status != SS_LIVE )
		{
		  cbuf[0] = EOS;
		  appsstatus( status, cbuf );
		  /*		  appstr("(", cbuf);
		  appint(spid[i], cbuf);
		  appstr(")", cbuf);
		  */
		  
		  cdputs( cbuf, lin, col - 2 - strlen( cbuf ) );
		}
	    }
	  i = i + 1;
	  lin = lin + 1;
	}
      if ( i > MAXSHIPS )
	{
	  /* We're displaying the last page. */
	  putpmt( "--- press space when done ---", MSG_LIN2 );
	  cdrefresh( TRUE );
	  if ( iogtimed( &ch, 1 ) )
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
	  putpmt( "--- press space for more ---", MSG_LIN2 );
	  cdrefresh( TRUE );
	  if ( iogtimed( &ch, 1 ) )
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


/*##  pseudo - change an user's pseudonym */
/*  SYNOPSIS */
/*    int unum, snum */
/*    pseudo( unum, snum ) */
void pseudo( int unum, int snum )
{
  char ch, buf[MSGMAXLINE];
  int savcsnum;
  
  savcsnum = csnum;
  csnum = ERR;		/* turn off ship display */
  
  cdclrl( MSG_LIN1, 2 );
  c_strcpy( "Old pseudonym: ", buf );
  if ( snum > 0 && snum <= MAXSHIPS )
    appstr( spname[snum], buf );
  else
    appstr( upname[unum], buf );
  cdputc( buf, MSG_LIN1 );
  ch = getcx( "Enter a new pseudonym: ",
	     MSG_LIN2, -4, TERMS, buf, MAXUSERPNAME );
  if ( ch != TERM_ABORT && ( buf[0] != EOS || ch == TERM_EXTRA ) )
    {
      stcpn( buf, upname[unum], MAXUSERPNAME );
      if ( snum > 0 && snum <= MAXSHIPS )
	stcpn( buf, spname[snum], MAXUSERPNAME );
    }
  cdclrl( MSG_LIN1, 2 );
  
  csnum = savcsnum;		/* restore csnum */
  
  return;
  
}


/*##  register - register a new user (DOES LOCKING) */
/*  SYNOPSIS */
/*    char lname(), rname() */
/*    int team, unum */
/*    int flag, register */
/*    flag = register( lname, rname, team, unum ) */
int c_register( char *lname, char *rname, int team, int *unum ) 
{
  int i, j;
  
  PVLOCK(lockword);
  for ( i = 0; i < MAXUSERS; i = i + 1 )
    if ( ! ulive[i] )
      {
	ulive[i] = TRUE;
	PVUNLOCK(lockword);
	urating[i] = 0.0;
	uteam[i] = team;
	urobot[i] = FALSE;
	umultiple[i] = 2;			/* but the option bit is off */
	
	for ( j = 0; j < MAXUSTATS; j = j + 1 )
	  ustats[i][j] = 0;
	
	for ( j = 0; j < NUMTEAMS; j = j + 1 )
	  uwar[i][j] = TRUE;
	uwar[i][uteam[i]] = FALSE;
	
	for ( j = 0; j < MAXOPTIONS; j = j + 1 )
	  uoption[i][j] = TRUE;
/*	uoption[i][OPT_INTRUDERALERT] = FALSE; JET - this turns off ALL msgs
                                                     from planets. not a
						     good default... */
	uoption[i][OPT_NUMERICMAP] = FALSE;
	uoption[i][OPT_TERSE] = FALSE;
	
	for ( j = 0; j < MAXOOPTIONS; j = j + 1 )
	  uooption[i][j] = FALSE;
	
	uooption[i][OOPT_SWITCHTEAMS] = TRUE; /* allow users to switchteams when dead */

	stcpn( "never", ulastentry[i], DATESIZE );
	stcpn( lname, cuname[i], MAXUSERNAME );
	stcpn( rname, upname[i], MAXUSERPNAME );
	*unum = i;
	return ( TRUE );
      }
  
  PVUNLOCK(lockword);
  
  return ( FALSE );
  
}


/*##  resign - remove a user from the user list (DOES LOCKING) */
/*  SYNOPSIS */
/*    int unum */
/*    resign( unum ) */
void resign( int unum )
{
  int i;
  
  PVLOCK(lockword);
  if ( unum >= 0 && unum < MAXUSERS )
    {
      ulive[unum] = FALSE;
      for ( i = 0; i < MAXHISTLOG; i = i + 1 )
	if ( unum == histunum[i] )
	  {
	    histunum[i] = -1;
	    histlog[i][0] = EOS;
	  }
    }
  PVUNLOCK(lockword);
  
  return;
  
}


/*##  review - review old messages */
/*  SYNOPSIS */
/*    int flag, review */
/*    int snum, slm */
/*    flag = review( snum, slm ) */
int review( int snum, int slm )
{
  int i, msg, lastone; 
  int didany;
  
  didany = FALSE;
  
  lastone = modp1( *lastmsg+1, MAXMESSAGES );
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      if ( slastmsg[snum] == LMSG_NEEDINIT )
	return ( FALSE );				/* none to read */
      i = salastmsg[snum];
      if ( i != LMSG_READALL )
	lastone = i;
    }
  
  cdclrl( MSG_LIN1, 1 );
  
  for ( msg = slm; msg != lastone; msg = modp1( msg-1, MAXMESSAGES ) )
    if ( canread( snum, msg ) || snum == msgfrom[msg] )
      {
	readmsg( snum, msg, MSG_LIN1 );
	didany = TRUE;
	cdrefresh( TRUE );
	if ( ! more( "" ) )
	  break;
      }
  
  cdclrl( MSG_LIN1, 2 );
  
  return ( didany );
  
}


/*##  takeplanet - take a planet (DOES SPECIAL LOCKING) */
/*  SYNOPSIS */
/*    int pnum, snum */
/*    takeplanet( pnum, snum ) */
/*  Note: This routines ASSUMES you have the common locked before you it. */
void takeplanet( int pnum, int snum )
{
  int i;
  char buf[MSGMAXLINE];
  
  pteam[pnum] = steam[snum];
  parmies[pnum] = 1;
  skills[snum] = skills[snum] + PLANET_KILLS;
  ustats[suser[snum]][USTAT_CONQPLANETS] =
    ustats[suser[snum]][USTAT_CONQPLANETS] + 1;
  tstats[steam[snum]][TSTAT_CONQPLANETS] =
    tstats[steam[snum]][TSTAT_CONQPLANETS] + 1;
  sprintf( buf, "All hail the liberating %s armies.  Thanks, ",
	 tname[steam[snum]] );
  appship( snum, buf );
  appchr( '!', buf );
  
  /* Check whether the universe has been conquered. */
  for ( i = 0; i < NUMCONPLANETS; i = i + 1 )
    if ( ptype[i] == PLANET_CLASSM || ptype[i] == PLANET_DEAD )
      if ( pteam[i] != steam[snum] || ! preal[i] )
	{
	  /* No. */
	  stormsg( -pnum, -steam[snum], buf );
	  return;
	}
  /* Yes! */
  getdandt( conqtime );
  stcpn( spname[snum], conqueror, MAXUSERPNAME );
  lastwords[0] = EOS;
  ustats[suser[snum]][USTAT_CONQUERS] = ustats[suser[snum]][USTAT_CONQUERS] + 1;
  tstats[steam[snum]][TSTAT_CONQUERS] = tstats[steam[snum]][TSTAT_CONQUERS] + 1;
  stcpn( tname[steam[snum]], conqteam, MAXTEAMNAME );
  ikill( snum, KB_CONQUER );
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( sstatus[i] == SS_LIVE )
      ikill( i, KB_NEWGAME );
  
  PVUNLOCK(lockword);
  initgame();
  PVLOCK(lockword);
  
  return;
  
}


/*##  teamlist - list team statistics */
/*  SYNOPSIS */
/*    int team */
/*    teamlist( team ) */
void teamlist( int team )
{
  int i, j, lin, col, ctime, etime;
  int godlike, showcoup;
  char buf[MSGMAXLINE], timbuf[5][DATESIZE];
  real x[5];
  string dfmt="%15s %11d %11d %11d %11d %11d";
  string pfmt="%15s %10.2f%% %10.2f%% %10.2f%% %10.2f%% %10.2f%%";
  string sfmt="%15s %11s %11s %11s %11s %11s";
  /*  string dfmt="%15s %12d %12d %12d %12d %12d";
      string pfmt="%15s %11.1f%% %11.1f%% %11.1f%% %11.1f%% %11.1f%%";
      string sfmt="%15s %12s %12s %12s %12s %12s";
      */ 
  godlike = ( team < 0 || team >= NUMTEAMS );
  col = 0; /*1*/
  
  lin = 1;
  sprintf( buf, "Statistics since %s:", inittime );
  cdputc( buf, lin );
  
  lin = lin + 1;
  sprintf( buf, "Universe last conquered at %s", conqtime );
  cdputc( buf, lin );
  
  lin = lin + 1;
  sprintf( buf, "by %s for the %s team.", conqueror, conqteam );
  cdputc( buf, lin );
  
  lin = lin + 1;
  cdclrl( lin, 1 );
  if ( lastwords[0] != EOS )
    {
      sprintf( buf, "%c%s%c", '"', lastwords, '"' );
      cdputc( buf, lin );
    }
  
  lin = lin + 2;
  sprintf( buf, sfmt, " ",
	 tname[0], tname[1], tname[2], tname[3], "Totals" );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  for ( i = 0; buf[i] != EOS; i = i + 1 )
    if ( buf[i] != ' ' )
      buf[i] = '-';
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Conquers",
	 tstats[0][TSTAT_CONQUERS], tstats[1][TSTAT_CONQUERS],
	 tstats[2][TSTAT_CONQUERS], tstats[3][TSTAT_CONQUERS],
	 tstats[0][TSTAT_CONQUERS] + tstats[1][TSTAT_CONQUERS] +
	 tstats[2][TSTAT_CONQUERS] + tstats[3][TSTAT_CONQUERS] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Wins",
	 tstats[0][TSTAT_WINS], tstats[1][TSTAT_WINS],
	 tstats[2][TSTAT_WINS], tstats[3][TSTAT_WINS],
	 tstats[0][TSTAT_WINS] + tstats[1][TSTAT_WINS] +
	 tstats[2][TSTAT_WINS] + tstats[3][TSTAT_WINS] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Losses",
	 tstats[0][TSTAT_LOSSES], tstats[1][TSTAT_LOSSES],
	 tstats[2][TSTAT_LOSSES], tstats[3][TSTAT_LOSSES],
	 tstats[0][TSTAT_LOSSES] + tstats[1][TSTAT_LOSSES] +
	 tstats[2][TSTAT_LOSSES] + tstats[3][TSTAT_LOSSES] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Ships",
	 tstats[0][TSTAT_ENTRIES], tstats[1][TSTAT_ENTRIES],
	 tstats[2][TSTAT_ENTRIES], tstats[3][TSTAT_ENTRIES],
	 tstats[0][TSTAT_ENTRIES] + tstats[1][TSTAT_ENTRIES] +
	 tstats[2][TSTAT_ENTRIES] + tstats[3][TSTAT_ENTRIES] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  etime = tstats[0][TSTAT_SECONDS] + tstats[1][TSTAT_SECONDS] +
    tstats[2][TSTAT_SECONDS] + tstats[3][TSTAT_SECONDS];
  fmtseconds( tstats[0][TSTAT_SECONDS], timbuf[0] );
  fmtseconds( tstats[1][TSTAT_SECONDS], timbuf[1] );
  fmtseconds( tstats[2][TSTAT_SECONDS], timbuf[2] );
  fmtseconds( tstats[3][TSTAT_SECONDS], timbuf[3] );
  fmtseconds( etime, timbuf[4] );
  sprintf( buf, sfmt, "Time",
	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  ctime = tstats[0][TSTAT_CPUSECONDS] + tstats[1][TSTAT_CPUSECONDS] +
    tstats[2][TSTAT_CPUSECONDS] + tstats[3][TSTAT_CPUSECONDS];
  fmtseconds( tstats[0][TSTAT_CPUSECONDS], timbuf[0] );
  fmtseconds( tstats[1][TSTAT_CPUSECONDS], timbuf[1] );
  fmtseconds( tstats[2][TSTAT_CPUSECONDS], timbuf[2] );
  fmtseconds( tstats[3][TSTAT_CPUSECONDS], timbuf[3] );
  fmtseconds( ctime, timbuf[4] );
  sprintf( buf, sfmt, "Cpu time",
	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  for ( i = 0; i < 4; i = i + 1 )
    {
      j = tstats[i][TSTAT_SECONDS];
      if ( j <= 0 )
	x[i] = 0.0;
      else
	x[i] = 100.0 * ((real) tstats[i][TSTAT_CPUSECONDS] / (real) j);
    }
  if ( etime <= 0 )
    x[4] = 0.0;
  else
    x[4] = 100.0 * (real) ctime / (real)etime;
  sprintf( buf, pfmt, "Cpu usage", x[0], x[1], x[2], x[3], x[4] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Phaser shots",
	 tstats[0][TSTAT_PHASERS], tstats[1][TSTAT_PHASERS],
	 tstats[2][TSTAT_PHASERS], tstats[3][TSTAT_PHASERS],
	 tstats[0][TSTAT_PHASERS] + tstats[1][TSTAT_PHASERS] +
	 tstats[2][TSTAT_PHASERS] + tstats[3][TSTAT_PHASERS] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Torps fired",
	 tstats[0][TSTAT_TORPS], tstats[1][TSTAT_TORPS],
	 tstats[2][TSTAT_TORPS], tstats[3][TSTAT_TORPS],
	 tstats[0][TSTAT_TORPS] + tstats[1][TSTAT_TORPS] +
	 tstats[2][TSTAT_TORPS] + tstats[3][TSTAT_TORPS] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Armies bombed",
	 tstats[0][TSTAT_ARMBOMB], tstats[1][TSTAT_ARMBOMB],
	 tstats[2][TSTAT_ARMBOMB], tstats[3][TSTAT_ARMBOMB],
	 tstats[0][TSTAT_ARMBOMB] + tstats[1][TSTAT_ARMBOMB] +
	 tstats[2][TSTAT_ARMBOMB] + tstats[3][TSTAT_ARMBOMB] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Armies captured",
	 tstats[0][TSTAT_ARMSHIP], tstats[1][TSTAT_ARMSHIP],
	 tstats[2][TSTAT_ARMSHIP], tstats[3][TSTAT_ARMSHIP],
	 tstats[0][TSTAT_ARMSHIP] + tstats[1][TSTAT_ARMSHIP] +
	 tstats[2][TSTAT_ARMSHIP] + tstats[3][TSTAT_ARMSHIP] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Planets taken",
	 tstats[0][TSTAT_CONQPLANETS], tstats[1][TSTAT_CONQPLANETS],
	 tstats[2][TSTAT_CONQPLANETS], tstats[3][TSTAT_CONQPLANETS],
	 tstats[0][TSTAT_CONQPLANETS] + tstats[1][TSTAT_CONQPLANETS] +
	 tstats[2][TSTAT_CONQPLANETS] + tstats[3][TSTAT_CONQPLANETS] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Coups",
	 tstats[0][TSTAT_COUPS], tstats[1][TSTAT_COUPS],
	 tstats[2][TSTAT_COUPS], tstats[3][TSTAT_COUPS],
	 tstats[0][TSTAT_COUPS] + tstats[1][TSTAT_COUPS] +
	 tstats[2][TSTAT_COUPS] + tstats[3][TSTAT_COUPS] );
  cdputs( buf, lin, col );
  
  lin = lin + 1;
  sprintf( buf, dfmt, "Genocides",
	 tstats[0][TSTAT_GENOCIDE], tstats[1][TSTAT_GENOCIDE],
	 tstats[2][TSTAT_GENOCIDE], tstats[3][TSTAT_GENOCIDE],
	 tstats[0][TSTAT_GENOCIDE] + tstats[1][TSTAT_GENOCIDE] +
	 tstats[2][TSTAT_GENOCIDE] + tstats[3][TSTAT_GENOCIDE] );
  cdputs( buf, lin, col );
  
  for ( i = 0; i < 4; i = i + 1 )
    if ( couptime[i] == 0 )
      timbuf[i][0] = EOS;
    else
      sprintf( timbuf[i], "%d", couptime[i] );
  
  if ( ! godlike )
    for ( i = 0; i < 4; i = i + 1 )
      if ( team != i )
	c_strcpy( "-", timbuf[i] );
      else if ( ! tcoupinfo[i] && timbuf[i][0] != EOS )
	c_strcpy( "?", timbuf[i] );
  
  timbuf[4][0] = EOS;
  
  lin = lin + 1;
  sprintf( buf, sfmt, "Coup time",
 	 timbuf[0], timbuf[1], timbuf[2], timbuf[3], timbuf[4] );
  cdputs( buf, lin, col );
  
  return;
  
}


/*##  userline - format user statistics */
/*  SYNOPSIS */
/*    int unum, snum */
/*    char buf() */
/*    int showgods, showteam */
/*    userline( unum, snum, buf, showgods, showteam ) */
/* Special hack: If snum is valid, the team and pseudonym are taken from */
/* the ship instead of the user. */
void userline( int unum, int snum, char *buf, int showgods, int showteam )
{
  int i, team;
  char ch, ch2, junk[MSGMAXLINE], timstr[20], name[MAXUSERPNAME];
  
  string hd="name          pseudonym           team skill  wins  loss mxkls  ships     time";
  
  
  if ( unum < 0 || unum >= MAXUSERS )
    {
      c_strcpy( hd, buf );
      return;
    }
  if ( ! ulive[unum] )
    {
      buf[0] = EOS;
      return;
    }
  
  ch2 = ' ';
  if ( showgods )
    {
      for ( i = 2; i < MAXOOPTIONS; i = i + 1)
	if ( uooption[unum][i] )
	  {
	    ch2 = '+';
	    break;
	  }
      if ( ch2 != '+' )
	if ( isagod(cuname[unum]) )
	  ch2 = '+';
    }
  
  /* If we were given a valid ship number, use it's information. */
  if ( snum > 0 && snum <= MAXSHIPS )
    {
      c_strcpy( spname[snum], name );
      team = steam[snum];
    }
  else
    {
      c_strcpy( upname[unum], name );
      team = uteam[unum];
    }
  
  /* Figure out which team he's on. */
  if ( uooption[unum][OOPT_MULTIPLE] && ! showteam )
    ch = 'M';
  else
    ch = chrteams[team];
  
  sprintf( junk, "%-12s %c%-21.21s %c %6.1f",
	 cuname[unum],
	 ch2,
	 name,
	 ch,
	 urating[unum] );
  
  fmtminutes( ( ustats[unum][USTAT_SECONDS] + 30 ) / 60, timstr );
  
  sprintf( buf, "%s %5d %5d %5d %5d %9s",
	 junk,
	 ustats[unum][USTAT_WINS],
	 ustats[unum][USTAT_LOSSES],
	 ustats[unum][USTAT_MAXKILLS],
	 ustats[unum][USTAT_ENTRIES],
	 timstr );
  
  return;
  
}


/*##  userlist - display the user list */
/*  SYNOPSIS */
/*    userlist( godlike ) */
void userlist( int godlike )
{
  int i, j, unum, nu, fuser, fline, lline, lin;
  static int uvec[MAXUSERS];
  int ch;
  
  /* Sort user numbers into uvec() in an insertion sort on urating(). */
  
  
  for (i=0; i<MAXUSERS; i++)
    uvec[i] = i;
  
  nu = 0;
  
  for ( unum = 0; unum < MAXUSERS; unum++)
    if ( ulive[unum])
      {
	for ( i = 0; i < nu; i++ )
	  if ( urating[uvec[i]] < urating[unum] )
	    {
	      for ( j = nu - 1; j >= i; j = j - 1 )
		uvec[j+1] = uvec[j];
	      break;
	    }
	uvec[i] = unum;
	nu++;
      }
  
  /* Do some screen setup. */
  cdclear();
  lin = 0;
  cdputc( "U S E R   L I S T", lin );
  
  lin = lin + 2;
  userline( -1, -1, cbuf, FALSE, FALSE );
  cdputs( cbuf, lin, 1 );
  
  for ( j = 0; cbuf[j] != EOS; j = j + 1 )
    if ( cbuf[j] != ' ' )
      cbuf[j] = '-';
  lin = lin + 1;
  cdputs( cbuf, lin, 1 );
  
  fline = lin + 1;				/* first line to use */
  lline = MSG_LIN1;				/* last line to use */
  fuser = 0;					/* first user in uvec */
  
  while (TRUE) /* repeat-while */
    {
      if ( ! godlike )
	if ( ! stillalive( csnum ) )
	  break;
      i = fuser;
      cdclrl( fline, lline - fline + 1 );
      lin = fline;
      while ( i < nu && lin <= lline )
	{
	  userline( uvec[i], -1, cbuf, godlike, FALSE );
	  cdputs( cbuf, lin, 1 );
	  i = i + 1;
	  lin = lin + 1;
	}
      if ( i >= nu )
	{
	  /* We're displaying the last page. */
	  putpmt( "--- press space when done ---", MSG_LIN2 );
	  cdrefresh( TRUE );
	  if ( iogtimed( &ch, 1 ) )
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
	  putpmt( "--- press space for more ---", MSG_LIN2 );
	  cdrefresh( TRUE );
	  if ( iogtimed( &ch, 1 ) )
	    if ( ch == TERM_EXTRA )
	      fuser = 0;			/* move to first page */
	    else if ( ch == ' ' )
	      fuser = i;			/* move to next page */
	    else
	      break;
	}
    }
  
  return;
  
}


/*##  userstats - display the user list */
/*  SYNOPSIS */
/*    userstats( godlike ) */
void userstats( int godlike )
{
  int i, j, unum, nu, fuser, fline, lline, lin;
  static int uvec[MAXUSERS];
  int ch;
  string hd="name         cpu  conq coup geno  taken bombed/shot  shots  fired   last entry";
  
  for (i=0; i<MAXUSERS; i++)
    uvec[i] = i;
  
  nu = 0;
  
  for ( unum = 0; unum < MAXUSERS; unum++)
    if ( ulive[unum])
      {
	for ( i = 0; i < nu; i++ )
	  if ( urating[uvec[i]] < urating[unum] )
	    {
	      for ( j = nu - 1; j >= i; j = j - 1 )
		uvec[j+1] = uvec[j];
	      break;
	    }
	uvec[i] = unum;
	nu++;
      }
  
  /* Do some screen setup. */
  cdclear();
  lin = 1;
  cdputc( "M O R E   U S E R   S T A T S", lin );
  
  lin = lin + 2;
  cdputs( "planets  armies    phaser  torps", lin, 34 );
  
  c_strcpy( hd, cbuf );
  lin = lin + 1;
  cdputs( cbuf, lin, 1 );
  
  for ( j = 0; cbuf[j] != EOS; j = j + 1 )
    if ( cbuf[j] != ' ' )
      cbuf[j] = '-';
  lin = lin + 1;
  cdputs( cbuf, lin, 1 );
  
  fline = lin + 1;				/* first line to use */
  lline = MSG_LIN1;				/* last line to use */
  fuser = 0;					/* first user in uvec */
  
  while (TRUE) /* repeat-while */
    {
      if ( ! godlike )
	if ( ! stillalive( csnum ) )
	  break;
      i = fuser;
      cdclrl( fline, lline - fline + 1 );
      lin = fline;
      while ( i < nu && lin <= lline )
	{
	  statline( uvec[i], cbuf );
	  cdputs( cbuf, lin, 1 );
	  i = i + 1;
	  lin = lin + 1;
	}
      if ( i >= nu )
	{
	  /* We're displaying the last page. */
	  putpmt( "--- press space when done ---", MSG_LIN2 );
	  cdrefresh( TRUE );
	  if ( iogtimed( &ch, 1 ) )
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
	  putpmt( "--- press space for more ---", MSG_LIN2 );
	  cdrefresh( TRUE );
	  if ( iogtimed( &ch, 1 ) )
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


/*##  statline - format a user stat line */
/*  SYNOPSIS */
/*    int unum */
/*    char buf() */
/*    statline( unum, buf ) */
void statline( int unum, char *buf )
{
  int i, j, length;
  char ch, junk[MSGMAXLINE], percent[MSGMAXLINE], morejunk[MSGMAXLINE];
  
  if ( unum < 0 || unum >= MAXUSERS )
    {
      buf[0] = EOS;
      return;
    }
  if ( ! ulive[unum] )
    {
      buf[0] = EOS;
      return;
    }
  
  if ( ustats[unum][USTAT_SECONDS] == 0 )
    c_strcpy( "- ", percent );
  else
    {
      i = 1000 * ustats[unum][USTAT_CPUSECONDS] / ustats[unum][USTAT_SECONDS];
      sprintf( percent, "%3d%%", (i + 5) / 10 );
    }
  
  sprintf( junk, "%-12s %4s %4d %4d %4d",
	 cuname[unum],
	 percent,
	 ustats[unum][USTAT_CONQUERS],
	 ustats[unum][USTAT_COUPS],
	 ustats[unum][USTAT_GENOCIDE] );
  
  sprintf( buf, "%s %6d %6d %4d %6d %5d",
	 junk,
	 ustats[unum][USTAT_CONQPLANETS],
	 ustats[unum][USTAT_ARMBOMB],
	 ustats[unum][USTAT_ARMSHIP],
	 ustats[unum][USTAT_PHASERS],
	 ustats[unum][USTAT_TORPS] );
  
  /* Convert zero counts to dashes. */
  ch = EOS;
  for ( i = 9; buf[i] != EOS; i = i + 1 )
    {
      if ( buf[i] == '0' )
	if ( ch == ' ' )
	  if ( buf[i+1] == ' ' || buf[i+1] == EOS )
	    buf[i] = '-';
      ch = buf[i];
    }
  
  
  
  sprintf( junk, " %16.16s", ulastentry[unum] );
  
  
  j = 0;
  for (i=0; i<6; i++)
    {
      morejunk[j++] = junk[i];
    }
  /* remove the seconds - ugh*/
  for (i=9; i < 17; i++)
    {
      morejunk[j++] = junk[i];
    }
  morejunk[j] = EOS;
  
  appstr( morejunk, buf );
  
  return;
  
}


/*##  zeroplanet - zero a planet (DOES SPECIAL LOCKING) */
/*  SYNOPSIS */
/*    int pnum, snum */
/*    zeroplanet( pnum, snum ) */
/*  NOTE */
/*    This routines ASSUMES you have the common area locked before you it. */
void zeroplanet( int pnum, int snum )
{
  int oteam, i; 
  
  oteam = pteam[pnum];
  pteam[pnum] = TEAM_NOTEAM;
  parmies[pnum] = 0;
  
  /* Make the planet not scanned. */
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    pscanned[pnum][i] = FALSE;
  
  if ( oteam != TEAM_SELFRULED && oteam != TEAM_NOTEAM )
    {
      /* Check whether that was the last planet owned by the vanquished. */
      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
	if ( pteam[i] == oteam )
	  return;
      /* Yes. */
      couptime[oteam] = rndint( MIN_COUP_MINUTES, MAX_COUP_MINUTES );
      tcoupinfo[oteam] = FALSE;		/* lost coup info */
      if ( snum > 0 && snum <= MAXSHIPS )
	{
	  ustats[suser[snum]][USTAT_GENOCIDE] =
	    ustats[suser[snum]][USTAT_GENOCIDE] + 1;
	  tstats[steam[snum]][TSTAT_GENOCIDE] =
	    tstats[steam[snum]][TSTAT_GENOCIDE] + 1;
	}
    }
  
  return;
  
}




