%{
#ifndef lint	/* .../appleprint/dwb/text/pic.d/picy.y */
#define _AC_NAME picy_y
#define _AC_NO_MAIN "@(#) Copyright (c) 1984-85 AT&T-IS, All Rights Reserved.  {Apple version 1.2 87/11/11 21:57:31}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987\tVersion 1.2 of picy.y on 87/11/11 21:57:31";    
#endif		/* _AC_HISTORY */
#endif		/* lint */
%}

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

%{
#define _AC_MODS
#include <stdio.h>
#include "pic.h"
#include <math.h>
YYSTYPE	y;
%}

%token	<i>	BOX	1	/* DON'T CHANGE THESE! */
%token	<i>	LINE	2
%token	<i>	ARROW	3
%token	<i>	CIRCLE	4
%token	<i>	ELLIPSE	5
%token	<i>	ARC	6
%token	<i>	SPLINE	7
%token	<i>	BLOCK	8
%token	<p>	TEXT	9
%token	<i>	TROFF	10
%token	<i>	MOVE	11
%token	<i>	PLOT	12
%token	<i>	BLOCKEND 13
%token	<i>	PLACE
%token	<i>	PRINT THRU UNTIL
%token	<o>	FOR IF COPY
%token	<p>	THENSTR ELSESTR DOSTR DEFNAME PLACENAME VARNAME
%token	<i>	ATTR TEXTATTR
%token	<i>	LEFT RIGHT UP DOWN FROM TO AT BY WITH HEAD CW CCW THEN
%token	<i>	HEIGHT WIDTH RADIUS DIAMETER LENGTH SIZE
%token	<i>	CORNER HERE LAST NTH SAME BETWEEN AND
%token	<i>	EAST WEST NORTH SOUTH NE NW SE SW START END
%token	<i>	DOTX DOTY DOTHT DOTWID DOTRAD
%token	<f>	NUMBER
%token	<f>	LOG EXP SIN COS ATAN2 SQRT RAND MIN MAX INT
%token	<i>	DIR
%token	<i>	DOT DASH CHOP
%token	<o>	ST	/* statement terminator */

%right	<f>	'='
%left	<f>	OROR
%left	<f>	ANDAND
%nonassoc <f>	GT LT LE GE EQ NEQ
%left	<f>	'+' '-'
%left	<f>	'*' '/' '%'
%right	<f>	UMINUS NOT

%type	<f>	expr if_expr asgn
%type	<p>	name
%type	<i>	optop
%type	<o>	if for copy

/* this is a lie:  picture and position are really the whole union */
%type	<o>	leftbrace picture piclist position lbracket
%type	<o>	prim place blockname
%type	<i>	textlist	/* not a sensible value */
%type	<i>	last type

%%

top:
	  piclist
	| /* empty */
	| error		{ yyerror("syntax error"); }
	;

piclist:
	  picture
	| piclist picture
	;

picture:
	  prim ST			{ codegen = 1; makeiattr(0, 0); }
	| leftbrace piclist '}'		{ rightthing($1, '}'); $$ = $2; }
	| PLACENAME ':' picture		{ y.o=$3; makevar($1,PLACENAME,y); $$ = $3; }
	| PLACENAME ':' ST picture	{ y.o=$4; makevar($1,PLACENAME,y); $$ = $4; }
	| PLACENAME ':' position ST	{ y.o=$3; makevar($1,PLACENAME,y); $$ = $3; }
	| asgn ST			{ y.f = $1; $$ = y.o; }
	| DIR				{ setdir($1); }
	| PRINT expr ST			{ printexpr($2); }
	| PRINT position ST		{ printpos($2); }
	| PRINT TEXT ST			{ fprintf(stderr, "%s\n", $2); free($2); }
	| copy
	| for
	| if
	| ST
	;

asgn:
	  VARNAME '=' expr	{ $$=y.f=$3; makevar($1,VARNAME,y); checkscale($1); }
	;

copy:
	  COPY copylist		{ copy(); }
	;
copylist:
	  copyattr
	| copylist copyattr
	;
copyattr:
	  TEXT			{ copyfile($1); }
	| THRU DEFNAME		{ copydef($2); }
	| UNTIL TEXT		{ copyuntil($2); }
	;

for:
	  FOR name FROM expr TO expr BY optop expr DOSTR
		{ forloop($2, $4, $6, $8, $9, $10); }
	| FOR name FROM expr TO expr DOSTR
		{ forloop($2, $4, $6, '+', 1.0, $7); }
	| FOR name '=' expr TO expr BY optop expr DOSTR
		{ forloop($2, $4, $6, $8, $9, $10); }
	| FOR name '=' expr TO expr DOSTR
		{ forloop($2, $4, $6, '+', 1.0, $7); }
	;

