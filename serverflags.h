/*
 * Various server flags governing features.  This header describes
 * various flags that enable certain behaviors, like allowing users to
 * switch teams to use the slingshot bug.
 *
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef SERVERFLAGS_H_INCLUDED
#define SERVERFLAGS_H_INCLUDED


#include "conqdef.h"

// these are meant to be stored in an uint32_t.

#define SERVER_F_NONE        0x00000000
#define SERVER_F_REFIT       0x00000001 // allow users to refit their ships
#define SERVER_F_VACANT      0x00000002 // allow user to go vacant
#define SERVER_F_SLINGSHOT   0x00000004 // allow use of slingshot bug
#define SERVER_F_NODOOMSDAY  0x00000008 // disable the doomsday machine
#define SERVER_F_KILLBOTS    0x00000010 // robots created with randomized kills
#define SERVER_F_SWITCHTEAM  0x00000020 // allow users to switch teams
#define SERVER_F_NOTEAMWAR   0x00000040 // disallow user from setting
                                        // war with own team
#define SERVER_F_NODRIFT     0x00000080 // disallow use of the drift bug

#endif /* SERVERFLAGS_H_INCLUDED */
