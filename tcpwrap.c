/* 
 * TCP Wrappers support
 *
 * $Id$
 *
 * Copyright 2007 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "config.h"

#if defined(HAVE_TCPW) && defined(HAVE_TCPD_H) 

#include <setjmp.h>

#include <tcpd.h>
#include "tcpwrap.h"

int allow_severity;
int deny_severity;

extern int hosts_ctl(char *daemon, char *client_name, char *client_addr,
                     char *client_user);

int tcpwCheckHostAccess(char *remotehost)
{
  int allowed = 0;

  if (NOT_INADDR(remotehost))
    {
      allowed = hosts_ctl(TCPW_DAEMON, remotehost, 
                          "", STRING_UNKNOWN);
    }
  else
    {
      allowed = hosts_ctl(TCPW_DAEMON, "", 
                           remotehost, STRING_UNKNOWN);
    }
  
  if (!allowed)
    clog("TCPW: %s: ACCESS DENIED",
         remotehost);
  else
    clog("TCPW: %s: ACCESS GRANTED",
         remotehost);
      
  return((allowed) ? TRUE : FALSE);

}
 
#endif /* HAVE_TCPW && HAVE_TCPD_H */

