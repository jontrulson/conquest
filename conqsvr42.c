#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q V M S */
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

#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"


/* int GetConquestUID(void) - return conquest's User ID */
int GetConquestUID(void)
{
  struct passwd *conq_pwd;
  static int theuid;
  
  if ((conq_pwd = getpwnam(ROOT_USER)) == NULL)
    {
      fprintf(stderr, "conqsvr42: GetConquestUID(%s): can't get user: %s",
	      ROOT_USER,
	      sys_errlist[errno]);
      
      return(ERR);
    }
  
  theuid = conq_pwd->pw_uid;
  
  return(theuid);
}


/* int GetConquestGID(void) - return conquest's Group ID */
int GetConquestGID(void)
{
  struct group *conq_grp;
  static int thegid;
  
  if ((conq_grp = getgrnam(CONQUEST_GROUP)) == NULL)
    {
      fprintf(stderr, "conqsvr42: GetConquestGID(%s): can't get group: %s",
	      CONQUEST_GROUP,
	      sys_errlist[errno]);
      
      return(ERR);
    }
  
  thegid = conq_grp->gr_gid;
  
  return(thegid);
}




/*  astoff - disable asts */
/*  SYNOPSIS */
/*    astoff */
void astoff(void)
{
  
  /*    sys__setast( %val(0) )		/* disable asts */
    /*  JET_ASTOFF();			/* block sigs? */
  return;
  
}


/*  aston - enable asts */
/*  SYNOPSIS */
/*    aston */
void aston(void)
{
  
  /*    sys$setast( %val(1) )		/* enable asts */
    /*  JET_ASTON();			/* enable sigs? */
  return;
  
}

void EnableConquestSignalHandler(void)
{
#ifdef DEBUG_SIG
  clog("EnableConquestSignalHandler() ENABLED");
#endif
  
  signal(SIGHUP, (void (*)(int))DoConquestSig);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTERM, (void (*)(int))DoConquestSig);  
  signal(SIGINT, SIG_IGN);

  if (isagod(-1) || sysconf_AllowSigquit == TRUE)
    {
      signal(SIGQUIT, (void (*)(int))DoConquestSig);
    }
  else
    {
      signal(SIGQUIT, SIG_IGN);
    }
  
  return;
}

void EnableConqoperSignalHandler(void)
{
#ifdef DEBUG_SIG
  clog("EnableConquestSignalHandler() ENABLED");
#endif
  
  signal(SIGHUP, (void (*)(int))DoConqoperSig);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTERM, (void (*)(int))DoConqoperSig);  
  signal(SIGINT, (void (*)(int))DoConqoperSig);
  signal(SIGQUIT, (void (*)(int))DoConqoperSig);

  return;
}

void DoConquestSig(int sig)
{
  
#ifdef DEBUG_SIG
  clog("DoConquestSig() got SIG %d", sig);
#endif
  
  switch(sig)
    {
    case SIGQUIT:
      stoptimer();
      drpexit();
      cdclear();
      cdrefresh();
      conqstats(CqContext.snum);		/* update stats */
				/* now we clear ship's elapsed/cpu seconds
				   so that there won't be a huge addition to
				   the Teams/Users/Ships timing stats when
				   a VACANT ships re-enters Conquest */
      Ships[CqContext.snum].ctime = 0;
      Ships[CqContext.snum].etime = 0;
      conqend();
      cdend();
      
      exit(0);
      break;

    case SIGINT:
    case SIGTERM:
    case SIGHUP:
      cdend();
      exit(0);
      break;
    default:
      break;
    }

  EnableConquestSignalHandler();	/* reset */
  return;
}

void DoConqoperSig(int sig)
{
  
#ifdef DEBUG_SIG
  clog("DoSig() got SIG %d", sig);
#endif
  
  switch(sig)
    {
    case SIGTERM:
    case SIGINT:
    case SIGHUP:
    case SIGQUIT:
      stoptimer();
      cdrefresh();
      conqend();
      cdend();
      exit(0);			/* WE EXIT HERE */
      break;
    default:
      break;
    }

  EnableConqoperSignalHandler();	/* reset */
  return;
}


