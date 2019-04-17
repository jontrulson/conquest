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

#include "cb.h"

#include "context.h"

#include "conqdef.h"
#include "conqnet.h"
#include "conqunix.h"

#include "conf.h"

#include "conqutil.h"

#include "meta.h"

#include "tcpwrap.h"

#include <string>
#include <vector>
#include <algorithm>

#define LISTEN_BACKLOG 5 /* # of requests we're willing to to queue */

static metaServerVec_t metaServerList;
static uint16_t listenPort = META_DFLT_PORT;
static char *progName;
static int localOnly = false;   /* whether to only listen on loopback */
const int expireSeconds = (60 * 5); /* 5 minutes */

void catchSignals(void);
void handleSignal(int sig);

// sort in descending order based on number of active ships
static bool cmpmeta(int cmp1, int cmp2)
{
    return metaServerList[cmp1].numactive > metaServerList[cmp2].numactive;
}

static void sortmetas( std::vector<int>& mv )
{
    std::sort(mv.begin(), mv.end(), cmpmeta);

    return;
}

void printUsage()
{
    printf("Usage: conqmetad [ -d ] [ -l ] [ -u user ] [ -v ] \n");
    printf("   -d            daemon mode\n");
    printf("   -l            listen for local connections only\n");
    printf("   -u user       run as user 'user'.\n");
    printf("   -v            be more verbose.\n");

    return;
}

void ageServers(void)
{
    int i;
    time_t now = time(0);

    for (i=0; i<metaServerList.size(); i++)
    {
        if (metaServerList[i].valid)
            if (abs(metaServerList[i].lasttime - now) > expireSeconds)
            {
                utLog("META: Expiring %s:%u(%s), slot %d\n",
                      metaServerList[i].altaddr.c_str(),
                      metaServerList[i].port,
                      metaServerList[i].addr.c_str(),
                      i);

                metaServerList[i] = {};
                metaServerList[i].valid = false;
            }
    }

    return;
}


/* find the server slot for this record (or make a new one).  returns the
   slot number found/created, or -1 for error */
int findSlot(const metaSRec_t& srec, bool& isUpdate)
{
    bool foundSlot = false;
    int rv = -1;

    isUpdate = false;

    /* first look for an existing, valid, matching entry */
    for (int i=0; i<metaServerList.size(); i++)
    {
        if (metaServerList[i].addr == srec.addr &&
            metaServerList[i].altaddr == srec.altaddr &&
            metaServerList[i].valid &&
            metaServerList[i].port == srec.port)
        {
            rv = i;
            foundSlot = true;
            isUpdate = true;
            break;
        }
    }

    if (!foundSlot)
    {                           /* didn't find one, see if there's an
                                 * invalid slot we can use */
        for (int i=0; i<metaServerList.size(); i++)
            if (!metaServerList[i].valid)
            {                     /* found a previously used slot */
                rv = i;
                foundSlot = true;
                break;
            }
    }

    if (!foundSlot)
    {
        // if one is still not found, create a new empty one if it will fit
        if (metaServerList.size() < META_MAXSERVERS)
        {
            metaServerList.push_back({});
            return metaServerList.size() - 1;
        }
        else
            return -1;                  /* can't fit */
    }
    else
        return rv;
}

void metaProcList(int sock, char *hostbuf)
{
    std::vector<int> mvec;
    std::string tbuf;

    /* init mvec */
    mvec.clear();
    for (int i=0; i < metaServerList.size(); i++)
        if (metaServerList[i].valid)
            mvec.push_back(i);

    sortmetas(mvec);

    utLog("META: server query from %s", hostbuf);

    /* dump the sorted server list */
    for (int i=0; i<mvec.size(); i++)
    {
        metaServerRec2Buffer(tbuf, metaServerList[mvec[i]]);
        if (write(sock, tbuf.c_str(), tbuf.size()) <= 0)
        {
            utLog("META: write failed to %s", hostbuf);
            return;
        }
    }


    return;
}

void metaProcUpd(char *buf, int rlen, char *hostbuf)
{
    metaSRec_t sRec = {};
    int slot;
    bool isUpdate;

    if (!metaBuffer2ServerRec(sRec, buf))
    {
        utLog("META: malformed buffer '%s', ignoring", buf);
        return;
    }

    sRec.addr = hostbuf;

    /* if altaddr is empty, we copy hostbuf into it. */

    if (sRec.altaddr.empty())
        sRec.altaddr = sRec.addr;

    if (sRec.port == 0)
        sRec.port = CN_DFLT_PORT;

    /* now find a slot for it. */
    if ((slot = findSlot(sRec, isUpdate)) == -1)
    {
        utLog("META: findSlot() failed, ignoring\n");
        return;
    }

    /* init the rest of slot */
    sRec.valid = true;
    sRec.lasttime = time(0);
    metaServerList[slot] = sRec;

    if (!isUpdate)                /* new server */
        utLog("META: Added server %s:%u(%s), slot %d",
              metaServerList[slot].altaddr.c_str(),
              metaServerList[slot].port,
              metaServerList[slot].addr.c_str(),
              slot);
    else
    {
#if defined(DEBUG_META)
        utLog("META: Updated server %s:%u(%s), slot %d",
              metaServerList[slot].altaddr.c_str(),
              metaServerList[slot].port,
              metaServerList[slot].addr.c_str(),
              slot);
#endif
    }

    return;
}

