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

#ifndef META_H_INCLUDED
#define META_H_INCLUDED

#include "conqdef.h"
#include <string>
#include <vector>

#define META_VERMAJ 0
#define META_VERMIN 2
#define META_VERSION  (uint16_t)((META_VERMAJ << 8) | META_VERMIN)

#define META_MAXSERVERS   1000  /* max number of servers we will track */
#define META_MAX_PKT_SIZE          (1024 * 64)

#define META_GEN_STRSIZE  256   /* generic meta str size */

/* internal representation of a server record for the meta server */
typedef struct _meta_srec {
    bool     valid;
    uint16_t version;
    uint8_t  numactive;
    uint8_t  numvacant;
    uint8_t  numrobot;
    uint8_t  numtotal;
    time_t   lasttime;          /* last contact time */
    uint32_t flags;             /* same as spServerStat_t */
    uint16_t port;
    std::string addr;           /* server's detected address */
    std::string altaddr;        /* specified real address */
    std::string servername;
    std::string serverver;      /* server version */
    std::string motd;

    uint16_t protovers;         // meta protocol version
    std::string contact;
    std::string walltime;
} metaSRec_t;

typedef std::vector<metaSRec_t> metaServerVec_t;

int  metaBuffer2ServerRec(metaSRec_t *srec, const char *buf);
void metaServerRec2Buffer(std::string& buf, const metaSRec_t& srec);
int  metaUpdateServer(const char *remotehost, const char *name, int port);
int  metaGetServerList(const char *remotehost, metaServerVec_t& srvlist);

#endif /* META_H_INCLUDED */
