#ifndef lint	/* .../sys/psn/io/keyboard.c */
#define _AC_NAME keyboard_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1987 Apple Computer, Inc., All Rights Reserved.  {Apple version 1.13 87/12/04 13:32:03}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987\tVersion 1.13 of keyboard.c on 87/12/04 13:32:03";
#endif		/* _AC_HISTORY */
#endif		/* lint */

#define _AC_MODS

/*	@(#)keyboard.c	UniPlus VVV.2.1.22	*/
/*
 * (C) 1986 UniSoft Corp. of Berkeley CA
 *
 * UniPlus Source Code. This program is proprietary
 * with Unisoft Corporation and is not to be reproduced
 * or used in any manner except as authorized in
 * writing by Unisoft.
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/errno.h"
#include "sys/mmu.h"
#include "sys/page.h"
#include "sys/seg.h"
#include "sys/systm.h"
#include "sys/time.h"
#include "sys/user.h"
#include "sys/file.h"
#include "sys/ioctl.h"
#include "sys/tty.h"
#include "sys/termio.h"
#include "sys/conf.h"
#include "sys/sysinfo.h"
#include "sys/var.h"
#include "sys/reg.h"
#include "sys/sysmacros.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/ttx.h"
#include "sys/key.h"
#include "sys/font.h"
#include "sys/video.h"
#include "sys/ioctl.h"
#include "sys/mouse.h"
#include "sys/debug.h"

#define DISP_CONSOLE 0

extern int nulldev();
extern int qenable();

/*
 *	These data structures describe the streams interface
 */

static int disp_open();
static int disp_close();
static int disp_proc();
static int disp_ioctl();
static int disp_param();

static struct module_info disp_info = {5321, "display", 0, 256, 256, 256};
static struct qinit disp_rq = {NULL, ttx_rsrvc, disp_open, disp_close,
				nulldev, &disp_info, NULL};
static struct qinit disp_wq = {ttx_wputp, ttx_wsrvc, disp_open, disp_close,
				nulldev, &disp_info, NULL};
struct streamtab disp_tab = {&disp_rq, &disp_wq, NULL, NULL};

/*
 *	This is the number of displays supported ... to support more than
 *		one the problem of associating keyboards with video cards
 *		must be solved (an Apple problem)
 *
 */

#define NDISPLAYS 1

struct ttx disptty[NDISPLAYS];			/* the Q head interface 
						   structure */

static disp_rintr();

static struct font disp_font[NDISPLAYS];	/* the screen interface
						   structure */
static int disp_cflag[NDISPLAYS];		/* The current screen state */

static char disp_bval[NDISPLAYS];		/* current mouse button value */

static char disp_button[NDISPLAYS];		/* Do we report ALL mouse
						   mouse movement or just
						   changes in the button */

static char disp_mouse[NDISPLAYS];		/* Are we inserting mouse
						   events into our input
						   input stream */

static short disp_mx[NDISPLAYS];		/* mouse x position saved for
						   reporting changes in delta
						   x positions */
static short disp_my[NDISPLAYS];		/* same for y */

static char *disp_fontname[] = {		/* font name table */
	0,    		/* B0 */
	0,   		/* B50 */
	0,     		/* B75 */
	0,    		/* B110 */
	0,    		/* B134 */
	0,    		/* B150 */
	0,    		/* B200 */
	0,    		/* B300 */
	0,   		/* B600 */
	0,    		/* B1200 */
	0,   		/* B1800 */
	0,    		/* B2400 */
	0,    		/* B4800 */
	0,    		/* B9600 */
	0,    		/* EXTA */
	0,    		/* EXTB */
};

static unsigned char disp_point[] = {		/* font structure table */
	0,		/* B0 */
	0,		/* B50 */
	0, 		/* B75 */
	0,		/* B110 */
	0,		/* B134 */
	0,		/* B150 */
	0,		/* B200 */
	0,		/* B300 */
	0,		/* B600 */
	0,		/* B1200 */
	0,		/* B1800 */
	0,		/* B2400 */
	0,		/* B4800 */
	0,		/* B9600 */
	0,		/* EXTA */
	0,		/* EXTB */
};

extern struct chorddata {
    short volume;
    long spinloop;
    long notespeed;
    long countdown;
    short nnotes;
    long note[4];
} chorddata;
#ifdef DISP_CONSOLE
static dispputchar();
extern int (*locputchar)();
#endif DISP_CONSOLE

/*
 *	Initialise the screens, associate fonts with them, and paint a 
 *		cursor. If there is a console setup the kernel printf
 *		address.
 */

