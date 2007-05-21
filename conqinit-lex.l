%{

/* 
 * conqinit - lexical scanner for conqinit
 *
 * $Id$
 *
 */

#include "c_defs.h"
#include "conqinit.h"
#include "conqinit-yacc.h"

#define NOEXTERN
#include "conqdef.h"
#include "conqcom.h"
#include "context.h"

int goterror = 0;
int lineNum = 1; /* current line number of the scanner */

extern int cqiDebugl;                       /* from conqinit-yacc.y */

/* try to short-circuit ECHO redef warnings from compiler.. */
#undef ECHO
#define ECHO { if (cqiDebugl) (void)fprintf(yyout, "Unrecognized input: '%s', ignored.\n", yytext); }

%}

WS	 [ \t]+
OWS	 [ \t]*
string   \"([^"]|\\.)*\"
number   [0-9]+
rational -?(([0-9]+)|([0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?)
hex      [0-9a-f]+
opensect  \{
closesect \}
%%

name                   {return( NAME );}

global                 {return( GLOBAL );}
planetmax              {return( PLANETMAX );}
shipmax                {return( SHIPMAX );}
usermax                {return( USERMAX );}
msgmax                 {return( MSGMAX );}
histmax                {return( HISTMAX );}

shiptype               {return( SHIPTYPE );} 
engfac                 {return( ENGFAC );} 
weafac                 {return( WEAFAC );} 
accelfac               {return( ACCELFAC );} 
torpwarp               {return( TORPWARP );} 
warpmax                {return( WARPMAX );} 
armymax                {return( ARMYMAX );} 
shmax                  {return( SHMAX );} 
dammax                 {return( DAMMAX );} 
torpmax                {return( TORPMAX );} 
fuelmax                {return( FUELMAX );} 

planet                 {return ( PLANET );}
primary                {return ( PRIMARY );}
angle                  {return ( ANGLE );}
velocity               {return ( VELOCITY );}
radius                 {return ( RADIUS );}
ptype                  {return ( PTYPE );}
pteam                  {return ( PTEAM );}
armies                 {return ( ARMIES );}
visible                {return ( VISIBLE );}
core                   {return ( CORE );}
xcoord                 {return ( XCOORD );}
{WS}x{WS}              {return ( XCOORD );}
ycoord                 {return ( YCOORD );}
{WS}y{WS}              {return ( YCOORD );}
texname                {return ( TEXNAME );}
color                  {return ( COLOR );}
size                   {return ( SIZE );}
homeplanet             {return ( HOMEPLANET );}

filename               {return ( FILENAME );}
texture                {return ( TEXTURE );}

animation              {return ( ANIMATION );}
animdef                {return ( ANIMDEF );}

stages                 {return ( STAGES );}
loops                  {return ( LOOPS );}
delayms                {return ( DELAYMS );}
looptype               {return ( LOOPTYPE );}

timelimit              {return ( TIMELIMIT );}

texanim                {return ( TEXANIM );}
colanim                {return ( COLANIM );}
geoanim                {return ( GEOANIM );}
toganim                {return ( TOGANIM );}
istate                 {return ( ISTATE );}

deltaa                 {return ( DELTAA );}
deltar                 {return ( DELTAR );}
deltag                 {return ( DELTAG );}
deltab                 {return ( DELTAB );}
deltax                 {return ( DELTAX );}
deltay                 {return ( DELTAY );}
deltaz                 {return ( DELTAZ );}
deltas                 {return ( DELTAS );}
deltat                 {return ( DELTAT );}

soundconf              {return ( SOUNDCONF );}
samplerate             {return ( SAMPLERATE );}
volume                 {return ( VOLUME );}
pan                    {return ( PAN );}
stereo                 {return ( STEREO );}
fxchannels             {return ( FXCHANNELS );}
chunksize              {return ( CHUNKSIZE );}

effect                 {return ( EFFECT );}
fadeinms               {return ( FADEINMS );}
fadeoutms              {return ( FADEOUTMS );}
framelimit             {return ( FRAMELIMIT );}
limit                  {return ( LIMIT );}

music                  {return ( MUSIC );}

{WS}s{WS}              {return ( SCOORD );}
{WS}t{WS}              {return ( SCOORD );}

{number}		{ 
                          yylval.num = atoi( yytext );
                          return NUMBER;
                        }
{string}                { 
                          yylval.ptr = (char *)yytext;
                          return STRING; 
                        }

{rational}              { 
                          yylval.rnum = atof( yytext );
                          return RATIONAL;
                        }

{opensect}              {return( OPENSECT );}

{closesect}             {return( CLOSESECT );}

#+.*			/* eat comments */


{WS}			/* eat gratuitous white space */

\n			{lineNum++; /* eat newlines */}

%%

/* parser error reporting routine */
void yyerror(char *s)
{
   fprintf(stderr, "conqinit: %s at or near '%s' on line %d.\n", 
           s,
           yytext,
           lineNum);

   goterror = TRUE;

  return;
}


