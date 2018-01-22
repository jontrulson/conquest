#include "c_defs.h"

/************************************************************************
 *
 * conquestd - the Conquest server/client driver
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

#define NOEXTERN_GLOBALS
#include "global.h"

#define NOEXTERN_CONF
#include "conf.h"

#include "conqdef.h"
#include "cb.h"
#include "conqlb.h"
#include "rndlb.h"
#include "conqutil.h"
#include "conqai.h"
#include "conqunix.h"

#define NOEXTERN_CONTEXT
#include "context.h"

#include "color.h"
#include "record.h"
#include "ibuf.h"
#include "sem.h"

#include "conqnet.h"
#include "packet.h"
#include "udp.h"

#define NOEXTERN_SERVER
#include "server.h"
#include "serverpkt.h"

#include "servauth.h"
#include "servercmd.h"
#include "meta.h"

#include "conqinit.h"

#include "tcpwrap.h"

#define LISTEN_BACKLOG 5 /* # of requests we're willing to to queue */

static char cbuf[BUFFER_SIZE_1024]; /* general purpose buffer */
static char *progName;

static int localOnly = false;   /* whether to only listen on loopback */
static const char *metaServer = META_DFLT_SERVER; /* meta server hostname */
static int updateMeta = false;  /* whether to notify meta server */
static char *myServerName = NULL; /* to meta */

cpHello_t chello;		/* client hello info we want to keep */

static int hello(void);		/* meet and greet the client */
int updateClient(int force);
void updateProc(void);
void startUpdate(void);
void stopUpdate(void);

void catchSignals(void);
void handleSignal(int sig);
void conqend(void);
int welcome( int *unum );
void menu(void);
int play(void);

void printUsage()
{
    printf("Usage: conquestd [ -d ] [ -l ] [ -p port ] [ -u user ]\n");
    printf("                 [ -m ] [ -M metaserver ] [ -N myname ]\n");
    printf("\n");
    printf("   -d            daemon mode\n");
    printf("   -l            listen for local connections only\n");
    printf("   -p port       specify port to listen on\n");
    printf("                 default is %d\n", CN_DFLT_PORT);
    printf("   -m            notify the metaserver (%s)\n", META_DFLT_SERVER);
    printf("   -M metaserver specify an alternate metaserver to contact\n");
    printf("   -N myname     explicitly specify server name 'myname' to metaserver\n");
    printf("   -u user       run as user 'user'.\n");
    printf("   -v            be more verbose.\n");
    return;
}


int getHostname(int sock, char *buf, int buflen)
{
    struct sockaddr_in addr;
    socklen_t len;
    struct hostent *host;

    len = sizeof(struct sockaddr_in);
    if (getpeername(sock, (struct sockaddr *) &addr, &len) < 0)
    {
        utLog("getpeername failed: %s\n", strerror(errno));
        return false;
    }
    else
    {
        // copy the ip addr into the Context, for later deposit into
        // the ship structure.  We store it in the last 4 bytes -
        // eventually we would like to be able to handle IP6 addresses
        // too (16 bytes long), which would start at 0.
        memcpy(Context.ipaddr + 12, &addr.sin_addr, 4);

        if ((host = gethostbyaddr((char *) &addr.sin_addr.s_addr,
                                  sizeof(unsigned long),
                                  AF_INET)) == NULL)
	{
            utStrncpy(buf, inet_ntoa((struct in_addr)addr.sin_addr),
                      buflen);
	}
        else
        {
            utStrncpy(buf, host->h_name, buflen);
        }
    }

    return true;
}


/* we only return if we are a client driver, else we listen for requests,
   updating the meta server if requested */
void checkMaster(void)
{
    int s,t;			/* socket descriptor */
    int rv;			/* general purpose integer */
    socklen_t sockln;
    struct sockaddr_in sa, isa;	/* internet socket addr. structure */
    struct timeval tv;
    fd_set readfds;
    static const int optOn = 1;
    int opt = optOn;

    signal(SIGCLD, SIG_IGN);	/* allow children to die */

    if (!checkPID(cbConqInfo->conqservPID))
    {				/* see if one is really running */
        /* if we are here, we will be the listener */
        cbLock(&cbConqInfo->lockword);
        cbConqInfo->conqservPID = getpid();
        cbUnlock(&cbConqInfo->lockword);
        sInfo.isMaster = true;
        utLog("NET: master server listening on port %d\n", sInfo.listenPort);
    }

    sa.sin_port = htons(sInfo.listenPort);

    if (localOnly)
        sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    else
        sa.sin_addr.s_addr = htonl(INADDR_ANY); /* Bind to all addresses.
                                                   -Werewolf */
    sa.sin_family = AF_INET;

    /* allocate an open socket for incoming connections */
    if (( s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        utLog("NET: socket() failed: %s", strerror(errno));
        exit(1);
    }

#if defined(SO_REUSEPORT)
    /* set reuse port */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEPORT,
                   (SSOType)&opt, sizeof(opt)) < 0)
    {
        utLog("NET: setsockopt SO_REUSEPORT: %s", strerror(errno));
    }

#endif

#if defined(SO_REUSEADDR)
    /* set reuse address */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
                   (SSOType)&opt, sizeof(opt)) < 0)
    {
        utLog("NET: setsockopt SO_REUSEADDR: %s",
              strerror(errno));
    }
#endif

    /* bind the socket to the service port so we hear incoming
     * connections
     */
    if ( bind( s, (struct sockaddr *)&sa, sizeof ( sa )) < 0 )
    {
        utLog("NET: bind() failed: %s", strerror(errno));
        exit(1);
    }

    /* set the maximum connections we will fall behind */
    listen( s, LISTEN_BACKLOG );


    /* go into infinite loop waiting for new connections */
    while (true)
    {
        if (updateMeta)
        {
            /* get any changes to sysconf so that meta updates are
               up to date */
            GetSysConf(true);
            metaUpdateServer(metaServer, myServerName, sInfo.listenPort);
        }

        tv.tv_sec = 120;           /* update meta server every 120 secs */
        tv.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(s, &readfds);

        if ((rv = select(s+1, &readfds, NULL, NULL, &tv)) < 0)
        {
            utLog("checkMaster: select failed: %s", strerror(errno));
            exit(1);
        }

        if (FD_ISSET(s, &readfds))
        {

            sockln = (socklen_t)sizeof (isa);

            /* hang in accept() while waiting for new connections */
            if ((t = accept(s, (struct sockaddr *)&isa, &sockln )) < 0)
            {
                perror ( "accept" );
                exit (1);
            }

            if ( fork() == 0 )
            {			/* child - client driver */
                utLog("NET: forked client driver, pid = %d", getpid());
                sInfo.sock = t;

                pktSetSocketFds(sInfo.sock, PKT_SOCKFD_NOCHANGE);

                memset(sInfo.remotehost, 0, MAXHOSTNAME);
                getHostname(sInfo.sock, sInfo.remotehost, MAXHOSTNAME);
                if (!tcpwCheckHostAccess(TCPW_DAEMON_CONQUESTD,
                                         sInfo.remotehost))
                {
                    // flag it for the hello packet.  This will set
                    // the appropriate flag and terminate the
                    // connection.

                    Context.accessDenied = true;
                }
                else
                    Context.accessDenied = false;

                pktSetNodelay();

                return;
            }

            /* parent */
            close(t);	/* make socket go away */
        }

    }

    return;			/* NOTREACHED */
}


