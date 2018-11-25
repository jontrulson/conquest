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

#include <string>

real     utAccurateDist( real curvel, real newvel, real acc );
bool     utSafeCToI(int *num, const std::string& buf, int offset = 0);
real     utSubAngle( real a1, real a2);
real     utAngle( real fromx, real fromy, real tox, real toy );
void     utAppendInt(char *str, int i);
void     utAppendShipStatus(char *buf, int status);
void     utAppendShipStatus(std::string& buf, int status);
void     utAppendTitle(std::string& buf, int team);

void     utAppendShip(char *str, int snum);
void     utAppendShip(std::string& str, int snum);
const std::string utShipStr(int snum);

void     utAppendKilledBy(char *buf, killedBy_t kb, uint16_t detail);
void     utAppendKilledBy(std::string& buf, killedBy_t kb, uint16_t detail);
int      utArrowsToDir( const char *str, real *dir );
void     utSetLogConfig(int usesys, int echostderr);
void     utSetLogProgramName(const std::string& progName);

// we want special format checks on this one
void     utLog(const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));

void     utDeleteBlanks( char *str );
void     utDeleteBlanks( std::string& str );
time_t   utDeltaSecs( time_t s, time_t *n );
real     utExplosionHits( real basehits, real dis );
void     utFormatMinutes( int itime, char *buf );
void     utFormatMinutes( int itime, std::string& buf );
void     utFormatSeconds( int itime, char *buf );
int      utGetMsg( int snum, int *msg );
void     utFormatTime( char *buf, time_t thetime );
void     utFormatTime( std::string& buf, time_t thetime );
uint64_t utDeltaGrand(uint64_t s, uint64_t *n);
void     utGrand( uint64_t *h );
void     utGetSecs( time_t *s );
real     utMod360( real r );
bool     utIsSpecial( const std::string& str, int *what, int *token,
                      int *count );
void     utStrncpy( char *to, const char *from, unsigned int tosize );
void     utStrncat( char *to, const char *from, unsigned int tosize );
bool     utStringMatch(const std::string& str1, const std::string& str2,
                       bool casesensitive );
char     *utGetPath(const char *thepath);
void     utSleep(real seconds);
bool     utIsDigits(const std::string& str);
void     utToUpperCase(char *buf);
void     utAppendChar(char *buf, char ch);


#endif /* _CONQUTIL_H */
