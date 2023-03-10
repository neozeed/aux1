#ifndef lint	/* .../appleprint/dwb/text/tbl.d/t..c */
#define _AC_NAME t__c
#define _AC_NO_MAIN "@(#) Copyright (c) 1984-85 AT&T-IS, All Rights Reserved.  {Apple version 1.2 87/11/11 22:13:53}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *t_c_sccsid = "@(#)Copyright Apple Computer 1987	Version 1.2 of t..c on 87/11/11 22:13:53";
#endif		/* _AC_HISTORY */
#endif		/* lint */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define _AC_MODS
/* t..c : external declarations */


# include "stdio.h"
# include "ctype.h"

# define MAXLIN 250
# define MAXHEAD 44
# define MAXCOL 30
 /* Do NOT make MAXCOL bigger with adjusting nregs[] in tr.c */
# define MAXCHS 2000
# define MAXRPT 100
# define CLLEN 10
# define SHORTLINE 4
extern int nlin, ncol, iline, nclin, nslin;

extern int (*style)[MAXHEAD];
extern char (*font)[MAXHEAD][2];
extern char (*csize)[MAXHEAD][4];
extern char (*vsize)[MAXHEAD][4];
extern char (*cll)[CLLEN];
extern int (*flags)[MAXHEAD];
# define ZEROW 001
# define HALFUP 002
# define CTOP 004
# define CDOWN 010
extern int stynum[];
extern int qcol;
extern int *doubled, *acase, *topat;
extern int F1, F2;
extern int (*lefline)[MAXHEAD];
extern int fullbot[];
extern char *instead[];
extern int expflg;
extern int ctrflg;
extern int evenflg;
extern int *evenup;
extern int boxflg;
extern int dboxflg;
extern int linsize;
extern int tab;
extern int pr1403;
extern int linsize, delim1, delim2;
extern int allflg;
extern int textflg;
extern int left1flg;
extern int rightl;
struct colstr {char *col, *rcol;};
extern struct colstr *table[];
extern char *cspace, *cstore;
extern char *exstore, *exlim, *exspace;
extern int *sep;
extern int *used, *lused, *rused;
extern int linestop[];
extern int leftover;
extern char *last, *ifile;
extern int texname;
extern int texct, texmax;
extern char texstr[];
extern int linstart;


extern FILE *tabin, *tabout;
# define CRIGHT 2
# define CLEFT 0
# define CMID 1
# define S1 31
# define S2 32
# define S3 30
# define TMP 38
# define SF 35
# define SL 34
# define LSIZE 33
# define SIND 37
# define SVS 36
/* this refers to the relative position of lines */
# define LEFT 1
# define RIGHT 2
# define THRU 3
# define TOP 1
# define BOT 2
