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
#include "conqdef.h"
#include "cb.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"

#include "protocol.h"
#include "packet.h"
#include "conqutil.h"
#include "cprintf.h"

#include "protocol.h"
#include "client.h"
#include "record.h"

#define NOEXTERN_PLAYBACK
#include "playback.h"
#undef NOEXTERN_PLAYBACK

/* open, create/load our cmb, and get ready for action if elapsed == NULL
   otherwise, we read the entire file to determine the elapsed time of
   the game and return it */
int pbInitReplay(char *fname, time_t *elapsed)
{
    int pkttype;
    time_t starttm = 0;
    time_t curTS = 0;
    char buf[PKT_MAXSIZE];

    if (!recOpenInput(fname))
    {
        printf("pbInitReplay: recOpenInput(%s) failed\n", fname);
        return(false);
    }

    /* now lets read in the file header and check a few things. */

    if (!recReadHeader(&recFileHeader))
        return(false);

    // Weather we need to have the old hardcoded limits installed.
    // Otherwise for newer rec versions, the fileheader will contain
    // the correct limits.
    bool needsOldLimits = false; 

    /* version check */
    switch (recFileHeader.vers)
    {
    case RECVERSION: // from rec version 20171108 onward we use the
                     // stored protocol version.  Also, we get the
                     // cbLimits in the file header.
        break;

    case RECVERSION_20060409:
        // we used protocol version 6 in this version, but did not
        // have an element at the time to record this in the header,
        // so add it.
        recFileHeader.protoVers = 0x0006;
        needsOldLimits = true;
        break;

    case RECVERSION_20031004:
    {
        // we used protocol version 6 in this version, but did not
        // have an element at the time to record this in the header,
        // so add it.
        recFileHeader.protoVers = 0x0006;

        /* in this version we differentiated server/client recordings
         *  by looking at snum.  If snum == 0, then it was a server
         *  recording, else it was a client.  the 'flags' member did
         *  not exist.  So here we massage it so it will work ok.
         */

        if (recFileHeader.snum == 0)     /* it was a server recording */
            recFileHeader.flags |= RECORD_F_SERVER;

        needsOldLimits = true;
    }
    break;

    default:
    {
        utLog("pbInitReplay: unknown recording version: %d\n",
              recFileHeader.vers);
        printf("pbInitReplay: unknown recording version: %d\n",
               recFileHeader.vers);
        return false;
    }
    break;
    }


    // Now we need to setup cbLimits, either based on fileheader data
    // (if it's a new/current version), or the old hardcoded limits if
    // an old version.  Then we (re)map the common block with the new
    // limits.

    if (needsOldLimits)
    {
        // Need to use the old hardcoded values for MAXSHIPS,
        // MAXPLANETS, etc...
        cbLimits.setMaxPlanets(60);
        cbLimits.setMaxShips(20);
        cbLimits.setMaxUsers(500);
        cbLimits.setMaxHist(40);
        cbLimits.setMaxMsgs(60);
        cbLimits.setMaxTorps(9);
    }
    else
    {
        // get the values from the file header
        cbLimits.setMaxPlanets(recFileHeader.maxplanets);
        cbLimits.setMaxShips(recFileHeader.maxships);
        cbLimits.setMaxUsers(recFileHeader.maxusers);
        cbLimits.setMaxHist(recFileHeader.maxhist);
        cbLimits.setMaxMsgs(recFileHeader.maxmsgs);
        cbLimits.setMaxTorps(recFileHeader.maxtorps);
    }

    // now map the common block (but not if we are only scanning -
    // elapsed == NULL)
    if (!elapsed)
    {
        if (cbIsMapped())
            cbUnmapLocal();

        cbMapLocal();
    }

    // set the correct client protocol version depending on above checks.
    pktSetClientProtocolVersion(recFileHeader.protoVers);

    /* if we are looking for the elapsed time, scan the whole file
       looking for timestamps. */
    if (elapsed)			/* we want elapsed time */
    {
        int done = false;

        starttm = recFileHeader.rectime;

        curTS = 0;
        /* read through the entire file, looking for timestamps. */

#if defined(DEBUG_REC)
        utLog("conqreplay: pbInitReplay: reading elapsed time");
#endif

        while (!done)
	{
            if ((pkttype = recReadPkt(buf, PKT_MAXSIZE)) == SP_FRAME)
            {
                PKT_PROCSP(buf);
                curTS = sFrame.time;
            }

            if (pkttype == SP_NULL)
                done = true;	/* we're done */
	}

        if (curTS != 0)
            *elapsed = (curTS - starttm);
        else
            *elapsed = 0;

        /* now close the file so that the next call of pbInitReplay can
           get a fresh start. */
        recCloseInput();
    }

    /* now we are ready to start running packets */

    return(true);
}


