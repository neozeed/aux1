#ifndef lint	/* .../appleprint/dwb/text/tbl.d/ti.c */
#define _AC_NAME ti_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1984-85 AT&T-IS, All Rights Reserved.  {Apple version 1.2 87/11/11 22:14:50}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987	Version 1.2 of ti.c on 87/11/11 22:14:50";
#endif		/* _AC_HISTORY */
#endif		/* lint */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define _AC_MODS
 /* ti.c: classify line intersections */


# include "t..c"
/* determine local environment for intersections */
interv(i,c)
{
int ku, kl;
if (c>=ncol || c == 0)
	{
	if (dboxflg)
		{
		if (i==0) return(BOT);
		if (i>=nlin) return(TOP);
		return(THRU);
		}
	if (c>=ncol)
		return(0);
	}
ku = i>0 ? lefdata(i-1,c) : 0;
if (i+1 >= nlin && allh(i))
	kl=0;
else
kl = lefdata(allh(i) ? i+1 : i, c);
if (ku==2 && kl==2) return(THRU);
if (ku ==2) return(TOP);
if (kl==BOT) return(2);
return(0);
}
interh(i,c)
{
int kl, kr;
if (fullbot[i]== '=' || (dboxflg && (i==0 || i>= nlin-1)))
	{
	if (c==ncol)
		return(LEFT);
	if (c==0)
		return(RIGHT);
	return(THRU);
	}
if (i>=nlin) return(0);
kl = c>0 ? thish (i,c-1) : 0;
if (kl<=1 && i>0 && allh(up1(i)))
	kl = c>0 ? thish(up1(i),c-1) : 0;
kr = thish(i,c);
if (kr<=1 && i>0 && allh(up1(i)))
	kr = c>0 ? thish(up1(i), c) : 0;
if (kl== '=' && kr ==  '=') return(THRU);
if (kl== '=') return(LEFT);
if (kr== '=') return(RIGHT);
return(0);
}
up1(i)
{
i--;
while (instead[i] && i>0) i--;
return(i);
}