dispinit()
{
	register struct fpnt *fp;
	register int i;
	char c;

	fp = font_find("TTY", 12);
	for (i = 0; i < NDISPLAYS && i < video_count; i++) {
		font_set(&disp_font[i], fp, video_desc[i],
				LEFT_SHAVE, RIGHT_SHAVE, TOP_SHAVE, BOTTOM_SHAVE);
		disp_font[i].font_inverse = 1;
		disp_cflag[i] = CS8;
		vt100_setup(i, &disp_font[i], disp_rintr);
		video_bitmap(video_desc[i]);
		(*disp_font[i].font_invert)(&disp_font[i], 0, 0);
	}
#ifdef DISP_CONSOLE
	if (video_count)
		locputchar = dispputchar;
#endif DISP_CONSOLE
}

/*
 *	Open a tty .... find a keyboard
 */

static
disp_open(q, dev, flag, sflag, err)
queue_t *q;
dev_t dev;
int *err;
{
	register struct ttx *tp;
	register int s;
	register struct key_data *kdp;

	dev = minor(dev);
	if (dev >= NDISPLAYS || dev >= video_count) {
		*err = ENXIO;
		return(OPENFAIL);
	}
	tp = &disptty[dev];
	kdp = video_desc[dev]->video_key;
	s = spl1();
	if ((tp->t_state&(ISOPEN|WOPEN)) == 0) {

		if ((*kdp->key_op)(dev, KEY_OP_OPEN, 0) ||
		    (*kdp->key_open)(dev, disp_rintr, KEY_ASCII) == 0) {
			splx(s);
			return(OPENFAIL);
		}
		/*
		 *	If no other processes have this device open
		 *		set up the Q interface structure
		 *		and open the keyboard
		 */

		tp->t_proc = disp_proc;
		tp->t_ioctl = disp_ioctl;
		ttxinit(q, tp, 4);
		tp->t_state |= CARR_ON;
		tp->t_dev = dev;
		disp_cflag[dev] = tp->t_cflag;
	}

	/*
	 *	Mark the keyboard open and return success
	 */

	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;
	splx(s);
	return(1);
}

/*
 *	Close the tty before the Q is dismantled
 */

/* ARGSUSED */
static
disp_close(q, flag)
queue_t *q;
int flag;
{
	register struct ttx *tp;
	struct mpcc *addr;
	int s;

	tp = (struct ttx *)q->q_ptr;
	s = spl1();
	if (disp_mouse[tp->t_dev]) {
		disp_mouse[tp->t_dev] = 0;
		(*video_desc[tp->t_dev]->video_mouse->mouse_close)(tp->t_dev);
	}
	disp_button[tp->t_dev] = 0;
	(*video_desc[tp->t_dev]->video_key->key_close)(tp->t_dev);
	splx(s);
	ttx_close(tp);
	return(0);
}

/*
 *	Receive interrupt routine, called whenever characters are available
 *		from the keyboard (or fake characters from answerback etc in
 *		the vt100 emulator)
 */

static
disp_rintr(id, cmd, c, next)
int id;
register int c;
{
	register mblk_t *m; 
	register struct ttx *tp;
	int  s, lcnt, flg;
	char lbuf[3];

	tp = &disptty[id];

	/*
	 *	We got a character .... undim the screen
	 */

	(*disp_font[tp->t_dev].font_screen->video_func)
		(disp_font[tp->t_dev].font_screen, VF_UNDIM);

	/*
	 *	Do XON/XOFF processing
	 */

	sysinfo.rcvint++;
	flg = tp->t_iflag;
	if (flg&ISTRIP)
		c &= 0177;
	else 
		c &= 0377;

	if (flg&IXON) {
		if (tp->t_state&TTSTOP) {
			if (c == CSTART || tp->t_iflag&IXANY)
				(*tp->t_proc)(tp, T_RESUME);
		} else {
			if (c == CSTOP)
				(*tp->t_proc)(tp, T_SUSPEND);
		}
		if (c == CSTART || c == CSTOP)
			return;
	}

	/*
	 *	If no buffers .... return ... nothing to do
	 */

	if ((m = tp->t_rm) == NULL) {
		return;
	}

	/*
	 *	Do parity processing
	 */

	lcnt = 1;
	if ((flg&ISTRIP) == 0) {
		if (c == 0377 && flg&PARMRK) {
			lbuf[1] = 0377;
			lcnt = 2;
		}
	}

	/*
	 *	Pass data to the stream driver code
	 */

	if (lcnt != 1) {
		lbuf[0] = c;
		while (lcnt) {
			*m->b_wptr++ = lbuf[--lcnt];
			if (--tp->t_count == 0) {
				if (ttx_put(tp))
					return;
				if ((m = tp->t_rm) == NULL)
					return;
			}
		}
		if (m && next == 0 && m->b_wptr != m->b_rptr) {
			(void) ttx_put(tp);
		}
	} else {
		*m->b_wptr++ = c;
		if ((--tp->t_count) == 0 || next == 0) {
			(void) ttx_put(tp);
		}
	}
}

