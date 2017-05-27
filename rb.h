/************************************************************************
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/* A generic ringbuffer implementation */

#ifndef RB_H
#define RB_H



typedef struct _ringBuffer {
  unsigned int  ndata;          /* data in the rb */
  unsigned int  len;            /* length of RB */
  uint8_t        *rp, *wp;       /* r/w ptrs for ring */
  uint8_t        *data;          /* the buffer */
} ringBuffer_t;

void          rbFlush(ringBuffer_t *RB);

ringBuffer_t *rbCreate(unsigned int len);
void          rbDestroy(ringBuffer_t *RB);

unsigned int  rbBytesUsed(ringBuffer_t *RB);
unsigned int  rbBytesFree(ringBuffer_t *RB);

unsigned int  rbPut(ringBuffer_t *RB, uint8_t *buf, unsigned int len);
unsigned int  rbGet(ringBuffer_t *RB, uint8_t *buf, unsigned int len, 
                    int update);

#endif /* RB_H */
