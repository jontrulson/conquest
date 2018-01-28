//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "c_defs.h"
#include "global.h"
#include "conqutil.h"
#include "ibuf.h"

#define NOEXTERN_IOLB
#include "iolb.h"
#undef NOEXTERN_IOLB

#include "cd2lb.h"

/* iochav - test whether a char is available to be read or not */
/* synopsis */
/*    int avail, iochav */
/*    avail = iochav() */
/* note */
/*    This routine does \not/ iopeek() because of speed considerations. */
/*    The two routines are meant to virtually be copies of one another and */
/*    should be modified in parallel. */
int iochav( void )
{
#if defined(USE_SELECT)
    /* we do a test for linux/sys_select_h
       for glibc systems... for some reason
       glibc defined fd_set as a typedef, rather
       than as a struct like the rest of the
       planet...*/
# if defined(LINUX) && defined(HAVE_SYS_SELECT_H)
    /* a linux, glibc system */
    static fd_set readfds;
# else
    static struct fd_set readfds;
# endif

    static struct timeval timeout;

#else  /* poll */
    static struct pollfd Stdin_pfd;
    static struct strbuf CtlMsg;
    static struct strbuf DataMsg;
    int flagsp;
#endif

    int retval;

    if (ibufCount())
    {
        return(true);
    }

#if defined(USE_SELECT)

    /* here, we'll just use select */
    FD_ZERO(&readfds);
    FD_SET(iolbStdinFD, &readfds);

    timeout.tv_sec = 0;		/* setting to zero should ret immediately */
    timeout.tv_usec = 0;

    if ((retval = select(iolbStdinFD+1, &readfds, NULL, NULL, &timeout)) == -1)
    {
        utLog("iochav(): select(): %s", strerror(errno));
        return(false);
    }

    if (retval == 0)		/* nothing there */
    {
        return(false);
    }
    else
    {				/* something avail */
        return(true);
    }

#else  /* use poll() */

    CtlMsg.maxlen = 0;
    CtlMsg.buf = NULL;
    DataMsg.maxlen = 0;
    DataMsg.buf = NULL;

    flagsp = 0;

    Stdin_pfd.fd = iolbStdinFD;		/* stdin */
    Stdin_pfd.events = (POLLIN | POLLRDNORM);

    if (poll(&Stdin_pfd, 1, 0) > 0) /* return immediately if a char avail */
    {
#ifdef DEBUG_IO
        utLog("ALTiochav(): had a char via poll");
#endif

        if ((retval = getmsg(iolbStdinFD, &CtlMsg, &DataMsg,
                             &flagsp)) == -1)
	{
            utLog("iochav(): getmsg(): failed: %s", strerror(errno));
	}
        else
	{
#ifdef DEBUG_IO
            utLog("iochav(): getmsg() = %d: DataMsg.len = %d CtlMsg.len = %d",
                  retval,
                  DataMsg.len,
                  CtlMsg.len);
#endif

            if (DataMsg.len <= 0 && retval > 0)
	    {
#ifdef DEBUG_IO
                utLog("iochav(): getmsg(): DataMsg.len <= 0 retval = %d - true",
                      DataMsg.len,
                      retval);
#endif

                return(true);
	    }

            if (DataMsg.len <= 0 && CtlMsg.len <= 0)
	    {
#ifdef DEBUG_IO
                utLog("iochav(): getmsg(): DataMsg.len & CtlMsg.len <= 0 ret false");
#endif
                return(false);
	    }
            else
	    {
#ifdef DEBUG_IO
                utLog("iochav(): getmsg(): DataMsg.len | CtlMsg.len > 0 ret true");
#endif
                return(true);
	    }
	}
    }
    else
    {
#ifdef DEBUG_IO
        utLog("ALTiochav(): NO char via poll");
#endif
        return(false);
    }

#endif /* !USE_SELECT */

}

/*  ioeat - swallow any input that has come so far */
/* synopsis */
/*    ioeat */
/* description */
/*    This routines flushes the type ahead buffer. That is, it reads any */
/*    characters that are available, but no more. */
void ioeat(void)
{
    while ( iochav() )
        iogchar();

    return;

}

/*  iogchar - get a character */
/*  SYNOPSIS */
/*    char ch, iogchar */
/*    ch = iogchar ( ch ) */

int iogchar ( void )
{
    static unsigned int thechar = 0;

    /* This is a good place to flush the output buffer and to */
    /*  check for terminal broadcasts. */

    cdrefresh();

    timeout(-1);			/* wait for a while */

reloop:

    if (!ibufCount())
    {
        utSleep(0.1);
        thechar = wgetch(stdscr);
    }
    else
        thechar = ibufGetc();

    if (thechar == -1)
    {
#ifdef DEBUG_IO
        utLog("iogchar() thechar == ERR, errno = %d", errno);
#endif
        goto reloop;
    }

    return(thechar);
}


