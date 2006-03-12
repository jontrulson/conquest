/* A Bison parser, made from /home/jon/src/conquest/ng/roy-new1/conquest/conqstrat-yacc.y, by GNU bison 1.50.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

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

#ifndef BISON_CONQSTRAT_YACC_H
# define BISON_CONQSTRAT_YACC_H

/* Tokens.  */
#ifndef YYTOKENTYPE
# if defined (__STDC__) || defined (__cplusplus)
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     TOK_VARIABLE = 258,
     TOK_OPERATOR = 259,
     TOK_NUMBER = 260,
     TOK_TERMINATOR = 261,
     TOK_ACTION = 262,
     TOK_AND = 263,
     TOK_NOT = 264
   };
# endif
  /* POSIX requires `int' for tokens in interfaces.  */
# define YYTOKENTYPE int
#endif /* !YYTOKENTYPE */
#define TOK_VARIABLE 258
#define TOK_OPERATOR 259
#define TOK_NUMBER 260
#define TOK_TERMINATOR 261
#define TOK_ACTION 262
#define TOK_AND 263
#define TOK_NOT 264




#ifndef YYSTYPE
typedef int yystype;
# define YYSTYPE yystype
#endif

extern YYSTYPE yylval;


#endif /* not BISON_CONQSTRAT_YACC_H */

