/* 
 * record.c - recording games in conquest
 *
 * $Id$
 *
 * Copyright 1999-2002 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conf.h"

#include "record.h"

#if defined(HAVE_LIBZ) && defined(HAVE_ZLIB_H)
#include <zlib.h>		/* lets try compression. */
#endif

/* we will have our own copy of the common block here, so setup some stuff */
static char *rcBasePtr = NULL;
static int rcoff = 0;
				/* our own copy */
#define map1d(thevarp, thetype, size) {  \
              thevarp = (thetype *) (rcBasePtr + rcoff); \
              rcoff += (sizeof(thetype) * (size)); \
	}

static rData_t rdata;		/* the input/output record */
static int rdata_wfd = -1;	/* the currently open file for writing */
static int rdata_rfd = -1;	/* ... reading */
#ifdef HAVE_LIBZ
static gzFile rdata_wfdz = NULL; /* for compressed files */
static gzFile rdata_rfdz = NULL;
#endif

/* We need our own copies of the variables stored in the common block -
   which is why it's important to keep this code synced up with conqcm.c */

static int *lCBlockRevision;    /* common block rev number */
static ConqInfo_t *lConqInfo;   /* misc game info */
static User_t *lUsers;          /* User statistics. */
static Robot_t *lRobot;         /* Robots. */
static Planet_t *lPlanets;      /* Planets. */
static Team_t *lTeams;          /* Teams. */
static Doomsday_t *lDoomsday;   /* Doomsday machine. */
static History_t *lHistory;     /* History */
static Driver_t *lDriver;       /* Driver. */
static Ship_t *lShips;          /* Ships. */
static ShipType_t *lShipTypes;          /* Ship types. */
static Msg_t *lMsgs;            /* Messages. */
static int *lEndOfCBlock;       /* end of the common block */

static int diffShip(Ship_t *ship1, Ship_t *ship2);
static int updateShips(void);

static int diffPlanet(Planet_t *p1, Planet_t *p2);
static int updatePlanets(void);

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

/* create the recording output file, and alloc space for a cmb copy. */
/* runs under user level privs */
int recordOpenOutput(char *fname)
{
  struct stat sbuf;
  int rv;

  rdata_wfd = -1;
#ifdef HAVE_LIBZ
  rdata_wfdz = NULL;
#endif

  /* check to see if the file exists.  If so, it's an error. */
  if (stat(fname, &sbuf) != -1) 
    {				/* it exists.  issue error and return */
      printf("%s: file exists.  You cannot record to an existing file\n",
	     fname);
      return(FALSE);
    }

  /* now create it */

  if ((rdata_wfd = creat(fname, S_IWUSR|S_IRUSR)) == -1)
    {
      printf("recordOpenOutput(): creat(%s) failed: %s\n",
	     fname,
	     strerror(errno));
      return(FALSE);
    }

#ifdef HAVE_LIBZ
  if ((rdata_wfdz = gzdopen(rdata_wfd, "wb")) == NULL)
    {
      printf("initReplay: gzdopen failed\n");
      return(FALSE);
    }
#endif

  /* create our copy of the common block */
  if ((rcBasePtr = malloc(SZ_CMB)) == NULL)
    {
      printf("recordOpenOutput(): memory allocation failed\n");
#ifdef HAVE_LIBZ
      gzclose(rdata_wfdz);
      rdata_wfdz = NULL;
#else
      close(rdata_wfd);
#endif
      rdata_wfd = -1;
      unlink(fname);
      return(FALSE);
    }

  return(TRUE);
}

/* close the output stream */  
void recordCloseOutput(void)
{

  /* write out a final timestamp */
  rdata.index = 0;		/* no index for these */
  rdata.data.rtime = getnow(NULL, 0);

  recordWritePkt(RDATA_TIME, &rdata, "TIMESTAMP");

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
  /* assumes you've already opened the stream */
  if (rdata_rfd == -1)
    return(FALSE);

#ifdef HAVE_LIBZ
  if (gzread(rdata_rfdz, (char *)fhdr, SZ_FILEHEADER) != SZ_FILEHEADER)
#else
  if (read(rdata_rfd, (char *)fhdr, SZ_FILEHEADER) != SZ_FILEHEADER)
#endif
    {
      printf("recordReadHeader: could not read a proper header\n");
      return(FALSE);
    }

  return(TRUE);
}

/* write a file header */
int recordWriteHeader(fileHeader_t *fhdr)
{
  /* assumes you've already opened the stream */
  if (rdata_wfd == -1)
    return(FALSE);

#ifdef HAVE_LIBZ
  if (gzwrite(rdata_wfdz, (char *)fhdr, SZ_FILEHEADER) != SZ_FILEHEADER)
#else
  if (write(rdata_wfd, (char *)fhdr, SZ_FILEHEADER) != SZ_FILEHEADER)
#endif
    {
      printf("recordWriteHeader: could not write the file header\n");
      return(FALSE);
    }

  return(TRUE);
}

