#ifndef lint	/* .../appleprint/dwb/text/pic.d/blockgen.c */
#define _AC_NAME blockgen_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1984-85 AT&T-IS, All Rights Reserved.  {Apple version 1.2 87/11/11 21:56:27}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987\tVersion 1.2 of blockgen.c on 87/11/11 21:56:27";
#endif		/* _AC_HISTORY */
#endif		/* lint */

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#define _AC_MODS
#include	<stdio.h>
#include	"pic.h"
#include	"y.tab.h"

#define	NBRACK	20	/* depth of [...] */
#define	NBRACE	20	/* depth of {...} */

struct pushstack stack[NBRACK];
int	nstack	= 0;
struct pushstack bracestack[NBRACE];
int	nbstack	= 0;

obj *leftthing(c)	/* called for {... or [... */
			/* really ought to be separate functions */
	int c;
{
	obj *p;

	if (c == '[') {
		if (nstack >= NBRACK)
			fatal("[...] nested too deep");
		stack[nstack].p_x = curx;
		stack[nstack].p_y = cury;
		stack[nstack].p_hvmode = hvmode;
		curx = cury = 0;
		stack[nstack].p_xmin = xmin;
		stack[nstack].p_xmax = xmax;
		stack[nstack].p_ymin = ymin;
		stack[nstack].p_ymax = ymax;
		nstack++;
		xmin = ymin = 30000;
		xmax = ymax = -30000;
		p = makenode(BLOCK, 7);
		p->o_val[4] = nobj;	/* 1st item within [...] */
		if (p->o_nobj != nobj-1)
			fprintf(stderr, "nobjs wrong%d %d\n", p->o_nobj, nobj);
	} else {
		if (nbstack >= NBRACK)
			fatal("{...} nested too deep");
		bracestack[nbstack].p_x = curx;
		bracestack[nbstack].p_y = cury;
		bracestack[nbstack].p_hvmode = hvmode;
		nbstack++;
		p = NULL;
	}
	return(p);
}

obj *rightthing(p, c)	/* called for ... ] or ... } */
	obj *p;
{
	obj *q;

	if (c == '}') {
		nbstack--;
		curx = bracestack[nbstack].p_x;
		cury = bracestack[nbstack].p_y;
		hvmode = bracestack[nbstack].p_hvmode;
		q = makenode(MOVE, 0);
		dprintf("M %g %g\n", curx, cury);
	} else {
		nstack--;
		curx = stack[nstack].p_x;
		cury = stack[nstack].p_y;
		hvmode = stack[nstack].p_hvmode;
		q = makenode(BLOCKEND, 7);
		q->o_val[4] = p->o_nobj + 1;	/* back pointer */
		p->o_val[5] = q->o_nobj - 1;	/* forward pointer */
		p->o_val[0] = xmin; p->o_val[1] = ymin;
		p->o_val[2] = xmax; p->o_val[3] = ymax;
		p->o_dotdash = q->o_dotdash = (int) stack[nstack+1].p_symtab;
		xmin = stack[nstack].p_xmin;
		ymin = stack[nstack].p_ymin;
		xmax = stack[nstack].p_xmax;
		ymax = stack[nstack].p_ymax;
	}
	return(q);
}

