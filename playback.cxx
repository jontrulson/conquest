/*
 * recordclnt.c - client playback
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

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
    Context.display = false; /* don't display things while looking */

    while (recCurrentTime < newtime)
        if ((pbProcessPackets() == SP_NULL))
            break;		/* no more data */

    Context.display = true;

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
