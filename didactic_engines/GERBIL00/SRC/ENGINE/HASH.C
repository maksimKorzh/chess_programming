
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Gerbil
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	This file is part of the Gerbil chess program project.
//
//	Gerbil is free software; you can redistribute it and/or modify it under
//	the terms of the GNU General Public License as published by the Free
//	Software Foundation; either version 2 of the License, or (at your option)
//	any later version.
//
//	Gerbil is distributed in the hope that it will be useful, but WITHOUT ANY
//	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//	details.
//
//	You should have received a copy of the GNU General Public License along
//	with Gerbil; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include "engine.h"
#include "protocol.h"
#include <stdlib.h>

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This module has two basic functions.  It manages the repetition hash
//	table, and it manages the main transposition hash table.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These functions affect the repetition-check hash table, which is a small
//	table designed to hold all the moves in the current line I'm thinking
//	about, plus all the moves that have already been played on the board.

//	The hash table must be messed with in first-in/last-out order.  If you
//	hash element A, then you hash element B, then you remove element A, it is
//	not guaranteed that you will be able to find element B anymore, which is
//	very bad.  You must remove element B before element A.  Alpha-beta works
//	in this order naturally, so no problem.

//	If you mess up and leave orphan elements in this table while doing a
//	search, you risk a crash at best, and hard to find day-long bugs at worst.

//	This table must be a power of two in size.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This checks to see if this hash key is already represented in the hash
//	table, which indicates that a repetition has taken place.

BOOL FRepSet(PCON pcon, HASHK hashk)
{
	unsigned	ihashk;
	
	ihashk = (unsigned)hashk & (chashkREP - 1);
	for (;;) {
		if (pcon->arghashkRep[ihashk] == hashk)
			return fTRUE;
		if (!pcon->arghashkRep[ihashk])
			return fFALSE;
		ihashk = (ihashk + 1) & (chashkREP - 1);
	}
}

//	This sticks a hash key in the table.

void VSetRep(PCON pcon, HASHK hashk)
{
	unsigned	ihashk;

	ihashk = (unsigned)hashk & (chashkREP - 1);
	for (;;) {
		if (!pcon->arghashkRep[ihashk]) {
			pcon->arghashkRep[ihashk] = hashk;
			return;
		}
		ihashk = (ihashk + 1) & (chashkREP - 1);
	}
}

//	This removes a hash key from the table.

void VClearRep(PCON pcon, HASHK hashk)
{
	unsigned	ihashk;

	ihashk = (unsigned)hashk & (chashkREP - 1);
	for (;;) {
		if (pcon->arghashkRep[ihashk] == hashk) {
			pcon->arghashkRep[ihashk] = 0;
			return;
		}
		Assert(pcon->arghashkRep[ihashk]);
		ihashk = (ihashk + 1) & (chashkREP - 1);
	}
}

//	This function is called before the search is started, in order to
//	initialize the repetition table and prime it with positions that have
//	occured in the game.

//	The function only inserts an element if a position has already occured
//	twice.  You could change this to insert every unique position, but this
//	would result in some behaviors I think are stupid.  Some people like to
//	do it that way, but I don't.

//	If you leave it my way, sometimes the program will repeat a position.
//	This has some negative consequences, like you stand a higher chance of
//	getting a 50-move draw.

