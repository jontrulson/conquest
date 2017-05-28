/*
 * TCP Wrappers support
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "global.h"
#include "config.h"

#if defined(HAVE_TCPW) && defined(HAVE_TCPD_H)

#include <setjmp.h>

#include <tcpd.h>
#include "tcpwrap.h"

int allow_severity;
int deny_severity;

extern int hosts_ctl(char *daemon, char *client_name, char *client_addr,
                     char *client_user);

int tcpwCheckHostAccess(char *daemon, char *remotehost)
{
    int allowed = 0;

    if (NOT_INADDR(remotehost))
    {
        allowed = hosts_ctl(daemon, remotehost,
                            "", STRING_UNKNOWN);
    }
    else
    {
        allowed = hosts_ctl(daemon, "",
                            remotehost, STRING_UNKNOWN);
    }

    if (!allowed)
        utLog("TCPW: %s: %s: ACCESS DENIED",
              daemon, remotehost);
    else if (cqDebug)
        utLog("TCPW: %s: %s: ACCESS GRANTED",
              daemon, remotehost);

    return((allowed) ? TRUE : FALSE);

}

#endif /* HAVE_TCPW && HAVE_TCPD_H */
