#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

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
#include "cdcom.h"

#include "conqcom2.h"
#include "global.h"
#include "color.h"

#define MSGMAXLINE 90 			/* used for screen formatting */

/*## cdbox - draw a box */
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


/*## cdcput - output a char (internal) */
/* synopsis */
/*    char ch, f */
/*    cdcput ( ch, f ) */
/*       f - font for character */
void cdcput ( char ch, char f )
{
  
  addch(ch);
  
  return;
  
}


/*## cdclear - clear the desired image */
/* synopsis */
/*    cdclear */
void cdclear(void)
{
  clear();
  return;
  
}


/*## cdclra - clear rectangular area in desired image */
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
  
  rfc = max0 ( 0, min0 ( c1, c2 ) );
  
  /*    rlc = min0 ( maxcol - 1, max0 ( c1, c2 ) );*/
  
  rlc = min0 ( maxcol, max0 ( c1, c2 ) );
  
  /* Calculate length. */
  j = rlc - rfc + 1;
  
  rfl = max0 ( 0, min0 ( l1, l2 ) );
  
  /*    rll = min0 ( maxlin - 1, max0 ( l1, l2 ) );*/
  
  rll = min0 ( maxlin, max0 ( l1, l2 ) );
  
  cdfill(' ', tmpstr, j);
  tmpstr[j] = '\0';

/*  clog("cdclra(): rlc = %d, maxcol = %d", rlc, maxcol);*/


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


/*## cdclrl - clear lines in desired image */
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


/*## cdcols - return the number of columns for this terminal */
/* synopsis */
/*    int cols, cdcols */
/*    cols = cdcols ( 0 ) */
int cdcols ( int dummy )
{
  return ( maxcol );
  
}

/*## cdend - end display environment */
/* synopsis */
/*    cdend */
/* description */
/*    This must be the last routine called from cdlb. */
void cdend(void)
{
  
  endwin();
  
  return;
}


/*## cdfill - fill buffer (portable version) */
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

/*## cdgetn - read a number from the terminal */
/* synopsis */
/*    char pmt() */
/*    int status, cdgetn, num, lin, col */
/*    status = cdgetn ( pmt, lin, col, num ) */
/*       num - number read */
/*       status - OK if a number was read, else ERR */
int cdgetn ( char pmt[], int lin, int col, int *num )
{
  char buf[MSGMAXLINE];
  int i;
  
  if ( cdgets ( pmt, lin, col, buf, MSGMAXLINE ) == ERR )
    return ( ERR );
  
  *num = ctoi ( buf );
  return ( OK );
  
}


/*## cdgets - read a line from the terminal */
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
  int length;
  int termch;
  string terms="\r\n";
  
  termch = cdgetx ( pmt, lin, col, terms, str, maxlen );
  
  if ( strlen ( str ) == 0 )
    return(ERR);
  else
    return(OK);
  
  
}


/*## cdgetx - read a line from the terminal with special terminators */
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
	    int maxlen )
{
  str[0] = EOS;
  return ( cdgetp ( pmt, lin, col, terms, str, maxlen ) );
  
}


