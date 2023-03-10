#ifndef lint	/* .../sys/COMMON/os/vfs_dnlc.c */
#define _AC_NAME vfs_dnlc_c
#define _AC_NO_MAIN "@(#) Copyright (c) 1987 Apple Computer, Inc., 1983-87 Sun Microsystems Inc., All Rights Reserved.  {Apple version 1.3 87/11/11 21:13:18}"
#include <apple_notice.h>

#ifdef _AC_HISTORY
  static char *sccsid = "@(#)Copyright Apple Computer 1987\tVersion 1.3 of vfs_dnlc.c on 87/11/11 21:13:18";
#endif		/* _AC_HISTORY */
#endif		/* lint */

#define _AC_MODS
/*      @(#)vfs_dnlc.c 1.1 86/02/03 Copyr 1985 Sun Micro     */  
/*      NFSSRC @(#)vfs_dnlc.c   2.2 86/05/14 */

/*
 * Copyright (c) 1985 by Sun Microsystems.
 */

#include "sys/param.h"
#include "sys/types.h"
#include "sys/sysmacros.h"
#include "sys/time.h"
#ifdef PAGING
#include "sys/mmu.h"
#include "sys/seg.h"
#include "sys/page.h"
#endif PAGING
#include "sys/systm.h"
#include "sys/vnode.h"
#include "sys/dnlc.h"
#include "sys/signal.h"
#include "sys/user.h"
#include "vax/vaxque.h"

/*
 * Directory name lookup cache.
 * Based on code originally done by Robert Els at Melbourne.
 *
 * Names found by directory scans are retained in a cache
 * for future referene.  It is managed LRU, so frequently
 * used names will hang around.  Cache is indexed by hash value
 * obtained from (vp, name) where the vp refers to the
 * directory containing the name.
 *
 * For simplicity (and economy of storage), names longer than
 * some (small) maximum length are not cached, they occur
 * infrequently in any case, and are almost never of interest.
 */

#define NC_SIZE			64	/* size of name cache */
#define	NC_HASH_SIZE		16	/* size of hash table */

#define	NC_HASH(namep, namlen, vp)	\
	((name[0] + name[namlen-1] + namlen + (int) vp) & (NC_HASH_SIZE-1))

/*
 * Macros to insert, remove cache entries from hash, LRU lists.
 */
#define	INS_HASH(ncp,nch)	insque(ncp, nch)
#define	RM_HASH(ncp)		remque(ncp)

#define	INS_LRU(ncp1,ncp2)	insque2((struct ncache *) ncp1, (struct ncache *) ncp2)
#define	RM_LRU(ncp)		remque2((struct ncache *) ncp)

#define	NULL_HASH(ncp)		(ncp)->hash_next = (ncp)->hash_prev = (ncp)

/*
 * Stats on usefulness of name cache.
 */
struct	ncstats {
	int	hits;		/* hits that we can really use */
	int	misses;		/* cache misses */
	int	long_enter;	/* long names tried to enter */
	int	long_look;	/* long names tried to look up */
	int	lru_empty;	/* LRU list empty */
	int	purges;		/* number of purges of cache */
};

/*
 * Hash list of name cache entries for fast lookup.
 */
struct	nc_hash	{
	struct	ncache	*hash_next, *hash_prev;
} nc_hash[NC_HASH_SIZE];

/*
 * LRU list of cache entries for aging.
 */
struct	nc_lru	{
	struct	ncache	*hash_next, *hash_prev;	/* hash chain, unused */
	struct 	ncache	*lru_next, *lru_prev;	/* LRU chain */
} nc_lru;

struct	ncstats ncstats;		/* cache effectiveness statistics */

struct ncache *dnlc_search();
int	doingcache = 1;

struct ncache ncache[NC_SIZE];

/*
 * Initialize the directory cache.
 * Put all the entries on the LRU chain and clear out the hash links.
 */