/* setup our copy of the common block, build and generate a file header,
   output a cmb rdata header and an cmb rdata unit. assumes conq privs
   are on */
int recordInit(int unum, time_t thetime)
{
  int i;
  fileHeader_t fhdr;
  rDataHeader_t rdatahdr;
  
  if (rdata_wfd == -1)
    return(FALSE);

  /* the fd is already open, and the cmb allocated. map our local versions */

  map1d(lCBlockRevision, int, 1);        /* this *must* be the first var */
  map1d(lConqInfo, ConqInfo_t, 1)
  map1d(lUsers, User_t, MAXUSERS);
  map1d(lRobot, Robot_t, 1);
  map1d(lPlanets, Planet_t, NUMPLANETS + 1);
  map1d(lTeams, Team_t, NUMALLTEAMS);
  map1d(lDoomsday, Doomsday_t, 1);
  map1d(lHistory, History_t, MAXHISTLOG);
  map1d(lDriver, Driver_t, 1);
  map1d(lShips, Ship_t, MAXSHIPS + 1);
  map1d(lShipTypes, ShipType_t, MAXNUMSHIPTYPES);
  map1d(lMsgs, Msg_t, MAXMESSAGES);
  map1d(lEndOfCBlock, int, 1);

  /* now copy the current common block into our copy */
  memcpy((char *)lCBlockRevision, (char *)CBlockRevision, SZ_CMB);


  /* To be nice, lets loop through all of the user slots,
     and clear out all the passwords for remote users ;-) */
  for (i=0; i<MAXUSERS; i++)
    memset(&lUsers[i].pw, 0, SIZEUSERNAME);

  /* now make a file header and write it */
  memset(&fhdr, 0, sizeof(fhdr));
  fhdr.vers = RECVERSION;
  if (SysConf.AllowFastUpdate == TRUE && UserConf.DoFastUpdate == TRUE)
    fhdr.samplerate = 2;
  else
    fhdr.samplerate = 1;

  fhdr.rectime = thetime;
  strncpy(fhdr.user, Users[unum].username, SIZEUSERNAME - 1);

  if (!recordWriteHeader(&fhdr))
    return(FALSE);

  /* write a cmb packet */
  rdata.index = 0;
  memcpy(&rdata.data.rcmb, (char *)lCBlockRevision, SZ_CMB);

  if (!recordWritePkt(RDATA_CMB, &rdata, "COMMONBLOCK"))
    return(FALSE);

  /* ready to go I hope */
  return(TRUE);
}

int recordAddMsg(Msg_t *themsg)
{
  if (Context.recmode != RECMODE_ON)
    return(FALSE);              /* bail */

  rdata.index = 0;
  rdata.data.rmsg = *themsg;

  recordWritePkt(RDATA_MESSAGE, &rdata, "MSG");

  return(TRUE);
}

/* check some things and write some packets */
int recordUpdateState(void)
{

  if (Context.recmode != RECMODE_ON)
    return(FALSE);		/* bail */
  
  /* write a TS packet */
  rdata.index = 0;		/* no index for these */
  rdata.data.rtime = getnow(NULL, 0);

  recordWritePkt(RDATA_TIME, &rdata, "TIMESTAMP");

  updateShips();

  updatePlanets();

  return(TRUE);
}

/* return the expected size of a packet based on type.  Note:  This function
   adds SZ_DRHSIZE to the returned value (unless read error). */
int recordPkt2Sz(int rtype)
{
  int rdsize = 0;

  switch(rtype)
    {
    case RDATA_CMB:
      rdsize = SZ_CMB;
      break;

    case RDATA_SHIP:
      rdsize = SZ_SHIPD;
      break;

    case RDATA_PLANET:
      rdsize = SZ_PLANETD;
      break;

    case RDATA_MESSAGE:
      rdsize = SZ_MSG;
      break;

    case RDATA_TIME:
      rdsize = SZ_TIME;
      break;

    default:
#ifdef DEBUG_REC
      fprintf(stderr, "recordPkt2Sz: invalid rtype: %d, returning 0\n",
	   rtype);
#endif
      
      rdsize = 0;
      break;
    }

  return((rdsize == 0) ? 0 : (rdsize + SZ_DRHSIZE));
}
  
