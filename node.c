/* nodes....
 *
 * Jon Trulson, 1/2004
 *
 * $Id$
 *
 * Copyright 2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#include "c_defs.h"
#include "gldisplay.h"
#include "node.h"

static scrNode_t *nodePtr = NULL;      /* the currently running node */

void setNode(scrNode_t *node)
{
  nodePtr = node;

  return;
}

scrNode_t *getNode(void)
{
  return nodePtr;
}


