#ifndef lint	/* .../sys/psn/cf/bnetuconf.c */
#define _AC_NAME bnetuconf_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1987 Apple Computer, Inc., 1985-87 UniSoft Corporation, All Rights Reserved.  {Apple version 1.14 87/11/11 21:40:55}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987\tVersion 1.14 of bnetuconf.c on 87/11/11 21:40:55";
#endif		/* _AC_HISTORY */
#endif		/* lint */

#define _AC_MODS

/*	@(#)bnetuconf.c		UniPlus VVV.2.1.21	*/
#ifdef HOWFAR
extern int	T_uconfig;
#endif HOWFAR
#ifdef notdef
int 	T_cacheoff = 0;
int 	cachedelay = 0x10;
#endif notdef

/*
 * This file contains
 *	1. oem modifiable configuration personality parameters
 *	2. oem modifiable system specific kernel personality code
 */

#ifdef lint
#include "sys/sysinclude.h"
#else lint
#include "compat.h"
#include "sys/param.h"
#include "sys/uconfig.h"
#include "sys/types.h"
#include "sys/mmu.h"
#include "sys/seg.h"
#include "sys/sysmacros.h"
#include "sys/page.h"
#include "sys/systm.h"
#include "sys/map.h"
#include "sys/dir.h"
#include "sys/signal.h"
#include "sys/time.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/region.h"
#include "sys/proc.h"
#include "sys/buf.h"
#include "sys/reg.h"
#include "sys/file.h"
#include "sys/acct.h"
#include "sys/sysinfo.h"
#include "sys/var.h"
#include "sys/ipc.h"
#include "sys/shm.h"
#include "sys/termio.h"
#include "sys/utsname.h"
#include "sys/pmmu.h"
#include "vaxuba/ubavar.h"
#include "sys/debug.h"
#include "sys/ttychars.h"
#endif lint
#include "sys/via6522.h"
#include "sys/module.h"
#include "sys/gdisk.h"

char oemmsg[] =	"A/UX by Apple Computer.";

int	sspeed = B9600;		/* default console speed */
/* int	minmem = 0;		/* 2 Mbyte memory sizing limit */
int	parityno = -1;		/* parity interrupt vector (bus error) */

int	cmask = CMASK;		/* default file creation mask */
int	cdlimit = CDLIMIT;	/* default file size limit */
		/* BSD compatibility flags defaults */
int	compatflags = COMPAT_BSDPROT | COMPAT_BSDNBIO;
extern short	fp881;			/* is there an MC68881? (set in mch.s) */
short physiosize = PHYSIOSIZE;	/* size of the physiobuf (in pages) */
int runtime = RT_M20CACHE | RT_MMB;

struct uba_device ubdinit[5] = {
	/* ui_driver,	ui_unit,	ui_addr,		ui_flags */
	0,			0,			 0,		0
};

int ubdcnt = sizeof(ubdinit)/sizeof(ubdinit[0]);

#ifdef PDISK
int	pdisksize = btop(1<<18);/* size of pseudo disk */
#endif

/*
 * tty output low and high water marks
 */
#define TTHIGH
#ifdef TTLOW
#define M	1
#define N	1
#endif
#ifdef TTHIGH
#define M	3
#define N	1
#endif
int	tthiwat[16] = {
	0*M,	60*M,	60*M,	60*M,	60*M,	60*M,	60*M,	120*M,
	120*M,	180*M,	180*M,	240*M,	240*M,	240*M,	100*M,	100*M,
};
int	ttlowat[16] = {
	0*N,	20*N,	20*N,	20*N,	20*N,	20*N,	20*N,	40*N,
	40*N,	60*N,	60*N,	80*N,	80*N,	80*N,	50*N,	50*N,
};

/*
 * Default terminal characteristics
 */