/*## cdgetp - read a line from the terminal with special terminators */
/* synopsis */
/*    char pmt(), terms(), str() */
/*    int maxlen, lin, col */
/*    char termch, cdgetx */
/*    termch = cdgetp ( pmt, lin, col, terms, str, maxlen ) */
/*       terms - string of acceptable line terminators */
/*       maxlen - size of str */
/* description */
/*    This routine prompts for input from the terminal. The prompt ``pmt'' */
/*    and initial contents of ``str'' and displayed and then input is read */
/*    and the result is returned in ``str''. This means that a default value */
/*    of str can be provided. This routine is otherwise similar to cdgetx(). */
int cdgetp ( char pmt[], int lin, int col, char terms[], char str[], 
	    int maxlen )
{
  int i, len, icol, scol, imaxlen;
  int ch;
  
  cdputs ( pmt, lin, col );
  scol = col + strlen ( pmt );
  cdputs ( str, lin, scol );
  len = strlen ( str );
  icol = scol + len;
  
  imaxlen = min0 ( maxlen, maxcol - scol + 1 );
  
  while (TRUE)
    {
      str[ min0 ( len+1, imaxlen ) ] = EOS;
      cdmove ( lin, icol );
      cdrefresh ( TRUE );
      
      ch = iogchar(ch);
      
      if ( terms[0] != EOS )
	if ( c_index ( terms, ch ) != ERR )
	  break;
	else if ( ch == TERM_NORMAL )
	  break;
      if ( ch == '\b' || ch == '\x7f' || ch == KEY_BACKSPACE )
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
      else if ( ch == '\x17' )
	{
	  /* Delete the last word. */
	  if ( len > 0 )
	    {
	      /* Back up over blanks. */
	      i = icol;				/* remember the end */
	      while ( len > 0 )
		if ( str[len] == ' ' )
		  {
		    icol = icol - 1;
		    len = len - 1;
		  }
		else
		  break;
	      
	      /* Back up over non-blanks. */
	      while ( len > 0 )
		if ( str[len] == ' ' )
		  break;
		else
		  {
		    icol = icol - 1;
		    len = len - 1;
		  }
	      
	      /* Clear things in the actual image, if necessary. */
	      if ( icol < i )
		cdclra ( lin, icol, lin, i - 1 );
	    }
	}
      else if ( ch == '\x15' || ch == '\x18' )
	{
	  if ( len > 0 )
	    {
	      cdclra ( lin, scol, lin, icol - 1 );
	      icol = scol;
	      len = 0;
	    }
	}
      else if ( ch == '\x0c' )
	cdredo();
      else if ( ! isprint ( ch ) )
	{
#ifdef DEBUG_IO
	  clog("cdgetp1:Got a strange char: ascii %d, errno=%d", ch, errno);
#endif
	  cdbeep();
	}
      else if ( len + 1 < imaxlen )
	{
	  str[len] = (char)ch;
	  len = len + 1;
	  str[len] = '\0';
	  /*str[len] = ch;*/
	  /*	    cdput ( ch, lin, icol );*/
	  cdput ( ch, lin, icol );
	  cdrefresh(FALSE);
	  icol = icol + 1;
	}
      else
	{
#ifdef DEBUG_IO
	  clog("cdgetp2:Got a strange char: '%c' = ascii %d", ch, ch);
#endif
	  cdbeep();
	}
    }
  
  str[ min0 ( len+1, imaxlen ) ] = EOS;
  
  return ( ch );
  
}

/*## cdgoto - move the cursor optimally (internal) */
/* synopsis */
/*    int lin, col */
/*    cdgoto ( lin, col ) */
/* description */
/*    This routine chooses a cursor movement strategy by comparing */
/*    char costs. Currently, it uses the following strategies: */
/*    Special note: relative cursor movement to the right usually */
/*    costs only one char per position moved, even on terminals */
/*    whose cursor-right commands are more than one char long. */
/*    This is because movement to the right is accomplished by re-typing */
/*    the characters that are already in the positions being moved over. */
/*    The exception to this is when the char has attributes other */
/*    than the current ones set. */
void cdgoto ( int lin, int col )
{
  
  cdmove(lin, col);
  
  return;
}



void cdbeep(void)
{
  int i;
  static int old = 0;
  
  if (conf_DoLimitBell == TRUE)	/* limit beeps to no more than 1 per sec */
    {
      i = time(0);
      
      if (i != old)
	{
	  old = i;
	  beep();
	}
    }
  else
    beep();			/* else beep whenever the urge hits you */
  
  return;
}


