/*
 * record.c - recording games in conquest
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "conqlb.h"
#include "conqutil.h"

#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"

#include "protocol.h"
#include "packet.h"

#define NOEXTERN_RECORD
#include "record.h"
#undef NOEXTERN_RECORD

#include "playback.h"

extern char *ConquestVersion;
extern char *ConquestDate;


#if defined(HAVE_LIBZ) && defined(HAVE_ZLIB_H)
#include <zlib.h>		/* lets try compression. */
#endif

static int rdata_wfd = -1;	/* the currently open file for writing */
static int rdata_rfd = -1;	/* ... reading */

static uint32_t recordFrameCount = 0;

#ifdef HAVE_LIBZ
static gzFile rdata_wfdz = NULL; /* for compressed files */
static gzFile rdata_rfdz = NULL;
#endif

/* open a recording input file */
int recOpenInput(char *fname)
{
    rdata_rfd = -1;

    /* mingw */
#if !defined(O_BINARY)
#define O_BINARY (0)
#endif

    if ((rdata_rfd = open(fname, O_RDONLY | O_BINARY)) == -1)
    {
        printf("recOpenInput: open(%s) failed: %s\n", fname,
               strerror(errno));
        return(FALSE);
    }

#ifdef HAVE_LIBZ
    if ((rdata_rfdz = gzdopen(rdata_rfd, "rb")) == NULL)
    {
        printf("recOpenInput: gzdopen failed\n"); /* we use printf here
                                                     since utLog maynot be
                                                     available */
        return(FALSE);
    }
#endif

    return(TRUE);
}

void recCloseInput(void)
{
#ifdef HAVE_LIBZ
    if (rdata_rfdz != NULL)
        gzclose(rdata_rfdz);

    rdata_rfdz = NULL;
#else
    if (rdata_rfd != -1)
        close(rdata_rfd);
#endif

    rdata_rfd = -1;

    return;
}

/* create the recording output file. */
/* runs under user level privs */
int recOpenOutput(char *fname, int logit)
{
    struct stat sbuf;

    rdata_wfd = -1;
#ifdef HAVE_LIBZ
    rdata_wfdz = NULL;
#endif

    /* check to see if the file exists.  If so, it's an error. */
    if (stat(fname, &sbuf) != -1)
    {				/* it exists.  issue error and return */
        if (logit)
            utLog("%s: file exists.  You cannot record to an existing file\n",
                  fname);
        else
            printf("%s: file exists.  You cannot record to an existing file\n",
                   fname);
        return(FALSE);
    }

    /* now create it */

    if ((rdata_wfd = creat(fname, S_IWUSR|S_IRUSR)) == -1)
    {
        if (logit)
            utLog("recOpenOutput(): creat(%s) failed: %s\n",
                  fname,
                  strerror(errno));
        else
            printf("recOpenOutput(): creat(%s) failed: %s\n",
                   fname,
                   strerror(errno));
        return(FALSE);
    }

#if !defined(MINGW)
    chmod(fname, (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH));
#endif

#ifdef HAVE_LIBZ
    if ((rdata_wfdz = gzdopen(rdata_wfd, "wb")) == NULL)
    {
        if (logit)
            utLog("recInitReplay: gzdopen failed\n");
        else
            printf("recInitReplay: gzdopen failed\n");
        return(FALSE);
    }
#endif

    return(TRUE);
}

/* close the output stream */
void recCloseOutput(void)
{
    recUpdateFrame();

#ifdef HAVE_LIBZ
    if (rdata_wfdz != NULL)
        gzclose(rdata_wfdz);

    rdata_wfdz = NULL;
#else
    if (rdata_wfd != -1)
        close(rdata_wfd);
#endif

    rdata_wfd = -1;

    return;
}

