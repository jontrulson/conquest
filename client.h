/*
 * Per client (conquest) info
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include "conqdef.h"

#include "msg.h"

#include "conqnet.h"
#include "protocol.h"
#include "packet.h"

#include "cproc.h"

/* client state */
#define CLT_STATE_NONE      0x00000000
#define CLT_STATE_PREINIT   0x00000001	/* local init */
#define CLT_STATE_INIT      0x00000002	/* negotiate with client/login */
#define CLT_STATE_MAINMENU  0x00000004	/* in main menu */
#define CLT_STATE_PLAYMENU  0x00000008	/* a menu/info screen engaged
					   during play */
#define CLT_STATE_PLAY      0x00000010	/* playing */
#define CLT_STATE_TERMINATE 0x00000020	/* terminating */

typedef struct {
    int  sock;			/* socket to client */
    int usock;			/* UDP socket to client */
    int doUDP;                    /* whether we should look for udp packets */
    int tryUDP;                   /* should we try UDP? */
    struct sockaddr_in  servaddr;
    uint32_t  state;		/* current state */
    uint32_t  serverDead;		/* is the client dead/ */
    uint32_t  isLoggedIn;		/* is the client logged in? */
    char    *remotehost;          /* self evident */
    uint16_t  remoteport;
    char     metaServer[MAXHOSTNAME]; /* the metaserver we are using */
} ClientInfo_t;

#ifdef NOEXTERN_CLIENT
ClientInfo_t          cInfo;
spHello_t             sHello;   /* some server info we want to keep */
spServerStat_t        sStat;    /* server status info */
int                   lastServerError = 0; /* set by an ACK from server */
uint8_t                clientFlags = 0; /* set according to CLIENTSTAT packets */

/* we need global versions of these around so we can use the dispatch
 * table routines to load them, yet allow special code to check the values
 * afterwards (like Auth, Welcome, etc)...
 */

spAckMsg_t            sAckMsg;  /* hold Ack or AckMsg pkt */
spClientStat_t        sClientStat;
spFrame_t             sFrame;
#else
extern ClientInfo_t   cInfo;
extern spHello_t      sHello;   /* some server info we want to keep */
extern spServerStat_t sStat;    /* server status info */
extern int            lastServerError;
extern uint8_t         clientFlags;

extern spAckMsg_t     sAckMsg;  /* hold Ack or AckMsg pkt */
extern spClientStat_t sClientStat;
extern spFrame_t      sFrame;
#endif

int             sendAuth(int sock, uint8_t flag, char *login, char *pw);
int             sendSetCourse(int sock, courseLock_t lock, uint16_t lockDetail,
                              real head);

int             sendSetName(char *name);
int             sendCommand(uint8_t cmd, uint16_t detail);
int             sendFireTorps(int num, real dir);
int             sendMessage(msgTo_t to, uint16_t toDetail, char *msg);

int             clientHello(char *clientname);

void            sendUDPKeepAlive(uint32_t timebase);

spClientStat_t *chkClientStat(char *buf);

#endif /* CLIENT_H_INCLUDED */
