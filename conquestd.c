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

static uint16_t listenPort = CN_DFLT_PORT;

static char cbuf[BUFFER_SIZE_1024]; /* general purpose buffer */
static char *progName;

static int localOnly = FALSE;   /* whether to only listen on loopback */
static char *metaServer = META_DFLT_SERVER; /* meta server hostname */
static int updateMeta = FALSE;  /* whether to notify meta server */
static char *myServerName = NULL; /* to meta */

cpHello_t chello;		/* client hello info we want to keep */
ServerInfo_t sInfo;

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
        return FALSE;
    }
    else
    {
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

    return TRUE;
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

    if (!checkPID(ConqInfo->conqservPID))
    {				/* see if one is really running */
        /* if we are here, we will be the listener */
        cbLock(&ConqInfo->lockword);
        ConqInfo->conqservPID = getpid();
        cbUnlock(&ConqInfo->lockword);
        sInfo.isMaster = TRUE;
        utLog("NET: master server listening on port %d\n", listenPort);
    }

    sa.sin_port = htons(listenPort);

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
    while (TRUE)
    {
        if (updateMeta)
        {
            /* get any changes to sysconf so that meta updates are
               up to date */
            GetSysConf(TRUE);
            metaUpdateServer(metaServer, myServerName, listenPort);
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
                    pktSendAck(PSEV_FATAL, PERR_UNSPEC,
                               "Access Denied: You are not allowed to connect to this server.");
                    close(t);
                    exit(1);
                }

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
    int dodaemon = FALSE;

    progName = argv[0];
    sInfo.state = SVR_STATE_PREINIT;
    sInfo.sock = -1;
    sInfo.usock = -1;
    sInfo.doUDP = FALSE;
    sInfo.tryUDP = TRUE;
    sInfo.clientDead = TRUE;
    sInfo.isMaster = FALSE;
    sInfo.isLoggedIn = FALSE;


    while ((i = getopt(argc, argv, "dlp:u:mM:N:v")) != EOF)    /* get command args */
        switch (i)
        {
        case 'd':
            dodaemon = TRUE;
            break;

        case 'p':
            listenPort = (uint16_t)atoi(optarg);
            break;

        case 'l':                 /* local conn only */
            localOnly = TRUE;
            break;

        case 'u':
            myuidname = optarg;
            break;

        case 'm':
            updateMeta = TRUE;
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


    if (!pktInit())
    {
        fprintf(stderr, "pktInit failed, exiting\n");
        utLog("pktInit failed, exiting");
    }
    pktSetSocketFds(sInfo.sock, sInfo.usock);

    /* need to make sure that the serverPackets array is setup */
    pktSetClientProtocolVersion(PROTOCOL_VERSION);

    /* clear out our stored packets */
    spktInit();

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

    if (GetSysConf(FALSE) == -1)
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

    cbMap();

    if ( *CBlockRevision != COMMONSTAMP )
    {
        fprintf(stderr,"conquestd: Common block ident mismatch.\n" );
        fprintf(stderr,"           You must initialize the universe with conqoper.\n" );
        exit(1);
    }


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

    /* re-read sys conf here for indivulual client drivers... */
    if (GetSysConf(FALSE) == -1)
    {
#ifdef DEBUG_CONFIG
        utLog("%s@%d: main(): GetSysConf() returned -1.", __FILE__, __LINE__);
#endif
    }

    conqinit();			/* machine dependent initialization */

    /* load the globals/planets/textures, for each client */
    cqiLoadRC(CQI_FILE_CONQINITRC, NULL, 1, 0);
    cqiLoadRC(CQI_FILE_TEXTURESRC, NULL, 1, 0);

    rndini();		/* initialize random numbers */

    utLog("CONNECT: client %s", sInfo.remotehost);

    /* now we need to negotiate. */
    if (!hello())
    {
        utLog("conquestd: hello() failed");
        exit(1);
    }

    utLog("CONNECT: client %s SUCCESS.", sInfo.remotehost);

    Context.recmode = RECMODE_OFF; /* always */

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
    updateClient(FALSE);

    recUpdateFrame();

    /* check for and send any new messages */
    if ( utGetMsg( Context.snum, &Ships[Context.snum].lastmsg ) )
        sendMessage(&(Msgs[Ships[Context.snum].lastmsg]));

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
        for ( j=0; j<MAXPLANETS; j++ )
	{
            if (PHOMEPLANET(j) && Planets[j].defendteam == i)
            {
                // if we don't own it, bail
                if (Planets[j].team != Ships[snum].team)
                    break;
            }
	}

        if (j < MAXPLANETS)
            owned[i] = false;
        else
            owned[i] = true;
    }

    owned[Ships[snum].team] = TRUE;  // we can always enter our own
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
        *system = Ships[snum].team; /* tell the client */
        if (!sendClientStat(sInfo.sock, SPCLNTSTAT_FLAG_NONE, snum,
                            Ships[snum].team,
                            Context.unum, 0))
            return FALSE;
        return TRUE;
    }

    /* ask the client - use a clientstat with non-zero esystem */

    if (!sendClientStat(sInfo.sock, SPCLNTSTAT_FLAG_NONE, snum, Ships[snum].team,
                        Context.unum, esystem))
        return FALSE;

    while ( clbStillAlive( Context.snum ) )
    {

        /* now we wait for another ENTER command packet with it's detail member
           indicating the desired system. */

        if ((pkttype = pktWaitForPacket(CP_COMMAND,
                                        buf, PKT_MAXSIZE, 1, NULL)) < 0)
        {
            utLog("conquestd:capentry: waitforpacket returned %d", pkttype);
            return FALSE;
        }


        if ( pkttype == 0 )       /* timeout */
            continue;

        ccmd = (cpCommand_t *)buf;
        ccmd->detail = ntohs(ccmd->detail);

        utLog("conquestd: capentry: got CP_COMMAND, detail =0x%x",
              ccmd->detail);

        if (ccmd->cmd != CPCMD_ENTER)
	{			/* we'll just use the home team */
            *system = Ships[snum].team;
            return TRUE;
	}

        if (ccmd->detail == 0)	/* didn't want to select one */
            return FALSE;

        /* else we'll use the first set bit */
        esystem &= (uint8_t)(ccmd->detail & 0x00ff);

        for ( i = 0; i < NUMPLAYERTEAMS; i++ )
            if (esystem & (1 << i))
            {
                *system = i;
                return TRUE;
            }

        /* shouldn't happen, but... */
        *system = Ships[snum].team;
        return TRUE;
    }

    return FALSE;	    /* can get here because of clbStillAlive() */

}


