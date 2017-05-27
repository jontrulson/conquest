#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*                              C D 2 L B */
/*                       Craig's Display Library */
/* Modification History */
/* ------------ ------- */
/* 03m 11Sep84 cal .Renamed include files "cdlb" and "cdlb.c" to "cddef" and */
/*                  "cdcom". Removed some unused variables. */
/* 03l 16Jul84 cal .Modified to use new iolb routine names. */
/* 03k 08Jun84 cal .Changed cdend() to position the cursor. */
/* 03j 20Apr84 cal .Added requirements of minimum terminal capabilities to */
/*                  cdinit(). */
/* 03i 11Mar84 cal .Fixed bug in cdgetp() that precluded clear to end of line */
/*                  from being used. */
/* 03h 22Feb84 cal .Added cdgetp() which gets a string with a default and */
/*                  modified cdgetx() to use it. */
/* 03g 14Dec83 cal .Modified cdclra() to accept points in any order. Removed */
/*                  definition of NOIMPLICIT. Removed several unused variables. */
/* 03f 23Sep83 cal .Added a portable version of cdfill() and cdmovtc() and */
/*                  removed fast macro conditional code. Changed cdinit() to */
/*                  not cdmaketc(). Added cdbox(). Changed to use */
/*                  scdisp(). Modified cdgetx() to accept carriage return as */
/*                  a valid terminator if no terminators are specified. */
/*                  Speeded up loop in cdrefresh(). Modified cdline() to accept */
/*                  points in any order. */
/* 03e 10Sep83 cal .Modified cdrefresh() to keep track of the last line it */
/*                  updated. Renamed fillch(), movtchs(), and maktbl() */
/*                  to cdfill(), cdmovtc(), and cdmaketc(). Removed cdget(), */
/*                  cdgetk(), and cdpeek() since they duplicate scget(), */
/*                  scgetk(), scpeek(). Removed cdlredo() since cdlb is not */
/*                  for hardcopy terminals. Reformatted routine headers. */
/* 03d 05Sep83 cal .Created cdsetfont(), which contains some code moved */
/*                  out from cdcput(). Modified all routines that deal with */
/*                  fonts to always treat them as characters. */
/* 03c 14Jul83 cal .Ratfix. */
/* 03b 10Jul83 cal .Modified to not use cursor right, but rather put */
/*                  out the char that is really on the screen. */
/* 03a 29May83 cal .Rewritten to go like hell */

#include "conqdef.h"

#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"

#define NOEXTERN_CD2LB
#include "cd2lb.h"
#undef NOEXTERN_CD2LB

#include "cumisc.h"
#include "iolb.h"
#include "ui.h"
#include "ibuf.h"

#define MSGMAXLINE 90 			/* used for screen formatting */

/* Maximum screen dimensions. */
static int maxlin;                              /* maximum line */
static int maxcol;                              /* maximum column */



/* cdbox - draw a box */
/* synopsis */
/*    int lin1, col1, lin2, col2 */
/*    cdbox ( lin1, col1, lin2, col2 ) */
/* description */
/*    This routine draws a box defined by the two points. */
void cdbox ( int lin1, int col1, int lin2, int col2 )
{

  cdline ( lin1, col1, lin1, col2 );	/* right */
  cdline ( lin1, col2, lin2, col2 );	/* down */
  cdline ( lin2, col2, lin2, col1 );	/* left */
  cdline ( lin1, col1, lin2, col1 );	/* up */

  /* Now the corners */

  cdmove(lin1, col1);	/* Upper left corner */
  addch(ACS_ULCORNER);
  cdmove(lin1, col2); /* upper right corner */
  addch(ACS_URCORNER);
  cdmove(lin2, col1); /* lower left corner */
  addch(ACS_LLCORNER);
  cdmove(lin2, col2); /* lower right corner */
  addch(ACS_LRCORNER);

  return;

}


/* cdcput - output a char (internal) */
/* synopsis */
/*    char ch, f */
/*    cdcput ( ch, f ) */
/*       f - font for character */
void cdcput ( char ch )
{

  addch(ch);

  return;

}


