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
#include "conqcom2.h"
#include "conf.h"

#include "record.h"

/* we will have our own copy of the common block here, so setup some stuff */
static char *rcBasePtr = NULL;
static int rcoff = 0;
				/* our own copy */
#define map1d(thevarp, thetype, size) {  \
              thevarp = (thetype *) (rcBasePtr + rcoff); \
              rcoff += (sizeof(thetype) * (size)); \
	}

static rData_t rdata;		/* the input/output record */
static int rdata_wfd;		/* the currently open file for writing */


/* We need our own copies of the varaibles stored in the common block -
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

static int writePacket(int rtype, rData_t *rdat, char *comment);

static int diffShip(Ship_t *ship1, Ship_t *ship2);
static int updateShips(void);

static int diffPlanet(Planet_t *p1, Planet_t *p2);
static int updatePlanets(void);



/* create the recording output file, and alloc space for a cmb copy. */
/*  run under user level privs */
int recordOpenOutput(char *fname)
{
  struct stat sbuf;
  int rv;

  rdata_wfd = -1;

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
	     sys_errlist[errno]);
      return(FALSE);
    }

  /* create our copy of the common block */
  if ((rcBasePtr = malloc(SZ_CMB)) == NULL)
    {
      printf("recordOpenOutput(): memory allocation failed\n");
      close(rdata_wfd);
      rdata_wfd = -1;
      unlink(fname);
      return(FALSE);
    }

  return(TRUE);
}

/* close the output stream */  
void recordCloseOutput(void)
{
  if (rdata_wfd != -1)
    close(rdata_wfd);

  rdata_wfd = -1;
  return;
}



/* setup our copy of the common block, build and generate a file header,
   output a cmb rdata header and an cmb rdata unit. assumes conq privs
   are on */
int recordInit(int unum, time_t thetime)
{
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

  /* now make a file header and write it */
  memset(&fhdr, 0, sizeof(fhdr));
  fhdr.vers = RECVERSION;
  if (sysconf_AllowFastUpdate == TRUE && conf_DoFastUpdate == TRUE)
    fhdr.samplerate = 2;
  else
    fhdr.samplerate = 1;

  fhdr.rectime = thetime;
  strncpy(fhdr.user, Users[unum].username, SIZEUSERNAME - 1);

  write(rdata_wfd, &fhdr, SZ_FILEHEADER);

  /* write a cmb packet */
  rdata.index = 0;
  memcpy(&rdata.data.rcmb, (char *)lCBlockRevision, SZ_CMB);

  if (writePacket(RDATA_CMB, &rdata, "COMMONBLOCK") == FALSE)
    return(FALSE);

  /* ready to go I hope */
  return(TRUE);
}

int recordAddMsg(Msg_t *themsg)
{
  if (CqContext.recmode != RECMODE_ON)
    return(FALSE);              /* bail */

  rdata.index = 0;
  rdata.data.rmsg = *themsg;

  writePacket(RDATA_MESSAGE, &rdata, "MSG");

  return(TRUE);
}

/* check some things and write some packets */
int recordUpdateState(void)
{

  if (CqContext.recmode != RECMODE_ON)
    return(FALSE);		/* bail */
  
  /* write a TS packet */
  rdata.index = 0;		/* no index for these */
  rdata.data.rtime = getnow(NULL, 0);

  writePacket(RDATA_TIME, &rdata, "TIMESTAMP");

  updateShips();

  updatePlanets();

  return(TRUE);
}

/* return the expected size of a packet based on type */
int recordPkt2Sz(int rtype)
{
  int rdsize;

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
      clog("recordPkt2Sz: invalid rtype: %d, returning 0\n",
	   rtype);
      
      rdsize = 0;
      break;
    }

  return(rdsize);
}
  


/* write out an rdata header and it's rdata block */
static int writePacket(int rtype, rData_t *rdat, char *comment)
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
  strcpy(rdatahdr.comment, comment);
  
  clog("writePacket: write header: type %d (%s) %d bytes\n",
       rtype, comment, SZ_RDATAHEADER);

  if (write(rdata_wfd, &rdatahdr, SZ_RDATAHEADER) != SZ_RDATAHEADER)
    {
      clog("writePacket: couldn't write full packet header of %d bytes\n",
	   rdsize);
      return(FALSE);
    }


  clog("writePacket: type %d (%s) %d bytes\n",
       rtype, comment, rdsize);

  /* write data block */
  clog("WP: rdat.index = %d\n", rdat->index);
  if (write(rdata_wfd, rdat, rdsize + SZ_DRHSIZE) != (rdsize + SZ_DRHSIZE))
    {
      /* hmmm */
      clog("writePacket: couldn't write full packet of %d bytes\n",
	   rdsize + SZ_DRHSIZE);
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
	  
	  writePacket(RDATA_SHIP, &rdata, "SHIP");
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

	      writePacket(RDATA_PLANET, &rdata, "PLANET");
	    }
	}
    }

  return(TRUE);
}

