#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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
#include "context.h"
#include "conf.h"
#include "global.h"
#include "conqlb.h"
#include "conqutil.h"
#include "record.h"

#include "server.h"
/* int getUID(void) - return a User ID */
int getUID(char *name)
{
  struct passwd *conq_pwd;
  char *myusername = clbGetUserLogname();
  char *chkname;

#if defined(CYGWIN)
  /* name root doesn't usually exist, so default to myusername */
  name = NULL;
#endif

  if (!name)
    chkname = myusername;
  else
    chkname = name;

  if ((conq_pwd = getpwnam(chkname)) == NULL)
    {
      fprintf(stderr, "conqsvr42: getUID(%s): can't get user: %s\n",
	      chkname,
	      strerror(errno));
      
      return(ERR);
    }
  
  return(conq_pwd->pw_uid);
}


/* int getConquestGID(void) - return conquest's Group ID */
int getConquestGID(void)
{
  struct group *conq_grp;
  
  if ((conq_grp = getgrnam(CONQUEST_GROUP)) == NULL)
    {
      fprintf(stderr, "conqsvr42: getConquestGID(%s): can't get group: %s",
	      CONQUEST_GROUP,
	      strerror(errno));
      
      return(ERR);
    }
  
  return(conq_grp->gr_gid);
}


