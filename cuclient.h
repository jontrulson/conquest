/* 
 * $Id$
 *
 * Copyright 2004 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef _CUCLIENT_H
#define _CUCLIENT_H

void cucPseudo( int unum, int snum );
void cucDoWar( int snum );
void cucSendMsg( int from, int terse, int remote );

#endif /* _CUCLIENT_H */
