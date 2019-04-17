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


#ifndef CONQINFO_H_INCLUDED
#define CONQINFO_H_INCLUDED

#include "conqdef.h"

typedef struct {
    int  lockword;                /* exclusionary flag */
    int  lockmesg;                /* exclusive access to the message area */
    unsigned int  ccpuseconds;    /* cpu seconds used by conquest */
    unsigned int  celapsedseconds; /* elapsed seconds for conquest */
    unsigned int  dcpuseconds;    /* cpu seconds used by conqdriv */
    unsigned int  delapsedseconds; /* elapsed seconds for conqdriv */
    unsigned int  rcpuseconds;     /* cpu seconds used by robots */
    unsigned int  relapsedseconds; /* elapsed seconds for robots */
    unsigned int  raccum;          /* accumulated cpu hundredths */
    int  histptr;                  /* points to most recent usage */
    int  lastmsg;                  /* the last message sent */
    bool  externrobots;            /* used to disable internal robot code */
    int  glastmsg;                 /* the last message read by God */
    char lastupchuck[MAX_DATESIZE]; /* last time the common block was updated */
    char ptname[MAXPLANETTYPES][MAXPTYPENAME]; /* planet type names */
    char chrplanets[MAXPLANETTYPES]; /* class M = M, dead = D, sun = S */
    char conqueror[MAX_USERNAME];   /* user who last conquered */
    char conqteam[MAXTEAMNAME];    /* team s/he was on */
    char conqtime[MAX_DATESIZE];    /* time of last conquering */
    char lastwords[MAX_LASTWORDS];  /* remarks upon conquering */
    char inittime[MAX_DATESIZE];    /* time the universe was initialized */
    int  conqservPID;              /* the conqserv pid */
} cbConqInfo_t;

#endif /* CONQINFO_H_INCLUDED */
