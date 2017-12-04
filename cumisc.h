/*
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _CUMISC_H
#define _CUMISC_H

void mcuHistList( int godlike );
void mcuPutThing( int what, int lin, int col );
int  mcuReadMsg( int msgnum, int dsplin );
int  mcuKPAngle(int ch, real *angle);
int  mcuKP2DirKey(int *ch);
void mcuInfoPlanet( const char *str, int pnum, int snum );
void mcuInfoShip( int snum, int scanner );
void mcuPlanetList( int team, int snum );
void mcuPlayList( int godlike, int doall, int snum );
int  mcuReviewMsgs( int snum, int slm );
void mcuTeamList( int team );
void mcuUserList( int godlike, int snum );
void mcuUserStats( int godlike , int snum );
int  mcuConfirm(void);
int  mcuAskYN(const char *question, int lin, int col);
char mcuGetCX( const char *pmt, int lin, int offset, const char *terms,
               char *buf, int len );
int  mcuGetTarget( const char *pmt, int lin, int col, real *dir,
                   real cdefault );
int  mcuMore( const char *pmt );
void mcuPageFile( const char *file, const char *errmsg );
void mcuPutMsg( const char *msg, int line );
void mcuPutPrompt( const char *pmt, int line );
void mcuHelpLesson(void);
void mcuNews(void);

#endif /* _CUMISC_H */