/* cdclear - clear the desired image */
/* synopsis */
/*    cdclear */
void cdclear(void)
{
  clear();
  return;

}


/* cdclra - clear rectangular area in desired image */
/* synopsis */
/*    int l1, c1, l2, c2 */
/*    cdclra ( l1, c1, l2, c2 ) */
/* description */
/*    This routine clears (to blanks in the normal font) all characters */
/*    in the desired image contained within the rectangle defined by */
/*    the two points. */
void cdclra ( int l1, int c1, int l2, int c2 )
{
  int i, j, rfc, rlc, rfl, rll;
  static char tmpstr[256];

  rfc = max ( 0, min ( c1, c2 ) );

  /*    rlc = min ( maxcol - 1, max ( c1, c2 ) );*/

  rlc = min ( maxcol, max ( c1, c2 ) );

  /* Calculate length. */
  j = rlc - rfc + 1;

  rfl = max ( 0, min ( l1, l2 ) );

  /*    rll = min ( maxlin - 1, max ( l1, l2 ) );*/

  rll = min ( maxlin, max ( l1, l2 ) );

  cdfill(' ', tmpstr, j);
  tmpstr[j] = '\0';

  /*  utLog("cdclra(): rfl = %d rll = %d rfc = %d rlc = %d, maxcol = %d", rfl, rll, rfc, rlc, maxcol);
   */


  for ( i = rfl ; i <= rll ; i = i + 1 )
    {
      cdmove(i, rfc);
      if (rlc == maxcol)
	{
	  clrtoeol();
	}
      else
	addstr(tmpstr);
    }

  return;

}


/* cdclrl - clear lines in desired image */
/* synopsis */
/*    int f, n */
/*    cdclrl ( f, n ) */
void cdclrl ( int f, int n )
{
  int i;

  i = f + n - 1;
  if ( i > 0 )
    cdclra ( f, 1, i, maxcol );

  return;

}


/* cdcols - return the number of columns for this terminal */
/* synopsis */
/*    int cols, cdcols */
/*    cols = cdcols ( 0 ) */
int cdcols ( void )
{
  return ( maxcol );

}

/* cdend - end display environment */
/* synopsis */
/*    cdend */
/* description */
/*    This must be the last routine called from cdlb. */
void cdend(void)
{
#if defined(HAVE_TERMIOS_H)
  struct termios term;
#endif

  endwin();

#if defined (HAVE_TERMIOS_H)
  tcgetattr(iolbStdinFD, &term);
  term.c_cc[VINTR] = Context.intrchar; /* restore INTR */
  tcsetattr(iolbStdinFD, TCSANOW, &term);
#endif

  return;
}


/* cdfill - fill buffer (portable version) */
/* synopsis */
/*    char ch, buf() */
/*    int count */
/*    cdfill ( ch, buf, count ) */
/*       buf - buffer to fill */
/*       count - fill count */
void cdfill ( char ch, char *buf, int count )
{
  register int i;

  for ( i = 0; i < count; i++ )
    buf[i] = ch;

}

/* cdgetn - read a number from the terminal */
/* synopsis */
/*    char pmt() */
/*    int status, cdgetn, num, lin, col */
/*    status = cdgetn ( pmt, lin, col, num ) */
/*       num - number read */
/*       status - 0 if a number was read, else -1 */
int cdgetn ( char pmt[], int lin, int col, int *num )
{
  char buf[MSGMAXLINE];

  cdfill('\0', buf, MSGMAXLINE);
  if ( cdgets ( pmt, lin, col, buf, MSGMAXLINE ) == -1 )
    return -1;

  if (strlen(buf) == 0)
    return -1;

  if (!alldig(buf))
    return -1;

  *num = atoi ( buf );
  return 0;

}


