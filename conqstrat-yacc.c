
/*  A Bison parser, made from conqstrat-yacc.y
    by GNU Bison version 1.28  */

#define YYBISON 1  /* Identify Bison output.  */

#define	TOK_VARIABLE	257
#define	TOK_OPERATOR	258
#define	TOK_NUMBER	259
#define	TOK_TERMINATOR	260
#define	TOK_ACTION	261
#define	TOK_AND	262
#define	TOK_NOT	263

#line 1 "conqstrat-yacc.y"


/*
 * conqstrat.c.y - yacc parser for conqstrat
 *
 * $Id$
 *
 */

/*############################################################################# */
/*                              C O N Q S T R A T */
/*            Copyright (C)1985-1986 by Jef Poskanzer and Craig Leres */
/*    Permission to use, copy, modify, and distribute this software and */
/*    its documentation for any purpose and without fee is hereby granted, */
/*    provided that this copyright notice appear in all copies and in all */
/*    supporting documentation. Jef Poskanzer and Craig Leres make no */
/*    representations about the suitability of this software for any */
/*    purpose. It is provided "as is" without express or implied warranty. */
/*############################################################################ */

/**********************************************************************/
/* Unix/C specific porting and supporting code Copyright (C)1994-1996 */
/* by Jon Trulson <jon@radscan.com> under the same terms and          */
/* conditions of the original copyright by Jef Poskanzer and Craig    */
/* Leres.                                                             */
/**********************************************************************/

#include "c_defs.h"

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"

#include "global.h"
#include "conqstrat.h"

int rulenum, trstrat[MAX_VAR][10], trvec[32];
int debug_scanner, verbose;

static int UpdateCmnBlock = FALSE;

extern int Lineno;
extern int goterror;
extern void yyerror(char *s);
#ifndef YYSTYPE
#define YYSTYPE int
#endif
#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		22
#define	YYFLAG		-32768
#define	YYNTBASE	10

#define YYTRANSLATE(x) ((unsigned)(x) <= 263 ? yytranslate[x] : 14)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     5,     9,    12,    14,    16,    20,    24,    28,
    30
};

static const short yyrhs[] = {    10,
    11,     0,    11,     0,    12,     6,     7,     0,     6,     7,
     0,     1,     0,    13,     0,    12,     8,    13,     0,     3,
     4,     5,     0,     5,     4,     3,     0,     3,     0,     9,
     3,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    58,    59,    62,    66,    70,    73,    74,    77,    81,    85,
    89
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","TOK_VARIABLE",
"TOK_OPERATOR","TOK_NUMBER","TOK_TERMINATOR","TOK_ACTION","TOK_AND","TOK_NOT",
"rulelist","rule","exprlist","expr", NULL
};
#endif

static const short yyr1[] = {     0,
    10,    10,    11,    11,    11,    12,    12,    13,    13,    13,
    13
};

static const short yyr2[] = {     0,
     2,     1,     3,     2,     1,     1,     3,     3,     3,     1,
     2
};

static const short yydefact[] = {     0,
     5,    10,     0,     0,     0,     0,     2,     0,     6,     0,
     0,     4,    11,     1,     0,     0,     8,     9,     3,     7,
     0,     0
};

static const short yydefgoto[] = {     6,
     7,     8,     9
};

static const short yypact[] = {     7,
-32768,     3,    10,     4,    15,     0,-32768,    -4,-32768,    14,
    17,-32768,-32768,-32768,    16,    12,-32768,-32768,-32768,-32768,
    22,-32768
};

static const short yypgoto[] = {-32768,
    18,-32768,     9
};


#define	YYLAST		25


static const short yytable[] = {    21,
     1,    15,     2,    16,     3,     4,    10,     1,     5,     2,
    12,     3,     4,    11,     2,     5,     3,    13,    17,    18,
     5,    22,    19,    14,    20
};

