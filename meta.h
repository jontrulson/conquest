/* 
 * Stuff for the meta server
 *
 * $Id$
 *
 * Copyright 2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef META_H_INCLUDED
#define META_H_INCLUDED

#include "conqdef.h"
#include "datatypes.h"

#define META_VERMAJ 0
#define META_VERMIN 2
#define META_VERSION  (Unsgn16)((META_VERMAJ << 8) | META_VERMIN)

#define META_MAXSERVERS   1000  /* max number of servers we will track */
#define BUFFERSZ          (1024 * 64)

#define META_GEN_STRSIZE  256   /* generic meta str size */

/* internal representation of a server record for the meta server */
typedef struct _meta_srec {
  int     valid;
  Unsgn16 version;
  Unsgn8  numactive;
  Unsgn8  numvacant;
  Unsgn8  numrobot;
  Unsgn8  numtotal;
  time_t  lasttime;             /* last contact time */
  Unsgn32 flags;                /* same as spServerStat_t */
  Unsgn16 port;
  char    addr[CONF_SERVER_NAME_SZ]; /* server's detected address */
  char    altaddr[CONF_SERVER_NAME_SZ]; /* specified real address */
  char    servername[CONF_SERVER_NAME_SZ];
  char    serverver[CONF_SERVER_NAME_SZ]; /* server's proto version */
  char    motd[CONF_SERVER_MOTD_SZ];

  /* Version 0x0002 */
  Unsgn16 protovers;
  char    contact[META_GEN_STRSIZE];
  char    walltime[META_GEN_STRSIZE];

} metaSRec_t;

void pipe2ul(char *str);
int str2srec(metaSRec_t *srec, char *buf);
void srec2str(char *buf, metaSRec_t *srec);
int metaUpdateServer(char *remotehost, char *name, int port);
int metaGetServerList(char *remotehost, metaSRec_t **srvlist);

#endif /* META_H_INCLUDED */
