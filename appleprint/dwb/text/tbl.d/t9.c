#ifndef lint	/* .../appleprint/dwb/text/tbl.d/t9.c */
#define _AC_NAME t9_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1984-85 AT&T-IS, All Rights Reserved.  {Apple version 1.2 87/11/11 22:14:29}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987	Version 1.2 of t9.c on 87/11/11 22:14:29";
#endif		/* _AC_HISTORY */
#endif		/* lint */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define _AC_MODS
 /* t9.c: write lines for tables over 200 lines */


# include "t..c"
static useln;
extern char *maknew();
yetmore()
{
for(useln=0; useln<MAXLIN && table[useln]==0; useln++);
if (useln>=MAXLIN)
	error("Wierd.  No data in table.");
table[0]=table[useln];
for(useln=nlin-1; useln>=0 && (fullbot[useln] || instead[useln]); useln--);
if (useln<0)
	error("Wierd.  No real lines in table.");
domore(leftover);
while (gets1(cstore=cspace) && domore(cstore))
	;
last =cstore;
return;
}
domore(dataln)
	char *dataln;
{
	int icol, ch;
if (prefix(".TE", dataln))
	return(0);
if (dataln[0] == '.' && !isdigit(dataln[1]))
	{
	puts(dataln);
	return(1);
	}
fullbot[0]=0;
instead[0]=(char *)0;
if (dataln[1]==0)
switch(dataln[0])
	{
	case '_': fullbot[0]= '-'; putline(useln,0);  return(1);
	case '=': fullbot[0]= '='; putline(useln, 0); return(1);
	}
for (icol = 0; icol <ncol; icol++)
	{
	table[0][icol].col = dataln;
	table[0][icol].rcol=0;
	for(; (ch= *dataln) != '\0' && ch != tab; dataln++)
			;
	*dataln++ = '\0';
	switch(ctype(useln,icol))
		{
		case 'n':
			table[0][icol].rcol = maknew(table[0][icol].col);
			break;
		case 'a':
			table[0][icol].rcol = table[0][icol].col;
			table[0][icol].col= "";
			break;
		}
	while (ctype(useln,icol+1)== 's') /* spanning */
		table[0][++icol].col = "";
	if (ch == '\0') break;
	}
while (++icol <ncol)
	table[0][icol].col = "";
putline(useln,0);
exstore=exspace; /* reuse space for numerical items */
return(1);
}
