/************************************************************************
 *
 * $Id$
 *
 ***********************************************************************/

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/*                                                                    */
/**********************************************************************/

#ifndef IBUF_H
#define IBUF_H

/* Input Buffer (ibuf.c) variables */
#define    IBUFMAX 128
static int iBufOffset;
static char iBuffer[IBUFMAX];

#endif