char	ttcchar[NCC] = {
	0x03,
	CQUIT,
	0x7f,
	CKILL,
	CEOF,
	0,
	0,
	0
};
struct ttychars ttycdef = {
	0x7f,
	CKILL,
	0x03,
	CQUIT,
	CSTART,
	CSTOP,
	CEOF,
	CBRK,
	0xFF,	/* suspc */
	0xFF,	/* dsuspc */
	0xFF,	/* rprntc */
	0xFF,	/* flushc */
	0xFF,	/* werasc */
	0xFF,	/* lnextc */
};

/*
 * Kernel initialization functions.
 * Called from main.c while at splhi in the kernel.
 */
oem7init()
{
	register  int  (**fptr)();
	extern	int	(*init_normal[])();
	extern	int	(*init_second[])();
	extern 	int	init_normall, init_secondl;
	register int i;

#ifdef mc68881
	if (fp881)
		printf("MC68881 Floating Point Coprocessor ID %d\n", fp881);
#endif mc68881
	for (i = 0, fptr = init_second; i < init_secondl && *fptr; fptr++) {
		TRACE(T_uconfig,("init_second: %x \n",**fptr));
		(**fptr)();
	}
	for (i = 0, fptr = init_normal; i < init_normall && *fptr; fptr++) {
		TRACE(T_uconfig,("init_normal: %x \n",**fptr));
		(**fptr)();
	}
}

/*
 * Kernel initialization functions.
 * Called from main.c while at spl0 in the kernel.
 */
oem0init()
{
	register  int  (**fptr)();
	extern	int	(*init_0[])();
	extern 	int	init_0l;
	register int i;

	for (i = 0, fptr = init_0; i < init_0l && *fptr; fptr++) {
		TRACE(T_uconfig,("init_0: %x \n",**fptr));
		(**fptr)();
	}
}

#define	TIMECONST ((783360/60)/2)
/*
 * Start the system clock
 *	The vias are clocked off the "E clock frequency", which is 783.36 KHz.
 * We arrange for via2 to send out a 30 HZ square wave.  This will cause
 * an interrupt to be generated by via1 on each edge of the square wave.
 */
clkstart()
{
	register struct via *vp = (struct via *) VIA2_ADDR;

	vp->acr = VAC_T1CONT | VAC_T1PB7;
	vp->t1cl = TIMECONST;
	vp->t1ch = TIMECONST >> 8;
	((struct via *)VIA1_ADDR)->ier = VIE_CA1 | VIE_CA2| VIE_SET;
	TRACE(T_uconfig,("clkstart "));
}


#ifdef notdef
clkdelay(count)
{
	register i;

	for (i=0; i<count; i++) ;
}
#endif notdef

extern int scputchar();
int (*locputchar)() = scputchar;

/*
 * generic output to console
 */
kputchar(c)
char c;
{
	(*locputchar)(c);
}

/*
 * output to internal buffer as well as console
 */
outchar(c)
char c;
{
	putchar(c);
	kputchar(c);
}

#ifdef DEBUG
/* generic get a character from console */
getchar()
{
	return(scgetchar());
}
#endif DEBUG

#ifdef notdef
/*
 * Stop the system clock
 */
clkstop()
{

	VIA1_ADDR->vie = VIE_CLEAR | VIE_CA1;	/* Turn off interrupts */
}
#endif notdef

/*	clkreset and the onesec interrupt are hacked together as follows.
 *	The data value one_sec is only decremented when a full second has
 *	passed.  clkreset keeps it from going low.
 *	When a second has passed, we see if ticks are obviously missing
 *	from lbolt, if so, we add them.  The upshot of this code should be
 *	that lbolt time will be improved, but still marginal during long
 *	kernel critical sections, but time in seconds will be reliable.
 */

/*ARGSUSED*/
clkreset(on)
int on;
{
	viaclrius();
}

resettodr()
{
}

