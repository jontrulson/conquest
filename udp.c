/* borrowed from bzflag
 * Copyright (c) 1993 - 2003 Tim Riker
 *
 * I hacked it up somewhat - for straight C, and to better fit in with Conquest.
 * JET
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the LGPL. [JET]
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "c_defs.h"
#include "conqnet.h"
#include "udp.h"

int udpOpen(int port, struct sockaddr_in* addr)
{
#if defined(_WIN32)
  const BOOL optOn = TRUE;
  BOOL opt = optOn;
#else
  const int optOn = 1;
  int opt = optOn;
#endif
  int fd;

  /* check parameters */
  if (!addr) {
    clog("NET: openUDPnetwork: Must supply an address structure!");
    return -1;
  }
  memset(addr, 0, sizeof(*addr));

  /* open socket */
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    clog("NET: openUDPnetwork: socket");
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
    clog("NET: setsockopt SO_REUSEPORT: %s", strerror(errno));
    close(fd);
    return -1;
  }
#elif defined(SO_REUSEADDR)
  /* set reuse address */
  opt = optOn;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
				(SSOType)&opt, sizeof(opt)) < 0) {
    clog("NET: openUDPnetwork: setsockopt SO_REUSEADDR: %s", strerror(errno));
    close(fd);
    return -1;
  }
#endif

  /* bind address */
  if (bind(fd, (const struct sockaddr*)addr, sizeof(*addr)) < 0) {
    clog("NET: openUDPnetwork: bind: %s", strerror(errno));
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


int udpSend(int fd, const void* buffer,
                   int bufferLength,
                   const struct sockaddr_in* addr)
{
  return sendto(fd, (const char*)buffer, bufferLength, 0,
                (const struct sockaddr*)addr, sizeof(*addr));
}

int udpRecv(int fd, void* buffer, int bufferLength,
                   struct sockaddr_in* addr)
{
  struct sockaddr_in from;
  AddrLen fromLength = sizeof(from);
  
  int byteCount = recvfrom(fd, (char*)buffer, bufferLength, 0,
                           (struct sockaddr*)&from, (socklen_t*) &fromLength);
  if (byteCount < 0) {
    if (errno == EWOULDBLOCK) {
      return 0;
    }
    else {
      clog("NET: udpRecv: %s", strerror(errno));
      return -1;
    }
  }
  if (addr) 
    *addr = from;
  return byteCount;
}

// ex: shiftwidth=2 tabstop=8
