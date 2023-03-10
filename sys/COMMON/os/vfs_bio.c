#ifndef lint	/* .../sys/COMMON/os/vfs_bio.c */
#define _AC_NAME vfs_bio_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1987 Apple Computer, Inc., 1983-87 Sun Microsystems Inc., All Rights Reserved.  {Apple version 1.5 87/11/18 11:07:52}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987\tVersion 1.5 of vfs_bio.c on 87/11/18 11:07:52";
#endif		/* _AC_HISTORY */
#endif		/* lint */

#define _AC_MODS
/*      @(#)vfs_bio.c 1.1 86/02/03 SMI  */
/*      NFSSRC @(#)vfs_bio.c    2.1 86/04/15 */

#include "sys/types.h"
#include "sys/mmu.h"
#include "sys/sysmacros.h"
#include "sys/param.h"
#include "sys/time.h"
#include "sys/seg.h"
#ifdef PAGING
#include "sys/page.h"
#include "sys/region.h"
#endif PAGING
#include "sys/systm.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "sys/errno.h"
#include "sys/var.h"
#include "sys/buf.h"
#include "sys/conf.h"
#include "sys/proc.h"
#include "sys/trace.h"
#include "sys/vnode.h"
#include "sys/sysinfo.h"

extern int T_rz;
#include "sys/debug.h"

/*
 * Read in (if necessary) the block and return a buffer pointer.
 */
struct buf *
bread(vp, blkno, size)
	struct vnode *vp;
	daddr_t blkno;
	int size;
{
	register struct buf *bp;

	TRACE(T_rz, ("read bp = 0x%x\n", bp));
	if (size == 0)
		panic("bread: size 0");
	sysinfo.lread++;
	bp = getblk(vp, blkno, size);
	if (bp->b_flags&B_DONE) {
		trace(TR_BREADHIT, vp, blkno);
		return (bp);
	}
	bp->b_flags |= B_READ;
	if (bp->b_bcount > bp->b_bufsize)
		panic("bread");
	VOP_STRATEGY(bp);
	u.u_ior++;
	sysinfo.bread++;
	trace(TR_BREADMISS, vp, blkno);
	biowait(bp);
	return (bp);
}

/*
 * Read in the block, like bread, but also start I/O on the
 * read-ahead block (which is not allocated to the caller)
 */
struct buf *
breada(vp, blkno, size, rablkno, rabsize)
	struct vnode *vp;
	daddr_t blkno; int size;
	daddr_t rablkno; int rabsize;
{
	register struct buf *bp, *rabp;

	bp = NULL;
	/*
	 * If the block isn't in core, then allocate
	 * a buffer and initiate i/o (getblk checks
	 * for a cache hit).
	 */
	if (!incore(vp, blkno)) {
		sysinfo.lread++;
		bp = getblk(vp, blkno, size);
		if ((bp->b_flags&B_DONE) == 0) {
			bp->b_flags |= B_READ;
			if (bp->b_bcount > bp->b_bufsize)
				panic("breada");
			VOP_STRATEGY(bp);
			u.u_ior++;
			sysinfo.bread++;
			trace(TR_BREADMISS, vp, blkno);
		} else
			trace(TR_BREADHIT, vp, blkno);
	}

	/*
	 * If there's a read-ahead block, start i/o
	 * on it also (as above).
	 */
	if (rablkno && !incore(vp, rablkno)) {
		rabp = getblk(vp, rablkno, rabsize);
		if (rabp->b_flags & B_DONE) {
			brelse(rabp);
			trace(TR_BREADHITRA, vp, blkno);
		} else {
			rabp->b_flags |= B_READ|B_ASYNC;
			if (rabp->b_bcount > rabp->b_bufsize)
				panic("breadrabp");
			VOP_STRATEGY(rabp);
			u.u_ior++;
			sysinfo.bread++;
			trace(TR_BREADMISSRA, vp, rablock);
		}
	}

	/*
	 * If block was in core, let bread get it.
	 * If block wasn't in core, then the read was started
	 * above, and just wait for it.
	 */
	if (bp == NULL)
		return (bread(vp, blkno, size));
	biowait(bp);
	return (bp);
}

