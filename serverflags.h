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

#ifndef SERVERFLAGS_H_INCLUDED
#define SERVERFLAGS_H_INCLUDED


#include "conqdef.h"

// these are meant to be stored in an uint32_t.

#define SERVER_F_NONE        0x00000000
#define SERVER_F_REFIT       0x00000001 // allow users to refit their ships
#define SERVER_F_VACANT      0x00000002 // allow user to go vacant
#define SERVER_F_SLINGSHOT   0x00000004 // allow use of slingshot bug
#define SERVER_F_NODOOMSDAY  0x00000008 // disable the doomsday machine
#define SERVER_F_KILLBOTS    0x00000010 // robots created with randomized kills
#define SERVER_F_SWITCHTEAM  0x00000020 // allow users to switch teams
#define SERVER_F_NOTEAMWAR   0x00000040 // disallow user from setting
                                        // war with own team
#define SERVER_F_NODRIFT     0x00000080 // disallow use of the drift bug

#endif /* SERVERFLAGS_H_INCLUDED */