static
disp_mintr(id, cmd, v)
{
	struct mouse_data *mdp;

	if (cmd != MOUSE_CHANGE)
		return;
	if (disp_button[id]) {
		mdp = video_desc[id]->video_mouse;
		if (disp_bval[id] ==
	 	    (*mdp->mouse_op)(id, MOUSE_OP_BUTTON, 0))
			return;
		disp_bval[id] = (*mdp->mouse_op)(id, MOUSE_OP_BUTTON, 0);
		disp_rintr(id, KC_CHAR, MOUSE_ESCAPE, 1);
		disp_rintr(id, KC_CHAR, disp_bval[id], 0);
	} else {
		disp_rintr(id, KC_CHAR, MOUSE_ESCAPE, 1);
		disp_rintr(id, KC_CHAR, (v>>8)&0xff, 1);
		disp_rintr(id, KC_CHAR, v&0xff, 0);
	}
}

/*
 *	ioctl
 */

static
disp_ioctl(tp, iocbp, m)
struct ttx *tp;
struct iocblk *iocbp;
mblk_t *m;
{
	long len, *ip;
	short *sp;
	struct video *vp;
	struct mouse_data *mdp;
	struct winsize *w;

	switch (iocbp->ioc_cmd) {
	/*
	 *	If required put the keyboard into ascii raw mode
	 */

	case VIDEO_RAW:
		(*video_desc[tp->t_dev]->video_key->key_op)(tp->t_dev,
							    KEY_OP_MODE, KEY_ARAW);
		return(0);

	/*
	 *	If required put the keyboard into normal ascii mode
	 */

	case VIDEO_ASCII:
		(*video_desc[tp->t_dev]->video_key->key_op)(tp->t_dev,
							    KEY_OP_MODE, KEY_ASCII);
		return(0);

	/*
	 *	Return the size of the screen
	 */

	case TIOCGWINSZ:
		if (m->b_cont == NULL) {
			m->b_cont = allocb(sizeof(struct winsize),
					BPRI_MED);
			if (m->b_cont) {
				m->b_cont->b_wptr +=
					sizeof(struct winsize);
				iocbp->ioc_count =
					sizeof(struct winsize);
			}
		}
		if (m->b_cont) {
			w = (struct winsize *)m->b_cont->b_rptr;
			w->ws_col = disp_font[tp->t_dev].font_maxx;
			w->ws_row = disp_font[tp->t_dev].font_maxy;
			w->ws_xpixel = video_desc[tp->t_dev]->video_scr_x;
			w->ws_ypixel = video_desc[tp->t_dev]->video_scr_y;
			m->b_datap->db_type = M_IOCACK;
		} else {
			m->b_datap->db_type = M_IOCNAK;
		}
		return(0);

  	case VIDEO_SIZE:
		if (m->b_cont) {
			freemsg(m->b_cont);
			iocbp->ioc_count = 0;
		}
		m->b_cont = allocb(2*sizeof(long), BPRI_MED);
		if (m->b_cont == NULL) 
			return(1);
		iocbp->ioc_count = 2*sizeof(long);
		m->b_cont->b_wptr += 2*sizeof(long);
		ip = (long *)m->b_cont->b_rptr;
		*ip++ = disp_font[tp->t_dev].font_maxx;
		*ip = disp_font[tp->t_dev].font_maxy;
		return(0);

	case VIDEO_MOUSE:
		mdp = video_desc[tp->t_dev]->video_mouse;
		if (disp_mouse[tp->t_dev])
			return(1);
		if ((*mdp->mouse_op)(tp->t_dev, MOUSE_OP_OPEN, 0))
			return(1);
		if ((*mdp->mouse_open)(tp->t_dev, disp_mintr, 1) == 0)
			return(1);
		disp_mouse[tp->t_dev] = 1;
		disp_mx[tp->t_dev] = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_X, 0);
		disp_my[tp->t_dev] = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_Y, 0);
		return(0);
		
	case VIDEO_NOMOUSE:
		mdp = video_desc[tp->t_dev]->video_mouse;
		if (!disp_mouse[tp->t_dev])
			return(1);
		disp_button[tp->t_dev] = 0;
		disp_mouse[tp->t_dev] = 0;
		(*mdp->mouse_close)(tp->t_dev); 
		return(0);

	case VIDEO_M_BUTTON:
		mdp = video_desc[tp->t_dev]->video_mouse;
		if (!disp_mouse[tp->t_dev])
			return(1);
		disp_button[tp->t_dev] = 1;
		disp_bval[tp->t_dev] = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_BUTTON, 0);
		return(0);

	case VIDEO_M_ALL:
		if (!disp_mouse[tp->t_dev])
			return(1);
		disp_button[tp->t_dev] = 0;
		return(0);

	case VIDEO_M_ABS:
		mdp = video_desc[tp->t_dev]->video_mouse;
		if (!disp_mouse[tp->t_dev])
			return(1);
		if (iocbp->ioc_count != 4)
			return(1);
		sp = (short *)m->b_cont->b_rptr;
		*sp++ = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_X, 0);
		*sp = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_Y, 0);
		return(0);

	case VIDEO_M_DELTA:
		mdp = video_desc[tp->t_dev]->video_mouse;
		if (!disp_mouse[tp->t_dev])
			return(1);
		if (iocbp->ioc_count != 4)
			return(1);
		sp = (short *)m->b_cont->b_rptr;
		*sp++ = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_X, 0) - disp_mx[tp->t_dev];
		*sp = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_Y, 0) - disp_my[tp->t_dev];
		disp_mx[tp->t_dev] = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_X, 0);
		disp_my[tp->t_dev] = (*mdp->mouse_op)(tp->t_dev, MOUSE_OP_Y, 0);
		return(0);

	case VIDEO_ADDR:
		if (iocbp->ioc_count != 4)
			return(1);
		ip = (long *)m->b_cont->b_rptr;
		*ip = (long)video_desc[tp->t_dev]->video_base;
		return(0);

	case VIDEO_REFRESH:
		vt100_setup(tp->t_dev, &disp_font[tp->t_dev], disp_rintr);
		video_bitmap(video_desc[tp->t_dev]);
		(*disp_font[tp->t_dev].font_clear)(&disp_font[tp->t_dev]);
		(*disp_font[tp->t_dev].font_invert)(&disp_font[tp->t_dev], 0, 0);
		return(0);

	case VIDEO_PIXSIZE:
		if (m->b_cont) {
			freemsg(m->b_cont);
			iocbp->ioc_count = 0;
		}
		m->b_cont = allocb(sizeof(struct video_size), BPRI_MED);
		if (m->b_cont == NULL) 
			return(1);
		iocbp->ioc_count = sizeof(struct video_size);
		m->b_cont->b_wptr += sizeof(struct video_size);
		((struct video_size *)(m->b_cont->b_rptr))->pix_scr_x = video_desc[tp->t_dev]->video_scr_x;
		((struct video_size *)(m->b_cont->b_rptr))->pix_scr_y = video_desc[tp->t_dev]->video_scr_y;
		((struct video_size *)(m->b_cont->b_rptr))->pix_mem_x = video_desc[tp->t_dev]->video_mem_x;
		return(0);

	case VIDEO_BELL:
		if (iocbp->ioc_count != sizeof(long))
			return(1);
		len = chorddata.countdown;
		chorddata.countdown = *(long *)m->b_cont->b_rptr;
		chord(&chorddata);
		chorddata.countdown = len;
		iocbp->ioc_count = 0;
		freemsg(unlinkb(m));
		return(0);

	case VIDEO_DATA:
		if (m->b_cont) {
			freemsg(m->b_cont);
			iocbp->ioc_count = 0;
		}
		m->b_cont = allocb(sizeof(struct video_data), BPRI_MED);
		if (m->b_cont == NULL) 
			return(1);
		iocbp->ioc_count = sizeof(struct video_data);
		m->b_cont->b_wptr += sizeof(struct video_data);
		* (struct video_data *) m->b_cont->b_rptr = video_desc[tp->t_dev]->video_data;
		return(0);

	case VIDEO_MAP:
		{
		struct video_map *vmp;
		struct video *video;
		unsigned int size;

		video = video_desc[tp->t_dev];
		if (m->b_cont) {
			freemsg(m->b_cont);
			iocbp->ioc_count = 0;
		}
		m->b_cont = allocb(sizeof(struct video_map), BPRI_MED);
		if (m->b_cont == NULL) 
			return(1);
		iocbp->ioc_count = sizeof(struct video_map);
		vmp = (struct video_map *) m->b_cont->b_wptr;
		m->b_cont->b_wptr += sizeof(struct video_map);
		if (video->video_base == 0)
			return(EINVAL);
		size = video->video_data.v_bottom * video->video_data.v_rowbytes + 
			video->video_data.v_baseoffset;
		dophys(vmp->map_physnum,vmp->map_virtaddr,size,video->video_base);
		return(u.u_error);
		}

	default:
		vp = disp_font[tp->t_dev].font_screen;
		return((*vp->video_ioctl)(vp, iocbp, m));
	}
}