/*  conquestd - main program */
int main(int argc, char *argv[])
{
    int i;
    char *myuidname = NULL;              /* what user do I run under? */
    int dodaemon = false;

    progName = argv[0];
    sInfo.state = SVR_STATE_PREINIT;
    sInfo.sock = -1;
    sInfo.usock = -1;
    sInfo.doUDP = false;
    sInfo.clientDead = true;
    sInfo.isMaster = false;
    sInfo.listenPort = CN_DFLT_PORT; // 1701, of course


    while ((i = getopt(argc, argv, "dlp:u:mM:N:v")) != EOF)    /* get command args */
        switch (i)
        {
        case 'd':
            dodaemon = true;
            break;

        case 'p':
            sInfo.listenPort = (uint16_t)atoi(optarg);
            break;

        case 'l':                 /* local conn only */
            localOnly = true;
            break;

        case 'u':
            myuidname = optarg;
            break;

        case 'm':
            updateMeta = true;
            break;

        case 'M':
            metaServer = optarg;
            break;

        case 'N':
            myServerName = optarg;
            break;

        case 'v':
            cqDebug++;
            break;

        default:
            printUsage();
            exit(1);
        }


    rndini();		/* initialize random numbers */

    if (!pktInit())
    {
        fprintf(stderr, "pktInit failed, exiting\n");
        utLog("pktInit failed, exiting");
    }
    pktSetSocketFds(sInfo.sock, sInfo.usock);

    /* need to make sure that the serverPackets array is setup */
    pktSetClientProtocolVersion(PROTOCOL_VERSION);

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
            fprintf(stderr, "%s: getUID(%s) failed\n", progName, myuidname);
            exit(1);
        }

        if (setuid(myuid) == -1)
        {
            fprintf(stderr, "%s: setuid(%d) failed: %s\n", progName, myuid,
                    strerror(errno));
            exit(1);
        }
        else
            utLog("INFO: running as user '%s', uid %d.", myuidname, myuid);
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

    if (setgid(ConquestGID) == -1)
    {
        utLog("conquest: setgid(%d): %s",
              ConquestGID,
              strerror(errno));
        fprintf(stderr, "conquest: setgid(): failed\n");
        exit(1);
    }

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() *STARTING*", __FILE__, __LINE__);
#endif

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() getting semephores - semInit()", __FILE__, __LINE__);
#endif

    if (semInit() == -1)
    {
        fprintf(stderr, "semInit() failed to get semaphores. exiting.\n");
        exit(1);
    }

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() mapping common block.", __FILE__, __LINE__);
#endif

    // load the globals/planets (conqinitrc), before we map...
    if (cqiLoadRC(CQI_FILE_CONQINITRC, NULL, 1, 0))
    {
        fprintf(stderr, "FATAL: can't load conqinitrc file.\n");
        exit(1);
    }
    cbMap();

    if ( *cbRevision != COMMONSTAMP )
    {
        fprintf(stderr,"conquestd: Common block ident mismatch.\n" );
        fprintf(stderr,"           You must initialize the universe with conqoper.\n" );
        exit(1);
    }


    /* clear out our stored packets */
    spktInit();

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() starting conqinit().", __FILE__, __LINE__);
#endif

    Context.maxlin = 0;		/* not used here */
    Context.maxcol = 0;		/* not used here */

    Context.snum = 0;
    Context.histslot = -1;
    Context.lasttang = Context.lasttdist = 0;
    Context.lasttarg[0] = 0;
    Context.updsec = 10;		/* 10 per second default update rate */


    /* if daemon mode requested, fork off and detach */
    if (dodaemon)
    {
        int cpid;
        utLog("INFO: becoming daemon");
        if (chdir("/") == -1)
        {
            fprintf(stderr,"chdir(/) failed: %s\n", strerror(errno));
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
                fprintf(stderr,"daemon(0, )) failed: %s\n", strerror(errno));
                exit(1);
            }
#else
# if defined(HAVE_SETPGRP)
#  if defined(SETPGRP_VOID)
            setpgrp();
#  else
            setpgrp(0, getpid());
#  endif
# endif

            close(0);
            close(1);
            close(2);

            /* Set up the standard file descriptors. */

            (void) open("/", O_RDONLY);        /* root inode already in core */
            (void) dup2(0, 1);
            (void) dup2(0, 2);

#endif /* !HAVE_DAEMON */

            break;

        case -1:
            /* error */
            fprintf(stderr, "daemon fork failed: %s\n", strerror(errno));
            exit(1);
            break;

        default:
            /* parent */
            exit(0);
        }
    }

    /* see if we are master server.  If we are, we won't return from this
       call.  If we are a forked client driver, we will. */
    checkMaster();

    /* if we are here, then we are a client driver, with an active socket */
    sInfo.state = SVR_STATE_INIT;

    /* re-read sys conf here for individual client drivers... */
    if (GetSysConf(false) == -1)
    {
#ifdef DEBUG_CONFIG
        utLog("%s@%d: main(): GetSysConf() returned -1.", __FILE__, __LINE__);
#endif
    }

    rndini();		/* initialize random numbers (again) */

    conqinit();                 /* basic client Context initialization */
    Context.recmode = RECMODE_OFF; /* always */

    utLog("CONNECT: client %s", sInfo.remotehost);

    /* now we need to negotiate. */
    if (!hello())
    {
        utLog("conquestd: hello() failed");
        exit(1);
    }

    utLog("CONNECT: client %s SUCCESS.", sInfo.remotehost);

    if ( welcome( &Context.unum ) )
    {
        sInfo.state = SVR_STATE_MAINMENU;

        menu();
    }

    drpexit();			/* make the driver go away */
    conqend();			/* machine dependent clean-up */

#ifdef DEBUG_FLOW
    utLog("%s@%d: main() *EXITING*", __FILE__, __LINE__);
#endif

    exit(0);

}

