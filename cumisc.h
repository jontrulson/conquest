/* 
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _CUMISC_H
#define _CUMISC_H

void mcuHistList( int godlike );
void mcuPutThing( int what, int lin, int col );
int  mcuReadMsg( int snum, int msgnum, int dsplin );
int  mcuKPAngle(int ch, real *angle);
int  mcuKP2DirKey(int *ch);
int  mcuConqLogo(void);
void mcuInfoPlanet( char *str, int pnum, int snum );
void mcuInfoShip( int snum, int scanner );
void mcuPlanetList( int team, int snum );
void mcuPlayList( int godlike, int doall, int snum );
int  mcuReviewMsgs( int snum, int slm );
void mcuTeamList( int team );
void mcuUserList( int godlike, int snum );
void mcuUserStats( int godlike , int snum );
int  mcuConfirm(void);
int  mcuAskYN(char *question, int lin, int col);
char mcuGetCX( char *pmt, int lin, int offset, char *terms, 
               char *buf, int len );
int  mcuGetTarget( char *pmt, int lin, int col, real *dir, real cdefault );
int  mcuMore( char *pmt );
void mcuPageFile( char *file, char *errmsg );
void mcuPutMsg( char *msg, int line );
void mcuPutPrompt( char *pmt, int line );
void mcuHelpLesson(void);
void mcuNews(void);

#endif /* _CUMISC_H */