dnlc_init()
{
	register struct ncache *ncp;
	register int i;

	nc_lru.lru_next = (struct ncache *) &nc_lru;
	nc_lru.lru_prev = (struct ncache *) &nc_lru;
	for (i = 0; i < NC_SIZE; i++) {
		ncp = &ncache[i];
		INS_LRU(ncp, &nc_lru);
		NULL_HASH(ncp);
		ncp->dp = ncp->vp = (struct vnode *) 0;
	}
	for (i = 0; i < NC_HASH_SIZE; i++) {
		ncp = (struct ncache *) &nc_hash[i];
		NULL_HASH(ncp);
	}
}

/*
 * Add a name to the directory cahce.
 */
dnlc_enter(dp, name, vp, cred)
	register struct vnode *dp;
	register char *name;
	struct vnode *vp;
	struct ucred *cred;
{
	register int namlen;
	register struct ncache *ncp;
	register int hash;

	if (!doingcache) {
		return;
	}
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		ncstats.long_enter++;
		return;
	}
	hash = NC_HASH(name, namlen, dp);
	ncp = dnlc_search(dp, name, namlen, hash, cred);
	if (ncp != (struct ncache *) 0) {
		return;
	}
	/*
	 * Take least recently used cache struct.
	 */
	ncp = nc_lru.lru_next;
	if (ncp == (struct ncache *) &nc_lru) {	/* LRU queue empty */
		ncstats.lru_empty++;
		return;
	}
	/*
	 * Remove from LRU, hash chains.
	 */
	RM_LRU(ncp);
	RM_HASH(ncp);
	/*
	 * Drop hold on vnodes (if we had any).
	 */
	if (ncp->dp != (struct vnode *) 0) {
		VN_RELE(ncp->dp);
	}
	if (ncp->vp != (struct vnode *) 0) {
		VN_RELE(ncp->vp);
	}
	/*
	 * Hold the vnodes we are entering and
	 * fill in cache info.
	 */
	ncp->dp = dp;
	VN_HOLD(dp);
	ncp->vp = vp;
	VN_HOLD(vp);
	ncp->namlen = namlen;
	bcopy(name, ncp->name, (unsigned)namlen);
	ncp->cred = cred;
	if (cred) {
		crhold(cred);
	}
	/*
	 * Insert in LRU, hash chains.
	 */
	INS_LRU(ncp, nc_lru.lru_prev);
	INS_HASH(ncp, &nc_hash[hash]);
}

/*
 * Look up a name in the directory name cache.
 */
struct vnode *
dnlc_lookup(dp, name, cred)
	struct vnode *dp;
	register char *name;
	struct ucred *cred;
{
	register int namlen;
	register int hash;
	register struct ncache *ncp;
	register struct ncache *prev;

	if (!doingcache) {
		return ((struct vnode *) 0);
	}
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		ncstats.long_look++;
		return ((struct vnode *) 0);
	}
	hash = NC_HASH(name, namlen, dp);
	ncp = dnlc_search(dp, name, namlen, hash, cred);
	if (ncp == (struct ncache *) 0) {
		ncstats.misses++;
		return ((struct vnode *) 0);
	}
	ncstats.hits++;
	/*
	 * Move this slot to the end of LRU
	 * chain.
	 */
	RM_LRU(ncp);
	INS_LRU(ncp, nc_lru.lru_prev);
	/*
	 * If not at the head of the hash chain,
	 * move forward so will be found
	 * earlier if looked up again.
	 */
	if (ncp->hash_prev != (struct ncache *) &nc_hash[hash]) {
		/* don't assume that remque() preserves links! */
		prev = ncp->hash_prev->hash_prev;
		RM_HASH(ncp);
		INS_HASH(ncp, prev);
	}
	return (ncp->vp);
}

/*
 * Remove an entry in the directory name cache.
 */
dnlc_remove(dp, name)
	struct vnode *dp;
	register char *name;
{
	register int namlen;
	register struct ncache *ncp;
	int hash;

	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		return;
	}
	hash = NC_HASH(name, namlen, dp);
	while (ncp = dnlc_search(dp, name, namlen, hash, ANYCRED)) {
		dnlc_rm(ncp);
	}
}

