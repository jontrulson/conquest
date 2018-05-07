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

#ifndef CONTEXT_H
#define CONTEXT_H

#include <string>

#include "ship.h"
#include "record.h"

/* a 'Conquest Context' for the current player/operator */

typedef struct {
    int unum;                   /* user number */
    int snum;                   /* ship number */

    pid_t pid;                  /* pid of our owner process */
    pid_t childpid;             /* pid of driver we spawned */

    // global "text terminal" limits
    int maxlin;                 /* number of lines */
    int maxcol;                 /* number of columns */

    uint64_t msgrand;           /* hundreths timestamp since last message */
    time_t drchklastime;        /* last time drcheck was called */

    bool redraw;                /* should redraw from scratch in *display()? */

    bool msgok;                 /* allow new messages */

    recordMode_t recmode;       /* recording mode: one of RECMODE_* */

    int lasttang;               /* last target angle */
    unsigned int lasttdist;     /* last target distance */

    unsigned short updsec;      /* updates per second */

    // Info and TAD related data
    std::string lastInfoTarget; /* string of last ship/planet (I)nfo'd */
    std::string lastInfoStr;    /* last string used in doinfo() */

    uint8_t ipaddr[SHIP_IPADDR_LEN]; /* IP address */

} Context_t;


#ifdef NOEXTERN_CONTEXT
Context_t Context = {};
#else
extern Context_t Context;
#endif

#endif /* CONTEXT_H */
