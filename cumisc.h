/* 
 * $Id$
 *
 * Copyright 2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _CUMISC_H
#define _CUMISC_H

void cumHistList( int godlike );
void cumPutThing( int what, int lin, int col );
int cumReadMsg( int snum, int msgnum, int dsplin );
int cumKPAngle(int ch, real *angle);
int cumKP2DirKey(int *ch);
int cumConqLogo(void);
void cumInfoPlanet( char *str, int pnum, int snum );
void cumInfoShip( int snum, int scanner );
void cumPlanetList( int team, int snum );
void cumPlayList( int godlike, int doall, int snum );
int cumReviewMsgs( int snum, int slm );
void cumTeamList( int team );
void cumUserList( int godlike, int snum );
void cumUserStats( int godlike , int snum );
int cumConfirm(void);
int cumAskYN(char *question, int lin, int col);
char cumGetCX( char *pmt, int lin, int offset, char *terms, char *buf, int len );
int cumGetTarget( char *pmt, int lin, int col, real *dir, real cdefault );
int cumMore( char *pmt );
void cumPageFile( char *file, char *errmsg );
void cumPutMsg( char *msg, int line );
void cumPutPrompt( char *pmt, int line );
void cumHelpLesson(void);
void cumNews(void);

#endif /* _CUMISC_H */