if:
	  IF if_expr THENSTR ELSESTR	{ ifstat($2, $3, $4); }
	| IF if_expr THENSTR		{ ifstat($2, $3, (char *) 0); }
	;
if_expr:
	  expr
	| TEXT EQ TEXT		{ $$ = strcmp($1,$3) == 0; free($1); free($3); }
	| TEXT NEQ TEXT		{ $$ = strcmp($1,$3) != 0; free($1); free($3); }
	;

name:
	  VARNAME	{ y.f = 0; makevar($1, VARNAME, y); }
	;
optop:
	  '+'		{ $$ = '+'; }
	| '-'		{ $$ = '-'; }
	| '*'		{ $$ = '*'; }
	| '/'		{ $$ = '/'; }
	| /* empty */	{ $$ = ' '; }
	;


leftbrace:
	  '{'			{ $$ = leftthing('{'); }
	;

prim:
	  BOX attrlist		{ $$ = boxgen($1); }
	| CIRCLE attrlist	{ $$ = circgen($1); }
	| ELLIPSE attrlist	{ $$ = circgen($1); }
	| ARC attrlist		{ $$ = arcgen($1); }
	| LINE attrlist		{ $$ = linegen($1); }
	| ARROW attrlist	{ $$ = linegen($1); }
	| SPLINE attrlist	{ $$ = linegen($1); }
	| MOVE attrlist		{ $$ = movegen($1); }
	| PLOT expr attrlist	{ $$ = plotgen($1, $2); }
	| textlist attrlist	{ $$ = textgen($1); }
	| TROFF			{ $$ = troffgen($1); }
	| lbracket piclist ']' { $<o>$=rightthing($1,']'); } attrlist
				{ $$ = blockgen($1, $2, $<o>4); }
	;

lbracket:
	  '['			{ $$ = leftthing('['); }
	;

attrlist:
	  attrlist attr
	| /* empty */
	;

attr:
	  ATTR expr		{ makefattr($1, !DEFAULT, $2); }
	| ATTR			{ makefattr($1, DEFAULT, 0.0); }
	| DIR expr		{ makefattr($1, !DEFAULT, $2); }
	| DIR			{ makefattr($1, DEFAULT, 0.0); }
	| FROM position		{ makeoattr($1, $2); }
	| TO position		{ makeoattr($1, $2); }
	| AT position		{ makeoattr($1, $2); }
	| BY position		{ makeoattr($1, $2); }
	| WITH CORNER		{ makeiattr(WITH, $2); }
	| WITH '.' PLACENAME	{ makeoattr(PLACE, getblock(getlast(1,BLOCK), $3)); }
	| WITH '.' PLACENAME CORNER
		{ makeoattr(PLACE, getpos(getblock(getlast(1,BLOCK), $3), $4)); }
	| WITH position		{ makeoattr(PLACE, $2); }
	| SAME			{ makeiattr(SAME, $1); }
	| TEXTATTR		{ maketattr($1, 0); }
	| HEAD			{ makeiattr(HEAD, $1); }
	| DOT expr		{ makefattr(DOT, !DEFAULT, $2); }
	| DOT			{ makefattr(DOT, DEFAULT, 0.0); }
	| DASH expr		{ makefattr(DASH, !DEFAULT, $2); }
	| DASH			{ makefattr(DASH, DEFAULT, 0.0); }
	| CHOP expr		{ makefattr(CHOP, !DEFAULT, $2); }
	| CHOP			{ makefattr(CHOP, DEFAULT, 0.0); }
	| textlist
	;

textlist:
	  TEXT			{ maketattr(CENTER, $1); }
	| TEXT TEXTATTR		{ maketattr($2, $1); }
	| textlist TEXT		{ maketattr(CENTER, $2); }
	| textlist TEXT TEXTATTR { maketattr($3, $2); }
	;

position:		/* absolute, not relative */
	  place
	| '(' position ')'			{ $$ = $2; }
	| expr ',' expr				{ $$ = makepos($1, $3); }
	| position '+' expr ',' expr		{ $$ = fixpos($1, $3, $5); }
	| position '-' expr ',' expr		{ $$ = fixpos($1, -$3, -$5); }
	| position '+' '(' expr ',' expr ')'	{ $$ = fixpos($1, $4, $6); }
	| position '-' '(' expr ',' expr ')'	{ $$ = fixpos($1, -$4, -$6); }
	| '(' place ',' place ')'	{ $$ = makepos(getcomp($2,DOTX), getcomp($4,DOTY)); }
	| expr LT position ',' position GT	{ $$ = makebetween($1, $3, $5); }
	| expr BETWEEN position AND position	{ $$ = makebetween($1, $3, $5); }
	;

