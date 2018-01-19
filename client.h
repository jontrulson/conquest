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

typedef struct {
    int sock;			/* TCP socket to client */
    int usock;			/* UDP socket to client */
    int doUDP;                    /* whether we should look for udp packets */
    int tryUDP;                   /* should we try UDP? */
    struct sockaddr_in  servaddr;
    char    *remotehost;          /* self evident */
    uint16_t  remoteport;
    char     metaServer[MAXHOSTNAME]; /* the metaserver we are using */
} ClientInfo_t;

#ifdef NOEXTERN_CLIENT
ClientInfo_t          cInfo;
spHello_t             sHello;   /* some server info we want to keep */
spServerStat_t        sStat;    /* server status info */
// set by an ACK from server after it's been processed
int                   clientLastServerAckCode = 0;
// set to true when a clientstat packet is processed
bool                  clientStatReceived = false;
// set when last clientstat packet processed
uint8_t               clientStatLastFlags = 0;

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
extern int            clientLastServerAckCode;
extern bool           clientStatReceived;
extern uint8_t        clientStatLastFlags;

extern spAckMsg_t     sAckMsg;  /* hold Ack or AckMsg pkt */
extern spClientStat_t sClientStat;
extern spFrame_t      sFrame;
#endif

int             sendAuth(int sock, uint8_t flag, const char *login,
                         const char *pw);
int             sendSetCourse(int sock, courseLock_t lock, uint16_t lockDetail,
                              real head);

int             sendSetName(char *name);
int             sendCommand(uint8_t cmd, uint16_t detail);
int             sendFireTorps(int num, real dir);
int             sendMessage(msgTo_t to, uint16_t toDetail, char *msg);

int             clientHello(const char *clientname);

void            sendUDPKeepAlive(uint32_t timebase);

spClientStat_t *chkClientStat(char *buf);

#endif /* CLIENT_H_INCLUDED */
