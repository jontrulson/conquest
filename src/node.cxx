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
#include "gldisplay.h"
#include "node.h"

static scrNode_t *topNodePtr = NULL; /* the currently running top node */
static scrNode_t *topONodePtr = NULL; /* the currently running top overlay
                                         node */
static scrNode_t *currentNodePtr = NULL; /* the currently running node */

/* can only set the top node */
void setNode(scrNode_t *node)
{
    topNodePtr = node;

    return;
}

/* can only set the top Onode */
void setONode(scrNode_t *node)
{
    topONodePtr = node;

    return;
}

scrNode_t *getTopNode(void)
{
    return topNodePtr;
}

scrNode_t *getTopONode(void)
{
    return topONodePtr;
}

scrNode_t *getCurrentNode(void)
{
    return currentNodePtr;
}
