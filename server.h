/* 
 * Per server (conquestd) info
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include "conqdef.h"
#include "conqnet.h"
#include "datatypes.h"
#include "protocol.h"
#include "packet.h"

#define SVR_STATE_NONE      0x00000000	/* local init */
#define SVR_STATE_PREINIT   0x00000001	/* local init */
#define SVR_STATE_INIT      0x00000002	/* negotiate with client/login */
#define SVR_STATE_MAINMENU  0x00000004	/* in main menu */
#define SVR_STATE_PLAYMENU  0x00000008	/* a menu/info screen engaged
					   during play */
#define SVR_STATE_PLAY      0x00000010	/* playing */
#define SVR_STATE_TERMINATE 0x00000020	/* terminating */

typedef struct {
  int sock;			/* socket to client */
  int usock;			/* udp socket to client */
  struct sockaddr_in clntaddr;  /* client's saddrin struct */
  int doUDP;                    /* send udp? */
  int tryUDP;                    /* try udp? */
  Unsgn32 state;		/* current state */
  Unsgn32 clientDead;		/* is the client dead/ */
  Unsgn32 isMaster;		/* is the Maseter server? */
  Unsgn32 isLoggedIn;		/* is the client logged in? */
  Unsgn8 remotehost[MAXHOSTNAME]; /* self evident */
  Unsgn8 localhost[MAXHOSTNAME]; /* self evident */
} ServerInfo_t;


/* we also want to keep a copy of all packets sent */

#ifdef SERVER_NOEXTERN
# define EXTERN
#else
# define EXTERN extern
#endif
EXTERN ServerInfo_t sInfo;

#undef EXTERN

int sendClientStat(int sock, Unsgn8 flags, Unsgn8 snum, Unsgn8 team, 
		   Unsgn16 unum, Unsgn8 esystem);
int sendUser(int sock, Unsgn16 unum);
int sendShip(int sock, Unsgn8 snum);
int sendPlanet(int sock, Unsgn8 pnum);
int sendServerStat(int sock);
int sendTorp(int sock, Unsgn8 snum, Unsgn8 tnum);
int sendMessage(int sock, Msg_t *msg);
int sendTeam(int sock, Unsgn8 team, int force);
int sendConqInfo(int sock, int force);
int sendHistory(int sock, int hnum);
int sendDoomsday(int sock);

void procSetName(Unsgn8 *buf);
void procSetCourse(Unsgn8 *buf);

void procSetWarp(cpCommand_t *swarp);
void procSetShields(cpCommand_t *cmd);
void procAlloc(cpCommand_t *cmd);
void procCloak(cpCommand_t *cmd);
void procDetSelf(cpCommand_t *cmd);
void procDetEnemy(cpCommand_t *cmd);
void procDistress(cpCommand_t *cmd);
void procFirePhaser(cpCommand_t *cmd);
void procOrbit(cpCommand_t *cmd);
void procRepair(cpCommand_t *cmd);
void procSetWar(cpCommand_t *cmd);
void procRefit(cpCommand_t *cmd);
void procSetRate(cpCommand_t *cmd);
void procTow(cpCommand_t *cmd);
void procUnTow(cpCommand_t *cmd);
void procCoup(cpCommand_t *cmd);
void procBomb(cpCommand_t *cmd);
void procBeam(cpCommand_t *cmd);
void procDestruct(cpCommand_t *cmd);
void procAutoPilot(cpCommand_t *cmd);

void procFireTorps(Unsgn8 *buf);
void procMessage(Unsgn8 *buf);
void procChangePassword(Unsgn8 *buf);
void procReload(cpCommand_t *cmd);

int checkOperExec(int from, int to, char *msg);

#endif /* SERVER_H_INCLUDED */