/* responsible for updating the client */
void updateProc(void)
{
    /* Don't do anything if we're not supposed to. */
    if (sInfo.state != SVR_STATE_PLAY)
        return;

    stopUpdate();

#if 0
    {
        /* debugging code to see update latencies */
        static uint32_t lastms = 0;
        uint32_t millis = clbGetMillis();

        if ((millis - lastms) > (int)((1.0 / (real)Context.updsec) * 1000.0))
            utLog("millisdiff = %u(%d)", (millis - lastms), Context.updsec);
        lastms = millis;
    }
#endif

    /* update client view of the universe */
    updateClient(false);

    recUpdateFrame();

    /* check for and send any new messages */
    if ( utGetMsg( Context.snum, &cbShips[Context.snum].lastmsg ) )
        sendMessage(&(cbMsgs[cbShips[Context.snum].lastmsg]));

    /* Schedule for next time. */
    startUpdate();

    return;

}

void startUpdate(void)
{
    static struct sigaction Sig;

#ifdef HAVE_SETITIMER
    struct itimerval itimer;
#endif

    Sig.sa_handler = (void (*)(int))updateProc;

    Sig.sa_flags = 0;

    if (sigaction(SIGALRM, &Sig, NULL) == -1)
    {
        utLog("startUpdater():sigaction(): %s\n", strerror(errno));
        exit(errno);
    }

#ifdef HAVE_SETITIMER
    if (Context.updsec >= 1 && Context.updsec <= 10)
    {
        if (Context.updsec == 1)
	{
            itimer.it_value.tv_sec = 1;
            itimer.it_value.tv_usec = 0;
	}
        else
	{
            itimer.it_value.tv_sec = 0;
            itimer.it_value.tv_usec = (1000000 / Context.updsec);
	}
    }
    else
    {
        itimer.it_value.tv_sec = 0;
        itimer.it_value.tv_usec = (1000000 / 2); /* 2/sec */
    }

    itimer.it_interval.tv_sec = itimer.it_value.tv_sec;
    itimer.it_interval.tv_usec = itimer.it_value.tv_usec;

    setitimer(ITIMER_REAL, &itimer, NULL);
#else
    alarm(1);			/* set alarm() */
#endif

    return;
}

void stopUpdate(void)
{
#ifdef HAVE_SETITIMER
    struct itimerval itimer;
#endif

    signal(SIGALRM, SIG_IGN);

#ifdef HAVE_SETITIMER
    itimer.it_value.tv_sec = itimer.it_interval.tv_sec = 0;
    itimer.it_value.tv_usec = itimer.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &itimer, NULL);
#else
    alarm(0);
#endif

    return;

}



/*  capentry - captured system entry for a new ship */
/*  SYNOPSIS */
/*    int flag, capentry */
/*    int capentry, snum, system */
/*    system = capentry( snum, system ) */
int capentry( int snum, int *system )
{
    int i, j;
    bool owned[NUMPLAYERTEAMS] = {};
    int pkttype;
    cpCommand_t *ccmd;
    char buf[PKT_MAXSIZE];
    uint8_t esystem = 0;

    /* First figure out which systems we can enter from. */

    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    {
        /* We must own all home planets of a team (defendteam). */
        for ( j=0; j<cbLimits.maxPlanets(); j++ )
	{
            if (PHOMEPLANET(j) && cbPlanets[j].defendteam == i)
            {
                // if we don't own it, bail
                if (cbPlanets[j].team != cbShips[snum].team)
                    break;
            }
	}

        if (j < cbLimits.maxPlanets())
            owned[i] = false;
        else
            owned[i] = true;
    }

    owned[cbShips[snum].team] = true;  // we can always enter our own
                                     // system regardless of who owns
                                     // it

    /* Now count how many systems we can enter from. */
    j = 0;
    esystem = 0;
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        if ( owned[i] )
        {
            esystem |= (1 << i);
            j++;
        }

    /* If we can only enter from one, we're done. */
    if ( j <= 1 )
    {
        *system = cbShips[snum].team; /* tell the client */
        if (!sendClientStat(sInfo.sock, SPCLNTSTAT_FLAG_NONE, snum,
                            cbShips[snum].team,
                            Context.unum, 0))
            return false;
        return true;
    }

    /* ask the client - use a clientstat with non-zero esystem */

    if (!sendClientStat(sInfo.sock, SPCLNTSTAT_FLAG_NONE, snum, cbShips[snum].team,
                        Context.unum, esystem))
        return false;

    while ( clbStillAlive( Context.snum ) )
    {

        /* now we wait for another ENTER command packet with it's detail member
           indicating the desired system. */

        if ((pkttype = pktWaitForPacket(CP_COMMAND,
                                        buf, PKT_MAXSIZE, 1, NULL)) < 0)
        {
            utLog("conquestd:capentry: waitforpacket returned %d", pkttype);
            return false;
        }


        if ( pkttype == 0 )       /* timeout */
            continue;

        ccmd = (cpCommand_t *)buf;
        ccmd->detail = ntohs(ccmd->detail);

        utLog("conquestd: capentry: got CP_COMMAND, detail =0x%x",
              ccmd->detail);

        if (ccmd->cmd != CPCMD_ENTER)
	{			/* we'll just use the home team */
            *system = cbShips[snum].team;
            return true;
	}

        if (ccmd->detail == 0)	/* didn't want to select one */
            return false;

        /* else we'll use the first set bit */
        esystem &= (uint8_t)(ccmd->detail & 0x00ff);

        for ( i = 0; i < NUMPLAYERTEAMS; i++ )
            if (esystem & (1 << i))
            {
                *system = i;
                return true;
            }

        /* shouldn't happen, but... */
        *system = cbShips[snum].team;
        return true;
    }

    return false;	    /* can get here because of clbStillAlive() */

}


