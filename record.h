/*
 * record.h - recording games in conquest
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef RECORD_H_INCLUDED
#define RECORD_H_INCLUDED

#include "conqdef.h"
#include "conqcom.h"

#define RECVERSION          20060409 /* current recording version */
#define RECVERSION_20031004 20031004 /* older recording version(s) */

/* recording modes */
#define RECMODE_OFF      0
#define RECMODE_STARTING 1
#define RECMODE_ON       2
#define RECMODE_PLAYING  3
#define RECMODE_PAUSED   4

/* file header */
#define SZ_FILEHEADER (256)
#define FHEADER_PAD (SZ_FILEHEADER - (sizeof(uint32_t) +        \
                                      sizeof(uint8_t) +         \
                                      sizeof(uint32_t) +        \
                                      MAXUSERNAME +             \
                                      sizeof(uint32_t) +        \
                                      sizeof(uint8_t) +         \
                                      sizeof(uint32_t)))

#pragma pack(1)
typedef struct _fheader {
    uint32_t vers;                 /* version of this file */
    uint8_t samplerate;            /* recorded at samples per sec */
    uint32_t rectime;		/* time recorded */
    uint8_t user[MAXUSERNAME];	/* user that made recording */
    uint32_t cmnrev;               /* common block rev */
    uint8_t snum;                  /* ship that made rec.  0 == server record */
    uint32_t flags;                /* flags. duh. */
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
void recGenTorpLoc(void);


#endif /* RECORD_H_INCLUDED */
