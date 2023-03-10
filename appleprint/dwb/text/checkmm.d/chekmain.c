#ifndef lint	/* .../appleprint/dwb/text/checkmm.d/chekmain.c */
#define _AC_NAME chekmain_c
#define _AC_MAIN "@(#) Copyright (c) 1984-85 AT&T-IS, All Rights Reserved.  {Apple version 1.2 87/11/11 21:50:21}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987	Version 1.2 of chekmain.c on 87/11/11 21:50:21";
  char *_Version_ = "A/UX Release 1.0";
#endif		/* _AC_HISTORY */
#endif		/* lint */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define _AC_MODS
#define _AC_MODS

	/* This Program checks for many of the errors
	 * which can be committed in PWB/MM input including some tbl & eqn.
	 *
	 * It is intended to be faster to save both user and processor
	 * time.
	 *
	 * Savings of 15:1 have been measured on files of 20,000 char
	 * X 1000 lines with only a few errors to print.
	 *
	 *
	 * It also avoids garbled output from mixing formatted results
	 * with error messages although there are other ways to do this.
	 *
	 * Another advantage is that it makes explicit statements about
	 * mistakes which cause output to be lost.
	 *
	 *
	 * The errors are those recognized from the relevant
	 * documents on the macros (mm 8.2) and Tbl and Eqn.
	 *
	 * In fact the Eqn checking code was lifted directly
	 * from the checkeq program.
	 *
	 * A couple of minor additional eqn checks are made in the
	 * Lex part of this program.
	 *
	 * The remainder of the Lex checks for nesting,
	 * missing, and out of sequence errors of other commonly
	 * used macros.
	 *
	 * There is no claim to completeness and the error messages
	 * are cast differently from those of PWB/MM.
	 *
	 * The need for this program may well diminish with the
	 * error diagnostics accompanying version 2.0.
	 *
	 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
FILE *fout;
int errors = 0;

main(argc, argv) 

char **argv ; 
int argc ;

{
char *malloc();
char *mktemp();
char outfile[128] ;
char *buf;
extern int yylineno ;
extern FILE *yyin;
FILE *fin;

struct stat stbuf ;
	if (argc == 1)
		{ yyin = stdin;
		  yylineno = 1 ;
		  yylex() ;
		}
	else
	while (--argc > 0) {
		stat(*++argv, &stbuf) ;

		if ( stbuf.st_mode &  S_IFDIR ) {
			continue ;
		}

		if ((fin = fopen(*argv, "r")) == NULL) {
			printf("Can't open %s\n", *argv) ;
			continue ; 
		}
		strcpy (outfile, "/usr/tmp/checkmm.tXXXXXX") ;
		mktemp(outfile) ;
		fout = fopen(outfile, "w");
		fprintf(fout, "   %s:\n", *argv) ;
		yyin = fin ;
		yylineno = 1 ;
		yylex() ;
		fclose(fin) ;
		fclose(fout) ;
		sprintf((buf = malloc(100)), "checkmm1 %s >> %s\n", *argv, outfile) ;
		system( buf ) ;
		sprintf(buf, "sort %s\n", outfile) ;
		system( buf) ;
		sprintf(buf, "rm %s\n", outfile ) ;
		system ( buf ) ;
	}

	exit(errors);
}