/*  astservice - ast service routine for conquest */
/*  SYNOPSIS */
/*    astservice */
/* This routine gets called from a sys$setimr ast. Normally, it outputs */
/* one screen update and then sets up another timer request. */
void astservice(int sig)
{
  int now;
  int readone;
  static int RMsggrand = 0;
  int difftime;

  /* Don't do anything if we're not supposed to. */
  if ( ! CqContext.display )
    return;
  
  /* Don't do anything if we're dead. */
  if ( ! stillalive( CqContext.snum ) )
    return;
  stoptimer();
  drcheck();				/* handle driver logic */
  
  /* See if we can display a new message. */

				/* for people with 25 lines, we
				   use a different timer so that
				   NEWMSG_GRAND intervals will determine
				   whether it's time to display a new
				   message... Otherwise, CqContext.msgrand
				   is used - which means that NEWMSG_GRAND
				   interval will have to pass after issuing
				   any command before a new msg will disp
				   12/28/98 */
  readone = FALSE;
  if ( CqContext.msgok )
    {
      /*
      clog("### RMsggrand = %d, CqContext.msgrand = %d", RMsggrand, CqContext.msgrand);
      */

      if (RMsg_Line != MSG_LIN1)
	{			/* we have line 25 for msgs */
	  difftime = dgrand( RMsggrand, &now );
	}
      else
	{
	  difftime = dgrand( CqContext.msgrand, &now );
	}

      /*
	clog("difftime = %d, RMsg_Line = %d, MSG_LIN1 = %d\n\tNEWMSG_GRAND = %d, now = %d", difftime, RMsg_Line, MSG_LIN1, NEWMSG_GRAND, now);
       */

      if ( difftime >= NEWMSG_GRAND )
	if ( getamsg( CqContext.snum, &Ships[CqContext.snum].lastmsg ) )
	  {
	    if (readmsg( CqContext.snum, Ships[CqContext.snum].lastmsg, 
			 RMsg_Line ) == TRUE)
	      {
		if (Msgs[Ships[CqContext.snum].lastmsg].msgfrom != 
		    CqContext.snum)
		  if (conf_MessageBell == TRUE)
		    cdbeep();
				/* set both timers, regardless of which
				   one we're actally concerned with */
		CqContext.msgrand = now;
		RMsggrand = now;
		readone = TRUE;
		recordAddMsg(&Msgs[Ships[CqContext.snum].lastmsg]);
	      }
	  }
    }

  /* Perform one ship display update. */
  display( CqContext.snum, FALSE );
  
  
  /* Un-read the message if there's a chance it got garbaged. */
  /* JET 3/24/96 - another try with curses timer disabled */
  if ( readone )
    if (RMsg_Line == MSG_LIN1)	/* we don't have an extra msg line */
      if ( iochav() )
	Ships[CqContext.snum].lastmsg = modp1( Ships[CqContext.snum].lastmsg - 1, MAXMESSAGES );
  
  /* Schedule for next time. */
  settimer();
  
  return;
  
}

/*  astoperservice - ast service routine for conqoper */
/*  SYNOPSIS */
/*    astservice */
/* This routine gets called from a sys$setimr ast. Normally, it outputs */
/* one screen update and then sets up another timer request. */
void astoperservice(int sig)
{
  /* Don't do anything if we're not supposed to. */
  if ( ! CqContext.display )
    return;
  
  stoptimer();
  
  /* Perform one ship display update. */
  display( CqContext.snum, headerflag );
  
  /* Schedule for next time. */
  setopertimer();
  
  return;
  
}


/*  comsize - return size of the common block (in bytes) */
/*  SYNOPSIS */
/*    int size */
/*    comsize( size ) */
void comsize( unsigned long *size )
{
  unsigned long int val;

  if ((int)EndOfCBlock > (int)CBlockRevision) 
    val = (int)EndOfCBlock - (int)CBlockRevision; 
  else
    val = (int)CBlockRevision - (int)EndOfCBlock;
  *size = val + sizeof(int); 

  /*clog("sizeof(real) = %d, val = %d", sizeof(real), val); */
  
  return;
  
}


