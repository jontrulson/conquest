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

#ifndef HISTORY_H_INCLUDED
#define HISTORY_H_INCLUDED

#include "conqdef.h"

typedef struct {
    int    unum;              // user number, if -1, then this marks a
                              // resigned user.
    time_t enterTime;         /* date of entry */
    time_t elapsed;           /* elapsed time of this entry */
    char   username[MAX_USERNAME]; /* username */
} History_t;

// instantiate in history.cxx only
#if defined(NOEXTERN_HISTORY)
// The cbHistory[] slot currently being used (if any) for the current "user"
int historyCurrentSlot = -1;
#else
extern int historyCurrentSlot;
#endif

#endif /* HISTORY_H_INCLUDED */
