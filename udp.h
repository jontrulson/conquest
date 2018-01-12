/*
 * UDP handling
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef	_UDP_H
#define	_UDP_H

#include <sys/types.h>

int udpOpen(int port, struct sockaddr_in* addr);
int udpClose(int fd);
int udpSendTo(int fd, const void* buffer,
            int bufferLength, const struct sockaddr_in*);
int udpRecvFrom(int fd, void* buffer,
            int bufferLength, struct sockaddr_in*);

int udpSendPacket(int sock, const void* buffer, size_t buflen);
int udpRecvPacket(int sock, char* buffer, size_t buflen);
uint32_t udpGetReadSeq();
uint32_t udpGetWriteSeq();



#endif /* _UDP_H */
