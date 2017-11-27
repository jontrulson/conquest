/*
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _CUCLIENT_H
#define _CUCLIENT_H

void cucPseudo( int unum, int snum );
void cucDoWar( int snum );
void cucSendMsg( msgFrom_t from, uint16_t fromDetail, int terse, int remote );

#endif /* _CUCLIENT_H */
