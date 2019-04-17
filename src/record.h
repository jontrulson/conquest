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


#ifndef RECORD_H_INCLUDED
#define RECORD_H_INCLUDED

#include "conqdef.h"
#include "cb.h"

#define RECVERSION          20171108 /* current recording version */
#define RECVERSION_20031004 20031004 /* older recording version(s) */
#define RECVERSION_20060409 20060409

/* recording modes */
typedef enum {
    RECMODE_OFF          = 0,
    RECMODE_STARTING,
    RECMODE_ON,
    RECMODE_PLAYING,
    RECMODE_PAUSED,
} recordMode_t;

/* file header */
#define SZ_FILEHEADER (256)
#define FHEADER_PAD (SZ_FILEHEADER - (sizeof(uint32_t) +        \
                                      sizeof(uint8_t) +         \
                                      sizeof(uint32_t) +        \
                                      MAX_USERNAME +             \
                                      sizeof(uint32_t) +        \
                                      sizeof(uint8_t) +         \
                                      sizeof(uint32_t) +        \
                                      sizeof(uint16_t) +        \
                                      (sizeof(uint32_t) * 7)))


#pragma pack(1)
typedef struct _fheader {
    uint32_t vers;                 /* version of this file */
    uint8_t samplerate;            /* recorded at samples per sec */
    uint32_t rectime;		/* time recorded */
    uint8_t user[MAX_USERNAME];	/* user that made recording */
    uint32_t cmnrev;               /* common block rev */
    uint8_t snum;                  /* ship that made rec.  0 == server
                                    * record (old versions) */
    uint32_t flags;                /* flags. duh. */
    uint16_t protoVers;             /* network protocol version */

    // cbLimits data :)
    uint32_t maxplanets;
    uint32_t maxships;
    uint32_t maxusers;
    uint32_t maxhist;
    uint32_t maxmsgs;
    uint32_t maxtorps;
    uint32_t maxshiptypes;

    uint8_t pad[FHEADER_PAD];	/* padding */
} fileHeader_t;
#pragma pack()

/* recording flags */
#define RECORD_F_NONE           0x00000000
#define RECORD_F_SERVER         0x00000001 /* server recording */

/* playback */
#ifdef NOEXTERN_RECORD
time_t          recTotalElapsed = 0; /* total game time (recorded) */
time_t          recStartTime    = 0;
time_t          recCurrentTime  = 0;
char           *recFilename     = NULL; /* recording filename for playback */
fileHeader_t    recFileHeader   = {};
real            recFrameDelay   = -1.0; /* delaytime between frames */
uint32_t         recFrameCount   = 0;
Msg_t           recMsg          = {}; /* current message */
#else
extern time_t   recTotalElapsed;
extern time_t   recStartTime;
extern time_t   recCurrentTime;
extern char    *recFilename;
extern fileHeader_t recFileHeader;
extern real     recFrameDelay;
extern uint32_t  recFrameCount;
extern Msg_t    recMsg;
#endif


/* function protos */
int  recOpenInput(char *fname);
void recCloseInput(void);

int  recOpenOutput(char *fname, int logit);
void recCloseOutput(void);
int  recInitOutput(int unum, time_t thetime, int snum, int isserver);

int  recReadHeader(fileHeader_t *fhdr);
int  recReadPkt(char *buf, int blen);

int  recWriteBuf(void *buf, int len);

void recUpdateFrame(void);
void recWriteEvent(void *data);

int  recInitReplay(char *fname, time_t *elapsed);


#endif /* RECORD_H_INCLUDED */