/*  dead - do dead-type stuff for the server. (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    int leave */
/*    dead( snum, leave ) */
void dead( int snum )
{
    int i, j;
    unsigned int entertime;
    killedBy_t kb;
    unsigned int detail;
    uint8_t flags = SPCLNTSTAT_FLAG_NONE; /* for clientstat msg */
    char buf[PKT_MAXSIZE];	/* gen purpose */

    /* If something is wrong, don't do anything. */
    if ( snum < 0 || snum >= cbLimits.maxShips() )
        return;

    /* If our ships pid is wrong, we are indeed lost. */
    if ( cbShips[snum].pid != Context.pid )
        return;

    kb = cbShips[snum].killedBy;
    detail = cbShips[snum].killedByDetail;

    /* Delay while our torps are exploding. */
    entertime = clbGetMillis();
    i = 0;
    while ( (clbGetMillis() - entertime) < TORPEDOWAIT_GRAND )
    {
        updateClient(false);
        i = 0;
        for ( j = 0; j < cbLimits.maxTorps(); j++ )
            if ( cbShips[snum].torps[j].status == TS_DETONATE )
                i++;
        if ( i <= 0 )
            break;
        utSleep( (1.0 / (real)Context.updsec) );
    }

    /* There aren't supposed to be any torps left. */
    if ( i > 0 )
    {
        buf[0] = 0;
        utAppendShip(buf , snum) ;
        utLog("INFO: dead: %s, detonating torp count is %d.",
              buf, i);
    }

    buf[0] = 0;
    utAppendShip(buf , snum) ;
    utLog("INFO: dead: %s was killed by %d(%d).", buf, (int)kb, (int)detail);

    updateClient(false);
    for ( i=0; i<10 && cbShips[snum].status == SS_DYING; i++ )
    {
        utSleep( (1.0 / (real)Context.updsec) );
        updateClient(false);
    }

    /* if you conquered the universe, let the client know, and wait for
       a cpMessage packet indicating the desired last words.  We'll wait up
       to 2 minutes before we decide on our own. */

    flags |= SPCLNTSTAT_FLAG_KILLED; /* you are quite dead */

    if (kb == KB_CONQUER)
        flags |= SPCLNTSTAT_FLAG_CONQUER;

    /* send the clientstat */
    if (!sendClientStat(sInfo.sock, flags, Context.snum, cbUsers[Context.unum].team,
                        Context.unum, 0))
    {				/* an error, let the ai code choose some
				   last words and bail */
        if (kb == KB_CONQUER)
        {
            robreply(buf);
            utStrncpy(cbConqInfo->lastwords, buf, MAXLASTWORDS);
        }

        utLog("conquestd: dead(): sendClientStat failed, fl = 0x%0x\n",
              flags);

        return;
    }

    /* fix things up */
    cbShips[snum].status = SS_RESERVED;
    cbShips[snum].sdfuse = -TIMEOUT_PLAYER;
    /*  cbShips[snum].killedby = 0;*/

    /* let the client know. */
    updateClient(false);

    /* if conquered, wait for the cpMessage_t */
    if (kb == KB_CONQUER)
    {
        if (pktWaitForPacket(CP_MESSAGE,
                             buf, PKT_MAXSIZE,
                             (60 * 5), NULL) <= 0)
	{			/* error or timeout.  gen lastwords */
            robreply(buf);
            utStrncpy(cbConqInfo->lastwords, buf, MAXLASTWORDS);

            return;
	}
        else
	{
            cpMessage_t *cmsg = (cpMessage_t *)buf;

            /* copy as much of the message as you can. */
            utStrncpy(cbConqInfo->lastwords, (char *)cmsg->msg, MAXLASTWORDS);
	}
    }

    /* Turn off sticky war so we can change war settings from menu(). */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        cbShips[snum].rwar[i] = false;

    return;

}

/* send all pertinent data, and any users attached to them. */
int updateClient(int force)
{
    int i,j;
    static int sentallusers = false; /* we will send all user data once. */
    static time_t oldtime = 0;
    time_t newtime = time(0);
    int seciter = false;
    static time_t histtime = 0;   /* timers that try to save some time() */
    static time_t infotime = 0;
    static time_t teamtime = 0;
    int dohist = false;
    int doinfo = false;
    int doteam = false;

    if (force)
    {                           /* we need to reload everything */
        oldtime = 0;
        histtime = 0;
        infotime = 0;
        teamtime = 0;
        sentallusers = false;
    }


    /* some things really should not be checked every update iter */
    if (oldtime != newtime)
    {
        seciter = true;
        oldtime = newtime;
    }

    /* hist */
    if ((abs(int(newtime - histtime)) >
         HISTORY_UPDATE_INTERVAL))
    {
        dohist = true;
        histtime = newtime;
    }

    /* info */
    if ((abs(int(newtime - infotime)) >
         CONQINFO_UPDATE_INTERVAL))
    {
        doinfo = true;
        infotime = newtime;
    }

    /* team */
    if ((abs(int(newtime - teamtime)) >
         TEAM_UPDATE_INTERVAL))
    {
        doteam = true;
        teamtime = newtime;
    }


    if (!sentallusers)
    {                           /* send all valid user data the first time */
        sentallusers = true;
        for (i=0; i<cbLimits.maxUsers(); i++)
            if (ULIVE(i))
                if (!sendUser(sInfo.sock, i))
                    return false;
    }

    for (i=0; i<cbLimits.maxShips(); i++)
    {
        if (!sendShip(sInfo.sock, i))
            return false;

        for (j=0; j<cbLimits.maxTorps(); j++)
            if (!sendTorp(sInfo.sock, i, j))
                return false;

        /* we only send user data for active ships. */
        if (cbShips[i].status != SS_OFF)
	{
            if (seciter)
            {
                if (!sendUser(sInfo.sock, cbShips[i].unum))
                {
                    return false;
                }
            }
        }
    }

    for (i=0; i<cbLimits.maxPlanets(); i++)
        sendPlanet(sInfo.sock, i, force);

    if (doteam)
        for (i=0; i<NUMALLTEAMS; i++)
            sendTeam(sInfo.sock, i, false);

    if (doinfo)
        sendcbConqInfo(sInfo.sock, false);

    if (dohist)
    {
        for (i=0; i<cbLimits.maxHist(); i++)
        {
            sendHistory(sInfo.sock, i);
        }
    }

    sendDoomsday(sInfo.sock);

    return true;
}