/*  conqend - machine dependent clean-up */
/*  SYNOPSIS */
/*    conqend */
void conqend(void)
{

  char msgbuf[128];

  if (CqContext.entship == TRUE)
    {				/* let everyone know we're leaving */
      sprintf(msgbuf, "%s has left the game.",
	      Users[CqContext.unum].alias);
      stormsg(MSG_COMP, MSG_ALL, msgbuf);
    }
  
  recordCloseOutput();

  return;
  
}


/*  conqinit - machine dependent initialization */
/*  SYNOPSIS */
/*    conqinit */
void conqinit(void)
{
  /* First things first. */
  if ( *CBlockRevision != COMMONSTAMP )
    error( "conquest: Common block ident mismatch.  \nInitialize the Universe via conqoper." );
  
  
#ifdef SET_PRIORITY
  /* Increase our priority a bit */
  
  if (nice(CONQUEST_PRI) == -1)
    {
      clog("conqinit(): nice(CONQUEST_PRI (%d)): failed: %s",
	   CONQUEST_PRI,
	   sys_errlist[errno]);
    }

#if defined(DEBUG_FLOW)
    clog("conqinit(): nice(CONQUEST_PRI (%d)): succeeded.",
	 CONQUEST_PRI);
#endif

#endif
  
  /* Set up game environment. */
  
  /* Figure out which gamcron file to use (and if we're gonna use one). */
  
  /* Other house keeping. */
  CqContext.pid = getpid();		
  CqContext.hasnewsfile = ( strcmp( C_CONQ_NEWSFILE, "" ) != 0 );
  
  /* Zero process id of our child (since we don't have one yet). */
  CqContext.childpid = 0;
  
  /* Zero last time drcheck() was called. */
  CqContext.drchklastime = 0;
  
  /* Haven't scanned anything yet. */
  CqContext.lastinfostr[0] = EOS;

  
  return;
  
}


/*  conqstats - handle cpu and elapsed statistics (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    conqstats( snum ) */
void conqstats( int snum )
{
  int unum, team, cadd, eadd;
  time_t difftime;
  cadd = 0;
  eadd = 0;
  
  upstats( &Ships[snum].ctime, &Ships[snum].etime, 
	   &Ships[snum].cacc, &Ships[snum].eacc,
	   &cadd, &eadd );
  
  /* Add in the new amounts. */
  PVLOCK(&ConqInfo->lockword);
  if ( Ships[snum].pid != 0 )
    {
      /* Update stats for a humanoid ship. */
      unum = Ships[snum].unum;

      Users[unum].stats[USTAT_CPUSECONDS] += cadd;
      Users[unum].stats[USTAT_SECONDS] += eadd;

				/* update elapsed time in History[] 
				   for this user */

      if (CqContext.histslot != ERR && History[CqContext.histslot].histunum == unum)
	{
	  difftime = getnow(NULL, 0) - History[CqContext.histslot].histlog;
	  if (difftime < (time_t)0)
	    difftime = (time_t)0;
	  History[CqContext.histslot].elapsed = difftime;
	}

      team = Users[unum].team;
      Teams[team].stats[TSTAT_CPUSECONDS] += cadd;
      Teams[team].stats[TSTAT_SECONDS] += eadd;

      ConqInfo->ccpuseconds += cadd;
      ConqInfo->celapsedseconds += eadd;


    }
  PVUNLOCK(&ConqInfo->lockword);
  
  return;
  
}


