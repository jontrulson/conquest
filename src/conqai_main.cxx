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

#include "conqdef.h"
#include "cb.h"
#include "conqutil.h"
#include "conqai.h"

#include "context.h"
#include "conqunix.h"

#include "global.h"

#include "user.h"

#include "conf.h"

#include "sem.h"
#include "conqinit.h"

static int debug;
static void exitai(void);

/*  conqai - robot AI test program */

int main(int argc, char *argv[])
{
    int arg;

    /* First things first. */

    utSetLogProgramName(argv[0]);

    if ((ConquestGID = getConquestGID()) == -1)
    {
        fprintf(stderr, "conqai: getConquestGID() failed\n");
        exit(1);
    }

    if (GetSysConf(false) == -1)
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

    debug = false;

    while ((arg = getopt(argc, argv, "dG:r")) != EOF)
    {
        switch (arg)
	{
	case 'r':
            exitai();		/* release robot control to driver */
            printf("The conquest driver now has control of the robots\n");
            exit(0);
            break;		/* NOTREACHED */
        case 'G':
            gameSubdirectory.set(optarg);
            break;
	case 'd':
            debug = true;
            break;
	default:
            printf("Options: -d = debugging, -r = return robot control to driver\n");
            printf("         -G <subdir> = specify alternate game subdirectory\n");
                   exit(1);
	}
    }

    // load the globals/planets (conqinitrc), before we map...
    if (cqiLoadRC(CQI_FILE_CONQINITRC, NULL, 0))
    {
        printf("FATAL: can't load conqinitrc file, check log.\n");
        exit(1);
    }

    cbMap();

    if ( *cbRevision != COMMONSTAMP )
    {
        fprintf(stderr, "conquestd: Common block ident mismatch.\n" );
        fprintf(stderr, "           You must initialize the universe with conqoper.\n" );
        exit(1);
    }


    robotloop();

    exit(0);

}

/*  exitai - exit handler */
/*  SYNOPSIS */
/*    extern exitai */
static void exitai(void)
{

    cbConqInfo->externrobots = false;

    return;

}
