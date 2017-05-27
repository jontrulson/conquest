/* 
 * server packet proc routines (from client)
 *
 * Copyright Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

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


#endif /* SPROC_H_INCLUDED */
