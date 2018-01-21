/*
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
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
void     utAppendKilledBy(char *buf, killedBy_t kb, uint16_t detail);
int      utArrowsToDir( char *str, real *dir );
void     utSetLogConfig(int usesys, int echostderr);

// we want special format checks on this one
void     utLog(const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));

void     utDeleteBlanks( char *str );
unsigned int utDeltaSecs( unsigned int s, unsigned int *n );
real     utExplosionHits( real basehits, real dis );
void     utFormatMinutes( int itime, char *buf );
void     utFormatSeconds( int itime, char *buf );
int      utGetMsg( int snum, int *msg );
void     utFormatTime( char *buf, time_t thetime );
unsigned int utDeltaGrand(unsigned int s, unsigned int *n);
void     utGrand( unsigned int *h );
void     utGetSecs( unsigned int *s );
real     utMod360( real r );
int      utModPlusOne( int i, int modulus );
int      utIsSpecial( char *str, int *what, int *token, int *count );
void     utStrncpy( char *to, const char *from, unsigned int tosize );
void     utStrncat( char *to, const char *from, unsigned int tosize );
int      utStringMatch( const char *str1, const char *str2, int casesensitive );
char     *utGetPath(const char *thepath);
void     utSleep(real seconds);
bool     utIsDigits(const char *buf);
void     utToUpperCase(char *buf);
void     utAppendChar(char *buf, char ch);


#endif /* _CONQUTIL_H */