/* handle simple client commands */
void handleSimpleCmdPkt(cpCommand_t *ccmd)
{
    int cmd;

    if (!ccmd)
        return;

    cmd = ccmd->cmd;
    /*  Some commands are only available in certain states */

    switch (cmd)
    {
    case CPCMD_SWITCHTEAM:
        if (sInfo.state == SVR_STATE_MAINMENU)
	{
            int team = (int)ntohs(ccmd->detail);

            if (team >= 0 && team < NUMPLAYERTEAMS)
	    {

                cbShips[Context.snum].team = team;
                cbShips[Context.snum].shiptype =
                    cbTeams[cbShips[Context.snum].team].shiptype;
                cbUsers[Context.unum].team = cbShips[Context.snum].team;
                cbShips[Context.snum].war[cbShips[Context.snum].team] = false;
                cbUsers[Context.unum].war[cbUsers[Context.unum].team] = false;
	    }
	}

        break;

    case CPCMD_SETWAR:
        if (sInfo.state == SVR_STATE_MAINMENU || sInfo.state == SVR_STATE_PLAY)
            procSetWar(ccmd);

        break;

    case CPCMD_SETWARP:
        if (sInfo.state == SVR_STATE_PLAY)
            procSetWarp(ccmd);

        break;

    case CPCMD_SETSHIELDS:
        if (sInfo.state == SVR_STATE_PLAY)
            procSetShields(ccmd);

        break;

    case CPCMD_ALLOC:
        if (sInfo.state == SVR_STATE_PLAY)
            procAlloc(ccmd);

        break;

    case CPCMD_CLOAK:
        if (sInfo.state == SVR_STATE_PLAY)
            procCloak(ccmd);

        break;

    case CPCMD_DETSELF:
        if (sInfo.state == SVR_STATE_PLAY)
            procDetSelf(ccmd);

    case CPCMD_DETENEMY:
        if (sInfo.state == SVR_STATE_PLAY)
            procDetEnemy(ccmd);

        break;

    case CPCMD_DISTRESS:
        if (sInfo.state == SVR_STATE_PLAY)
            procDistress(ccmd);

        break;

    case CPCMD_REPAIR:
        if (sInfo.state == SVR_STATE_PLAY)
            procRepair(ccmd);

        break;

    case CPCMD_FIREPHASER:
        if (sInfo.state == SVR_STATE_PLAY)
            procFirePhaser(ccmd);

        break;

    case CPCMD_ORBIT:
        if (sInfo.state == SVR_STATE_PLAY)
            procOrbit(ccmd);

        break;

    case CPCMD_REFIT:
        if (sInfo.state == SVR_STATE_PLAY)
            procRefit(ccmd);

        break;

    case CPCMD_SETRATE:
        procSetRate(ccmd);

        break;

    case CPCMD_TOW:
        if (sInfo.state == SVR_STATE_PLAY)
            procTow(ccmd);

        break;

    case CPCMD_UNTOW:
        if (sInfo.state == SVR_STATE_PLAY)
            procUnTow(ccmd);

        break;

    case CPCMD_COUP:
        if (sInfo.state == SVR_STATE_PLAY)
            procCoup(ccmd);

        break;

    case CPCMD_BOMB:
        if (sInfo.state == SVR_STATE_PLAY)
            procBomb(ccmd);

        break;

    case CPCMD_BEAM:
        if (sInfo.state == SVR_STATE_PLAY)
            procBeam(ccmd);

        break;

    case CPCMD_DESTRUCT:
        if (sInfo.state == SVR_STATE_PLAY)
            procDestruct(ccmd);

        break;

    case CPCMD_AUTOPILOT:
        if (sInfo.state == SVR_STATE_PLAY)
            procAutoPilot(ccmd);

        break;

    case CPCMD_RELOAD:
        clbBlockAlarm();
        procReload(ccmd);
        updateClient(true);
        clbUnblockAlarm();

        break;

    case CPCMD_DISCONNECT:
        utLog("CPCMD_DISCONNECT");
        /* 'fake' signal.  cleans up and exits. */
        handleSignal(0);
        /* NOTREACHED */
        break;

    case CPCMD_UDP:
        clbBlockAlarm();
        serverStartUDP(getpid(), ccmd);
        clbUnblockAlarm();
        break;

// These will never be seen here - they are intercepted by the packet
// reader and dealt with there.
//    case CPCMD_PING:
//    case CPCMD_KEEPALIVE:

    default:
        utLog("conquestd: handleSimpleCmdPkt(): unexpected command code %d",
              cmd);
        break;
    }

    return;
}

/* DOES LOCKING */
void freeship(void)
{
    conqstats( Context.snum );
    cbLock(&cbConqInfo->lockword);
    cbShips[Context.snum].sdfuse = 0;
    cbShips[Context.snum].status = SS_OFF;
    cbUnlock(&cbConqInfo->lockword);
    return;
}

/*  menu - main user menu (DOES LOCKING) */
/*  SYNOPSIS */
/*    menu */
void menu(void)
{
    int i;
    uint32_t sleepy;
    int lose;
    int playrv;
    int pkttype;
    char buf[PKT_MAXSIZE];
    cpCommand_t *ccmd;
    static const uint32_t sleeplimit = ((1000 * 60) * 5); /* 5 minutes */

    catchSignals();	/* enable trapping of interesting signals */

    /* we'll set some things up,  */

    /* Initialize statistics. */
    initstats( &cbShips[Context.snum].ctime, &cbShips[Context.snum].etime );

    /* Log this entry into the Game. */
    Context.histslot = clbLogHist( Context.unum );

    /* Set up a few ship characteristics here rather than in clbInitShip(). */
    cbShips[Context.snum].unum = Context.unum;
    cbShips[Context.snum].team = cbUsers[Context.unum].team;
    cbShips[Context.snum].shiptype = cbTeams[cbShips[Context.snum].team].shiptype;

    cbShips[Context.snum].pid = Context.pid;
    cbShips[Context.snum].killedBy = KB_NONE;
    cbShips[Context.snum].killedByDetail = 0;

    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
        cbShips[Context.snum].rwar[i] = false;
        cbShips[Context.snum].war[i] = cbUsers[Context.unum].war[i];
    }

    // if NoTeamWar is set, clear war with own team regardless of settings
    if (SysConf.NoTeamWar)
        cbShips[Context.snum].war[cbShips[Context.snum].team] = false;

    utStrncpy( cbShips[Context.snum].alias, cbUsers[Context.unum].alias,
               MAXUSERNAME );

    /* Set up some things for the menu display. */
    sleepy = clbGetMillis();
    playrv = false;

    /* send a ship packet for our ship.  Since this is the first ship packet,
       an SP_SHIP should get sent. */
    if (!sendShip(sInfo.sock, Context.snum))
    {				/* something went wrong */
        /* Make our ship available for others to use. */
        if ( cbShips[Context.snum].status == SS_RESERVED )
	{
            freeship();
            return;
	}
    }

    do
    {
        if (!updateClient(false))	/* sends packets */
	{
            freeship();
            return;
	}

        /* Make sure things are proper. */
        if (playrv == ERR)
	{
            if ( Context.snum < 0 || Context.snum >= cbLimits.maxShips() )
                lose = true;
            else if ( cbShips[Context.snum].pid != Context.pid )
                lose = true;
            else if ( cbShips[Context.snum].status != SS_RESERVED )
	    {
                utLog( "menu(): Ship %d no longer reserved.", Context.snum );
                lose = true;
	    }
            else
                lose = false;
	}
        else
            lose = false;

        if ( lose )				/* again, Jorge? */
	{
            pktSendAck(PSEV_FATAL, PERR_LOSE, NULL);
            return;
	}

        /* Reset up the destruct fuse. */
        cbShips[Context.snum].sdfuse = -TIMEOUT_PLAYER;

        if ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                        buf, PKT_MAXSIZE, 0, NULL)) < 0)
	{
            freeship();
            utLog("conquestd:menu: waitforpacket returned %d", pkttype);
            handleSignal(0);
            /* not reached */

            return;
	}

        /* we get sleepy if we are recieving no packets */
        if ( pkttype == 0)
	{
            if ((clbGetMillis() - sleepy) > sleeplimit)
            {
                // tell the client
                utLog("%s: Idle timeout after 5 minutes of inactivity, "
                      "exiting.", __FUNCTION__);
                pktSendAck(PSEV_FATAL, PERR_IDLETIMEOUT, NULL);

                break;
            }
            else
                utSleep(0.05);

            continue;
	}

        switch(pkttype)
	{
	case CP_ACKUDP:
            procAckUDP(buf);
            break;

	case CP_SETNAME:
            procSetName(buf);
            break;

	case CP_AUTHENTICATE:
            procChangePassword(buf);
            break;

	case CP_COMMAND:
            ccmd = (cpCommand_t *)buf;
            if (ccmd->cmd == CPCMD_ENTER)
	    {			/* time to play */
                playrv = play();
                sInfo.state = SVR_STATE_MAINMENU;
                break;
	    }
            else if (ccmd->cmd == CPCMD_RESIGN)
	    {
                for ( i = 0; i < cbLimits.maxShips(); i++ )
                    if ( !((cbShips[i].status == SS_LIVE ||
                            cbShips[i].status == SS_ENTERING) &&
                           cbShips[i].unum == Context.unum))
                    {
                        clbResign( Context.unum, false );
                        cbShips[Context.snum].status = SS_OFF;
                        exit(0);	/* exit here */
                    }
	    }
            else
	    {
                handleSimpleCmdPkt((cpCommand_t *)buf);
	    }

            break;

	default:
            if (cqDebug)
                utLog("conquestd: MENU: got unexp packet type %d", pkttype);
            break;
	}

        /* Got something, reset timeout. */

        sleepy = clbGetMillis();
    }
    while ( clbStillAlive( Context.snum ) );

    /* Make our ship available for others to use. */
    if ( cbShips[Context.snum].status == SS_RESERVED )
        freeship();

    /* Try to kill the driver if we started one the last time */
    /*  we played and we've been in the menu long enough. */
    drkill();

    return;

}


