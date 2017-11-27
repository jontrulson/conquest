/*
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _IOLB_H
#define _IOLB_H

#if defined(NOEXTERN_IOLB)
/* a copy of stdin's fd (0) for poll()/select() (curses) */
const int iolbStdinFD = 0;
#else
extern int iolbStdinFD;	/* a copy of stdin's fd for poll()/select() */
#endif /* NOEXTERN_IOLB */

int  iochav( void );
int  iogchar ( void );
int  iogtimed ( int *ch, real seconds );
void ioeat(void);

#endif /* _IOLB_H */