/*
 * Write the buffer, waiting for completion.
 * Then release the buffer.
 */
bwrite(bp)
	register struct buf *bp;
{
	register flag;

	TRACE(T_rz, ("write bp = 0x%x\n", bp));
	sysinfo.lwrite++;
	flag = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	trace(TR_BWRITE, bp->b_vp, bp->b_blkno);
	if (bp->b_bcount > bp->b_bufsize)
		panic("bwrite");
	VOP_STRATEGY(bp);
	u.u_iow++;
	sysinfo.bwrite++;

	/*
	 * If the write was synchronous, then await i/o completion.
	 * If the write was "delayed", then we put the buffer on
	 * the q of blocks awaiting i/o completion status.
	 */
	if ((flag&B_ASYNC) == 0) {
		biowait(bp);
		brelse(bp);
	} else if (flag & B_DELWRI)
		bp->b_flags |= B_AGE;
}

/*
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * This can't be done for magtape, since writes must be done
 * in the same order as requested.
 */
bdwrite(bp)
	register struct buf *bp;
{
	sysinfo.lwrite++;
#ifdef notdef
	/*
	 * This does not work for buffers associated with
	 * vnodes that are remote - they have no dev.
	 * Besides, we don't use bio with tapes, so rather
	 * than develop a fix, we just ifdef this out for now.
	 */
	flags = bdevsw[major(bp->b_dev)].d_flags;
	if (flags & B_TAPE)
		bawrite(bp);
	else {
		bp->b_flags |= B_DELWRI | B_DONE;
		brelse(bp);
	}
#endif
	bp->b_flags |= B_DELWRI | B_DONE;
	brelse(bp);
}

/*
 * Release the buffer, start I/O on it, but don't wait for completion.
 */
bawrite(bp)
	register struct buf *bp;
{

	bp->b_flags |= B_ASYNC;
	bwrite(bp);
}

/*
 * Release the buffer, with no I/O implied.
 */
brelse(bp)
	register struct buf *bp;
{
	register struct buf *flist;
	register s;



	/*
	 * If someone's waiting for the buffer, or
	 * is waiting for a buffer wake 'em up.
	 */
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	if (bfreelist[0].b_flags&B_WANTED) {
		bfreelist[0].b_flags &= ~B_WANTED;
		wakeup((caddr_t)bfreelist);
	}
	if (bp->b_flags & B_NOCACHE)
		bp->b_flags |= B_INVAL;
	if (bp->b_flags&B_ERROR)
		if (bp->b_flags & B_LOCKED)
			bp->b_flags &= ~B_ERROR;	/* try again later */
		else
			brelvp(bp);

	/*
	 * Stick the buffer back on a free list.
	 */
	s = spl6();
	if (bp->b_bufsize <= 0) {
		/* block has no buffer ... put at front of unused buffer list */
		flist = &bfreelist[BQ_EMPTY];
		binsheadfree(bp, flist);
	} else if (bp->b_flags & (B_ERROR|B_INVAL)) {
		/* block has no info ... put at front of most free list */
		flist = &bfreelist[BQ_AGE];
		binsheadfree(bp, flist);
	} else {
		if (bp->b_flags & B_LOCKED)
			flist = &bfreelist[BQ_LOCKED];
		else if (bp->b_flags & B_AGE)
			flist = &bfreelist[BQ_AGE];
		else
			flist = &bfreelist[BQ_LRU];
		binstailfree(bp, flist);
	}
	bp->b_flags &= ~(B_WANTED|B_BUSY|B_ASYNC|B_AGE|B_NOCACHE);
	(void) splx(s);
}

/*
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada)
 */
incore(vp, blkno)
	struct vnode *vp;
	daddr_t blkno;
{
	register struct buf *bp;
	register struct buf *dp;

	dp = BUFHASH(vp, blkno);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
		if (bp->b_blkno == blkno && bp->b_vp == vp &&
		    (bp->b_flags & B_INVAL) == 0)
			return (1);
	return (0);
}

struct buf *
baddr(vp, blkno, size)
	struct vnode *vp;
	daddr_t blkno;
	int size;
{

	if (incore(vp, blkno))
		return (bread(vp, blkno, size));
	return (0);
}

