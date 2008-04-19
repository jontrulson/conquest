/* 
 * Per client (conquest) info
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include "conqdef.h"
#include "datatypes.h"
#include "conqnet.h"
#include "protocol.h"
#include "packet.h"

#ifdef CLIENT_NOEXTERN
# define EXTERNAL 
#else
# define EXTERNAL extern
#endif

#define CLT_STATE_NONE      0x00000000	
#define CLT_STATE_PREINIT   0x00000001	/* local init */
#define CLT_STATE_INIT      0x00000002	/* negotiate with client/login */
#define CLT_STATE_MAINMENU  0x00000004	/* in main menu */
#define CLT_STATE_PLAYMENU  0x00000008	/* a menu/info screen engaged
					   during play */
#define CLT_STATE_PLAY      0x00000010	/* playing */
#define CLT_STATE_TERMINATE 0x00000020	/* terminating */

typedef struct {
  int sock;			/* socket to client */
  int usock;			/* UDP socket to client */
  int doUDP;                    /* whether we should look for udp packets */
  int tryUDP;                   /* should we try UDP? */
  struct sockaddr_in  servaddr;
  Unsgn32 state;		/* current state */
  Unsgn32 serverDead;		/* is the client dead/ */
  Unsgn32 isLoggedIn;		/* is the client logged in? */
  char    localhost[MAXHOSTNAME];
  char   *remotehost; /* self evident */
  Unsgn16 remoteport;
  char    metaServer[MAXHOSTNAME]; /* the metaserver we are using */
} ClientInfo_t;

EXTERNAL ClientInfo_t cInfo;
EXTERNAL spHello_t sHello;		/* some server info we want to keep */
EXTERNAL spServerStat_t sStat;		/* server status info */
#ifdef CLIENT_NOEXTERN
int lastServerError = 0;	/* set by an ACK from server */
Unsgn8 clientFlags = 0;         /* set according to CLIENTSTAT packets */
#else
EXTERNAL int lastServerError;
EXTERNAL Unsgn8 clientFlags;    
#endif

#undef EXTERNAL

int sendAuth(int sock, Unsgn8 flag, char *login, char *pw);
int sendSetCourse(int sock, Sgn8 lock, real head);

/* packet proc routines */
int procUser(char *buf);
int procShip(char *buf);
int procShipSml(char *buf);
int procShipLoc(char *buf);
int procPlanet(char *buf);
int procPlanetSml(char *buf);
int procPlanetLoc(char *buf);
int procPlanetLoc2(char *buf);
int procPlanetInfo(char *buf);
int procTorp(char *buf);
int procTorpLoc(char *buf);
int procTorpEvent(char *buf);
int procTeam(char *buf);
int procMessage(char *buf);
int procServerStat(char *buf);
int procConqInfo(char *buf);
int procHistory(char *buf);
int procDoomsday(char *buf);

int sendSetName(char *name);
int sendCommand(Unsgn8 cmd, Unsgn16 detail);
int sendFireTorps(int num, real dir);
int sendMessage(int to, char *msg);

int clientHello(char *clientname);

void processPacket(char *buf);

void sendUDPKeepAlive(Unsgn32 timebase);

spClientStat_t *chkClientStat(char *buf);

#endif /* CLIENT_H_INCLUDED */
