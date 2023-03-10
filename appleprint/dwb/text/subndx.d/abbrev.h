#ifndef lint	/* .../appleprint/dwb/text/subndx.d/abbrev.h */
#define _AC_NAME abbrev_h
#define _AC_NO_MAIN "@(#) Copyright (c) 1984-85 AT&T-IS, All Rights Reserved.  {Apple version 1.2 87/11/11 22:10:36}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *abbrev_h_sccsid = "@(#)Copyright Apple Computer 1987	Version 1.2 of abbrev.h on 87/11/11 22:10:36";
#endif		/* _AC_HISTORY */
#endif		/* lint */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define _AC_MODS
#define abtsize 71
#define abp1 61
#define abp2 67
extern struct dict abbrev_d[];
struct hnode abroot[abtsize];
char
abbrev(a0, a1, ptr)
char	*a0;
struct dict *ptr;
{
	return(aahash(a0, a1, abtsize, abp1, abp2, abroot, ptr));
}


getab()
{
	struct dict *ptr;
	ptr = abbrev_d;
	while (ptr->entry != 0) {
		abbrev(ptr->entry, 0, ptr);
		ptr++;
	}
}


struct dict abbrev_d[] = {
	"St", 'N',
	"Dr", 'N',
	"Drs", 'N',
	"Mr", 'N',
	"Mrs", 'N',
	"Ms", 'N',
	"Rev", 'N',
	"No", 'Y',
	"Nos", 'Y',
	"NO", 'Y',
	"NOs", 'Y',
	"no", 'Y',
	"Fig", 'Y',
	"Figs", 'Y',
	"Dept", 'Y',
	"Depts", 'Y',
	"dept", 'Y',
	"depts", 'Y',
	"Eq", 'Y',
	"Eqs", 'Y',
	"eq", 'Y',
	"eqs", 'Y',
	"dB", 'Y',
	"vs", 'P',
	"in", 'Y',
	"ft", 'Y',
	"yr", 'Y',
	"ckts", 'Y',
	"mi", 'Y',
	"Jr", 'J',
	"jr", 'J',
	"Ch", 'Y',
	"ch", 'Y',
	"Ref", 'Y',
	"Refs", 'Y',
	"ref", 'Y',
	"refs", 'Y',
	"Inc", 'J',
	"Co", 'N',
	"Corp", 'N',
	"Jan", 'N',
	"Feb", 'N',
	"Mar", 'N',
	"Apr", 'N',
	"Jun", 'N',
	"Aug", 'N',
	"Sept", 'N',
	"Oct", 'N',
	"Nov", 'N',
	"Dec", 'N',
	"Sen", 'Y',
	"Sens", 'Y',
	"Rep", 'Y',
	"Hon", 'Y',
	"Gov", 'Y',
	"Lt", 'Y',
	"Col", 'Y',
	"Comdr", 'Y',
	"Cmdr", 'Y',
	"Capt", 'Y',
	"Calif", 'N',
	"Ky", 'N',
	"Va", 'N',
	0, 0
};


