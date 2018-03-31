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


#ifndef _PRM_H
#define _PRM_H

/* return values from prmProcInput() */
/* retval > 0 is the terminator char entered */

#define PRM_OK            0     /* everything ok */
#define PRM_MAXLEN        -1    /* maxlen exceeded */


typedef struct _prompt {
    bool  preinit;                /* pre-initing the prompt resp? */
    char *buf;                    /* response */
    size_t   buflen;
    const char *pbuf;                   /* the prompt */
    int   index;                  /* only used in CP for now */
} prm_t;

int prmProcInput(prm_t *prm, int ch);


#endif /* _PRM_H */
