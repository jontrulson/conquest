#include "c_defs.h"

/************************************************************************
 *
 * sem.c - semaphore operations on the common block. Finally.
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/signal.h>

#include "defs.h"
#include "global.h"
#include "conqcom.h"
#include "conqlb.h"

#define CONQSEMKEY   (0xff001701) /* hope that's unique! */
#define CONQSEMPERMS (00664)
#define CONQNUMSEMS  (2)

static key_t ConquestSemID = -1; 
static struct sembuf semops[CONQNUMSEMS];

char *semGetName(int what)
{
  static char *LMSGTXT = "LOCKCOMN";
  static char *LCMNTXT = "LOCKMESG";

  if (what == LOCKMSG)
    return(LMSGTXT);
  else
    return(LCMNTXT);
}

int semInit(void)
{
  int semflags;

				/* try to create first */
  semflags = CONQSEMPERMS | IPC_CREAT;

  ConquestSemID = semget(CONQSEMKEY, CONQNUMSEMS, semflags);

  if (ConquestSemID == -1)
    {				/* already exists? */
#ifdef DEBUG_SEM
      clog("semInit(): semget(IPC_CREAT): failed: %s",
	   strerror(errno));
#endif

      semflags = CONQSEMPERMS;

      ConquestSemID = semget(CONQSEMKEY, CONQNUMSEMS, semflags);

      if (ConquestSemID == -1)
	{
#ifdef DEBUG_SEM
	  clog("semInit(): semget(GET): failed: %s",
	       strerror(errno));
#endif

	  fprintf(stderr, "semInit(): can't get semaphore: %s",
		  strerror(errno));

	  return(ERR);
	}
    }

				/* hopefully we have an ID now.. */


#if defined(DEBUG_FLOW) || defined(DEBUG_SEM)
  clog("semInit(): semget(GET): succeeded, semaphore ID is %d",
       ConquestSemID);
#endif
  
  return(TRUE);
}


/* Lock() - lock part of the common block by attempting to inc a semaphore. */
void Lock(int what)
{
  static int Done;

  if (ConquestSemID == -1)
    return;			/* clients don't use sems... */

#ifdef DEBUG_SEM
  clog("Lock(%s): Attempting to aquire a lock.",
       semGetName(what));
  clog("Lock(%s): %s", semGetName(what), semGetStatusStr());
#endif

  Done = FALSE;
				/* Wait for sem to be zero, then inc */
  semops[0].sem_num = (short)what;
  semops[0].sem_op = 0;		/* test for 0, if so... */
  semops[0].sem_flg = 0;

				/* then increment it. */
  semops[1].sem_num = (short)what;
  semops[1].sem_op = 1;
  semops[1].sem_flg = SEM_UNDO;	/* undo if we die unexpectedly */
  
                                /* block ALRM signals */
  clbBlockAlarm();

  while (Done == FALSE)
    {
      if (semop(ConquestSemID, semops, 2) == -1)
      {
	if (errno != EINTR)
	  {
	    int err;

	    err = errno;
	    clog("Lock(%s): semop(): failed: %s",
		 semGetName(what),
		 strerror(err));
	    fprintf(stderr, "Lock(%s): semop(): failed: %s\n",
		 semGetName(what),
		 strerror(err));
	    
	    exit(1);
	  }
	else
	  {
	    clog("Lock(%s): semop(): interrupted. Retrying lock attempt.", semGetName(what));
	  }
      }
      else			/* we got a successful lock */
	Done = TRUE;
    }

#ifdef DEBUG_SEM
  clog("Lock(%s): semop(): succeeded, got a lock",
       semGetName(what));
#endif

  return;
}

/* Unlock() - unlock part of the common block (dec a semaphore to 0) */
void Unlock(int what)
{
  int err = 0, retval;
  ushort semvals[25];
  union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
  } arg;

  if (ConquestSemID == -1)
    return;			/* clients don't use sems... */

  arg.array = semvals;

#ifdef DEBUG_SEM
  clog("Unlock(%s): Attempting to free a lock.",
       semGetName(what));
  clog("Unlock(%s): %s", semGetName(what), semGetStatusStr());
