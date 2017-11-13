/*
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _CONQUTIL_H
#define _CONQUTIL_H

real     utAccurateDist( real curvel, real newvel, real acc );
int      utSafeCToI(int *num, char *buf, int offset);
real     utSubAngle( real a1, real a2);
real     utAngle( real fromx, real fromy, real tox, real toy );
void     utAppendInt(char *str, int i);
void     utAppendShipStatus(char *buf, int status);
void     utAppendTitle(char *buf, int team);
void     utAppendShip(char *str, int snum);
void     utAppendKilledBy(char *buf, killedBy_t kb, unsigned int detail);
int      utArrowsToDir( char *str, real *dir );
void     utSetLogConfig(int usesys, int echostderr);
void     utLog(char *fmt, ...);
void     utDeleteBlanks( char *str );
int      utDeltaSecs( int s, int *n );
real     utExplosionHits( real basehits, real dis );
void     utFormatMinutes( int itime, char *buf );
void     utFormatSeconds( int itime, char *buf );
int      utGetMsg( int snum, int *msg );
void     utFormatTime( char *buf, time_t thetime );
int      utDeltaGrand(int s, int *n);
void     utGrand( int *h );
void     utGetSecs( int *s );
real     utMod360( real r );
int      utModPlusOne( int i, int modulus );
int      utIsSpecial( char *str, int *what, int *token, int *count );
void     utStcpn( char *from, char *to, int tosize );
int      utStringMatch( char *str1, char *str2, int casesensitive );
char     *utGetPath(const char *thepath);
void     utSleep(real seconds);
bool     utIsDigits(const char *buf);
void     utToUpperCase(char *buf);
void     utAppendChar(char *buf, char ch);

#endif /* _CONQUTIL_H */
