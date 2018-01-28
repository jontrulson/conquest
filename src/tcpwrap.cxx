//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


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

    return((allowed) ? true : false);

}

#endif /* HAVE_TCPW && HAVE_TCPD_H */
