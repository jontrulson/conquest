//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#ifndef CLIENT_H_INCLUDED
#define CLIENT_H_INCLUDED

#include "conqdef.h"

#include <string>

#include "msg.h"

#include "conqnet.h"
#include "protocol.h"
#include "packet.h"

#include "cproc.h"

typedef enum {
    CLIENT_STATE_INIT = 0,
    CLIENT_STATE_MENU,
    CLIENT_STATE_PLAY,
} clientState_t;

typedef struct {
    int sock;			/* TCP socket to client */
    int usock;			/* UDP socket to client */
    int doUDP;                    /* whether we should look for udp packets */
    int tryUDP;                   /* should we try UDP? */
    struct sockaddr_in  servaddr;
    char    *remotehost;          /* self evident */
    uint16_t  remoteport;
    char     metaServer[MAXHOSTNAME]; /* the metaserver we are using */

    uint32_t nodeMillis;        /* set to clbGetMillis() in
                                 * renderNode() every frame */
    clientState_t state;        // current client state
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

spAck_t               sAck;  /* hold Ack */
spClientStat_t        sClientStat;
spFrame_t             sFrame;
#else
extern ClientInfo_t   cInfo;
extern spHello_t      sHello;   /* some server info we want to keep */
extern spServerStat_t sStat;    /* server status info */
extern int            clientLastServerAckCode;
extern bool           clientStatReceived;
extern uint8_t        clientStatLastFlags;

extern spAck_t        sAck;  /* hold Ack pkt */
extern spClientStat_t sClientStat;
extern spFrame_t      sFrame;
#endif

int             sendAuth(int sock, uint8_t flag, const char *login,
                         const char *pw);
int             sendSetCourse(int sock, courseLock_t lock, uint16_t lockDetail,
                              real head);

int             sendSetName(const char *name);
int             sendCommand(uint8_t cmd, uint16_t detail);
int             sendFireTorps(int num, real dir);
void            sendMessage(msgTo_t to, uint16_t toDetail,
                            const std::string& msg);

int             clientHello(const char *clientname);

void            sendUDPKeepAlive(uint32_t timebase);

spClientStat_t *chkClientStat(char *buf);

#endif /* CLIENT_H_INCLUDED */