/*  newship - create a new ship for a user (DOES LOCKING) */

/*  here we will find a ship for the client.  If the client has a VACANT
    ship it will return that ship, else it will get a new one.  No more
    multiples (for clients) supported currently I'm afraid..., if the
    client would be able to enter from multiple systems, this will be
    indicated by a non-zero esystems member of the clnt stat pkt */

/*  SYNOPSIS */
/*    flag = newship( unum, snum ) */
int newship( int unum, int *snum )
{
    int i, system;
    int fresh;

    /* cleanup any unliving ships - this is the first thing we need to do */
    clbCheckShips(false);

    cbLock(&cbConqInfo->lockword);

    cbShips[*snum].status = SS_ENTERING;		/* show intent to fly */

    fresh = true;				/* assume we want a fresh ship*/

    /* See if she is already flying at least one ship. */
    int j = -1;
    for ( i = 0; i < cbLimits.maxShips(); i++ )
        if ( cbShips[i].status == SS_LIVE || cbShips[i].status == SS_ENTERING )
            if ( cbShips[i].unum == unum && *snum != i )
            {
                j = i;
                break;
            }

    cbUnlock(&cbConqInfo->lockword);

    /* see if we need to reincarnate to a vacant ship */
    if ( j >= 0 )
    {
        /* Need to reincarnate. */

        if (!SVACANT(j))
        {		   /* if it's available, we'll take it */
			   /* ...if it's not already being flown... */
            pktSendAck(PSEV_ERROR, PERR_FLYING, NULL);
            cbShips[*snum].status = SS_RESERVED;
            return ( false );
        }


        /* Look for a live ship for us to take. */
        cbLock(&cbConqInfo->lockword);
        for (i=0; i<cbLimits.maxShips(); i++)
            if ( cbShips[i].unum == unum && cbShips[i].status == SS_LIVE )
            {
                fresh = false;
                cbShips[*snum].status = SS_OFF;
                *snum = i;
                cbShips[*snum].pid = Context.pid;
                cbShips[*snum].status = SS_ENTERING;
                SFCLR(*snum, SHIP_F_VACANT);
                break;
            }
        cbUnlock(&cbConqInfo->lockword);
    }

    /* Figure out which system to enter. */
    if ( fresh )
    {
        /* (re)init the ship's team! (bug 1/10/98) */
        cbShips[*snum].team = cbUsers[Context.unum].team;
        cbShips[*snum].shiptype = cbTeams[cbShips[*snum].team].shiptype;
        system = cbShips[*snum].team;

        if ( ! capentry( *snum, &system ) )
	{
            cbShips[*snum].status = SS_RESERVED;
            return ( false );
	}
    }
    else
    {
        /* now we clear ship's elapsed/cpu seconds
           so that there won't be a huge addition to
           the Teams/Users/Ships timing stats when
           a VACANT ships re-enters Conquest (in
           case the signal handler didn't get a
           chance to run) */
        cbShips[*snum].ctime = 0;
        cbShips[*snum].etime = 0;
    }

    cbLock(&cbConqInfo->lockword);

    /* If necessary, initalize the ship */
    if ( fresh )
    {
        clbInitShip( *snum, unum );

        /* Randomly position the ship near the home sun (or planet). */
        if (!clbFindTeamHomeSun(system, &i))
        {
            // couldn't find a home sun, use homeplanet
            i = cbTeams[system].homeplanet;
        }
        // good to go
        clbPutShip( *snum, cbPlanets[i].x, cbPlanets[i].y );
        cbShips[*snum].dhead = rnduni( 0.0, 359.9 );
        cbShips[*snum].head = cbShips[*snum].dhead;
        cbShips[*snum].dwarp = (real) rndint( 2, 5 );
        // lock onto the homeplanet
        cbShips[*snum].lock = LOCK_PLANET;
        cbShips[*snum].lockDetail = (uint16_t)cbTeams[system].homeplanet;
    }
    else
    {				/* if we're reincarnating, skip any
				   messages that might have been sent
				   while we were gone */
        cbLock(&cbConqInfo->lockmesg);
        cbShips[*snum].lastmsg = cbConqInfo->lastmsg;
        cbShips[*snum].alastmsg = cbShips[*snum].lastmsg;
        cbUnlock(&cbConqInfo->lockmesg);
        /* init user's last entry time */
        cbUsers[cbShips[*snum].unum].lastentry = time(0);
    }

    SFCLR(*snum, SHIP_F_ROBOT);
    cbShips[*snum].action = 0;


    /* Straighten out the ships deltas. */
    clbFixDeltas( *snum );

    /* Finally, turn the ship on. */
    cbShips[*snum].status = SS_LIVE;

    // copy in the ship's IP address (16 bytes)
    memcpy(cbShips[*snum].ipaddr, Context.ipaddr, SHIP_IPADDR_LEN);

    cbUnlock(&cbConqInfo->lockword);
    Context.entship = true;
    return ( true );

}