/* cdgets - read a line from the terminal */
/* synopsis */
/*    char pmt(), str() */
/*    int status, cdgets, maxlen, lin, col */
/*    status = cdgets ( pmt, lin, col, str, maxlen ) */
/*       maxlen - size of str */
/* description */
/*    A line is read from the terminal and returned in str. A maximum of */
/*    maxlen characters will be put into the string, although more may be */
/*    read. Characters are read up to a newline or linefeed. */
/*    You can erase characters with either delete or backspace. You can */
/*    erase the line with either control-x or control-u. You can erase */
/*    the last word with control-w. You can redisplay the screen with */
/*    control-l. */
int cdgets ( char pmt[], int lin, int col, char str[], int maxlen )
{

  cdgetx ( pmt, lin, col, "\r\n", str, maxlen, TRUE );

  if ( strlen ( str ) == 0 )
    return(-1);
  else
    return(0);
}


/* cdgetx - read a line from the terminal with special terminators */
/* synopsis */
/*    char pmt(), terms(), str() */
/*    int maxlen, lin, col */
/*    char termch, cdgetx */
/*    termch = cdgetx ( pmt, lin, col, terms, str, maxlen ) */
/*       terms - string of acceptable line terminators */
/*       maxlen - size of str */
/* description */
/*    This routine prompts for input from the terminal using ``pmt''. The */
/*    input string is returned in ``str''. A maximum of maxlen characters */
/*    will be put into the string, although more may be read. Characters */
/*    are read up to one of the terminator characters in the string */
/*    ``terms''. If ``terms'' is an empty string, carriage return is */
/*    allowed as a terminator. */
/*    You can erase characters with either delete or backspace. You can */
/*    erase the line with either control-x or control-u. You can erase */
/*    the last word with control-w. You can redisplay the screen with */
/*    control-l. */
/*    The terminating char is returned as value. */
int cdgetx ( char pmt[], int lin, int col, char terms[], char str[],
	    int maxlen, int doecho )
{
  int append_flg = FALSE;
  int do_append_flg = FALSE;
  return ( cdgetp ( pmt, lin, col, terms, str, maxlen, &append_flg, do_append_flg, doecho ) );

}


