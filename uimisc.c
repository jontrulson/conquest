/*
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"

#include "record.h"
#include "conqutil.h"

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
        time_t recon = (time_t)recFileHeader.rectime;

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

        strncpy(recordedon, ctime(&recon), MSGMAXLINE - 1);
        recordedon[MSGMAXLINE - 1] = 0;

        for (i=0; i < strlen(recordedon); i++)
            if (recordedon[i] == '\n')
                recordedon[i] = 0;
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

    cprintf(lin,col,ALIGN_NONE,sfmt, "File               ", recFilename);
    lin++;

    cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded By        ", recFileHeader.user);
    lin++;

    if (recFileHeader.flags & RECORD_F_SERVER)
    {
        // server recording ship was always 0 in this version
        if (recFileHeader.vers == RECVERSION_20031004)
            sprintf(cbuf, "Server [%d, 0x%04x]", recFileHeader.vers,
                    (int)recFileHeader.protoVers);
        else
            sprintf(cbuf, "Server (Ship %d) [%d, 0x%04x]",
                    recFileHeader.snum, recFileHeader.vers,
                    (int)recFileHeader.protoVers);
    }
    else
    {
        sprintf(cbuf, "Client (Ship %d) [%d, 0x%04x]",
                recFileHeader.snum, recFileHeader.vers,
                (int)recFileHeader.protoVers);
    }

    cprintf(lin,col,ALIGN_NONE,sfmt, "Recording Type     ", cbuf);

    lin++;

    cprintf(lin,col,ALIGN_NONE,sfmt, "Recorded On        ", recordedon);

    lin++;
    sprintf(cbuf, "%d (delay: %0.3fs)", recFileHeader.samplerate, recFrameDelay);
    cprintf(lin,col,ALIGN_NONE,sfmt, "Updates per second ", cbuf);
    lin++;
    utFormatSeconds(recTotalElapsed, cbuf);

    if (cbuf[0] == '0')	/* see if we need the day count */
        c = &cbuf[2];
    else
        c = cbuf;

    cprintf(lin,col,ALIGN_NONE,sfmt, "Total Game Time    ", c);
    lin++;
    utFormatSeconds((recCurrentTime - recStartTime), cbuf);

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

/* display help for replaying */
void dspReplayHelp(void)
{
    int lin, col, tlin;
    static int FirstTime = TRUE;
    static char sfmt[MSGMAXLINE * 2];

    if (FirstTime == TRUE)
    {
        FirstTime = FALSE;
        sprintf(sfmt,
                "#%d#%%-9s#%d#%%s",
                InfoColor,
                LabelColor);
    }

    cprintf(1,0,ALIGN_CENTER,"#%d#%s", LabelColor, "WATCH WINDOW COMMANDS");

    lin = 4;

    /* Display the left side. */
    tlin = lin;
    col = 4;
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "w", "watch a ship");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt,
            "<>", "decrement/increment ship number\n");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "/", "player list");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "f", "forward 30 seconds");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "F", "forward 2 minutes");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "b", "backward 30 seconds");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "B", "backward 2 minutes");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "r", "reset to beginning");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "q", "quit");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "[SPACE]", "pause/resume playback");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "-", "slow down playback by doubling the frame delay");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "+", "speed up playback by halfing the frame delay");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "M", "short/long range sensor toggle");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "n", "reset to normal playback speed");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "`", "toggle between two ships");
    tlin++;
    cprintf(tlin,col,ALIGN_NONE,sfmt, "!", "display toggle line");

    return;
}

/* get the 'real' strlen of a string, skipping past any embedded colors */
int uiCStrlen(char *buf)
{
    register char *p;
    register int l;

    l = 0;
    p = buf;
    while (*p)
    {
        if (*p == '#')
        {                       /* a color sequence */
            p++;
            while (*p && isdigit(*p))
                p++;

            if (*p == '#')
                p++;
        }
        else
        {
            p++;
            l++;
        }
    }

    return l;
}