/* read in a rdata header and it's rdata block, returning the packet type */
int recordReadPkt(rData_t *rdat, char *comment)
{
  rDataHeader_t rdatahdr;
  int rdsize, rv;

  if (rdata_rfd == -1)
    return(FALSE);

  /* first read in the data header */
#ifdef HAVE_LIBZ
  if ((rv = gzread(rdata_rfdz, (char *)&rdatahdr, SZ_RDATAHEADER)) !=
      SZ_RDATAHEADER)
#else
  if ((rv = read(rdata_rfd, (char *)&rdatahdr, SZ_RDATAHEADER)) !=
      SZ_RDATAHEADER)
#endif
    {
#ifdef DEBUG_REC
      fprintf(stderr, 
	      "recordReadPkt: could not read data header, returned %d\n",
	      rv);
#endif

      return(RDATA_NONE);
    }

  if (!(rdsize = recordPkt2Sz(rdatahdr.type)))
    return(RDATA_NONE);
  
  /* if comment is non-NULL, memcpy the comment string in */
  if (comment)
    memcpy(comment, rdatahdr.comment, SZ_RDATAHDR_COMMENT);

  /* so now read in the data pkt */
#ifdef HAVE_LIBZ
  if ((rv = gzread(rdata_rfdz, (char *)rdat, rdsize)) !=
      rdsize)
#else
  if ((rv = read(rdata_rfd, (char *)rdat, rdsize)) !=
      rdsize)
#endif
    {
#ifdef DEBUG_REC
      fprintf(stderr, 
	      "recordReadPkt: could not read data packet, returned %d\n",
	      rv);
#endif
      
      return(RDATA_NONE);
    }

#ifdef DEBUG_REC
  fprintf(stderr, "recordReadPkt: rdatahdr.type = %d, rdata.index = %d\n",
          rdatahdr.type, rdat->index);
#endif

  return(rdatahdr.type);
}


/* write out an rdata header and it's rdata block */
int recordWritePkt(int rtype, rData_t *rdat, char *comment)
{				/* kind of a misnomer as we are actually
				   writing 2 'packets' - the header and
				   the data */
  rDataHeader_t rdatahdr;
  int rdsize, rv;

  if (rdata_wfd == -1)
    return(FALSE);

  /* invalid size? */
  if (!(rdsize = recordPkt2Sz(rtype)))
    return(FALSE);

  /* write data header */
  memset(&rdatahdr, 0, SZ_RDATAHEADER);
  rdatahdr.type = rtype;
  strncpy(rdatahdr.comment, comment, SZ_RDATAHDR_COMMENT - 1);
  
#ifdef DEBUG_REC
  clog("recordWritePkt: write header: type %d (%s) %d bytes\n",
       rtype, comment, SZ_RDATAHEADER);
#endif

#ifdef HAVE_LIBZ
  if (gzwrite(rdata_wfdz, &rdatahdr, SZ_RDATAHEADER) != SZ_RDATAHEADER)
#else
  if (write(rdata_wfd, &rdatahdr, SZ_RDATAHEADER) != SZ_RDATAHEADER)
#endif
    {
      clog("recordWritePkt: couldn't write full packet header of %d bytes\n",
	   rdsize);
      return(FALSE);
    }


  /* write data block */
#ifdef HAVE_LIBZ
  if (gzwrite(rdata_wfdz, rdat, rdsize) != rdsize)
#else
  if (write(rdata_wfd, rdat, rdsize) != rdsize)
#endif
    {
      /* hmmm */
      clog("recordWritePkt: couldn't write full packet of %d bytes\n",
	   rdsize);
      return(FALSE);
    }

  return(TRUE);
}

static int diffShip(Ship_t *ship1, Ship_t *ship2)
{
  /* for now we will just memcmp them */

  if (memcmp(ship1, ship2, SZ_SHIPD) == 0)
    return(FALSE);
  else
    return(TRUE);
}

/* loop through the ships, writing update packets for any that have changed */
static int updateShips(void)
{
  int i;

  for (i=1; i<=MAXSHIPS; i++)
    {
      if (diffShip(&Ships[i], &lShips[i]))
	{
	  /* need to write a packet */
	  
	  /* update our local common block */
	  lShips[i] = Ships[i];
	  
	  /* now write the puppy */
	  
	  rdata.index = i;
	  rdata.data.rship = lShips[i];
	  
	  recordWritePkt(RDATA_SHIP, &rdata, "SHIP");
	}
    }

  return(TRUE);
}

static int diffPlanet(Planet_t *p1, Planet_t *p2)
{
  /* for now we will just memcmp them */

  if (memcmp(p1, p2, SZ_PLANETD) == 0)
    return(FALSE);
  else
    return(TRUE);
}

/* loop through the planets, writing update packets for any that have
   changed */
static int updatePlanets(void)
{
  int i;

  for (i=1; i<=NUMPLANETS; i++)
    {
				/* we are only interested in real ones */
      if (Planets[i].real)
	{			/* see if it's different than our copy */
	  if (diffPlanet(&Planets[i], &lPlanets[i]))
	    {
	      /* need to write a packet */
	      
	      /* update our local common block */
	      lPlanets[i] = Planets[i];

	      /* now write the puppy */

	      rdata.index = i;
	      rdata.data.rplanet = lPlanets[i];

	      recordWritePkt(RDATA_PLANET, &rdata, "PLANET");
	    }
	}
    }

  return(TRUE);
}