static const short yycheck[] = {     0,
     1,     6,     3,     8,     5,     6,     4,     1,     9,     3,
     7,     5,     6,     4,     3,     9,     5,     3,     5,     3,
     9,     0,     7,     6,    16
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "//usr/lib/bison.simple"
/* This file comes from bison-1.28.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 217 "//usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 3:
#line 63 "conqstrat-yacc.y"
{
			  dumprule( yyvsp[0] );
			;
    break;}
case 4:
#line 67 "conqstrat-yacc.y"
{
			  dumprule( yyvsp[0] );
			;
    break;}
case 8:
#line 78 "conqstrat-yacc.y"
{
			  addrule( yyvsp[-2], yyvsp[-1], yyvsp[0] );
			;
    break;}
case 9:
#line 82 "conqstrat-yacc.y"
{
			  addrule( yyvsp[0], invertop( yyvsp[-1] ), yyvsp[-2] );
			;
    break;}
case 10:
#line 86 "conqstrat-yacc.y"
{
			  addrule( yyvsp[0], OP_NE, 0 );
			;
    break;}
case 11:
#line 90 "conqstrat-yacc.y"
{
			  addrule( yyvsp[0], OP_EQ, 0 );
			;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 543 "//usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 95 "conqstrat-yacc.y"


/* conqstrat - main program */
int main(int argc, char **argv)
{
  extern char *optarg;
  extern int optind, opterr, optopt;
  char filename[FILENAMESIZE];
  int ch;
  FILE *filefd;

  debug_scanner = FALSE;
  verbose = FALSE;
  filefd = stdout;

  while ( (ch = getopt( argc, argv, "dvo:U" )) != EOF )
    {
      switch(ch)
	{
	case 'v':
	  verbose = TRUE;
	  break;
	case 'd':
	  debug_scanner = TRUE;
	  break;
	case 'o':
	  strcpy(filename, optarg);
	  filefd = NULL;
	  break;
	case 'U':
	  UpdateCmnBlock = TRUE;
	  break;
	default:
	  print_usage();
	  exit(1);
	}
    }

  if (filefd == NULL)
    {
      if ((filefd = fopen(filename, "w")) == NULL)
	{
	  fprintf(stderr, "conqstrat: can't open %s: %s\n",
		  filename,
		  strerror(errno));
	  exit(1);
	}
      else
	{
	  if (verbose)
	    { 
	      fprintf(stderr, "conqstrat: sending output to file %s\n", filename);
	    }
	}
    }
	  
  initrun();

  if ( yyparse() == ERR )
    fprintf(stderr, "conqstrat: yyparse() error.\n" );
  if ( goterror )
    {
      fprintf(stderr,  "conqstrat: got error - aborting.\n" );
      exit(1);
    }
  
  if ( UpdateCmnBlock )
    dumprun();
  else
    displayrun(filefd);
  
  if ( filefd != stdin )
      fclose( filefd );
  
  return(0);

}


/* initrun - initalize for the run */
void initrun(void)
{

    int i, j;

    /* Initialize random stuff. */

    rulenum = 0;
    goterror = FALSE;

    /* Initialize the strategy table to accept everything. */
    for ( i = 0; i < MAX_VAR; i = i + 1 )
	for ( j = 0; j < 10; j = j + 1 )
	    trstrat[i][j] = -1;		/* set all bits */

    /* Initialize the rule to action vector to nothing. */
    for ( i = 0; i < 32; i = i + 1 )
	trvec[i] = ROB_NOOP;

    return;

}


/* dumprun - dump the run into the common block */
void dumprun(void)
{

  int i, j;

  map_common();         /* Map the conquest common block */


  /* This is the only place we include/modify the shared common block. */
  if ( *CBlockRevision != COMMONSTAMP )
    {
      fprintf(stderr,"conqstrat: Common block ident mismatch.\n" );
      exit(1);
    }
  
  /* Copy the strategy table. */
  for ( i = 0; i < MAX_VAR; i = i + 1 )
    for ( j = 0; j < 10; j = j + 1 )
      Robot->rstrat[i][j] = trstrat[i][j];

  /* Copy the action vector. */
  for ( i = 0; i < 32; i = i + 1 )
    Robot->rvec[i] = trvec[i];

  fprintf(stderr, "conqstrat: parsed %d rules.\n", rulenum);
  fprintf(stderr, "conqstrat: common block updated.\n");

  return;

}


/* displayrun - dump the run to filefd */
void displayrun(FILE *filefd)
{
    int i, j;
    char buf[MAXLINE];
    string strat="trstrat";
    string vec="trvec";

    getdandt( buf, 0 );

    fprintf(filefd, "/*\n * conqdata.h - robot strategy data generated by conqstrat on %s\n */\n", buf );

    fprintf(filefd, "#ifndef CONQDATA_H\n#define CONQDATA_H\n\n#ifdef NOCDHEXTERN\n# define CDHEXTERN\n#else\n# define CDHEXTERN extern\n#endif\n\n");

    fprintf(filefd, "CDHEXTERN int %s[%d][10] = {\n", strat, MAX_VAR);

    for ( i = 0; i < MAX_VAR; i = i + 1 )
      {
	fprintf(filefd,"  { ");
	for ( j = 0; j < 10; j = j + 1 )
	  {
	    if (j != 9)
	      fprintf(filefd,  "%d, ", trstrat[i][j] );
	    else
	      fprintf(filefd,  "%d", trstrat[i][j] );
	    
	    if ( j == 4 )
	      fprintf(filefd,  "\n     " );
	  }
	if (i != MAX_VAR - 1)
	  fprintf(filefd,  "},\n" );	
	else
	  fprintf(filefd, "}\n");
      }

    fprintf(filefd, "};\n\n");

    fprintf(filefd,  "CDHEXTERN int %s[32] = {", vec );
    for ( i = 0; i < 32; i = i + 1 )
      {
	if (i != 31)
	  fprintf(filefd,  "%d, ", trvec[i] );
	else
	  fprintf(filefd,  "%d ", trvec[i] );
	if ( i == 15 )
	  fprintf(filefd,  "\n\t" );
      }
    fprintf(filefd, "};\n\n#undef CDHEXTERN /* cleanup */\n\n#endif\n\n");

    fprintf(stderr, "conqstrat: parsed %d rules.\n", rulenum);
    
    return;

}


/* dumprule - dump the current rule */
void dumprule( int action )
{
  int i, j, tbits;
  char buf[MAXLINE];
  
    /* Store action. */
  trvec[rulenum] = action;

  robstr(action, buf);
  if (debug_scanner | verbose)
    fprintf(stderr, "DEBUG: ### dumprule(): rulenum = %d, action = %s\n", rulenum, buf);

    /* Check for impossible rules. */
  for ( i = 0; i < MAX_VAR; i = i + 1 )
    {
      /* There must be at least one value that this variable accepts. */
      tbits = 0;
      for ( j = 0; j < 10; j = j + 1 )
	tbits |= trstrat[i][j];

      if (debug_scanner)
	{
	  fprintf(stderr, "\t\tDEBUG:dumprule(): tbits = 0x%08X\n", tbits);
	}

	if ( ( tbits & ibset( 0, rulenum ) ) == 0 )
	    {
	      valstr( i, buf );
	      fprintf(stderr, "conqstrat: Rule on line %d can't happen, %s is broken\n",
		    Lineno, buf);
	    }
	}

    /* Update rule number. */
    rulenum = rulenum + 1;

    return;

}


/* addrule - add info to the current rule */
void addrule( int var, int op, int num )
{
  int i, rulebits;
  char svar[32], sop[32];

  if ( debug_scanner || verbose)
    {
      valstr( var, svar );
      valstr( op, sop );
      fprintf( stderr, "DEBUG: addrule(%s, %s, %d)\n", svar, sop, num );

    }

  if ( rulenum >= 32 )
    {
      fprintf(stderr, "ERROR: conqstrat: More than 32 rules; line %d\n", Lineno );
      exit(1);
    }

  if ( num < 0 || num > 9 )
    {
      fprintf(stderr, "ERROR: addrule: impossible number %d, line %d\n", 
	      num,
	      Lineno);
      exit(1);
    }

  if ( var < 0 || num >= MAX_VAR )
    {
      fprintf(stderr, "ERROR: addrule: impossible variable %d, line %d\n", 
	      var,
	      Lineno);
      exit(1);
    }

  rulebits = ~(ibset( 0, rulenum ));

  if (debug_scanner )
    fprintf(stderr, "DEBUG:\taddrule(): rulebits = 0x%0X rulenum = %d\n", 
	    rulebits, 
	    rulenum);

  switch ( op )
    {
    case OP_LT:
      for ( i = num; i <= 9; i = i + 1 )
	trstrat[var][i] &= rulebits;
      break;
    case OP_LE:
      for ( i = num + 1; i <= 9; i = i + 1 )
	trstrat[var][i] &= rulebits;
      break;
    case OP_GT:
      for ( i = 0; i <= num; i = i + 1 )
	trstrat[var][i] &= rulebits;
      break;
    case OP_GE:
      for ( i = 0; i < num; i = i + 1 )
	trstrat[var][i] &= rulebits;
      break;
    case OP_EQ:
      for ( i = 0; i <= 9; i = i + 1 )
	if ( i != num )
	  trstrat[var][i] &= rulebits;
      break;
    case OP_NE:
      trstrat[var][num] &= rulebits;
      break;
    default:
      fprintf(stderr, "ERROR: addrule: impossible op %d\n", op );
      exit(1);
    }
  
  return;

}


/* invertop - invert an operator */
int invertop( int op )
{
  int iop;

  switch ( op )
    {
    case OP_LT:
      iop = OP_GE;
      break;
    case OP_LE:
      iop = OP_GT;
      break;
    case OP_GT:
      iop = OP_LE;
      break;
    case OP_GE:
      iop = OP_LT;
      break;
    default:
      iop = op;
    }

  return ( iop );

}


/* valstr - convert a var/op value to a string */
void valstr( int value, char *buf )
{
  switch ( value )
    {
    case VAR_RANDOM:
      c_strcpy( "random", buf );
      break;
    case VAR_DNE:
      c_strcpy( "dne", buf );
      break;
    case VAR_DAMAGE:
      c_strcpy( "damage", buf );
      break;
    case VAR_INCOMING:
      c_strcpy( "incoming", buf );
      break;
    case VAR_FUEL:
      c_strcpy( "fuel", buf );
      break;
    case VAR_NUMTORPS:
      c_strcpy( "numtorps", buf );
      break;
    case VAR_SHIELDS:
      c_strcpy( "shields", buf );
      break;
    case VAR_ETEMP:
      c_strcpy( "etemp", buf );
      break;
    case VAR_WTEMP:
      c_strcpy( "wtemp", buf );
      break;
    case VAR_PHASERDAM:
      c_strcpy( "phaserdam", buf );
      break;
    case VAR_TORPDAM:
      c_strcpy( "torpdam", buf );
      break;
    case VAR_WARP:
      c_strcpy( "warp", buf );
      break;
    case VAR_SHUP:
      c_strcpy( "shup", buf );
      break;
    case VAR_WALLOC:
      c_strcpy( "walloc", buf );
      break;
    case VAR_ORBITING:
      c_strcpy( "orbiting", buf );
      break;
    case VAR_REPAIRING:
      c_strcpy( "repairing", buf );
      break;
    case VAR_CLOAKED:
      c_strcpy( "cloaked", buf );
      break;
    case VAR_ENEMYCLOAKED:
      c_strcpy( "enemycloaked", buf );
      break;
    case VAR_ENEMYDAMAGE:
      c_strcpy( "enemydamage", buf );
      break;
    case VAR_CANREAD:
      c_strcpy( "canread", buf );
      break;
    case OP_LT:
      c_strcpy( "lt", buf );
      break;
    case OP_LE:
      c_strcpy( "le", buf );
      break;
    case OP_GT:
      c_strcpy( "gt", buf );
      break;
    case OP_GE:
      c_strcpy( "ge", buf );
      break;
    case OP_EQ:
      c_strcpy( "eq", buf );
      break;
    case OP_NE:
      c_strcpy( "ne", buf );
      break;
    default:
      sprintf( buf, "OP_<%d>", value );
    }
  
  return;

}

/* tokstr - convert a token to a string */
void tokstr( int token, char *buf )
{
  switch ( token )
    {
    case TOK_VARIABLE:
      c_strcpy( "TOK_VARIABLE", buf );
      break;
    case TOK_OPERATOR:
      c_strcpy( "TOK_OPERATOR", buf );
      break;
    case TOK_NUMBER:
      c_strcpy( "TOK_NUMBER", buf );
      break;
    case TOK_TERMINATOR:
      c_strcpy( "TOK_TERMINATOR", buf );
      break;
    case TOK_ACTION:
      c_strcpy( "TOK_ACTION", buf );
      break;
    case TOK_AND:
      c_strcpy( "TOK_AND", buf );
      break;
    case TOK_NOT:
      c_strcpy( "TOK_NOT", buf );
      break;
    default:
      sprintf( buf, "TOK_<%d>", token );
    }

  return;

}

/* ibset - set a bit for world peace */
unsigned int ibset(int start, int end)
{
  register unsigned int retbits;

  retbits = (start | (1 << end));

  if (debug_scanner)
    fprintf(stderr, "#### IBSET() start = %d, end = %d, retbits = 0x%08X Dec(%d)\n", 
	    start, end, retbits, (int)retbits);

  return(retbits);

}

/* print_usage - print usage. Duh. */
void print_usage()
{
  fprintf(stderr, "Usage: conqstrat [-vdU] [ -o file] < conqrule\n");
  fprintf(stderr, "\t-v            be verbose about everything.\n");
  fprintf(stderr, "\t-d            turns on *lots* of debugging.\n");
  fprintf(stderr, "\t-U            update the common block directly.\n");
  fprintf(stderr, "\t-o file       send generated header file to <file>\n");
  fprintf(stderr, "\t              (typically conqdata.h). stdout is the default.\n\n");
  fprintf(stderr, "\t  The default is to read from stdin. conqrule is a file\n");
  fprintf(stderr, "\t  containing rules for robot strategy.\n\n");

}