/* cdgetp - read a line from the terminal with special terminators */
/* synopsis */
/*    termch = cdgetp ( pmt, lin, col, terms, str, maxlen, doecho ) */
/*       terms - string of acceptable line terminators */
/*       maxlen - size of str */
/*       append_flg - return for more input flag */
/*       do_append_flg - append operation acceptable flag */
/*       doecho - whether or not to echo input */
/* description */
/*    This routine prompts for input from the terminal. The prompt ``pmt'' */
/*    and initial contents of ``str'' and displayed and then input is read */
/*    and the result is returned in ``str''. This means that a default value */
/*    of str can be provided. This routine is otherwise similar to cdgetx(). */
int cdgetp ( char pmt[], int lin, int col, char terms[], char str[],
	    int maxlen, int *append_flg, int do_append_flg, int doecho )
{
  int i, len, icol, scol, imaxlen;
  int ch = 0;
  int StrInit;

  char mbuf[MSGMAXLINE];
  char sbuf[MSGMAXLINE];

  char *append_fmt1 = "%s%s%c";
  char *append_fmt2 = "%s%c";
  char *append_str = "- ";
  int max_size, diff;

  if (str[0] != 0)
    {				/* theres a value there */
      StrInit = TRUE;
    }
  else
    {
      StrInit = FALSE;
    }

  uiPutColor(InfoColor);           /* colorize prompt string */
  cdputs ( pmt, lin, col );
  scol = col + strlen ( pmt );
  uiPutColor(SpecialColor);        /* colorize secondary string */
  cdputs ( str, lin, scol );
  uiPutColor(NoColor);
  len = strlen ( str );
  icol = scol + len;

  imaxlen = min ( maxlen, maxcol - scol + 1 );

  while (TRUE)
    {
      str[ min ( len+1, imaxlen ) ] = 0;
      cdmove ( lin, icol );
      cdrefresh ();

      ch = iogchar();

      if ( terms[0] != 0 )
	{
	  if ( c_index ( terms, ch ) != -1 )
	    break;
	  else if ( ch == TERM_NORMAL )
	    break;
	}

				/* translate KP keys (if any)
				   into 'direction' keys. */
      (void)mcuKP2DirKey(&ch);

      if (ch != TERM_NORMAL && ch != TERM_EXTRA && StrInit == TRUE &&
	  isprint(ch & 0xff))
	{			/* clear out the preload */
	  str[0] = ch;
	  str[1] = 0;
	  len = 1;
	  icol = scol + len;
	  cdclra ( lin, icol, lin, cdcols() );
	  if (doecho)
	    cdputs ( str, lin, scol );

	  StrInit = FALSE;
	  continue;
	}
      else
	if (!isprint(ch & 0xff))
	  {			/* redraw init str in std input color */
	    uiPutColor(NoColor);
	    cdclra ( lin, icol, lin, cdcols() );
	    if (doecho)
	      cdputs ( str, lin, scol );
	    StrInit = FALSE;
	    cdrefresh();
	  }



      if ( ch == '\b' || ch == 0x7f || ch == KEY_BACKSPACE )
	{
	  /* Delete one character. */
	  if ( len > 0 )
	    {
	      len = len - 1;
	      str[len] = '\0';
	      icol = icol - 1;
	      cdclra ( lin, icol, lin, icol );
	    }
	}
      else if ( ch == 0x17 )	/* ^W */
	{
	  /* Delete the last word. */
	  if ( len > 0 )
	    {
	      /* Back up over blanks. */
	      i = icol;				/* remember the end */
	      while ( len >= 0 )
		if ( str[len] == ' ' )
		  {
		    icol = icol - 1;
		    len = len - 1;
		  }
		else
		  break;

	      /* Back up over non-blanks. */
	      while ( len >= 0 )
		if ( str[len] == ' ' )
		  break;
		else
		  {
		    icol = icol - 1;
		    len = len - 1;
		  }

	      if (len < 0 )
		{
		  len = 0;
		}
	      str[len] = 0;

	      /* Clear things in the actual image, if necessary. */
	      if ( icol < i )
		{
		  if (icol <= scol)
		    {
		      cdclra ( lin, scol, lin, i - 1 );
		      icol = scol;
		    }
		  else
		    cdclra ( lin, icol, lin, i - 1 );
		}
	    }
	}
      else if ( ch == 0x15 || ch == 0x18 ) /* ^U || ^X */
	{
	  if ( len > 0 )
	    {
	      cdclra ( lin, scol, lin, icol - 1 );
	      icol = scol;
	      len = 0;
	      str[0] = 0;
	    }
	}
      else if ( ch == TERM_REDRAW )
	{
	  cdredo();
	}
      else if ( ! isprint ( ch & 0xff) )
	{
#ifdef DEBUG_IO
	  utLog("cdgetp1:Got a strange char: ascii %d, errno=%d", ch, errno);
#endif
	  cdbeep();
	}
      else if ( len + 1 < imaxlen )
	{
	  str[len] = (char)ch;
	  len = len + 1;
	  str[len] = '\0';

	  if (doecho == TRUE)
	    cdput ( ch, lin, icol );
	  cdrefresh();
	  icol = icol + 1;
	}
      else
	{
#ifdef DEBUG_IO
	  utLog("cdgetp2:Got a strange char: '%c' = ascii %d", ch, ch);
#endif
	  cdbeep(); /* exceeded max. allowable characters. */
	  if (do_append_flg == TRUE) {
	    /*
	     * Push (if char != ' ') last word back into buffer which will be
	     * inserted back into the msg buffer when cdgetp() is called again.
	     * Replace pushed back chars with "append" symbol and blanks.
	     * Set the flag to come on back and then return from whence we came.
	     */
	    cdclra ( lin, scol, lin, maxlen+1);
	    mbuf[0] = 0;
	    sbuf[0] = 0;
	    str[len] = 0;
	    for (i=len; i>=0 && str[i] != ' '; i--)
	      ;
	    i++;
	    if (i < 0)
	      i=0;
	    max_size = strlen(&str[i]) + strlen(append_str);
	    diff = max_size - (maxlen - 2);

	    /*
	     * Assume the word is too big to reinsert (just repeating the line).
	     * Save last char entered and go on.
	     */
	    if (diff > 0) {
	      strcpy(sbuf, &str[max_size - diff]);
	      str[max_size - diff] = (int)'-';
	      sprintf(mbuf, append_fmt1, append_str, sbuf, ch);
	    }
	    else {
	      if (str[maxlen-2] != ' ') { /* Save last word plus last char entered */
		strcpy(sbuf,&str[i]);
		str[i] = (int)'-';
		sprintf(mbuf, append_fmt1, append_str, sbuf, ch);
		for (i=i+1;i<len;i++)
		  str[i] = ' ';
	      }
	      else { /* Have a space so save last char entered */
		str[maxlen-2] = (int)'-';
		sprintf(mbuf, append_fmt2, append_str, ch);
		for (i=i+1;i<len;i++)
		  str[i] = ' ';
	      }
	    }
	    ibufPut(mbuf);
	    *append_flg = TRUE;
	    break;
	  } /* end if do_append_flg */
	}
    }

  str[ min ( len+1, imaxlen ) ] = 0;

  uiPutColor(0);
  return ( ch );

}

