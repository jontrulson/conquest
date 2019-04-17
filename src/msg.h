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


#ifndef MSG_H_INCLUDED
#define MSG_H_INCLUDED

#include "conqdef.h"

// maximum size of message text, 4-byte (32b) multiple
#define MESSAGE_SIZE 72

// maximum size for formatting messages 
#define MAX_MSGLINE_LENGTH 80

// message flags
#define MSG_FLAGS_NONE       0x00 /* normal message */
#define MSG_FLAGS_FEEDBACK   0x01 /* server feedback - no formatting */
#define MSG_FLAGS_TERSABLE   0x02 /* can be ignored if UserConf.Terse is
				     set */
#define MSG_FLAGS_ROBOT      0x04 /* an insult from a robot */
#define MSG_FLAGS_INTRUDER   0x08 /* intruder alert */

// msgFrom_t and msgTo_t values must be limited to a max value of 255 since
// it is sent as a uint8_t in the protocol.

// msg from values
typedef enum {
    MSG_FROM_NOONE           = 0, // must be first entry

    MSG_FROM_SHIP            = 1, // ships
    MSG_FROM_PLANET          = 2, // intruder alerts
    MSG_FROM_GOD             = 3,
    MSG_FROM_COMP            = 4, // computer
    MSG_FROM_DOOM            = 5, // doomsday machine

    MSG_FROM_MAX                  // must be last entry
} msgFrom_t;

// msg to values
typedef enum {
    MSG_TO_NOONE             = 0, // must be first entry

    MSG_TO_SHIP              = 1,
    MSG_TO_GOD               = 2,
    MSG_TO_IMPLEMENTORS      = 3,
    MSG_TO_FRIENDLY          = 4,
    MSG_TO_ALL               = 5,
    MSG_TO_TEAM              = 6,

    MSG_TO_MAX                    // must be last entry
} msgTo_t;


// common block representation of a message
typedef struct {
    msgFrom_t     from;
    msgTo_t       to;

    uint16_t      fromDetail; // ship/planet numbers
    uint16_t      toDetail;

    uint8_t       flags;		/* some flags. */
    char          msgbuf[MESSAGE_SIZE]; /* the message itself */
} Msg_t;

#endif /* MSG_H_INCLUDED */
