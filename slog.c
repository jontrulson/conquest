/* server for logging calls using Eric's script based flat file database */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "slog.h"

#define BACKLOG 5		/* # of requests we're willing to to queue */
#define MAXHOSTNAME	32	/* maximum host name length we tolerate */

main ( int argc, char **argv )
{
  int s,t;		/* socket descriptor */
  int i;			/* general purpose integer */
  int len;		/* length of received data */
  struct sockaddr_in sa,isa;	/* internet socket addr. structure */
  struct hostent *hp;	/* result of host name lookup */
  char *myname;		/* pointer to name of this pgm */
  char localhost[MAXHOSTNAME+1];	/* local host name as char string */
  
  myname = argv[0];
  
  signal(SIGCLD, SIG_IGN);	/* JET 12/1/93 - allow children to die */
  
  /* get our own host information */
  gethostname ( localhost, MAXHOSTNAME );
  if ( ( hp = gethostbyname ( localhost ) ) == NULL ) {
    fprintf ( stderr, "%s: cannot get local host info?\n", myname );
    exit ( 1 );
  }
  else
    printf("Hostname: %s\n",
	   localhost);
  
  /* put slog socket number and our address info into the
   * socket structure 
   */
  sa.sin_port = htons(SLOG_PORT);
  bcopy ( (char *)hp->h_addr, (char *)&sa.sin_addr, hp->h_length );
  sa.sin_family = hp->h_addrtype;
  
  /* allocate an open sockeet for incoming connections */
  if ( ( s = socket ( hp->h_addrtype, SOCK_STREAM, 0 ) ) < 0 ) {
    perror ( "socket" );
    exit ( 1 );
  }
  
  /* bind the socket to the service port so we hear incoming
   * connections 
   */
  if ( bind ( s, &sa, sizeof ( sa )) < 0 ) {
    perror ( "bind" );
    exit ( 1 );
  }
  
  /* set the maximum connections we will fall behind */
  listen ( s, BACKLOG );
  
  /* go into infinite loop waiting for new connections */
  for (;;) {
    i = sizeof (isa);
    
    /* hang in accept() while waiting for new connections */
    if ( ( t = accept ( s, &isa, &i ) ) < 0 ) {
      perror ( "accept" );
      exit ( 1 );
    }
    
    if ( fork() == 0 ) {
      slog (t);	/* perform actual slog service */
      close (t);
      exit (0);
    }
    close(t);	/* JET 1/6/93 make socket go away! */
  }
  
}

/* get the data record from client and write to file */

slog (int sock)
{
  time_t filetime;
  struct tm *ctm;
  char buf[BUFSIZ];
  int i;
  struct sockaddr_in addr;
  socklen_t len;
  struct hostent *host;
  
  len = sizeof(struct sockaddr_in);
  if (getpeername(sock, (struct sockaddr *) &addr, &len) < 0)
    perror("getpeername");
  else
    {
      if ((host = gethostbyaddr((char *) &addr.sin_addr.s_addr,
				sizeof(unsigned long),
				AF_INET)) == NULL)
	{
	  perror("getpeername");
	}
      else
	{
	  printf("Connect from %s\n", host->h_name);
	}
    }
  
  /* get one line request */
  if ( ( i = read ( sock, buf, BUFSIZ ) ) <= 0 )
    return;
  buf[i] ='\n';	/* add newline */
  buf[i+1] ='\0';	/* null terminate */
  
  /* get the date so we know the name of the file to use */
  filetime= time(0);
  ctm= localtime(&filetime);
  
  /* now write the record to the file */
  write ( 1, buf, strlen ( buf ) );
  bzero ( buf, BUFSIZ ); 
  
  /* close the file */
  
  return;

}
	