obj *blockgen(p, type, q)	/* handles [...] */
	obj *p, *q;
	int type;
{
	static float prevh = HT;
	static float prevw = WID;	/* golden mean, sort of */
	int i, invis, at, ddtype, with;
	float ddval, h, w, xwith, ywith;
	float x0, y0, x1, y1, cx, cy;
	obj *ppos;
	Attr *ap;

	invis = at = 0;
	with = xwith = ywith = 0;
	ddtype = ddval = 0;
	w = p->o_val[2] - p->o_val[0];
	h = p->o_val[3] - p->o_val[1];
	cx = (p->o_val[2] + p->o_val[0]) / 2;	/* geom ctr of [] wrt local orogin */
	cy = (p->o_val[3] + p->o_val[1]) / 2;
	dprintf("cx,cy=%g,%g\n", cx, cy);
	for (i = 0; i < nattr; i++) {
		ap = &attr[i];
		switch (ap->a_type) {
		case HEIGHT:
			h = ap->a_val.f;
			break;
		case WIDTH:
			w = ap->a_val.f;
			break;
		case SAME:
			h = prevh;
			w = prevw;
			break;
		case WITH:
			with = ap->a_val.i;	/* corner */
			break;
		case PLACE:	/* actually with position ... */
			ppos = ap->a_val.o;
			xwith = cx - ppos->o_x;
			ywith = cy - ppos->o_y;
			with = PLACE;
			break;
		case AT:
			ppos = ap->a_val.o;
			curx = ppos->o_x;
			cury = ppos->o_y;
			at++;
			break;
		case INVIS:
			invis = INVIS;
			break;
		case TEXTATTR:
			savetext(ap->a_sub, ap->a_val.p);
			break;
		}
	}
	if (with) {
		switch (with) {
		case NORTH:	ywith = -h / 2; break;
		case SOUTH:	ywith = h / 2; break;
		case EAST:	xwith = -w / 2; break;
		case WEST:	xwith = w / 2; break;
		case NE:	xwith = -w / 2; ywith = -h / 2; break;
		case SE:	xwith = -w / 2; ywith = h / 2; break;
		case NW:	xwith = w / 2; ywith = -h / 2; break;
		case SW:	xwith = w / 2; ywith = h / 2; break;
		}
		curx += xwith;
		cury += ywith;
	}
	if (!at) {
		if (isright(hvmode))
			curx += w / 2;
		else if (isleft(hvmode))
			curx -= w / 2;
		else if (isup(hvmode))
			cury += h / 2;
		else
			cury -= h / 2;
	}
	x0 = curx - w / 2;
	y0 = cury - h / 2;
	x1 = curx + w / 2;
	y1 = cury + h / 2;
	extreme(x0, y0);
	extreme(x1, y1);
	p->o_x = curx;
	p->o_y = cury;
	p->o_nt1 = ntext1;
	p->o_nt2 = ntext;
	ntext1 = ntext;
	p->o_val[0] = w;
	p->o_val[1] = h;
	p->o_val[2] = cx;
	p->o_val[3] = cy;
	p->o_val[5] = q->o_nobj - 1;		/* last item in [...] */
	p->o_ddval = ddval;
	p->o_attr = invis;
	dprintf("[] %g %g %g %g at %g %g, h=%g, w=%g\n", x0, y0, x1, y1, curx, cury, h, w);
	if (isright(hvmode))
		curx = x1;
	else if (isleft(hvmode))
		curx = x0;
	else if (isup(hvmode))
		cury = y1;
	else
		cury = y0;
	for (i = 0; i <= 5; i++)
		q->o_val[i] = p->o_val[i];
	stack[nstack+1].p_symtab = NULL;	/* so won't be found again */
	blockadj(p);	/* fix up coords for enclosed blocks */
	prevh = h;
	prevw = w;
	return(p);
}

blockadj(p)	/* adjust coords in block starting at p */
	obj *p;
{
	obj *q;
	float dx, dy;
	int n, lev;

	dx = p->o_x - p->o_val[2];
	dy = p->o_y - p->o_val[3];
	n = p->o_nobj + 1;
	q = objlist[n];
	dprintf("into blockadj: dx,dy=%g,%g\n", dx, dy);
	for (lev = 1; lev > 0; n++) {
		p = objlist[n];
		if (p->o_type == BLOCK)
			lev++;
		else if (p->o_type == BLOCKEND)
			lev--;
		dprintf("blockadj: type=%d o_x,y=%g,%g;", p->o_type, p->o_x, p->o_y);
		p->o_x += dx;
		p->o_y += dy;
		dprintf(" becomes %g,%g\n", p->o_x, p->o_y);
		switch (p->o_type) {	/* other absolute coords */
		case LINE:
		case ARROW:
		case SPLINE:
			p->o_val[0] += dx;
			p->o_val[1] += dy;
			break;
		case ARC:
			p->o_val[0] += dx;
			p->o_val[1] += dy;
			p->o_val[2] += dx;
			p->o_val[3] += dy;
			break;
		}
	}
}
