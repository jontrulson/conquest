/* 
 * TCP Wrappers support
 *
 * $Id$
 *
 * Copyright 2007 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */


#ifndef _TCPWRAP_H 
#define _TCPWRAP_H

#define TCPW_DAEMON   "conquestd"

#if defined(HAVE_TCPW) && defined(HAVE_TCPD_H) 
int tcpwCheckHostAccess(char *remotehost);
#else
#define tcpwCheckHostAccess(x)  (TRUE)
#endif

#endif /* _TCPWRAP_H */
