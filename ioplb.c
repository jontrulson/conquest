#include "c_defs.h"

/************************************************************************
 *
 * $Header$
 *
 *
 * $Log$
 * Revision 1.1  1996/11/23 07:00:19  jon
 * Initial revision
 *
 * Revision 1.5  1996/07/02  20:29:37  jon
 * - code cleanup
 * - changes to support linux and the use of the select() syscall.
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

/* ioplb - machine independent terminal I/O library */

/* ver  date   who remarks */
/* --- ------- --- ------------------------------------------------------ */
/* 08a 12May85 cal .Reformatted. */
/* 07B 08Aug84 cal .Modified iopstr() to use iopbuf(). */
/* 07A 16Jul84 cal .Renamed iomunch() to iofmtstr(), iogetq() to iogquick(), */
/*                  ioputs() to iopstr(), ioputx() to iopline(), and iogetz() */
/*                  to iogstr(). Removed iocant() and ioyn(). Rewrote ioerror() */
/*                  to use fprintf(). Added features to iogstr(). Modified to */
/*                  not include "ctrlsyms". Added NOIMPLICITs. Trimmed revision */
/*                  history. */
/* 06D 25Mar84 vp  .Added iogetz(). */
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
/*    This library contains the portable part of this io package. */


/*##  ioeat - swallow any input that has come so far */
/* synopsis */
/*    ioeat */
/* description */
/*    This routines flushes the type ahead buffer. That is, it reads any */
/*    characters that are available, but no more. */
void ioeat(void)
{
  
  int ch; 
  
  while ( iochav( 0 ) )
    ch = iogchar( ch );
  
  return;
  
}


/*##  iogquick - get a char if one is available */
/* synopsis */
/*    int gotone, iogquick */
/*    char ch */
/*    gotone = iogquick( ch ) */
/* description */
/*    If a char is available, it is read, placed in "ch" and TRUE */
/*    is returned, otherwise FALSE is returned. */
int iogquick( char ch )
{
  
  if ( iochav( 0 ) )
    {
      ch = iogchar( ch );
      return ( TRUE );
    }
  
  return ( FALSE );
  
}