onesec()
{
	extern int one_sec;
	extern int lticks;
	static time_t nextlbolt;

	viaclrius();
	if(lbolt < nextlbolt) {
		lbolt = nextlbolt;
		lticks -= (nextlbolt - lbolt);
		one_sec -= (nextlbolt - lbolt);
	}
	nextlbolt = lbolt + HZ - 1;
}


/*
 * enable parity error detection
 */
parityenable()
{
}

/*
 * parityerror()
 *	Called from trap for parity error traps via
 *	interrupt level "parityno" (above).
 *	return code:
 *	- negative - fatal error - causes panic if not in
 *		user mode, or user is sent SIGBUS
 *	- zero - soft (recoverable) error
 *	- positive - number of trap that should be taken
 */
parityerror()
{
#ifdef lint
	return(0);
#else lint
	panic("parity error");	/* There is no parity on the machine */
#endif lint
}

/*
 * reboot the system
 * 	- turn off the mmu (we assume that this code is mapped 1-1 in pstart)
 *	- get the ROM stack/start pc info
 *	- cause a bus reset (which remaps the ROMs)
 *	- jump to the bootstrap start
 */

long reboot_temp[2] = {0};
 
doboot()
{
	SPLHI();

	/*
	 *	make sure the cache is turned on
	 */

	asm("		mov.l	&1, %d0");
	asm("		short	0x4e7b");		/* mov.l %d0, cacr */
	asm("		short	0x0002");

	/*
	 * turn off the MMU
	 */

	asm("		short	0xf03c, 0x4000");	/* pmove &0, tc */
	asm("		short   0x0000, 0x0000");

	/*
	 * set up the ROM entry point
	 */

	asm("		mov.l	0x40000000, %sp");
	asm("		mov.l	0x40000004, %a0");

	/*
	 * copy the final two instructions to a longword aligned location
	 */

	asm("		mov.l	&rst%, %a1");
	asm("		mov.l	&reboot_temp, %d1");
	asm("		add.l	&4, %d1");
	asm("		and.l	&0xfffffffc, %d1");
	asm("		mov.l	%d1, %a2");
	asm("		mov.l	(%a1), (%a2)");

	/*
	 * reset and jump to the ROMs (get the last 2 instructions in 1
	 *	32-bit chunk so that when the reset is done, and the ROM
	 *	mapped over the RAM, the next instruction does not need to be
	 *	fetched as it is already in the cache)
	 */

	asm("		jmp	(%a2)");
	asm("rst%:	reset");
	asm("		jmp	(%a0)");
}

/*
 * powerdown the system
 *	- Turn on PB2 in via 2 which is /POWEROFF
 */

dopowerdown()
{
	SPLHI();

	asm("		mov.l	&0x50F02000, %a0");
	asm("		bclr	&2, (%a0)");
	asm("		bset	&2, 0x400(%a0)");
	asm("pwd%:	bra	pwd%");
}

/*
 * OEM supplied subroutine called on process exit
 */

/* ARGSUSED */
oemexit(p)
struct proc *p;
{
}

/*
 * mmu error reset and report code - optional
 */
/* ARGSUSED */
mmuerror(f)
{
}

/*
 * set real time clock - optional
 */
/* ARGSUSED */
setrtc(tvar)
time_t tvar;
{
}

/*
 * dummy spurintr
 */
spurintr()
{
}

/*
 *	abort switch
 */

abintr(args)
	struct args *args;
{
	register int i;

	printf("Minimal UNIX Debugger\n");
	printf("sr = %x, pc = %x\n", args->a_ps, args->a_pc);
	printf("d0 - d7 ");
	for (i = 0; i < 7; i++)
		printf("%x ", args->a_regs[i]);
	printf("\na0 - a7 ");
	for (; i < 15; i++)
		printf("%x ", args->a_regs[i]);
	printf("\n");
}

/*
 *	Powerfail (from rear power switch)
 */

powerintr()
{
	psignal(&proc[1], SIGPWR);
}

