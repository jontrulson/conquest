#include "c_defs.h"
/************************************************************************
 *
 * I/O library routines
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#include "global.h"

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
  
  if (iBufEmpty() == FALSE)
    {
      return(TRUE);
    }
  
#if defined(USE_SELECT)	
  
  /* here, we'll just use select */
  FD_ZERO(&readfds);
  FD_SET(PollInputfd, &readfds);
  
  timeout.tv_sec = 0;		/* setting to zero should ret immediately */
  timeout.tv_usec = 0;
  
  if ((retval = select(PollInputfd+1, &readfds, NULL, NULL, &timeout)) == -1)
    {
      clog("iochav(): select(): %s", strerror(errno));
      return(FALSE);
    }
  
  if (retval == 0)		/* nothing there */
    {
      return(FALSE);
    }
  else
    {				/* something avail */
      return(TRUE);
    }
  
#else  /* use poll() */
  
  CtlMsg.maxlen = 0;
  CtlMsg.buf = NULL;
  DataMsg.maxlen = 0;
  DataMsg.buf = NULL;
  
  flagsp = 0;
  
  Stdin_pfd.fd = PollInputfd;		/* stdin */
  Stdin_pfd.events = (POLLIN | POLLRDNORM);
  
  if (poll(&Stdin_pfd, 1, 0) > 0) /* return immediately if a char avail */
    {
#ifdef DEBUG_IO
      clog("ALTiochav(): had a char via poll");
#endif
      
      if ((retval = getmsg(PollInputfd, &CtlMsg, &DataMsg, 
			   &flagsp)) == -1)
	{
	  clog("iochav(): getmsg(): failed: %s", strerror(errno));
	}
      else
	{
#ifdef DEBUG_IO
	  clog("iochav(): getmsg() = %d: DataMsg.len = %d CtlMsg.len = %d",
	       retval,
	       DataMsg.len,
	       CtlMsg.len);
#endif
	  
	  if (DataMsg.len <= 0 && retval > 0)
	    {
#ifdef DEBUG_IO
              clog("iochav(): getmsg(): DataMsg.len <= 0 retval = %d - TRUE",
		   DataMsg.len,
		   retval);
#endif
	      
	      return(TRUE);
	    }
	  
	  if (DataMsg.len <= 0 && CtlMsg.len <= 0)
	    {
#ifdef DEBUG_IO
	      clog("iochav(): getmsg(): DataMsg.len & CtlMsg.len <= 0 ret FALSE");
#endif
	      return(FALSE);
	    }
	  else
	    {
#ifdef DEBUG_IO
	      clog("iochav(): getmsg(): DataMsg.len | CtlMsg.len > 0 ret TRUE");
#endif
	      return(TRUE);
	    }
	}
    }
  else 
    {
#ifdef DEBUG_IO
      clog("ALTiochav(): NO char via poll");
#endif
      return(FALSE);
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
  static unsigned int thechar;
  
  /* This is a good place to flush the output buffer and to */
  /*  check for terminal broadcasts. */
  
  cdrefresh();
  
  timeout(-1);			/* wait for a while */
  
 reloop:
  
  if (iBufEmpty() == TRUE)
    thechar = wgetch(stdscr);
  else
    thechar = iBufGetCh();
  
  if (thechar == ERR)
    {
#ifdef DEBUG_IO
      clog("iogchar() thechar == ERR, errno = %d", errno);
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

int iogtimed ( int *ch, real seconds )
{
  static int c;
  unsigned int secs, usecs;

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
  static int starttime = 0, curtime;
  
  /* This is a good place to flush the output buffer. */
  
  cdrefresh();
  
  if (iBufEmpty() == FALSE)
    {
      *ch = iBufGetCh();
      return(TRUE);
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
  FD_SET(PollInputfd, &readfds);
  
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
  
  while (TRUE)
    {
      errno = 0;			/* reset errno */
      
#if defined(USE_SELECT) && !defined(LINUX)
      /* if we're using select, but not on linux,
	 then we have to restart the whole timer
	 again if it's interrupted... don't know
	 how to get around that... */
      
      FD_ZERO(&readfds);
      FD_SET(PollInputfd, &readfds);
      
      timeout.tv_sec = secs;
      timeout.tv_usec = usecs;
#endif 
      
#if defined(USE_SELECT)

      if ((retval = select(PollInputfd + 1, &readfds, NULL, NULL, 
			   &timeout)) == -1)
	{			/* timeout or signal? */
	  if (errno != EINTR)	/* some error */
	    {
	      *ch = 0;
	      clog("iogtimed(): select() failed: %s", strerror(errno));
	      cdrefresh();
	      return(FALSE);
	    }

          else
	    {
# if defined(DEBUG_IOGTIMED)
	      clog("iogtimed(): select(): interrupted: %s", 
		   strerror(errno));
# endif

# if !defined(LINUX)
				/* determine if we've waited long enough, if so
				 return 0, else try again */
	      curtime = time(0);

	      if (curtime >= (starttime + secs))
		{
		  *ch = 0;
		  return(FALSE);
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
          if (FD_ISSET(PollInputfd,&readfds))
	    clog("After Linux select - FD_ISSET is SET");
          else
	    clog("After Linux select - FD_ISSET is NOT SET");
          clog("iogtimed(): After Linux select - errno   = %d", errno);
          clog("iogtimed(): After Linux select - tv_sec  = %d", 
	       timeout.tv_sec);
          clog("iogtimed(): After Linux select - tv_usec = %d", 
	       timeout.tv_usec);
# endif
	  
	  c = wgetch(stdscr);   /* I hope select wasn't lying... */
	  *ch = c;
	  
# if defined(DEBUG_IOGTIMED)
	  clog("iogtimed(): retval = %d, PollInputfd = %d, *ch = %d",
	       retval,PollInputfd,*ch);
# endif
	  
	  return(TRUE);
	}
      else
	{
	  *ch = 0;
# if defined(DEBUG_IOGTIMED)
	  clog("iogtimed(): retval = %d, PollInputfd = %d, *ch = %d",
	       retval,PollInputfd,*ch);
# endif
	  cdrefresh();
	  return(FALSE);
	}
      
#else
#error "You need the select() system call"
#endif
      
      /* If we're here, the read was interupted, try again */
      
    } /* while */
  
}




