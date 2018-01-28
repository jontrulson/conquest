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

#define NOEXTERN_CONTEXT
#include "context.h"

#define NOEXTERN_GLOBALS
#include "global.h"

#define NOEXTERN_CONF
#include "conf.h"

#include "color.h"

#include "conqutil.h"
#include "rndlb.h"

#include "conqinit.h"

/* print_usage - print usage. Duh. */
void print_usage()
{
    fprintf(stderr, "Usage: conqinit [-f <file>] [-vdD] \n");
    fprintf(stderr, "       conqinit -f <file> -h [-vd] \n\n");
    fprintf(stderr, "\t-f <file>     read from <file>.\n");
    fprintf(stderr, "\t-v            be verbose about everything.\n");
    fprintf(stderr, "\t-d            turns on debugging.\n");
    fprintf(stderr, "\t-t            parse texture data instead of conqinit data\n");
    fprintf(stderr, "\t-s            parse sound data instead of conqinit data\n");
    fprintf(stderr, "\t-D            dump current universe (global, shiptypes,"
                    "                planets) to stdout in conqinitrc format\n");
}


int main(int argc, char **argv)
{
    extern char *optarg;
    int debuglevel = 0, verbosity = 0, ftype = CQI_FILE_CONQINITRC;
    int ch;
    char *filenm = NULL;

    rndini();

    utSetLogConfig(false, true);    /* log + stderr! :) */

    while ( (ch = getopt( argc, argv, "vdDf:ts" )) != EOF )
    {      switch(ch)
	{
	case 'v':
            verbosity = true;
            break;
        case 'd':
            debuglevel++;
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
            break;
        case 's':
            ftype = CQI_FILE_SOUNDRC_ADD;
            break;
	default:
            print_usage();
            return 1;
	}
    }

    if (cqiLoadRC(ftype, filenm, verbosity, debuglevel))
    {
        fprintf(stderr, "conqinit: FATAL: load failed.\n");
        return 1;
    }

    return 0;

}
