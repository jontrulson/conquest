/* 
 * Team structure
 *
 * $Id$
 *
 * Copyright 1999-2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _UI_H
#define _UI_H

/* ui specific */
void uiInitColors(void);
void uiPutColor(cqColor col);

/* ui generic */
void dspReplayMenu(void);
void dspReplayHelp(void);

int uiCStrlen(char *buf);
#endif /* _UI_H */
