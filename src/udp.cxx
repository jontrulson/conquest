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


#include "c_defs.h"

#include "conqnet.h"
#include "conf.h"
#include "global.h"
#include "udp.h"
#include "conqutil.h"
#include "packet.h"

// incremented for each packet written, and sent as the first 4 bytes
// of every udp packet.  Start at one so we don't confuse the recv.
static uint32_t writeSeq = 1;

// expected as the first 4 bytes of every udp packet read.  We use
// this to detect duplicate and out-of-order (OOO) packets
static uint32_t readSeq = 0;

int udpOpen(int port, struct sockaddr_in* addr)
{
#if defined(_WIN32)
    const BOOL optOn = true;
    BOOL opt = optOn;
#else
    const int optOn = 1;
    int opt = optOn;
#endif
    int fd;

    /* check parameters */
    if (!addr) {
        utLog("NET: openUDPnetwork: Must supply an address structure!");
        return -1;
    }
    memset(addr, 0, sizeof(*addr));

    /* open socket */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        utLog("NET: openUDPnetwork: socket");
        return -1;
    }

    /* set address info */
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);

#if defined(SO_REUSEPORT)
    /* set reuse port */
    opt = optOn;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
                   (SSOType)&opt, sizeof(opt)) < 0) {
        utLog("NET: setsockopt SO_REUSEPORT: %s", strerror(errno));
        close(fd);
        return -1;
    }
#endif

#if defined(SO_REUSEADDR)
    /* set reuse address */
    opt = optOn;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   (SSOType)&opt, sizeof(opt)) < 0) {
        utLog("NET: openUDPnetwork: setsockopt SO_REUSEADDR: %s", strerror(errno));
        close(fd);
        return -1;
    }
#endif

    /* bind address */
    if (bind(fd, (const struct sockaddr*)addr, sizeof(*addr)) < 0) {
        utLog("NET: openUDPnetwork: bind: %s", strerror(errno));
        close(fd);
        return -1;
    }

    return fd;
}

int udpClose(int fd)
{
    if (fd == -1)
        return 0;
    return close(fd);
}

int udpSendTo(int fd, const void* buffer,
            int bufferLength,
            const struct sockaddr_in* addr)
{
    return sendto(fd, (const char*)buffer, bufferLength, 0,
                  (const struct sockaddr*)addr, sizeof(*addr));
}

int udpRecvFrom(int fd, void* buffer, int bufferLength,
            struct sockaddr_in* addr)
{
    struct sockaddr_in from;
    socklen_t fromLength = sizeof(from);

    int byteCount = recvfrom(fd, (char*)buffer, bufferLength, 0,
                             (struct sockaddr*)&from, &fromLength);
    if (byteCount < 0)
    {
#if !defined(MINGW)
        if (errno == EWOULDBLOCK)
        {
            return 0;
        }
        else
#endif  /* MINGW */
        {
            utLog("NET: udpRecvFrom: %s", strerror(errno));
            return -1;
        }
    }
    if (addr)
        *addr = from;
    return byteCount;
}

int udpSendPacket(int sock, const void* buffer, size_t buflen)
{
    size_t sendLen = buflen + sizeof(uint32_t);
    uint8_t sendBuf[sendLen];

    uint32_t sendSeq = htonl(writeSeq);
    writeSeq++;

    sendBuf[0] = (sendSeq & 0x000000ff);
    sendBuf[1] = ((sendSeq & 0x0000ff00) >> 8);
    sendBuf[2] = ((sendSeq & 0x00ff0000) >> 16);
    sendBuf[3] = ((sendSeq & 0xff000000) >> 24);

    memcpy((void *)&sendBuf[4], (const void *)buffer, buflen);

    // send it off

    int rv = send(sock, sendBuf, sendLen, 0);

    return ((rv > 0) ? buflen : rv);
}

int udpRecvPacket(int sock, char* buffer, size_t buflen)
{
    size_t recvLen = PKT_MAXSIZE + sizeof(uint32_t);
    uint8_t readPkt[recvLen];
    uint32_t theSeq = 0;

    int rv = recv(sock, readPkt, recvLen, 0);

    if (rv <= 0)
        return rv;

    // must have a minimum of 8 bytes (seq # + pktid + at least 3
    // bytes of pkt).  All packets, minus the seq number are at least
    // 4 bytes long.
    if (rv < 8)
    {
        utLog("%s: received short packet: %d, ignoring", __FUNCTION__, rv);
        return 0;
    }

    // unpack the seq # and check for things to make us go
    theSeq = (readPkt[0]
              | (readPkt[1] << 8)
              | (readPkt[2] << 16)
              | (readPkt[3] << 24));

    theSeq = ntohl(theSeq);

    if (theSeq > readSeq)
    {
        unsigned int lost = theSeq - readSeq;
        if (lost > 1)
        {
            pktStats.lostPackets += (lost - 1);

            if (cqDebug)
                utLog("%s: Lost %u packet(s)", __FUNCTION__,
                      lost - 1);
        }
        readSeq = theSeq;
    }
    else if (theSeq == readSeq)
    {
        if (cqDebug)
            utLog("%s: Duplicate packet (seq %u), ignoring", __FUNCTION__,
                  theSeq);
        pktStats.duplicatePackets++;
        return 0;
    }
    else if (theSeq < readSeq)
    {
        // check for wrap-around
        if (abs(int(readSeq - theSeq)) > (UINT_MAX / 2))
        { // wrap around
            readSeq = theSeq;
        }
        else
        {
            if (cqDebug)
                utLog("%s: Out of Order packet (seq %u, last seq %u) ignoring",
                      __FUNCTION__, theSeq, readSeq);
            pktStats.oooPackets++;
            return 0;
        }
    }

    // if we are here, we can copy in the packet, and return the RV
    // value (minus the bytes used for the seq #)
    rv -= sizeof(uint32_t);
    if (rv > buflen)
    {
        // then the supplied buffer was too small, so we must dump the packet
        utLog("%s: received packet is larger than supplied buffer (%d > %lu)"
              ", dumping",
              __FUNCTION__,
              rv, buflen);
        return 0;
    }

    memcpy(buffer, &readPkt[4], rv);

    return rv;
}

uint32_t udpGetReadSeq()
{
    return readSeq;
}

uint32_t udpGetWriteSeq()
{
    return writeSeq;
}
