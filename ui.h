/* 
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
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

/* planet updating (textures, etc) 
 * for the GL client, this is defined in GL.c, for the curses client
 *  this will be a noop defined in cumisc.c 
 */
int uiUpdatePlanet(int pnum);
/* update the direction of the torp (GL only) */
int uiUpdateTorpDir(int snum, int tnum);

#endif /* _UI_H */