place:
	  PLACENAME		{ y = getvar($1); $$ = y.o; }
	| PLACENAME CORNER	{ y = getvar($1); $$ = getpos(y.o, $2); }
	| CORNER PLACENAME	{ y = getvar($2); $$ = getpos(y.o, $1); }
	| HERE			{ $$ = gethere($1); }
	| last type		{ $$ = getlast($1, $2); }
	| last type CORNER	{ $$ = getpos(getlast($1, $2), $3); }
	| CORNER last type	{ $$ = getpos(getlast($2, $3), $1); }
	| NTH type		{ $$ = getfirst($1, $2); }
	| NTH type CORNER	{ $$ = getpos(getfirst($1, $2), $3); }
	| CORNER NTH type	{ $$ = getpos(getfirst($2, $3), $1); }
	| blockname
	| blockname CORNER	{ $$ = getpos($1, $2); }
	| CORNER blockname	{ $$ = getpos($2, $1); }
	;

blockname:
	  last BLOCK '.' PLACENAME	{ $$ = getblock(getlast($1,$2), $4); }
	| NTH BLOCK '.' PLACENAME	{ $$ = getblock(getfirst($1,$2), $4); }
	| PLACENAME '.' PLACENAME	{ y = getvar($1); $$ = getblock(y.o, $3); }
	;

last:
	  last LAST		{ $$ = $1 + 1; }
	| NTH LAST		{ $$ = $1; }
	| LAST			{ $$ = 1; }
	;

type:
	  BOX
	| CIRCLE
	| ELLIPSE
	| ARC
	| LINE
	| ARROW
	| SPLINE
	| BLOCK
	;

expr:
	  NUMBER
	| VARNAME		{ $$ = getfval($1); }
	| asgn
	| expr '+' expr		{ $$ = $1 + $3; }
	| expr '-' expr		{ $$ = $1 - $3; }
	| expr '*' expr		{ $$ = $1 * $3; }
	| expr '/' expr		{ if ($3 == 0.0) {
					yyerror("division by 0"); $3 = 1; }
				  $$ = $1 / $3; }
	| expr '%' expr		{ if ((long)$3 == 0) {
					yyerror("mod does division by 0"); $3 = 1; }
				  $$ = (long)$1 % (long)$3; }
	| '-' expr %prec UMINUS	{ $$ = -$2; }
	| '(' expr ')'		{ $$ = $2; }
	| place DOTX		{ $$ = getcomp($1, $2); }
	| place DOTY		{ $$ = getcomp($1, $2); }
	| place DOTHT		{ $$ = getcomp($1, $2); }
	| place DOTWID		{ $$ = getcomp($1, $2); }
	| place DOTRAD		{ $$ = getcomp($1, $2); }
	| expr GT expr		{ $$ = $1 > $3; }
	| expr LT expr		{ $$ = $1 < $3; }
	| expr LE expr		{ $$ = $1 <= $3; }
	| expr GE expr		{ $$ = $1 >= $3; }
	| expr EQ expr		{ $$ = $1 == $3; }
	| expr NEQ expr		{ $$ = $1 != $3; }
	| expr ANDAND expr	{ $$ = $1 && $3; }
	| expr OROR expr	{ $$ = $1 || $3; }
	| NOT expr		{ $$ = !($2); }
	| LOG '(' expr ')'		{ $$ = Log10($3); }
	| EXP '(' expr ')'		{ $$ = Exp($3 * log(10.0)); }
	| SIN '(' expr ')'		{ $$ = sin($3); }
	| COS '(' expr ')'		{ $$ = cos($3); }
	| ATAN2 '(' expr ',' expr ')'	{ $$ = atan2($3, $5); }
	| SQRT '(' expr ')'		{ $$ = Sqrt($3); }
	| RAND '(' expr ')'		{ $$ = rand() % (int) $3 + 1; }
	| MAX '(' expr ',' expr ')'	{ $$ = $3 >= $5 ? $3 : $5; }
	| MIN '(' expr ',' expr ')'	{ $$ = $3 <= $5 ? $3 : $5; }
	| INT '(' expr ')'		{ $$ = (long) $3; }
	;