/*  drcheck - make sure the driver is still around (DOES LOCKING) */
/*  SYNOPSIS */
/*    drcheck */
void drcheck(void)
{
  int ppid;
  
  /* If we haven't been getting cpu time in recent history, do no-thing. */
  /*  gsecs(playtime);*/
  if ( dsecs( CqContext.drchklastime, &CqContext.drchklastime ) > TIMEOUT_DRCHECK )
    return;
  
  if ( dsecs( Driver->drivtime, &(Driver->playtime) ) > TIMEOUT_DRIVER )
    {
      if ( CqContext.childpid != 0 )
	{
	  /* We own the driver. See if it's still there. */
	  ppid = CqContext.childpid;
	  if ( kill(CqContext.childpid, 0) != -1 )
	    {
	      /* He's still alive and belongs to us. */
	      gsecs( &(Driver->drivtime) );
	      return;
	    }
	  else
	    clog( "drcheck(): Wrong ppid %d.", ppid );
	  
	  /* If we got here, something was wrong; disown the child. */
	  CqContext.childpid = 0;
	}
      
      PVLOCK(&ConqInfo->lockword);
      if ( dsecs( Driver->drivtime, &(Driver->playtime) ) > TIMEOUT_DRIVER )
	{
	  drcreate();
	  Driver->drivcnt = modp1( Driver->drivcnt + 1, 1000 );
	  clog( "Driver timeout #%d.", Driver->drivcnt );
	}
      PVUNLOCK(&ConqInfo->lockword);
    }
  drstart();
  
  return;
  
}


/*  drcreate - create a new driver process */
/*  SYNOPSIS */
/*    drcreate */
void drcreate(void)
{
  int pid;
  char drivcmd[BUFFER_SIZE];

  
  gsecs( &(Driver->drivtime) );			/* prevent driver timeout */
  Driver->drivpid = 0;			/* zero current driver pid */
  Driver->drivstat = DRS_RESTART;		/* driver state to restart */
  
  /* fork the child - mmap()'s should remain */
  /*  intact */
  if ((pid = fork()) == -1)
    {				/* error */
      Driver->drivstat = DRS_OFF;
      clog( "drcreate(): fork(): %s", sys_errlist[errno]);
      return;
    }
  
  if (pid == 0)
    {				/* The child: aka "The Driver" */
      sprintf(drivcmd, "%s/%s", CONQLIBEXEC, C_CONQ_CONQDRIV);
      execl(drivcmd, drivcmd, NULL);
      clog("drcreate(): exec(): %s", sys_errlist[errno]);
      perror("exec");		/* shouldn't be reached */
      exit(1);
      /* NOTREACHED */
    }
  else
    {				/* We're the parent, store pid */
      CqContext.childpid = pid;	
    }
  
  return;
  
}


/*  drkill - make the driver go away if we started it (DOES LOCKING) */
/*  SYNOPSIS */
/*    drkill */
void drkill(void)
{
  if ( CqContext.childpid != 0 )
    if ( CqContext.childpid == Driver->drivpid && Driver->drivstat == DRS_RUNNING )
      {
	PVLOCK(&ConqInfo->lockword);
	if ( CqContext.childpid == Driver->drivpid && Driver->drivstat == DRS_RUNNING )
	  Driver->drivstat = DRS_KAMIKAZE;
	PVUNLOCK(&ConqInfo->lockword);
      }
  
  return;
  
}


/*  drpexit - make the driver go away if we started it */
/*  SYNOPSIS */
/*    drpexit */
void drpexit(void)
{
  
  int i;
  
  if ( CqContext.childpid != 0 )
    {
      /* We may well have started the driver. */
      drkill();
      for ( i = 1; CqContext.childpid == Driver->drivpid && i <= 50; i = i + 1 )
	c_sleep( 0.1 );
      if ( CqContext.childpid == Driver->drivpid )
	clog("drpexit(): Driver didn't exit; pid = %08x", CqContext.childpid );
    }
  
  return;
  
}


/*  drstart - Start a new driver if necessary (DOES LOCKING) */
/*  SYNOPSIS */
/*    drstart */
void drstart(void)
{
  
  if ( Driver->drivstat == DRS_OFF )
    {
      PVLOCK(&ConqInfo->lockword);
      if ( Driver->drivstat == DRS_OFF )
	drcreate();
      PVUNLOCK(&ConqInfo->lockword);
    }
  return;
  
}


/*  gcputime - get cpu time */
/*  SYNOPSIS */
/*    int cpu */
/*    gcputime( cpu ) */
/*  DESCRIPTION */
/*    The total cpu time (in hundreths) for the current process is returned. */
void gcputime( int *cpu )
{
  static struct tms Ptimes;
  
  times(&Ptimes);
  
  *cpu = round( ((real)(Ptimes.tms_stime + Ptimes.tms_utime) / 
		 (real)CLK_TCK) * 
	       100.0);
  
  /* clog("gcputime() - *cpu = %d", *cpu); */
  
  return;
  
}


