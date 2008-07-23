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

#ifndef	_UDP_H
#define	_UDP_H

#include <sys/types.h>

int udpOpen(int port, struct sockaddr_in* addr);
int udpClose(int fd);
int udpSend(int fd, const void* buffer,
            int bufferLength, const struct sockaddr_in*);
int udpRecv(int fd, void* buffer,
            int bufferLength, struct sockaddr_in*);

#endif /* _UDP_H */
