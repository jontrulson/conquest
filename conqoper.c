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
/*                                                                    */
/**********************************************************************/

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"
#include "color.h"

static char *conqoperId = "$Id$";

/*  conqoper - main program */
main(int argc, char *argv[])
{
  int i;
  char name[MAXUSERNAME];
  char msgbuf[128];
  static int RegenSysConf = FALSE;

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
	RegenSysConf = TRUE;
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
  

  if (RegenSysConf == TRUE)
    {
				/* need to be conq grp for this */
      if (setgid(ConquestGID) == -1)
	{
	  clog("conqoper: setgid(%d): %s",
	       ConquestGID,
	       sys_errlist[errno]);
	  fprintf(stderr, "conqoper: setgid(): failed\n");
	  exit(1);
	}
      
      
      GetSysConf(TRUE);		/* init defaults... */

      if (MakeSysConf() == ERR)
	exit(1);
      else
	exit(0);

      /* the process must exit in this block. */

    }
  

  if (GetSysConf(FALSE) == ERR)
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
      /* */
      ;
    }

  if (GetConf() == ERR)	/* use one if there, else defaults
				   A missing or out-of-date conquestrc file
				   will be ignored */
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
      /* */
      ;
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
  cdisplay = TRUE;
  cmaxlin = cdlins();			/* number of lines */
  
  cmaxcol = cdcols();			/* number of columns */

				/* send a message telling people
				   that a user has entered conqoper */
  sprintf(msgbuf, "User %s has entered conqoper.",
          name);

  stormsg(MSG_COMP, MSG_ALL, msgbuf);

  
  operate();
  
  cdend();
  
  exit(0);
  
}


/*  bigbang - fire a torp from every ship */
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
	if ( ! launch( snum, dir, 1, LAUNCH_NORMAL ) )
	  break;
	else
	  {
	    dir = mod360( dir + 40.0 );
	    cnt = cnt + 1;
	  }
  cprintf(MSG_LIN1,0,ALIGN_CENTER, 
  "#%d#bigbang: Fired #%d#%d #%d#torpedos, hoo hah won't they be surprised!", 
	InfoColor,SpecialColor,cnt,InfoColor );
  
  return;
  
}


/*  debugdisplay - verbose and ugly version of display */
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
  
  int i, j, unum, lin, tcol, dcol;
  real x;
  char buf[MSGMAXLINE];
  char *torpstr;
  
#define TOFF "OFF"
#define TLAUNCHING "LAUNCHING"
#define TLIVE "LIVE"
#define TDETONATE "DETONATE"
#define TFIREBALL "EXPLODING"
  
  
  cdclrl( 1, MSG_LIN1 - 1 ); 	/* don't clear the message lines */
  unum = suser[snum];
  
  lin = 1;
  tcol = 1;
  dcol = tcol + 10;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "    ship:");
  buf[0] = EOS;
  appship( snum, buf );
  if ( srobot[snum] )
    appstr( " (ROBOT)", buf );
  cprintf( lin, dcol,ALIGN_NONE,"#%d#%s",InfoColor,buf );
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      sx:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(sx[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      sy:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(sy[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     sdx:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(sdx[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     sdy:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(sdy[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  skills:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, 
	(oneplace(skills[snum] + sstrkills[snum])));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   swarp:");
  x = oneplace(swarp[snum]);
  if ( x == ORBIT_CW )
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, "ORBIT_CW");
  else if ( x == ORBIT_CCW )
    cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, "ORBIT_CCW");
  else
  	cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, x);
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  sdwarp:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(sdwarp[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   shead:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, round(shead[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  sdhead:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, round(sdhead[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, " sarmies:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, round(sarmies[snum]));
  
  lin = 1;
  tcol = 23;
  dcol = tcol + 12;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      name:");
  if ( spname[snum] != EOS )
  	cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, spname[snum]);
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  username:");
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
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     slock:");
  cprintf(lin+1,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "       dtt:");
  i = slock[snum];
  if ( -i >= 1 && -i <= NUMPLANETS )
    {
  	  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, pname[-i]);
	  cprintf(lin+1,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
		  round( dist( sx[snum], sy[snum], px[-i], py[-i] ) ));
    }
  else if ( i != 0 )
  	cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, i);
  lin+=2;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     sfuel:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, round(sfuel[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "       w/e:");
  sprintf( buf, "%d/%d", sweapons[snum], sengines[snum] );
  if ( swfuse[snum] > 0 || sefuse[snum] > 0 )
    {
      appstr( " (", buf );
      appint( swfuse[snum], buf );
      appchr( '/', buf );
      appint( sefuse[snum], buf );
      appchr( ')', buf );
    }
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      temp:");
  sprintf( buf, "%d/%d", round(swtemp[snum]), round(setemp[snum]) );
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   ssdfuse:");
  i = ssdfuse[snum];
  buf[0] = EOS;
  if ( i != 0 )
    {
      sprintf( buf, "%d ", i );
    }
  if ( scloaked[snum] )
    appstr( "(CLOAKED)", buf );
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",LabelColor, buf);
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      spid:");
  i = spid[snum];
  if ( i != 0 )
    {
/*      sprintf( buf, "%d", i ); */
  	  cprintf(lin,dcol,ALIGN_NONE,"#%d#%d",InfoColor, i);
    }
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "slastblast:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(slastblast[snum]));
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "slastphase:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(slastphase[snum]));
  
  lin = 1;
  tcol = 57;
  dcol = tcol + 12;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   sstatus:");
  buf[0] = EOS;
  appsstatus( sstatus[snum], buf );
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, " skilledby:");
  i = skilledby[snum];
  if ( i != 0 )
    {
      buf[0] = EOS;
      appkb( skilledby[snum], buf );
	  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    }
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   shields:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, round(sshields[snum]));
  if ( ! sshup[snum] )
  	cprintf(lin,dcol+5,ALIGN_NONE,"#%d#%c",InfoColor, 'D');
  else
  	cprintf(lin,dcol+5,ALIGN_NONE,"#%d#%c",InfoColor, 'U');
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   sdamage:");
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, round(sdamage[snum]));
  if ( srmode[snum] )
  	cprintf(lin,dcol+5,ALIGN_NONE,"#%d#%c",InfoColor, 'R');
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "  stowedby:");
  i = stowedby[snum];
  if ( i != 0 )
    {
      buf[0] = EOS;
      appship( i, buf );
  	  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    }
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   stowing:");
  i = stowing[snum];
  if ( i != 0 )
    {
      buf[0] = EOS;
      appship( i, buf );
  	  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    }
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "      swar:");
  buf[0] = '(';
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    if ( swar[snum][i] )
      buf[i+1] = chrteams[i];
    else
      buf[i+1] = '-';
  buf[NUMTEAMS+1] = ')';
  buf[NUMTEAMS+2] = EOS;
  fold( buf );
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
  
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "     srwar:");
  buf[0] = '(';
  for ( i = 0; i < NUMTEAMS; i = i + 1 )
    if ( srwar[snum][i] )
      buf[i+1] = chrteams[i];
    else
      buf[i+1] = '-';
  buf[NUMTEAMS+1] = ')';
  buf[NUMTEAMS+2] = EOS;
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
  
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   soption:");
  c_strcpy( "(gpainte)", buf );
  for ( i = 0; i < MAXOPTIONS; i = i + 1 )
    if ( soption[snum][i] )
      buf[i+1] = (char)toupper(buf[i+1]);
  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
  
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   uoption:");
  if ( unum >= 0 && unum < MAXUSERS )
    {
      c_strcpy( "(gpainte)", buf );
      for ( i = 0; i < MAXOPTIONS; i = i + 1 )
	if ( uoption[unum][i] )
	  buf[i + 1] = (char)toupper(buf[i + 1]);
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    }
  
  lin++;
  cprintf(lin,tcol,ALIGN_NONE,"#%d#%s",LabelColor, "   saction:");
  i = saction[snum];
  if ( i != 0 )
    {
      robstr( i, buf );
	  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
    }
  
  lin = (cmaxlin - (cmaxlin - MSG_LIN1)) - (MAXTORPS+1);  /* dwp */
  cprintf(lin,3,ALIGN_NONE,"#%d#%s",LabelColor,
	 "tstatus    tfuse    tmult       tx       ty      tdx      tdy     twar");
  for ( i = 0; i < MAXTORPS; i = i + 1 )
    {
      lin++;
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
	  cprintf(lin,3,ALIGN_NONE,"#%d#%s",InfoColor, torpstr);
      if ( tstatus[snum][i] != TS_OFF )
	{
	  cprintf(lin,13,ALIGN_NONE,"#%d#%6d",InfoColor, tfuse[snum][i]);
	  cprintf(lin,19,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(tmult[snum][i]));
	  cprintf(lin,28,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(tx[snum][i]));
	  cprintf(lin,37,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(ty[snum][i]));
	  cprintf(lin,46,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(tdx[snum][i]));
	  cprintf(lin,55,ALIGN_NONE,"#%d#%9g",InfoColor, oneplace(tdy[snum][i]));
	  buf[0] = '(';
	  for ( j = 0; j < NUMTEAMS; j++ )
	    if ( twar[snum][i][j] )
	      buf[j+1] = chrteams[j];
	    else
	      buf[j+1] = '-';
	  buf[NUMTEAMS+1] = ')';
	  buf[NUMTEAMS+2] = EOS;
	  cprintf(lin,67,ALIGN_NONE,"#%d#%s",InfoColor, buf);
	}
    }
  
  cdmove( 1, 1 );
  cdrefresh();
  
  return;
  
}


/*  debugplan - debugging planet list */
/*  SYNOPSIS */
/*    debugplan */
void debugplan(void)
{
  
  int i, j, k, cmd, lin, col, olin;
  int outattr;
  static int sv[NUMPLANETS + 1];
  char buf[MSGMAXLINE], junk[10], uninhab[20];
  char hd0[MSGMAXLINE*4];
  char *hd1="D E B U G G I N G  P L A N E T   L I S T";
  string hd2="planet        C T arm uih scan        planet        C T arm uih scan";
  char hd3[BUFFER_SIZE];
  int FirstTime = TRUE;
  int PlanetOffset;             /* offset into NUMPLANETS for this page */
  int PlanetIdx = 0;
  int Done;


  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(hd0,
	      "#%d#%s#%d#%s#%d#%s#%d#%s",
	      LabelColor,
		  hd1,
	      InfoColor,
		  "  ('",
	      SpecialColor,
		  "-", 
	      InfoColor,
		  "' = hidden)");

      for ( i = 1; i <= NUMPLANETS; i = i + 1 )
	sv[i] = i;
      sortplanets( sv );
    }
  
  strcpy( hd3, hd2 );
  for ( i = 0; hd3[i] != EOS; i++ )
    if ( hd3[i] != ' ' )
      hd3[i] = '-';

  PlanetIdx = 0;

  PlanetOffset = 1;
  cdclear();
  Done = FALSE;
  
  cdclear();
  
  Done = FALSE;
  do
    {
      cdclra(0, 0, MSG_LIN1 + 2, cdcols() - 1);
      PlanetIdx = 0;
      
      lin = 1;
      
      cprintf(lin,0,ALIGN_CENTER,"%s", hd0 );
      
      lin++;
      cprintf( lin,0,ALIGN_CENTER,"#%d#%s", LabelColor, hd2 );
      lin++;
      cprintf( lin,0,ALIGN_CENTER,"#%d#%s", LabelColor, hd3 );
      lin++;
      olin = lin;
      col = 6;
      
      PlanetIdx = 0;
      
      if (PlanetOffset <= NUMPLANETS)
        {
          while ((PlanetOffset + PlanetIdx) <= NUMPLANETS)
            {
              i = PlanetOffset + PlanetIdx;
              PlanetIdx++;
              k = sv[i];
	      
	      for ( j = 0; j < NUMTEAMS; j++ )
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
	      
	      switch(ptype[k])
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
		  outattr = A_BOLD;
		  break;
		case PLANET_GHOST:
		  outattr = NoColor;
		  break;
		default:
		  outattr = SpecialColor;
		  break;
		}
		  
	      cprintf(lin,col,ALIGN_NONE,"#%d#%-13s %c %c %3d %3s %4s",
		      outattr,pname[k], chrplanets[ptype[k]], chrteams[pteam[k]],
		      parmies[k], uninhab, junk );
	      
	      if ( ! preal[k] )
		cprintf(lin,col-2,ALIGN_NONE, "#%d#%c", SpecialColor, '-');
	      
	      lin++;
	      if ( lin == MSG_LIN2 )
		{
		  if (col == 44)
		    break;	/* out of while */
		  lin = olin;
		  col = 44;
		}
	      attrset(0);
	    } /* while */
	  
	  putpmt( "--- press [SPACE] to continue, q to quit ---", MSG_LIN2 );
          cdrefresh();
	  
          if (iogtimed( &cmd, 1 ))
            {                   /* got a char */
              if (cmd == 'q' || cmd == 'Q' || cmd == TERM_ABORT)
                {               /* quit */
                  Done = TRUE;
                }
              else
                {               /* some other key... */
		  /* setup for new page */
                  PlanetOffset += PlanetIdx;
                  if (PlanetOffset > NUMPLANETS)
                    {           /* pointless to continue */
                      Done = TRUE;
                    }
                }
            }
	  
	  /* didn't get a char, update */
        } /* if PlanetOffset <= NUMPLANETS */
      else
	Done = TRUE;            /* else PlanetOffset > NUMPLANETS */
      
    } while(Done != TRUE); /* do */
  
  return;
  
}