/* read a packet and dispatch it. */
int pbProcessPackets(void)
{
    char buf[PKT_MAXSIZE];
    int pkttype;

#if defined(DEBUG_REC)
    utLog("%s: ENTER", __FUNCTION__);
#endif

    if ((pkttype = recReadPkt(buf, PKT_MAXSIZE)) != SP_NULL)
    {
        if (pkttype < 0 || pkttype >= serverPktMax)
            fprintf(stderr, "%s: Invalid pkttype %d\n", __FUNCTION__, pkttype);
        else
            PKT_PROCSP(buf);
    }

    return pkttype;
}


/* seek around in a game.  backwards seeks will be slooow... */
void pbFileSeek(time_t newtime)
{
    if (newtime == recCurrentTime)
        return;			/* simple case */

    if (newtime < recCurrentTime)
    {				/* backward */
        /* we have to reset everything and start from scratch. */

        recCloseInput();

        if (!pbInitReplay(recFilename, NULL))
            return;			/* bummer */

        recCurrentTime = recStartTime;
    }

    /* start searching */

    /* just read packets until 1. recCurrentTime exceeds newtime, or 2. no
       data is left */
    while (recCurrentTime < newtime)
        if ((pbProcessPackets() == SP_NULL))
            break;		/* no more data */

    return;
}

/* read and process packets until a FRAME packet or EOD is found */
int pbProcessIter(void)
{
    int rtype;

    while(((rtype = pbProcessPackets()) != SP_NULL) && rtype != SP_FRAME)
        ;

    return(rtype);
}

/* This function accepts a playback speed, checks the limits, and sets
 * the recFrameDelay appropriately.
 */
void pbSetPlaybackSpeed(int speed, int samplerate)
{
    /* first check the limits and adjust accordingly */

    if (speed > PB_SPEED_INFINITE)
        speed = PB_SPEED_INFINITE;

    if (speed < -PB_SPEED_MAX_TIMES)
        speed = -PB_SPEED_MAX_TIMES;

    /* now we need to check for and 'skip' speeds of 0 and -1 */

    /* going slower (can only get there from 1x incrementally) */
    if (speed == 0)
        speed = -2;

    /* going faster (can only get there from -2x incrementally) */
    if (speed == -1)
        speed = 1;

    /* save the new speed */
    pbSpeed = speed;

    /* now compute the new recFrameDelay */

    if (pbSpeed == PB_SPEED_INFINITE)
        recFrameDelay = 0.0;
    else if (pbSpeed > 0)
        recFrameDelay = (1.0 / (real)samplerate) / (real)pbSpeed;
    else if (pbSpeed < 0)
        recFrameDelay = -(real)pbSpeed * (1.0 / (real)samplerate);

    return;
}

void pbDisplayReplayMenu(void)
{
    int lin, col;
    char *c;
    int i;
    char cbuf[MSGMAXLINE];
    static bool FirstTime = true;
    static char sfmt[MSGMAXLINE * 2];
    static char cfmt[MSGMAXLINE * 2];
    static char recordedon[MSGMAXLINE];
    extern char *ConquestVersion;
    extern char *ConquestDate;

    if (FirstTime)
    {
        time_t recon = (time_t)recFileHeader.rectime;

        FirstTime = false;
        sprintf(sfmt,
                "#%d#%%s#%d#: %%s",
                InfoColor,
                GreenColor);

        sprintf(cfmt,
                "#%d#(#%d#%%c#%d#)#%d# - %%s",
                LabelColor,
                InfoColor,
                LabelColor,
                NoColor);

        utStrncpy(recordedon, ctime(&recon), MSGMAXLINE);

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
void pbDisplayReplayHelp(void)
{
    int lin, col, tlin;
    static bool FirstTime = true;
    static char sfmt[MSGMAXLINE * 2];

    if (FirstTime)
    {
        FirstTime = false;
        sprintf(sfmt,
                "#%d#%%-9s#%d#%%s",
                InfoColor,
                NoColor);
    }

    cprintf(1,0,ALIGN_CENTER,"#%d#%s", GreenColor, "WATCH WINDOW COMMANDS");

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

    return;
}

