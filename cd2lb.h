/*
 * Copyright Jon Trulson under the MIT License. (See LICENSE).
 */

#ifndef _CD2LB_H
#define _CD2LB_H


void cdbeep(void);
void cdbox ( int lin1, int col1, int lin2, int col2 );
void cdcput ( char ch );
void cdclear(void);
void cdclra ( int l1, int c1, int l2, int c2 );
void cdclrl ( int f, int n );
int  cdcols (void);
void cdend(void);
void cdfill ( char ch, char buf[], int count );
int  cdgetn ( const char pmt[], int lin, int col, int *num );
int  cdgets ( const char pmt[], int lin, int col, char str[], int maxlen );
int  cdgetx ( const char pmt[], int lin, int col, const char terms[],
              char str[],
              int maxlen, int doecho );
int  cdgetp ( const char pmt[], int lin, int col, const char terms[],
              char str[],
              int maxlen, int *append_flg, int do_append_flg, int doecho );
void cdinit(void);
void cdline ( int lin1, int col1, int lin2, int col2 );
int  cdlins( void );
void cdmove ( int lin, int col );
void cdmovtc ( char ibuf[], char obuf[], char trntbl[], int count );
void cdrefresh ( void );
void cdput ( char ch, int lin, int col );
void cdputc ( const char str[], int lin );
void cdputn ( int iint, int wid, int lin, int col );
void cdputr ( real x, int wid, int lin, int col );
void cdputs ( const char str[], int lin, int col );
void cdredo(void);

#endif /* _CD2LB_H */
