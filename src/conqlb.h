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

#ifndef _CONQLB_H
#define _CONQLB_H

#include "conqdef.h"

#include "conf.h"
#include "cb.h"

#include "protocol.h"

#include <string>
#include <vector>
#include <algorithm>

int      clbRegister( const char *lname, const char *rname,
                      int team, int *unum );
void     clbChalkup( int snum );
void     clbDamage( int snum, real dam, killedBy_t kb, uint16_t detail );
void     clbDetonate( int snum, int tnum );
int      clbEnemyDet( int snum );
void     clbHit( int snum, real ht, killedBy_t kb, uint16_t detail );
void     clbIKill(int snum, killedBy_t kb, uint16_t detail);
char    *clbETAStr(real warp, real distance);
void     clbKillShip(int snum, killedBy_t kb, uint16_t detail);
int      clbCheckLaunch( int snum, int number );
int      clbLaunch( int snum, real dir, int number, int ltype );
void     clbOrbit( int snum, int pnum );
int      clbPhaser( int snum, real dir );
real     clbPhaserHit( int snum, real dis );
void     clbResign( int unum, int isoper );
int      clbTakePlanet( int pnum, int snum );
void     clbUserline( int unum, int snum, char *buf,
                      int showgods, int showteam );
void     clbStatline( int unum, char *buf );
int      clbZeroPlanet( int pnum, int snum );
const char *clbWarPrompt(int snum, int twar[]);

int      clbCanRead( int snum, int msgnum );
void     clbClearShips(void);
int      clbCVTCoords( real cenx, real ceny, real x, real y, real scale,
                       int *lin, int *col );
void     clbDoomFind(void);
void     clbDoomsday(void);
int      clbFindOrbit( int snum, int *pnum );
int      clbFindShip( int *snum );
int      clbFindSpecial( int snum, int token, int count,
                         int *sorpnum, int *xsorpnum );
void     clbFixDeltas( int snum );
int      clbGetUserNum( int *unum, const char *lname, userTypes_t ltype );
void     clbInitEverything(bool cbIsLocal);
void     clbInitGame(bool cbIsLocal);
void     clbInitMsgs(void);
void     clbInitRobots(void);
void     clbInitShip( int snum, int unum );
void     clbInitUniverse(bool cbIsLocal);
void     clbIntrude( int snum, int pnum );
int      clbLogHist( int unum );
real     clbNewWarp( int snum, real dwarp );
int      clbPlanetMatch( const char *str, int *pnum, int godlike );
void     clbPutShip( int snum, real basex, real basey );
int      clbFmtMsg(msgFrom_t from, uint16_t fromDetail, msgTo_t to,
                   uint16_t toDetail, char *buf);
void     clbSortPlanets( int sv[] );
void     clbSortUsers( int uv[], int numentries );
int      clbSPWar( int snum, int pnum );
int      clbStillAlive( int snum );
void     clbStoreMsg( msgFrom_t from, uint16_t fromDetail, msgTo_t to,
                      uint16_t toDetail, const char *msg );
void     clbStoreMsgf( msgFrom_t from, uint16_t fromDetail, msgTo_t to,
                       uint16_t toDetail, const char *msg,
                       unsigned char flags );
int      clbUseFuel( int snum, real fuel, int weapon, int forreal );
void     clbZeroEverything(void);
void     clbZeroShip( int snum );
const char *clbGetUserLogname(void);
void     clbPlanetDrive(real itersec);
void     clbTorpDrive(real itersec);
void     clbAdjOrbitalPosition(int snum);
uint32_t  clbGetMillis(void);
void     clbBlockAlarm(void);
void     clbUnblockAlarm(void);
void     clbCheckShips(int isDriver);
bool     clbFindTeamHomeSun(int team, int *pnum);
real     clbFixCoord(real coord);
const std::vector<int>& clbGetEnabledTeams();


#endif /* _CONQLB_H */
