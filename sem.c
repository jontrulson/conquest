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

#include "defs.h"
#include "global.h"
#include "conqcom.h"

#define CONQSEMKEY   (0xff001701) /* hope that's unique! */
#define CONQSEMPERMS (00664)
#define CONQNUMSEMS  (2)

static key_t ConquestSemID = -1; 
static struct sembuf semops[CONQNUMSEMS];

char *getsemtxt(int what)
{
  static char *LMSGTXT = "LOCKCOMN";
  static char *LCMNTXT = "LOCKMESG";

  if (what == LOCKMSG)
    return(LMSGTXT);
  else
    return(LCMNTXT);
}

int GetSem(void)
{
  int semflags;

				/* try to create first */
  semflags = CONQSEMPERMS | IPC_CREAT;

  ConquestSemID = semget(CONQSEMKEY, CONQNUMSEMS, semflags);

  if (ConquestSemID == -1)
    {				/* already exists? */
#ifdef DEBUG_SEM
      clog("GetSem(): semget(IPC_CREAT): failed: %s",
	   strerror(errno));
#endif

      semflags = CONQSEMPERMS;

      ConquestSemID = semget(CONQSEMKEY, CONQNUMSEMS, semflags);

      if (ConquestSemID == -1)
	{
#ifdef DEBUG_SEM
	  clog("GetSem(): semget(GET): failed: %s",
	       strerror(errno));
#endif

	  fprintf(stderr, "GetSem(): can't get semaphore: %s",
		  strerror(errno));

	  return(ERR);
	}
    }

				/* hopefully we have an ID now.. */


#if defined(DEBUG_FLOW) || defined(DEBUG_SEM)
  clog("GetSem(): semget(GET): succeeded, semaphore ID is %d",
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
       getsemtxt(what));
  clog("Lock(%s): %s", getsemtxt(what), GetSemVal(0));
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
  
  while (Done == FALSE)
    {
      if (semop(ConquestSemID, semops, 2) == -1)
      {
	if (errno != EINTR)
	  {
	    int err;

	    err = errno;
	    clog("Lock(%s): semop(): failed: %s",
		 getsemtxt(what),
		 strerror(err));
	    fprintf(stderr, "Lock(%s): semop(): failed: %s\n",
		 getsemtxt(what),
		 strerror(err));
	    
	    exit(1);
	  }
	else
	  {
	    clog("Lock(%s): semop(): interrupted. Retrying lock attempt.", getsemtxt(what));
	  }
      }
      else			/* we got a successful lock */
	Done = TRUE;
    }

#ifdef DEBUG_SEM
  clog("Lock(%s): semop(): succeeded, got a lock",
       getsemtxt(what));
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
       getsemtxt(what));
  clog("Unlock(%s): %s", getsemtxt(what), GetSemVal(0));
#endif


				/* get the values of the semaphores */
  retval = semctl(ConquestSemID, 0, GETALL, arg);
  
  if (retval != 0)
    {				/* couldn't get semvals */
#if !defined(CYGWIN)
      clog("Unlock(%s): semctl(GETALL) failed: %s",
	   getsemtxt(what),
	   strerror(errno));
#endif
    }
  else
    {				/* got semvals... */
				/* check to see if already unlocked */
      if (semvals[what] == 0)	/* sem already unlocked - report and continue */
	{
	  clog("Unlock(%s): semaphore already unlocked.",
	       getsemtxt(what));
	  
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
	       getsemtxt(what),
	       strerror(errno));
	  fprintf(stderr,"Unlock(%s): semop(): failed: %s",
	       getsemtxt(what),
	       strerror(errno));
	  exit(1);
	}
      else
	{
	  clog("Unlock(%s): semop(): interrupted. continuing...", getsemtxt(what));
	  err = EINTR;
	}
    }

				/* hopefully we got a lock */
#ifdef DEBUG_SEM
  if (!err)
    clog("Unlock(%s): semop(): succeeded, removed lock",
	 getsemtxt(what));
#endif

  return;
}


char *GetSemVal(int thesem)
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
      clog("GetSemVal(%d): semctl(GETALL) failed: %s",
	   thesem,
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
      clog("GetSemVal(%d): semctl(IPC_STAT) failed: %s",
	   0,
	   strerror(errno));
    }

  lastoptime = SemDS.sem_otime;

  retval = semctl(ConquestSemID, LOCKCMN, IPC_STAT, arg);

  if (retval != 0)
    {
      clog("GetSemVal(%d): semctl(IPC_STAT) failed: %s",
	   1,
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

