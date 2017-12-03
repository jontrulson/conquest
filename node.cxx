/* nodes....
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

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