/*
 *	This proc routine is simple since the screen is simple
 */

static
disp_proc(tp, cmd)
register struct ttx *tp;
{
	register int s, c, x;
	register mblk_t *m, *m1;

	s = spl1();
	switch (cmd) {

	case T_TIME:
		goto start;

	case T_WFLUSH:
		if (tp->t_xm) {
			freemsg(tp->t_xm);
			tp->t_xm = NULL;
		}

	case T_RESUME:
		tp->t_state &= ~TTSTOP;
		goto start;
	start:
		if (tp->t_q)
			qenable(WR(tp->t_q));
		break;

		/*
	 	 *	On output:
		 *		- undim the screen
		 *		- loop copying messages to the vt100
		 *		  emulator
		 *		- when empty, enable the Q to get some
		 *		  more data
		 */

	case T_OUTPUT:
		if (tp->t_state&(TIMEOUT|TTSTOP|XMT_DELAY) || !tp->t_q)
			break;
		(*disp_font[tp->t_dev].font_screen->video_func)
			(disp_font[tp->t_dev].font_screen, VF_UNDIM);
		m = tp->t_xm;
		for(;;) {
			if (m == NULL) {
				qenable(WR(tp->t_q));
				break;
			}
			if (tp->t_state&TTSTOP)
				break;
			tp->t_xm = unlinkb(m);
			SPL0();
			vt100_char(tp->t_dev, m->b_rptr, m->b_wptr-m->b_rptr);
			freeb(m);
			spl1();
			m = tp->t_xm;
		} 
		break;
			
	case T_SUSPEND:
		tp->t_state |= TTSTOP;
		break;

	case T_BLOCK:
		tp->t_state |= TBLOCK;
		goto start;

	case T_RFLUSH:
		if (m = tp->t_rm) {
			tp->t_count = tp->t_size;
			m->b_wptr = m->b_rptr;
		}
		if (!(tp->t_state&TBLOCK))
			break;
	case T_UNBLOCK:
		tp->t_state &= ~TBLOCK;
		goto start;

	case T_PARM:
		disp_param(tp);
		break;
	
	case T_BREAK:
		ttx_break(tp);
		break;
	}
	splx(s);
}

