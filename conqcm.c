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
/* Have Phun!                                                         */
/**********************************************************************/

#include "global.h"

#include "conqdef.h"

#define NOCOMEXTERN
#include "conqcom.h"		/* common block vars defined here */
#include "conqcom2.h"		/* some extra stuff */

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
/* let's try sems.. */

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
    
# else /* don't USE_SEMS */  

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
				/* Lock it in memory */

  if (memcntl((caddr_t)cBasePtr, SIZEOF_COMMONBLOCK,
	     MC_LOCK, (caddr_t)0, 0, 0) == -1)
    {
      clog("map_common(): couldn't lock the common block: %s, continuing...",
	   sys_errlist[errno]);
/*      exit(2);*/
    }
}
#endif

void map_common(void)
{
  int cmn_fd;
  char cmnfile[BUFFER_SIZE];
  extern char *c_conq_commonblk;

  coff = 0;
  
  umask(0);			/* clear umask, just in case... */

  sprintf(cmnfile, "%s/%s", CONQHOME, c_conq_commonblk);

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
  
  /* Now lets mmap() it! */

  if ((cBasePtr = mmap((caddr_t) 0, (size_t) SIZEOF_COMMONBLOCK, 
#if !defined(LINUX)
                       PROT_READ | PROT_WRITE, MAP_SHARED,
#else
                       PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, 
#endif
		       cmn_fd, 0)) == (caddr_t) -1)
    {
      perror("map_common():mmap()");
      printf("map_common():mmap(): %s\n", sys_errlist[errno]);
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
  map2d(tstats, int, NUMTEAMS, MAXTSTATS);
  map1d(tcoupinfo, int, NUMTEAMS);

  map1d(ccpuseconds, int, 1);
  map1d(celapsedseconds, int, 1);
  map1d(dcpuseconds, int, 1);		
  map1d(delapsedseconds, int, 1);	
  map1d(rcpuseconds, int, 1);		
  map1d(relapsedseconds, int, 1);
  map1d(raccum, int, 1);

  map1d(lastupchuck, char, DATESIZE);	

  map1d(ulive, int, MAXUSERS);
  map1d(uteam, int, MAXUSERS);	
  map1d(umultiple, int, MAXUSERS);	
  map2d(ustats, int, MAXUSERS, MAXUSTATS);
  map1d(urating, real, MAXUSERS);		
  map1d(urobot, int, MAXUSERS);	
  map2d(uwar, int, MAXUSERS, NUMTEAMS);	
  map2d(uoption, int, MAXUSERS, MAXOPTIONS); 
  map2d(uooption, int, MAXUSERS, MAXOOPTIONS);
  map2d(cuname, char, MAXUSERS, SIZEUSERNAME);
  map2d(upname, char, MAXUSERS, SIZEUSERPNAME);
  map2d(ulastentry, char, MAXUSERS, DATESIZE); 

  map2d(rstrat, int, REAL_MAX_VAR, 10);
  map1d(rvec, int, 32);

  map1d(pprimary, int, NUMPLANETS + 1);
  map1d(porbrad, real, NUMPLANETS + 1);
  map1d(porbvel, real, NUMPLANETS + 1);
  map1d(ptype, int, NUMPLANETS + 1);	
  map1d(preal, int, NUMPLANETS + 1);	
  map2d(pname, char, NUMPLANETS + 1, MAXPLANETNAME);
  map2d(ptname, char, MAXPLANETTYPES, MAXPTYPENAME);

  map1d(homeplanet, int, NUMTEAMS);	
  map1d(homesun, int, NUMTEAMS);	
  map2d(teamplanets, int, NUMTEAMS, 3);
  map1d(armylim, int, NUMTEAMS);		
  map1d(warplim, real, NUMTEAMS);
  map1d(engfac, real, NUMTEAMS);	
  map1d(accelfac, real, NUMTEAMS);	
  map1d(weafac, real, NUMTEAMS);	
  map1d(torpwarp, real, NUMTEAMS);	
  map2d(tname, char, NUMALLTEAMS, MAXTEAMNAME);

  map1d(chrplanets, char, MAXPLANETTYPES);
  map1d(chrtorps, char, NUMTEAMS);	
  map1d(chrteams, char, NUMALLTEAMS);	

  map1d(couptime, int, NUMTEAMS);	

  map1d(puninhabtime, int, NUMPLANETS + 1);	

  map1d(dstatus, int, 1);	
  map1d(dtype, int, 1);		
  map1d(dx, real, 1);		
  map1d(dy, real, 1);
  map1d(ddx, real, 1);		
  map1d(ddy, real, 1);
  map1d(dhead, real, 1);		
  map1d(dlock, int, 1);		
  map1d(dname, char, SIZEUSERPNAME);	

  map1d(porbang, real, NUMPLANETS + 1);	
  map1d(px, real, NUMPLANETS + 1);	
  map1d(py, real, NUMPLANETS + 1);	
  map1d(pteam, int, NUMPLANETS + 1);	
  map1d(parmies, int, NUMPLANETS + 1);	
  map2d(pscanned, int, NUMPLANETS + 1, NUMTEAMS); 
					

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

  map1d(sstatus, int, MAXSHIPS + 1);	
  map1d(skilledby, int, MAXSHIPS + 1);	
				
				
  map1d(suser, int, MAXSHIPS + 1);	
  map1d(steam, int, MAXSHIPS + 1);	
  map1d(spid, int, MAXSHIPS + 1);		
  map1d(sx, real, MAXSHIPS + 1);		
  map1d(sy, real, MAXSHIPS + 1);
  map1d(sdx, real, MAXSHIPS + 1);		
  map1d(sdy, real, MAXSHIPS + 1);		
  map1d(shead, real, MAXSHIPS + 1);	
  map1d(sdhead, real, MAXSHIPS + 1);	
  map1d(swarp, real, MAXSHIPS + 1);	
				
  map1d(sdwarp, real, MAXSHIPS + 1);	
  map1d(slock, int, MAXSHIPS + 1);	
				
  map1d(sshup, int, MAXSHIPS + 1);	
  map1d(sshields, real, MAXSHIPS + 1);	
  map1d(skills, real, MAXSHIPS + 1);	
  map1d(sdamage, real, MAXSHIPS + 1);	
  map1d(sfuel, real, MAXSHIPS + 1);	
  map1d(swtemp, real, MAXSHIPS + 1);	
  map1d(setemp, real, MAXSHIPS + 1);	
  map1d(swfuse, int, MAXSHIPS + 1);	
  map1d(sefuse, int, MAXSHIPS + 1);	
  map1d(sweapons, int, MAXSHIPS + 1);	
  map1d(sengines, int, MAXSHIPS + 1);	
  map1d(sarmies, int, MAXSHIPS + 1);	
  map1d(srmode, int, MAXSHIPS + 1);	
  map1d(scloaked, int, MAXSHIPS + 1);	
  map2d(soption, int, MAXSHIPS + 1, MAXOPTIONS);
  map2d(swar, int, MAXSHIPS + 1, NUMTEAMS);	
					
  map2d(srwar, int, MAXSHIPS + 1, NUMTEAMS);	
  map2d(ssrpwar, int, MAXSHIPS + 1, NUMPLANETS + 1); 
  map1d(ssdfuse, int, MAXSHIPS + 1);		
  map1d(slastmsg, int, MAXSHIPS + 1);		
  map1d(salastmsg, int, MAXSHIPS + 1);		
  map1d(smap, int, MAXSHIPS + 1);		
  map1d(stowing, int, MAXSHIPS + 1);	
  map1d(stowedby, int, MAXSHIPS + 1);	
  map1d(slastblast, real, MAXSHIPS + 1);	
  map1d(slastphase, real, MAXSHIPS + 1);	
  map1d(spfuse, int, MAXSHIPS + 1);	
  map2d(sscanned, int, MAXSHIPS + 1, NUMTEAMS); 
					
  map1d(stalert, int, MAXSHIPS + 1);		
  map1d(srobot, int, MAXSHIPS + 1);		
  map1d(saction, int, MAXSHIPS + 1);		
  map2d(spname, char, MAXSHIPS + 1, SIZEUSERPNAME);
					
  map1d(sctime, int, MAXSHIPS + 1);		
  map1d(setime, int, MAXSHIPS + 1);		
  map1d(scacc, int, MAXSHIPS + 1);		
  map1d(seacc, int, MAXSHIPS + 1);		

				/* This is used for robots to 'randomize'
				   their efficiency.  This value will be
				   set to 0 for normal palyers, but robots
				   will have it set to a random value which
				   will represent kills.  These 'phantom'
				   kills will be used in addition to the ship's
				   'real' kills in computing the efficiency
				   of the robot's weapons and engines, though
				   kept seperate in this array, so the
				   statistics don't get screwed up. */

  map1d(sstrkills, real, MAXSHIPS + 1);

  map2d(tstatus, int, MAXSHIPS + 1, MAXTORPS);	
  map2d(tfuse, int, MAXSHIPS + 1, MAXTORPS);	
  map2d(tx, real, MAXSHIPS + 1, MAXTORPS);	
  map2d(ty, real, MAXSHIPS + 1, MAXTORPS);
  map2d(tdx, real, MAXSHIPS + 1, MAXTORPS);	
  map2d(tdy, real, MAXSHIPS + 1, MAXTORPS);	
  map2d(tmult, real, MAXSHIPS + 1, MAXTORPS);
  map3d(twar, int, MAXSHIPS + 1, MAXTORPS, NUMTEAMS); 

  map2d(msgbuf, char, MAXMESSAGES, MESSAGE_SIZE);
  map1d(msgfrom, int, MAXMESSAGES);	
  map1d(msgto, int, MAXMESSAGES);	
  map1d(lastmsg, int, 1);		

  map1d(externrobots, int, 1);		

  map1d(glastmsg, int, 1);	

				/* now lets lock it */
#if defined(USE_COMMONMLOCK)
  lock_common();
#endif

  return;
}

