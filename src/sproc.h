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

#ifndef SPROC_H_INCLUDED
#define SPROC_H_INCLUDED

#include "conqdef.h"
#include "conqnet.h"

#include "protocol.h"
#include "packet.h"

void procSetName(char *buf);
void procSetCourse(char *buf);

void procSetWarp(cpCommand_t *swarp);
void procSetShields(cpCommand_t *cmd);
void procAlloc(cpCommand_t *cmd);
void procCloak(cpCommand_t *cmd);
void procDetSelf(cpCommand_t *cmd);
void procDetEnemy(cpCommand_t *cmd);
void procDistress(cpCommand_t *cmd);
void procFirePhaser(cpCommand_t *cmd);
void procOrbit(cpCommand_t *cmd);
void procRepair(cpCommand_t *cmd);
void procSetWar(cpCommand_t *cmd);
void procRefit(cpCommand_t *cmd);
void procSetRate(cpCommand_t *cmd);
void procTow(cpCommand_t *cmd);
void procUnTow(cpCommand_t *cmd);
void procCoup(cpCommand_t *cmd);
void procBomb(cpCommand_t *cmd);
void procBeam(cpCommand_t *cmd);
void procDestruct(cpCommand_t *cmd);
void procAutoPilot(cpCommand_t *cmd);

void procFireTorps(char *buf);
void procMessage(char *buf);
void procChangePassword(char *buf);
void procReload(cpCommand_t *cmd);
void procAckUDP(char *buf);


#endif /* SPROC_H_INCLUDED */
