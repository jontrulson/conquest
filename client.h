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
# define EXTERNAL extern
#else
# define EXTERNAL
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
  Unsgn32 state;		/* current state */
  Unsgn32 serverDead;		/* is the client dead/ */
  Unsgn32 isLoggedIn;		/* is the client logged in? */
  Unsgn8 localhost[MAXHOSTNAME];
  Unsgn8 *remotehost; /* self evident */
  Unsgn16 remoteport;
} ClientInfo_t;

EXTERNAL ClientInfo_t cInfo;
EXTERNAL spHello_t sHello;		/* some server info we want to keep */
EXTERNAL spServerStat_t sStat;		/* server status info */

#undef EXTERNAL

int sendAuth(int sock, Unsgn8 flag, Unsgn8 *login, Unsgn8 *pw);
int sendSetCourse(int sock, Sgn8 lock, real head);

/* packet proc routines */
int procUser(Unsgn8 *buf);
int procShip(Unsgn8 *buf);
int procShipSml(Unsgn8 *buf);
int procShipLoc(Unsgn8 *buf);
int procPlanet(Unsgn8 *buf);
int procPlanetSml(Unsgn8 *buf);
int procPlanetLoc(Unsgn8 *buf);
int procTorp(Unsgn8 *buf);
int procTorpLoc(Unsgn8 *buf);
int procTeam(Unsgn8 *buf);
int procMessage(Unsgn8 *buf);
int procServerStat(Unsgn8 *buf);
int procConqInfo(Unsgn8 *buf);
int procHistory(Unsgn8 *buf);
int procDoomsday(Unsgn8 *buf);

int sendSetName(Unsgn8 *name);
int sendCommand(Unsgn8 cmd, Unsgn16 detail);
int sendFireTorps(int num, real dir);
int sendMessage(int to, char *msg);

#endif /* CLIENT_H_INCLUDED */