/*  gplanmatch - GOD's check if a string matches a planet name */
/*  SYNOPSIS */
/*    int gplanmatch, pnum */
/*    char str() */
/*    int status */
/*    status = gplanmatch( str, pnum ) */
int gplanmatch( char str[], int *pnum )
{
  int i;
  
  if ( alldig( str ) == TRUE )
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


/*  kiss - give the kiss of death */
/*  SYNOPSIS */
/*    kiss */
void kiss(int snum, int prompt_flg)
{
  
  int i, unum;
  char ch, buf[MSGMAXLINE], mbuf[MSGMAXLINE], ssbuf[MSGMAXLINE];
  int didany;
  char *prompt_str = "Kill what (<cr> for driver)? ";
  char *kill_driver_str = "Killing the driver."; 
  char *cant_kill_ship_str = "You can't kill ship %c%d (%s) status (%s).";
  char *kill_ship_str1 = "Killing ship %c%d (%s).";
  char *kill_ship_str2 = "Killing ship %c%d (%s) user (%s).";
  char *nobody_str = "Nobody here but us GODs.";
  char *no_user_str = "No such user.";
  char *no_ship_str = "No such ship.";
  char *not_flying_str = "User %s (%s) isn't flying right now.";

  /* Find out what to kill. */
  if (prompt_flg)
    {
      cdclrl( MSG_LIN1, 2 );
      if (snum == 0)
	buf[0] = EOS;
      else
	sprintf(buf, "%d", snum);

      ch = (char)cdgetx( prompt_str, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
      if ( ch == TERM_ABORT )
	{
	  cdclrl( MSG_LIN1, 1 );
	  cdmove( 1, 1 );
	  return;
	}
      delblanks( buf );
    }
  else 
    {
      if (snum == 0)
	return;
      else
	sprintf(buf,"%d",snum);
    }
  
  /* Kill the driver? */
  if ( buf[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 ); 
      sprintf(mbuf,"%s", kill_driver_str);
      cdputs( mbuf, MSG_LIN1, 1 );
      if ( confirm() )
	if ( *drivstat == DRS_RUNNING )
	  *drivstat = DRS_KAMIKAZE;
      cdclrl( MSG_LIN1, 2 );
      cdmove( 1, 1 );
      return;
    }
  
  /* Kill a ship? */
  if ( alldig( buf ) == TRUE )
    {
      i = 0;
      safectoi( &snum, buf, i );		/* ignore status */
      if ( snum < 1 || snum > MAXSHIPS )
		cdputs( no_ship_str, MSG_LIN2, 1 );
      else if ( sstatus[snum] != SS_LIVE ) {
		cdclrl( MSG_LIN1, 1 );
	    ssbuf[0] = EOS; 
	    appsstatus( sstatus[snum], ssbuf);
		sprintf(mbuf, cant_kill_ship_str,
			chrteams[steam[snum]], snum, spname[snum], ssbuf);
	    cdputs( mbuf, MSG_LIN1, 1 );
	  }
      else {
		  cdclrl( MSG_LIN1, 1 );
		  sprintf(mbuf, kill_ship_str1,
			chrteams[steam[snum]], snum, spname[snum]);
	      cdputs( mbuf, MSG_LIN1, 1 );
		  if ( confirm() )
		  {
			  killship( snum, KB_GOD );
			  cdclrl( MSG_LIN2, 1 );
		  }
		  cdclrl( MSG_LIN1, 1 );
	}
      cdmove( 1, 1 );
      return;
    }
  
  /* Kill EVERYBODY? */
  if ( stmatch( buf, "all", FALSE ) )
    {
      didany = FALSE;
      for ( snum = 1; snum <= MAXSHIPS; snum++ )
	if ( sstatus[snum] == SS_LIVE )
	  {
	    didany = TRUE;
	    cdclrl( MSG_LIN1, 1 );
		sprintf(buf, kill_ship_str1,
			chrteams[steam[snum]], snum, spname[snum]);
	    cdputs( buf, MSG_LIN1, 1 );
	    if ( confirm() )
	      killship( snum, KB_GOD );
	  }
      if ( didany )
	cdclrl( MSG_LIN1, 2 );
      else
	cdputs( nobody_str, MSG_LIN2, 1 );
      cdmove( 1, 1 );
      return;
    }
  
  /* Kill a user? */
  if ( ! gunum( &unum, buf ) )
    {
      cdputs( no_user_str, MSG_LIN2, 1 );
      cdmove( 0, 0 );
      return;
    }
  
  /* Yes. */
  didany = FALSE;
  for ( snum = 1; snum <= MAXSHIPS; snum++ )
    if ( sstatus[snum] == SS_LIVE )
      if ( suser[snum] == unum )
	{
	  didany = TRUE;
	  cdclrl( MSG_LIN1, 1 );
	  sprintf(mbuf, kill_ship_str2,
		chrteams[steam[snum]], snum, spname[snum], buf);
	  cdputs( mbuf, MSG_LIN1, 1 );
	  if ( confirm() )
	    killship( snum, KB_GOD );
	}
  if ( ! didany ) {
	cdclrl( MSG_LIN1, 1 );
	sprintf(mbuf, not_flying_str, cuname[unum], upname[unum]);
	cdputs( mbuf, MSG_LIN1, 1 );
  }
  else
    cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*  opback - put up the background for the operator program */
/*  SYNOPSIS */
/*    int lastrev, savelin */
/*    opback( lastrev, savelin ) */
void opback( int lastrev, int *savelin )
{
  int i, lin, col;
  char cbuf[MSGMAXLINE];
  extern char *ConquestVersion;
  extern char *ConquestDate;
  
  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#(#%d#%%c#%d#) - %%s",
	      LabelColor,
	      InfoColor,
	      LabelColor);
	}

  cdclear();
  
  lin = 1;
  if ( lastrev == COMMONSTAMP ) {
    attrset(NoColor|A_BOLD);
    cdputc( "CONQUEST OPERATOR PROGRAM", lin );
    attrset(YellowLevelColor);
    sprintf( cbuf, "%s (%s)",
	   ConquestVersion, ConquestDate);
    cdputc( cbuf, lin+1 );
  }
  else
    {
      attrset(RedLevelColor);
      sprintf( cbuf, "CONQUEST COMMON BLOCK MISMATCH %d != %d",
	       lastrev, COMMONSTAMP );
      cdputc( cbuf, lin );
      attrset(0);
    }

  EnableSignalHandler();	/* enable trapping of interesting signals */

  lin++;
  
  lin+=2;
  *savelin = lin;
  lin+=3;
  
  cprintf(lin,0,ALIGN_CENTER,"#%d#%s",LabelColor, "Options:");
  lin+=2;
  i = lin;
  
  col = 5;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'f', "flip the open/closed flag");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'd', "flip the doomsday machine!");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'h', "hold the driver");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'I', "initialize");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'b', "big bang");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'H', "user history");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, '/', "player list");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, '\\', "full player list");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, '?', "planet list");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, '$', "debugging planet list");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'p', "edit a planet");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'w', "watch a ship");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'i', "info");
  
  lin = i;
  col = 45;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'r', "create a robot ship");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'L', "review messages");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'm', "message from GOD");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'T', "team stats");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'U', "user stats");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'S', "more user stats");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 's', "special stats page");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'a', "add a user");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'e', "edit a user");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'R', "resign a user");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'k', "kiss of death");
  lin++;
  cprintf(lin,col,ALIGN_NONE,sfmt, 'q', "exit");
  
  return;
  
}

