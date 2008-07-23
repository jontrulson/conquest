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
#include "conqutil.h"

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
      utLog("semInit(): semget(IPC_CREAT): failed: %s",
	   strerror(errno));
#endif

      semflags = CONQSEMPERMS;

      ConquestSemID = semget(CONQSEMKEY, CONQNUMSEMS, semflags);

      if (ConquestSemID == -1)
	{
#ifdef DEBUG_SEM
	  utLog("semInit(): semget(GET): failed: %s",
	       strerror(errno));
#endif

	  fprintf(stderr, "semInit(): can't get semaphore: %s",
		  strerror(errno));

	  return(ERR);
	}
    }

				/* hopefully we have an ID now.. */


#if defined(DEBUG_FLOW) || defined(DEBUG_SEM)
  utLog("semInit(): semget(GET): succeeded, semaphore ID is %d",
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
  utLog("Lock(%s): Attempting to aquire a lock.",
       semGetName(what));
  utLog("Lock(%s): %s", semGetName(what), semGetStatusStr());
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
	    utLog("Lock(%s): semop(): failed: %s",
		 semGetName(what),
		 strerror(err));
	    fprintf(stderr, "Lock(%s): semop(): failed: %s\n",
		 semGetName(what),
		 strerror(err));
	    
	    exit(1);
	  }
	else
	  {
	    utLog("Lock(%s): semop(): interrupted. Retrying lock attempt.", semGetName(what));
	  }
      }
      else			/* we got a successful lock */
	Done = TRUE;
    }

#ifdef DEBUG_SEM
  utLog("Lock(%s): semop(): succeeded, got a lock",
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


#ifdef DEBUG_SEM
  utLog("Unlock(%s): Attempting to free a lock.",
       semGetName(what));
  utLog("Unlock(%s): %s", semGetName(what), semGetStatusStr());
#endif


  /* get the values of the semaphores */
  arg.array = semvals;
  retval = semctl(ConquestSemID, 0, GETALL, arg);
  
  if (retval != 0)
    {				/* couldn't get semvals */
#if !defined(CYGWIN)
      utLog("Unlock(%s): semctl(GETALL) failed: %s",
	   semGetName(what),
	   strerror(errno));
#endif
    }
  else
    {				/* got semvals... */
				/* check to see if already unlocked */
      if (semvals[what] == 0)	/* sem already unlocked - report and continue */
	{
	  utLog("Unlock(%s): semaphore already unlocked.",
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
	  utLog("Unlock(%s): semop(): failed: %s",
	       semGetName(what),
	       strerror(errno));
	  fprintf(stderr,"Unlock(%s): semop(): failed: %s",
	       semGetName(what),
	       strerror(errno));
	  exit(1);
	}
      else
	{
	  utLog("Unlock(%s): semop(): interrupted. continuing...", semGetName(what));
	  err = EINTR;
	}
    }

				/* hopefully we got a lock */
#ifdef DEBUG_SEM
  if (!err)
    utLog("Unlock(%s): semop(): succeeded, removed lock",
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

#if defined(CYGWIN)
  /* apparently not implemented */
  lastcmnpid = 0;
  cmnzcnt = 0;
  lastmsgpid = 0;
  msgzcnt = 0;
#else /* !CYGWIN */

  lastcmnpid = semctl(ConquestSemID, LOCKCMN, GETPID, arg);
  cmnzcnt = semctl(ConquestSemID, LOCKCMN, GETZCNT, arg);
  lastmsgpid = semctl(ConquestSemID, LOCKMSG, GETPID, arg);
  msgzcnt = semctl(ConquestSemID, LOCKMSG, GETZCNT, arg);
#endif

 
#if defined(CYGWIN)
  /* these do not appear to be implemented in cygwin */
  lastoptime = 0;
#else /* !CYGWIN */    

  /* get latest semop time  */
  arg.buf = &SemDS;
  retval = semctl(ConquestSemID, LOCKMSG, IPC_STAT, arg);

  if (retval != 0)
    {
      utLog("semGetStatusStr(): %s semctl(IPC_STAT) failed: %s",
           semGetName(LOCKMSG),
	   strerror(errno));
    }

  lastoptime = SemDS.sem_otime;

  retval = semctl(ConquestSemID, LOCKCMN, IPC_STAT, arg);

  if (retval != 0)
    {
      utLog("semGetStatusStr(%d): %s semctl(IPC_STAT) failed: %s",
           semGetName(LOCKCMN),
	   strerror(errno));
    }

  lastoptime = max(lastoptime, SemDS.sem_otime);
#endif

  /* get the values of the semaphores */
  arg.array = semvals;
  retval = semctl(ConquestSemID, 0, GETALL, arg);

  if (retval != 0)
    {
      utLog("semGetStatusStr(): semctl(GETALL) failed: %s",
	   strerror(errno));
    }


  snprintf(mesgtxt, 80 - 1, "%sMesgCnt = %d(%d:%d)", 
           (semvals[LOCKMSG]) ? "*" : "",
           ConqInfo->lockmesg, 
           lastmsgpid, 
           msgzcnt);

  snprintf(wordtxt, 80 - 1, "%sCmnCnt = %d(%d:%d)", 
           (semvals[LOCKCMN]) ? "*" : "",
           ConqInfo->lockword, 
           lastcmnpid, 
           cmnzcnt);

  strcpy(stimebuffer, ctime(&lastoptime));
  strncpy(newtime, &stimebuffer[4], 15); /* get the interesting part */

				/* now build the string */

  snprintf(buf, 80 - 1, "%s %s Last: %s",
	  mesgtxt,
	  wordtxt,
	  newtime);

  return(buf);
}

