#ifndef lint	/* .../appleprint/iw/daiw/do_hn.c */
#define _AC_NAME do_hn_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1987 Apple Computer, Inc., All Rights Reserved.  {Apple version 1.2 87/11/11 21:43:49}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char _do_hn_c[] = "@(#)Copyright Apple Computer 1987\tVersion 1.2 of do_hn.c on 87/11/11 21:43:49";
#endif		/* _AC_HISTORY */
#endif		/* lint */

#ifndef lint
#define _AC_MODS
#endif lint
/*
 *  Function:
 *	do_hn(ifile)
 *
 *  Arguments:
 *	ifile	The stream I/O pointer to a FILE structure. The input file
 *		contains the input produced by ditroff.
 *
 *  Returns:
 *	(nothing)
 *
 *  Description:
 *	This functions handles the ditroff output language `hn' command,
 *	where `h' is the horizontal movement to the right command,
 *	and `n' is the number of units.
 *
 *  Algorithm:
 *	Eat the digits indicating horizontal movement and add them
 *	to the horizontal position variable.
 *
 *  History:
 *	Coded October, 1984 by Philip K. Ronzone.
 *
 */

#include "local.h"

void do_hn(ifile)
    FILE   *ifile;
{
    tpos_horz += eatnnn(ifile);
    pos_horz = tpos_horz / dev_scale;
}