/*
 * Assign a buffer for the given block.  If the appropriate
 * block is already associated, return it; otherwise search
 * for the oldest non-busy buffer and reassign it.
 *
 * We use splx here because this routine may be called
 * on the interrupt stack during a dump, and we don't
 * want to lower the ipl back to 0.
 */
struct buf *
getblk(vp, blkno, size)
	struct vnode *vp;
	daddr_t blkno;
	int size;
{
	register struct buf *bp, *dp;
	int s;

	/*
	 * Search the cache for the block.  If we hit, but
	 * the buffer is in use for i/o, then we wait until
	 * the i/o has completed.
	 */
	dp = BUFHASH(vp, blkno);
loop:
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno != blkno || bp->b_vp != vp ||
		    bp->b_flags&B_INVAL)
			continue;
		s = spl6();
		if (bp->b_flags&B_BUSY) {
			bp->b_flags |= B_WANTED;
			syswait.iowait++;
			(void) sleep((caddr_t)bp, PRIBIO+1);
			syswait.iowait--;
			(void) splx(s);
			goto loop;
		}
		(void) splx(s);
		notavail(bp);
		if (brealloc(bp, size) == 0)
			goto loop;
		bp->b_flags |= B_CACHE;
		return (bp);
	}
#ifdef BDEVCHECK
	if (major(vp->v_rdev) >= bdevcnt)
		panic("blkdev");
#endif BDEVCHECK
	bp = getnewbuf();
	bfree(bp);
	bremhash(bp);
	binshash(bp, dp);
	bsetvp(bp, vp);
	bp->b_dev = vp->v_rdev;
	bp->b_blkno = blkno;
	bp->b_error = 0;
	bp->b_resid = 0;
	if (brealloc(bp, size) == 0)
		goto loop;
	return (bp);
}

/*
 * get an empty block,
 * not assigned to any particular device
 */
struct buf *
geteblk(size)
	int size;
{
	register struct buf *bp, *flist;

loop:
	bp = getnewbuf();
	bp->b_flags |= B_INVAL;
	bfree(bp);
	bremhash(bp);
	flist = &bfreelist[BQ_AGE];
	binshash(bp, flist);
	brelvp(bp);
	bp->b_error = 0;
	bp->b_resid = 0;
	if (brealloc(bp, size) == 0)
		goto loop;
	return (bp);
}

/*
 * Allocate space associated with a buffer.
 * If can't get space, buffer is released
 */
brealloc(bp, size)
	register struct buf *bp;
	int size;
{
	daddr_t start, last;
	register struct buf *ep;
	struct buf *dp;
	int s;

	/*
	 * First need to make sure that all overlaping previous I/O
	 * is dispatched with.
	 */
	if (size == bp->b_bcount)
		return (1);
	if (size < bp->b_bcount) { 
		if (bp->b_flags & B_DELWRI) {
			bwrite(bp);
			return (0);
		}
		if (bp->b_flags & B_LOCKED)
			panic("brealloc");
		return (allocbuf(bp, size));
	}
	bp->b_flags &= ~B_DONE;
	if (bp->b_vp == (struct vnode *) 0)
		return (allocbuf(bp, size));

	/*
	 * Search cache for any buffers that overlap the one that we
	 * are trying to allocate. Overlapping buffers must be marked
	 * invalid, after being written out if they are dirty. (indicated
	 * by B_DELWRI) A disk block must be mapped by at most one buffer
	 * at any point in time. Care must be taken to avoid deadlocking
	 * when two buffer are trying to get the same set of disk blocks.
	 */
	start = bp->b_blkno;
	last = start + btod(size) - 1;
	dp = BUFHASH(bp->b_vp, bp->b_blkno);
loop:
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if (ep == bp || ep->b_vp != bp->b_vp || (ep->b_flags&B_INVAL))
			continue;
		/* look for overlap */
		if (ep->b_bcount == 0 || ep->b_blkno > last ||
		    ep->b_blkno + btod(ep->b_bcount) <= start)
			continue;
		s = spl6();
		if (ep->b_flags&B_BUSY) {
			ep->b_flags |= B_WANTED;
			(void) sleep((caddr_t)ep, PRIBIO+1);
			(void) splx(s);
			goto loop;
		}
		(void) splx(s);
		notavail(ep);
		if (ep->b_flags & B_DELWRI) {
			bwrite(ep);
			goto loop;
		}
		ep->b_flags |= B_INVAL;
		brelse(ep);
	}
	return (allocbuf(bp, size));
}

