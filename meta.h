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
#define META_VERMIN 1
#define META_VERSION  (Unsgn16)((META_VERMAJ << 8) | META_VERMIN)

#define META_MAXSERVERS   1000  /* max number of servers we will track */
#define BUFFERSZ       (1024 * 64)

/* internal representation of a server record for the meta server */
typedef struct _meta_srec {
  int valid;
  Unsgn8 numactive;
  Unsgn8 numvacant;
  Unsgn8 numrobot;
  Unsgn8 numtotal;
  time_t lasttime;              /* last contact time */
  Unsgn32 flags;                /* same as spServerStat_t */
  Unsgn16 port;
  Unsgn8 addr[CONF_SERVER_NAME_SZ]; /* server's detected address */
  Unsgn8 altaddr[CONF_SERVER_NAME_SZ]; /* specified real address */
  Unsgn8 servername[CONF_SERVER_NAME_SZ];
  Unsgn8 serverver[CONF_SERVER_NAME_SZ]; /* server's version */
  Unsgn8 motd[CONF_SERVER_MOTD_SZ];
} metaSRec_t;

void pipe2ul(char *str);
int str2srec(metaSRec_t *srec, char *buf);
void srec2str(char *buf, metaSRec_t *srec);
int metaUpdateServer(char *remotehost, char *name, int port);
int metaGetServerList(char *remotehost, metaSRec_t **srvlist);

#endif /* META_H_INCLUDED */
