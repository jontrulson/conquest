/************************************************************************
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 ***********************************************************************/

#ifndef _NODE_H
#define _NODE_H

/* node proc return values */
#define NODE_OK        TRUE
#define NODE_EXIT      FALSE
#define NODE_ERR       ERR
/* menu/screen node */


typedef struct _scr_node {
  int (*display)(dspConfig_t *); /* display routine */
  int (*idle)(void);            /* idle routine */
  int (*input)(int ch);         /* input routine */
  struct _scr_node *next;       /* sub-nodes */
} scrNode_t;

/* some nodes are called from other, multiple nodes, so we need
   a way for them to know what node to return to when complete */
#define DSP_NODE_CP        1    /* cockpit node */
#define DSP_NODE_MENU      2    /* main menu */
#define DSP_NODE_PLAYBMENU 3    /* playback menu */
#define DSP_NODE_PLAYB     4    /* watching during playback */

void setNode(scrNode_t *node);
scrNode_t *getTopNode(void);
void setONode(scrNode_t *node);
scrNode_t *getTopONode(void);

#endif /* _NODE_H */