/*  comsize - return size of the common block (in bytes) */
/*  SYNOPSIS */
/*    int size */
/*    comsize( size ) */
void comsize( unsigned long *size )
{
  *size = labs((char *)EndOfCBlock - (char *)CBlockRevision) + sizeof(int);

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
  
  /* Set up game environment. */
  
  /* Other house keeping. */
  Context.pid = getpid();		
  Context.hasnewsfile = ( strcmp( C_CONQ_NEWSFILE, "" ) != 0 );
  
  /* Zero process id of our child (since we don't have one yet). */
  Context.childpid = 0;
  
  /* Zero last time drcheck() was called. */
  Context.drchklastime = 0;
  
  /* Haven't scanned anything yet. */
  Context.lastinfostr[0] = EOS;

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

      if (Context.histslot != ERR && History[Context.histslot].histunum == unum)
	{
	  difftime = getnow(NULL, 0) - History[Context.histslot].histlog;
	  if (difftime < (time_t)0)
	    difftime = (time_t)0;
	  History[Context.histslot].elapsed = difftime;
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
  if ( utDeltaSecs( Context.drchklastime, &Context.drchklastime ) > TIMEOUT_DRCHECK )
    return;
  
  if ( utDeltaSecs( Driver->drivtime, &(Driver->playtime) ) > TIMEOUT_DRIVER )
    {
      if ( Context.childpid != 0 )
	{
	  /* We own the driver. See if it's still there. */
	  ppid = Context.childpid;
	  if ( kill(Context.childpid, 0) != -1 )
	    {
	      /* He's still alive and belongs to us. */
	      utGetSecs( &(Driver->drivtime) );
	      return;
	    }
	  else
	    utLog( "drcheck(): Wrong ppid %d.", ppid );
	  
	  /* If we got here, something was wrong; disown the child. */
	  Context.childpid = 0;
	}
      
      PVLOCK(&ConqInfo->lockword);
      if ( utDeltaSecs( Driver->drivtime, &(Driver->playtime) ) > TIMEOUT_DRIVER )
	{
	  drcreate();
	  Driver->drivcnt = utModPlusOne( Driver->drivcnt + 1, 1000 );
	  utLog( "Driver timeout #%d.", Driver->drivcnt );
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

  
  utGetSecs( &(Driver->drivtime) );			/* prevent driver timeout */
  Driver->drivpid = 0;			/* zero current driver pid */
  Driver->drivstat = DRS_RESTART;		/* driver state to restart */
  
  /* fork the child - mmap()'s should remain */
  /*  intact */
  if ((pid = fork()) == -1)
    {				/* error */
      Driver->drivstat = DRS_OFF;
      utLog( "drcreate(): fork(): %s", strerror(errno));
      return;
    }
  
  if (pid == 0)
    {				/* The child: aka "The Driver" */
      sprintf(drivcmd, "%s/%s", CONQLIBEXEC, C_CONQ_CONQDRIV);
      execl(drivcmd, drivcmd, NULL);
      utLog("drcreate(): exec(): %s", strerror(errno));
      perror("exec");		/* shouldn't be reached */
      exit(1);
      /* NOTREACHED */
    }
  else
    {				/* We're the parent, store pid */
      Context.childpid = pid;	
    }
  
  return;
  
}


/*  drkill - make the driver go away if we started it (DOES LOCKING) */
/*  SYNOPSIS */
/*    drkill */
void drkill(void)
{
  if ( Context.childpid != 0 )
    if ( Context.childpid == Driver->drivpid && Driver->drivstat == DRS_RUNNING )
      {
	PVLOCK(&ConqInfo->lockword);
	if ( Context.childpid == Driver->drivpid && Driver->drivstat == DRS_RUNNING )
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
  
  if ( Context.childpid != 0 )
    {
      /* We may well have started the driver. */
      drkill();
      for ( i = 1; Context.childpid == Driver->drivpid && i <= 50; i = i + 1 )
	c_sleep( 0.1 );
      if ( Context.childpid == Driver->drivpid )
	utLog("drpexit(): Driver didn't exit; pid = %08x", Context.childpid );
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
#ifndef CLK_TCK
# ifdef LINUX
   extern long int __sysconf (int);
#  define CLK_TCK ((__clock_t) __sysconf (_SC_CLK_TCK))
# else
#  define CLK_TCK CLOCKS_PER_SEC
# endif
#endif

   /* JET - I think this function has outlived it's usefulness. */
  
  times(&Ptimes);
  
  *cpu = round( ((real)(Ptimes.tms_stime + Ptimes.tms_utime) / 
		 (real)CLK_TCK) * 
	       100.0);
  
  /* utLog("gcputime() - *cpu = %d", *cpu); */
  
  return;
  
}



/*  initstats - statistics setup */
/*  SYNOPSIS */
/*    int ctemp, etemp */
/*    initstats( ctemp, etemp ) */
void initstats( int *ctemp, int *etemp )
{
  
  gcputime( ctemp );
  utGrand( etemp );
  
  return;
  
}


/*  isagod - determine if a user is a god (oper) or not */
/*   if a valid user is passed, then the OOPT_OPER priviledge is */
/*    checked.  Otherwise, the current user must be a member of */
/*    the 'conquest' group (conqoper, conqdriv, etc */
/*  SYNOPSIS */
/*    int flag, isagod */
/*    flag = isagod() */

/* For cygwin, everybody is a god for non-user num checks. */
#if defined(CYGWIN)
int isagod( int unum )
{
  if (unum == -1)               /* get god status for current user */
    {
      return TRUE;
    }
  else
    {				/* else a user number passed in */
				/* just check for OOPT_OPER */
      if (Users[unum].ooptions[OOPT_OPER])
	return TRUE;
      else
	return FALSE;
    }
}
#else /* !CGYWIN */
int isagod( int unum )
{
  static struct group *grp = NULL;
  static int god = FALSE;
  static char myname[BUFFER_SIZE];
  int i;
  
  god = FALSE;
  
  if (unum == -1)		/* get god status for current user */
    {
      strncpy(myname, clbGetUserLogname(), BUFFER_SIZE);
      myname[BUFFER_SIZE - 1] = EOS;
    }
  else
    {				/* else a user number passed in */
				/* just check for OOPT_OPER */
      if (Users[unum].ooptions[OOPT_OPER])
	return TRUE;
      else
	return FALSE;
    }
  
  if (grp == NULL)
    {				/* first time */
      grp = getgrnam(CONQUEST_GROUP);
      
      if (grp == NULL)
	{
	  utLog("isagod(%s): getgrnam(%s) failed: %s",
	       myname,
	       CONQUEST_GROUP,
	       strerror(errno));
	  
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

#endif /* !CYGWIN */





/*  upchuck - update the common block to disk. */
/*  SYNOPSIS */
/*    upchuck */
void upchuck(void)
{
  
  PVLOCK(&ConqInfo->lockword);
  
  utFormatTime( ConqInfo->lastupchuck, 0 );
  flush_common();
  
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
    utGrand(etemp);
  
  *eaccum = *eaccum + utDeltaGrand( *etemp, &now );

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
int checkPID(int pidnum)
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
