#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#define NOEXTERN

#include "conqdef.h"
#include "conqcom.h"
#include "conqcom2.h"
#include "global.h"
#include "user.h"

static int nenum; 
static int debug; 
static real dne, ane;

/*  conqai - robot AI test program */

main(int argc, char *argv[])
{
  int i;
  string usage="usage: conqai [-vd]";
  int arg;
  
  /* First things first. */
  
  if ((ConquestUID = GetConquestUID()) == ERR)
    {
      fprintf(stderr, "conqai: GetConquestUID() failed\n");
      exit(1);
    }
  
  if ((ConquestGID = GetConquestGID()) == ERR)
    {
      fprintf(stderr, "conqai: GetConquestGID() failed\n");
      exit(1);
    }
  
  if (GetSysConf(FALSE) == ERR)
    {
#ifdef DEBUG_CONFIG
      clog("%s@%d: main(): GetSysConf() returned ERR.", __FILE__, __LINE__);
#endif
/*      exit(1);*/
    }
  
  if (setgid(ConquestGID) == -1)
    {
      clog("conqai: setgid(%d): %s",
	   ConquestGID,
	   strerror(errno));
      fprintf(stderr, "conqai: setgid(): failed\n");
      exit(1);
    }
  
#ifdef USE_SEMS
  if (GetSem() == ERR)
    {
      fprintf(stderr, "GetSem() failed to get semaphores. exiting.\n");
      exit(1);
    }
#endif
  
  
  
  map_common();
  
  if ( *CBlockRevision != COMMONSTAMP )
    error( "conqai: Common block ident mismatch.  \nInitialize the Universe via conqoper." );
  
  
  debug = FALSE;
  
  while ((arg = getopt(argc, argv, "d")) != EOF)
    {
      switch (arg)
	{
	case 'r':
	  exitai();		/* release robot control to driver */
	  printf("The conquest driver now has control of the robots\n");
	  exit(0);
	  break;		/* NOTREACHED */
	case 'd':
	  debug = TRUE;
	  break;
	default:
	  printf("Options: -d = debugging, -r = return robot control to driver\n");
	  exit(1);
	}
    }
  robotloop();
  
  exit(0);
  
}
