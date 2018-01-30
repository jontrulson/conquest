//
// Author: Jon Trulson <jon@radscan.com>
// Copyright (c) 1994-2018 Jon Trulson
//
// The MIT License
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//


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
time_t   utDeltaSecs( time_t s, time_t *n );
real     utExplosionHits( real basehits, real dis );
void     utFormatMinutes( int itime, char *buf );
void     utFormatSeconds( int itime, char *buf );
int      utGetMsg( int snum, int *msg );
void     utFormatTime( char *buf, time_t thetime );
uint64_t utDeltaGrand(uint64_t s, uint64_t *n);
void     utGrand( uint64_t *h );
void     utGetSecs( time_t *s );
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
