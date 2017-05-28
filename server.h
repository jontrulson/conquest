/*
 * Per server (conquestd) info
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include "conqdef.h"
#include "conqnet.h"

#include "protocol.h"
#include "packet.h"

#include "sproc.h"

#define SVR_STATE_NONE      0x00000000	/* local init */
#define SVR_STATE_PREINIT   0x00000001	/* local init */
#define SVR_STATE_INIT      0x00000002	/* negotiate with client/login */
#define SVR_STATE_MAINMENU  0x00000004	/* in main menu */
#define SVR_STATE_PLAYMENU  0x00000008	/* a menu/info screen engaged
					   during play */
#define SVR_STATE_PLAY      0x00000010	/* playing */
#define SVR_STATE_TERMINATE 0x00000020	/* terminating */

typedef struct {
    int     sock;			/* socket to client */
    int     usock;                /* udp socket to client */
    struct  sockaddr_in clntaddr; /* client's saddrin struct */
    int     doUDP;                /* send udp? */
    int     tryUDP;               /* try udp? */
    uint32_t state;		/* current state */
    uint32_t clientDead;		/* is the client dead/ */
    uint32_t isMaster;		/* is the Maseter server? */
    uint32_t isLoggedIn;		/* is the client logged in? */
    char    remotehost[MAXHOSTNAME]; /* self evident */
} ServerInfo_t;


/* we also want to keep a copy of all packets sent */

#ifdef NOEXTERN_SERVER
ServerInfo_t sInfo;
#else
extern ServerInfo_t sInfo;
#endif

int sendClientStat(int sock, uint8_t flags, uint8_t snum, uint8_t team,
		   uint16_t unum, uint8_t esystem);
int sendUser(int sock, uint16_t unum);
int sendShip(int sock, uint8_t snum);
int sendPlanet(int sock, uint8_t pnum, int force);
int sendServerStat(int socktype);
int sendTorp(int sock, uint8_t snum, uint8_t tnum);
int sendFeedBack(char *msg);
void sendFeedback(char *msg);
int sendMessage(Msg_t *msg);
int sendTeam(int sock, uint8_t team, int force);
int sendConqInfo(int sock, int force);
int sendHistory(int sock, int hnum);
int sendDoomsday(int sock);

int checkOperExec(int from, int to, char *msg);

#endif /* SERVER_H_INCLUDED */