/*  iogtimed - get a char with timeout */
/*  SYNOPSIS */
/*    int *ch */
/*    int seconds */
/*    gotone = iogtimed ( &ch, seconds ) */

/*  iogtimed - get a char with timeout */
/*  SYNOPSIS */
/*    int *ch */
/*    int seconds */
/*    gotone = iogtimed ( &ch, seconds ) */

int iogtimed ( int *ch, real seconds )
{
    static int c;
    unsigned int secs, usecs;

#if !defined(LINUX)
    time_t starttime, curtime;
#endif

#if defined(USE_SELECT)
    static struct timeval timeout;

# if defined(LINUX) && defined(HAVE_SYS_SELECT_H)
    /* a linux, glibc system */
    static fd_set readfds;
# else
    static struct fd_set readfds;
# endif

#endif
    int retval;

    /* This is a good place to flush the output buffer. */

    cdrefresh();

    if (ibufCount())
    {
        *ch = ibufGetc();
        return(true);
    }


    /* compute proper secs/usecs to allow for subsecond sleeps */
    /* some systems don't grok usecs >1,000,000 */
    if (seconds >= 1.0)
    {
        secs = (int) seconds;
        usecs = (int) (1000000 * (seconds - (real) secs));
    }
    else
    {
        secs = 0;
        usecs = (1000000 * seconds);
    }


#if defined(LINUX) && defined(USE_SELECT)
    /* with linux, select returns the time
       remaining if the select is interrupted,
       this means we can set the timer once
       and reuse it, so that the select will
       sleep for whatever time is left if it's
       interrupted.  Don't count on this
       behavior with other unix's... */
    FD_ZERO(&readfds);
    FD_SET(iolbStdinFD, &readfds);

    timeout.tv_sec = secs;
    timeout.tv_usec = usecs;
#endif

#if !defined(LINUX)
    /* Linux will return the amount of time left in a
       select() call if it is interupted, others won't,
       so we need to set a timer to make sure we leave
       when 'seconds' are up */

    starttime = time(0);
#endif

    while (true)
    {
        errno = 0;			/* reset errno */

#if defined(USE_SELECT) && !defined(LINUX)
        /* if we're using select, but not on linux,
           then we have to restart the whole timer
           again if it's interrupted... don't know
           how to get around that... */

        FD_ZERO(&readfds);
        FD_SET(iolbStdinFD, &readfds);

        timeout.tv_sec = secs;
        timeout.tv_usec = usecs;
#endif

#if defined(USE_SELECT)

        if ((retval = select(iolbStdinFD + 1, &readfds, NULL, NULL,
                             &timeout)) == -1)
	{			/* timeout or signal? */
            if (errno != EINTR)	/* some error */
	    {
                *ch = 0;
                utLog("iogtimed(): select() failed: %s", strerror(errno));
                cdrefresh();
                return(false);
	    }

            else
	    {
# if defined(DEBUG_IOGTIMED)
                utLog("iogtimed(): select(): interrupted: %s",
                      strerror(errno));
# endif

# if !defined(LINUX)
                /* determine if we've waited long enough, if so
                   return 0, else try again */
                curtime = time(0);

                if (curtime >= (starttime + secs))
		{
                    *ch = 0;
                    return(false);
		}
                else
		{
                    continue;
		}
# endif
	    }
	}
        else if (retval > 0)
	{
# if defined(DEBUG_IOGTIMED)
            if (FD_ISSET(iolbStdinFD,&readfds))
                utLog("After Linux select - FD_ISSET is SET");
            else
                utLog("After Linux select - FD_ISSET is NOT SET");
            utLog("iogtimed(): After Linux select - errno   = %d", errno);
            utLog("iogtimed(): After Linux select - tv_sec  = %d",
                  timeout.tv_sec);
            utLog("iogtimed(): After Linux select - tv_usec = %d",
                  timeout.tv_usec);
# endif

            c = wgetch(stdscr);   /* I hope select wasn't lying... */
            *ch = c;

# if defined(DEBUG_IOGTIMED)
            utLog("iogtimed(): retval = %d, iolbStdinFD = %d, *ch = %d",
                  retval,iolbStdinFD,*ch);
# endif

            return(true);
	}
        else
	{
            *ch = 0;
# if defined(DEBUG_IOGTIMED)
            utLog("iogtimed(): retval = %d, iolbStdinFD = %d, *ch = %d",
                  retval,iolbStdinFD,*ch);
# endif
            cdrefresh();
            return(false);
	}

#else
#error "You need the select() system call"
#endif

        /* If we're here, the read was interupted, try again */

    } /* while */

}
