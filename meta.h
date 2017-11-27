/*
 * Stuff for the meta server
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef META_H_INCLUDED
#define META_H_INCLUDED

#include "conqdef.h"


#define META_VERMAJ 0
#define META_VERMIN 2
#define META_VERSION  (uint16_t)((META_VERMAJ << 8) | META_VERMIN)

#define META_MAXSERVERS   1000  /* max number of servers we will track */
#define BUFFERSZ          (1024 * 64)

#define META_GEN_STRSIZE  256   /* generic meta str size */

/* internal representation of a server record for the meta server */
typedef struct _meta_srec {
    int     valid;
    uint16_t version;
    uint8_t  numactive;
    uint8_t  numvacant;
    uint8_t  numrobot;
    uint8_t  numtotal;
    time_t  lasttime;             /* last contact time */
    uint32_t flags;                /* same as spServerStat_t */
    uint16_t port;
    char    addr[CONF_SERVER_NAME_SZ]; /* server's detected address */
    char    altaddr[CONF_SERVER_NAME_SZ]; /* specified real address */
    char    servername[CONF_SERVER_NAME_SZ];
    char    serverver[CONF_SERVER_NAME_SZ]; /* server's proto version */
    char    motd[CONF_SERVER_MOTD_SZ];

    /* Version 0x0002 */
    uint16_t protovers;
    char    contact[META_GEN_STRSIZE];
    char    walltime[META_GEN_STRSIZE];

} metaSRec_t;

int  metaBuffer2ServerRec(metaSRec_t *srec, char *buf);
void metaServerRec2Buffer(char *buf, metaSRec_t *srec);
int  metaUpdateServer(char *remotehost, char *name, int port);
int  metaGetServerList(char *remotehost, metaSRec_t **srvlist);

#endif /* META_H_INCLUDED */
