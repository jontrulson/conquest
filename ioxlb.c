
/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 07:01:52  jon
 * Initial revision
 *
 * Revision 1.7  1996/11/23  07:00:37  jon
 * - changes to support autoconf
 * - linux fixes...
 * - new debugable: DEBUG_IOGTIMED
 *
 * Revision 1.6  1996/07/02  20:29:37  jon
 * - code cleanup
 * - changes to support linux and the use of the select() syscall.
 *
 * Revision 1.5  1996/04/01  00:31:39  jon
 * - added ibuf processing to iogchar() and iogtimed() to allow macro
 *   capability.
 *
 * Revision 1.4  1996/03/16  22:10:30  jon
 * cerror() -> clog() changes.
 *
 * Revision 1.3  1995/01/27  04:37:40  jon
 * *() - another attempt at using the curses input routines. Changed
 * input routine return values to int's so can decode terminfo data on
 * keypad and arrow keys.  Changed team effectiveness values back to
 * their original settings.
 * ^ regardless of DOALT_UNIV definition.
 *
 * Revision 1.2  1995/01/09  00:46:44  jon
 * Added PVLOCK/UNLOCK capability, fixed some minor bugs, changed cloak
 * code so that a cloaked, warp 0 ship is invisible to alert scanners.
 * Increase torp fuse to 50 seconds, thereby increasing range.  Added
 * msync() to implement the upchuck() function.
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 * Revision 1.1  1995/01/01  08:32:54  jon
 * Initial revision
 *
 *
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

#include "c_defs.h"
#include "global.h"

/*                                I O N L B */
/* Version Date Initials Remarks */
/* ------- ---- -------- ------------------------------------------------------- */
/*   08c  17Sep84  cal   Modified to not include "tslb". Renamed include from */
/*                       "ionlb.c" to "ioncom". */

/*##  iogchar - get a character */
/*  SYNOPSIS */
/*    char ch, iogchar */
/*    ch = iogchar ( ch ) */

int iogchar ( int dummy )
{
  static unsigned int thechar;
  
  /* This is a good place to flush the output buffer and to */
  /*  check for terminal broadcasts. */
  
  cdrefresh(TRUE);
  
  timeout(-1);			/* wait for a while */
  
 reloop:
  
#ifdef ENABLE_MACROS
  if (iBufEmpty() == TRUE)
    thechar = wgetch(stdscr);
  else
    thechar = iBufGetCh();
#else
  thechar = wgetch(stdscr);
#endif
  
  if (thechar == ERR)
    {
#ifdef DEBUG_IO
      clog("iogchar() thechar == ERR, errno = %d", errno);
#endif
      goto reloop;
    }
  
  
  return(thechar);
}


/*##  iogtimed - get a char with timeout */
/*  SYNOPSIS */
/*    int *ch */
/*    int seconds */
/*    gotone = iogtimed ( &ch, seconds ) */

int iogtimed ( int *ch, int seconds )
{
  static int c;
#if defined(USE_SELECT)
  static struct timeval timeout;
  static struct fd_set readfds;
#endif
  int retval;
  static int starttime = 0, curtime;
  
  /* This is a good place to flush the output buffer. */
  
  cdrefresh(TRUE);
  
#ifdef ENABLE_MACROS
  if (iBufEmpty() == FALSE)
    {
      *ch = iBufGetCh();
      return(TRUE);
    }
#endif
  
#if !defined(USE_SELECT)
  timeout(seconds * 1000);	/* msecs - apparently useless
				   on Linux ncurses 1.9.9 */
#endif

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
  
  timeout.tv_sec = seconds;
  timeout.tv_usec = 0;
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
      
      timeout.tv_sec = seconds;
      timeout.tv_usec = 0;
#endif 
      
#if defined(USE_SELECT)

      if ((retval = select(PollInputfd + 1, &readfds, NULL, NULL, 
			   &timeout)) == -1)
	{			/* timeout or signal? */
	  if (errno != EINTR)	/* some error */
	    {
	      *ch = 0;
	      clog("iogtimed(): select() failed: %s", sys_errlist[errno]);
	      cdrefresh(FALSE);
	      return(FALSE);
	    }

          else
	    {
# if defined(DEBUG_IOGTIMED)
	      clog("iogtimed(): select(): interrupted: %s", 
		   sys_errlist[errno]);
# endif

# if !defined(LINUX)
				/* determine if we've waited long enough, if so
				 return 0, else try again */
	      curtime = time(0);

	      if (curtime >= (starttime + seconds))
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
	  cdrefresh(FALSE);
	  return(FALSE);
	}
      
#else /* ! USE_SELECT */
      
      if((c = wgetch(stdscr)) == ERR)	/* timeout or signal? */
	{
	  
	  if (errno != EINTR)	/* timeout */
	    {
	      *ch = 0;
	      
	      cdrefresh(FALSE);
	      return(FALSE);
	    }
	}
      else				/* char avail */
	{
	  *ch = c;
	  
	  return(TRUE);
	}
#endif 
      
      /* If we're here, the read was interupted, try again */
      
    } /* while */
  
}