/*  operate - main part of the conquest operator program */
/*  SYNOPSIS */
/*    operate */
void operate(void)
{
  
  int i, lin, savelin;
  int redraw, now, readone;
  int lastrev, msgrand;
  char buf[MSGMAXLINE], junk[MSGMAXLINE];
  char xbuf[MSGMAXLINE];
  int ch;
  
  *glastmsg = *lastmsg;
  glname( buf );
  if ( gunum( &i, buf ) )
    uooption[i][MAXOOPTIONS] = TRUE;

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

      if (*commonrev == COMMONSTAMP)
	{
	  /* game status */
	  if ( *closed )
	    c_strcpy( "CLOSED", junk );
	  else
	    c_strcpy( "open", junk );
	  
	  /* driver status */
	  
	  switch ( *drivstat )
	    {
	    case DRS_OFF:
	      c_strcpy( "OFF", xbuf );
	      break;
	    case DRS_RESTART:
	      c_strcpy( "RESTART", xbuf );
	      break;
	    case DRS_STARTING:
	      c_strcpy( "STARTING", xbuf );
	      break;
	    case DRS_HOLDING:
	      c_strcpy( "HOLDING", xbuf );
	      break;
	    case DRS_RUNNING:
	      c_strcpy( "on", xbuf );
	      break;
	    case DRS_KAMIKAZE:
	      c_strcpy( "KAMIKAZE", xbuf );
	      break;
	    default:
	      c_strcpy( "???", xbuf );
	    }
	  
	  /* eater status */
	  i = *dstatus;
	  if ( i == DS_OFF )
	    c_strcpy( "off", buf );
	  else if ( i == DS_LIVE )
	    {
	      c_strcpy( "ON (", buf );
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
	  cprintf(lin,0,ALIGN_CENTER,"#%d#%s#%d#%s#%d#%s#%d#%s#%d#%s#%d#%s",
		  LabelColor,"game ",InfoColor,junk,LabelColor,", driver ",InfoColor,
		  xbuf,LabelColor,", eater ",InfoColor,buf);
	  
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
      
	  lin++;
	  cdclrl( lin, 1 );
	  attrset(SpecialColor);
	  cdputc( buf, lin );
	  attrset(0);
      

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
	  cdrefresh();
	  /* Un-read message, if there's a chance it got garbaged. */
	  if ( readone )
	    if ( iochav() )
	      *glastmsg = modp1( *glastmsg - 1, MAXMESSAGES );

	} /* *commonrev != COMMONSTAMP */
      else 
	{ /* COMMONBLOCK MISMATCH */

	  cprintf(4, 0, ALIGN_CENTER, "#%d#You must (I)nitialize (E)verything.",
		  RedLevelColor);
	  cdmove( 1, 1 );
	  cdrefresh();

	}


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
	  if ( confirm() )
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
	  else if ( confirm() )
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
	  kiss(0,TRUE);
	  break;
	case 'L':
	  review( MSG_GOD, *glastmsg );
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
	  userstats( TRUE , 0 ); /* we're always neutral ;-) - dwp */
	  redraw = TRUE;
	  break;
	case 'T':
	  opteamlist();
	  redraw = TRUE;
	  break;
	case 'U':
	  userlist( TRUE, 0 );
	  redraw = TRUE;
	  break;
	case 'w':
	  watch();
	  stoptimer();		/* to be sure */
	  redraw = TRUE;
	  break;
	case '/':
	  playlist( TRUE, FALSE, 0 );
	  redraw = TRUE;
	  break;
	case '\\':
	  playlist( TRUE, TRUE, 0 );
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


/*  opinfo - do an operator info command */
/*  SYNOPSIS */
/*    int snum */
/*    opinfo( snum ) */
void opinfo( int snum )
{
  int i, j, now[8];
  char ch;
  string pmt="Information on: ";
  string huh="I don't understand.";
  
  cdclrl( MSG_LIN1, 2 );
  
  cbuf[0] = EOS;
  ch = (char)cdgetx( pmt, MSG_LIN1, 1, TERMS, cbuf, MSGMAXLINE );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  delblanks( cbuf );
  fold( cbuf );
  if ( cbuf[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  
  if ( cbuf[0] == 's' && alldig( &cbuf[1] ) == TRUE )
    {
      i = 0;
      safectoi( &j, &cbuf[1], i );		/* ignore status */
      infoship( j, snum );
    }
  else if ( alldig( cbuf ) == TRUE )
    {
      i = 0;
      safectoi( &j, cbuf, i );		/* ignore status */
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


/*  opinit - handle the various kinds of initialization */
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
  attrset(LabelColor);
  cdputc( "Conquest Initialization", lin );
  
  lin+=3;
  col = icol - 2;
  c_strcpy( "(r)obots", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  attrset(LabelColor);
  cdputs( "<-", lin, col );
  col = col + 4;
  c_strcpy( "(e)verything", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  attrset(LabelColor);
  cdputs( "->", lin, col );
  col = col + 4;
  c_strcpy( "(z)ero everything", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  
  col = icol + 20;
  lin+=3;
  attrset(LabelColor);
  cdput( '|', lin, col );
  lin++;
  cdput( 'v', lin, col );
  lin+=3;
  
  col = icol;
  c_strcpy( "(s)hips", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  attrset(LabelColor);
  cdputs( "<-", lin, col );
  col = col + 4;
  c_strcpy( "(u)niverse", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  attrset(LabelColor);
  cdputs( "->", lin, col );
  col = col + 4;
  c_strcpy( "(g)ame", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 4;
  attrset(LabelColor);
  cdputs( "->", lin, col );
  col = col + 4;
  c_strcpy( "(p)lanets", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  
  col = icol + 20;
  lin+=3;
  attrset(LabelColor);
  cdput( '|', lin, col );
  lin++;
  cdput( 'v', lin, col );
  lin+=3;
  
  col = icol + 15;
  c_strcpy( "(m)essages", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  col = col + i + 8;
  c_strcpy( "(l)ockwords", buf );
  i = strlen( buf );
  attrset(InfoColor);
  cdputs( buf, lin, col+1 );
  attrset(A_BOLD);
  cdbox( lin-1, col, lin+1, col+i+1 );
  
  while (TRUE)  /*repeat */
    {
      lin = MSG_LIN1;
      col = 30;
      cdclrl( lin, 1 );
      attrset(InfoColor);
      buf[0] = EOS;
      ch = (char)cdgetx( pmt, lin, col, TERMS, buf, MSGMAXLINE );
      cdclrl( lin, 1 );
      cdputs( pmt, lin, col );
  	  attrset(0);
      col = col + strlen( pmt );
      if ( ch == TERM_ABORT || buf[0] == EOS )
	break;
      switch ( buf[0] )
	{
	case 'e':
	  cdputs( "everything", lin, col );
	  if ( confirm() )
	    {
	      initeverything();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'z':
	  cdputs( "zero everything", lin, col );
	  if ( confirm() )
	    zeroeverything();
	  break;
	case 'u':
	  cdputs( "universe", lin, col );
	  if ( confirm() )
	    {
	      inituniverse();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'g':
	  cdputs( "game", lin, col );
	  if ( confirm() )
	    {
	      initgame();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'p':
	  cdputs( "planets", lin, col );
	  if ( confirm() )
	    {
	      initplanets();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 's':
	  cdputs( "ships", lin, col );
	  if ( confirm() )
	    {
	      clearships();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'm':
	  cdputs( "messages", lin, col );
	  if ( confirm() )
	    {
	      initmsgs();
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'l':
	  cdputs( "lockwords", lin, col );
	  if ( confirm() )
	    {
	      PVUNLOCK(lockword);
	      PVUNLOCK(lockmesg);
	      *commonrev = COMMONSTAMP;
	    }
	  break;
	case 'r':
	  cdputs( "robots", lin, col );
	  if ( confirm() )
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


/*  oppedit - edit a planet's characteristics */
/*  SYNOPSIS */
/*    oppedit */
void oppedit(void)
{
  
  int i, j, lin, col, datacol;
  static int pnum = PNUM_EARTH;
  real x;
  int ch;
  char buf[MSGMAXLINE];
  int attrib;

  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#(#%d#%%s#%d#)   %%s",
	      LabelColor,
	      InfoColor,
	      LabelColor);
	}

  
  col = 4;
  datacol = col + 28;
  
  cdredo();
  cdclear();
  while (TRUE) /*repeat*/
    {
      
      /* Display the planet. */
      i = 20;			/* i = 10 */
      j = 57;


	  /* colorize planet name according to type */
      if ( ptype[pnum] == PLANET_SUN )
	attrib = RedLevelColor;
      else if ( ptype[pnum] == PLANET_CLASSM )
	attrib = GreenLevelColor;
      else if ( ptype[pnum] == PLANET_DEAD )
	attrib = YellowLevelColor;
      else if ( ptype[pnum] == PLANET_CLASSA ||
		ptype[pnum] == PLANET_CLASSO ||
		ptype[pnum] == PLANET_CLASSZ ||
		ptype[pnum] == PLANET_GHOST )   /* white as a ghost ;-) */
	attrib = A_BOLD;
      else 
	attrib = SpecialColor;
      
				/* if we're doing a sun, use yellow
				   else use attrib set above */
      if (ptype[pnum] == PLANET_SUN)
	attrset(YellowLevelColor);
      else
	attrset(attrib);
      puthing(ptype[pnum], i, j );
      attrset(0);

	  /* suns have red cores, others are cyan. */
      if (ptype[pnum] == PLANET_SUN)
	attrset(RedLevelColor);
      else
	attrset(InfoColor);
      cdput( chrplanets[ptype[pnum]], i, j + 1);
      attrset(0);

      sprintf(buf, "%s\n", pname[pnum]);
      attrset(attrib);
      cdputs( buf, i + 1, j + 2 ); /* -[] */
      attrset(0);
      
      /* Display info about the planet. */
      lin = 4;
      i = pnum;
      cprintf(lin,col,ALIGN_NONE,sfmt, "p", "  Planet:\n");
      cprintf( lin,datacol,ALIGN_NONE,"#%d#%s (%d)", 
		attrib, pname[pnum], pnum );
      
      lin++;
      i = pprimary[pnum];
      if ( i == 0 )
	{
      lin++;
	  
	  x = porbvel[pnum];
	  if ( x == 0.0 )
	    cprintf(lin,col,ALIGN_NONE,sfmt, "v", "  Stationary\n");
	  else
	    {
	      cprintf(lin,col,ALIGN_NONE,sfmt, "v", "Velocity:\n");
	      cprintf( lin,datacol,ALIGN_NONE,"#%d#Warp %f", 
			InfoColor, oneplace(x) );
	    }
	}
      else
	{
	  cprintf(lin,col,ALIGN_NONE,sfmt, "o", "  Orbiting:\n");
	  cprintf( lin,datacol,ALIGN_NONE,"#%d#%s (%d)", 
		InfoColor, pname[i], i );
	  
      lin++;
	  cprintf(lin,col,ALIGN_NONE,sfmt, "v", "  Orbit velocity:\n");
	  cprintf( lin,datacol,ALIGN_NONE,"#%d#%.1f degrees/minute", 
		InfoColor, porbvel[pnum] );
	}
      
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "r", "  Radius:\n");
      cprintf( lin,datacol,ALIGN_NONE, "#%d#%.1f", InfoColor, porbrad[pnum] );
      
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "a", "  Angle:\n");
      cprintf( lin,datacol,ALIGN_NONE, "#%d#%.1f", InfoColor, porbang[pnum] );
      
      lin++;
      i = ptype[pnum];
      cprintf(lin,col,ALIGN_NONE,sfmt, "t", "  Type:\n");
      cprintf( lin,datacol,ALIGN_NONE, "#%d#%s (%d)", InfoColor, ptname[i], i );
      
      lin++;
      i = pteam[pnum];
      if ( pnum <= NUMCONPLANETS )
	  {
		cprintf(lin,col,ALIGN_NONE,"#%d#%s",LabelColor,"        Owner team:\n");
	  }
      else
		cprintf(lin,col,ALIGN_NONE,sfmt, "T", "  Owner team:\n");
      cprintf( lin,datacol,ALIGN_NONE, "#%d#%s (%d)", InfoColor,tname[i], i );
      
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "x,y", "Position:\n");
      cprintf( lin,datacol,ALIGN_NONE, "#%d#%.1f, %.1f",
		InfoColor, px[pnum], py[pnum] );
      
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "A", "  Armies:\n");
      cprintf( lin,datacol,ALIGN_NONE, "#%d#%d", InfoColor, parmies[pnum] );
      
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "s", "  Scanned by:\n");
      buf[0] = '(';
      for ( i = 1; i <= NUMTEAMS; i = i + 1 )
		if ( pscanned[pnum][i - 1] )
		  buf[i] = chrteams[i - 1];
		else
		  buf[i] = '-';
      buf[NUMTEAMS+1] = ')';
      buf[NUMTEAMS+2] = '\0';
      cprintf( lin, datacol,ALIGN_NONE, "#%d#%s",InfoColor, buf);
      
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "u", "  Uninhabitable time:\n");
      cprintf( lin,datacol,ALIGN_NONE, "#%d#%d", 
		InfoColor, puninhabtime[pnum] );
      
      lin++;
      if ( preal[pnum] )
		  cprintf(lin,col,ALIGN_NONE,sfmt, "-", "  Visible\n");
      else
		  cprintf(lin,col,ALIGN_NONE,sfmt, "+", "  Hidden\n");
      
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "n", "  Change planet name\n");
      
      lin++;
      cprintf(lin,col,ALIGN_NONE,sfmt, "<>", 
		" decrement/increment planet number\n");
      
      cdclra(MSG_LIN1, 0, MSG_LIN1 + 2, cdcols() - 1);
      
      cdmove( 0, 0 );
      cdrefresh();
      
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
	  if ( ch == TERM_ABORT || buf[0] == EOS )
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
	  cdrefresh();
	  ch = (char)toupper( iogchar() );
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
	  if ( ch == TERM_ABORT || buf[0] == EOS )
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
	case '>': /* forward rotate planet number - dwp */
	case KEY_RIGHT:
	case KEY_UP:
	  pnum = mod( pnum + 1, NUMPLANETS );
	  pnum = (pnum == 0) ? NUMPLANETS : pnum;
	  break;
	case '<':  /* reverse rotate planet number - dwp */
	case KEY_LEFT:
	case KEY_DOWN:
	  pnum = (pnum >= 0) ? -pnum : pnum; 
	  pnum = mod( (NUMPLANETS + 1) - (pnum + 1), NUMPLANETS + 1 );
	  pnum = (pnum == 0) ? NUMPLANETS : pnum;
	  break;
	case ' ':
	  /* do no-thing */
	  break;
	case 0x0c:
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


/*  opplanlist - display the planet list for an operator */
/*  SYNOPSIS */
/*    opplanlist */
void opplanlist(void)
{
  planlist( TEAM_NOTEAM, 0 );		/* we get extra info */
}


/*  opresign - resign a user */
/*  SYNOPSIS */
/*    opresign */
void opresign(void)
{
  
  int unum;
  char ch, buf[MSGMAXLINE];
  
  cdclrl( MSG_LIN1, 2 );
  buf[0] = EOS;
  ch = (char)cdgetx( "Resign user: ", MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
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
      cdrefresh();
      c_sleep( 1.0 );
    }
  else if ( confirm() )
    resign( unum );
  cdclrl( MSG_LIN1, 2 );
  
  return;
  
}


/*  oprobot - handle gratuitous robot creation */
/*  SYNOPSIS */
/*    oprobot */
void oprobot(void)
{
  
  int i, j, snum, unum, num, anum;
  char ch, buf[MSGMAXLINE];
  int warlike;
  char xbuf[MSGMAXLINE];
  
  cdclrl( MSG_LIN1, 2 );
  buf[0] = EOS;
  ch = (char)cdgetx( "Enter username for new robot (Orion, Federation, etc): ",
	      MSG_LIN1, 1, TERMS, buf, MAXUSERNAME );
  if ( ch == TERM_ABORT || buf[0] == EOS )
    {
      cdclrl( MSG_LIN1, 1 );
      return;
    }
  /* catch lowercase, uppercase typos - dwp */
  strcpy(xbuf,buf);
  j = strlen(xbuf);
  buf[0] = (char)toupper(xbuf[0]);
  if (j>1)
  	for (i=1;i<j && xbuf[i] != EOS;i++)
		buf[i] = (char)tolower(xbuf[i]);
  delblanks( buf );
  if ( ! gunum( &unum, buf ) )
    {
				/* un-upper case first char and
				   try again */
      buf[0] = (char)tolower(buf[0]);
      if ( ! gunum( &unum, buf ) )
	{
	  cdputs( "No such user.", MSG_LIN2, 1 );
	  return;
	}
    }
  
  /* Defaults. */
  num = 1;
  warlike = FALSE;
  
  if ( ch == TERM_EXTRA )
    {
      buf[0] = EOS;
      ch = (char)cdgetx( "Enter number desired (TAB for warlike): ",
		  MSG_LIN2, 1, TERMS, buf, MAXUSERNAME );
      if ( ch == TERM_ABORT )
	{
	  cdclrl( MSG_LIN1, 2 );
	  return;
	}
      warlike = ( ch == TERM_EXTRA );
      delblanks( buf );
      i = 0;
      safectoi( &num, buf, i );
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


/*  opstats - display operator statistics */
/*  SYNOPSIS */
/*    opstats */
void opstats(void)
{
  
  int i, lin, col;
  unsigned long size;
  char buf[MSGMAXLINE], junk[MSGMAXLINE*2], timbuf[32];
  int ch;
  real x;
  string sfmt="#%d#%32s #%d#%12s\n";
  string tfmt="#%d#%32s #%d#%20s\n";
  string pfmt="#%d#%32s #%d#%11.1f%%\n";
  
  col = 8;
  cdclear();
  do /*repeat*/
    {
      lin = 2;
      fmtseconds( *ccpuseconds, timbuf );
      cprintf( lin,col,ALIGN_NONE,sfmt, 
		LabelColor,"Conquest cpu time:", InfoColor,timbuf );
      
      lin++;
      i = *celapsedseconds;
      fmtseconds( i, timbuf );
      cprintf( lin,col,ALIGN_NONE,sfmt, 
		LabelColor,"Conquest elapsed time:", InfoColor,timbuf );
      
      lin++;
      if ( i == 0 )
	x = 0.0;
      else
	x = oneplace( 100.0 * creal(*ccpuseconds) / creal(i) );
      cprintf( lin,col,ALIGN_NONE,pfmt, 
		LabelColor,"Conquest cpu usage:", InfoColor,x);
      
      lin+=2;
      fmtseconds( *dcpuseconds, timbuf );
      cprintf( lin,col,ALIGN_NONE,sfmt, 
		LabelColor,"Conqdriv cpu time:", InfoColor,timbuf );
      
      lin++;
      i = *delapsedseconds;
      fmtseconds( i, timbuf );
      cprintf( lin,col,ALIGN_NONE,sfmt, 
		LabelColor,"Conqdriv elapsed time:", InfoColor,timbuf );
      
      lin++;
      if ( i == 0 )
	x = 0.0;
      else
	x = oneplace( 100.0 * creal(*dcpuseconds) / creal(i) );
      cprintf( lin,col,ALIGN_NONE,pfmt, 
		LabelColor,"Conqdriv cpu usage:", InfoColor,x);
      
      lin+=2;
      fmtseconds( *rcpuseconds, timbuf );
      cprintf( lin,col,ALIGN_NONE,sfmt, 
		LabelColor,"Robot cpu time:", InfoColor,timbuf );
      
	  lin++;
      i = *relapsedseconds;
      fmtseconds( i, timbuf );
      cprintf( lin,col,ALIGN_NONE,sfmt, 
		LabelColor,"Robot elapsed time:", InfoColor,timbuf );
      
      lin++;
      if ( i == 0 )
	x = 0.0;
      else
	x = ( 100.0 * creal(*rcpuseconds) / creal(i) );
      cprintf( lin,col,ALIGN_NONE,pfmt, 
		LabelColor,"Robot cpu usage:", InfoColor,x);
      
      lin+=2;
      cprintf( lin,col,ALIGN_NONE,tfmt, 
		LabelColor,"Last initialize:", InfoColor,inittime);
      
      lin++;
      cprintf( lin,col,ALIGN_NONE,tfmt, 
		LabelColor,"Last conquer:", InfoColor,conqtime);
      
      lin++;
      fmtseconds( *playtime, timbuf );
      cprintf( lin,col,ALIGN_NONE,sfmt, 
		LabelColor,"Driver time:", InfoColor,timbuf);
      
      lin++;
      fmtseconds( *drivtime, timbuf );
      cprintf( lin,col,ALIGN_NONE,sfmt, 
		LabelColor,"Play time:", InfoColor,timbuf);
      
      lin++;
      cprintf( lin,col,ALIGN_NONE,tfmt, 
		LabelColor,"Last upchuck:", InfoColor,lastupchuck);
      
      lin++;
      getdandt( timbuf );
      cprintf( lin,col,ALIGN_NONE,tfmt, 
		LabelColor,"Current time:", InfoColor,timbuf);
      
      lin+=2;
      if ( drivowner[0] != EOS )
	sprintf( junk, "%d #%d#(#%d#%s#%d#)", 
		 *drivpid,LabelColor,SpecialColor, drivowner,LabelColor );
      else if ( *drivpid != 0 )
	sprintf( junk, "%d", *drivpid );
      else
	junk[0] = EOS;

      if (junk[0] == EOS)
	cprintf( lin,col,ALIGN_NONE, 
		 "#%d#drivsecs = #%d#%03d#%d#, drivcnt = #%d#%d\n",
		 LabelColor, InfoColor, 
		 *drivsecs,LabelColor,InfoColor,*drivcnt);
      else
	cprintf( lin,col,ALIGN_NONE, 
                 "#%d#%s#%d#, drivsecs = #%d#%03d#%d#, drivcnt = #%d#%d\n",
		 InfoColor,junk,LabelColor,InfoColor, 
                 *drivsecs,LabelColor,InfoColor,*drivcnt);
      
      lin++;
      comsize( &size );
      cprintf( lin,col,ALIGN_NONE, 
	       "#%d#%u #%d#bytes (out of #%d#%d#%d#) in the common block.\n",
	       InfoColor,size,LabelColor,InfoColor, SIZEOF_COMMONBLOCK,LabelColor );
      
      lin++;
      sprintf( buf, "#%d#Common ident is #%d#%d", 
	       LabelColor,InfoColor,*commonrev);
      if ( *commonrev != COMMONSTAMP )
	{
	  sprintf( junk, " #%d#(binary ident is #%d#%d#%d#)\n", 
		   LabelColor,InfoColor,COMMONSTAMP,LabelColor );
	  appstr( junk, buf );
	}
      cprintf( lin,col,ALIGN_NONE, "%s",buf);
      
      cdmove( 1, 1 );
      attrset(0);
      cdrefresh();
    }
  while ( !iogtimed( &ch, 1 ) ); /* until */
  
  return;
  
}


/*  opteamlist - display the team list for an operator */
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
      cdrefresh();
    }
  while ( !iogtimed( &ch, 1 ) ); /* until */
  
}


/*  opuadd - add a user */
/*  SYNOPSIS */
/*    opuadd */
void opuadd(void)
{
  
  int i, unum, team;
  char ch;
  char buf[MSGMAXLINE], junk[MSGMAXLINE], name[MSGMAXLINE];
  
  cdclrl( MSG_LIN1, 2 );
  name[0] = EOS;
  ch = (char)cdgetx( "Add user: ", MSG_LIN1, 1, TERMS, name, MAXUSERNAME );
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
      cdrefresh();
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
      buf[0] = EOS;
      ch = (char)cdgetx( junk, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
      if ( ch == TERM_ABORT )
	{
	  cdclrl( MSG_LIN1, 1 );
	  return;
	}
      else if ( ch == TERM_EXTRA && buf[0] == EOS )
	team = rndint( 0, NUMTEAMS - 1);
      else
	{
	  ch = (char)toupper( buf[0] );
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
  buf[i] = (char)toupper( buf[i] );
  buf[MAXUSERPNAME] = EOS;
  if ( ! c_register( name, buf, team, &unum ) )
    {
      cdputs( "Error adding new user.", MSG_LIN2, 1 );
      cdmove( 0, 0 );
      cdrefresh();
      c_sleep( 1.0 );
    }
  cdclrl( MSG_LIN1, 2 );
  
  return;

}


/*  opuedit - edit a user */
/*  SYNOPSIS */
/*    opuedit */
void opuedit(void)
{
  
#define MAXUEDITROWS (MAXOPTIONS+2) 
  int i, unum, row = 1, lin, olin, tcol, dcol, lcol, rcol;
  char buf[MSGMAXLINE];
  int ch, left = TRUE;
  
  cdclrl( MSG_LIN1, 2 );
  attrset(InfoColor);
  ch = getcx( "Edit which user: ", MSG_LIN1, 0, TERMS, buf, MAXUSERNAME );
  if ( ch == TERM_ABORT )
    {
      cdclrl( MSG_LIN1, 2 );
	  attrset(0);
      return;
    }
  delblanks( buf );
  if ( ! gunum( &unum, buf ) )
    {
      cdclrl( MSG_LIN1, 2 );
      cdputs( "Unknown user.", MSG_LIN1, 1 );
	  attrset(0);
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 1.0 );
      return;
    }
  cdclear();
  cdclrl( MSG_LIN1, 2 );
  
  while (TRUE) /* repeat */
    {
      /* Do the right side first. */
      cdclrl( 1, MSG_LIN1 -1 );

      lin = 1;
      tcol = 43;
      dcol = 62;
      rcol = dcol - 1;
      
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"         Username:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%s", InfoColor, cuname[unum]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"   Multiple count:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d", InfoColor, umultiple[unum]);
      
      lin++;
      for ( i = 0; i < MAXOPTIONS; i++ )
	{
      cprintf(lin+i,tcol,ALIGN_NONE,"#%d#%17d:", LabelColor,i);
	  if ( uoption[unum][i] )
      	cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c", GreenLevelColor,'T');
	  else
      	cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c", RedLevelColor,'F');
	}
      cprintf(lin+OPT_PHASERGRAPHICS,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"  Phaser graphics:");
      cprintf(lin+OPT_PLANETNAMES,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"     Planet names:");
      cprintf(lin+OPT_ALARMBELL,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"       Alarm bell:");
      cprintf(lin+OPT_INTRUDERALERT,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"  Intruder alerts:");
      cprintf(lin+OPT_NUMERICMAP,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"      Numeric map:");
      cprintf(lin+OPT_TERSE,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"            Terse:");
      cprintf(lin+OPT_EXPLOSIONS,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"       Explosions:");
      
      lin+=(MAXOPTIONS + 1);
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"          Urating:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0g",InfoColor, oneplace(urating[unum]));
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"             Uwar:");
      buf[0] = '(';
      for ( i = 0; i < NUMTEAMS; i = i + 1 )
	if ( uwar[unum][i] )
	  buf[i+1] = chrteams[i];
	else
	  buf[i+1] = '-';
      buf[NUMTEAMS+1] = ')';
      buf[NUMTEAMS+2] = EOS;
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, buf);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"           Urobot:");
      if ( urobot[unum] )
      	cprintf(lin,dcol,ALIGN_NONE,"#%d#%c",GreenLevelColor, 'T');
      else
      	cprintf(lin,dcol,ALIGN_NONE,"#%d#%c",RedLevelColor, 'F');
      
      /* Now the left side. */
      lin = 1;
      tcol = 3;
      dcol = 22;
      lcol = dcol - 1;
      
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"             Name:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, upname[unum]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"             Team:");
      i = uteam[unum];
      if ( i < 0 || i >= NUMTEAMS )
		  cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, i);
      else
		  cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, tname[i]);
      
      lin++;
      for ( i = 0; i < MAXOOPTIONS; i++ )
	{
      cprintf(lin+i,tcol,ALIGN_NONE,"#%d#%17d:", LabelColor,i);
	  if ( uooption[unum][i] )
      	cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c", GreenLevelColor,'T');
	  else
      	cprintf(lin+i,dcol,ALIGN_NONE,"#%d#%c", RedLevelColor,'F');
	}
      cprintf(lin+OOPT_MULTIPLE,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"         Multiple:");
      cprintf(lin+OOPT_SWITCHTEAMS,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"     Switch teams:");
      cprintf(lin+OOPT_PLAYWHENCLOSED,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor," Play when closed:");
      cprintf(lin+OOPT_SHITLIST,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"          Disable:");
      cprintf(lin+OOPT_GODMSG,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"     GOD messages:");
      cprintf(lin+OOPT_LOSE,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"             Lose:");
      cprintf(lin+OOPT_AUTOPILOT,tcol,ALIGN_NONE,"#%d#%s", 
		LabelColor,"        Autopilot:");
      
      lin+=(MAXOOPTIONS + 1);
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"       Last entry:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%s",InfoColor, ulastentry[unum]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"  Elapsed seconds:");
      fmtseconds( ustats[unum][TSTAT_SECONDS], buf );
      i = dcol + 11 - strlen( buf );
      cprintf(lin,i,ALIGN_NONE,"#%d#%s",InfoColor, buf);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"      Cpu seconds:");
      fmtseconds( ustats[unum][TSTAT_CPUSECONDS], buf );
      i = dcol + 11 - strlen ( buf );
      cprintf(lin,i,ALIGN_NONE,"#%d#%s",InfoColor, buf);
      
      lin++;
      
      /* Do column 4 of the bottom stuff. */
      olin = lin;
      tcol = 62;
      dcol = 72;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "Maxkills:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, 
      	ustats[unum][USTAT_MAXKILLS]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "Torpedos:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
      	ustats[unum][USTAT_TORPS]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Phasers:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, 
      	ustats[unum][USTAT_PHASERS]);
      
      /* Do column 3 of the bottom stuff. */
      lin = olin;
      tcol = 35;
      dcol = 51;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Planets taken:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
      	ustats[unum][USTAT_CONQPLANETS]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Armies bombed:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, 
      	ustats[unum][USTAT_ARMBOMB]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "   Ship armies:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
      	ustats[unum][USTAT_ARMSHIP]);
      
      /* Do column 2 of the bottom stuff. */
      lin = olin;
      tcol = 18;
      dcol = 29;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Conquers:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
      	ustats[unum][USTAT_CONQUERS]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor,"    Coups:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor,
      	ustats[unum][USTAT_COUPS]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "Genocides:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, 
      	ustats[unum][USTAT_GENOCIDE]);
      
      /* Do column 1 of the bottom stuff. */
      lin = olin;
      tcol = 1;
      dcol = 10;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "   Wins:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, 
      	ustats[unum][USTAT_WINS]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, " Losses:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, 
      	ustats[unum][USTAT_LOSSES]);
      
      lin++;
      cprintf(lin,tcol,ALIGN_NONE,"#%d#%s", LabelColor, "Entries:");
      cprintf(lin,dcol,ALIGN_NONE,"#%d#%0d",InfoColor, 
      	ustats[unum][USTAT_ENTRIES]);
      
      /* Display the stuff */
      cprintf(MSG_LIN2,0,ALIGN_CENTER,"#%d#%s", InfoColor,
		"Use arrow keys to position, SPACE to modify, q to quit. ");
      if ( left )
	i = lcol;
      else
	i = rcol;
      cdput( '+', row, i );
      cdmove( row, i );
      cdrefresh();
      
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
	      if ( ch != TERM_ABORT && buf[0] != EOS)
	      {
		delblanks( buf );
		if ( ! gunum( &i, buf ) )
		  stcpn( buf, cuname[unum], MAXUSERNAME );
		else
		  {
		    cdclrl( MSG_LIN1, 2 );
		    cdputc( "That username is already in use.",
			   MSG_LIN2 );
		    cdmove( 1, 1 );
		    cdrefresh();
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
		  safectoi( &umultiple[unum], buf, i );
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
	case 0x0c:
	  cdredo();
	  break;
	default:
	  cdbeep();
	}
    }
  
  /* NOTREACHED */
  
}


/*  watch - peer over someone's shoulder */
/*  SYNOPSIS */
/*    watch */
void watch(void)
{
  
  int snum, tmp_snum, old_snum;
  int ch, normal;
  int msgrand, readone, now;
  char buf[MSGMAXLINE];
  int live_ships = TRUE;
  int toggle_flg = FALSE;   /* jon no like the toggle line ... :-) */

  normal = TRUE;

  if (!prompt_ship(buf, &snum, &normal))
    return;
  else
    {
	  old_snum = tmp_snum = snum;
	  credraw = TRUE;
	  cdclear();
	  cdredo();
	  grand( &msgrand );

	  csnum = snum;		/* so display knows what to display */
	  setopertimer();

	  while (TRUE)	/* repeat */
	    {
	      if (!normal)
		cdisplay = FALSE; /* can't use it to display debugging */
	      else
		cdisplay = TRUE;

		/* set up toggle line display */
		/* cdclrl( MSG_LIN1, 1 ); */
		if (toggle_flg)
			toggle_line(snum,old_snum);

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

	      setdheader( TRUE ); /* always true for watching ships and
				     doomsday.  We may want to turn it off
				     if we ever add an option for watching
				     planets though, so we'll keep this
				     in for now */
	      
	      if ( !normal )
		{
		  debugdisplay( snum );
		}
	      
	      /* doomdisplay(); */
	      
	      /* Un-read message, if there's a chance it got garbaged. */
	      if ( readone )
		if ( iochav() )
		  *glastmsg = modp1( *glastmsg - 1, MAXMESSAGES );
	      
	      /* Get a char with timeout. */
	      if ( ! iogtimed( &ch, 1 ) )
		continue; /* next */
	      cdclrl( MSG_LIN1, 2 );
	      switch ( ch )
		{
		case 'd':    /* flip the doomsday machine (only from doomsday window) */
		  if (snum == DISPLAY_DOOMSDAY)
		    {
		      if ( *dstatus == DS_LIVE )
			*dstatus = DS_OFF;
		      else
			doomsday();
		    }
		  else
		    cdbeep();
		  break;
		case 'h':
		  stoptimer();
		  dowatchhelp();
		  credraw = TRUE;
		  setopertimer();
		  break;
		case 'i':
		  opinfo( MSG_GOD );
		  break;
		case 'k':
		  kiss(csnum, TRUE);
		  break;
		case 'm':
		  sendmsg( MSG_GOD, TRUE );
		  break;
		case 'r':  /* just for fun - dwp */
		  oprobot();
		  break;
		case 'L':
		  review( MSG_GOD, *glastmsg );
		  break;
		case 0x0c:
		  stoptimer();
		  cdredo();
		  credraw = TRUE;
		  setopertimer();
		  break;
		case 'q':
		case 'Q':
		  stoptimer();
		  return;
		  break;
		case 'w': /* look at any ship (live or not) if specifically asked for */
		  tmp_snum = snum;
		  if (prompt_ship(buf, &snum, &normal)) 
		    {
		      if (tmp_snum != snum) 
			{
			  old_snum = tmp_snum;
			  tmp_snum = snum;
			  credraw = TRUE;
			}
		      csnum = snum;
		      if (normal)
			{
			  stoptimer();
			  display( csnum, headerflag );
			  setopertimer();
			}
		    }
		  break;
		case 'W': /* toggle live_ships flag */
		  if (live_ships)
		    live_ships = FALSE;
		  else
		    live_ships = TRUE;
		  break;
		case '`':                 /* toggle between two ships */
		  if (normal || (!normal && old_snum > 0))
		    {
		      if (old_snum != snum) 
			{
			  tmp_snum = snum;
			  snum = old_snum;
			  old_snum = tmp_snum;
			  
			  csnum = snum;
			  credraw = TRUE;
			  if (normal)
			    {
			      stoptimer();
			      display( csnum, headerflag );
			      setopertimer();
			    }
			}
		    }
		  else
		    cdbeep();
		  break;
		case '~':                 /* toggle debug display */
		  if (csnum > 0) 
		    {
		      if (normal)
			normal = FALSE;
		      else
			normal = TRUE;
		      credraw = TRUE;
		      cdclear();
		    }
		  else
		    cdbeep();
		  break;
		case '/':                /* ship list - dwp */
		  stoptimer();
		  playlist( TRUE, FALSE, 0 );
		  credraw = TRUE;
		  setopertimer();
		  break;
		case '\\':               /* big ship list - dwp */
		  stoptimer();
		  playlist( TRUE, TRUE, 0 );
		  credraw = TRUE;
		  setopertimer();
		  break;
		case '!':
		  if (toggle_flg)
			toggle_flg = FALSE;
		  else
			toggle_flg = TRUE;
		  break;
		case '>':  /* forward rotate ship numbers (including doomsday) - dwp */
		case KEY_RIGHT:
		case KEY_UP:
		  while (TRUE)
		    {
		      int i;

		      if (live_ships)
			{	/* we need to make sure that there is
				   actually something alive or an
				   infinite loop will result... */
			  int foundone = FALSE;

			  for (i=1; i <= MAXSHIPS; i++)
			    {
			      if (stillalive(i))
				{
				  foundone = TRUE;
				}
			    }
			  if (foundone == FALSE)
			    {	/* check the doomsday machine */
			      if (*dstatus == DS_LIVE)
				foundone = TRUE;
			    }

			  if (foundone == FALSE)
			    {
			      cdbeep();
			      break; /* didn't find one, beep, leave everything
				      alone*/
			    }
			}

		      if (snum == DISPLAY_DOOMSDAY)
			{	  /* doomsday - wrap around to first ship */
			  i = 1;
			}
		      else	
			i = snum + 1;

		      if (i > MAXSHIPS)
			{	/* if we're going past
				   now loop thu specials (only doomsday for
				   now... ) */
			  if (normal)
			    i = DISPLAY_DOOMSDAY;
			  else
			    i = 1;
			}
		      
		      snum = i;
			
		      credraw = TRUE;
		      
		      if (live_ships)
			if ((snum > 0 && stillalive(snum)) || 
			    (snum == DISPLAY_DOOMSDAY && *dstatus == DS_LIVE))
			  {
			    csnum = snum;
			    break;
			  }
			else
			  continue;
		      else
			{
			  csnum = snum;
			  break;
			}
		    }
		  if (normal)
		    {
		      stoptimer();
		      display( csnum, headerflag );
		      setopertimer();
		    }
		  break;
		case '<':  /* reverse rotate ship numbers (including doomsday)  - dwp */
		case KEY_LEFT:
		case KEY_DOWN:
		  while (TRUE)
		    {
		      int i;

		      if (live_ships)
			{	/* we need to make sure that there is
				   actually something alive or an
				   infinite loop will result... */
			  int foundone = FALSE;

			  for (i=1; i <= MAXSHIPS; i++)
			    {
			      if (stillalive(i))
				{
				  foundone = TRUE;
				}
			    }
			  if (foundone == FALSE)
			    {	/* check the doomsday machine */
			      if (*dstatus == DS_LIVE)
				foundone = TRUE;
			    }

			  if (foundone == FALSE)
			    {
			      cdbeep();
			      break; /* didn't find one, beep, leave everything
				      alone*/
			    }
			}


		      if (snum == DISPLAY_DOOMSDAY)
			{	  /* doomsday - wrap around to last ship */
			  i = MAXSHIPS;
			}
		      else	
			i = snum - 1;

		      if (i <= 0)
			{	/* if we're going past
				   now loop thu specials (only doomsday for
				   now... )*/
			  if (normal)
			    i = DISPLAY_DOOMSDAY;
			  else
			    i = MAXSHIPS;
			}
		      
		      snum = i;
			
		      credraw = TRUE;
		      
		      if (live_ships)
			if ((snum > 0 && stillalive(snum)) || 
			    (snum == DISPLAY_DOOMSDAY && *dstatus == DS_LIVE))
			  {
			    csnum = snum;
			    break;
			  }
			else
			  continue;
		      else
			{
			  csnum = snum;
			  break;
			}
		    }
		  if (normal)
		    {
		      stoptimer();
		      display( csnum, headerflag );
		      setopertimer();
		    }

		  break;
		case TERM_ABORT:
		  return;
		  break;
		default:
		  cdbeep();
		  break;
		}
	      /* Disable messages for awhile. */
	      grand( &msgrand );
	    }
	} /* end else */

  /* NOTREACHED */
  
    }

void setdheader(int show_header)
{

  headerflag = show_header;
  return;
}

int prompt_ship(char buf[], int *snum, int *normal)
{
  int tch;
  int tmpsnum = 0;
  string pmt="Watch which ship (<cr> for doomsday)? ";
  string nss="No such ship.";

  tmpsnum = *snum;

  cdclrl( MSG_LIN1, 2 );
  buf[0] = EOS;
  tch = cdgetx( pmt, MSG_LIN1, 1, TERMS, buf, MSGMAXLINE );
  cdclrl( MSG_LIN1, 1 );

  if ( tch == TERM_ABORT )
    {
      return(FALSE); /* dwp */
    }
  
  /* 
   * DWP - Removed old code to watch doomsday machine.
   * Watch it with conqlb.c display() now.
   */
  
  *normal = ( tch != TERM_EXTRA );		/* line feed means debugging */

  delblanks( buf );

  if ( strlen( buf ) == 0 ) 
    {              /* watch doomsday machine */
      tmpsnum = DISPLAY_DOOMSDAY;
      *normal = TRUE;		/* doomsday doesn't have a debugging view */
    }
  else
    {
      if ( alldig( buf ) != TRUE )
	{
	  cdputs( nss, MSG_LIN2, 1 );
	  cdmove( 1, 1 );
	  cdrefresh();
	  c_sleep( 1.0 );
	  return(FALSE); /* dwp */
	}
      safectoi( &tmpsnum, buf, 0 );	/* ignore return status */
    }

  if ( (tmpsnum < 1 || tmpsnum > MAXSHIPS) && tmpsnum != DISPLAY_DOOMSDAY )
    {
      cdputs( nss, MSG_LIN2, 1 );
      cdmove( 1, 1 );
      cdrefresh();
      c_sleep( 1.0 );
      return(FALSE); /* dwp */
    }

  *snum = tmpsnum;
  return(TRUE);

} /* end prompt_ship() */


/*  dowatchhelp - display a list of commands while watching a ship */
/*  SYNOPSIS */
/*    dowatchhelp( void ) */
void dowatchhelp(void)
{
  int lin, col, tlin;
  int ch;
  int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#%%-9s#%d#%%s",
	      InfoColor,
	      LabelColor);
	}

  cdclear();
  cprintf(1,0,ALIGN_CENTER,"#%d#%s", LabelColor, "WATCH WINDOW COMMANDS");
  
  lin = 4;
  
  /* Display the left side. */
  tlin = lin;
  col = 4;
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
	"d", "flip the doomsday machine - doomsday window only");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "m", "message from GOD");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "L", "review messages");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "k", "kill this ship");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "h", "this message");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "w", "watch a ship");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  	"W", "toggle between (live status) and (any status) ships");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  	"<>", "decrement/increment ship number\n");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "`", "toggle between two ships");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, 
  	"~", "toggle between window and debug ship screen");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "!", "display toggle line");
  tlin++;
  cprintf(tlin,col,ALIGN_NONE,sfmt, "/", "player list");

  putpmt( "--- press space when done ---", MSG_LIN2 );
  cdrefresh();
  while ( ! iogtimed( &ch, 1 ) )
    ;
  cdclrl( MSG_LIN2, 1 );
  
  return;
  
}


