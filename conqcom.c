#include "c_defs.h"

/************************************************************************
 * Conquest Common Block (aka: The Universe)
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q C M */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */


#define NOEXTERN_CONQCOM
#include "conqcom.h"		/* common block vars defined here */

#include "global.h"
#include "conqdef.h"
#include "conqlb.h"
#include "conqutil.h"
#include "conqunix.h"

#if !defined(MINGW)
# include <sys/mman.h>
#endif

#include "context.h"		/* some extra stuff */
#include "user.h"
#include "sem.h"

static char *cBasePtr = NULL;	/* common block ptr */
static unsigned int coff = 0;   /* offset into common block */

static int fakeCommon = FALSE;	/* for the clients */

/* Some (most) architectures do not like unaligned accesses (like
 *  sparc) so we need to ensure proper alignment of the structures
 *  contained within the common block.  We will use 16-byte alignment.
 */

#define CB_ALIGNMENT            (16)
#define CB_ALIGN(_off, _align)  ( ((_off) + (_align)) & ~((_align) - 1) )

/* On those platforms where we want proper alignment, align everything
 * to 16 bytes.
 */
#define map1d(thevarp, thetype, size) {                 \
        thevarp = (thetype *) (cBasePtr + coff);        \
        coff += (sizeof(thetype) * (size));             \
        coff = CB_ALIGN(coff, CB_ALIGNMENT);            \
    }


static void map_vars(void);


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

/* flush_common() - flush a common block */
void flush_common(void)
{
    if (fakeCommon)
        return;

#if !defined(MINGW)
    /* fbsd doesn't like MS_SYNC       */
    /* which is prefered, but oh well */
# if defined(FREEBSD)
    if (msync((caddr_t)cBasePtr, SIZEOF_COMMONBLOCK, 0) == -1)
# else
        if (msync((caddr_t)cBasePtr, SIZEOF_COMMONBLOCK, MS_SYNC) == -1)
# endif
            utLog("flush_common(): msync(): %s", strerror(errno));
#endif

    return;
}

/* check_cblock() - open/verify a common block - init if necc, return TRUE
   if successful */
int check_cblock(char *fname, int fmode, int sizeofcb)
{
    int ffd;
    struct stat sbuf;

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
                   (unsigned int)sbuf.st_size);
            if (unlink(fname) == -1)
	    {
                printf("check_cblock(): unlink(%s) failed: %s\n",
                       fname,
                       strerror(errno));
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
                       strerror(errno));
                return(FALSE);
	    }
            else
	    {			/* Create it */
                printf("Initializing common block: %s\n", fname);
                cBasePtr = (char *) mymalloc(sizeofcb); /* this exits if malloc fails */
                memset(cBasePtr, 0, sizeofcb);

                if (write(ffd, cBasePtr, sizeofcb) <= 0)
                {
                    printf("check_cblock(): write() failed: %s\n",
                           strerror(errno));

                    close(ffd);
                    free(cBasePtr);
                    cBasePtr = NULL;
                    return FALSE;
                }

                close(ffd);
                free(cBasePtr);
                cBasePtr = NULL;
	    }
	}
        else
	{			/* some other error */
            printf("check_cblock(): open(%s, O_RDONLY) failed: %s\n",
                   fname,
                   strerror(errno));
            return(FALSE);
	}
    }

    close(ffd);			/* everything ok.. */

#if !defined(MINGW)
    /* set ownership */
    if (chown(fname, 0, -1) == -1)
    {
        printf("check_cblock(): chown() failed: %s\n",
               strerror(errno));

        //      return(FALSE);
    }
#endif

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

void map_common(void)
{
#if !defined(MINGW)
    int cmn_fd;
    static char cmnfile[PATH_MAX] = {};
#endif

    if (fakeCommon)
        return;

#if defined(MINGW)
    fprintf(stderr, "%s: Only fake common blocks are supported under MINGW\n");
    exit(1);
#else  /* MINGW */
    snprintf(cmnfile, PATH_MAX, "%s/%s", CONQSTATE, C_CONQ_COMMONBLK);

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

# ifndef MAP_FILE
#  define MAP_FILE 0		/* some arch's don't def this */
# endif

    if ((cBasePtr = mmap((caddr_t) 0, (size_t) SIZEOF_COMMONBLOCK,
                         PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE,
                         cmn_fd, 0)) == MAP_FAILED)
    {
        perror("map_common():mmap()");
        exit(1);
    }
#endif  /* MINGW */

    /* now map the variables into the
       common block */
    map_vars();

    return;
}

void zero_common(void)
{				/* zero the common block, called from
				   init everything */
    memset(cBasePtr, 0, SIZEOF_COMMONBLOCK);
    upchuck();			/* flush the commonblock */

    return;
}

/* maps the actual vars into the common block */
static void map_vars(void)
{
    coff = 0;

    map1d(CBlockRevision, int, 1);	/* this *must* be the first var */

    map1d(CBGlobal, CBGlobal_t, 1);

    map1d(ConqInfo, ConqInfo_t, 1);

    map1d(Users, User_t, MAXUSERS);

    map1d(Robot, Robot_t, 1);

    map1d(Planets, Planet_t, MAXPLANETS);

    map1d(Teams, Team_t, NUMALLTEAMS);

    map1d(Doomsday, Doomsday_t, 1);

    map1d(History, History_t, MAXHISTLOG);

    map1d(Driver, Driver_t, 1);

    map1d(Ships, Ship_t, MAXSHIPS);

    map1d(ShipTypes, ShipType_t, MAXNUMSHIPTYPES);

    map1d(Msgs, Msg_t, MAXMESSAGES);

    map1d(EndOfCBlock, int, 1);

    return;
}

void fake_common(void)
{
    fakeCommon = TRUE;

    /* this will exit if it fails */
    if (!cBasePtr)
        cBasePtr = mymalloc(SIZEOF_COMMONBLOCK);

    map_vars();

    zero_common();
    return;
}

/* short cut */
void map_lcommon(void)
{
    /* a parallel universe, it is */
    fake_common();
    clbInitEverything();
    clbInitMsgs();
    *CBlockRevision = COMMONSTAMP;
    ConqInfo->closed = FALSE;
    Driver->drivstat = DRS_OFF;
    Driver->drivpid = 0;
    Driver->drivowner[0] = 0;

    return;
}