/*
 * Purge the entire cache.
 */
dnlc_purge()
{
	register struct nc_hash *nch;
	register struct ncache *ncp;

	ncstats.purges++;
start:
	for (nch = nc_hash; nch < &nc_hash[NC_HASH_SIZE]; nch++) {
		ncp = nch->hash_next;
		while (ncp != (struct ncache *) nch) {
			if (ncp->dp == 0 || ncp->vp == 0) {
				panic("dnlc_purge: zero vp");
			}
			dnlc_rm(ncp);
			goto start;
		}
	}
}

#ifdef notneeded
/*
 * Purge any cache entries referencing a vnode.
 */
dnlc_purge_vp(vp)
	register struct vnode *vp;
{
	register int moretodo;
	register struct ncache *ncp;

	do {
		moretodo = 0;
		for (ncp = nc_lru.lru_next; ncp != (struct ncache *) &nc_lru;
		    ncp = ncp->lru_next) {
			if (ncp->dp == vp || ncp->vp == vp) {
				dnlc_rm(ncp);
				moretodo = 1;
				break;
			}
		}
	} while (moretodo);
}

/*
 * Purge any cache entry.
 * Called by iget when inode freelist is empty.
 */
dnlc_purge1()
{
	register struct ncache *ncp;

	for (ncp = nc_lru.lru_next; ncp != (struct ncache *) &nc_lru;
	    ncp = ncp->lru_next) {
		if (ncp->dp) {
			dnlc_rm(ncp);
			return (1);
		}
	}
	printf("dnlc_purge1: no entries to purge\n");
	return (0);
}
#endif notneeded

/*
 * Obliterate a cache entry.
 */
static
dnlc_rm(ncp)
	register struct ncache *ncp;
{
	/*
	 * Remove from LRU, hash chains.
	 */
	RM_LRU(ncp);
	RM_HASH(ncp);
	/*
	 * Release ref on vnodes.
	 */
	VN_RELE(ncp->dp);
	ncp->dp = (struct vnode *) 0;
	VN_RELE(ncp->vp);
	ncp->vp = (struct vnode *) 0;
	if (ncp->cred != NOCRED) {
		crfree(ncp->cred);
		ncp->cred = NOCRED;
	}
	/*
	 * Insert at head of LRU list (first to grab).
	 */
	INS_LRU(ncp, &nc_lru);
	/*
	 * And make a dummy hash chain.
	 */
	NULL_HASH(ncp);
}

/*
 * Utility routine to search for a cache entry.
 */
static struct ncache *
dnlc_search(dp, name, namlen, hash, cred)
	register struct vnode *dp;
	register char *name;
	register int namlen;
	int hash;
	struct ucred *cred;
{
	register struct nc_hash *nhp;
	register struct ncache *ncp;

	nhp = &nc_hash[hash];
	for (ncp = nhp->hash_next; ncp != (struct ncache *) nhp;
	    ncp = ncp->hash_next) {
		if (ncp->dp == dp && ncp->namlen == namlen &&
		    (cred == ANYCRED || ncp->cred == cred) &&
		    bcmp(ncp->name, name, (unsigned) namlen) == 0) {
			return (ncp);
		}
	}
	return ((struct ncache *) 0);
}

/*
 * Insert into queue, where the queue pointers are
 * in the second two longwords.
 * Should be in assembler like insque.
 */
static
insque2(ncp2, ncp1)
	register struct ncache *ncp2, *ncp1;
{
	register struct ncache *ncp3;

	ncp3 = ncp1->lru_next;
	ncp1->lru_next = ncp2;
	ncp2->lru_next = ncp3;
	ncp3->lru_prev = ncp2;
	ncp2->lru_prev = ncp1;
}

/*
 * Remove from queue, like insque2.
 */
static
remque2(ncp)
	register struct ncache *ncp;
{
	ncp->lru_prev->lru_next = ncp->lru_next;
	ncp->lru_next->lru_prev = ncp->lru_prev;
}
