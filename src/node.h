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


#ifndef _NODE_H
#define _NODE_H

#include "cqmouse.h"
#include "anim.h"

/* node proc return values */
typedef enum {
    NODE_OK                 = 0,
    // like OK, but wants to do it's own packet processing
    NODE_OK_NO_PKTPROC      = 1,
    NODE_EXIT               = 2,
    NODE_ERR                = 3,
} nodeStatus_t;

/* menu/screen node */


typedef struct _scr_node {
    nodeStatus_t (*display)(dspConfig_t *); /* display routine */
    nodeStatus_t (*idle)(void);          /* idle routine */
    nodeStatus_t (*input)(int ch);       /* keyboard input routine */
    nodeStatus_t (*minput)(mouseData_t *mdata); /* mouse input routine */
    animVec_t *animVec;                /* optional animation que. */
} scrNode_t;

/* some nodes are called from other, multiple nodes, so we need
   a way for them to know what node to return to when complete */
#define DSP_NODE_CP        1    /* cockpit node */
#define DSP_NODE_MENU      2    /* main menu */
#define DSP_NODE_PLAYBMENU 3    /* playback menu */
#define DSP_NODE_PLAYB     4    /* watching during playback */

void       setNode(scrNode_t *node);
scrNode_t *getTopNode(void);

void       setONode(scrNode_t *node);
scrNode_t *getTopONode(void);

#endif /* _NODE_H */
