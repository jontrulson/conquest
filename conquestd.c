#include "c_defs.h"

/************************************************************************
 *
 * conquestd - the Conquest server/client driver
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/


#define NOEXTERN

#include "conqdef.h"
#include "conqcom.h"
#include "conqlb.h"
#include "context.h"
#include "conf.h"
#include "global.h"
#include "color.h"
#include "record.h"
#include "ibuf.h"

#include "conqnet.h"
#include "packet.h"
#include "udp.h"

#define SERVER_NOEXTERN
#include "server.h"
#undef SERVER_NOEXTERN
#include "serverpkt.h"

#include "servauth.h"
#include "servercmd.h"
#include "meta.h"

#define LISTEN_BACKLOG 5 /* # of requests we're willing to to queue */

static Unsgn16 listenPort = CN_DFLT_PORT;

static char cbuf[MID_BUFFER_SIZE]; /* general purpose buffer */
static char *progName;

static int localOnly = FALSE;   /* whether to only listen on loopback */
static char *metaServer = META_DFLT_SERVER; /* meta server hostname */
static int updateMeta = FALSE;  /* whether to notify meta server */
static char *myServerName = NULL; /* to meta */

cpHello_t chello;		/* client hello info we want to keep */
ServerInfo_t sInfo;

static int hello(void);		/* meet and greet the client */
int updateClient(void);
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
      clog("getpeername failed: %s\n", strerror(errno));
      return FALSE;
    }
  else
    {
      if ((host = gethostbyaddr((char *) &addr.sin_addr.s_addr,
                                sizeof(unsigned long),
                                AF_INET)) == NULL)
	{
	  strncpy(buf, inet_ntoa((struct in_addr)addr.sin_addr), 
		  buflen);
	}
      else
        {
	  strncpy(buf, host->h_name, buflen);
        }
    }
  
  return TRUE;
}
    

/* we only return if we are a client driver, else we listen for requests,
   updating the meta server if requested */