/*  play - play the game (PLAY) */
/*  SYNOPSIS */
/*    play */
int play(void)
{
    unsigned int laststat, now;
    int didsomething;             /* update immediately if we did anything */
    int rv;
    char msgbuf[128];
    int pkttype;
    char buf[PKT_MAXSIZE];

    /* Can't carry on without a vessel. */
    if ( (rv = newship( Context.unum, &Context.snum )) != true)
    {
        utLog("conquestd:play: newship() returned %d",
              rv);
        return(rv);
    }

    drstart();			/* start a driver, if necessary */
    cbShips[Context.snum].sdfuse = 0;	/* zero self destruct fuse */
    utGrand( &Context.msgrand );		/* initialize message timer */
    Context.redraw = true;		/* want redraw first time */
    Context.msgok = true;		/* ok to get messages */
    Context.display = false;		/* ok to get messages */
    stopUpdate();			/* stop the display interrupt */
    utGetSecs( &laststat );		/* initialize stat timer */

    /* send a clientstat packet and a ship packet. */

    if (!sendClientStat(sInfo.sock, SPCLNTSTAT_FLAG_NONE, Context.snum,
                        cbShips[Context.snum].team,
                        Context.unum, 0))
        return false;

    if (!sendShip(sInfo.sock, Context.snum))
        return false;

    sInfo.state = SVR_STATE_PLAY;

    startUpdate();			/* setup for next interval */

    /* Tell everybody, we're here */

    sprintf(msgbuf, "%c%d (%s) has entered the game.",
            cbTeams[cbShips[Context.snum].team].teamchar,
            Context.snum,
            cbShips[Context.snum].alias);

    clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, msgbuf);

#if defined(AUTO_RECORD)
    {                             /* automatically record all games */
        extern void startRecord(int);

        if (Context.recmode != RECMODE_ON)
            startRecord(false);
    }
#endif

    /* client updates will be handled by updateProc */
    /* While we're alive, field commands and process them. */
    while ( clbStillAlive( Context.snum ) )
    {
        /* Make sure we still control our ship. */
        if ( cbShips[Context.snum].pid != Context.pid )
            break;

        didsomething = 0;
        if ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                        buf, PKT_MAXSIZE, 0, NULL)) < 0)
	{
            if (errno != EINTR)
	    {
                utLog("conquestd:play:pktWaitForPacket: %s", strerror(errno));
                handleSignal(0);
                /* NOTREACHED */
                return false;
	    }
	}

        switch (pkttype)
	{
	case CP_ACKUDP:
            procAckUDP(buf);
            break;

	case CP_SETCOURSE:
            procSetCourse(buf);
            didsomething++;
            break;

	case CP_FIRETORPS:
            procFireTorps(buf);
            didsomething++;
            break;

	case CP_AUTHENTICATE:
            procChangePassword(buf);
            didsomething++;
            break;

	case CP_SETNAME:
            procSetName(buf);
            didsomething++;
            break;

	case CP_MESSAGE:
            procMessage(buf);
            didsomething++;
            break;

	case CP_COMMAND:
            handleSimpleCmdPkt((cpCommand_t *)buf);
            didsomething++;
            break;

	default:
            if (pkttype != 0 && pkttype != -1)
                utLog("conquestd: play: got unexpected packet type %d", pkttype);
            break;
	}

        if (didsomething)         /* update immediately if we did something */
        {
            clbBlockAlarm();
            updateClient(false);
            clbUnblockAlarm();
        }

        utGrand( &Context.msgrand );
        Context.msgok = true;

        /* See if it's time to update the statistics. */
        if ( utDeltaSecs( laststat, &now ) >= 15 )
	{
            conqstats( Context.snum );
            laststat = now;
	}

        clbBlockAlarm();          /* no signals can be accepted when
                                     drcheck is run */
        drcheck();
        clbUnblockAlarm();

        if (didsomething)
            continue;               /* see if there is another pkt */

        utSleep(0.05);
    }

    conqstats( Context.snum );
    upchuck();

    /* Asts are still enabled, simply cancel the next screen update. */

    stopUpdate();
    updateClient(false);	/* one last, to be sure. */
    sendcbConqInfo(sInfo.sock, true);
    utLog("PLAY: ship %d died, calling dead()", Context.snum);
    dead( Context.snum );

    return(true);

}



/*  SYNOPSIS */
/*    int flag, welcome */
/*    int unum */
/*    flag = welcome( unum ) */
int welcome( int *unum )
{
    int i, team;
    char name[MAXUSERNAME];
    char password[MAXUSERNAME];	/* encrypted pw, "" if local */
    uint8_t flags = SPCLNTSTAT_FLAG_NONE;

    if (!Authenticate(name, password))
        return false;

    if ( ! clbGetUserNum( unum, name, USERTYPE_NORMAL ) )
    {
        flags |= SPCLNTSTAT_FLAG_NEW;
        /* Must be a new player. */
        if ( cbConqInfo->closed )
	{
            pktSendAck(PSEV_FATAL, PERR_CLOSED,
                       NULL);
            return ( false );
	}
        team = rndint( 0, NUMPLAYERTEAMS - 1 );

        cbuf[0] = 0;
        utAppendTitle(cbuf , team) ;
        utAppendChar(cbuf , ' ') ;
        i = strlen( cbuf );
        strcat(cbuf , name) ;
        cbuf[i] = (char)toupper( cbuf[i] );

        if ( ! clbRegister( name, cbuf, team, unum ) )
	{
            pktSendAck(PSEV_FATAL, PERR_REGISTER,
                       NULL);
            return ( false );
	}

        utLog("conquestd: clbRegister complete: unum = %d, team = %d\n",
              *unum, team);

        /* copy in the password */
        strcpy(cbUsers[*unum].pw, password);
        /* set lastentry time for new players */
        cbUsers[*unum].lastentry = time(0);

    }


    /* Must be special to play when closed. */
    if ( cbConqInfo->closed && ! UPLAYWHENCLOSED(*unum) )
    {
        pktSendAck(PSEV_FATAL, PERR_CLOSED,
                   NULL);
        utLog("conquestd: welcome: game closed\n");
        return ( false );
    }

    /* Can't play without a ship. */
    if ( ! clbFindShip( &Context.snum ) )
    {
        pktSendAck(PSEV_FATAL, PERR_NOSHIP,
                   NULL);
        utLog("WELCOME: findship failed");
        return ( false );
    }

    /* send a clntstat packet if everything's ok */

    if (!sendClientStat(sInfo.sock, flags, Context.snum, cbUsers[*unum].team,
                        *unum, 0))
        return false;

    /* send a user packet for the user as well. */
    if (sendUser(sInfo.sock, *unum) <= 0)
        return false;

    return ( true );

}


