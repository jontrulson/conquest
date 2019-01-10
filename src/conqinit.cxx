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

#include "context.h"

#include "global.h"

#include "conf.h"

#include "color.h"

#include "conqutil.h"
#include "rndlb.h"

#include "conqinit.h"

/* print_usage - print usage. Duh. */
void print_usage()
{
    fprintf(stderr, "Usage: conqinit [-f <file>] [-v] [-D] [-t | -s] [-G <subdir>] \n");
    fprintf(stderr, "\t-G <subdir>   specify alternate game subdirectory\n");
    fprintf(stderr, "\t-f <file>     read from <file>.\n");
    fprintf(stderr, "\t-v            increase verbosity level by 1.\n");
    fprintf(stderr, "\t-t            parse texture data instead of conqinit data\n");
    fprintf(stderr, "\t-s            parse sound data instead of conqinit data\n");
    fprintf(stderr, "\t-D            dump current universe (global, shiptypes,\n"
                    "                planets) to stdout in conqinitrc format\n");
}


int main(int argc, char **argv)
{
    int verbosity = 0, ftype = CQI_FILE_CONQINITRC;
    int ch;
    char *filenm = NULL;

    rndini();

    utSetLogConfig(false, true);    /* log + stderr! :) */
    utSetLogProgramName(argv[0]);

    while ( (ch = getopt( argc, argv, "Df:G:stv" )) != EOF )
    {
        switch(ch)
	{
	case 'v':
            verbosity++;
            break;
        case 'D':
            dumpUniverse();
            return 0;
            break; /* NOTREACHED */
        case 'f':
            filenm = optarg;
            break;
        case 't':
            ftype = CQI_FILE_TEXTURESRC_ADD;
            verbosity++;        // get stats on this option
            break;
        case 's':
            ftype = CQI_FILE_SOUNDRC_ADD;
            verbosity++;        // get stats on this option
            break;
        case 'G':
            gameSubdirectory.set(optarg);
            break;
	default:
            print_usage();
            return 1;
	}
    }

    if (cqiLoadRC(ftype, filenm, verbosity))
    {
        fprintf(stderr, "conqinit: FATAL: cqiLoadRC() failed.\n");
        return 1;
    }

    return 0;

}