/* read the file header */
int recReadHeader(fileHeader_t *fhdr)
{
    int rv;

    memset(fhdr, 0, sizeof(fileHeader_t));

    /* assumes you've already opened the stream */

    if (rdata_rfd == -1)
        return(FALSE);

#ifdef HAVE_LIBZ
    if ((rv = gzread(rdata_rfdz, (char *)fhdr, SZ_FILEHEADER)) != SZ_FILEHEADER)
#else
        if ((rv = read(rdata_rfd, (char *)fhdr, SZ_FILEHEADER)) != SZ_FILEHEADER)
#endif
        {
            printf("recReadHeader: could not read a proper header\n");
            return(FALSE);
        }

#ifdef DEBUG_REC
    utLog("recReadHeader: read %d bytes\n",
          rv);
#endif

#if !defined(HAVE_LIBZ)
    /* we will look at the first two bytes, and see if it's gzip format */
    {
        unsigned char *cptr = (unsigned char *)fhdr;

        if (cptr[0] == GZIPPED_BYTE_1 && cptr[1] == GZIPPED_BYTE_2)
        {
            /* it is, tell the user to gunzip it */
            fprintf(stderr,
                    "CQR file is gzip compressed, but gzip support is not\n");
            fprintf(stderr,
                    "compiled in.  Please decompress the file with gunzip first.\n");

            return(FALSE);
        }
    }
#endif  /* !HAVE_LIBZ */


    /* now de-endianize the data */

    fhdr->vers = (uint32_t)ntohl(fhdr->vers);
    fhdr->rectime = (uint32_t)ntohl(fhdr->rectime);
    fhdr->cmnrev = (uint32_t)ntohl(fhdr->cmnrev);
    fhdr->flags = (uint32_t)ntohl(fhdr->flags);
    fhdr->protoVers = (uint16_t)ntohs(fhdr->protoVers);

#if defined(DEBUG_REC)
    utLog("recReadHeader: vers = %d, rectime = %d, cmnrev = %d\n",
          fhdr->vers, fhdr->rectime, fhdr->cmnrev);
#endif


    return(TRUE);
}

/* build and generate a file header
 */
int recInitOutput(int unum, time_t thetime, int snum, int isserver)
{
    fileHeader_t fhdr;

    if (rdata_wfd == -1)
        return(FALSE);

    recordFrameCount = 0;

    /* now make a file header and write it */
    memset(&fhdr, 0, sizeof(fhdr));

    /* set all neccesary flags here, before endianizing below */
    if (isserver)                 /* this is a server recording */
        fhdr.flags |= RECORD_F_SERVER;

    fhdr.vers = (uint32_t)htonl(RECVERSION);

    fhdr.samplerate = (uint8_t)Context.updsec;

    fhdr.rectime = (uint32_t)htonl((uint32_t)thetime);
    strncpy((char*)fhdr.user, Users[unum].username, MAXUSERNAME - 1);

    fhdr.cmnrev = (uint32_t)htonl((uint32_t)COMMONSTAMP);
    fhdr.snum = snum;
    fhdr.flags = (uint32_t)htonl((uint32_t)fhdr.flags);
    fhdr.protoVers = (uint32_t)htons((uint16_t)PROTOCOL_VERSION);


    if (!recWriteBuf(&fhdr, sizeof(fileHeader_t)))
        return(FALSE);

    /* add a frame packet */
    recUpdateFrame();

    /* ready to go I hope */
    return(TRUE);
}

/* note, if we get a write error here, we turn off recording */
void recWriteEvent(void *data)
{
   char *buf = (char *)data;
    uint8_t pkttype;
    int len;

    if (Context.recmode != RECMODE_ON)
        return;

    if (!buf)
        return;

    pkttype = (uint8_t)*buf;

    len = pktServerPacketSize(pkttype);
    if (!len)
    {
        utLog("recWriteEvent: invalid packet type %d", pkttype);
        return;
    }

    if (!recWriteBuf(buf, len))
    {
        utLog("recWriteEvent: write error: %s, recording terminated",
              strerror(errno));
        Context.recmode = RECMODE_OFF;
    }

    return;
}

/* write a frame packet and increment recordFrameCount */
void recUpdateFrame(void)
{
    spFrame_t frame;

    if (Context.recmode != RECMODE_ON)
        return;		/* bail */

    memset((void *)&frame, 0, sizeof(spFrame_t));

    frame.type = SP_FRAME;
    frame.frame = (uint32_t)htonl(recordFrameCount);
    frame.time = (uint32_t)htonl((uint32_t)time(0));

    recWriteEvent(&frame);

    recordFrameCount++;

    return;
}

/* write out a buffer */
int recWriteBuf(void *buf, int len)
{
    if (rdata_wfd == -1)
        return(FALSE);

#ifdef DEBUG_REC
    utLog("recWriteBuf: len = %d\n", len);
#endif

#ifdef HAVE_LIBZ
    if (gzwrite(rdata_wfdz, buf, len) != len)
#else
        if (write(rdata_wfd, buf, len) != len)
#endif
        {
            utLog("recWriteBuf: couldn't write buffer of %d bytes\n",
                  len);
            return(FALSE);
        }

    return(TRUE);
}