void VSetRepHash(PCON pcon)
{
	int	i;

	memset(pcon->arghashkRep, 0, sizeof(pcon->arghashkRep));
	for (i = 0; i <= pcon->gc.ccm; i++) {
		int	j;
		
		for (j = i + 1; j <= pcon->gc.ccm; j++)
			if (pcon->gc.arghashk[i] == pcon->gc.arghashk[j])
				if (!FRepSet(pcon, pcon->gc.arghashk[i]))
					VSetRep(pcon, pcon->gc.arghashk[i]);
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These functions deal with the transposition hash table.  The transposition
//	hash table is very large, and is actually two tables.

//	The first table operates via a "replace always" scheme.  If you try to
//	insert an element into the table, it always is inserted.

//	The second table operates via a "replace if deeper or more recent search"
//	replacement scheme.  Every time the "think" function is called, a sequence
//	number is incremented.  If you try to insert an element into th table, it
//	is inserted if the sequence number now is different than the one
//	associated with the element in the table.  If the sequence numbers are the
//	same, an element will be inserted only if the depth associated with the
//	element you are trying to add is greater than the depth associated with
//	the element already here.

//	This two table scheme is commonly known, although I found out about it via
//	an email from Ken Thompson in 1994 or so.  His idea is that deep searches
//	can stick around for a while, while stuff that is very recent can also
//	stay around, even if it is shallow.

//	His system also allowed for repetitions to be detected through this
//	main hash table, but I find that using a smaller table specifically for
//	that results in less complexity and (hopefully) fewer bugs.

//	If you make changes to the transposition hash, you should test the program
//	with Fine #70:

//	8/k/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1

//	The best move is Kb1 (a1b1), which should be found with a decent positive
//	score after a couple of seconds, or perhaps less, depending upon how
//	efficient this program ends up getting.  If you've broken the
//	transposition hash system, you won't solve this problem in sensible time.

//	It is very easy to screw up the transposition table hash, and it's
//	possible that this implementation has bugs already.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The hash table.

RGHASHD s_rghashd;
unsigned	s_chashdMac;
int	s_seq;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Given a hash element, this checks to see if conditions are ripe for a
//	cutoff.  In order to produce a cutoff, the element has to record a
//	search that's deep enough.  If so, an "exact" element always returns
//	a cutoff.  An "alpha" or "beta" element only returns a cutoff if it can
//	be proven that the score associated with it would result in a cutoff
//	given the current alpha or beta.

BOOL FCutoff(PCON pcon, PSTE pste, PHASH phash, int * pval)
{
	if (pste->plyRem <= phash->plyRem)
		if (phash->hashf & hashfALPHA) {
			if (phash->hashf & hashfBETA) {
				*pval = phash->val;
				return fTRUE;
			}
			if (phash->val <= pste->valAlpha) {
				*pval = pste->valAlpha;
				return fTRUE;
			}
		} else if (phash->hashf & hashfBETA) {
			if (phash->val >= pste->valBeta) {
				*pval = pste->valBeta;
				return fTRUE;
			}
		}
	return fFALSE;
}

//	This checks both hash tables, looking primarily for an element that
//	causes a cutoff.  If neither element can produce a cutoff, I'll check
//	to see if either of them has a "best" move associated with it.  If so,
//	I'll remember the element, so I can increase the candidate move key (cmk)
//	for the move after I generate moves.

//	A "best" move is indicated by the "isqFrom" field being something other
//	than "isqNIL".  If it is "isqNIL", the other two move fields ("isqTo" and
//	"pcTo") are undefined.

//	It's very possible to have two different hash elements for this position,
//	each with a different best move.  If both elements have the same move,
//	I'll only remember one of them.

BOOL FProbeHash(PCON pcon, PSTE pste, int * pval)
{
	PHASHD	phashd = &s_rghashd[pste->hashk & s_chashdMac];
	
	pste->phashDepth = NULL;
	pste->phashAlways = NULL;
	if (phashd->hashAlways.hashk == pste->hashk) {
		if (FCutoff(pcon, pste, &phashd->hashAlways, pval))
			return fTRUE;
		if (phashd->hashAlways.isqFrom != isqNIL)
			pste->phashAlways = &phashd->hashAlways;
	}
	if (phashd->hashDepth.hashk == pste->hashk) {
		if (FCutoff(pcon, pste, &phashd->hashDepth, pval))
			return fTRUE;
		if ((phashd->hashDepth.isqFrom != isqNIL) &&
			((pste->phashAlways == NULL) ||
			(phashd->hashDepth.isqFrom != phashd->hashAlways.isqFrom) ||
			(phashd->hashDepth.isqTo != phashd->hashAlways.isqTo) ||
			(phashd->hashDepth.pcTo != phashd->hashAlways.pcTo)))
			pste->phashDepth = &phashd->hashDepth;
	}
	return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tries to update both hash tables with the results of this search.

//	A fine point here is that I don't overwrite the "best" move in an element
//	with a null move if there was already a "best" move in this element, as
//	long as the hash key in here is the key for the current position.

//	Meaning, if I found that this position was >=50 at some time past, I might
//	have a "best" move in here, which is the move that generated the score of
//	>=50.  Later on if I come back with a higher window, and I find that the
//	move is <=75 (a fail low), I won't have a best move from that search,
//	because none of the moves worked.  I'll keep the old best move in the
//	element, because it's possible that it's still best, and in any case it's
//	a good guess.

void VRecordHash(PCON pcon, PSTE pste, PCM pcm, int val, int hashf)
{
	PHASHD	phashd;

	phashd = &s_rghashd[pste->hashk & s_chashdMac];
	//
	//	Replace "always replace" element.
	//
	phashd->hashAlways.val = val;
	if (pcm != NULL) {
		phashd->hashAlways.isqFrom = pcm->isqFrom;
		phashd->hashAlways.isqTo = pcm->isqTo;
		phashd->hashAlways.pcTo = pcm->cmf & cmfPR_MASK;
	} else if (phashd->hashAlways.hashk != pste->hashk)
		phashd->hashAlways.isqFrom = isqNIL;
	phashd->hashAlways.hashk = pste->hashk;
	phashd->hashAlways.hashf = hashf;
	phashd->hashAlways.plyRem = pste->plyRem;
	//
	//	Replace "replace if deeper or more recent search" element only if
	//	the appropriate criteria are met.
	//
	if ((s_seq != phashd->hashDepth.seq) ||
		(phashd->hashDepth.plyRem < pste->plyRem)) {
		phashd->hashDepth.val = val;
		if (pcm != NULL) {
			phashd->hashDepth.isqFrom = pcm->isqFrom;
			phashd->hashDepth.isqTo = pcm->isqTo;
			phashd->hashDepth.pcTo = pcm->cmf & cmfPR_MASK;
		} else if (phashd->hashDepth.hashk != pste->hashk)
			phashd->hashDepth.isqFrom = isqNIL;
		phashd->hashDepth.hashk = pste->hashk;
		phashd->hashDepth.hashf = hashf;
		phashd->hashDepth.plyRem = pste->plyRem;
		phashd->hashDepth.seq = s_seq;
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VIncrementHashSeq(void)
{
	s_seq++;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Zero all hash memory.

void VClearHashe(void)
{
	memset(s_rghashd, 0, (s_chashdMac + 1) * sizeof(HASHD));
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This inits the main transposition hash table.  As of now this function
//	uses a constant for the table size, but this could easily be changed to
//	allow a variable.

//	The table must be a power of two in size or things will break elsewhere,
//	because I don't use "key % size" to get the element, I use
//	"key & (size - 1)", which only works with a power of two.

BOOL FInitHashe(PCON pcon)
{
	int	chashdMax;

	chashdMax = 1;
	for (;;) {
		if (chashdMax * 2 * sizeof(HASHD) > pcon->cbMaxHash)
			break;
		chashdMax *= 2;
	}
	if ((s_rghashd = malloc(chashdMax * sizeof(HASHD))) == NULL) {
		VPrSendToIface("Can't allocate hash memory: %d bytes",
			chashdMax * sizeof(HASHD));
		return fFALSE;
	}
	s_chashdMac = chashdMax - 1;
	VClearHashe();
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