static int hello(void)
{
    spHello_t shello = {};
    char buf[PKT_MAXSIZE];
    char cbuf[MESSAGE_SIZE * 2];
    int pkttype;
    extern char *ConquestVersion, *ConquestDate;
    int rv;
    struct timeval tv;
    fd_set readfds;
    struct sockaddr_in usa;	/* internet socket addr. structure - udp */
    cpAck_t *cpack;

    /* first send a server hello */
    shello.type = SP_HELLO;
    shello.protover = (uint16_t)htons(PROTOCOL_VERSION);

    // This isn't of real use to the client anymore, but might help in
    // debugging...
    shello.cmnrev = (uint32_t)htonl(COMMONSTAMP);

    // send the cbLimits data so the client knows how to build an
    // appropriate universe
    shello.maxplanets = htonl(cbLimits.maxPlanets());
    shello.maxships = htonl(cbLimits.maxShips());
    shello.maxusers = htonl(cbLimits.maxUsers());
    shello.maxhist = htonl(cbLimits.maxHist());
    shello.maxmsgs = htonl(cbLimits.maxMsgs());
    shello.maxtorps = htonl(cbLimits.maxTorps());

    utStrncpy((char *)shello.servername, SysConf.ServerName,
              CONF_SERVER_NAME_SZ);

    utStrncpy((char *)shello.serverver, ConquestVersion, CONF_SERVER_NAME_SZ);
    utStrncat((char *)shello.serverver, " ", CONF_SERVER_NAME_SZ);
    utStrncat((char *)shello.serverver, ConquestDate,
              CONF_SERVER_NAME_SZ);
    utStrncpy((char *)shello.motd, SysConf.ServerMotd, CONF_SERVER_MOTD_SZ);
    shello.flags = 0;

    if (cbConqInfo->closed)
        shello.flags |= SPHELLO_FLAGS_CLOSED;

    if (Context.accessDenied)
        shello.flags |= SPHELLO_FLAGS_ACCESS_DENIED;

    if (pktWrite(PKT_SENDTCP, &shello) <= 0)
    {
        utLog("NET: SERVER: hello: write shello failed\n");
        return false;
    }

    utLog("NET: SERVER: hello: sent server hello to client");

    // say good bye if access was denied...
    if (Context.accessDenied)
    {
        utLog("%s: Access was denied for this connection, returning false",
              __FUNCTION__);
        return false;
    }

    /* now we want a client hello in response */
    if ((pkttype = pktRead(buf, PKT_MAXSIZE, 60)) < 0)
    {
        utLog("NET: SERVER: hello: read client hello failed, pkttype = %d",
              pkttype);
        return false;
    }

    if (pkttype == 0)
    {
        utLog("NET: SERVER: hello: read client hello: timeout.\n");
        return false;
    }

    if (pkttype != CP_HELLO)
    {
        utLog("NET: SERVER: hello: read client hello: wrong packet type %d\n", pkttype);
        return false;
    }

    chello = *(cpHello_t *)buf;

    /* fix up byte ordering */
    chello.protover = ntohs(chello.protover);
    chello.cmnrev = ntohl(chello.cmnrev);

    chello.clientname[CONF_SERVER_NAME_SZ - 1] = 0;
    chello.clientver[CONF_SERVER_NAME_SZ - 1] = 0;

    utLog("CLIENTID:%s:%s:%d:0x%04hx:%d",
          chello.clientname,
          chello.clientver,
          chello.updates,
          chello.protover,
          chello.cmnrev);

    /* do some checks - send a NAK and fail if things aren't cool */
    if (chello.protover != PROTOCOL_VERSION)
    {
        sprintf(cbuf, "SERVER: Protocol mismatch, server 0x%x, client 0x%x",
                PROTOCOL_VERSION, chello.protover);
        pktSendAck(PSEV_FATAL, PERR_BADPROTO, cbuf);
        utLog("NET: %s", cbuf);
        return false;
    }

    /* for now just a mild notification, since this isn't a real problem
     *  anymore.
     */
    if (chello.cmnrev != COMMONSTAMP)
    {
        utLog("NET: INFO: common block mismatch: client %d server %d",
              chello.cmnrev, COMMONSTAMP);
    }

    if (chello.updates >= 1 && chello.updates <= 10)
        Context.updsec = chello.updates;

    /* now send the server stats normally */
    if (!sendServerStat(PKT_SENDTCP))
    {
        utLog("NET: SERVER: hello: sendServerStat failed");
        return false;
    }

    /* now we want an ack.  If we get it, we're done! */
    if ((pkttype = pktRead(buf, PKT_MAXSIZE, 60)) < 0)
    {
        utLog("NET: SERVER: hello: read client Ack failed");
        return false;
    }

    if (pkttype != CP_ACK)
    {
        utLog("NET: SERVER: hello: got packet type %d, expected CP_ACK",
              pkttype);
        return false;
    }

    return true;
}

void catchSignals(void)
{
    signal(SIGHUP, (void (*)(int))handleSignal);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTERM, (void (*)(int))handleSignal);
    signal(SIGINT, (void (*)(int))handleSignal);
    signal(SIGPIPE, (void (*)(int))handleSignal);
    signal(SIGQUIT, (void (*)(int))handleSignal);

    return;
}

void handleSignal(int sig)
{
    stopUpdate();

    if (sig)
        utLog("conquestd: exiting on signal %d", sig);

    if (sInfo.state == SVR_STATE_PLAY)
    {
        if (SysConf.AllowVacant)
        {			/* this allows vacant ships */
            drpexit();
            conqstats(Context.snum);          /* update stats */
            /* now we clear ship's elapsed/cpu seconds so
               that there won't be a huge addition to the
               Teams/Users/Ships timing stats when a VACANT
               ships re-enters Conquest */
            cbShips[Context.snum].ctime = 0;
            cbShips[Context.snum].etime = 0;

            SFSET(Context.snum, SHIP_F_VACANT); /* help the driver */
        }
        else
        {
            /* so we can detect cowards */
            clbKillShip( Context.snum, KB_LIGHTNING, 0 );
            /* turn ship off */
            cbShips[Context.snum].status = SS_OFF;
        }
    }
    else
    {                       /* not playing (main menu, etc) */
        /* if we aren't playing, then just turn it off */
        if (Context.snum >= 0 && Context.snum < cbLimits.maxShips())
            cbShips[Context.snum].status = SS_OFF;
    }

    conqend();
    exit(0);

}

/*  conqend - machine dependent clean-up */
/*  SYNOPSIS */
/*    conqend */
void conqend(void)
{

    char msgbuf[128];

    if (Context.entship == true)
    {				/* let everyone know we're leaving */
        sprintf(msgbuf, "%s has left the game.",
                cbUsers[Context.unum].alias);
        clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, msgbuf);
    }

    recCloseOutput();

    return;

}
