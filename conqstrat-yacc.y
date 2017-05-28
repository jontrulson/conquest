%{

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

#include "conqdef.h"
#include "conqcom.h"

#include "conqutil.h"
#include "conqai.h"

#define NOEXTERN_CONTEXT
#include "context.h"

#define NOEXTERN_GLOBALS
#include "global.h"

#define NOEXTERN_CONF
#include "conf.h"

#include "conqstrat.h"

int rulenum, trstrat[MAX_VAR][10], trvec[32];
int debug_scanner, verbose;

static int UpdateCmnBlock = FALSE;

extern int Lineno;
extern int goterror;
extern void yyerror(char *s);
int yylex(void);
%}


%token TOK_VARIABLE
%token TOK_OPERATOR
%token TOK_NUMBER
%token TOK_TERMINATOR
%token TOK_ACTION
%token TOK_AND
%token TOK_NOT

%%
rulelist	:	rulelist rule
		|	rule
		;

rule		:	exprlist TOK_TERMINATOR TOK_ACTION
			{
			  dumprule( $3 );
			}
		|	TOK_TERMINATOR TOK_ACTION
			{
			  dumprule( $2 );
			}
		|	error
		;

exprlist	:	expr
		|	exprlist TOK_AND expr
		;

expr		:	TOK_VARIABLE TOK_OPERATOR TOK_NUMBER
			{
			  addrule( $1, $2, $3 );
			}
		|	TOK_NUMBER TOK_OPERATOR TOK_VARIABLE
			{
			  addrule( $3, invertop( $2 ), $1 );
			}
		|	TOK_VARIABLE
			{
			  addrule( $1, OP_NE, 0 );
			}
		|	TOK_NOT TOK_VARIABLE
			{
			  addrule( $2, OP_EQ, 0 );
			}
		;

%%


/* this program uses a yacc parser that would conflict with the
   conqinit parser, so we do not link it here.  However, conqlb references
   a function it provides (but is not needed in the program, so we make a
   fake version here. */

void cqiInitPlanets(void)
{
  return;
}


/* conqstrat - main program */
int main(int argc, char **argv)
{
  extern char *optarg;
  extern int optind, opterr, optopt;
  char filename[PATH_MAX] = {};
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
          strncpy(filename, optarg, PATH_MAX - 1);
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
    char *strat="trstrat";
    char *vec="trvec";

    utFormatTime( buf, 0 );

    fprintf(filefd, "/*\n * conqdata.h - robot strategy data generated by conqstrat on %s\n */\n", buf );

    fprintf(filefd, "#ifndef CONQDATA_H\n#define CONQDATA_H\n\n#ifdef NOEXTERN_CONQDATA\n# define CDHEXTERN\n#else\n# define CDHEXTERN extern\n#endif\n\n");

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