/*
 * Find a buffer which is available for use.
 * Select something from a free list.
 * Preference is to AGE list, then LRU list.
 */
struct buf *
getnewbuf()
{
	register struct buf *bp, *dp;
	int s;

loop:
	s = spl6();
	for (dp = &bfreelist[BQ_AGE]; dp > bfreelist; dp--)
		if (dp->av_forw != dp)
			break;
	if (dp == bfreelist) {          /* no free blocks */
		dp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)dp, PRIBIO+1);
		(void) splx(s);
		goto loop;
	}
	(void) splx(s);
	bp = dp->av_forw;
	notavail(bp);
	if (bp->b_flags & B_DELWRI) {
		bp->b_flags |= B_ASYNC;
		bwrite(bp);
		goto loop;
	}
	brelvp(bp);
	trace(TR_BRELSE, bp->b_vp, bp->b_blkno);
	bp->b_flags = B_BUSY;
	return (bp);
}

/*
 * Wait for I/O completion on the buffer; return errors
 * to the user.
 */
biowait(bp)
	register struct buf *bp;
{
	int s;

	syswait.iowait++;
	s = spl6();
	while ((bp->b_flags&B_DONE)==0)
#ifdef	PAGING
		if (u.u_procp->p_flag & SPAGEIN)
			/*
			 * If faulting in a page, then set PCATCH to make sure 
			 * we don't resume on u_qsav.  If pagein was successful,
			 * we check for signals before returning to user mode.
			 */
			(void) sleep((caddr_t)bp, PRIBIO|PCATCH);
		else
#endif	PAGING
			(void) sleep((caddr_t)bp, PRIBIO);
	(void) splx(s);
	syswait.iowait--;
	if (u.u_error == 0)			/* XXX */
		u.u_error = geterror(bp);
}

/*
 * Mark I/O complete on a buffer.
 * If someone should be called, e.g. the pageout
 * daemon, do so.  Otherwise, wake up anyone
 * waiting for it.
 */
biodone(bp)
	register struct buf *bp;
{

	TRACE(T_rz, ("biodone on bp=0x%x\n", bp));
	if (bp->b_flags & B_DONE)
		panic("dup biodone");
	bp->b_flags |= B_DONE;
	if (bp->b_flags & B_CALL) {
		bp->b_flags &= ~B_CALL;
		(*bp->b_iodone)(bp);
		return;
	}
	if (bp->b_flags&B_ASYNC)
		brelse(bp);
	else {
		bp->b_flags &= ~B_WANTED;
		wakeup((caddr_t)bp);
	}
}

/*
 * Insure that no part of a specified block is in an incore buffer.
 */
blkflush(vp, blkno, size)
	struct vnode *vp;
	daddr_t blkno;
	long size;
{
	register struct buf *ep;
	struct buf *dp;
	daddr_t start, last;
	int s;

	start = blkno;
	last = start + btod(size) - 1;
	dp = BUFHASH(vp, blkno);
loop:
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if (ep->b_vp != vp || (ep->b_flags&B_INVAL))
			continue;
		/* look for overlap */
		if (ep->b_bcount == 0 || ep->b_blkno > last ||
		    ep->b_blkno + btod(ep->b_bcount) <= start)
			continue;
		s = spl6();
		if (ep->b_flags&B_BUSY) {
			ep->b_flags |= B_WANTED;
			(void) sleep((caddr_t)ep, PRIBIO+1);
			(void) splx(s);
			goto loop;
		}
		if (ep->b_flags & B_DELWRI) {
			(void) splx(s);
			notavail(ep);
			bwrite(ep);
			goto loop;
		}
		(void) splx(s);
	}
}

/*
 *	Make sure all write-behind blocks associated with vp
 *	are flushed out. (from fsync)
 *
 *	Note:	changes here attempt to avoid the O(n**2) performance that can
 *		make sync's performance in systems with lots of buffers degrade.
 *		The idea is to read the list and cache a group of blocks that
 *		need flushing, then go back over the group flushing them to
 *		their queues. The reason why you have to it this way is because
 *		it is conceivable that a device's strategy routine can sleep.
 */

