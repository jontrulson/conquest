/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2008 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/* A generic ringbuffer implementation */

#ifndef RB_H
#define RB_H

#include "datatypes.h"

typedef struct _ringBuffer {
  unsigned int  ndata; /* data in the rb */
  unsigned int  len;   /* length of RB */
  Unsgn8        *rp, *wp;       /* r/w ptrs for ring */
  Unsgn8        *data;          /* the buffer */
} ringBuffer_t;

ringBuffer_t *rbCreate(unsigned int len);
void          rbDestroy(ringBuffer_t *RB);
int           rbPut(ringBuffer_t *RB, Unsgn8 *buf, unsigned int len);
unsigned int  rbGet(ringBuffer_t *RB, Unsgn8 *buf, unsigned int len, 
                    int update);

#endif /* RB_H */