/*  dead - do dead-type stuff for the server. (DOES LOCKING) */
/*  SYNOPSIS */
/*    int snum */
/*    int leave */
/*    dead( snum, leave ) */
void dead( int snum, int leave )
{
    int i, j, now, entertime;
    killedBy_t kb;
    unsigned int detail;
    uint8_t flags = SPCLNTSTAT_FLAG_NONE; /* for clientstat msg */
    char buf[PKT_MAXSIZE];	/* gen purpose */

    /* If something is wrong, don't do anything. */
    if ( snum < 0 || snum >= MAXSHIPS )
        return;

    /* If our ships pid is wrong, we are indeed lost. */
    if ( Ships[snum].pid != Context.pid )
        return;

    kb = Ships[snum].killedBy;
    detail = Ships[snum].killedByDetail;

    /* Delay while our torps are exploding. */
    utGrand( &entertime );
    i = 0;
    while ( utDeltaGrand( entertime, &now ) < TORPEDOWAIT_GRAND )
    {
        updateClient(FALSE);
        i = 0;
        for ( j = 0; j < MAXTORPS; j++ )
            if ( Ships[snum].torps[j].status == TS_DETONATE )
                i = i + 1;
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

    updateClient(FALSE);
    for ( i=0; i<10 && Ships[snum].status == SS_DYING; i++ )
    {
        utSleep( (1.0 / (real)Context.updsec) );
        updateClient(FALSE);
    }

    /* if you conquered the universe, let the client know, and wait for
       a cpMessage packet indicating the desired last words.  We'll wait up
       to 2 minutes before we decide on our own. */

    flags |= SPCLNTSTAT_FLAG_KILLED; /* you are quite dead */

    if (kb == KB_CONQUER)
        flags |= SPCLNTSTAT_FLAG_CONQUER;

    /* send the clientstat */
    if (!sendClientStat(sInfo.sock, flags, Context.snum, Users[Context.unum].team,
                        Context.unum, 0))
    {				/* an error, let the ai code choose some
				   last words and bail */
        if (kb == KB_CONQUER)
        {
            robreply(buf);
            utStrncpy(ConqInfo->lastwords, buf, MAXLASTWORDS);
        }

        utLog("conquestd: dead(): sendClientStat failed, fl = 0x%0x\n",
              flags);

        return;
    }

    utLog("INFO: dead(): sent sendClientStat, fl = 0x%0x",
          flags);

    /* fix things up */
    Ships[snum].status = SS_RESERVED;
    Ships[snum].sdfuse = -TIMEOUT_PLAYER;
    /*  Ships[snum].killedby = 0;*/

    /* let the client know. */
    updateClient(FALSE);

    /* if conquered, wait for the cpMessage_t */
    if (kb == KB_CONQUER)
    {
        if (pktWaitForPacket(CP_MESSAGE,
                             buf, PKT_MAXSIZE,
                             (60 * 5), NULL) <= 0)
	{			/* error or timeout.  gen lastwords */
            robreply(buf);
            utStrncpy(ConqInfo->lastwords, buf, MAXLASTWORDS);

            return;
	}
        else
	{
            cpMessage_t *cmsg = (cpMessage_t *)buf;

            /* copy as much of the message as you can. */
            utStrncpy(ConqInfo->lastwords, (char *)cmsg->msg, MAXLASTWORDS);
	}
    }

    /* Turn off sticky war so we can change war settings from menu(). */
    for ( i = 0; i < NUMPLAYERTEAMS; i++ )
        Ships[snum].rwar[i] = FALSE;

    return;

}

/* send all pertinent data, and any users attached to them. */
int updateClient(int force)
{
    int i,j;
    static int sentallusers = FALSE; /* we will send all user data once. */
    static time_t oldtime = 0;
    time_t newtime = time(0);
    int seciter = FALSE;
    static time_t histtime = 0;   /* timers that try to save some time() */
    static time_t infotime = 0;
    static time_t teamtime = 0;
    int dohist = FALSE;
    int doinfo = FALSE;
    int doteam = FALSE;

    if (force)
    {                           /* we need to reload everything */
        oldtime = 0;
        histtime = 0;
        infotime = 0;
        teamtime = 0;
        sentallusers = FALSE;
    }


    /* some things really should not be checked every update iter */
    if (oldtime != newtime)
    {
        seciter = TRUE;
        oldtime = newtime;
    }

    /* hist */
    if ((abs((unsigned int)newtime - (unsigned int)histtime) >
         HISTORY_UPDATE_INTERVAL))
    {
        dohist = TRUE;
        histtime = newtime;
    }

    /* info */
    if ((abs((unsigned int)newtime - (unsigned int)infotime) >
         CONQINFO_UPDATE_INTERVAL))
    {
        doinfo = TRUE;
        infotime = newtime;
    }

    /* team */
    if ((abs((unsigned int)newtime - (unsigned int)teamtime) >
         TEAM_UPDATE_INTERVAL))
    {
        doteam = TRUE;
        teamtime = newtime;
    }


    if (!sentallusers)
    {                           /* send all valid user data the first time */
        sentallusers = TRUE;
        for (i=0; i<MAXUSERS; i++)
            if (ULIVE(i))
                if (!sendUser(sInfo.sock, i))
                    return FALSE;
    }

    for (i=0; i<MAXSHIPS; i++)
    {
        if (!sendShip(sInfo.sock, i))
            return FALSE;

        for (j=0; j<MAXTORPS; j++)
            if (!sendTorp(sInfo.sock, i, j))
                return FALSE;

        /* we only send user data for active ships. */
        if (Ships[i].status != SS_OFF)
	{
            if (seciter)
            {
                if (!sendUser(sInfo.sock, Ships[i].unum))
                {
                    return FALSE;
                }
            }
        }
    }

    for (i=0; i<MAXPLANETS; i++)
        sendPlanet(sInfo.sock, i, force);

    if (doteam)
        for (i=0; i<NUMALLTEAMS; i++)
            sendTeam(sInfo.sock, i, FALSE);

    if (doinfo)
        sendConqInfo(sInfo.sock, FALSE);

    if (dohist)
    {
        for (i=0; i<MAXHISTLOG; i++)
        {
            sendHistory(sInfo.sock, i);
        }
    }

    sendDoomsday(sInfo.sock);

    return TRUE;
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

                Ships[Context.snum].team = team;
                Ships[Context.snum].shiptype =
                    Teams[Ships[Context.snum].team].shiptype;
                Users[Context.unum].team = Ships[Context.snum].team;
                Ships[Context.snum].war[Ships[Context.snum].team] = FALSE;
                Users[Context.unum].war[Users[Context.unum].team] = FALSE;
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
        updateClient(TRUE);
        clbUnblockAlarm();

        break;

    case CPCMD_DISCONNECT:
        utLog("CPCMD_DISCONNECT");
        /* 'fake' signal.  cleans up and exits. */
        handleSignal(0);
        /* NOTREACHED */
        break;

    case CPCMD_PING:
        clbBlockAlarm();
        pktSendAck(PSEV_INFO, PERR_PINGRESP, NULL);
        clbUnblockAlarm();
        break;

    case CPCMD_KEEPALIVE:       /* these we just ignore */
        break;

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
    cbLock(&ConqInfo->lockword);
    Ships[Context.snum].sdfuse = 0;
    Ships[Context.snum].status = SS_OFF;
    cbUnlock(&ConqInfo->lockword);
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
    initstats( &Ships[Context.snum].ctime, &Ships[Context.snum].etime );

    /* Log this entry into the Game. */
    Context.histslot = clbLogHist( Context.unum );

    /* Set up a few ship characteristics here rather than in clbInitShip(). */
    Ships[Context.snum].unum = Context.unum;
    Ships[Context.snum].team = Users[Context.unum].team;
    Ships[Context.snum].shiptype = Teams[Ships[Context.snum].team].shiptype;

    Ships[Context.snum].pid = Context.pid;
    Ships[Context.snum].killedBy = KB_NONE;
    Ships[Context.snum].killedByDetail = 0;

    for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
        Ships[Context.snum].rwar[i] = FALSE;
        Ships[Context.snum].war[i] = Users[Context.unum].war[i];
    }
    utStrncpy( Ships[Context.snum].alias, Users[Context.unum].alias,
               MAXUSERNAME );

    /* Set up some things for the menu display. */
    Context.leave = FALSE;
    sleepy = clbGetMillis();
    playrv = FALSE;

    /* send a ship packet for our ship.  Since this is the first ship packet,
       an SP_SHIP should get sent. */
    if (!sendShip(sInfo.sock, Context.snum))
    {				/* something went wrong */
        /* Make our ship available for others to use. */
        if ( Ships[Context.snum].status == SS_RESERVED )
	{
            freeship();
            return;
	}
    }

    do
    {
        if (!updateClient(FALSE))	/* sends packets */
	{
            freeship();
            return;
	}

        /* Make sure things are proper. */
        if (playrv == ERR)
	{
            if ( Context.snum < 0 || Context.snum >= MAXSHIPS )
                lose = TRUE;
            else if ( Ships[Context.snum].pid != Context.pid )
                lose = TRUE;
            else if ( Ships[Context.snum].status != SS_RESERVED )
	    {
                utLog( "menu(): Ship %d no longer reserved.", Context.snum );
                lose = TRUE;
	    }
            else
                lose = FALSE;
	}
        else
            lose = FALSE;

        if ( lose )				/* again, Jorge? */
	{
            pktSendAck(PSEV_FATAL, PERR_LOSE, NULL);
            return;
	}

        /* Reset up the destruct fuse. */
        Ships[Context.snum].sdfuse = -TIMEOUT_PLAYER;

        if ((pkttype = pktWaitForPacket(PKT_ANYPKT,
                                        buf, PKT_MAXSIZE, 0, NULL)) < 0)
	{
            freeship();
            utLog("conquestd:menu: waitforpacket returned %d", pkttype);
            handleSignal(0);
            /* not reached */

            return;
	}

        /* we get sleepy if we are recieving no packets, or only
           keepalive packets */
        if ( pkttype == 0 || ((pkttype == CP_COMMAND) &&
                              CPCMD_KEEPALIVE == (((cpCommand_t *)buf)->cmd)))
	{
            if ((clbGetMillis() - sleepy) > sleeplimit)
                break;
            else
                utSleep(0.05);

            continue;
	}

        switch(pkttype)
	{
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
                for ( i = 0; i < MAXSHIPS; i++ )
                    if ( !((Ships[i].status == SS_LIVE ||
                            Ships[i].status == SS_ENTERING) &&
                           Ships[i].unum == Context.unum))
                    {
                        clbResign( Context.unum, FALSE );
                        Ships[Context.snum].status = SS_OFF;
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
    while ( clbStillAlive( Context.snum ) &&  !Context.leave );

    /* Make our ship available for others to use. */
    if ( Ships[Context.snum].status == SS_RESERVED )
        freeship();

    /* Try to kill the driver if we started one the last time */
    /*  we played and we've been in the menu long enough. */
    drkill();

    return;

}


/*  newship - create a new ship for a user (DOES LOCKING) */

/*  here we will fnd a ship for the client.  If the client has a VACANT
    ship it will return that ship, else it will get a new one.  No more
    multiples (for clients) supported currently I'm afraid..., if the
    client would be able to enter from multiple systems, this will be
    indicated by a non-zero esystems member of the clnt stat pkt */

/*  SYNOPSIS */
/*    int status, newship, unum, snum */
/*    int flag, newship */
/*    flag = newship( unum, snum ) */
int newship( int unum, int *snum )
{
    int i, j, system;
    int fresh;
    int vec[MAXSHIPS];
    int numavail = 0;
    int numvec = 0;

    /* cleanup any unliving ships - this is the first thing we need to do */
    clbCheckShips(FALSE);

    cbLock(&ConqInfo->lockword);

    Ships[*snum].status = SS_ENTERING;		/* show intent to fly */

    fresh = TRUE;				/* assume we want a fresh ship*/

    /* Count number of his ships flying. */
    j = 0;
    numvec = 0;
    for ( i = 0; i < MAXSHIPS; i++ )
        if ( Ships[i].status == SS_LIVE || Ships[i].status == SS_ENTERING )
            if ( Ships[i].unum == unum && *snum != i )
            {
                j++;
                vec[numvec++] = i;
            }

    cbUnlock(&ConqInfo->lockword);

    /* see if we need to reincarnate to a vacant ship */
    if ( j > 0 )
    {
        /* Need to reincarnate. */

        if (!SVACANT(vec[0]))
        {		   /* if it's available, we'll take it */
			   /* ...if it's not already being flown... */
            pktSendAck(PSEV_ERROR, PERR_FLYING, NULL);
            Ships[*snum].status = SS_RESERVED;
            return ( FALSE );
        }


        /* Look for a live ship for us to take. */
        cbLock(&ConqInfo->lockword);
        for (i=0; i<MAXSHIPS; i++)
            if ( Ships[i].unum == unum && Ships[i].status == SS_LIVE )
            {
                fresh = FALSE;
                Ships[*snum].status = SS_OFF;
                *snum = i;
                Ships[*snum].pid = Context.pid;
                Ships[*snum].status = SS_ENTERING;
                SFCLR(*snum, SHIP_F_VACANT);
                break;
            }
        cbUnlock(&ConqInfo->lockword);
    }

    /* Figure out which system to enter. */
    if ( fresh )
    {
        /* (re)init the ship's team! (bug 1/10/98) */
        Ships[*snum].team = Users[Context.unum].team;
        Ships[*snum].shiptype = Teams[Ships[*snum].team].shiptype;
        system = Ships[*snum].team;

        if ( ! capentry( *snum, &system ) )
	{
            Ships[*snum].status = SS_RESERVED;
            return ( FALSE );
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
        Ships[*snum].ctime = 0;
        Ships[*snum].etime = 0;
    }

    cbLock(&ConqInfo->lockword);

    /* If necessary, initalize the ship */
    if ( fresh )
    {
        clbInitShip( *snum, unum );

        /* Randomly position the ship near the home sun (or planet). */
        if (!clbFindTeamHomeSun(system, &i))
        {
            // couldn't find a home sun, use homeplanet
            i = Teams[system].homeplanet;
        }
        // good to go
        clbPutShip( *snum, Planets[i].x, Planets[i].y );
        Ships[*snum].dhead = rnduni( 0.0, 359.9 );
        Ships[*snum].head = Ships[*snum].dhead;
        Ships[*snum].dwarp = (real) rndint( 2, 5 );
        // lock onto the homeplanet
        Ships[*snum].lock = LOCK_PLANET;
        Ships[*snum].lockDetail = (uint16_t)Teams[system].homeplanet;
    }
    else
    {				/* if we're reincarnating, skip any
				   messages that might have been sent
				   while we were gone */
        cbLock(&ConqInfo->lockmesg);
        Ships[*snum].lastmsg = ConqInfo->lastmsg;
        Ships[*snum].alastmsg = Ships[*snum].lastmsg;
        cbUnlock(&ConqInfo->lockmesg);
        /* init user's last entry time */
        Users[Ships[*snum].unum].lastentry = time(0);
    }

    SFCLR(*snum, SHIP_F_ROBOT);
    Ships[*snum].action = 0;


    /* Straighten out the ships deltas. */
    clbFixDeltas( *snum );

    /* Finally, turn the ship on. */
    Ships[*snum].status = SS_LIVE;

    cbUnlock(&ConqInfo->lockword);
    Context.entship = TRUE;

    return ( TRUE );

}


/*  play - play the game (PLAY) */
/*  SYNOPSIS */
/*    play */
int play(void)
{
    int laststat, now;
    int didsomething;             /* update immediately if we did anything */
    int rv;
    char msgbuf[128];
    int pkttype;
    char buf[PKT_MAXSIZE];

    /* Can't carry on without a vessel. */
    if ( (rv = newship( Context.unum, &Context.snum )) != TRUE)
    {
        utLog("conquestd:play: newship() returned %d",
              rv);
        return(rv);
    }

    drstart();			/* start a driver, if necessary */
    Ships[Context.snum].sdfuse = 0;	/* zero self destruct fuse */
    utGrand( &Context.msgrand );		/* initialize message timer */
    Context.leave = FALSE;		/* assume we won't want to bail */
    Context.redraw = TRUE;		/* want redraw first time */
    Context.msgok = TRUE;		/* ok to get messages */
    Context.display = FALSE;		/* ok to get messages */
    stopUpdate();			/* stop the display interrupt */
    utGetSecs( &laststat );		/* initialize stat timer */

    /* send a clientstat packet and a ship packet. */

    if (!sendClientStat(sInfo.sock, SPCLNTSTAT_FLAG_NONE, Context.snum,
                        Ships[Context.snum].team,
                        Context.unum, 0))
        return FALSE;

    if (!sendShip(sInfo.sock, Context.snum))
        return FALSE;

    sInfo.state = SVR_STATE_PLAY;

    startUpdate();			/* setup for next interval */

    /* Tell everybody, we're here */

    sprintf(msgbuf, "%c%d (%s) has entered the game.",
            Teams[Ships[Context.snum].team].teamchar,
            Context.snum,
            Ships[Context.snum].alias);

    clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, msgbuf);

#if defined(AUTO_RECORD)
    {                             /* automatically record all games */
        extern void startRecord(int);

        if (Context.recmode != RECMODE_ON)
            startRecord(FALSE);
    }
#endif

    /* client updates will be handled by updateProc */
    /* While we're alive, field commands and process them. */
    while ( clbStillAlive( Context.snum ) )
    {
        /* Make sure we still control our ship. */
        if ( Ships[Context.snum].pid != Context.pid )
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
                return FALSE;
	    }
	}

        switch (pkttype)
	{
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
            updateClient(FALSE);
            clbUnblockAlarm();
        }

        utGrand( &Context.msgrand );
        Context.msgok = TRUE;

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
    updateClient(FALSE);	/* one last, to be sure. */
    sendConqInfo(sInfo.sock, TRUE);
    utLog("PLAY: ship %d died, calling dead()", Context.snum);
    dead( Context.snum, Context.leave );

    return(TRUE);

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
        return FALSE;

    sInfo.isLoggedIn = TRUE;

    if ( ! clbGetUserNum( unum, name, USERTYPE_NORMAL ) )
    {
        flags |= SPCLNTSTAT_FLAG_NEW;
        /* Must be a new player. */
        if ( ConqInfo->closed )
	{
            pktSendAck(PSEV_FATAL, PERR_CLOSED,
                       NULL);
            return ( FALSE );
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
            return ( FALSE );
	}

        utLog("conquestd: clbRegister complete: unum = %d, team = %d\n",
              *unum, team);

        /* copy in the password */
        strcpy(Users[*unum].pw, password);
        /* set lastentry time for new players */
        Users[*unum].lastentry = time(0);

    }


    /* Must be special to play when closed. */
    if ( ConqInfo->closed && ! UPLAYWHENCLOSED(*unum) )
    {
        pktSendAck(PSEV_FATAL, PERR_CLOSED,
                   NULL);
        utLog("conquestd: welcome: game closed\n");
        return ( FALSE );
    }

    /* Can't play without a ship. */
    if ( ! clbFindShip( &Context.snum ) )
    {
        pktSendAck(PSEV_FATAL, PERR_NOSHIP,
                   NULL);
        utLog("WELCOME: findship failed");
        return ( FALSE );
    }

    /* send a clntstat packet if everything's ok */

    if (!sendClientStat(sInfo.sock, flags, Context.snum, Users[*unum].team,
                        *unum, 0))
        return FALSE;

    /* send a user packet for the user as well. */
    if (sendUser(sInfo.sock, *unum) <= 0)
        return FALSE;

    return ( TRUE );

}


static int hello(void)
{
    spHello_t shello;
    char buf[PKT_MAXSIZE];
    char cbuf[MESSAGE_SIZE * 2];
    int pkttype;
    extern char *ConquestVersion, *ConquestDate;
    int rv;
    struct timeval tv;
    fd_set readfds;
    struct sockaddr_in usa;	/* internet socket addr. structure - udp */
    cpAck_t *cpack;

    /* open a UDP socket and bind to it */
    if ((sInfo.usock = udpOpen(listenPort, &usa)) < 0)
    {
        utLog("NET: SERVER hello: udpOpen() failed: %s", strerror(errno));
        sInfo.usock = -1;
        sInfo.tryUDP = FALSE;
        pktSetSocketFds(PKT_SOCKFD_NOCHANGE, sInfo.usock);
    }

    /* first loadup and send a server hello */
    shello.type = SP_HELLO;
    shello.protover = (uint16_t)htons(PROTOCOL_VERSION);

    shello.cmnrev = (uint32_t)htonl(COMMONSTAMP);
    utStrncpy((char *)shello.servername, SysConf.ServerName,
              CONF_SERVER_NAME_SZ);

    utStrncpy((char *)shello.serverver, ConquestVersion, CONF_SERVER_NAME_SZ);
    utStrncat((char *)shello.serverver, " ", CONF_SERVER_NAME_SZ);
    utStrncat((char *)shello.serverver, ConquestDate,
              CONF_SERVER_NAME_SZ);
    utStrncpy((char *)shello.motd, SysConf.ServerMotd, CONF_SERVER_MOTD_SZ);
    shello.flags = 0;

    if (ConqInfo->closed)
        shello.flags |= SPHELLO_FLAGS_CLOSED;

    if (pktWrite(PKT_SENDTCP, &shello) <= 0)
    {
        utLog("NET: SERVER: hello: write shello failed\n");
        return FALSE;
    }

    utLog("NET: SERVER: hello: sent server hello to client");

    if (sInfo.tryUDP)
    {
        /* wait a few seconds to see if client sends a udp */
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        FD_ZERO(&readfds);
        FD_SET(sInfo.usock, &readfds);
        if ((rv = select(sInfo.usock+1, &readfds, NULL, NULL, &tv)) <= 0)
        {
            if (rv == 0)
                utLog("NET: SERVER: hello: udp select timed out. No UDP");
            else
                utLog("NET: SERVER: hello: udp select failed: %s", strerror(errno));

            sInfo.tryUDP = FALSE;
        }
        else
        {
            if (FD_ISSET(sInfo.usock, &readfds))
            {                       /* get the packet, almost done negotiating udp */
                rv = udpRecv(sInfo.usock, buf, PKT_MAXSIZE, &sInfo.clntaddr);
                utLog("NET: SERVER: hello: got %d UDP bytes from client port %d", rv,
                      (int)ntohs(sInfo.clntaddr.sin_port));

                if (connect(sInfo.usock, (const struct sockaddr *)&sInfo.clntaddr,
                            sizeof(sInfo.clntaddr)) < 0)
                {
                    utLog("NET: SERVER: hello: udp connect() failed: %s", strerror(errno));
                    sInfo.tryUDP = FALSE;
                }
                else
                    utLog("NET: SERVER: hello: UDP connection to client established.");

            }
        }
    }

    /* now we want a client hello in response */
    if ((pkttype = pktRead(buf, PKT_MAXSIZE, 60)) < 0)
    {
        utLog("NET: SERVER: hello: read client hello failed, pkttype = %d",
              pkttype);
        return FALSE;
    }

    if (pkttype == 0)
    {
        utLog("NET: SERVER: hello: read client hello: timeout.\n");
        return FALSE;
    }

    if (pkttype != CP_HELLO)
    {
        utLog("NET: SERVER: hello: read client hello: wrong packet type %d\n", pkttype);
        return FALSE;
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
        return FALSE;
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

    /* send data to the client udp socket.  If the client gets it, it
       will acknowlege it in it's ACK packet, which will tell us we can
       do udp. woohoo! */
    if (sInfo.tryUDP)
    {
        send(sInfo.usock, "Open Me", 7, 0);
    }

    /* now send the server stats normally */
    if (!sendServerStat(PKT_SENDTCP))
    {
        utLog("NET: SERVER: hello: sendServerStat failed");
        return FALSE;
    }

    /* now we want an ack.  If we get it, we're done! */
    if ((pkttype = pktRead(buf, PKT_MAXSIZE, 60)) < 0)
    {
        utLog("NET: SERVER: hello: read client Ack failed");
        return FALSE;
    }

    if (pkttype != CP_ACK)
    {
        utLog("NET: SERVER: hello: got packet type %d, expected CP_ACK",
              pkttype);
        return FALSE;
    }

    if (sInfo.tryUDP)
    {
        /* see if the client could read our udp */
        cpack = (cpAck_t *)buf;
        if (cpack->code == PERR_DOUDP)
        {
            sInfo.doUDP = TRUE;
            utLog("NET: SERVER: hello: Client acknowleged UDP from server. Doing UDP.");
            pktSetSocketFds(PKT_SOCKFD_NOCHANGE, sInfo.usock);
        }
    }

    return TRUE;
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
            Ships[Context.snum].ctime = 0;
            Ships[Context.snum].etime = 0;

            SFSET(Context.snum, SHIP_F_VACANT); /* help the driver */
        }
        else
        {
            /* so we can detect cowards */
            clbKillShip( Context.snum, KB_LIGHTNING, 0 );
            /* turn ship off */
            Ships[Context.snum].status = SS_OFF;
        }
    }
    else
    {                       /* not playing (main menu, etc) */
        /* if we aren't playing, then just turn it off */
        if (Context.snum >= 0 && Context.snum < MAXSHIPS)
            Ships[Context.snum].status = SS_OFF;
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

    if (Context.entship == TRUE)
    {				/* let everyone know we're leaving */
        sprintf(msgbuf, "%s has left the game.",
                Users[Context.unum].alias);
        clbStoreMsg(MSG_FROM_COMP, 0, MSG_TO_ALL, 0, msgbuf);
    }

    recCloseOutput();

    return;

}
