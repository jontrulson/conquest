/* 
 * some common data typedefs
 *
 * $Id$
 *
 * Copyright 2003 Jon Trulson under the ARTISTIC LICENSE. (See LICENSE).
 */

#ifndef DATA_H_INCLUDED
#define DATA_H_INCLUDED

/* these will need to be adjusted per arch of course... */

/* This stuff is partularly important for the protoco/packets and the
 *  code that deals with them 
 */


# if defined(HAVE_STDINT_H)
typedef int32_t Sgn32;
typedef uint32_t Unsgn32;

typedef int16_t Sgn16;
typedef uint16_t Unsgn16;

typedef int8_t Sgn8;
typedef uint8_t Unsgn8;

# else
typedef int Sgn32;
typedef unsigned int Unsgn32;

typedef short Sgn16;
typedef unsigned short Unsgn16;

typedef char Sgn8;
typedef unsigned char Unsgn8;

# endif

#endif /* DATA_H_INCLUDED */
