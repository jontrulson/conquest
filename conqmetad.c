#include "c_defs.h"

/************************************************************************
 *
 * conqmetad - the Conquest meta server.  Similiar to conquestd no? :)
 *
 * $Id$
 *
 * Copyright 2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/


#include "global.h"
#include "conqcom.h"
#define NOEXTERN
#include "context.h"
#undef NOEXTERN
#include "conqdef.h"
#include "conqnet.h"
#include "conf.h"

#include "meta.h"

#define LISTEN_BACKLOG 5 /* # of requests we're willing to to queue */

metaSRec_t metaServerList[META_MAXSERVERS];
static Unsgn16 listenPort = META_DFLT_PORT;
static char *progName;
static int localOnly = FALSE;   /* whether to only listen on loopback */
static int maxSlot = 0;         /* the max slot we've ever used */
const int expireSeconds = (60 * 5); /* 5 minutes */

void catchSignals(void);
void handleSignal(int sig);

void printUsage()
{
  printf("Usage: conqmetad [ -d ] [ -l ] [ -u user ]\n");
  printf("   -d            daemon mode\n");
  printf("   -l            listen for local connections only\n");
  printf("   -u user       run as user 'user'.\n");

  return;
}

void ageServers(void)
{
  int i;
  time_t now = getnow(NULL, 0);

  for (i=0; i<maxSlot; i++)
    {
      if (metaServerList[i].valid)
        if (abs(metaServerList[i].lasttime - now) > expireSeconds)
          {
            clog("META: expiring %s(%s)\n",
                 metaServerList[i].altaddr,
                 metaServerList[i].addr);
            metaServerList[i].valid = FALSE;
          }
    }

  return;
}


/* find the server slot for this record (or make a new one).  returns the
   slot number found/'created', or -1 for error */
int findSlot(metaSRec_t *srec, int *isupdate)
{
  int i, found;
  int rv = 0;

  *isupdate = FALSE;

  /* first look for it */
  found = FALSE;
  for (i=0; i<maxSlot; i++)
    {
      if ((!strcmp(metaServerList[i].addr, srec->addr)) &&
          (!strcmp(metaServerList[i].altaddr, srec->altaddr)) &&
          metaServerList[i].valid)
        {
          rv = i;
          found = TRUE;
          *isupdate = TRUE;
          break;
        }
    }

  if (!found)
    {                           /* didn't find one */
      for (i=0; i<META_MAXSERVERS; i++)
        if (!metaServerList[i].valid)
          {                     /* found a slot */
            rv = i;
            if (rv >= maxSlot)
              maxSlot = rv + 1;
            found = TRUE;
            break;
          }
    }

  if (!found)
    return -1;                  /* nothing there george */
  else
    return rv;
}


void initServerList()
{
  int i;

  memset((void *)&metaServerList, 0, sizeof(metaSRec_t) * META_MAXSERVERS);

  for (i=0; i<META_MAXSERVERS; i++)
    metaServerList[i].valid = FALSE;

  return;
}

void metaProcList(int sock, char *hostbuf)
{
  char tbuf[BUFFERSZ];
  int i;

  /* dump the server list */
  for (i=0; i<maxSlot; i++)
    {
      if (metaServerList[i].valid)
        {
          srec2str(tbuf, &metaServerList[i]);
          write(sock, tbuf, strlen(tbuf));
        }
    }

  clog("META: server query from %s", hostbuf);

  return;
}