#define SYNCBSIZE	40

bflush(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register struct buf *flist;
	register int s, i, ind;
	struct buf *buflist[SYNCBSIZE];

loop:
	ind = 0;
	s = spl6();
	for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++) {
	for (bp = flist->av_forw; bp != flist; bp = bp->av_forw) {
		if ((bp->b_flags & B_DELWRI) == 0)
			continue;
		if (vp == bp->b_vp || vp == (struct vnode *) 0) {
			buflist[ind] = bp;
			ind++;
			if (ind >= SYNCBSIZE)
				goto out;
		}
	}  }
out:
	if (ind) {
		for (i = 0; i < ind; i++) {
			bp = buflist[i];
			if ((bp->b_flags & B_DELWRI) == 0)
				continue;
			if (vp == bp->b_vp || vp == (struct vnode *) 0) {
				bp->b_flags |= B_ASYNC;
				notavail(bp);
				(void) splx(s);
				bwrite(bp);
				(void) spl6();
			}
		}
		(void) splx(s);
		goto loop;
	}
	(void) splx(s);
}

/*
 * Invalidate blocks associated with vp which are on the freelist.
 * Make sure all write-behind blocks associated with vp are flushed out.
 */
binvalfree(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register struct buf *flist;
	int s;

loop:
	s = spl6();
	for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++)
	for (bp = flist->av_forw; bp != flist; bp = bp->av_forw) {
		if (vp == bp->b_vp || vp == (struct vnode *) 0) {
			if (bp->b_flags & B_DELWRI) {
				bp->b_flags |= B_ASYNC;
				notavail(bp);
				(void) splx(s);
				bwrite(bp);
			} else {
				brelvp(bp);
				bp->b_flags |= B_INVAL;
				(void) splx(s);
			}
			goto loop;
		}
	}
	(void) splx(s);
}

/*
 * Pick up the device's error number and pass it to the user;
 * if there is an error but the number is 0 set a generalized
 * code.  Actually the latter is always true because devices
 * don't yet return specific errors.
 */
geterror(bp)
	register struct buf *bp;
{
	int error = 0;

	if (bp->b_flags&B_ERROR)
		if ((error = bp->b_error)==0)
			return (EIO);
	return (error);
}

/*
 * Invalidate in core blocks belonging to closed or umounted filesystem
 *
 * This is not nicely done at all - the buffer ought to be removed from the
 * hash chains & have its dev/blkno fields clobbered, but unfortunately we
 * can't do that here, as it is quite possible that the block is still
 * being used for i/o. Eventually, all disc drivers should be forced to
 * have a close routine, which ought ensure that the queue is empty, then
 * properly flush the queues. Until that happy day, this suffices for
 * correctness.						... kre
 * This routine assumes that all the buffers have been written.
 */
binval(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register struct bufhd *hp;
#define dp ((struct buf *)hp)

loop:
	for (hp = bufhash; hp < &bufhash[v.v_hbuf]; hp++)
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
			if (bp->b_vp == vp && 
			   (bp->b_flags & (B_INVAL | B_DONE)) == B_DONE) {
				brelvp(bp);
				bp->b_flags |= B_INVAL;
				goto loop;
			}
}

bsetvp(bp, vp)
	struct buf *bp;
	struct vnode *vp;
{
	if (bp->b_vp) {
		brelvp(bp);
	}
	VN_HOLD(vp);
	bp->b_vp = vp;
}

brelvp(bp)
	struct buf *bp;
{
	if (bp->b_vp == (struct vnode *) 0) {
		return;
	}
	VN_RELE(bp->b_vp);
	bp->b_vp = (struct vnode *) 0;
}

/*
 * Get the major device number given a strategy routine address
 */
getmajor(strat)
register int (*strat)();
{
	register struct bdevsw *bdp;
	register i;

	bdp = &bdevsw[0]; 
	for (i = 0; i < bdevcnt; i++, bdp++)
		if (bdp->d_strategy == strat)
			return(i);
	panic("getmajor");
	/* NOTREACHED */
}