void cdbeep(void)
{
  time_t i;
  static time_t old = 0;

  i = time(0);

  if (i != old)
    {
      old = i;
      beep();
    }

  return;
}


/* cdinit - initialize cdlb */
/* synopsis */
/*    cdinit */
/* description */
/*    This routine must be called before all others in cdlb. */
void cdinit(void)
{

#if defined(HAVE_TERMIOS_H)
  struct termios term;
#endif

  /* Initialize screen library (this MUST be done first). */

  ibufInit();

  initscr();
  start_color();

  uiInitColors();

  nonl();
  typeahead(-1);		/* no typeahead checking */
  keypad(stdscr, TRUE);
  cbreak();
  notimeout(stdscr, TRUE);		/* JET 2/11/96 - this NEEDS to be set */
  intrflush(stdscr, TRUE);

  noecho();

  maxcol = ((COLS > 80) ? 80 : COLS);
  maxlin = LINES;

  /* make sure COLS is >= 80, LINES >= 24 */

  if (maxcol < 80 || LINES < 25)
    {
      cdend();

      fprintf(stderr,
              "\nYour terminal must have at least 80 columns and 25 lines.\n");

      exit(1);
    }

  /* get the users INTR char if possible so it can be stuffed into the
   *  KB buffer if a INTR is recieved.  Useful for unixware systems
   *  where * DEL is the default INTR char.  This should allow DEL
   *  to be used for deleting text for example
   */

  Context.intrchar = 0;	/* default - nothing */

#if defined (HAVE_TERMIOS_H)
  tcgetattr(iolbStdinFD, &term);
  Context.intrchar = term.c_cc[VINTR]; /* save it */
  term.c_cc[VINTR] = 0x03;	/* ^C - harmless */
  tcsetattr(iolbStdinFD, TCSANOW, &term);
#endif

  cdclear();

  return;

}


/* cdline - draw horizontal or vertical line */
/* synopsis */
/*    int lin1, col1, lin2, col2 */
/*    cdline ( lin1, col1, lin2, col2 ) */
/* description */
/*    This routine draws either a horizontal or a vertical line between */
/*    between the two screen positions. Bad parameters are ignored. */
void cdline ( int lin1, int col1, int lin2, int col2 )
{
  if ( lin1 == lin2 )
    {
      /* A horizontal line. */
      if ( col1 < col2 )
	{
	  /* Left to right. */
	  cdmove(lin1, col1);
	  hline(0, col2 - col1);
	}
      else if ( col1 > col2 )
	{
	  /* Right to left. */
	  cdmove(lin1, col2);
	  hline(0, col1 - col2);
	}
    }
  else if ( col1 == col2 )
    {
      /* A vertical line. */
      if ( lin1 < lin2 )
	{
	  /* Top down. */
	  cdmove(lin1, col1);
	  vline(0, lin2 - lin1);
	}
      else if ( lin1 > lin2 )
	{
	  /* Bottom up. */
	  cdmove(lin1, col1);
	  vline(0, lin1 - lin2);
	}
    }

  return;

}