void toggle_line(int snum, int old_snum)
{

  char *frmt_str = "(') = Toggle %s:%s";
  char snum_str[MSGMAXLINE];
  char old_snum_str[MSGMAXLINE];
  char buf[MSGMAXLINE];
  
  build_toggle_str(snum_str,snum);
  build_toggle_str(old_snum_str,old_snum);
  
  sprintf(buf,frmt_str,snum_str, old_snum_str);
  cdputs( buf, MSG_LIN1, (cmaxcol-(strlen(buf))));  /* at end of line */
  
  return;

}

char *build_toggle_str(char *snum_str, int snum)
{
  
  char buf[MSGMAXLINE];
  static char *doomsday_str = "DM";
  static char *deathstar_str = "DS";
  static char *unknown_str = "n/a";
  
  buf[0] = EOS;
  if (snum > 0 && snum <= MAXSHIPS)
    {          /* ship */
      sprintf(snum_str,"%c%d", chrteams[steam[snum]], snum);
    }
  else if (snum < 0 && -snum <= NUMPLANETS) 
    {  /* planet */

      sprintf(snum_str, "%c%c%c", 
	      pname[-snum][0], pname[-snum][1], pname[-snum][2]);
    }
  else if (snum == DISPLAY_DOOMSDAY)          /* specials */
    strcpy(snum_str,doomsday_str);
  else if (snum == DISPLAY_DEATHSTAR)
    strcpy(snum_str,deathstar_str);
  else                                        /* should not get here */
    strcpy(snum_str,unknown_str);
  
  return(snum_str);

}

