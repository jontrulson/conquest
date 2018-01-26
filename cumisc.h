/*
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _CUMISC_H
#define _CUMISC_H

void mcuHistList();
void mcuPutThing( int what, int lin, int col );
int  mcuReadMsg( int msgnum, int dsplin );
void mcuInfoPlanet( const char *str, int pnum );
void mcuInfoShip( int snum );
void mcuPlanetList();
void mcuPlayList( bool doall );
int  mcuReviewMsgs( int slm );
void mcuTeamList();
void mcuUserList();
void mcuUserStats();
bool  mcuConfirm(void);
int  mcuAskYN(const char *question, int lin, int col);
char mcuGetCX( const char *pmt, int lin, int offset, const char *terms,
               char *buf, int len );
int  mcuMore( const char *pmt );
void mcuPutMsg( const char *msg, int line );
void mcuPutPrompt( const char *pmt, int line );

#endif /* _CUMISC_H */
