#include "c_defs.h"

/************************************************************************
 *
 * sem.c - semaphore operations on the common block. Finally.
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#include <sys/ipc.h>
#include <sys/sem.h>

#include "defs.h"
#include "global.h"
#include "conqcom.h"

		/* For id purposes... */
static char *semId = "$Id$";

#define CONQSEMKEY   (0xff001701) /* hope that's unique! */
#define CONQSEMPERMS (00664)
#define CONQNUMSEMS  (2)

static int ConquestSemID;
static struct sembuf semops[CONQNUMSEMS], *sops;

char *getsemtxt(int what)
{
  static char *LMSGTXT = "LOCKMESG";
  static char *LCMNTXT = "LOCKWORD";

  if (what == LOCKMSG)
    return(LMSGTXT);
  else
    return(LCMNTXT);
}

int GetSem(void)
{
  key_t semkey;
  int semflags;
  int retval;

				/* try to create first */
  semflags = CONQSEMPERMS | IPC_CREAT;

  ConquestSemID = semget(CONQSEMKEY, CONQNUMSEMS, semflags);

  if (ConquestSemID == -1)
    {				/* already exists? */
#ifdef DEBUG_SEM
      clog("GetSem(): semget(IPC_CREAT): failed: %s",
	   sys_errlist[errno]);
#endif

      semflags = CONQSEMPERMS;

      ConquestSemID = semget(CONQSEMKEY, CONQNUMSEMS, semflags);

      if (ConquestSemID == -1)
	{
#ifdef DEBUG_SEM
	  clog("GetSem(): semget(GET): failed: %s",
	       sys_errlist[errno]);
#endif

	  fprintf(stderr, "GetSem(): can't get semaphore: %s",
		  sys_errlist[errno]);

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

  Done = FALSE;
				/* Wait for sem to be zero, then inc */
  semops[0].sem_num = what;
  semops[0].sem_op = 0;		/* test for 0, if so... */
  semops[0].sem_flg = 0;

				/* then increment it. */
  semops[1].sem_num = what;
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
		 sys_errlist[err]);
	    fprintf(stderr, "Lock(%s): semop(): failed: %s\n",
		 getsemtxt(what),
		 sys_errlist[err]);
	    
	    cdend();
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

				/* hopefully we got a lock */
#ifdef DEBUG_SEM
  clog("Lock(%s): semop(): succeeded, got a lock",
       getsemtxt(what));
#endif

  return;
}

/* Unlock() - unlock part of the common block (dec a semaphore to 0) */
void Unlock(int what)
{
  int err = 0;
  int retval;
  ushort semvals[16];

				/* get the values of the semaphores */
  retval = semctl(ConquestSemID, 0, GETALL, semvals);
  
  if (retval != 0)
    {				/* couldn't get semvals */
      clog("Unlock(%s): semctl(GETALL) failed: %s",
	   getsemtxt(what),
	   sys_errlist[errno]);
    }
  else
    {				/* got semvals.. check it */
				/* check to see if already unlocked */
      if (semvals[what] == 0)	/* sem already unlocked - report and continue */
	{
	  clog("Unlock(%s): semaphore already unlocked.",
	       getsemtxt(what));
	  
	  return;
	}
    }


				/* Decrement to 0 */
  semops[0].sem_num = what;
  semops[0].sem_op = -1;
  semops[0].sem_flg = 0;

  if (semop(ConquestSemID, semops, 1) == -1)
    {
      if (errno != EINTR)
	{
	  clog("Unlock(%s): semop(): failed: %s",
	       getsemtxt(what),
	       sys_errlist[errno]);
	  
	  cdend();
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
  struct sem *Sem;
  ushort semvals[16];
  static char buf[80];
  static char tmbuf[80];
  static char wordtxt[80];
  static char mesgtxt[80];
  static char newtime[80];
  int retval;
  int lastcmnpid, cmnzcnt, lastmsgpid, msgzcnt;

				/* get the values of the semaphores */
  retval = semctl(ConquestSemID, 0, GETALL, semvals);

  lastcmnpid = semctl(ConquestSemID, LOCKCMN, GETPID, semvals);
  cmnzcnt = semctl(ConquestSemID, LOCKCMN, GETZCNT, semvals);
  lastmsgpid = semctl(ConquestSemID, LOCKMSG, GETPID, semvals);
  msgzcnt = semctl(ConquestSemID, LOCKMSG, GETZCNT, semvals);
  retval = semctl(ConquestSemID, 0, GETALL, semvals);

  if (retval != 0)
    {
      clog("GetSemVal(%d): semctl(GETALL) failed: %s",
	   thesem,
	   sys_errlist[errno]);
    }

				/* get last semop time  */
  retval = semctl(ConquestSemID, thesem, IPC_STAT, &SemDS);

  if (retval != 0)
    {
      clog("GetSemVal(%d): semctl(IPC_STAT) failed: %s",
	   thesem,
	   sys_errlist[errno]);
    }

  Sem = SemDS.sem_base;

  if (semvals[LOCKMSG] != 0)	/* currently locked */
    sprintf(mesgtxt, "*MesgCnt = %d(%d:%d)", *lockmesg, lastmsgpid, msgzcnt);
  else
    sprintf(mesgtxt, "MesgCnt = %d(%d:%d)", *lockmesg, lastmsgpid, msgzcnt);

  if (semvals[LOCKCMN] != 0)
    sprintf(wordtxt, "*CmnCnt = %d(%d:%d)", *lockword, lastcmnpid, cmnzcnt);
  else
    sprintf(wordtxt, "CmnCnt = %d(%d:%d)", *lockword, lastcmnpid, cmnzcnt);


  strcpy(tmbuf, ctime(&SemDS.sem_otime));

  strncpy(newtime, &tmbuf[4], 15); /* get the interesting part */

				/* now build the string */

  sprintf(buf, "%s %s Last: %s",
	  mesgtxt,
	  wordtxt,
	  newtime);

  return(buf);
}