#endif


				/* get the values of the semaphores */
  retval = semctl(ConquestSemID, 0, GETALL, arg);
  
  if (retval != 0)
    {				/* couldn't get semvals */
#if !defined(CYGWIN)
      clog("Unlock(%s): semctl(GETALL) failed: %s",
	   semGetName(what),
	   strerror(errno));
#endif
    }
  else
    {				/* got semvals... */
				/* check to see if already unlocked */
      if (semvals[what] == 0)	/* sem already unlocked - report and continue */
	{
	  clog("Unlock(%s): semaphore already unlocked.",
	       semGetName(what));
	  
          /* allow alarms again */
          clbUnblockAlarm();
	  return;
	}
    }


				/* Decrement to 0 */
  semops[0].sem_num = (short)what;
  semops[0].sem_op = -1;
  semops[0].sem_flg = 0;

  if (semop(ConquestSemID, semops, 1) == -1)
    {
      if (errno != EINTR)
	{
	  clog("Unlock(%s): semop(): failed: %s",
	       semGetName(what),
	       strerror(errno));
	  fprintf(stderr,"Unlock(%s): semop(): failed: %s",
	       semGetName(what),
	       strerror(errno));
	  exit(1);
	}
      else
	{
	  clog("Unlock(%s): semop(): interrupted. continuing...", semGetName(what));
	  err = EINTR;
	}
    }

				/* hopefully we got a lock */
#ifdef DEBUG_SEM
  if (!err)
    clog("Unlock(%s): semop(): succeeded, removed lock",
	 semGetName(what));
#endif

  /* allow alarms again */
  clbUnblockAlarm();

  return;
}


char *semGetStatusStr(void)
{
  struct semid_ds SemDS;
  ushort semvals[25];
  static char buf[80];
  static char stimebuffer[80];
  static char wordtxt[80];
  static char mesgtxt[80];
  static char newtime[80];
  time_t lastoptime;
  int retval;
  int lastcmnpid, cmnzcnt, lastmsgpid, msgzcnt;
  union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
  } arg;

				/* get the values of the semaphores */
  arg.array = semvals;
  retval = semctl(ConquestSemID, 0, GETALL, arg);

#if defined(CYGWIN)
  /* apparently not implemented */
  lastcmnpid = 0;
  cmnzcnt = 0;
  lastmsgpid = 0;
  msgzcnt = 0;
#else /* !CYGWIN */

  lastcmnpid = semctl(ConquestSemID, LOCKCMN, GETPID, semvals);
  cmnzcnt = semctl(ConquestSemID, LOCKCMN, GETZCNT, semvals);
  lastmsgpid = semctl(ConquestSemID, LOCKMSG, GETPID, semvals);
  msgzcnt = semctl(ConquestSemID, LOCKMSG, GETZCNT, semvals);
#endif
  retval = semctl(ConquestSemID, 0, GETALL, semvals);

  if (retval != 0)
    {
      clog("semGetStatusStr(): semctl(GETALL) failed: %s",
	   strerror(errno));
    }

 
  arg.buf = &SemDS;

#if defined(CYGWIN)
  /* these do not appear to be implemented in cygwin */
  lastoptime = 0;
#else /* !CYGWIN */    
				/* get latest semop time  */
  retval = semctl(ConquestSemID, LOCKMSG, IPC_STAT, arg);

  if (retval != 0)
    {
      clog("semGetStatusStr(): %s semctl(IPC_STAT) failed: %s",
           semGetName(LOCKMSG),
	   strerror(errno));
    }

  lastoptime = SemDS.sem_otime;

  retval = semctl(ConquestSemID, LOCKCMN, IPC_STAT, arg);

  if (retval != 0)
    {
      clog("semGetStatusStr(%d): %s semctl(IPC_STAT) failed: %s",
           semGetName(LOCKCMN),
	   strerror(errno));
    }

  lastoptime = max(lastoptime, SemDS.sem_otime);
#endif

  if (semvals[LOCKMSG] != 0)	/* currently locked */
    sprintf(mesgtxt, "*MesgCnt = %d(%d:%d)", ConqInfo->lockmesg, lastmsgpid, msgzcnt);
  else
    sprintf(mesgtxt, "MesgCnt = %d(%d:%d)", ConqInfo->lockmesg, lastmsgpid, msgzcnt);

  if (semvals[LOCKCMN] != 0)
    sprintf(wordtxt, "*CmnCnt = %d(%d:%d)", ConqInfo->lockword, lastcmnpid, cmnzcnt);
  else
    sprintf(wordtxt, "CmnCnt = %d(%d:%d)", ConqInfo->lockword, lastcmnpid, cmnzcnt);

  strcpy(stimebuffer, ctime(&lastoptime));
  strncpy(newtime, &stimebuffer[4], 15); /* get the interesting part */

				/* now build the string */

  sprintf(buf, "%s %s Last: %s",
	  mesgtxt,
	  wordtxt,
	  newtime);

  return(buf);
}

