%{

/*
 * conqinit - lexical scanner for conqinit
 *
 */

#include "c_defs.h"
#include "conqinit.h"
#include "conqinit-yacc.h"

#include "conqdef.h"
#include "cb.h"

int goterror = 0;
int lineNum = 1; /* current line number of the scanner */

extern int cqiDebugl;                       /* from conqinit-yacc.y */

/* try to short-circuit ECHO redef warnings from compiler.. */
#undef ECHO
#define ECHO { if (cqiDebugl) (void)fprintf(yyout, "Unrecognized input: '%s', ignored.\n", yytext); }
#define YY_NO_INPUT
%}

%option noyywrap

whtspc	 [ \t]+
string   \"([^"]|\\.)*\"
number   [0-9]+
rational -?(([0-9]+)|([0-9]*\.[0-9]+)([eE][-+]?[0-9]+)?)
hex      [0-9a-f]+
opensect  \{
closesect \}
%%

name                   {return( TOK_NAME );}

global                 {return( TOK_GLOBAL );}
planetmax              {return( TOK_PLANETMAX );}
shipmax                {return( TOK_SHIPMAX );}
usermax                {return( TOK_USERMAX );}
msgmax                 {return( TOK_MSGMAX );}
histmax                {return( TOK_HISTMAX );}

shiptype               {return( TOK_SHIPTYPE );}
engfac                 {return( TOK_ENGFAC );}
weafac                 {return( TOK_WEAFAC );}
accelfac               {return( TOK_ACCELFAC );}
torpwarp               {return( TOK_TORPWARP );}
warpmax                {return( TOK_WARPMAX );}
armymax                {return( TOK_ARMYMAX );}
shmax                  {return( TOK_SHMAX );}
dammax                 {return( TOK_DAMMAX );}
torpmax                {return( TOK_TORPMAX );}
fuelmax                {return( TOK_FUELMAX );}

planet                 {return ( TOK_PLANET );}
primary                {return ( TOK_PRIMARY );}
angle                  {return ( TOK_ANGLE );}
velocity               {return ( TOK_VELOCITY );}
radius                 {return ( TOK_RADIUS );}
ptype                  {return ( TOK_PTYPE );}
pteam                  {return ( TOK_PTEAM );}
armies                 {return ( TOK_ARMIES );}
visible                {return ( TOK_VISIBLE );}
core                   {return ( TOK_CORE );}
xcoord                 {return ( TOK_XCOORD );}
{whtspc}x{whtspc}      {return ( TOK_XCOORD );}
ycoord                 {return ( TOK_YCOORD );}
{whtspc}y{whtspc}      {return ( TOK_YCOORD );}
texname                {return ( TOK_TEXNAME );}
color                  {return ( TOK_COLOR );}
size                   {return ( TOK_SIZE );}
homeplanet             {return ( TOK_HOMEPLANET );}

filename               {return ( TOK_FILENAME );}
texture                {return ( TOK_TEXTURE );}

animation              {return ( TOK_ANIMATION );}
animdef                {return ( TOK_ANIMDEF );}

stages                 {return ( TOK_STAGES );}
loops                  {return ( TOK_LOOPS );}
delayms                {return ( TOK_DELAYMS );}
looptype               {return ( TOK_LOOPTYPE );}

timelimit              {return ( TOK_TIMELIMIT );}

texanim                {return ( TOK_TEXANIM );}
colanim                {return ( TOK_COLANIM );}
geoanim                {return ( TOK_GEOANIM );}
toganim                {return ( TOK_TOGANIM );}
istate                 {return ( TOK_ISTATE );}

deltaa                 {return ( TOK_DELTAA );}
deltar                 {return ( TOK_DELTAR );}
deltag                 {return ( TOK_DELTAG );}
deltab                 {return ( TOK_DELTAB );}
deltax                 {return ( TOK_DELTAX );}
deltay                 {return ( TOK_DELTAY );}
deltaz                 {return ( TOK_DELTAZ );}
deltas                 {return ( TOK_DELTAS );}
deltat                 {return ( TOK_DELTAT );}

soundconf              {return ( TOK_SOUNDCONF );}
samplerate             {return ( TOK_SAMPLERATE );}
volume                 {return ( TOK_VOLUME );}
pan                    {return ( TOK_PAN );}
stereo                 {return ( TOK_STEREO );}
fxchannels             {return ( TOK_FXCHANNELS );}
chunksize              {return ( TOK_CHUNKSIZE );}

effect                 {return ( TOK_EFFECT );}
fadeinms               {return ( TOK_FADEINMS );}
fadeoutms              {return ( TOK_FADEOUTMS );}
framelimit             {return ( TOK_FRAMELIMIT );}
limit                  {return ( TOK_LIMIT );}

music                  {return ( TOK_MUSIC );}

{whtspc}s{whtspc}      {return ( TOK_SCOORD );}
{whtspc}t{whtspc}      {return ( TOK_SCOORD );}

{whtspc}w{whtspc}      {return ( TOK_WIDTH );}
{whtspc}h{whtspc}      {return ( TOK_HEIGHT );}

texarea                {return ( TOK_TEXAREA );}
mipmap                 {return ( TOK_MIPMAP );}

luminance              {return ( TOK_TEX_LUMINANCE );}

{number}		{
                          yylval.num = atoi( yytext );
                          return TOK_NUMBER;
                        }
{string}                {
                          yylval.ptr = (char *)yytext;
                          return TOK_STRING;
                        }

{rational}              {
                          yylval.rnum = atof( yytext );
                          return TOK_RATIONAL;
                        }

{opensect}              {return( TOK_OPENSECT );}

{closesect}             {return( TOK_CLOSESECT );}

#+.*			/* eat comments */


{whtspc}		/* eat gratuitous white space */

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
