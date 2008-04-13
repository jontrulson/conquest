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

  RB->rp = RB->wp = RB->data;
  RB->ndata = 0;
  RB->len = len;

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

/* put data into a ring buffer, or query available space */
int rbPut(ringBuffer_t *RB, Unsgn8 *buf, unsigned int len)
{
  unsigned int left, wlen = len, i;
  Unsgn8 *rptr = buf;

  if (!RB) 
    return 0;

  left = RB->len - RB->ndata; /* max space available */

  /* a null buf means to simply return the space left */

  if (!rptr)
    return(left);

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

  return(wlen);
}

/* get or remove data from a ring buffer */
unsigned int rbGet(ringBuffer_t *RB, Unsgn8 *buf, unsigned int len, int update)
{
  Unsgn8 *wptr = buf, *rptr;
  unsigned int rlen = len, tlen, ndata;

  rptr  = RB->rp;
  ndata = RB->ndata;

  if (rlen > RB->ndata)
    rlen = RB->ndata;

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

  if (update)
    {
      RB->rp    = rptr;
      RB->ndata = ndata;
    }

  return(tlen);
}

