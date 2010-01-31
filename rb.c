#include "c_defs.h"

/************************************************************************
 *
 * $Id$
 *
 * Copyright 1999-2008 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

/* A generic ringbuffer implementation */


#include "conf.h"
#include "global.h"
#include "defs.h"

#include "rb.h"

/* flush (empty) a ringbuffer */
void rbFlush(ringBuffer_t *RB)
{
  if (RB)
    {
      RB->rp = RB->wp = RB->data;
      RB->ndata = 0;
    }

  return;
}


/* create a ringbuffer of a specified length */
ringBuffer_t *rbCreate(unsigned int len)
{
  ringBuffer_t *RB;

  if (!len)
    return NULL;

  if ( !(RB = malloc(sizeof(ringBuffer_t))) )
    return NULL;

  if ( !(RB->data = malloc(len)) )
    {                           /* oops */
      free(RB);
      return NULL;
    }

  RB->len = len;                /* save the length */

  rbFlush(RB);                  /* init to empty */

  return RB;
}

/* destroy a ring buffer */
void rbDestroy(ringBuffer_t *RB)
{
  if (!RB)
    return;

  if (RB->data)
    free(RB->data);

  free(RB);

  return;
}

/* return the number of bytes in use in the rb */
unsigned int rbBytesUsed(ringBuffer_t *RB)
{
  if (RB)
    return RB->ndata;
  else 
    return 0;
}

/* return the number of bytes available in the rb */
unsigned int rbBytesFree(ringBuffer_t *RB)
{
  if (RB)
    return (RB->len - RB->ndata);
  else 
    return 0;
}

/* put data into a ring buffer */
unsigned int rbPut(ringBuffer_t *RB, Unsgn8 *buf, unsigned int len)
{
  unsigned int left, wlen = len, i;
  Unsgn8 *rptr = buf;

  if (!RB || !rptr) 
    return 0;

  left = rbBytesFree(RB); /* max space available */

  if (wlen > left)
    wlen = left;

  for (i=0; i < wlen; i++, rptr++)
    {
      if ( RB->wp >= (RB->data + RB->len) ) 
        {
          RB->wp = RB->data;
        }
      
      *RB->wp = *rptr;
      RB->ndata++;
      RB->wp++;
    }

  /* return amount written */
  return wlen;
}

/* get and/or remove data from a ring buffer */
unsigned int rbGet(ringBuffer_t *RB, Unsgn8 *buf, unsigned int len, int update)
{
  Unsgn8 *wptr = buf, *rptr;
  unsigned int rlen = len, tlen, ndata;

  if (!len)
    return 0;

  rptr  = RB->rp;
  ndata = RB->ndata;

  if (rlen > ndata)
    rlen = ndata;

  tlen = rlen;

  while (rlen--)
    {
      if (rptr >= (RB->data + RB->len)) 
        rptr = RB->data;
      
      /* a NULL write ptr doesn't copy data */
      if (wptr)
        {
          *wptr = *rptr;
          wptr++;
        }

      rptr++;
      ndata--;
    }

  /* if we wanted to really remove the returned data from the rb, do it. */
  if (update)
    {
      RB->rp    = rptr;
      RB->ndata = ndata;
    }

  return(tlen);
}

