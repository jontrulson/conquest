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
  
  if (lockptr == lockmesg)
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
  
  if (lockptr == lockmesg)
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

				/* my malloc wrapper. used only when mapping
				   the commonblock */
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
  
  umask(0);			/* clear umask, just in case... */

  sprintf(cmnfile, "%s/%s", CONQHOME, C_CONQ_COMMONBLK);

  if ((cmn_fd = open(cmnfile, O_RDONLY)) == -1)
    {				/* Error or not there...  */
      if (errno == ENOENT)	/* Not There */
	{			/* create it */
	  if ((cmn_fd = creat(cmnfile, CMN_MODE)) == -1)
	    {
	      perror("map_common:creat(CMN_MODE)");
	      exit(1);
	    }
	  else
	    {			/* Create it */
	      printf("Creating conquest common block: %s\n", cmnfile);
	      cBasePtr = (char *) mymalloc(SIZEOF_COMMONBLOCK);
	      cdfill('\0', cBasePtr, SIZEOF_COMMONBLOCK);

	      write(cmn_fd, cBasePtr, SIZEOF_COMMONBLOCK);
	      close(cmn_fd);
	      free(cBasePtr);
	    }
	}
      else
	{			/* some other error */
	  perror("map_common:open(O_RDONLY)");
	  exit(1);
	}
    }
  
  close(cmn_fd);		/* everything ok.. */

				/* set ownership */
  chown(cmnfile, 0, -1);

				/* reopen it... */
  if ((cmn_fd = open(cmnfile, O_RDWR)) == -1)
    {
      perror("map_common:open(O_RDWR)");
      exit(1);
    }
  
  /* Now lets map it */

  if ((cBasePtr = mmap((caddr_t) 0, (size_t) SIZEOF_COMMONBLOCK, 
#if !defined(LINUX)
                       PROT_READ | PROT_WRITE, MAP_SHARED,
#else
                       PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, 
#endif
		       cmn_fd, 0)) == (caddr_t) -1)
    {
      perror("map_common():mmap()");
      exit(1);
    }

			/* now map the variables into the common block */
  map1d(commonrev, int, 1);
  map1d(closed, int, 1);
  map1d(lockword, int, 1);
  map1d(lockmesg, int, 1);

  map1d(conqueror, char, SIZEUSERPNAME);
  map1d(conqteam, char, MAXTEAMNAME);
  map1d(conqtime, char, DATESIZE);
  map1d(lastwords, char, MAXLASTWORDS);
  map1d(inittime, char, DATESIZE);

  map1d(ccpuseconds, int, 1);
  map1d(celapsedseconds, int, 1);
  map1d(dcpuseconds, int, 1);		
  map1d(delapsedseconds, int, 1);	
  map1d(rcpuseconds, int, 1);		
  map1d(relapsedseconds, int, 1);
  map1d(raccum, int, 1);

  map1d(lastupchuck, char, DATESIZE);	

  map1d(Users, User_t, MAXUSERS);

  map2d(rstrat, int, REAL_MAX_VAR, 10);
  map1d(rvec, int, 32);

  map2d(ptname, char, MAXPLANETTYPES, MAXPTYPENAME);

  map1d(Planets, Planet_t, NUMPLANETS + 1);

  map1d(Teams, Team_t, NUMALLTEAMS);

  map1d(chrplanets, char, MAXPLANETTYPES);

  map1d(Doomsday, Doomsday_t, 1);

  map1d(histptr, int, 1);		
  map1d(histunum, int, MAXHISTLOG);	
  map2d(histlog, char, MAXHISTLOG, DATESIZE); 

  map1d(drivstat, int, 1);
  map1d(drivpid, int, 1);	
  map1d(drivcnt, int, 1);	
  map1d(drivsecs, int, 1);	
			
  map1d(drivowner, char, SIZEUSERNAME);
  map1d(drivtime, int, 1);
  map1d(playtime, int, 1);

  map1d(Ships, Ship_t, MAXSHIPS + 1);

  map1d(Msgs, Msg_t, MAXMESSAGES);
	
  map1d(lastmsg, int, 1);		

  map1d(externrobots, int, 1);		

  map1d(glastmsg, int, 1);	

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