/*  helplesson - verbose help */
/*  SYNOPSIS */
/*    helplesson */
void helplesson(void)
{
  
  char buf[MSGMAXLINE];
  char helpfile[BUFFER_SIZE];
  
  sprintf(helpfile, "%s/%s", CONQSHARE, C_CONQ_HELPFILE);
  sprintf( buf, "%s: Can't open.", helpfile );
  pagefile( helpfile, buf);
  
  return;
  
}


/*  initstats - statistics setup */
/*  SYNOPSIS */
/*    int ctemp, etemp */
/*    initstats( ctemp, etemp ) */
void initstats( int *ctemp, int *etemp )
{
  
  gcputime( ctemp );
  grand( etemp );
  
  return;
  
}


/*  isagod - determine if a user is a god or not - NULL means current user */
/*  SYNOPSIS */
/*    int flag, isagod */
/*    flag = isagod() */
int isagod( int unum )
{
  static struct group *grp = NULL;
  static int god = FALSE;
  static char myname[BUFFER_SIZE];
  int i;
  
  god = FALSE;
  
  if (unum == -1)		/* get god status for current user */
    {				/* now find out whether we're in it */
      strncpy(myname, (char *)cuserid(NULL), BUFFER_SIZE - 2);
      myname[BUFFER_SIZE - 1] = EOS;
    }
  else
    {				/* else a user number passed in */
				/* only locals can be godlike */
      if (Users[unum].type != UT_LOCAL)
	return(FALSE);
      strcpy(myname, Users[unum].username);
    }
  
  if (grp == NULL)
    {				/* first time */
      grp = getgrnam(CONQUEST_GROUP);
      
      if (grp == NULL)
	{
	  clog("isagod(%s): getgrnam(%s) failed: %s",
	       myname,
	       CONQUEST_GROUP,
	       sys_errlist[errno]);
	  
	  god = FALSE;
	  return(FALSE);
	}
    }
  
  /* root is always god */
  if (strcmp(myname, "root") == 0)
    god = TRUE;
  
  i = 0;
  
  if (grp->gr_mem != NULL)
    {
      while (grp->gr_mem[i] != NULL)
	{
	  if (strcmp(myname, grp->gr_mem[i]) == 0)
	    {		/* a match */
	      god = TRUE;
	      break;
	    }
	  
	  i++;
	}
    }
  
  endgrent();
  
  return(god);
  
}


/*  news - list current happenings */
/*  SYNOPSIS */
/*    news */
void news(void)
{
  char newsfile[BUFFER_SIZE];
  extern char *c_conq_newsfile;
  
  sprintf(newsfile, "%s/%s", CONQSHARE, C_CONQ_NEWSFILE);
  
  pagefile( newsfile, "No news is good news.");
  
  return;
  
}


/*  settimer - set timer to display() */
/*  SYNOPSIS */
/*    csetimer */
void settimer(void)
{
  static struct sigaction Sig;
  
#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif
  
  Sig.sa_handler = (void (*)(int))astservice;
  
/*  Sig.sa_flags = SA_RESTART;*/	/* restart syscalls! */

  Sig.sa_flags = 0;

  if (sigaction(SIGALRM, &Sig, NULL) == -1)
    {
      clog("settimer():sigaction(): %s\n", sys_errlist[errno]);
      exit(errno);
    }
  
#ifdef HAS_SETITIMER

  if (sysconf_AllowFastUpdate == TRUE && conf_DoFastUpdate == TRUE)
    {
      
      /* 2 updates per sec */
      itimer.it_value.tv_sec = 0;
      itimer.it_value.tv_usec = 500000;
      
      itimer.it_interval.tv_sec = 0;
      itimer.it_interval.tv_usec = 500000;
    }
  else
    {

      /* 1 update per second */
      itimer.it_value.tv_sec = 1;
      itimer.it_value.tv_usec = 0;
      
      itimer.it_interval.tv_sec = 1;
      itimer.it_interval.tv_usec = 0;
    }

  setitimer(ITIMER_REAL, &itimer, NULL);
#else
  alarm(1);			/* set alarm() */
#endif  
  return;
  
}


