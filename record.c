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
#include "protocol.h"
#include "packet.h"

#include "protocol.h"
#include "client.h"

#define REC_NOEXTERN
#include "record.h"
#undef REC_NOEXTERN

extern char *ConquestVersion;
extern char *ConquestDate;


#if defined(HAVE_LIBZ) && defined(HAVE_ZLIB_H)
#include <zlib.h>		/* lets try compression. */
#endif

static int rdata_wfd = -1;	/* the currently open file for writing */
static int rdata_rfd = -1;	/* ... reading */

static Unsgn32 recordFrameCount = 0;

#ifdef HAVE_LIBZ
static gzFile rdata_wfdz = NULL; /* for compressed files */
static gzFile rdata_rfdz = NULL;
#endif

/* open a recording input file */
int recordOpenInput(char *fname)
{
  rdata_rfd = -1;

  if ((rdata_rfd = open(fname, O_RDONLY)) == -1)
    {
      printf("recordOpenInput: open(%s) failed: %s\n", fname, 
	     strerror(errno));
      return(FALSE);
    }
  
#ifdef HAVE_LIBZ
  if ((rdata_rfdz = gzdopen(rdata_rfd, "rb")) == NULL)
    {
      printf("recordOpenInput: gzdopen failed\n"); /* we use printf here
						 since clog maynot be
						 available */
      return(FALSE);
    }
#endif

  return(TRUE);
}

void recordCloseInput(void)
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
int recordOpenOutput(char *fname, int logit)
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
        clog("%s: file exists.  You cannot record to an existing file\n",
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
        clog("recordOpenOutput(): creat(%s) failed: %s\n",
               fname,
               strerror(errno));
      else
        printf("recordOpenOutput(): creat(%s) failed: %s\n",
               fname,
               strerror(errno));
      return(FALSE);
    }

  chmod(fname, (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH));

#ifdef HAVE_LIBZ
  if ((rdata_wfdz = gzdopen(rdata_wfd, "wb")) == NULL)
    {
      if (logit)
        clog("initReplay: gzdopen failed\n");
      else
        printf("initReplay: gzdopen failed\n");
      return(FALSE);
    }
#endif

  return(TRUE);
}

