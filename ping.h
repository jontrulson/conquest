/************************************************************************
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 ***********************************************************************/

/* A generic ringbuffer implementation */

#ifndef PING_H
#define PING_H

static const uint32_t pingTime = 5000; // 5 seconds

void pingEnable(bool enable);
void pingSend(uint32_t iternow);
void pingResponse(uint32_t iternow);

#endif /* PING_H */
