/*
 * packet routines
 *
 * Some ideas borrowed from netrek.
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef PACKET_H_INCLUDED
#define PACKET_H_INCLUDED


#include "cb.h"
#include "protocol.h"

#define PKT_MAXSIZE     1024	/* no packet should ever be this large. gulp.*/

typedef int (*dispatchProc_t)(char *);

typedef struct _packetent {
    uint32_t        pktid;
    uint32_t        size;
    const char      *name;
    dispatchProc_t dispatch;
} packetEnt_t;

/* this will be implemented in cproc.c and sproc.c to initialize the
 *  (*dispatch) member of the server/clientPackets[] arrays in packet.c
 */
int procDispatchInit(uint16_t vers, packetEnt_t *pktList, int numpkts);

/* input/output ring buffer sizes. TCP RB's are larger to handle cases
 * where UDP is not available and the TCP RB is used instead.
 */
#define PKT_UDP_RB_MAX    (PKT_MAXSIZE * 64)
#define PKT_TCP_RB_MAX    (PKT_UDP_RB_MAX * 2)

/* for pktWrite, whether to send via UDP (if available) */
#define PKT_SENDTCP    0
#define PKT_SENDUDP    1

/* special value for pktSetSocketFds to ignore a set */
#define PKT_SOCKFD_NOCHANGE  (-2)

/* error/severity codes for Acks, should make sure these sync to
   pktSeverity2String(int psev) */
#define PSEV_INFO     0
#define PSEV_WARN     1
#define PSEV_ERROR    2
#define PSEV_FATAL    3

/* error codes */
#define PERR_OK           0	/* no error */
#define PERR_UNSPEC       1	/* unspecified */
#define PERR_BADPROTO     2	/* bad protocol */
#define PERR_BADCMN       3	/* common block mismatch */
#define PERR_INVUSER      4 	/* invalid username */
#define PERR_NOUSER       5 	/* no such user */
#define PERR_INVPWD       6 	/* invalid password */
#define PERR_BADPWD       7 	/* wrong password */
#define PERR_CLOSED       8 	/* game closed */
#define PERR_REGISTER     9 	/* register failed */
#define PERR_NOSHIP       10 	/* no slots available */
#define PERR_LOSE         11 	/* lose in menu() */
#define PERR_FLYING       12 	/* already flying a ship (newship()) */
#define PERR_TOOMANYSHIPS 13	/* you are flying too many ships (newship()) */
#define PERR_CANCELED     14	/* an operation (bombing, etc) was cancelled
				   for some reason. */
#define PERR_DONE         15    /* finished something - like beaming */
#define PERR_DOUDP        16	/* used in hello to tell server udp is ok */
#define PERR_PINGRESP     17	/* a ping reponse for nCP */

typedef struct _pkt_stats {
    unsigned int  rxBytes;        /* tx/rx byte counts */
    unsigned int  txBytes;
    unsigned int  pingAvg;        /* in ms */
} pktStats_t;

#ifdef NOEXTERN_PACKET
pktStats_t          pktStats = {};
packetEnt_t         *serverPackets = NULL;
int                 serverPktMax = 0;
#else
extern pktStats_t   pktStats;
extern packetEnt_t  *serverPackets;
extern int          serverPktMax;
#endif

#define PKT_PROCSP(_pkt)                                        \
    ( (*serverPackets[((uint8_t)(_pkt)[0])].dispatch)((_pkt)) )

int   pktInit(void);
void  pktSetClientMode(int isclient);
uint16_t pktGetClientProtocolVersion();
int   pktSetClientProtocolVersion(uint16_t vers);
void  pktSetSocketFds(int tcpsock, int udpsock);

int   pktSendAck(uint8_t severity, uint8_t code, const char *msg);
int   pktIsConnDead(void);
int   pktNotImpl(char *nothing);

void  pktSetNodelay(void);
void  pktSetNonBlocking(int enable);

const char *pktSeverity2String(int psev);

int   pktWaitForPacket(int type, char *buf, int blen,
                       int delay, const char *nakmsg);

int   pktClientPacketSize(int type);
int   pktServerPacketSize(int type);

int   pktSocketHasData(int sock);
int   pktWrite(int socktype, void *data);
int   pktReadPacketReady(void);
int   pktRead(char *buf, int len, unsigned int delay);
int   pktIsValid(int pkttype, void *pkt);

#endif /* PACKET_H_INCLUDED */
