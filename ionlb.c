
/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 06:59:59  jon
 * Initial revision
 *
 * Revision 1.7  1996/07/02  20:27:59  jon
 * - code cleanup
 * - changes to support linux and the use of the select() syscall.
 *
 * Revision 1.6  1996/04/01  00:29:59  jon
 * - got rid of ALT_IOCHAV
 * - rewrote iochave() using poll() and getmsg().  Seems to work...
 * - added ibuf processing to iochav().
 *
 * Revision 1.5  1996/03/16  22:10:09  jon
 * cerror() -> clog() changes.
 *
 * Revision 1.4  1995/02/08  05:03:33  jon
 * global change of all _And_'s to &&'s and _Or_ to ||'s.  other minor
 * 'corrections' and cleanups.
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

/* ionlb - VMS/VMS machine dependent terminal I/O library */

/* ver  date   who remarks */
/* --- ------- --- ------------------------------------------------------ */
/* 09a 04Mar85 cal .Upgraded to VMS V4. */
/*                  Reformatted. */
/*                  Modified to use pasthru. */
/*                  Improved terminal/mailbox broadcast code. */
/*                  Added iovmserror(). */
/*                  Added exit handler for ioend() */
/* 08d 14Jun85 cal .Fixed data overrun bug in iogtimed(). */
/* 08c 17Sep84 cal .Modified to not include "tslb". Renamed include from */
/*                  "ionlb.c" to "ioncom". */
/* 08b 08Aug84 cal .Added iopbuf(). */
/* 08a 16Jul84 cal .Renamed ioget() to iogchar(), iotget() to iogtimed(), and */
/*                  ioput() to iopchar(). Removed iomxname() and made code */
/*                  inline to ioinit(). Removed iobuffered(). Moved iofmtstr() */
/*                  to ioplb. Turned iogtimed() into a int and function */
/*                  added a time limit argument. Trimmed revision history. */
/* 07i 22Jun84 cal */
/*              & */
/*             vp  .Added iopeek(). */
/* 07h 22May84 cal .Modified iogbroadcast() to strip NULLs from the received */
/*                  terminal broadcast message. */
/* description */
/*    Iolb is designed to provide raw-mode I/O to and from the user's */
/*    terminal. Raw-mode I/O is the simplest thing possible: when the user */
/*    types a character, it is read by the program; when the program writes */
/*    a character, it is sent to the terminal. No line-by-line buffering, */
/*    no converting tabs to spaces, spaces to tabs, lower case to upper and */
/*    upper to lower, no ignoring some characters and intercepting others... */
/*    nothing but simple read-a-character, write-a-character. Is that asking */
/*    too much? */
/*    Apparently. On many computer systems in use today, it is IMPOSSIBLE */
/*    to implement raw I/O. On most systems, it is very difficult, it usually */
/*    requires some assembly language hacking, and is usually inefficient. */
/*    This annoying situation is not really anybody's fault. When today's */
/*    systems were being designed, the phrases "real-time", "screen oriented" */
/*    and "interactive" did not yet have any referents. We can only hope that */
/*    the systems unable to be adapted will die out from lack of use. */
/*    The low-level routines in this package (ioinit(), ioend(), iochav(), */
/*    iogchar(), and iopchar()) are system-dependent of course, but the rest */
/*    should be quite portable. */
/*    This library contains the non-portable low level part of this package. */



/*## iochav - test whether a char is available to be read or not */
/* synopsis */
/*    int avail, iochav */
/*    avail = iochav( 0 ) */
/* note */
/*    This routine does \not/ iopeek() because of speed considerations. */
/*    The two routines are meant to virtually be copies of one another and */
/*    should be modified in parallel. */
int iochav( int dummy )
{
#if defined(USE_SELECT)
  static struct timeval timeout;
  static struct fd_set readfds;
#else  /* poll */
  static struct pollfd Stdin_pfd;
  static struct strbuf CtlMsg;
  static struct strbuf DataMsg;
  int flagsp;
#endif
  
  int retval;
  
#ifdef ENABLE_MACROS
  if (iBufEmpty() == FALSE)
    {
      return(TRUE);
    }
#endif
  
#if defined(USE_SELECT)	
  
  /* here, we'll just use select */
  FD_ZERO(&readfds);
  FD_SET(PollInputfd, &readfds);
  
  timeout.tv_sec = 0;		/* setting to zero should ret immediately */
  timeout.tv_usec = 0;
  
  if ((retval = select(PollInputfd+1, &readfds, NULL, NULL, &timeout)) == -1)
    {
      clog("iochav(): select(): %s", sys_errlist[errno]);
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
	  clog("iochav(): getmsg(): failed: %s", sys_errlist[errno]);
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

