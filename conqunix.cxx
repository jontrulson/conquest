#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

/*                               C O N Q V M S */
/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "conqlb.h"
#include "conqutil.h"
#include "record.h"

#include "server.h"

#include "conqunix.h"

/* int getUID(void) - return a User ID */
int getUID(char *name)
{
#if defined(MINGW)
    return 0;
#else

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

        return(-1);
    }

    return(conq_pwd->pw_uid);
#endif  /* MINGW */
}


/* int getConquestGID(void) - return conquest's Group ID */
int getConquestGID(void)
{
#if defined(MINGW)
    return 0;
#else
    struct group *conq_grp;

    if ((conq_grp = getgrnam(CONQUEST_GROUP)) == NULL)
    {
        fprintf(stderr, "conqsvr42: getConquestGID(%s): can't get group: %s",
                CONQUEST_GROUP,
                strerror(errno));

        return(-1);
    }

    return(conq_grp->gr_gid);
#endif
}


/*  conqinit - machine dependent initialization */
/*  SYNOPSIS */
/*    conqinit */
void conqinit(void)
{
    /* Set up game context. */

    /* Other house keeping. */
    Context.pid = getpid();
    Context.hasnewsfile = ( strcmp( C_CONQ_NEWSFILE, "" ) != 0 );

    /* Zero process id of our child (since we don't have one yet). */
    Context.childpid = 0;

    /* Zero last time drcheck() was called. */
    Context.drchklastime = 0;

    /* Haven't scanned anything yet. */
    Context.lastinfostr[0] = 0;

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

    upstats( &cbShips[snum].ctime, &cbShips[snum].etime,
             &cbShips[snum].cacc, &cbShips[snum].eacc,
             &cadd, &eadd );

    /* Add in the new amounts. */
    cbLock(&cbConqInfo->lockword);
    if ( cbShips[snum].pid != 0 )
    {
        /* Update stats for a humanoid ship. */
        unum = cbShips[snum].unum;

        cbUsers[unum].stats[USTAT_CPUSECONDS] += cadd;
        cbUsers[unum].stats[USTAT_SECONDS] += eadd;

        /* update elapsed time in cbHistory[]
           for this user */

        if (Context.histslot != -1 && cbHistory[Context.histslot].unum == unum)
	{
            difftime = time(0) - cbHistory[Context.histslot].enterTime;
            if (difftime < (time_t)0)
                difftime = (time_t)0;
            cbHistory[Context.histslot].elapsed = difftime;
	}

        team = cbUsers[unum].team;
        cbTeams[team].stats[TSTAT_CPUSECONDS] += cadd;
        cbTeams[team].stats[TSTAT_SECONDS] += eadd;

        cbConqInfo->ccpuseconds += cadd;
        cbConqInfo->celapsedseconds += eadd;


    }
    cbUnlock(&cbConqInfo->lockword);

    return;

}


/*  drcheck - make sure the driver is still around (DOES LOCKING) */
/*  SYNOPSIS */
/*    drcheck */
void drcheck(void)
{
#if defined(MINGW)
    return;
#else
    int ppid;

    /* If we haven't been getting cpu time in recent history, do no-thing. */
    if ( utDeltaSecs( Context.drchklastime, &Context.drchklastime ) > TIMEOUT_DRCHECK )
        return;

    if ( utDeltaSecs( cbDriver->drivtime, &(cbDriver->playtime) ) > TIMEOUT_DRIVER )
    {
        if ( Context.childpid != 0 )
	{
            /* We own the driver. See if it's still there. */
            ppid = Context.childpid;
            if ( kill(Context.childpid, 0) != -1 )
	    {
                /* He's still alive and belongs to us. */
                utGetSecs( &(cbDriver->drivtime) );
                return;
	    }
            else
                utLog( "drcheck(): Wrong ppid %d.", ppid );

            /* If we got here, something was wrong; disown the child. */
            Context.childpid = 0;
	}

        cbLock(&cbConqInfo->lockword);
        if ( utDeltaSecs( cbDriver->drivtime, &(cbDriver->playtime) ) > TIMEOUT_DRIVER )
	{
            drcreate();
            cbDriver->drivcnt = utModPlusOne( cbDriver->drivcnt + 1, 1000 );
            utLog( "Driver timeout #%d.", cbDriver->drivcnt );
	}
        cbUnlock(&cbConqInfo->lockword);
    }
    drstart();

    return;
#endif  /* MINGW */
}


