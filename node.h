/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

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
    animQue_t *animQue;                /* optional animation que. */
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
