#include "c_defs.h"

/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */


#include "conqdef.h"
#include "cb.h"
#include "conqutil.h"
#include "conqai.h"

#define NOEXTERN_CONTEXT
#include "context.h"
#include "conqunix.h"

#define NOEXTERN_GLOBALS
#include "global.h"

#include "user.h"

#define NOEXTERN_CONF
#include "conf.h"

#include "sem.h"

static int debug;
static void exitai(void);

/*  conqai - robot AI test program */

int main(int argc, char *argv[])
{
    int arg;

    /* First things first. */

    if ((ConquestGID = getConquestGID()) == -1)
    {
        fprintf(stderr, "conqai: getConquestGID() failed\n");
        exit(1);
    }

    if (GetSysConf(FALSE) == -1)
    {
#ifdef DEBUG_CONFIG
        utLog("%s@%d: main(): GetSysConf() returned -1.", __FILE__, __LINE__);
#endif
/*      exit(1);*/
    }

    if (setgid(ConquestGID) == -1)
    {
        utLog("conqai: setgid(%d): %s",
              ConquestGID,
              strerror(errno));
        fprintf(stderr, "conqai: setgid(): failed\n");
        exit(1);
    }

    if (semInit() == -1)
    {
        fprintf(stderr, "semInit() failed to get semaphores. exiting.\n");
        exit(1);
    }


    cbMap();

    if ( *CBlockRevision != COMMONSTAMP )
    {
        printf("conqai: Common block ident mismatch.\n"
               "Initialize the Universe via conqoper.\n");
        exit(1);
    }


    debug = FALSE;

    while ((arg = getopt(argc, argv, "dr")) != EOF)
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

/*  exitai - exit handler */
/*  SYNOPSIS */
/*    extern exitai */
static void exitai(void)
{

    ConqInfo->externrobots = FALSE;

    return;

}
