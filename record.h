/* 
 * record.h - recording games in conquest
 *
 * $Id$
 *
 * Copyright 1999-2002 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef RECORD_H_INCLUDED
#define RECORD_H_INCLUDED

#include "conqdef.h"
#include "conqcom.h"

#define RECVERSION 200112311 /* recording version */

				/* recording modes */
#define RECMODE_OFF      0
#define RECMODE_STARTING 1
#define RECMODE_ON       2
#define RECMODE_PLAYING  3

				/* recdata types (_rheader) */
#define RDATA_NONE      0
#define RDATA_TIME      1
#define RDATA_PLANET    2
#define RDATA_SHIP      3
#define RDATA_MESSAGE   4
#define RDATA_CMB       5

				/* file header */
#define SZ_FILEHEADER (256)
#define FHEADER_PAD (SZ_FILEHEADER - (sizeof(unsigned int) + \
                                       sizeof(unsigned short) + \
                                       sizeof(time_t) + \
                                       SIZEUSERNAME))
typedef struct _fheader {	
  unsigned int vers;		/* verison of this file */
  unsigned short samplerate;	/* 1 or 2 per sec updates */
  time_t rectime;		/* time recorded */
  char user[SIZEUSERNAME];	/* user that made recording */
  char pad[FHEADER_PAD];	/* padding */
} fileHeader_t;

				/* rdata header */
typedef struct _rdata_hdr {	
  int type;			/* RDATA_* - type of data following */
  char comment[24];		/* anything you want */
} rDataHeader_t;

				/* now for the size of things */
#define SZ_RDATAHEADER	(sizeof(rDataHeader_t))

#define SZ_SHIPD        (sizeof(Ship_t))
#define SZ_PLANETD      (sizeof(Planet_t))
#define SZ_MSG          (sizeof(Msg_t))
#define SZ_CMB          (SIZEOF_COMMONBLOCK)
#define SZ_TIME         (sizeof(time_t))

#define SZ_DRHSIZE      (sizeof(int))
typedef struct _rdatarec {
  int index;			/* index for the object, if applicable */
  union {
    char rcmb[SZ_CMB];
    Ship_t rship;
    Planet_t rplanet;
    time_t rtime;
    Msg_t rmsg;
  } data;
} rData_t;

/* function protos */
int recordOpenOutput(char *fname);
void recordCloseOutput(void);
int recordInit(int unum, time_t thetime);
int recordUpdateState(void);
int recordPkt2Sz(int type);

#endif /* RECORD_H_INCLUDED */