/*
 * cache initialization
 */
cachinit()
{
	extern long m20cache;

	/* 
	 * 68020 cache 
	 * m20cach controls the enabling and disabling of processor caching
	 */
	if (runtime & RT_M20CACHE) {
		m20cache = CACHEON;
		printf("68020 ");
	} else
		m20cache = CACHEOFF;

#ifdef notdef
	/*
	 * VM04 data cache 
	 */
	*CNT4 = ((runtime & RT_VBMON)? 0xAE : 0xEE);
	if (runtime & RT_BCACHE) {
		register int i;

		for (i = cachedelay; --i > 0;) ;
		*CNT4 = ((runtime & RT_VBMON)? 0xA8 : 0xE8);
		for (i = cachedelay; --i > 0;) ;
		printf("vm04 ");
	}
#endif notdef

	if (runtime & (RT_M20CACHE|RT_BCACHE))
		printf("caching enabled\n");

	/* 
	 * current errata 
	 */
	/********** PKR
	if (runtime & (RT_A45J|RT_A92E|RT_A23G))
		printf("Mask-");

	if (runtime & RT_A45J)
		printf("A45J ");

	if (runtime & RT_A92E)
		printf("A92E ");

	if (runtime & RT_A23G)
		printf("A23G ");

	if (runtime & RT_MMB)
		printf("MMB-modify/reference ");

	if (runtime & RT_VBMON)
		printf("VERSAbus-monitior ");

	if (runtime & (RT_A45J|RT_A92E|RT_A23G|RT_MMB|RT_VBMON))
		printf("errata corrected\n");
	PKR **********/
}

clr_cache(off)
int off;
{
#ifdef notdef
	int s = spl7();
	register int i;

	if (!off && !T_cacheoff && (runtime & RT_BCACHE)) {
		*CNT4 = ((runtime & RT_VBMON)? 0xA8 : 0xE8);
		for (i = cachedelay; --i > 0;) ;
	}
	splx(s);
#else	notdef
#ifdef	lint
	off = off;
#endif 	lint
#endif notdef
}

/* generic init functions */

extern	inoinit(), binit(), cinit(), errinit(), choose_root(), iinit();
extern	shmfork(), shmexec(), shmexit();
extern	sxtinit(), flckinit(), msginit();
extern	seminit(), semexit();
#ifndef	AUTOCONFIG
extern	BNETinit();
#endif
extern	bhinit();
extern	dnlc_init();
extern	nvram_init();
#ifdef	mc68881
extern	fpnull();
#endif	mc68881

int	(*initfunc[])() = {
#ifndef	AUTOCONFIG
	BNETinit,
#endif
	clkstart,
	inoinit,
	bhinit,
	binit,
	cinit,
	dnlc_init,
	errinit,
	choose_root,
	iinit,
	nvram_init,
	msginit,
	seminit,
	flckinit,
	sxtinit,
	oem0init,
	(int (*)())0
} ;

int	(*forkfunc[11])() = {
	shmfork,
	(int (*)())0
} ;

int forksize = (sizeof(forkfunc)/sizeof(forkfunc[0])) - 1;

int	(*execfunc[11])() = {
	shmexec,
#ifdef	mc68881
	fpnull,
#endif	mc68881
	(int (*)())0
} ;

int execsize = (sizeof(execfunc)/sizeof(execfunc[0])) - 1;

int	(*exitfunc[15])() = {
	shmexit,
	semexit,
	oemexit,
	(int (*)())0
} ;

int exitsize = (sizeof(exitfunc)/sizeof(exitfunc[0])) - 1;

oem_mmuinit()
{
}

/*
 * memsize(tblend)
 *	size and clear memory starting at start
 *	sets up physmem - may exit through bus error
 *	pigco(ev):  this routine is still not totally correct
 *	because it assumes that someone else has set up the via2
 *	bits.  however, it does detect non-existent memory (you
 *	can not just write a value and read it back as that works even
 *	on non-existent memory [don't gag - it's not polite]) and also
 *	the wrap to the 'middle' of memory when bank 1 has smaller 
 *	chips than bank 0.
 */
