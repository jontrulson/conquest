/* 
 * record.c - recording games in conquest
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */
#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"
#include "datatypes.h"
#include "record.h"

void dspReplayMenu(void)
{
  int lin, col;
  char *c;
  int i;
  char cbuf[MSGMAXLINE];
  static int FirstTime = TRUE;
  static char sfmt[MSGMAXLINE * 2];
  static char cfmt[MSGMAXLINE * 2];
  static char recordedon[MSGMAXLINE];
  extern char *ConquestVersion;
  extern char *ConquestDate;

  if (FirstTime == TRUE)
    {
      FirstTime = FALSE;
      sprintf(sfmt,
	      "#%d#%%s#%d#: %%s",
	      InfoColor,
	      GreenColor);
      
      sprintf(cfmt,
              "#%d#(#%d#%%c#%d#) - %%s",
              LabelColor,
              InfoColor,
              LabelColor);

      strncpy(recordedon, ctime((time_t *)&fhdr.rectime), MSGMAXLINE - 1);
      recordedon[MSGMAXLINE - 1] = EOS;

      for (i=0; i < strlen(recordedon); i++)
        if (recordedon[i] == '\n')
          recordedon[i] = EOS;
    }


  lin = 1;
  cprintf(lin, 0, ALIGN_CENTER, "#%d#CONQUEST REPLAY PROGRAM",
          NoColor|CQC_A_BOLD);
  sprintf( cbuf, "%s (%s)",
           ConquestVersion, ConquestDate);
  cprintf(lin+1, 0, ALIGN_CENTER, "#%d#%s",
          YellowLevelColor, cbuf);
  
  lin+=3;
  
  cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Recording info:");
  lin+=2;
  
  col = 5;
  
  cprintf(lin,col,ALIGN_NONE,sfmt, "File               ", rfname);
  lin++;
  
  cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded By        ", fhdr.user);
  lin++;
  
  if (fhdr.snum == 0)
    cprintf(lin,col,ALIGN_NONE,sfmt, "Recording Type     ", "Server");
  else
    {
      sprintf(cbuf, "Client (Ship %d)", fhdr.snum);
      cprintf(lin,col,ALIGN_NONE,sfmt, "Recording Type     ", cbuf);
    }
  lin++;
  
  cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded On        ", recordedon);

  lin++;
  sprintf(cbuf, "%d (delay: %0.3fs)", fhdr.samplerate, framedelay);
  cprintf(lin,col,ALIGN_NONE,sfmt, "Updates per second ", cbuf);
  lin++;
  fmtseconds(totElapsed, cbuf);
  
  if (cbuf[0] == '0')	/* see if we need the day count */
    c = &cbuf[2];	
  else
    c = cbuf;
  
  cprintf(lin,col,ALIGN_NONE,sfmt, "Total Game Time    ", c);
  lin++;
  fmtseconds((currTime - startTime), cbuf);
  
  if (cbuf[0] == '0')
    c = &cbuf[2];	
  else
    c = cbuf;
  cprintf(lin,col,ALIGN_NONE,sfmt, "Current Time       ", c);
  lin++;
  lin++;
  
  cprintf(lin,0,ALIGN_CENTER,"#%d#%s",NoColor, "Commands:");
  lin+=3;
  
  cprintf(lin,col,ALIGN_NONE,cfmt, 'w', "watch a ship");
  lin++;
  cprintf(lin,col,ALIGN_NONE,cfmt, '/', "list ships");
  lin++;
  cprintf(lin,col,ALIGN_NONE,cfmt, 'r', "reset to beginning");
  lin++;
  cprintf(lin,col,ALIGN_NONE,cfmt, 'q', "quit");
  lin++;
  
  return;
}