/*  setopertimer - set timer to display() for conqoper...*/
/*  SYNOPSIS */
void setopertimer(void)
{
  static struct sigaction Sig;
  
#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif
  
  Sig.sa_handler = (void (*)(int))astoperservice;
  
  Sig.sa_flags = 0;

  if (sigaction(SIGALRM, &Sig, NULL) == -1)
    {
      clog("settimer():sigaction(): %s\n", sys_errlist[errno]);
      exit(errno);
    }
  
#ifdef HAS_SETITIMER

  if (sysconf_AllowFastUpdate == TRUE && conf_DoFastUpdate == TRUE)
    {
      
      /* 2 updates per sec */
      itimer.it_value.tv_sec = 0;
      itimer.it_value.tv_usec = 500000;
      
      itimer.it_interval.tv_sec = 0;
      itimer.it_interval.tv_usec = 500000;
    }
  else
    {

      /* 1 update per second */
      itimer.it_value.tv_sec = 1;
      itimer.it_value.tv_usec = 0;
      
      itimer.it_interval.tv_sec = 1;
      itimer.it_interval.tv_usec = 0;
    }

  setitimer(ITIMER_REAL, &itimer, NULL);
#else
  alarm(1);			/* set alarm() */
#endif  
  return;
  
}


/*  stoptimer - cancel timer */
/*  SYNOPSIS */
/*    stoptimer */
void stoptimer(void)
{
#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif
  
  CqContext.display = FALSE;
  
  signal(SIGALRM, SIG_IGN);
  
#ifdef HAS_SETITIMER
  itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
  itimer.it_value.tv_usec = itimer.it_interval.tv_usec = 0;
  
  setitimer(ITIMER_REAL, &itimer, NULL);
#else
  alarm(0);
#endif
  
  CqContext.display = TRUE;
  
  return;
  
}


/*  upchuck - update the common block to disk. */
/*  SYNOPSIS */
/*    upchuck */
void upchuck(void)
{
  
  PVLOCK(&ConqInfo->lockword);
  
  flush_common();
  getdandt( ConqInfo->lastupchuck, 0 );
  
  PVUNLOCK(&ConqInfo->lockword);
  
  return;
  
}


/*  upstats - update statistics */
/*  SYNOPSIS */
/*    int ctemp, etemp, caccum, eaccum, ctime, etime */
/*    upstats( ctemp, etemp, caccum, eaccum, ctime, etime ) */
void upstats( int *ctemp, int *etemp, int *caccum, int *eaccum, int *ctime, int *etime )
{
  
  int i, now;
  
  /* Update cpu time. */
  gcputime( &i );
  
  if (i >= *ctemp )		/* prevent oddities with timing - JET */
    {				/* - for multple godlike exits/entries */
      *caccum = *caccum + (i - *ctemp);
    }
  *ctemp = i;			/* if oddity above, this will self-correct */
  
  if ( *caccum > 100 )
    {
      /* Accumulated a cpu second. */
      *ctime = *ctime + (*caccum / 100);
      *caccum = mod( *caccum, 100 );
    }
  
  /* Update elapsed time. */
  if (*etemp == 0)		/* init etemp if 0 - for VACANT ships */
    grand(etemp);
  
  *eaccum = *eaccum + dgrand( *etemp, &now );

  if ( *eaccum > 1000 )
    {
      /* A second elapsed. */
      *etemp = now;
      *etime = *etime + (*eaccum / 1000);
      *eaccum = mod( *eaccum, 1000 );
    }
  
  return;
  
}

/* return true if a process is alive, else false... */
int CheckPid(int pidnum)
{
  int rv;

  if (pidnum == 0)
    return(FALSE);		/* can re-incarnate to robots */
  rv = kill(pidnum, 0);

  if (rv == -1)
    {
      switch (errno)
	{
	case ESRCH:
	  return(FALSE);
	  break;
	default:
	  return(TRUE);
	  break;
	}
    }
  else 
    return(TRUE);
}
