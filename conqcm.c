#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/*                               C O N Q C M */
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

#include "global.h"

#include "conqdef.h"

#define NOCOMEXTERN
#include "conqcom.h"		/* common block vars defined here */
#include "conqcom2.h"		/* some extra stuff */
#include "user.h"
static char *conqcmId = "$Id$";

static char *cBasePtr = NULL;	/* common block ptr */
static int coff = 0;		/* offset into common block */

#define CMN_MODE 0660		/* mode of a new common block */

				/* map a 1D variable into the common block */
#define map1d(thevarp, thetype, size) {  \
              thevarp = (thetype *) (cBasePtr + coff); \
              coff += (sizeof(thetype) * (size)); \
	}

				/* map a 2D variable into the common block */
#define map2d(thevarp, thetype, sizex, sizey) { \
              int i; \
              thevarp = (thetype **) mymalloc((sizex) * sizeof(thetype *));  \
	      for (i=0; i<(sizex); i++) \
		thevarp[i] = (thetype *) ((cBasePtr + coff) + (sizey) * \
					  sizeof(thetype) * i); \
	      coff += ((sizex) * (sizey) * sizeof(thetype)); \
	}

				/* map a 3D variable into the common block */
#define map3d(thevarp, thetype, sizex, sizey, sizez) { \
              int i, j; \
              thevarp = (thetype ***) mymalloc((sizex) * sizeof(thetype **));  \
	      for (i=0; i<(sizex); i++) \
		thevarp[i] = (thetype **) mymalloc((sizey) * sizeof(thetype *));\
	      for (i=0; i<(sizex); i++) \
		for (j=0; j<(sizey); j++) \
		     thevarp[i][j]= (thetype *)\
                      (cBasePtr + coff) + (sizez) * sizeof(thetype) * j + \
		        ((sizey) * (sizez) * i); \
	      coff += ((sizex) * (sizey) * (sizez) * sizeof(thetype)); \
	}


#if defined(USE_PVLOCK)		/* use a block locking mech */

# if defined(USE_SEMS)		/* use semaphores for locking (BEST) */
				/* when using sems, we use lockword
				   and lockmesg as counters */

/* we'll use a hack to translate the lock[mesg|word] pointers into
   a semaphore selector */

void PVLOCK(int *lockptr)
{
  int semnum;
  
  if (lockptr == &ConqInfo->lockmesg)
    semnum = LOCKMSG;
  else
    semnum = LOCKCMN;

  Lock(semnum);

  (*lockptr)++;

  return;
}
    
void PVUNLOCK(int *lockptr)
{
  int semnum;
  
  if (lockptr == &ConqInfo->lockmesg)
    semnum = LOCKMSG;
  else
    semnum = LOCKCMN;

  Unlock(semnum);

  return;
}
    
# else /* don't USE_SEMS - Bad idea... */  

void PVLOCK(int *lockptr)
{

  const int Timeout = 30;	/* ~3 seconds */
  static int tenths;
  int t;

  tenths = 0;
  while (*lockptr != 0 && tenths < Timeout) 
    {
#ifdef DEBUG_LOCKING
      if ((tenths % 2) == 0)
	clog("conqcm: PVLOCK(): lock held by %d, tenths = %d",
	     *lockptr,
	     tenths);
#endif

      c_sleep(0.1);
      tenths++;
    }

  if (tenths < Timeout)
    *lockptr = cpid;
  else
    {
      t = *lockptr;
      PVUNLOCK(lockptr);
      *lockptr = cpid;
      clog("conqcm: PVLOCK(%d): timed out - pid: %d was holding the lock",
	     cpid, t);
    }

  if (*lockptr != cpid)
    clog("conqcm: PVLOCK(%d): Lost the lock to: %d!", 
	 cpid,
	 *lockptr);

  return;
}

void PVUNLOCK(int *lockptr)
{

  *lockptr = 0;

  return;
}


# endif /* USE_SEMS */

#endif /* USE_PVLOCK */

/* flush_common() - flush a common block */
void flush_common(void)
{
				/* fbsd doesn't like MS_SYNC       */
				/* which is prefered, but oh well */
#if defined(FREEBSD)
  if (msync((caddr_t)cBasePtr, SIZEOF_COMMONBLOCK, 0) == -1)
#else
  if (msync((caddr_t)cBasePtr, SIZEOF_COMMONBLOCK, MS_SYNC) == -1)
#endif
    clog("flush_common(): msync(): %s", sys_errlist[errno]);

  return;
}

/* check_cblock() - open/verify a common block - init if necc, return TRUE
    if successful */