/*  drcreate - create a new driver process */
/*  SYNOPSIS */
/*    drcreate */
void drcreate(void)
{
#if defined(MINGW)
    return;
#else
    int pid;
    char drivcmd[BUFFER_SIZE_256];


    utGetSecs( &(cbDriver->drivtime) );			/* prevent driver timeout */
    cbDriver->drivpid = 0;			/* zero current driver pid */
    cbDriver->drivstat = DRS_RESTART;		/* driver state to restart */

    /* fork the child - mmap()'s should remain */
    /*  intact */
    if ((pid = fork()) == -1)
    {				/* error */
        cbDriver->drivstat = DRS_OFF;
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
#endif
}


/*  drkill - make the driver go away if we started it (DOES LOCKING) */
/*  SYNOPSIS */
/*    drkill */
void drkill(void)
{
#if !defined(MINGW)
    if ( Context.childpid != 0 )
        if ( Context.childpid == cbDriver->drivpid && cbDriver->drivstat == DRS_RUNNING )
        {
            cbLock(&cbConqInfo->lockword);
            if ( Context.childpid == cbDriver->drivpid && cbDriver->drivstat == DRS_RUNNING )
                cbDriver->drivstat = DRS_KAMIKAZE;
            cbUnlock(&cbConqInfo->lockword);
        }
#endif  /* MINGW */
    return;

}


/*  drpexit - make the driver go away if we started it */
/*  SYNOPSIS */
/*    drpexit */
void drpexit(void)
{
#if defined(MINGW)
    return;
#else
    int i;

    if ( Context.childpid != 0 )
    {
        /* We may well have started the driver. */
        drkill();
        for ( i = 1; Context.childpid == cbDriver->drivpid && i <= 50; i = i + 1 )
            utSleep( 0.1 );
        if ( Context.childpid == cbDriver->drivpid )
            utLog("drpexit(): Driver didn't exit; pid = %08x", Context.childpid );
    }

    return;
#endif  /* MINGW */
}


/*  drstart - Start a new driver if necessary (DOES LOCKING) */
/*  SYNOPSIS */
/*    drstart */
void drstart(void)
{
#if !defined(MINGW)
    if ( cbDriver->drivstat == DRS_OFF )
    {
        cbLock(&cbConqInfo->lockword);
        if ( cbDriver->drivstat == DRS_OFF )
            drcreate();
        cbUnlock(&cbConqInfo->lockword);
    }
#endif  /* MINGW */
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
#if defined(MINGW)
    *cpu = 0;
    return;
#else

    static struct tms Ptimes;

# ifndef CLK_TCK
#  ifdef LINUX
    extern long int __sysconf (int);
#   define CLK_TCK (sysconf (_SC_CLK_TCK))
#  else
#   define CLK_TCK CLOCKS_PER_SEC
#  endif
# endif

    /* JET - I think this function has outlived it's usefulness. */

    times(&Ptimes);

    *cpu = round( ((real)(Ptimes.tms_stime + Ptimes.tms_utime) /
                   (real)CLK_TCK) *
                  100.0);

    /* utLog("gcputime() - *cpu = %d", *cpu); */

#endif  /* MINGW */
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

/* For cygwin, everybody is a god for non-user num checks. */
#if defined(CYGWIN) || defined(MINGW)
int isagod( int unum )
{
    if (unum == -1)               /* get god status for current user */
    {
        return true;
    }
    else
    {				/* else a user number passed in */
        if (UISOPER(unum))
            return true;
        else
            return false;
    }
}
#else /* !CGYWIN */
int isagod( int unum )
{
    static struct group *grp = NULL;
    static int god = false;
    static char myname[BUFFER_SIZE_256];
    int i;

    god = false;

    if (unum == -1)		/* get god status for current user */
    {
        utStrncpy(myname, clbGetUserLogname(), BUFFER_SIZE_256);
    }
    else
    {				/* else a user number passed in */
				/* just check for OOPT_OPER */
        if (UISOPER(unum))
            return true;
        else
            return false;
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

            god = false;
            return(false);
	}
    }

    /* root is always god */
    if (strcmp(myname, "root") == 0)
        god = true;

    i = 0;

    if (grp->gr_mem != NULL)
    {
        while (grp->gr_mem[i] != NULL)
	{
            if (strcmp(myname, grp->gr_mem[i]) == 0)
	    {		/* a match */
                god = true;
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

    cbLock(&cbConqInfo->lockword);

    utFormatTime( cbConqInfo->lastupchuck, 0 );
    cbFlush();

    cbUnlock(&cbConqInfo->lockword);

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
#if defined(MINGW)
    return false;
#else
    int rv;

    if (pidnum == 0)
        return(false);		/* can re-incarnate to robots */
    rv = kill(pidnum, 0);

    if (rv == -1)
    {
        switch (errno)
	{
	case ESRCH:
            return(false);
            break;
	default:
            return(true);
            break;
	}
    }
    else
        return(true);
#endif  /* MINGW */
}
