#ifndef lint	/* .../appleprint/iw/daiw/do_vn.c */
#define _AC_NAME do_vn_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1987 Apple Computer, Inc., All Rights Reserved.  {Apple version 1.2 87/11/11 21:44:04}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char _do_vn_c[] = "@(#)Copyright Apple Computer 1987\tVersion 1.2 of do_vn.c on 87/11/11 21:44:04";
#endif		/* _AC_HISTORY */
#endif		/* lint */

#ifndef lint
#define _AC_MODS
#endif lint
/*
 *  Function:
 *	do_vn(ifile)
 *
 *  Arguments:
 *	ifile	The stream I/O pointer to a FILE structure. The input file
 *		contains the input produced by ditroff.
 *
 *  Returns:
 *	(nothing)
 *
 *  Description:
 *	This functions handles the ditroff output language `vn' command,
 *	which moves the vertical position `down' the paper (`n' > 0) by
 *	`n' amount of units.
 *
 *  Algorithm:
 *	Eat the vertical position movement value and add it to the current
 *	vertical position value.
 *
 *  History:
 *	Coded October, 1984 by Philip K. Ronzone.
 *
 */

#include "local.h"

void do_vn(ifile)
    FILE   *ifile;
{
    tpos_vert += eatnnn(ifile);
    pos_vert = tpos_vert / dev_scale;
}
