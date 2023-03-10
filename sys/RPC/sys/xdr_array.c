#ifndef lint	/* .../sys/RPC/sys/xdr_array.c */
#define _AC_NAME xdr_array_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1983-87 Sun Microsystems Inc., All Rights Reserved.  {Apple version 1.2 87/11/11 21:31:08}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987\tVersion 1.2 of xdr_array.c on 87/11/11 21:31:08 1.1 86/02/03 Copyr 1984 Sun Micro";
#endif		/* _AC_HISTORY */
#endif		/* lint */

/* NFSSRC @(#)xdr.c	2.1 86/04/14 */
#ifndef lint
#define _AC_MODS
#endif

/*
 * xdr_array.c, Generic XDR routines impelmentation.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * arrays.  See xdr.h for more info on the interface to xdr.
 */

#ifdef KERNEL
#include "sys/param.h"
#include "sys/signal.h"
#include "sys/types.h"
#include "rpc/types.h"
#include "rpc/xdr.h"
char	*kmem_alloc();
#else
#include "types.h"	/* <> */
#include "xdr.h"	/* <> */
#include <stdio.h>
char	*malloc();
#endif
#define LASTUNSIGNED	((u_int)0-1)


/*
 * XDR an array of arbitrary elements
 * *addrp is a pointer to the array, *sizep is the number of elements.
 * If addrp is NULL (*sizep * elsize) bytes are allocated.
 * elsize is the size (in bytes) of each element, and elproc is the
 * xdr procedure to call to handle each element of the array.
 */
bool_t
xdr_array(xdrs, addrp, sizep, maxsize, elsize, elproc)
	register XDR *xdrs;
	caddr_t *addrp;		/* array pointer */
	u_int *sizep;		/* number of elements */
	u_int maxsize;		/* max numberof elements */
	u_int elsize;		/* size in bytes of each element */
	xdrproc_t elproc;	/* xdr routine to handle each element */
{
	register u_int i;
	register caddr_t target = *addrp;
	register u_int c;  /* the actual element count */
	register bool_t stat = TRUE;
	register int nodesize;

	/* like strings, arrays are really counted arrays */
	if (! xdr_u_int(xdrs, sizep)) {
#ifdef KERNEL
		printf("xdr_array: size FAILED\n");
#endif
		return (FALSE);
	}
	c = *sizep;
	if ((c > maxsize) && (xdrs->x_op != XDR_FREE)) {
#ifdef KERNEL
		printf("xdr_array: bad size FAILED\n");
#endif
		return (FALSE);
	}
	nodesize = c * elsize;

	/*
	 * if we are deserializing, we may need to allocate an array.
	 * We also save time by checking for a null array if we are freeing.
	 */
	if (target == NULL)
		switch ((int) xdrs->x_op) {
		case XDR_DECODE:
			if (c == 0)
				return (TRUE);
			*addrp = target = mem_alloc(nodesize);
#ifndef KERNEL
			if (target == NULL) {
				fprintf(stderr, "xdr_array: out of memory\n");
				return (FALSE);
			}
#endif
			bzero(target, (u_int)nodesize);
			break;

		case XDR_FREE:
			return (TRUE);
	}
	
	/*
	 * now we xdr each element of array
	 */
	for (i = 0; (i < c) && stat; i++) {
		stat = (*elproc)(xdrs, target, LASTUNSIGNED);
		target += elsize;
	}

	/*
	 * the array may need freeing
	 */
	if (xdrs->x_op == XDR_FREE) {
		mem_free(*addrp, nodesize);
		*addrp = NULL;
	}
	return (stat);
}