void checkMaster(void)
{
  int s,t;			/* socket descriptor */
  int i,rv;			/* general purpose integer */
  struct sockaddr_in sa, isa;	/* internet socket addr. structure */
  struct hostent *hp;		/* result of host name lookup */
  struct timeval tv;
  fd_set readfds;

  signal(SIGCLD, SIG_IGN);	/* allow children to die */

  if (!checkPID(ConqInfo->conqservPID))
    {				/* see if one is really running */
      /* if we are here, we will be the listener */
      PVLOCK(&ConqInfo->lockword);
      ConqInfo->conqservPID = getpid();
      PVUNLOCK(&ConqInfo->lockword);
      sInfo.isMaster = TRUE;
      clog("NET: master server listening on port %d\n", listenPort);
    }

  /* get our own host information */
  memset(sInfo.localhost, 0, MAXHOSTNAME);

  gethostname ( sInfo.localhost, MAXHOSTNAME - 1 );

  if ((hp = gethostbyname(sInfo.localhost)) == NULL) 
    {
      perror("gethostbyname");
      exit (1);
    }
  
  sa.sin_port = htons(listenPort);

  if (localOnly)
    sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  else
    sa.sin_addr.s_addr = htonl(INADDR_ANY); /* Bind to all addresses.
                                               -Werewolf */
  sa.sin_family = hp->h_addrtype;
  
  /* allocate an open socket for incoming connections */
  if (( s = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0) 
    {
      perror ( "socket" );
      exit(1);
    }
  
  /* bind the socket to the service port so we hear incoming
   * connections 
   */
  if ( bind( s, (struct sockaddr *)&sa, sizeof ( sa )) < 0 ) 
    {
      perror( "bind" );
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
          clog("checkMaster: select failed: %s", strerror(errno));
          exit(1);
        }

      if (FD_ISSET(s, &readfds))
        {        

          i = sizeof (isa);
      
          /* hang in accept() while waiting for new connections */
          if ((t = accept(s, (struct sockaddr *)&isa, &i )) < 0) 
            {
              perror ( "accept" );
              exit (1);
            }
      
          if ( fork() == 0 ) 
            {			/* child - client driver */
              sInfo.sock = t;
              pktSetNodelay(sInfo.sock);
              memset(sInfo.remotehost, 0, MAXHOSTNAME);
              getHostname(sInfo.sock, sInfo.remotehost, MAXHOSTNAME - 1);
              
              clog("NET: forked client driver, pid = %d", getpid());
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

  while ((i = getopt(argc, argv, "dlp:u:mM:N:")) != EOF)    /* get command args */
    switch (i)
      {
      case 'd':
        dodaemon = TRUE;
        break; 

      case 'p':
	listenPort = (Unsgn16)atoi(optarg);
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

      default:
	printUsage();
	exit(1);
      }


  /* clear out our stored packets */
  spktInit();

  if ((ConquestUID = getUID(ROOT_USER)) == ERR)
    {
      fprintf(stderr, "%s: getUID() failed\n", progName);
      exit(1);
    }
  
  if ((ConquestGID = getConquestGID()) == ERR)
    {
      fprintf(stderr, "%s: getConquestGID() failed\n", progName);
      exit(1);
    }
  

  /* at this point, we see if the -u option was used.  If it was, we
     setuid() to it */

  if (myuidname)
    {
      int myuid;

      if ((myuid = getUID(myuidname)) == ERR)
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
        clog("INFO: running as user '%s', uid %d.", myuidname, myuid); 
    }

  
#ifdef DEBUG_CONFIG
  clog("%s@%d: main() Reading Configuration files.", __FILE__, __LINE__);
#endif
  
  if (GetSysConf(FALSE) == ERR)
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
    }

  if (setgid(ConquestGID) == -1)
    {
      clog("conquest: setgid(%d): %s",
           ConquestGID,
           strerror(errno));
      fprintf(stderr, "conquest: setgid(): failed\n");
      exit(1);
    }
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() *STARTING*", __FILE__, __LINE__);
#endif
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() getting semephores - GetSem()", __FILE__, __LINE__);
#endif
  
  if (GetSem() == ERR)
    {
      fprintf(stderr, "GetSem() failed to get semaphores. exiting.\n");
      exit(1);
    }
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() mapping common block.", __FILE__, __LINE__);
#endif
  
  map_common();

  if ( *CBlockRevision != COMMONSTAMP )
    {
      fprintf(stderr,"conquestd: Common block ident mismatch.\n" );
      fprintf(stderr,"           You must initialize the universe with conqoper.\n" );
      exit(1);
    }

  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() starting conqinit().", __FILE__, __LINE__);
#endif
  
  Context.maxlin = 0;		/* not used here */
  Context.maxcol = 0;		/* not used here */
  
  Context.snum = 0;
  Context.histslot = ERR;
  Context.lasttang = Context.lasttdist = 0;
  Context.lasttarg[0] = EOS;
  Context.updsec = 2;		/* two per second default update rate */


  /* if daemon mode requested, fork off and detach */
  if (dodaemon)
    {
      int cpid;
      clog("INFO: becoming daemon");
      chdir("/");

      cpid = fork();
      switch (cpid) 
        {
        case 0:
          /* child */

#if defined(HAVE_DAEMON)
          daemon(0, 0);
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
  if (GetSysConf(FALSE) == ERR)
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
    }



  conqinit();			/* machine dependent initialization */
  
  rndini( 0, 0 );		/* initialize random numbers */
  
  clog("CONNECT: client %s\n", sInfo.remotehost);

  /* now we need to negotiate. */
  if (!hello())
    {
      clog("conquestd: hello() failed\n");
      exit(1);
    }

  clog("CONNECT: client %s SUCCESS.\n", sInfo.remotehost);

  Context.recmode = RECMODE_OFF; /* always */

  if ( welcome( &Context.unum ) )
    {
      sInfo.state = SVR_STATE_MAINMENU;

      menu();
    }
  
  drpexit();			/* make the driver go away */
  conqend();			/* machine dependent clean-up */
  
#ifdef DEBUG_FLOW
  clog("%s@%d: main() *EXITING*", __FILE__, __LINE__);
#endif
  
  exit(0);
  
}

/* responsible for updating the client */
void updateProc(void)
{
#if 0
  static Unsgn32 lastms = 0;
  Unsgn32 millis = clbGetMillis();

  if ((millis - lastms) > 100)
    clog("millisdiff = %u", (millis - lastms));
  lastms = millis;
#endif

  /* Don't do anything if we're not supposed to. */
  if (sInfo.state != SVR_STATE_PLAY)
    return;

  stopUpdate();

  drcheck();			/* check the driver */  

  /* update client view of the universe */
  updateClient();

  recordUpdateFrame();

  /* check for and send any new messages */
  if ( getamsg( Context.snum, &Ships[Context.snum].lastmsg ) )
    sendMessage(sInfo.sock, &(Msgs[Ships[Context.snum].lastmsg]));

  /* Schedule for next time. */
  startUpdate();
  
  return;
  
}

void startUpdate(void)
{
  static struct sigaction Sig;

#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif

  Sig.sa_handler = (void (*)(int))updateProc;
  
  Sig.sa_flags = 0;

  if (sigaction(SIGALRM, &Sig, NULL) == -1)
    {
      clog("startUpdater():sigaction(): %s\n", strerror(errno));
      exit(errno);
    }
  
#ifdef HAS_SETITIMER
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
#ifdef HAS_SETITIMER
  struct itimerval itimer;
#endif
  
  signal(SIGALRM, SIG_IGN);
  
#ifdef HAS_SETITIMER
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
  int owned[NUMPLAYERTEAMS]; 
  int pkttype;
  cpCommand_t *ccmd;
  Unsgn8 buf[PKT_MAXSIZE];
  Unsgn8 esystem = 0;
  int sockl[2] = {sInfo.sock, sInfo.usock};
  
  /* First figure out which systems we can enter from. */
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      owned[i] = FALSE;
      /* We must own all three planets in a system. */
      for ( j = 0; j < 3; j = j + 1 )
	{
	  if ( Planets[Teams[i].teamhplanets[j]].team != Ships[snum].team )
	    goto cnext2_1; /* next 2; */
	}
      owned[i] = TRUE;
    cnext2_1:
      ;
    }
  owned[Ships[snum].team] = TRUE;		/* always can enter in our system */
  
  /* Now count how many systems we can enter from. */
  j = 0;
  esystem = 0;
  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
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

      if ((pkttype = waitForPacket(PKT_FROMCLIENT, sockl, CP_COMMAND,
				   buf, PKT_MAXSIZE, 1, NULL)) < 0)
        {
	  clog("conquestd:capentry: waitforpacket returned %d", pkttype); 
          return FALSE;
        }


      if ( pkttype == 0 )       /* timeout */
	continue; 

      ccmd = (cpCommand_t *)buf;
      ccmd->detail = ntohs(ccmd->detail);

      clog("conquestd: capentry: got CP_COMMAND, detail =0x%x", 
	   ccmd->detail);

      if (ccmd->cmd != CPCMD_ENTER)
	{			/* we'll just use the home team */
	  *system = Ships[snum].team;
	  return TRUE;
	}

      if (ccmd->detail == 0)	/* didn't want to select one */
	return FALSE;

      /* else we'll use the first set bit */
      esystem &= (Unsgn8)(ccmd->detail & 0x00ff);

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
  int i, j, kb, now, entertime; 
  Unsgn8 flags = SPCLNTSTAT_FLAG_NONE; /* for clientstat msg */
  char buf[PKT_MAXSIZE];	/* gen purpose */
  int sockl[2] = {sInfo.sock, sInfo.usock};
  
  /* If something is wrong, don't do anything. */
  if ( snum < 1 || snum > MAXSHIPS )
    return;
  
  /* If our ships pid is wrong, we are indeed lost. */
  if ( Ships[snum].pid != Context.pid )
    return;
  
  kb = Ships[snum].killedby;
  
  /* Delay while our torps are exploding. */
  grand( &entertime );
  i = 0;
  while ( dgrand( entertime, &now ) < TORPEDOWAIT_GRAND )
    {
      updateClient();
      i = 0;
      for ( j = 0; j < MAXTORPS; j = j + 1 )
	if ( Ships[snum].torps[j].status == TS_DETONATE )
	  i = i + 1;
      if ( i <= 0 )
	break;
      c_sleep( (1.0 / (real)Context.updsec) );
    }
  
  /* There aren't supposed to be any torps left. */
  if ( i > 0 )
    {
      buf[0] = EOS;
      appship( snum, buf );
      clog("INFO: dead: %s, detonating torp count is %d.",
	   buf, i);
    }
  
  buf[0] = EOS;
  appship( snum, buf );
  clog("INFO: dead: %s was killed by %d.", buf, kb);
  
  updateClient();
  for ( i=0; i<10 && Ships[snum].status == SS_DYING; i++ )
    {
      c_sleep( (1.0 / (real)Context.updsec) );
      updateClient();
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
          strncpy(ConqInfo->lastwords, buf, MAXLASTWORDS - 1);
          ConqInfo->lastwords[MAXLASTWORDS - 1] = 0;
        }

      clog("conquestd: dead(): sendClientStat failed, fl = 0x%0x\n",
           flags);

      return;
    }

  clog("INFO: dead(): sent sendClientStat, fl = 0x%0x",
       flags);

  /* fix things up */
  Ships[snum].status = SS_RESERVED;
  Ships[snum].sdfuse = -TIMEOUT_PLAYER;
  /*  Ships[snum].killedby = 0;*/

  /* let the client know. */
  updateClient();

  /* if conquered, wait for the cpMessage_t */
  if (kb == KB_CONQUER)
    {
      if (waitForPacket(PKT_FROMCLIENT, sockl, CP_MESSAGE, 
			buf, PKT_MAXSIZE,
			(60 * 2), NULL) <= 0)
	{			/* error or timeout.  gen lastwords */
	  robreply(buf);
	  strncpy(ConqInfo->lastwords, buf, MAXLASTWORDS);
	  ConqInfo->lastwords[MAXLASTWORDS - 1] = 0;
	  
	  return;
	}
      else
	{
	  cpMessage_t *cmsg = (cpMessage_t *)buf;

	  /* copy as much of the message as you can. */
	  strncpy(ConqInfo->lastwords, cmsg->msg, MAXLASTWORDS);
          ConqInfo->lastwords[MAXLASTWORDS - 1] = 0;
	}
    }
      
  /* Turn off sticky war so we can change war settings from menu(). */
  for ( i = 0; i < NUMPLAYERTEAMS; i++ )
    Ships[snum].rwar[i] = FALSE;
  
  return;
  
}

/* send all pertinent data, and any users attached to them. */
int updateClient(void)
{
  int i,j;
  static int sentallusers = FALSE; /* we will send all user data once. */
  static time_t oldtime = 0;
  time_t newtime = getnow(NULL, 0);
  int seciter = FALSE;
  static time_t histtime = 0;   /* timers that try to save some time() */
  static time_t infotime = 0;
  static time_t teamtime = 0;
  int dohist = FALSE;
  int doinfo = FALSE;
  int doteam = FALSE;

  
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
        if (Users[i].live)
          if (!sendUser(sInfo.sock, i))
            return FALSE;
    }

  for (i=1; i<=MAXSHIPS; i++)
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

  for (i=1; i<=NUMPLANETS; i++)
    {
      if (Planets[i].real)
	sendPlanet(sInfo.sock, i);
    }

  if (doteam)
    for (i=0; i<NUMALLTEAMS; i++)
      sendTeam(sInfo.sock, i, FALSE);

  if (doinfo)
    sendConqInfo(sInfo.sock, FALSE);

  if (dohist)
    {
      for (i=0; i<MAXHISTLOG; i++)
        {
          if (History[i].histunum >= 0)
            sendUser(sInfo.sock, History[i].histunum);
          
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
	  int team = (int)((Unsgn16)ntohs(ccmd->detail));

	  if (team >= 0 && team < NUMPLAYERTEAMS)
	    {

	      if (Users[Context.unum].ooptions[OOPT_SWITCHTEAMS] && 
		  Users[Context.unum].ooptions[OOPT_MULTIPLE] <= 1)
		{
		  Ships[Context.snum].team = team; 
		  Ships[Context.snum].shiptype = 
		    Teams[Ships[Context.snum].team].shiptype;
		  Users[Context.unum].team = Ships[Context.snum].team;
		  Ships[Context.snum].war[Ships[Context.snum].team] = FALSE;
		  Users[Context.unum].war[Users[Context.unum].team] = FALSE;
		}

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
      procReload(ccmd);

      break;

    case CPCMD_DISCONNECT:
      clog("CPCMD_DISCONNECT");
      /* 'fake' signal.  cleans up and exits. */
      handleSignal(0);
      /* NOTREACHED */
      break;

    default:
      clog("conquestd: handleSimpleCmdPkt(): unexpected command code %d",
	   cmd);
      break;
    }

  return;
}

/* DOES LOCKING */
void freeship(void)
{
  conqstats( Context.snum );
  PVLOCK(&ConqInfo->lockword);
  Ships[Context.snum].sdfuse = 0;
  Ships[Context.snum].status = SS_OFF;
  PVUNLOCK(&ConqInfo->lockword);
  return;
}

/*  menu - main user menu (DOES LOCKING) */
/*  SYNOPSIS */
/*    menu */
void menu(void)
{
  int i, sleepy, countdown;
  int lose, oclosed, switchteams, multiple, redraw;
  int playrv;
  int pkttype;
  Unsgn8 buf[PKT_MAXSIZE];
  cpCommand_t *ccmd;
  int sockl[2] = {sInfo.sock, sInfo.usock};

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
  Ships[Context.snum].killedby = 0;

  for ( i = 0; i < NUMPLAYERTEAMS; i = i + 1 )
    {
      Ships[Context.snum].rwar[i] = FALSE;
      Ships[Context.snum].war[i] = Users[Context.unum].war[i];
    }
  stcpn( Users[Context.unum].alias, Ships[Context.snum].alias, MAXUSERPNAME );
  
  /* Set up some things for the menu display. */
  switchteams = Users[Context.unum].ooptions[OOPT_SWITCHTEAMS];
  multiple = Users[Context.unum].ooptions[OOPT_MULTIPLE];
  oclosed = ConqInfo->closed;
  Context.leave = FALSE;
  redraw = TRUE;
  sleepy = 0;
  countdown = 0;
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
      if (!updateClient())	/* sends packets */
	{
	  freeship();
	  return;
	}

      /* Make sure things are proper. */
      if (playrv == ERR) 
	{
	  if ( Context.snum < 1 || Context.snum > MAXSHIPS )
	    lose = TRUE;
	  else if ( Ships[Context.snum].pid != Context.pid )
	    lose = TRUE;
	  else if ( Ships[Context.snum].status != SS_RESERVED )
	    {
	      clog( "menu(): Ship %d no longer reserved.", Context.snum );
	      lose = TRUE;
	    }
	  else
	    lose = FALSE;
	}
      else
	lose = FALSE;

      if ( lose )				/* again, Jorge? */
	{
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_FATAL, PERR_LOSE, NULL);
	  return;
	}
      
      /* Try to kill the driver if we started one the last time */
      /*  we played and we've been in the menu long enough. */
      if ( countdown > 0 )
	{
	  countdown = countdown - 1;
	  if ( countdown <= 0 )
	    drkill();
	}
      
      /* Reset up the destruct fuse. */
      Ships[Context.snum].sdfuse = -TIMEOUT_PLAYER;
      
      if ((pkttype = waitForPacket(PKT_FROMCLIENT, sockl, PKT_ANYPKT,
			buf, PKT_MAXSIZE, 1, NULL)) < 0)
	{
	  freeship();
	  clog("conquestd:menu: waitforpacket returned %d", pkttype); 
          handleSignal(0);
          /* not reached */

	  return;
	}
      
      if ( pkttype == 0 )	/* timeout */
	{
	  /* We get here if a packet hasn't been recieved. */
	  sleepy++;
	  if ( sleepy > 300 )
	    break;
	  continue; /* next */
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
	      if ( Context.childpid != 0 )
		countdown = 15;
	      else
		countdown = 0;
	      break;
	    }
	  else if (ccmd->cmd == CPCMD_RESIGN)
	    {
	      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
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
	  clog("conquestd: MENU: got unexp packet type %d", pkttype);
	  break;
	}
      
      /* Got a character, zero timeout. */
      
      sleepy = 0;
    }
  while ( clbStillAlive( Context.snum ) &&  !Context.leave );
  
  /* Make our ship available for others to use. */
  if ( Ships[Context.snum].status == SS_RESERVED )
    freeship();
  
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

  PVLOCK(&ConqInfo->lockword);
  
  Ships[*snum].status = SS_ENTERING;		/* show intent to fly */

  fresh = TRUE;				/* assume we want a fresh ship*/
  
  /* Count number of his ships flying. */
  j = 0;
  numvec = 0;
  for ( i = 1; i <= MAXSHIPS; i = i + 1 )
    if ( Ships[i].status == SS_LIVE || Ships[i].status == SS_ENTERING )
      if ( Ships[i].unum == unum && *snum != i )
	{
	  j++;
	  vec[numvec++] = i;
	}

  PVUNLOCK(&ConqInfo->lockword);

  if ( ! Users[unum].ooptions[OOPT_MULTIPLE] )
    {
      /* Isn't a multiple; see if we need to reincarnate. */
      if ( j > 0 )
	{
	  /* Need to reincarnate. */

	  if (!SVACANT(vec[0]))
	    {		   /* if it's available, we'll take it */
			   /* ...if it's not already being flown... */
	      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_ERROR, PERR_FLYING, NULL);
	      Ships[*snum].status = SS_RESERVED;
	      return ( FALSE );
	    }


	  /* Look for a live ship for us to take. */
	  PVLOCK(&ConqInfo->lockword);
	  for ( i = 1; i <= MAXSHIPS; i = i + 1)
	    if ( Ships[i].unum == unum && Ships[i].status == SS_LIVE )
	      {
		fresh = FALSE;
		Ships[*snum].status = SS_OFF;
		*snum = i;
		Ships[*snum].pid = Context.pid;
		Ships[*snum].status = SS_ENTERING;
		bitClear(Ships[*snum].flags, SHIP_F_VACANT);
		break;
	      }
	  PVUNLOCK(&ConqInfo->lockword);
	}
    }
  else
    {				/* a multiple, so see what's available */
      PVLOCK(&ConqInfo->lockword);
      
      /* Count number of his ships flying. */
      j = 0;
      numvec = 0;
      for ( i = 1; i <= MAXSHIPS; i = i + 1 )
	if ( Ships[i].status == SS_LIVE || Ships[i].status == SS_ENTERING )
	  if ( Ships[i].unum == unum && *snum != i )
	    {
	      j++;
	      vec[numvec++] = i;
	      /* JET maybe we should turn off vacant ships for
		 multiples? */
	    }
      
      PVUNLOCK(&ConqInfo->lockword);
      
      /* Is a multiple, max ships already in and no ships to
	 reincarnate too */
      if ( j >= Users[unum].multiple && numavail == 0)
	{
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_ERROR, PERR_TOOMANYSHIPS, 
		  NULL);
	  Ships[*snum].status = SS_RESERVED;
	  
	  return ( FALSE );
	}
      
      /* we can squeeze a new one in. */
	  
	  if ( j < Users[unum].multiple)
	    fresh = TRUE;
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
	  return ( ERR );
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

  PVLOCK(&ConqInfo->lockword);
  
  /* If necessary, initalize the ship */
  if ( fresh )
    {
      clbInitShip( *snum, unum );
      
      /* Randomly position the ship near the home sun (or planet). */
      if ( Planets[Teams[system].homeplanet].primary == Teams[system].homesun )
	i = Teams[system].homesun;
      else
	i = Teams[system].homeplanet;
      clPutShip( *snum, Planets[i].x, Planets[i].y );
      Ships[*snum].dhead = rnduni( 0.0, 359.9 );
      Ships[*snum].head = Ships[*snum].dhead;
      Ships[*snum].dwarp = (real) rndint( 2, 5 ) ;/* #~~~ this is a kludge*/
      Ships[*snum].lock = -Teams[system].homeplanet;
    }
  else
    {				/* if we're reincarnating, skip any
				   messages that might have been sent
				   while we were gone */
      PVLOCK(&ConqInfo->lockmesg);
      Ships[*snum].lastmsg = ConqInfo->lastmsg;
      Ships[*snum].alastmsg = Ships[*snum].lastmsg;
      PVUNLOCK(&ConqInfo->lockmesg);
      /* init user's last entry time */
      Users[Ships[*snum].unum].lastentry = getnow(NULL, 0);
    }
      
  SFCLR(*snum, SHIP_F_ROBOT);
  Ships[*snum].action = 0;
  
  
  /* Straighten out the ships deltas. */
  clbFixDeltas( *snum );
  
  /* Finally, turn the ship on. */
  Ships[*snum].status = SS_LIVE;
  
  PVUNLOCK(&ConqInfo->lockword);
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
  Unsgn8 buf[PKT_MAXSIZE];
  int sockl[2] = {sInfo.sock, sInfo.usock};

  /* Can't carry on without a vessel. */
  if ( (rv = newship( Context.unum, &Context.snum )) != TRUE)
    {
      clog("conquestd:play: newship() returned %d",
	   rv);
      return(rv);
    }
  
  drstart();			/* start a driver, if necessary */
  Ships[Context.snum].sdfuse = 0;	/* zero self destruct fuse */
  grand( &Context.msgrand );		/* initialize message timer */
  Context.leave = FALSE;		/* assume we won't want to bail */
  Context.redraw = TRUE;		/* want redraw first time */
  Context.msgok = TRUE;		/* ok to get messages */
  Context.display = FALSE;		/* ok to get messages */
  stopUpdate();			/* stop the display interrupt */
  gsecs( &laststat );		/* initialize stat timer */

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
  
  clbStoreMsg(MSG_COMP, MSG_ALL, msgbuf);
  

  /* client updates will be handled by updateProc */
  /* While we're alive, field commands and process them. */
  while ( clbStillAlive( Context.snum ) )
    {
      /* Make sure we still control our ship. */
      if ( Ships[Context.snum].pid != Context.pid )
	break;

      didsomething = 0;
      if ((pkttype = waitForPacket(PKT_FROMCLIENT, sockl, PKT_ANYPKT,
				   buf, PKT_MAXSIZE, 0, NULL)) < 0)
	{
	  if (errno != EINTR)
	    {
	      clog("conquestd:play:waitForPacket: %s", strerror(errno));
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
	    clog("conquestd: play: got unexpected packet type %d", pkttype);
	  break;
	}

      if (didsomething)         /* update immediately if we did something */
        {
          stopUpdate();
          updateClient();
          startUpdate();
        }

      grand( &Context.msgrand );
      Context.msgok = TRUE;
      
      /* See if it's time to update the statistics. */
      if ( dsecs( laststat, &now ) >= 15 )
	{
	  conqstats( Context.snum );
	  laststat = now;
	}

      c_sleep(ITER_SECONDS);
    }
  
  conqstats( Context.snum );
  upchuck();
  
  /* Asts are still enabled, simply cancel the next screen update. */


  stopUpdate();
  updateClient();	/* one last, to be sure. */
  sendConqInfo(sInfo.sock, TRUE);
  c_sleep( 2.0 );
  clog("INFO: ship %d died, calling dead()", Context.snum);
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
  Unsgn8 flags = SPCLNTSTAT_FLAG_NONE;

  if (!Authenticate(name, password))
    return FALSE;

  sInfo.isLoggedIn = TRUE;

  if ( ! clbGetUserNum( unum, name, 0 ) )
    {				
      flags |= SPCLNTSTAT_FLAG_NEW;
      /* Must be a new player. */
      if ( ConqInfo->closed )
	{
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_FATAL, PERR_CLOSED,
                  NULL);
	  return ( FALSE );
	}
      team = rndint( 0, NUMPLAYERTEAMS - 1 );
      clog("CONQD: TEAM = %d", team);
      
      cbuf[0] = EOS;
      apptitle( team, cbuf );
      appchr( ' ', cbuf );
      i = strlen( cbuf );
      appstr( name, cbuf );
      cbuf[i] = (char)toupper( cbuf[i] );

      if ( ! clbRegister( name, cbuf, team, unum ) )
	{
	  sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_FATAL, PERR_REGISTER,
                  NULL);
	  return ( FALSE );
	}

      clog("conquestd: clbRegister COMPLETE!!!! unum = %d, team = %d\n", 
	   *unum, team);

				/* copy in the password */
      strcpy(Users[*unum].pw, password);
				/* set lastentry time for new players */
      Users[*unum].lastentry = getnow(NULL, 0);

    }


  /* Must be special to play when closed. */
  if ( ConqInfo->closed && ! Users[*unum].ooptions[OOPT_PLAYWHENCLOSED] )
    {
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_FATAL, PERR_CLOSED,
	      NULL);
      clog("conquestd: welcome: game closed\n");
      return ( FALSE );
    }

  /* Can't play without a ship. */
  if ( ! clbFindShip( &Context.snum ) )
    {
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_FATAL, PERR_NOSHIP,
	      NULL);
      clog("WELCOME: findship failed");
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
  Unsgn8 buf[PKT_MAXSIZE];
  char cbuf[MESSAGE_SIZE * 2];
  int pkttype;
  extern char *ConquestVersion, *ConquestDate;
  int rv;
  struct timeval tv;
  fd_set readfds;
  struct sockaddr_in usa;	/* internet socket addr. structure - udp */
  cpAck_t *cpack;
  int sockl[2] = {sInfo.sock, sInfo.usock};

  /* open a UDP socket and bind to it */
  if ((sInfo.usock = udpOpen(listenPort, &usa)) < 0)
    {
      clog("NET: SERVER hello: udpOpen() failed: %s", strerror(errno));
      sInfo.usock = -1;
      sInfo.tryUDP = FALSE;
    }

  /* first loadup and send a server hello */
  shello.type = SP_HELLO;
  shello.protover = (Unsgn16)htons(PROTOCOL_VERSION);

  shello.cmnrev = (Unsgn32)htonl(COMMONSTAMP);
  strncpy(shello.servername, SysConf.ServerName, CONF_SERVER_NAME_SZ);
  strncpy(shello.serverver, ConquestVersion, CONF_SERVER_NAME_SZ);
  strcat(shello.serverver, " ");
  strncat(shello.serverver, ConquestDate, 
         (CONF_SERVER_NAME_SZ - strlen(ConquestVersion)) - 2);
  strncpy(shello.motd, SysConf.ServerMotd, CONF_SERVER_MOTD_SZ);
  shello.flags = 0;

  if (ConqInfo->closed)
    shello.flags |= SPHELLO_FLAGS_CLOSED;

  if (!writePacket(PKT_TOCLIENT, sInfo.sock, (Unsgn8 *)&shello))
    {
      clog("NET: SERVER: hello: write shello failed\n");
      return FALSE;
    }

  clog("NET: SERVER: hello: sent server hello to client");

  if (sInfo.tryUDP)
    {
      /* wait a few seconds to see if client sends a udp */
      tv.tv_sec = 4;
      tv.tv_usec = 0;
      FD_ZERO(&readfds);
      FD_SET(sInfo.usock, &readfds);
      if ((rv = select(sInfo.usock+1, &readfds, NULL, NULL, &tv)) <= 0)
        {
          clog("NET: SERVER: hello: udp select failed: %s", strerror(errno));
          sInfo.tryUDP = FALSE;
        }
      else
        {
          if (FD_ISSET(sInfo.usock, &readfds))
            {                       /* get the packet, almost done negotiating udp */
              rv = udpRecv(sInfo.usock, buf, PKT_MAXSIZE, &sInfo.clntaddr);
              clog("NET: SERVER: hello: got %d UDP bytes from client port %d", rv, 
                   (int)ntohs(sInfo.clntaddr.sin_port));
              
              if (connect(sInfo.usock, (const struct sockaddr *)&sInfo.clntaddr, 
                          sizeof(sInfo.clntaddr)) < 0)
                {
                  clog("NET: SERVER: hello: udp connect() failed: %s", strerror(errno));
                  sInfo.tryUDP = FALSE;
                }
            }
        }
    }

  /* now we want a client hello in response */
  if ((pkttype = readPacket(PKT_FROMCLIENT, sockl, buf, PKT_MAXSIZE, 10)) < 0)
  {
    clog("NET: SERVER: hello: read client hello failed, pkttype = %d",
         pkttype);
    return FALSE;
  }

  if (pkttype == 0)
  {
    clog("NET: SERVER: hello: read client hello: timeout.\n");
    return FALSE;
  }

  if (pkttype != CP_HELLO)
  {
    clog("NET: SERVER: hello: read client hello: wrong packet type %d\n", pkttype);
    return FALSE;
  }

  chello = *(cpHello_t *)buf;

  /* fix up byte ordering */
  chello.protover = ntohs(chello.protover);
  chello.cmnrev = ntohl(chello.cmnrev);

  chello.clientname[CONF_SERVER_NAME_SZ - 1] = 0;
  chello.clientver[CONF_SERVER_NAME_SZ - 1] = 0;

  clog("CLIENTID:%s:%s:%d:0x%04hx:%d",
       chello.clientname, 
       chello.clientver,
       chello.updates, 
       chello.protover, 
       chello.cmnrev);

  /* do some checks - send a NAK and fail if things aren't cool */
  if (chello.protover != PROTOCOL_VERSION)
    {
      sprintf(cbuf, "HELLO: SVR: protocol mismatch, expect 0x%04x, got 0x%04x",
	      PROTOCOL_VERSION, chello.protover);
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_FATAL, PERR_BADPROTO, cbuf);
      return FALSE;
    }

  if (chello.cmnrev != COMMONSTAMP)
    {
      sprintf(cbuf, "HELLO: SVR: CMB mismatch, expect %d, got %d",
	      COMMONSTAMP, chello.cmnrev);
      sendAck(sInfo.sock, PKT_TOCLIENT, PSEV_FATAL, PERR_BADCMN, cbuf);
    }

  if (chello.updates >= 1 && chello.updates <= 10)
    Context.updsec = chello.updates;

  /* send a server stat to the udp client socket.  If the client gets
     it, it will acknowlege it in it's ACK packet, which will tell us
     we can do udp. woohoo! */
  if (sInfo.tryUDP)
    sendServerStat(sInfo.usock);

  /* now send the server stats normally */
  if (!sendServerStat(sInfo.sock))
    return FALSE;

  /* now we want an ack.  If we get it, we're done! */
  if ((pkttype = readPacket(PKT_FROMCLIENT, sockl, buf, PKT_MAXSIZE, 5)) < 0)
    {
      clog("NET: SERVER: hello: read client Ack failed");
      return FALSE;
    }

  if (pkttype != CP_ACK)
    return FALSE;

  if (sInfo.tryUDP)
    {
      /* see if the client could read our udp */
      cpack = (cpAck_t *)buf;
      if (cpack->code == PERR_DOUDP)
        {
          sInfo.doUDP = TRUE;
          clog("NET: SERVER: hello: Client acknowleged UDP from server. Doing UDP.");
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
    clog("conquestd: exiting on signal %d", sig);
  
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
          clbKillShip( Context.snum, KB_LIGHTNING );
          /* turn ship off */
          Ships[Context.snum].status = SS_OFF;
        }
    }
  else
    {                       /* not playing (main menu, etc) */
      /* if we aren't playing, then just turn it off */
      if (Context.snum >= 1 && Context.snum <= MAXSHIPS)
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
      clbStoreMsg(MSG_COMP, MSG_ALL, msgbuf);
    }
  
  recordCloseOutput();

  return;
  
}