/*## cdinit - initialize cdlb */
/* synopsis */
/*    cdinit */
/* description */
/*    This routine must be called before all others in cdlb. */
void cdinit(void)
{
  
  int i;
  int ich;
  char ch;
  
  /* Initialize screen library (this MUST be done first). */
  
#ifdef ENABLE_MACROS
  iBufInit();
#endif
  
  
  PollInputfd = 0;		/* for stdin */
  
  initscr();
  start_color();
  
  if (has_colors() && conf_NoColor == FALSE)
    {
#ifdef DEBUG_COLOR
      clog("Terminal has colors, COLORS = %d, COLOR_PAIRS = %d",
	   COLORS,
	   COLOR_PAIRS);
#endif
      init_pair(COL_BACKGROUND, COLOR_WHITE, COLOR_BLACK);
      init_pair(COL_REDBLACK, COLOR_RED, COLOR_BLACK);
      init_pair(COL_GREENBLACK, COLOR_GREEN, COLOR_BLACK);
      init_pair(COL_YELLOWBLACK, COLOR_YELLOW, COLOR_BLACK);
      init_pair(COL_BLUEBLACK, COLOR_BLUE, COLOR_BLACK);
      init_pair(COL_MAGENTABLACK, COLOR_MAGENTA, COLOR_BLACK);
      init_pair(COL_CYANBLACK, COLOR_CYAN, COLOR_BLACK);

		/* now init the backgound */
		/* ncurses seems to toast the colors if you do this */
		/*  so no background init is done with ncurses. */
		/* This means that if your using ncurses, run conquest */
		/*  on a display with a black background for best results */
#if !defined(HAVE_NCURSES_H) && !defined(NCURSES_VERSION)
      bkgdset(COLOR_PAIR(COL_BACKGROUND) | ' ');
#endif

      HAS_COLORS = TRUE;

				/* init default attributes */
      LabelColor = COLOR_PAIR(COL_BLUEBLACK) | A_BOLD;
      InfoColor = COLOR_PAIR(COL_CYANBLACK) | A_BOLD;
      RedLevelColor = COLOR_PAIR(COL_REDBLACK) | A_BOLD;
      YellowLevelColor = COLOR_PAIR(COL_YELLOWBLACK) | A_BOLD;
      GreenLevelColor = COLOR_PAIR(COL_GREENBLACK) | A_BOLD;
    }
  else
    {

#ifdef DEBUG_COLOR
      clog("Terminal DOES NOT have colors, COLORS = %d, COLOR_PAIRS = %d",
	   COLORS,
	   COLOR_PAIRS);
#endif

      HAS_COLORS = FALSE;
      
				/* init default attributes */
      LabelColor = 0;
      InfoColor = 0;
      RedLevelColor = (A_BOLD | A_REVERSE);
      YellowLevelColor = A_BOLD;
      GreenLevelColor = 0;

    }
  
  nonl(); 
  typeahead(-1);		/* no typeahead checking */
  keypad(stdscr, TRUE);
  cbreak();
  notimeout(stdscr, TRUE);		/* JET 2/11/96 - this NEEDS to be set */
  intrflush(stdscr, TRUE);
  
  noecho();
  
  maxcol = ((COLS > 80) ? 80 : COLS);
  maxlin = LINES;

  /* make sure COLS is >= 80,
     LINES >= 24 */
  
  if (maxcol < 80 || LINES < 24)
    {
      cdend();
      
      fprintf(stderr, "Your terminal must have at least 80 columns and 24 lines.\n");
      
      exit(1);
    }
  
  /* If you have  25 or more lines, use line 25 to display messages in, else
     set to regular msg line... */

  if (LINES >= 25)
    {
      RMsg_Line = 25;
    }
  else
    {
      RMsg_Line = MSG_LIN1;
    }

#ifdef DEBUG_FLOW
  clog("cdinit(): RMsg_Line = %d", RMsg_Line);
#endif

  cdclear();
  return;
  
}


/*## cdline - draw horizontal or vertical line */
/* synopsis */
/*    int lin1, col1, lin2, col2 */
/*    cdline ( lin1, col1, lin2, col2 ) */
/* description */
/*    This routine draws either a horizontal or a vertical line between */
/*    between the two screen positions. Bad parameters are ignored. */
void cdline ( int lin1, int col1, int lin2, int col2 )
{
  int i;
  
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


/*## cdlins - return the number of lines for this terminal */
/* synopsis */
/*    int lin, cdlins */
/*    lin = cdlins ( 0 ) */
int cdlins ( int dummy )
{
  return ( maxlin );
  
}

/*## cdmove - move position of cursor in desired image */
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


/*## cdrefresh - make screen image look like desired image */
/* synopsis */
/*     int stoponinput */
/*     cdrefresh ( stoponinput ) */
/* description */
/*   This routine makes the screen image look like the desired image. */
/*   If stoponinput is set, cdrefresh checks periodically to see if there */
/*   is a char of input ready; if so, cdrefresh() returns when called */
/*   again, it will resume where it left off. */
void cdrefresh ( int stoponinput )
{
  refresh();
  return;
}


/*## cdput - put char in desired image */
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


/*## cdputc - put a string into the display buffer, centered */
/* synopsis */
/*    char str() */
/*    int lin */
/*    cdputc ( str, lin ) */
void cdputc ( char str[], int lin )
{
  cdputs ( str, lin, (maxcol-strlen(str))/2 );
  
  return;
}


/*## cdputn - put a number into the display buffer */
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


/*## cdputr - put a real number into the display buffer */
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


/*## cdputs - put a string into the display buffer */
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
  
  len = cmaxcol - col; /* - 1; /* max str that will fit on screen */
  slen = strlen(tmpstr);
  
  len = (len >= 0) ? len : 0;
  
  if (slen >= len )
    tmpstr[len] = '\0';
  
  cdmove(lin, col);
  
  addstr(tmpstr);	
  return;
}


/*## cdredo - force a complete refresh on next display */
/* synopsis */
/*    cdredo */
/* description */
/*   Clears the screen image, so that there will be a complete */
/*   refresh next time cdrefresh() is called. */
void cdredo(void)
{
  cdclear();
  return;
}