/*
 *	The param routine handles changes in baud rate etc
 *		curently this is used to change fonts (if the font tables
 *		above are set up) and to reverse video the screen 
 */

static
disp_param(tp)
register struct ttx *tp;
{
	register int dev;
	register struct fpnt *fp;
	register int i;

	dev = tp->t_dev;
	if ((disp_cflag[dev]&CBAUD) != (tp->t_cflag&CBAUD) &&
			tp->t_cflag&CBAUD) {
		if (disp_fontname[tp->t_cflag&CBAUD]) {
			fp = font_find(disp_fontname[tp->t_cflag&CBAUD], 
			       		disp_point[tp->t_cflag&CBAUD]);
		} else {
			fp = font_find("TTY", 12);
		}
		font_set(&disp_font[dev], fp, video_desc[dev],
				LEFT_SHAVE, RIGHT_SHAVE, TOP_SHAVE, BOTTOM_SHAVE);
		vt100_setup(dev, &disp_font[dev], disp_rintr);
	}
	/********************************************************************
	  --- Nuked by PKR to kill screen fuckup on CS7/CS8 shit. ---
	if ((disp_cflag[dev]&CS8) != (tp->t_cflag&CS8)) {
		if ((tp->t_cflag&CS8) == CS8) {
			disp_font[dev].font_inverse = 0;
		} else {
			disp_font[dev].font_inverse = 1;
		}
		(*disp_font[dev].font_invertall)(&disp_font[dev]);
	}
	********************************************************************/
	disp_cflag[dev] = tp->t_cflag;
}

#ifdef DISP_CONSOLE
static
dispputchar(c)
char c;
{
	register int s;
	register int i;

	s = spl7();
	if (c == '\n') {
		vt100_char(DISP_CONSOLE, "\r\n", 2);
	} else {
		vt100_char(DISP_CONSOLE, &c, 1);
	}
	splx(s);
}
#endif DISP_CONSOLE