/* close the output stream */  
void recordCloseOutput(void)
{
  recordUpdateFrame();

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
int recordReadHeader(fileHeader_t *fhdr)
{
  int rv;
  /* assumes you've already opened the stream */
  if (rdata_rfd == -1)
    return(FALSE);

#ifdef HAVE_LIBZ
  if ((rv = gzread(rdata_rfdz, (char *)fhdr, SZ_FILEHEADER)) != SZ_FILEHEADER)
#else
  if ((rv = read(rdata_rfd, (char *)fhdr, SZ_FILEHEADER)) != SZ_FILEHEADER)
#endif
    {
      printf("recordReadHeader: could not read a proper header\n");
      return(FALSE);
    }

#ifdef DEBUG_REC
  clog("recordReadHeader: read %d bytes\n",
       rv);
#endif

  /* now endianize the data */

  fhdr->vers = (Unsgn32)ntohl(fhdr->vers);
  fhdr->rectime = (Unsgn32)ntohl(fhdr->rectime);
  fhdr->cmnrev = (Unsgn32)ntohl(fhdr->cmnrev);

#ifdef DEBUG_REC
  clog("recordReadHeader: vers = %d, rectime = %d, cmnrev = %d\n",
       fhdr->vers, fhdr->rectime, fhdr->cmnrev);
#endif


  return(TRUE);
}

/* build and generate a file header
 */
int recordInitOutput(int unum, time_t thetime, int snum)
{
  fileHeader_t fhdr;
  
  if (rdata_wfd == -1)
    return(FALSE);

  recordFrameCount = 0;

  /* now make a file header and write it */
  memset(&fhdr, 0, sizeof(fhdr));
  fhdr.vers = (Unsgn32)htonl(RECVERSION);

  fhdr.samplerate = (Unsgn8)Context.updsec;

  fhdr.rectime = (Unsgn32)htonl((Unsgn32)thetime);
  strncpy(fhdr.user, Users[unum].username, MAXUSERNAME - 1);

  fhdr.cmnrev = (Unsgn32)htonl((Unsgn32)COMMONSTAMP);
  fhdr.snum = snum;

  if (!recordWriteBuf((Unsgn8 *)&fhdr, sizeof(fileHeader_t)))
    return(FALSE);

  /* add a frame packet */
  recordUpdateFrame();

  /* ready to go I hope */
  return(TRUE);
}

/* note, if we get a write error here, we turn off recording */
void recordWriteEvent(Unsgn8 *buf)
{
  int pkttype;
  int len;

  if (Context.recmode != RECMODE_ON)
    return;

  if (!buf)
    return;

  pkttype = (int)buf[0];
  
  len = serverPktSize(pkttype);
  if (!len)
    {
      clog("recordWriteEvent: invalid packet type %d", pkttype);
      return;
    }

  if (!recordWriteBuf(buf, len))
    {
      clog("recordWriteEvent: write error: %s, recording terminated",
           strerror(errno));
      Context.recmode = RECMODE_OFF;
    }

  return;
}

/* write a frame packet and increment recordFrameCount */
void recordUpdateFrame(void)
{
  spFrame_t frame;

  if (Context.recmode != RECMODE_ON)
    return;		/* bail */
  
  memset((void *)&frame, 0, sizeof(spFrame_t));

  frame.type = SP_FRAME;
  frame.frame = (Unsgn32)htonl(recordFrameCount);
  frame.time = (Unsgn32)htonl((Unsgn32)getnow(NULL, 0));

  recordWriteEvent((Unsgn8 *)&frame);

  recordFrameCount++;

  return;
}

/* write out a buffer */
int recordWriteBuf(Unsgn8 *buf, int len)
{
  if (rdata_wfd == -1)
    return(FALSE);
  
#ifdef DEBUG_REC
  clog("recordWriteBuf: len = %d\n", len);
#endif

#ifdef HAVE_LIBZ
  if (gzwrite(rdata_wfdz, buf, len) != len)
#else
  if (write(rdata_wfd, buf, len) != len)
#endif
    {
      clog("recordWriteBuf: couldn't write buffer of %d bytes\n",
	   len);
      return(FALSE);
    }

  return(TRUE);
}


/* read in a packet, returning the packet type */
int recordReadPkt(Unsgn8 *buf, int blen)
{
  int len, rv;
  int pkttype;

  if (rdata_rfd == -1)
    return(SP_NULL);

  if (!buf || !blen)
    return(SP_NULL);

  /* first read in the first byte to get the packet type */
#ifdef HAVE_LIBZ
  if ((rv = gzread(rdata_rfdz, (char *)buf, 1)) != 1)
#else
  if ((rv = read(rdata_rfd, (char *)buf, 1)) != 1)
#endif
    {
#ifdef DEBUG_REC
      clog("recordReadPkt: could not read pkt type, returned %d\n",
              rv);
#endif

      return(SP_NULL);
    }

  pkttype = (int)buf[0];

  len = serverPktSize(pkttype);

  if (blen < len)
    {
      fprintf(stderr,
              "recordReadPkt: buffer too small. got %d, need %d\n",
              len, blen);
      return(SP_NULL);
    }
    

  if (!len)
    {
      clog("recordReadPkt: invalid packet %d\n",
              pkttype);
      fprintf(stderr,
              "recordReadPkt: invalid packet %d\n",
              pkttype);
      return(SP_NULL);
    }
  else
    {
      len = len - sizeof(Unsgn8);

  /* so now read in the rest of the packet */
#ifdef HAVE_LIBZ
      if ((rv = gzread(rdata_rfdz, (char *)(buf + 1), len)) != len)
#else
      if ((rv = read(rdata_rfd, (char *)(buf + 1), len)) != len )
#endif
      {
#ifdef DEBUG_REC
         fprintf(stderr, 
	         "recordReadPkt: could not read data packet, returned %d\n",
	         rv);
#endif
      
         return(SP_NULL);
       }

    }

#ifdef DEBUG_REC
     clog("recordReadPkt: read pkttype  = %d\n",
          pkttype);
#endif

  return(pkttype);
}

/* open, create/load our cmb, and get ready for action if elapsed == NULL
   otherwise, we read the entire file to determine the elapsed time of
   the game and return it */
int initReplay(char *fname, time_t *elapsed)
{
  int pkttype;
  time_t starttm = 0;
  time_t curTS = 0;
  Unsgn8 buf[PKT_MAXSIZE];

  if (!recordOpenInput(fname))
    {
      printf("initReplay: recordOpenInput(%s) failed\n", fname);
      return(FALSE);
    }

  /* don't bother mapping for just a count */
  if (!elapsed)
    map_lcommon();

  /* now lets read in the file header and check a few things. */

  if (!recordReadHeader(&fhdr))
    return(FALSE);
      
  if (fhdr.vers != RECVERSION)
    {				/* wrong vers */
      clog("initReplay: version mismatch.  got %d, need %d\n",
           fhdr.vers,
           RECVERSION);
      printf("initReplay: version mismatch.  got %d, need %d\n",
	     fhdr.vers,
	     RECVERSION);
      return FALSE;
    }


  if ( fhdr.cmnrev != COMMONSTAMP )
    {
      clog("initReplay: CONQUEST COMMON BLOCK MISMATCH %d != %d",
             fhdr.cmnrev, COMMONSTAMP );
      printf("initReplay: CONQUEST COMMON BLOCK MISMATCH %d != %d",
             fhdr.cmnrev, COMMONSTAMP );
      return FALSE;
    }

  /* if we are looking for the elapsed time, scan the whole file
     looking for timestamps. */
  if (elapsed)			/* we want elapsed time */
    {
      int done = FALSE;

      starttm = fhdr.rectime;

      curTS = 0;
      /* read through the entire file, looking for timestamps. */
      
#if defined(DEBUG_REC)
      clog("conqreplay: initReplay: reading elapsed time");
#endif

      while (!done)
	{
          if ((pkttype = recordReadPkt(buf, PKT_MAXSIZE)) == SP_FRAME)
            {
              spFrame_t *frame = (spFrame_t *)buf;
              
              /* fix up the endianizational interface for the time */
              curTS = (time_t)ntohl(frame->time);
            }

	  if (pkttype == SP_NULL)
	    done = TRUE;	/* we're done */
	}

      if (curTS != 0)
	*elapsed = (curTS - starttm);
      else
	*elapsed = 0;

      /* now close the file so that the next call of initReplay can
	 get a fresh start. */
      recordCloseInput();
    }

  /* now we are ready to start running packets */
  
  return(TRUE);
}


