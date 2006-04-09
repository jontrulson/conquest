/* 
 * record.h - recording games in conquest
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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
#define FHEADER_PAD (SZ_FILEHEADER - (sizeof(Unsgn32) + \
                                       sizeof(Unsgn8) + \
                                       sizeof(Unsgn32) + \
                                       MAXUSERNAME + \
                                       sizeof(Unsgn32) + \
                                       sizeof(Unsgn8) + \
                                       sizeof(Unsgn32)))
#pragma pack(1)
typedef struct _fheader {	
  Unsgn32 vers;                 /* version of this file */
  Unsgn8 samplerate;            /* recorded at samples per sec */
  Unsgn32 rectime;		/* time recorded */
  Unsgn8 user[MAXUSERNAME];	/* user that made recording */
  Unsgn32 cmnrev;               /* common block rev */
  Unsgn8 snum;                  /* ship that made rec.  0 == server record */
  Unsgn32 flags;                /* flags. duh. */
  Unsgn8 pad[FHEADER_PAD];	/* padding */
} fileHeader_t;
#pragma pack()

/* recording flags */
#define RECORD_F_NONE           0x00000000
#define RECORD_F_SERVER         0x00000001 /* server recording */

/* playback */
#ifdef REC_NOEXTERN
time_t totElapsed = 0;          /* total game time (recorded) */
time_t startTime = 0;
time_t currTime = 0;
char *rfname = NULL;        /* name of recording file for playback */
fileHeader_t fhdr;
real frameDelay = -1.0;         /* delaytime between frames */
Unsgn32 frameCount = 0;
Msg_t recMsg = {};
#else
extern time_t totElapsed;          /* total game time (recorded) */
extern time_t startTime;
extern time_t currTime;
extern char *rfname;        /* name of recording file for playback */
extern fileHeader_t fhdr;
extern real frameDelay;         /* delaytime between frames */
extern Unsgn32 frameCount;
extern Msg_t recMsg;            /* current recMsg */
#endif


/* function protos */
int recordOpenInput(char *fname);
void recordCloseInput(void);

int recordOpenOutput(char *fname, int logit);
void recordCloseOutput(void);
int recordInitOutput(int unum, time_t thetime, int snum, int isserver);

int recordReadHeader(fileHeader_t *fhdr);
int recordReadPkt(Unsgn8 *buf, int blen);

int recordWriteBuf(Unsgn8 *buf, int len);

void recordUpdateFrame(void);
void recordWriteEvent(Unsgn8 *buf);

int initReplay(char *fname, time_t *elapsed);
void recordGenTorpLoc(void);


#endif /* RECORD_H_INCLUDED */
