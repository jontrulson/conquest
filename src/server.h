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

#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

#include "conqdef.h"
#include "conqnet.h"

#include "protocol.h"
#include "packet.h"

#include "sproc.h"

typedef enum {
    SVR_STATE_NONE = 0,                /* nothing */
    SVR_STATE_PREINIT,                 /* local init */
    SVR_STATE_INIT,                    /* negotiate with client/login */
    SVR_STATE_MAINMENU,                /* in main menu */
    SVR_STATE_PLAY,                    /* playing */
} serverStates_t;

typedef struct {
    int sock;                     /* socket to client */
    int usock;                    /* udp socket to client */
    uint16_t listenPort;          /* our listen port */
    struct  sockaddr_in clntaddr; /* client's saddrin struct */
    bool doUDP;                   /* send udp? */
    serverStates_t state;         /* current state */
    char remotehost[MAXHOSTNAME]; /* self evident */
} ServerInfo_t;

#ifdef NOEXTERN_SERVER
ServerInfo_t sInfo;
#else
extern ServerInfo_t sInfo;
#endif

int sendClientStat(int sock, uint8_t flags, int16_t snum, uint8_t team,
                   uint16_t unum, uint8_t esystem);
int sendUser(int sock, uint16_t unum);
int sendShip(int sock, uint8_t snum);
int sendPlanet(int sock, uint8_t pnum, int force);
int sendServerStat(int socktype);
int sendTorp(int sock, uint8_t snum, uint8_t tnum);
int sendFeedBack(char *msg);
void sendFeedback(const char *msg);
int sendMessage(Msg_t *msg);
int sendTeam(int sock, uint8_t team, int force);
int sendcbConqInfo(int sock, int force);
int sendHistory(int sock, int hnum);
int sendDoomsday(int sock);

void serverStartUDP(pid_t pid, const cpCommand_t *ccmd);

#endif /* SERVER_H_INCLUDED */