int check_cblock(char *fname, int fmode, int sizeofcb)
{
  int ffd;
  struct stat sbuf;
  int rv;

				/* first stat the file, if it exists
				   then verify the size.  unlink if size
				   mismatch */
  if (stat(fname, &sbuf) != -1)   /* ok if this fails */
    {				/* file exists - verify size */
      if (sbuf.st_size != sizeofcb)
	{
	  printf("%s: File size mismatch (expected %d, was %d), removing.\n", 
		 fname,
		 sizeofcb,
		 sbuf.st_size);
	  if (unlink(fname) == -1)
	    {
	      printf("check_cblock(): unlink(%s) failed: %s\n",
		     fname,
		     sys_errlist[errno]);
	      return(FALSE);
	    }
	}
    }


				/* ok, either the file exists with the right
				   size, or it doesn't exist at all -
				   now open (and create) if necc */

  umask(0);			/* clear umask, just in case... */

  if ((ffd = open(fname, O_RDONLY)) == -1)
    {				/* Error or not there...  */
      if (errno == ENOENT)	/* Not There */
	{			/* create it */
	  if ((ffd = creat(fname, fmode)) == -1)
	    {
	      printf("check_cblock(): creat(%s) failed: %s\n",
		     fname,
		     sys_errlist[errno]);
	      return(FALSE);
	    }
	  else
	    {			/* Create it */
	      printf("Initializing common block: %s\n", fname);
	      cBasePtr = (char *) mymalloc(sizeofcb); /* this exits if malloc fails */
	      cdfill('\0', cBasePtr, sizeofcb);

	      write(ffd, cBasePtr, sizeofcb);
	      close(ffd);
	      free(cBasePtr);
	      cBasePtr = NULL;
	    }
	}
      else
	{			/* some other error */
	  printf("check_cblock(): open(%s, O_RDONLY) failed: %s\n",
		 fname,
		 sys_errlist[errno]);
	  return(FALSE);
	}
    }
  
  close(ffd);			/* everything ok.. */

				/* set ownership */
  chown(fname, 0, -1);

  return(TRUE);			/* everything there, and right size */
}

				/* my malloc wrapper. used only when mapping
				   or initializing a commonblock */
char *mymalloc(int size) 
  { 
    char *ptr;

    if ((ptr = malloc(size)) == NULL) 
      { 
	perror("mymalloc"); 
	exit(1);
      } 
    return(ptr);
  }

#if defined(USE_COMMONMLOCK)
void lock_common(void)
{
				/* Lock it in memory.  this requires the
				   PLOCK privilege on Unixware. if we
				   fail, we'll complain to the logfile
				   and continue... */

  if (memcntl((caddr_t)cBasePtr, SIZEOF_COMMONBLOCK,
	     MC_LOCK, (caddr_t)0, 0, 0) == -1)
    {
      clog("map_common(): couldn't lock the common block: %s, continuing...",
	   sys_errlist[errno]);
    }
}
#endif

void map_common(void)
{
  int cmn_fd;
  static char cmnfile[MID_BUFFER_SIZE];

  coff = 0;
  
  sprintf(cmnfile, "%s/%s", CONQHOME, C_CONQ_COMMONBLK);

				/* verify it's validity */
  if (check_cblock(cmnfile, CMN_MODE, SIZEOF_COMMONBLOCK) == FALSE)
    exit(1);			/* an unrecoverable error */

				/* reopen it... */
  if ((cmn_fd = open(cmnfile, O_RDWR)) == -1)
    {
      perror("map_common:open(O_RDWR)");
      exit(1);
    }
  
  /* Now lets map it */

#ifndef MAP_FILE
# define MAP_FILE 0		/* some arch's don't def this */
#endif

  if ((cBasePtr = mmap((caddr_t) 0, (size_t) SIZEOF_COMMONBLOCK, 
                       PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, 
		       cmn_fd, 0)) == (caddr_t) -1)
    {
      perror("map_common():mmap()");
      exit(1);
    }

				        /* now map the variables into the
					   common block */

  map1d(CBlockRevision, int, 1);	/* this *must* be the first var */
  map1d(ConqInfo, ConqInfo_t, 1)

  map1d(Users, User_t, MAXUSERS);

  map1d(Robot, Robot_t, 1);

  map1d(Planets, Planet_t, NUMPLANETS + 1);

  map1d(Teams, Team_t, NUMALLTEAMS);

  map1d(Doomsday, Doomsday_t, 1);

  map1d(History, History_t, MAXHISTLOG);

  map1d(Driver, Driver_t, 1);

  map1d(Ships, Ship_t, MAXSHIPS + 1);

  map1d(Msgs, Msg_t, MAXMESSAGES);

  map1d(EndOfCBlock, int, 1);
				/* now lets lock it */
#if defined(USE_COMMONMLOCK)
  lock_common();
#endif

  return;
}

void zero_common(void)
{				/* zero the common block, called from
				   init everything */
  cdfill('\0', cBasePtr, SIZEOF_COMMONBLOCK);
  upchuck();			/* flush the commonblock */

  return;
}