memsize(tblend)
register int tblend;
{
	extern int physmem;
	register u_short *addr;
	register long save0;
	register long limit;
	register long save1;
	register long *mid;

	physmem = btop(tblend);
	limit = 1;
	while (limit < physmem) {
		limit <<= 1;
	}
	mid = (long *)ptob(limit>>1);
	save1 = *mid;
	save0 = *((long *)0);
	for (;;) {
		addr = (u_short *)ptob(physmem);
		addr[0] = 0x5AA5;
		addr[1] = 0xA55A;
		if (*((long *)0) != save0 || addr[0] != 0x5AA5
				|| addr[1] != 0xA55A)
			break;
		if (*mid != save1) {
			*mid = save1;
			break;
		}
		clear((caddr_t)addr, ptob(1));
		physmem++;
		if (physmem > limit) {
			mid = (long *)ptob(limit);
			save1 = *mid;
			limit <<= 1;
		}
	}
	*((long *)0) = save0;
}

/*
 *	The following three routines are support for the slots library
 */

static long oldbuserr;

slot_catch(kind, routine)
	int kind;			/* Signal type */
	long routine;			/* Routine to call */
{
	oldbuserr = *(long *)8;		/* the buserr routine */
	*(long *)8 = routine;
}

slot_ignore(kind)
	int kind;			/* Signal type */
{
	*(long *)8 = oldbuserr;
}

unsigned slot_address(slot)
	int slot;
{
	unsigned address;

	if((slot & 0xFF00000) == 0) {
	    /*
	     * The slot number passed in is just that, a slot number, and
	     * not an already valid address.
	     */
	    address = 0xf0ff0000+(slot*0x1000000);
	    return(address);
	} else {
	    return(slot);
	}
}


long	sdma_addr=SDMA_ADDR_R7, shsk_addr=SHSK_ADDR_R7, iwm_addr=IWM_ADDR_R7;

/*	rev8init -- initialize for revision 8 boards.
 *	    Three hardware addresses change in revision 8 of the mother board.
 *	Here we detect the board type, and initialize the addresses.
 */

rev8init()

{
	register struct via *vp;

	vp = (struct via *)VIA1_ADDR;
	if(!(vp->rega & VRA_REV8)) {	/* Rev 8 board */
		sdma_addr = SDMA_ADDR_R8;
		shsk_addr = SHSK_ADDR_R8;
		iwm_addr = IWM_ADDR_R8;
	}
}


/*	choose_root -- select root file system.
 *	     The booter firmware leaves us the controller and
 *	drive number of the boot disk.  We use that as the boot
 *	drive if 1) the magic number 0xffff is stored as rootdev
 *	in the kernel file, and 2) the id left is reasonable.
 *	WARNING there are constants (major number 24) specific to this release
 */

choose_root()

{
	int	newctrl, newdrive;

	if(AUTO_ADDR->auto_ctrl >= 0 && AUTO_ADDR->auto_ctrl <= 7
	  && AUTO_ADDR->auto_drive >= 0 && AUTO_ADDR->auto_drive <= 7) {
		newctrl = AUTO_ADDR->auto_ctrl + 24;
		newdrive = AUTO_ADDR->auto_drive;
	}
	else {
		newctrl = 24;
		newdrive = 0;
	}
	if(rootdev == 0xFFFF)
		rootdev = makedev(newctrl, mkgdminor(newdrive, 0));
	if(swapdev == 0xFFFF)
		swapdev = makedev(newctrl, mkgdminor(newdrive, 1));
	if(pipedev = 0xFFFF)
		pipedev = makedev(newctrl, mkgdminor(newdrive, 0));
}