/* we exit here on serious errors */
/* we listen on both a UDP and TCP socket.  UDP is used to register a server
   TCP is used to get a dump of the current servers. */
void metaListen(void)
{
    int s, t, tc;			/* socket descriptor */
    struct sockaddr_in sa, isa;	/* internet socket addr. structure UDP */
    struct sockaddr_in tsa, tisa;	/* internet socket addr. structure TCP */
    struct timeval tv;
    struct hostent *hp;
    int rv, rlen;
    socklen_t sockln;
    char hostbuf[CONF_SERVER_NAME_SZ];
    char rbuf[META_MAX_PKT_SIZE];
    socklen_t alen;
    fd_set readfds;

    memset(&sa, 0, sizeof(struct sockaddr));
    memset(&isa, 0, sizeof(struct sockaddr));
    memset(&tsa, 0, sizeof(struct sockaddr));
    memset(&tisa, 0, sizeof(struct sockaddr));

    sa.sin_port = htons(listenPort);
    tsa.sin_port = htons(listenPort);

    if (localOnly)
    {
        sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        tsa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    else
    {
        sa.sin_addr.s_addr = htonl(INADDR_ANY);
        tsa.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    sa.sin_family = AF_INET;
    tsa.sin_family = AF_INET;

    /* allocate an open socket for incoming UDP connections */
    if (( s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        utLog("META: socket (DGRAM) failed: %s, exiting", strerror(errno) );
        perror ( "UDP socket" );
        exit(1);
    }

    /* allocate an open socket for incoming TCP connections */
    if (( t = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        utLog("META: socket (SOCK_STREAM) failed: %s, exiting",
              strerror(errno) );
        perror ( "TCP socket" );
        exit(1);
    }

    /* bind the socket to the service port so we hear incoming
     * connections
     */
    if ( bind( s, (struct sockaddr *)&sa, sizeof ( sa )) < 0 )
    {
        utLog("META: bind (UDP) failed: %s, exiting", strerror(errno) );
        perror( "UDP bind" );
        exit(1);
    }

    if ( bind( t, (struct sockaddr *)&tsa, sizeof ( tsa )) < 0 )
    {
        utLog("META: bind (TCP) failed: %s, exiting", strerror(errno) );
        perror( "TCP bind" );
        exit(1);
    }

    /* set the maximum connections we will fall behind */
    if (listen( t, LISTEN_BACKLOG ) < 0)
    {
        utLog("META: listen() failed: %s", strerror(errno));
        exit(1);
    }

    alen = sizeof(struct sockaddr);

    utLog("NET: meta server listening on TCP and UDP port %d\n", listenPort);

    /* go into infinite loop waiting for new connections */
    while (true)
    {
        tv.tv_sec = 30;           /* age servers every 30 secs */
        tv.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(s, &readfds);
        FD_SET(t, &readfds);

        if ((rv = select(std::max(s,t)+1, &readfds, NULL, NULL, &tv)) < 0)
        {
            utLog("META: select failed: %s, exiting", strerror(errno));
            exit(1);
        }

        /* TCP (list) socket */
        if (FD_ISSET(t, &readfds))
        {
            sockln = sizeof (isa);
            if ((tc = accept(t, (struct sockaddr *)&tisa, &sockln )) < 0)
            {
                utLog("META: accept failed: %s", strerror(errno) );
                continue;
            }

            if ((hp = gethostbyaddr((char *) &tisa.sin_addr.s_addr,
                                    sizeof(unsigned long),
                                    AF_INET)) == NULL)
            {
                utStrncpy(hostbuf, inet_ntoa((struct in_addr)tisa.sin_addr),
                          CONF_SERVER_NAME_SZ);
            }
            else
            {
                utStrncpy(hostbuf, hp->h_name, CONF_SERVER_NAME_SZ);
            }

            hostbuf[CONF_SERVER_NAME_SZ - 1] = 0;

            if (!tcpwCheckHostAccess(TCPW_DAEMON_CONQMETAD, hostbuf))
            {
                close(tc);
                continue;
            }

            metaProcList(tc, hostbuf);
            close(tc);
        }

        /* UDP (ping) socket */
        if (FD_ISSET(s, &readfds))
        {
            memset(rbuf, 0, META_MAX_PKT_SIZE);
            rlen = recvfrom(s, rbuf, META_MAX_PKT_SIZE, 0,
                            (struct sockaddr *)&isa, &alen);

            if ((hp = gethostbyaddr((char *) &isa.sin_addr.s_addr,
                                    sizeof(unsigned long),
                                    AF_INET)) == NULL)
            {
                utStrncpy(hostbuf, inet_ntoa((struct in_addr)isa.sin_addr),
                          CONF_SERVER_NAME_SZ);
            }
            else
            {
                utStrncpy(hostbuf, hp->h_name, CONF_SERVER_NAME_SZ);
            }

            hostbuf[CONF_SERVER_NAME_SZ - 1] = 0;

            if (!tcpwCheckHostAccess(TCPW_DAEMON_CONQMETAD, hostbuf))
            {
                continue;
            }

            metaProcUpd(rbuf, rlen, hostbuf);

        }

        /* timeout */
        ageServers();
    }

    return;			/* NOTREACHED */
}


/*  conqmetad - main program */
int main(int argc, char *argv[])
{
    int i;
    char *myuidname = NULL;              /* what user do I run under? */
    int dodaemon = false;

    progName = argv[0];
    utSetLogProgramName(progName);

    while ((i = getopt(argc, argv, "dlp:u:v")) != EOF)    /* get command args */
        switch (i)
        {
        case 'd':
            dodaemon = true;
            break;

        case 'p':
            listenPort = (uint16_t)atoi(optarg);
            break;

        case 'l':                 /* local conn only */
            localOnly = true;
            break;

        case 'u':
            myuidname = optarg;
            break;

        case 'v':
            cqDebug++;
            break;

        default:
            printUsage();
            exit(1);
        }

    if ((ConquestGID = getConquestGID()) == -1)
    {
        fprintf(stderr, "%s: getConquestGID() failed\n", progName);
        exit(1);
    }


    /* at this point, we see if the -u option was used.  If it was, we
       setuid() to it */

    if (myuidname)
    {
        int myuid;

        if ((myuid = getUID(myuidname)) == -1)
        {
            utLog("%s: getUID(%s) failed: %s\n", progName, myuidname,
                  strerror(errno));
            fprintf(stderr, "%s: getUID(%s) failed: %s\n", progName, myuidname,
                    strerror(errno));
            exit(1);
        }

        if (setuid(myuid) == -1)
        {
            utLog("%s: setuid(%d) failed: %s\n", progName, myuid,
                  strerror(errno));
            fprintf(stderr, "%s: setuid(%d) failed: %s\n", progName, myuid,
                    strerror(errno));
            exit(1);
        }
        else
            utLog("INFO: META running as user '%s', uid %d.", myuidname, myuid);
    }


#ifdef DEBUG_CONFIG
    utLog("%s@%d: main() Reading Configuration files.", __FILE__, __LINE__);
#endif

    if (GetSysConf(false) == -1)
    {
#ifdef DEBUG_CONFIG
        utLog("%s@%d: main(): GetSysConf() returned -1.", __FILE__, __LINE__);
#endif
    }

    /* if daemon mode requested, fork off and detach */
    if (dodaemon)
    {
        int cpid;
        utLog("INFO: becoming daemon");
        if (chdir("/") == -1)
        {
            utLog("chdir(/) failed: %s", strerror(errno));
            exit(1);
        }



        cpid = fork();
        switch (cpid)
        {
        case 0:
            /* child */

#if defined(HAVE_DAEMON)
            if (daemon(0, 0) == -1)
            {
                utLog("daemon() failed: %s", strerror(errno));
                exit(1);
            }

#else
# if defined(HAVE_SETPGRP)
#  if defined(SETPGRP_VOID)
            setpgrp ();
#  else
            setpgrp (0, getpid());
#  endif
# endif

            close (0);
            close (1);
            close (2);

            /* Set up the standard file descriptors. */

            (void) open ("/", O_RDONLY);        /* root inode already in core */
            (void) dup2 (0, 1);
            (void) dup2 (0, 2);

#endif /* !HAVE_DAEMON */

            break;

        case -1:
            /* error */
            utLog("%s: daemon fork failed: %s", progName, strerror(errno));
            fprintf(stderr, "daemon fork failed: %s\n", strerror(errno));
            break;

        default:
            /* parent */
            exit(0);
        }
    }

    // init the server list
    metaServerList.clear();

    /* setup, listen for, and process  client connections. */

    metaListen();

    exit(0);

}

void catchSignals(void)
{
    signal(SIGHUP, (void (*)(int))handleSignal);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTERM, (void (*)(int))handleSignal);
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, (void (*)(int))handleSignal);
    signal(SIGCLD, SIG_IGN);	/* allow children to die */
    signal(SIGPIPE, SIG_IGN);

    return;
}


void handleSignal(int sig)
{
    utLog("META: exiting on signal %d (%s)", sig, strsignal(sig));
    exit(0);
}