/* cdlins - return the number of lines for this terminal */
/* synopsis */
/*    int lin, cdlins */
/*    lin = cdlins ( 0 ) */
int cdlins ( void )
{
  return ( maxlin );

}

/* cdmove - move position of cursor in desired image */
/* synopsis */
/*    int lin, col */
/*    cdmove ( lin, col ) */
void cdmove ( int lin, int col )
{

  /* ratfor sclib addresses screen */
  /* coordinates starting at 1, curses */
  /* always starts with 0, regardless */
  /* of terminal's convention. */
  if (lin != 0)
    lin--;
  if (col != 0)
    col--;

  lin = (lin > (maxlin - 1)) ? maxlin - 1 : lin; /* make sure we're */
  col = (col > (maxcol - 1)) ? maxcol - 1 : col; /* within screen */
  /* boundries */

  /* do it hard and fast (we hope)*/
  move(lin, col);

  return;

}


/* cdrefresh - make screen image look like desired image */
/* synopsis */
/*     int stoponinput */
/*     cdrefresh ( stoponinput ) */
/* description */
/*   This routine makes the screen image look like the desired image. */
/*   If stoponinput is set, cdrefresh checks periodically to see if there */
/*   is a char of input ready; if so, cdrefresh() returns when called */
/*   again, it will resume where it left off. */
void cdrefresh ( void )
{
  refresh();
  return;
}


/* cdput - put char in desired image */
/* synopsis */
/*    char ch */
/*    int lin, col */
/*    cdput ( ch, lin, col ) */
void cdput ( char ch, int lin, int col )
{
  char tmpstr[2];

  tmpstr[0] = ch;
  tmpstr[1] = '\0';

  cdputs(tmpstr, lin, col);

  return;

}


/* cdputc - put a string into the display buffer, centered */
/* synopsis */
/*    char str() */
/*    int lin */
/*    cdputc ( str, lin ) */
void cdputc ( char str[], int lin )
{
  cdputs ( str, lin, (int)(cdcols() - strlen(str)) / (int)2 );

  return;
}


/* cdputn - put a number into the display buffer */
/* synopsis */
/*    int int, wid, lin, col */
/*    cdputn ( int, wid, lin, col ) */
void cdputn ( int iint, int wid, int lin, int col )
{
  char fmt[20], buf[MSGMAXLINE];

  sprintf ( fmt, "%%%dd", wid );
  sprintf ( buf, fmt, iint );
  cdputs ( buf, lin, col );

  return;
}


/* cdputr - put a real number into the display buffer */
/* synopsis */
/*    real x */
/*    int wid, lin, col */
/*    cdputr ( x, wid, lin, col ) */
void cdputr ( real x, int wid, int lin, int col )
{
  char fmt[20], buf[MSGMAXLINE];

  sprintf ( fmt, "%%%dg", wid );
  sprintf ( buf, fmt, x);
  cdputs ( buf, lin, col );

  return;

}


/* cdputs - put a string into the display buffer */
/* synopsis */
/*    char str() */
/*    int lin, col */
/*    cdputs ( str, lin, col ) */
/* description */
/*    This routine puts the string into the display buffer starting at */
/*    the specified position and proceeding to the right. If the string */
/*    would go past the edge of the screen, it is truncated. */
void cdputs ( char str[], int lin, int col )
{
  int len, slen;
  static char tmpstr[128];

  strcpy(tmpstr, str);

  len = Context.maxcol - col; /* max str that will fit on screen */
  slen = strlen(tmpstr);

  len = (len >= 0) ? len : 0;

  if (slen >= len )
    tmpstr[len] = '\0';

  cdmove(lin, col);

  addstr(tmpstr);
  return;
}


/* cdredo - force a complete refresh on next display */
/* synopsis */
/*    cdredo */
/* description */
/*   Clears the screen image, so that there will be a complete */
/*   refresh next time cdrefresh() is called. */
void cdredo(void)
{
  wrefresh(curscr);

  return;
}
