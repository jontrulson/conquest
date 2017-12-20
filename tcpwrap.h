/*
 * TCP Wrappers support
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */


#ifndef _TCPWRAP_H
#define _TCPWRAP_H

#define TCPW_DAEMON_CONQUESTD   "conquestd"
#define TCPW_DAEMON_CONQMETAD   "conqmetad"

#if defined(HAVE_TCPW) && defined(HAVE_TCPD_H)
int tcpwCheckHostAccess(char *daemon, char *remotehost);
#else
#define tcpwCheckHostAccess(x, y)  (true)
#endif

#endif /* _TCPWRAP_H */
