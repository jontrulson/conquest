%{

/* 
 * conqstrat.c.l - lexical scanner for conqstrat
 *
 * $Id$
 *
 */


/*            Copyright (C)1983-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/* Have Phun!                                                         */
/**********************************************************************/

/* try to short-circuit ECHO redef warnings from compiler.. */
#undef ECHO

#include "c_defs.h"
#include "conqstrat-yacc.h"

#include "conqdef.h"
#include "conqcom.h"
#include "context.h"
#include "conqutil.h"

#include "conqstrat.h"

/* declared in conqstrat.c.y */

extern    int fd, rulenum;

int goterror = 0;
int Lineno = 1; /* current line number of the scanner */


extern int yylval; 

#undef ECHO
#define ECHO (void)fprintf(yyout, "Unrecognized input: '%s', ignored.\n",yytext)

%}

%option noyywrap

whtspc	[ \t]+

%%


random			{yylval = VAR_RANDOM; return( TOK_VARIABLE );}
dne			{yylval = VAR_DNE; return( TOK_VARIABLE );}
damage			{yylval = VAR_DAMAGE; return( TOK_VARIABLE );}
incoming		{yylval = VAR_INCOMING; return( TOK_VARIABLE );}
fuel			{yylval = VAR_FUEL; return( TOK_VARIABLE );}
numtorps		{yylval = VAR_NUMTORPS; return( TOK_VARIABLE );}
shields			{yylval = VAR_SHIELDS; return( TOK_VARIABLE );}
etemp			{yylval = VAR_ETEMP; return( TOK_VARIABLE );}
wtemp			{yylval = VAR_WTEMP; return( TOK_VARIABLE );}
phaserdam		{yylval = VAR_PHASERDAM; return( TOK_VARIABLE );}
torpdam			{yylval = VAR_TORPDAM; return( TOK_VARIABLE );}
warp			{yylval = VAR_WARP; return( TOK_VARIABLE );}
shup			{yylval = VAR_SHUP; return( TOK_VARIABLE );}
walloc			{yylval = VAR_WALLOC; return( TOK_VARIABLE );}
orbiting		{yylval = VAR_ORBITING; return( TOK_VARIABLE );}
repairing		{yylval = VAR_REPAIRING; return( TOK_VARIABLE );}
cloaked			{yylval = VAR_CLOAKED; return( TOK_VARIABLE );}
enemycloaked		{yylval = VAR_ENEMYCLOAKED; return( TOK_VARIABLE );}
enemydamage		{yylval = VAR_ENEMYDAMAGE; return( TOK_VARIABLE );}
canread			{yylval = VAR_CANREAD; return( TOK_VARIABLE );}

ROB_NOOP                {yylval = ROB_NOOP; return( TOK_ACTION );}
ROB_GOHOME		{yylval = ROB_GOHOME; return( TOK_ACTION );}
ROB_GOREPAIR		{yylval = ROB_GOREPAIR; return( TOK_ACTION );}
ROB_ALLOCATE		{yylval = ROB_ALLOCATE; return( TOK_ACTION );}
ROB_DETONATE		{yylval = ROB_DETONATE; return( TOK_ACTION );}
ROB_MYDETONATE		{yylval = ROB_MYDETONATE; return( TOK_ACTION );}
ROB_PHASER		{yylval = ROB_PHASER; return( TOK_ACTION );}
ROB_TORPEDO		{yylval = ROB_TORPEDO; return( TOK_ACTION );}
ROB_BURST		{yylval = ROB_BURST; return( TOK_ACTION );}
ROB_SHIELD		{yylval = ROB_SHIELD; return( TOK_ACTION );}
ROB_UNTRACTOR		{yylval = ROB_UNTRACTOR; return( TOK_ACTION );}
ROB_WARP_0		{yylval = ROB_WARP_0; return( TOK_ACTION );}
ROB_WARP_2		{yylval = ROB_WARP_2; return( TOK_ACTION );}
ROB_WARP_5		{yylval = ROB_WARP_5; return( TOK_ACTION );}
ROB_WARP_8		{yylval = ROB_WARP_8; return( TOK_ACTION );}
ROB_TRACK		{yylval = ROB_TRACK; return( TOK_ACTION );}
ROB_SILENT		{yylval = ROB_SILENT; return( TOK_ACTION );}
ROB_MESSAGE		{yylval = ROB_MESSAGE; return( TOK_ACTION );}
ROB_TAKEDRUGS		{yylval = ROB_TAKEDRUGS; return( TOK_ACTION );}
ROB_REPAIR		{yylval = ROB_REPAIR; return( TOK_ACTION );}
ROB_READMSG		{yylval = ROB_READMSG; return( TOK_ACTION );}
ROB_INSULT		{yylval = ROB_INSULT; return( TOK_ACTION );}
ROB_GOFUEL		{yylval = ROB_GOFUEL; return( TOK_ACTION );}
ROB_RUNAWAY		{yylval = ROB_RUNAWAY; return( TOK_ACTION );}

"<"			{yylval = OP_LT; return( TOK_OPERATOR );}
"<="			{yylval = OP_LE; return( TOK_OPERATOR );}
">"			{yylval = OP_GT; return( TOK_OPERATOR );}
">="			{yylval = OP_GE; return( TOK_OPERATOR );}
"="|"=="		{yylval = OP_EQ; return( TOK_OPERATOR );}
"!="|"<>"		{yylval = OP_NE; return( TOK_OPERATOR );}

"!"			return( TOK_NOT );
"&"			return( TOK_AND );
";"			return( TOK_TERMINATOR );

[0-9]			{
/*                        printf("**=- buf = %s yytext = %s -=**\n", buf, yytext); */
			yylval = atoi( yytext );
			return( TOK_NUMBER );
			}

#+.*			/*# eat comments */


{whtspc}		/* eat gratuitous white space */

\n			{Lineno++; /* eat newlines */}

%%

/* parser error reporting routine */
void yyerror(char *s)
{
   fprintf(stderr, "conqstrat: %s at or near '%s' on line %d.\n", 
           s,
           yytext,
           Lineno);

   goterror = TRUE;

  return;
}


