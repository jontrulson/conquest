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

#define RECVERSION 20031004 /* recording version */

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
                                       sizeof(Unsgn8)))
#pragma pack(1)
typedef struct _fheader {	
  Unsgn32 vers;		/* version of this file */
  Unsgn8 samplerate;	/* recorded at samples per sec */
  Unsgn32 rectime;		/* time recorded */
  Unsgn8 user[MAXUSERNAME];	/* user that made recording */
  Unsgn32 cmnrev;               /* common block rev */
  Unsgn8 snum;                  /* ship that made rec.  0 == server record */
  Unsgn8 pad[FHEADER_PAD];	/* padding */
} fileHeader_t;

/* function protos */
int recordOpenInput(char *fname);
void recordCloseInput(void);

int recordOpenOutput(char *fname, int logit);
void recordCloseOutput(void);
int recordInitOutput(int unum, time_t thetime, int snum);

int recordReadHeader(fileHeader_t *fhdr);
int recordReadPkt(Unsgn8 *buf, int blen);

int recordWriteBuf(Unsgn8 *buf, int len);

void recordUpdateFrame(void);
void recordWriteEvent(Unsgn8 *buf);
#endif /* RECORD_H_INCLUDED */