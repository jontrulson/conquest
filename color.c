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
/*                                                                    */
/**********************************************************************/

#include "conqdef.h"
#include "conqcom2.h"
#include "global.h"
#define NOEXPORT_COLORS
#include "color.h"		/* instantiate externs here */

/* initialize color variables.  if has_colorcap == FALSE, fake it */
void InitColors(int has_colorcap)
{


  NoColor = 0;			/* default color (white usually) */

  if (has_colorcap == TRUE)
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

      HasColors = TRUE;

				/* init colors */
      RedColor = COLOR_PAIR(COL_REDBLACK);
      GreenColor = COLOR_PAIR(COL_GREENBLACK);
      YellowColor = COLOR_PAIR(COL_YELLOWBLACK);
      MagentaColor = COLOR_PAIR(COL_MAGENTABLACK);
      CyanColor = COLOR_PAIR(COL_CYANBLACK);
      BlueColor = COLOR_PAIR(COL_BLUEBLACK);

				/* init default attributes */
      LabelColor = BlueColor | A_BOLD;
      InfoColor = CyanColor | A_BOLD;
      SpecialColor = MagentaColor | A_BOLD;
      RedLevelColor = RedColor | A_BOLD;
      YellowLevelColor = YellowColor | A_BOLD;
      GreenLevelColor = GreenColor | A_BOLD;

    }
  else				/* no colors */
    {
#ifdef DEBUG_COLOR
      clog("Terminal DOES NOT have colors, COLORS = %d, COLOR_PAIRS = %d",
           COLORS,
           COLOR_PAIRS);
#endif

      HasColors = FALSE;
      
				/* init colors to NoColor */
      RedColor = NoColor;
      GreenColor = NoColor;
      YellowColor = NoColor;
      MagentaColor = NoColor;
      CyanColor = NoColor;
      BlueColor = NoColor;


				/* init default attributes */
      LabelColor = NoColor;
      InfoColor = NoColor;
      SpecialColor = A_BOLD;
      RedLevelColor = (A_BOLD | A_REVERSE);
      YellowLevelColor = A_BOLD;
      GreenLevelColor = NoColor;

    }

  return;
}

/* 
 * int lin - line num if pertinent
 * int col - line num if pertinent
 * int align - ALIGN_CENTER 3
 *             ALIGN_RIGHT  2
 *             ALIGN_LEFT   1
 *             ALIGN_NONE   0
 * char *fmt - modified fmt string with color codes embedded
 *             '#' color code
 *             '%' regular format specs
 * ...       - args 
 */
void cprintf(int lin, int col, int align, char *fmt, ...)
{
  va_list ap;
  register int i, j, k;
  int len, vcol;
  int color_num;
  int two_passes = FALSE;
  static char buf[MID_BUFFER_SIZE] = "";
  static char xbuf[MID_BUFFER_SIZE] = "";
  static char color_code[BUFFER_SIZE] = "";

  vcol = col;
  buf[0] = EOS;
  xbuf[0] = EOS;
  color_code[0] = EOS;
  
  /* use vsprintf */
  va_start(ap, fmt);
  (void)vsprintf(buf, fmt, ap);
  va_end(ap);
  /* end of it */
  
  if ( align == ALIGN_CENTER )
    two_passes = TRUE;
  while (TRUE)
    {
      i=0;j=0;
      xbuf[0]=EOS;
      while (buf[i] != EOS && i < MID_BUFFER_SIZE)
	{
	  if ( buf[i] == '#' )   /* color code warning */
	    {
	      if ( buf[i+1] == '#' ) 
		{
		  xbuf[j++] = buf[i++]; /* escape'd # */
		  continue;
		}
	      /* test for phrase to output */

	      xbuf[j] = EOS;
	      if ( !two_passes ) 
		if ( strlen(xbuf) > 0 && i > 0 )
		  { 
		    cdputs(xbuf, lin, vcol);        /* if have something to */
		    vcol += strlen(xbuf);           /* display then do so */
		    j=0;
		  }

	      k=0;
	      color_code[0] = EOS;
	      if ( buf[i] == '#' ) 
		i++;                /* advance past '#' */
	      /* get color code */

	      while ( buf[i] != EOS && isdigit(buf[i]) ) 
		{  
		  if ( !two_passes ) 
		    {
		      color_code[k++] = buf[i++];
		      color_code[k] = EOS;
		    }
		  else
		    i++;
		}

	      if ( buf[i] == '#' )
		i++;                /* advance past '#' */
	      /* have color code, convert to int and set the color attribute */
	      if ( !two_passes )
		{
		  color_num = atoi(color_code);
		  attrset(color_num);
		}
	    } /* end if color code start*/
	  xbuf[j++] = buf[i++];  /* save char to output xbuf */
	} /* end outer while */	
      xbuf[j] = EOS;

      len = strlen(xbuf);
      /* display the last part of the line. */
      if ( !two_passes )
	if ( len > 0 && i > 0 )
	  cdputs(xbuf, lin, vcol);
      
      /* calculate where the complete line should start for centered text */
      if ( two_passes ) 
	{
	  vcol = ((cdcols() / 2) - (len / 2));
	  two_passes = FALSE;
	}
      else 
	break; /* one pass and we're done. */

    } /* end while (TRUE) */	
  
  attrset(0);
  
  return;
  
}