void metaProcUpd(char *buf, int rlen, char *hostbuf)
{
  metaSRec_t sRec;
  int slot;
  int wasfound;

  if (!str2srec(&sRec, buf))
    {
      clog("META: malformed buffer '%s', ignoring", buf);
      return;
    }

  strncpy(sRec.addr, hostbuf, CONF_SERVER_NAME_SZ - 1);

  /* if altaddr is empty, we copy hostbuf into it. */

  if (strlen(sRec.altaddr) == 0)
    strncpy(sRec.altaddr, sRec.addr, CONF_SERVER_NAME_SZ - 1);

  /* now find a slot for it. */
  if ((slot = findSlot(&sRec, &wasfound)) == -1)
    {
      clog("META: findSlot couldn't find one, ignoring\n");
      return;
    }

  /* init the slot */
  sRec.valid = TRUE;
  sRec.lasttime = getnow(NULL, 0);
  if (sRec.port == 0)
    sRec.port = CN_DFLT_PORT;
  metaServerList[slot] = sRec;

  if (!wasfound)                /* new server */
    clog("META: Added server %s(%s), slot %d",
         metaServerList[slot].altaddr,
         metaServerList[slot].addr,
         slot);
  else
    {
#if defined(DEBUG_META)
      clog("META: Updated server %s(%s)",
         metaServerList[slot].altaddr,
           metaServerList[slot].addr);
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
  socklen_t len;
  struct hostent *hp;
  int rv, rlen, i;
  char hostbuf[CONF_SERVER_NAME_SZ];
  char rbuf[BUFFERSZ];
  int alen;
  fd_set readfds;

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
      perror ( "UDP socket" );
      exit(1);
    }
  
  /* allocate an open socket for incoming TCP connections */
  if (( t = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
      perror ( "TCP socket" );
      exit(1);
    }
  
  /* bind the socket to the service port so we hear incoming
   * connections 
   */
  if ( bind( s, (struct sockaddr *)&sa, sizeof ( sa )) < 0 ) 
    {
      perror( "UDP bind" );
      exit(1);
    }

  if ( bind( t, (struct sockaddr *)&tsa, sizeof ( tsa )) < 0 ) 
    {
      perror( "TCP bind" );
      exit(1);
    }

  /* set the maximum connections we will fall behind */
  listen( t, LISTEN_BACKLOG );

  alen = sizeof(struct sockaddr);

  clog("NET: meta server listening on TCP and UDP port %d\n", listenPort);
  
  /* go into infinite loop waiting for new connections */
  while (TRUE) 
    {
      tv.tv_sec = 30;           /* age servers every 30 secs */
      tv.tv_usec = 0;
      FD_ZERO(&readfds);
      FD_SET(s, &readfds);
      FD_SET(t, &readfds);

      if ((rv = select(max(s,t)+1, &readfds, NULL, NULL, &tv)) < 0)
        {
          clog("META: select failed: %s", strerror(errno));
          exit(1);
        }

      /* TCP (list) socket */
      if (FD_ISSET(t, &readfds))
        {
          i = sizeof (isa);
          if ((tc = accept(t, (struct sockaddr *)&tisa, &i )) < 0) 
            {
              clog("META: accept failed: %s", strerror(errno) );
              continue;
            }
          
          if ((hp = gethostbyaddr((char *) &tisa.sin_addr.s_addr,
                                  sizeof(unsigned long),
                                  AF_INET)) == NULL)
            {
              strncpy(hostbuf, inet_ntoa((struct in_addr)tisa.sin_addr), 
                      CONF_SERVER_NAME_SZ);
            }
          else
            {
              strncpy(hostbuf, hp->h_name, CONF_SERVER_NAME_SZ);
            }
          
          hostbuf[CONF_SERVER_NAME_SZ - 1] = 0;

          metaProcList(tc, hostbuf);
          close(tc);
        }

      /* UDP (ping) socket */
      if (FD_ISSET(s, &readfds))
        {
          memset(rbuf, 0, BUFFERSZ);
          rlen = recvfrom(s, rbuf, BUFFERSZ, 0, 
                          (struct sockaddr *)&isa, &alen);

          
          len = sizeof(struct sockaddr_in);

          if ((hp = gethostbyaddr((char *) &isa.sin_addr.s_addr,
                                  sizeof(unsigned long),
                                  AF_INET)) == NULL)
            {
              strncpy(hostbuf, inet_ntoa((struct in_addr)isa.sin_addr), 
                      CONF_SERVER_NAME_SZ);
            }
          else
            {
              strncpy(hostbuf, hp->h_name, CONF_SERVER_NAME_SZ);
            }

          hostbuf[CONF_SERVER_NAME_SZ - 1] = 0;

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
  int dodaemon = FALSE;

  progName = argv[0];

  while ((i = getopt(argc, argv, "dlp:u:")) != EOF)    /* get command args */
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
      default:
	printUsage();
	exit(1);
      }

  if ((ConquestUID = GetUID(ROOT_USER)) == ERR)
    {
      fprintf(stderr, "%s: GetUID() failed\n", progName);
      exit(1);
    }
  
  if ((ConquestGID = GetConquestGID()) == ERR)
    {
      fprintf(stderr, "%s: GetConquestGID() failed\n", progName);
      exit(1);
    }
  

  /* at this point, we see if the -u option was used.  If it was, we
     setuid() to it */

  if (myuidname)
    {
      int myuid;

      if ((myuid = GetUID(myuidname)) == ERR)
        {
          fprintf(stderr, "%s: GetUID(%s) failed\n", progName, myuidname);
          exit(1);
        }

      if (setuid(myuid) == -1)
        {
          fprintf(stderr, "%s: setuid(%d) failed: %s\n", progName, myuid,
                  strerror(errno));
          exit(1);
        }        
      else
        clog("INFO: META running as user '%s', uid %d.", myuidname, myuid); 
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

  initServerList();

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

  return;
}


void handleSignal(int sig)
{
  clog("META: exiting on signal %d", sig);
  exit(0);
}
