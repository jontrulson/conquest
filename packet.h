/* 
 * packet routines
 * 
 * Some ideas borrowed from netrek.
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef PACKET_H_INCLUDED
#define PACKET_H_INCLUDED

#include "datatypes.h"
#include "protocol.h"

#define PKT_MAXSIZE     1024	/* no packet should ever be this large. gulp.*/

struct _packetent {
  Unsgn32 pktid;
  Unsgn32 size;
  Unsgn8 *name;
  void (*handler)();
};

void pktNotImpl(void *);	/* a no-show */

#ifndef NOEXTERN
extern struct _packetent clientPackets[];
extern struct _packetent serverPackets[];
#else
struct _packetent clientPackets[] = {
  { CP_NULL, 
    sizeof(cpNull_t), 
    "CP_NULL", 
    pktNotImpl 
  },	/* never used */
  { CP_HELLO, 
    sizeof(cpHello_t), 
    "CP_HELLO", 
    pktNotImpl 
  },
  { CP_ACK, 
    sizeof(cpAck_t), 
    "CP_ACK", 
    pktNotImpl 
  },
  { CP_COMMAND, 
    sizeof(cpCommand_t), 
    "CP_COMMAND", 
    pktNotImpl 
  },
  { CP_FIRETORPS, 
    sizeof(cpFireTorps_t), 
    "CP_FIRETORPS", 
    pktNotImpl 
  },
  { CP_GETSINFO, 
    sizeof(cpGetSInfo_t), 
    "CP_GETSINFO", 
    pktNotImpl 
  },
  { CP_SENDMSG, 
    sizeof(cpSendMsg_t), 
    "CP_SENDMSG", 
    pktNotImpl 
  },
  { CP_SETNAME, 
    sizeof(cpSetName_t), 
    "CP_SETNAME", 
    pktNotImpl 
  },
  { CP_AUTHENTICATE, 
    sizeof(cpAuthenticate_t), 
    "CP_AUTHENTICATE", 
    pktNotImpl 
  },
  { CP_SETCOURSE,
    sizeof(cpSetCourse_t),
    "CP_SETCOURSE",
    pktNotImpl 
  },
  { CP_MESSAGE,
    sizeof(cpMessage_t),
    "CP_MESSAGE",
    pktNotImpl 
  }
};

struct _packetent serverPackets[] = {
  { SP_NULL, 
    sizeof(spNull_t), 
    "SP_NULL", 
    pktNotImpl 
  },	/* never used */
  { SP_HELLO, 
    sizeof(spHello_t), 
    "SP_HELLO", 
    pktNotImpl 
  },
  { SP_ACK, 
    sizeof(spAck_t), 
    "SP_ACK", 
    pktNotImpl 
  },
  { SP_SERVERSTAT, 
    sizeof(spServerStat_t), 
    "SP_SERVERSTAT", 
    pktNotImpl 
  },
  { SP_CLIENTSTAT, 
    sizeof(spClientStat_t), 
    "SP_CLIENTSTAT", 
    pktNotImpl 
  },
  { SP_SHIP, 
    sizeof(spShip_t), 
    "SP_SHIP", 
    pktNotImpl 
  },
  { SP_SHIPSML, 
    sizeof(spShipSml_t), 
    "SP_SHIPSML", 
    pktNotImpl 
  },
  { SP_SHIPLOC, 
    sizeof(spShipLoc_t), 
    "SP_SHIPLOC", 
    pktNotImpl 
  },
  { SP_PLANET, 
    sizeof(spPlanet_t), 
    "SP_PLANET", 
    pktNotImpl },
  { SP_PLANETSML, 
    sizeof(spPlanetSml_t), 
    "SP_PLANETSML", 
    pktNotImpl },
  { SP_PLANETLOC, 
    sizeof(spPlanetLoc_t), 
    "SP_PLANETLOC", 
    pktNotImpl 
  },
  { SP_MESSAGE, 
    sizeof(spMessage_t), 
    "SP_MESSAGE", 
    pktNotImpl 
  },
  { SP_USER, 
    sizeof(spUser_t), 
    "SP_USER", 
    pktNotImpl 
  },
  { SP_TORP, 
    sizeof(spTorp_t), 
    "SP_TORP", 
    pktNotImpl 
  },
  { SP_ACKMSG, 
    sizeof(spAckMsg_t), 
    "SP_ACKMSG", 
    pktNotImpl 
  },
  { SP_TEAM, 
    sizeof(spTeam_t), 
    "SP_TEAM", 
    pktNotImpl 
  },
  { SP_TORPLOC, 
    sizeof(spTorpLoc_t), 
    "SP_TORPLOC", 
    pktNotImpl 
  },
  { SP_CONQINFO, 
    sizeof(spConqInfo_t), 
    "SP_CONQINFO", 
    pktNotImpl 
  },
  { SP_FRAME, 
    sizeof(spFrame_t), 
    "SP_FRAME", 
    pktNotImpl 
  },
  { SP_HISTORY, 
    sizeof(spHistory_t), 
    "SP_HISTORY", 
    pktNotImpl 
  }
};
#endif

#define CLIENTPKTMAX (sizeof(clientPackets) / sizeof(struct _packetent))
#define SERVERPKTMAX (sizeof(serverPackets) / sizeof(struct _packetent))

/* directions from client/server */
#define PKT_TOCLIENT 0
#define PKT_TOSERVER 1
#define PKT_FROMCLIENT 2
#define PKT_FROMSERVER 3

/* error/severity codes for Acks, should make sure these sync to
   psev2String(int psev) */
#define PSEV_INFO     0	
#define PSEV_WARN     1
#define PSEV_ERROR    2
#define PSEV_FATAL    3

/* error codes */
#define PERR_OK           0	/* no error */
#define PERR_UNSPEC       1	/* unspecified */
#define PERR_BADPROTO     2	/* bad protocol */
#define PERR_BADCMN       3	/* common block mismatch */
#define PERR_INVUSER      4 	/* invalid username */
#define PERR_NOUSER       5 	/* no such user */
#define PERR_INVPWD       6 	/* invalid password */
#define PERR_BADPWD       7 	/* wrong password */
#define PERR_CLOSED       8 	/* game closed */
#define PERR_REGISTER     9 	/* register failed */
#define PERR_NOSHIP       10 	/* no slots available */
#define PERR_LOSE         11 	/* lose in menu() */
#define PERR_FLYING       12 	/* already flying a ship (newship()) */
#define PERR_TOOMANYSHIPS 13	/* you are flying too many ships (newship()) */
#define PERR_CANCELED    14	/* an operation (bombing, etc) was canceled
				   for some reason. */
#define PERR_DONE         15    /* finished something - like beaming */

int sendAck(int sock, int dir, Unsgn8 severity, Unsgn8 code, Unsgn8 *msg);
int isConnDead(void);
void pktNotImpl(void *nothing);
char *psev2String(int psev);
int invertDir(int dir);

int waitForPacket(int dir, int sock, int type, Unsgn8 *buf, int blen, 
		  int delay, char *nakmsg);

int clientPktSize(int type);
int serverPktSize(int type);

int isPacketWaiting(int sock);
int writePacket(int direction, int sock, Unsgn8 *packet);
int readPacket(int direction, int sock, Unsgn8 *buf, int len, 
	       unsigned int delay);
int validPkt(int pkttype, void *pkt);

#endif /* PACKET_H_INCLUDED */