/* read in a packet, returning the packet type */
int recReadPkt(char *buf, int blen)
{
    int len, rv;
    int pkttype;

    if (rdata_rfd == -1)
    {
        return(SP_NULL);
    }

    if (!buf || !blen)
    {
        return(SP_NULL);
    }

    /* first read in the first byte to get the packet type */
#ifdef HAVE_LIBZ
    if ((rv = gzread(rdata_rfdz, (char *)buf, 1)) != 1)
#else
        if ((rv = read(rdata_rfd, (char *)buf, 1)) != 1)
#endif
        {
#ifdef DEBUG_REC
            utLog("%s: could not read pkt type, returned (rv = %d)\n",
                  __FUNCTION__, rv);
#endif

            return(SP_NULL);
        }

    pkttype = (int)buf[0];

    len = pktServerPacketSize(pkttype);

    if (blen < len)
    {
        fprintf(stderr,
                "recReadPkt: buffer too small. got %d, need %d\n",
                len, blen);
        return(SP_NULL);
    }


    if (!len)
    {
        utLog("recReadPkt: invalid packet %d\n",
              pkttype);
        fprintf(stderr,
                "recReadPkt: invalid packet %d\n",
                pkttype);
        return(SP_NULL);
    }
    else
    {
        len = len - sizeof(uint8_t);

        /* so now read in the rest of the packet */
#ifdef HAVE_LIBZ
        if ((rv = gzread(rdata_rfdz, (char *)(buf + 1), len)) != len)
#else
            if ((rv = read(rdata_rfd, (char *)(buf + 1), len)) != len )
#endif
            {
#ifdef DEBUG_REC
                fprintf(stderr,
                        "%s: could not read data packet, returned %d\n",
                        __FUNCTION__, rv);
                utLog("%s: could not read data packet, returned %d",
                      __FUNCTION__, rv);
#endif

                return(SP_NULL);
            }

    }

#ifdef DEBUG_REC
    utLog("%s: read pkttype  = %d", __FUNCTION__, pkttype);
#endif

    return(pkttype);
}

/* generate torploc packets for client recording purposes
   no more than once every ITER_SECONDS */
void recGenTorpLoc(void)
{
    int i, j;
    int snum = Context.snum;
    int team = Ships[snum].team;
    spTorpLoc_t storploc;
    static spTorpLoc_t pktTorpLoc[MAXSHIPS + 1][MAXTORPS] = {};
    real dis;
    real x, y;
    static uint32_t iterstart = 0;
    uint32_t iternow = clbGetMillis();
    const uint32_t iterwait = 100.0; /* ms */
    real tdelta = (real)iternow - (real)iterstart;

    if (Context.recmode != RECMODE_ON)
        return;                     /* not recording, no point */

    if (tdelta < iterwait)
        return;                     /* not yet time */

    iterstart = iternow;

    for (i=1; i<=MAXSHIPS; i++)
    {
        if ( Ships[i].status != SS_OFF )
        {
            for ( j = 0; j < MAXTORPS; j = j + 1 )
            {
                if ( Ships[i].torps[j].status == TS_LIVE )
                {
                    memset((void *)&storploc, 0, sizeof(spTorpLoc_t));
                    storploc.type = SP_TORPLOC;
                    storploc.snum = i;
                    storploc.tnum = j;

                    x = Ships[i].torps[j].x;
                    y = Ships[i].torps[j].y;

                    if (Ships[i].torps[j].war[team])
                    { /* it's at war with us. bastards. */
                      /* see if it's close enough to scan */
                        dis = (real) dist(Ships[snum].x, Ships[snum].y,
                                          Ships[i].torps[j].x,
                                          Ships[i].torps[j].y );

                        if (dis > ACCINFO_DIST)
                        {                       /* in the bermuda triangle */
                            x = 1e7;
                            y = 1e7;
                        }
                    }

                    storploc.x = (int32_t)htonl((int32_t)(x * 1000.0));
                    storploc.y = (int32_t)htonl((int32_t)(y * 1000.0));

                    /* only send 'war' status as it relates to our team */
                    if (Ships[i].torps[j].war[team])
                        storploc.war |= (1 << team);


                    if (memcmp((void *)&storploc,
                               (void *)&(pktTorpLoc[i][j]),
                               sizeof(spTorpLoc_t)))
                    {
                        pktTorpLoc[i][j] = storploc;
                        recWriteEvent(&storploc);
                    }
                }
            }
        }
    }

    return;
}
