#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/* This is a very short program that should be used as the
   'shell' of a telnet conquest server.  Assuming you have setup a
   conquest user (suitable for telneting into), simply define
   this program as the shell.  When a user telnets in to the conquest account
   this program will run.

   It's sole purpose is to execute the conquest client binary with appropriate
   options (like -t).  It can be easily modified by a server operator
   to pass other options as well, if need be.  */

#include "conqdef.h"

#ifndef CONQUESTSH_EXEC            /* this is determined during configure */
#error "CONQUESTSH_EXEC was not defined on the compile line"
#endif

#define TELNET_OPT "-t"
#define SERVER_OPT1 "-s"
#define SERVER_OPT2 "localhost" /* change this if you'd like */

int main(int argc, char *argv[])
{
  execlp(CONQUESTSH_EXEC, 
        CONQUESTSH_EXEC, 
        TELNET_OPT,
        SERVER_OPT1,
        SERVER_OPT2,
        NULL);

  /* should never get here */
  fprintf(stderr, "exec %s failed: %s\n", CONQUESTSH_EXEC, strerror(errno));
          
  exit(0);
  
}
