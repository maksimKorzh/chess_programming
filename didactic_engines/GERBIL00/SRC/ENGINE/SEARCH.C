
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

//	This will be used to figure out which move to search first.  It is very
//	important to try to search the best move first, since this will produce
//	a huge reduction in tree size.

//	Best moves are typically winning captures and moves that have been shown
//	to be good here in the past (PV move and hash table move).

void VSort(PSTE pste, PCM pcm)
{
	PCM	pcmI;
	PCM	pcmBest = pcm;
	CM	cm;
	
	for (pcmI = pcm + 1; pcmI < (pste + 1)->pcmFirst; pcmI++)
		if (pcmI->cmk > pcmBest->cmk)
			pcmBest = pcmI;
	cm = *pcm;
	*pcm = *pcmBest;
	*pcmBest = cm;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#ifdef	NULL_MOVE

//	This does a null-move search to see if the side not to move has a threat.
//	If they can't drive the score above alpha, even if they get to move twice
//	in a row, their positions probably sucks, and we'll prune this variation.

//	It's not called in endgames, because it has problems detecting zugzwang.

BOOL FThreat(PCON pcon, PSTE pste)
{
	int	val;

	if ((pste->plyRem <= 0) || (pste->fNull))
		return fTRUE;
	VMakeNullMove(pcon, pste);
	(pste + 1)->valAlpha = -pste->valBeta;
	(pste + 1)->valBeta = -pste->valBeta + 1;
	if ((pste + 1)->plyRem < 0)
		val = -ValSearchQ(pcon, pste + 1);
	else
		val = -ValSearch(pcon, pste + 1);
	if (val >= pste->valBeta)
		return fFALSE;
	return fTRUE;
}

#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Quiescent search.  This lets captures play out at the end of the search,
//	so I don't end up aborting a line at a stupid place.

//	The quiescent search doesn't see checks, so it will not detect checkmates,
//	and might actually end up making decisions based upon an illegal line.

//	This is one reason I don't record the PV while in quiescent search.

int ValSearchQ(PCON pcon, PSTE pste)
{
	PCM	pcm;
	int	valQuies;

	pste->ccmPv = 0;
	//
	//	The interface callback must be called now and then.
	//
	if (++pcon->nodes >= pcon->nodesNext) {
		pcon->nodesNext = pcon->nodes + 2000;	// 2000 nodes is maybe 1/50
		VPrCallback((void *)pcon);				//  to 1/100 of a second.
		VCheckTime(pcon);
		if ((pcon->fAbort) || (pcon->fTimeout))
			return 0;
	}
	//	See if static eval will cause a cutoff or raise alpha.
	//
	valQuies = ValEval(pcon, pste);
	if (valQuies > pste->valAlpha) {
		if (valQuies >= pste->valBeta)
			return pste->valBeta;
		pste->valAlpha = valQuies;
	}
	VGenMoves(pcon, pste, fTRUE);
	//
	//	Iterate through capturing moves, to see if I can improve upon alpha
	//	(which may be the static eval).
	//
	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
		int	val;

		if (pcm - pste->pcmFirst <= 4)
			VSort(pste, pcm);
		VMakeMove(pcon, pste, pcm);
		(pste + 1)->plyRem = pste->plyRem - 1;	// This line is not strictly
												//  necessary.
		(pste + 1)->valAlpha = -pste->valBeta;
		(pste + 1)->valBeta = -pste->valAlpha;
		val = -ValSearchQ(pcon, pste + 1);
		VUnmakeMove(pcon, pste, pcm);
		if ((pcon->fAbort) || (pcon->fTimeout))
			return 0;
		if (val > pste->valAlpha) {
			if (val >= pste->valBeta)
				return pste->valBeta;
			pste->valAlpha = val;
		}
	}
	return pste->valAlpha;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These wo functions look for specific moves in the move list, in order to
//	increase their sort key so they will sort first.

void VFindPvCm(PSTE pste)
{
	PCM	pcm;
	
	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++)
		if ((pcm->isqFrom == pste->cmPv.isqFrom) &&
			(pcm->isqTo == pste->cmPv.isqTo) &&
			(pcm->cmf == pste->cmPv.cmf)) {
			pcm->cmk |= cmkPV;
			return;
		}
}

void VFindHash(PSTE pste, PHASH phash)
{
	PCM	pcm;
	
	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++)
		if ((pcm->isqFrom == phash->isqFrom) &&
			(pcm->isqTo == phash->isqTo) &&
			((int)(pcm->cmf & cmfPR_MASK) == phash->pcTo)) {
			pcm->cmk |= cmkHASH;
			return;
		}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Full-width search.

int ValSearch(PCON pcon, PSTE pste)
{
	PCM	pcm;
	PCM	pcmBest;
	BOOL	fFound;
	int	hashf = hashfALPHA;
	int	val;

	pcon->nodes++;
	//
	//	Return draw score if this is a repeated node.
	//
	if ((pste != pcon->argste) && (FRepSet(pcon, pste->hashk))) {
		pste->ccmPv = 0;
		return 0;
	}
	//	Check hash table, in order to get "best" moves, and try to cut off.
	//
	if ((FProbeHash(pcon, pste, &val)) && (pste > pcon->argste + 1)) {
		pste->ccmPv = 0;
		return val;
	}
	//	Mark this move so if I find it in a sub-tree I can detect a rep draw.
	//
	VSetRep(pcon, pste->hashk);
#ifdef	NULL_MOVE
	//
	//	Do null-move search, which is a reduced depth that tries to figure out
	//	if the opponent has no threats.
	//
	if ((!pste->fInCheck) && (pste != pcon->argste) &&
		(pste->valPcUs > valROOK) && (!FThreat(pcon, pste))) {
		VClearRep(pcon, pste->hashk);
		return pste->valBeta;
	}
#endif
	//	Generate moves, and mark the PV and hash moves so they sort first.
	//
	VGenMoves(pcon, pste, fFALSE);
	if (pste->cmPv.isqFrom != isqNIL) {
		VFindPvCm(pste);
		pste->cmPv.isqFrom = isqNIL;
	}
	if (pste->phashAlways != NULL)
		VFindHash(pste, pste->phashAlways);
	if (pste->phashDepth != NULL)
		VFindHash(pste, pste->phashDepth);
	Assert(pste->plyRem >= 0);
	pcmBest = NULL;
	fFound = fFALSE;
	//
	//	Iterate through the move list, trying everything out.
	//
	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
		//
		//	For the first few moves, I'm going to use the sort key attached
		//	to the moves in order to try to get a good one.  If I don't find
		//	a fail-high within the first few, I'm going to assume that I don't
		//	have a clue how to order these moves, so I'll just take them all
		//	in natural order.
		//
		if (pcm - pste->pcmFirst <= 4)
			VSort(pste, pcm);
		VMakeMove(pcon, pste, pcm);
		//
		//	I'm going to discard moves that leave me in check.  This could be
		//	handled, perhaps more cheaply, elsewhere, but this is a sensible
		//	way to do this.
		//
		if (FAttacked(pcon, pcon->argpi[pste->coUs][0].isq, pste->coUs ^ 1)) {
			VUnmakeMove(pcon, pste, pcm);
			continue;
		}
		//	I know how many reversible moves have been made up until now.  I
		//	just tried to make another move.  If I am here, I didn't leave
		//	myself in check, so I succeeded.
		//
		//	If the number of reversible moves made before this is at least 50
		//	for each side, this was a draw.
		//
		//	Imagine a "one-move rule".  If I play Nf3 Nc6, that is not an
		//	immediate draw, because Nc6 might be mate.  So the reason I am
		//	here instead of doing this 50-move check in some other more
		//	obvious place is that I'm looking for any legal move, so I know
		//	that I wasn't mated on the 50th move.
		//
		if ((pste->plyFifty >= 50 * 2) && (pste != pcon->argste)) {
			pste->ccmPv = 0;
			VUnmakeMove(pcon, pste, pcm);
			VClearRep(pcon, pste->hashk);
			return 0;
		}
		//	Set up for recursion.
		//
		(pste + 1)->fInCheck = FAttacked(pcon,
			pcon->argpi[pste->coUs ^ 1][0].isq, pste->coUs);
		(pste + 1)->plyRem = (pste + 1)->fInCheck ?
			pste->plyRem : pste->plyRem - 1;
		(pste + 1)->valAlpha = -pste->valBeta;
		(pste + 1)->valBeta = -pste->valAlpha;
		//
		//	Recurse into normal search or quiescent search, as appropriate.
		//
		if ((pste + 1)->plyRem < 0)
			val = -ValSearchQ(pcon, pste + 1);
		else
			val = -ValSearch(pcon, pste + 1);
		VUnmakeMove(pcon, pste, pcm);
		if ((pcon->fAbort) || (pcon->fTimeout)) {
			VClearRep(pcon, pste->hashk);
			return 0;
		}
		if (val > pste->valAlpha) {
			if (val >= pste->valBeta) {
				//
				//	This move failed high, so we are going to return beta,
				//	but if we're sitting at the root of the search I will set
				//	up a one-move PV (if this isn't already the move I'm
				//	planning to make), so this move will be made if I run out
				//	of time before resolving the fail-high.
				//
				if (pste == pcon->argste) {
					if (memcmp(pcm, pste->argcmPv, sizeof(CM))) {
						pste->argcmPv[0] = *pcm;
						pste->ccmPv = 1;
					}
					VDumpPv(pcon, pcon->plyDepth, val, '+');
				}
				VRecordHash(pcon, pste, pcm, pste->valBeta, hashfBETA);
				VClearRep(pcon, pste->hashk);
				return pste->valBeta;
			}
			//	This move is between alpha and beta, which is actually pretty
			//	rare.  If this happens I have to add a PV move, and append the
			//	returned PV to it, and if I'm at the root I'll send the PV to
			//	the interface so it can display it.
			//
			memcpy(pste->argcmPv + 1, (pste + 1)->argcmPv,
				(pste + 1)->ccmPv * sizeof(CM));
			pste->argcmPv[0] = *pcm;
			pste->ccmPv = (pste + 1)->ccmPv + 1;
			pste->valAlpha = val;
			if (pste == pcon->argste)
				VDumpPv(pcon, pcon->plyDepth, val, '\0');
			hashf = hashfALPHA | hashfBETA;
			pcmBest = pcm;
		}
		fFound = fTRUE;
	}
	VClearRep(pcon, pste->hashk);
	if (!fFound) {
		pste->ccmPv = 0;
		if (pste->fInCheck) {
			VRecordHash(pcon, pste, pcmBest, -valMATE + 500, hashfALPHA);
			return -valMATE + (pste - pcon->argste);
		}
		VRecordHash(pcon, pste, pcmBest, 0, hashfALPHA | hashfBETA);
		return 0;
	}
	VRecordHash(pcon, pste, pcmBest, pste->valAlpha, hashf);
	return pste->valAlpha;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The "Think" function is an important part of the engine.  The engine
//	thread spends most of its time in here.

//	The basic idea is that it will sit in a loop and call the search function
//	over and over, each time with one more ply of depth, until the search
//	runs out of time.  This is called "iterative deepening".  One advantage of
//	this is is that this is easily interruptable, since the first few plies
//	take so little time that you'll always have *some* move to make, even if
//	you interrupt after a few milliseconds.  

//	The most important reason to do iterative deepening is that it's faster
//	than just calling the search with some larger final depth.  The reason is
//	that the PV from the previous iteration is passed along to the subsequent
//	iteration.  The moves in this PV are searched first.  The previous PV is
//	probably an excellent line, and alpha-beta is much more efficient if you
//	search a good line first, so you end up searching a smaller tree.

//	After the search has timed out, it will pass the best move to the
//	interface.  If the PV was two moves or more long, it will then go into
//	"ponder" mode, which means that it will execute the second move in the PV
//	on its internal board, and go into an infinite duration think about this
//	move.  This is called "pondering".

//	The the opponent makes the move the program is guessing that they'll make,
//	the engine will seamlessly switch from "ponder" mode to "thinking" mode,
//	as if the opponent really did make their move immediately after the
//	engine's last move.

//	It's possible that the opponent may have been thinking a long time on
//	their move, and by the time they are done, the engine will have already
//	used up its normal time allocation for this move.  If that's the case,
//	the engine will move instantly.

//	If the opponent doesn't make the move the engine is guessing, the search
//	will be aborted and a new search started.

//	Aborting incorrect pondering, and switching from ponder into think mode if
//	the engine guesses the right move, aren't a part of this function, but it
//	makes some sense to explain this here.

#define	plyMAX	120	// Maximum number of iterations.  This seems huge, but it
					//  is sometimes reached.

void VThink(PCON pcon, TM tmRemaining)
{
	Assert(pcon->smode == smodeIDLE);
	//
	//	Set up time-out, and remind myself that I'm not dead.
	//
	pcon->tmRemaining = tmRemaining;
	pcon->fAbort = fFALSE;
	//
	//	Analysis mode does a clear hash table for now, until Tim adds and
	//	explicit "reset" command, hopefully in protover 3.
	//
	if (tmRemaining == tmANALYZE) {
		pcon->smode = smodeANALYZE;
		VClearHashe();
	} else
		pcon->smode = smodeTHINK;
	//
	//	This loop is going to execute at least once.  It might execute for the
	//	whole game, if the program continuously ponders correctly.
	//
	for (;;) {
		char	aszMove[32];
		char	aszPonder[32];
		char	aszSanHint[32];
		int	i;
		SAN	sanHint;
		CM	cmHint;
		CM	cm;
		int	valCur;					// Value returned by search.
		int	valLow;					// Alpha-beta search window.
		int	valHigh;
		BOOL	f;


		pcon->fTimeout = fFALSE;
		pcon->tmStart = TmNow();	// Record when I started.
		if (pcon->smode ==			// If I'm pondering, I don't set the
			smodeTHINK)				//  time control stuff.  This will be
			VSetTime(pcon);			//  handled at the point it's clear that
									//  I picked the correct move.
		pcon->argste[0].ccmPv = 0;	// Clear PV.
		VSetRepHash(pcon);			// Pump game history into rep hash.
		VDrawCheck(pcon);			// Try to claim appropriate 50-move and
									//  3x rep draws before examining moves.
		//
		//	During the first 20 moves of the game I will try to find a book
		//	move.  If I find one I will play it.
		//
		if ((pcon->smode == smodeTHINK) && (pcon->fUseBook) &&
			(pcon->gc.ccm < 40) && (FBookMove(pcon, &cm))) {
			char	asz[16];
			BOOL	f;

			VCmToSz(&cm, asz);	// Make book move.
			f = FAdvance(pcon, asz);
			Assert(f);
			VPrSendMove(asz);	// I am not going to bother checking for draws
								//  and mates that occur while in book.  If by
								//  weird chance one of these happens, if
								//  we're hooked up to the chess server, the
								//  server might call it.
			break;
		}
		//	Increment sequence number.  The transposition hash system will
		//	overwrite nodes from old searches, and this is how it knows that
		//	this is a new search, rather than just more of the old one.
		//
		VIncrementHashSeq();
		//
		//	Search the position via iterative deepening.
		//
		pcon->nodes = 0;		// Global node counter.
		pcon->nodesNext = 2000;	// Time (in nodes) of next callback to
								//  interface.
		valCur = 0;
		valLow = valMIN;		// Initial window is wide open.
		valHigh = valMAX;
		for (i = 0; i < plyMAX; i++)  {
			pcon->argste[0].valAlpha = valLow;
			pcon->argste[0].valBeta = valHigh;
			pcon->argste[0].plyRem = i;
			pcon->plyDepth = i + 1;
			VSetPv(pcon);	// This remembers the current PV so that the
							//  search can search its elements first.
			valCur = ValSearch(pcon, &pcon->argste[0]);
			if (pcon->fAbort)
				break;
			if (pcon->fTimeout)
				break;
			//
			//	This checks for a result outside the window, and if it finds
			//	one it re-searches with the window wide open.
			//
			if ((valCur <= valLow) || (valCur >= valHigh)) {
				if (valCur <= valLow)	// Record a fail-low (fail-high is
										//  handled inside "ValSearch").
					VDumpPv(pcon, pcon->plyDepth, valCur, '-');
				pcon->argste[0].valAlpha = valMIN;
				pcon->argste[0].valBeta = valMAX;
				VSetPv(pcon);
				valCur = ValSearch(pcon, &pcon->argste[0]);
				if (pcon->fAbort)
					break;
				if (pcon->fTimeout)
					break;
			}
			//	Mated now or this move mates, drop out of here.
			//
			if ((valCur == valMATE - 1) || (valCur == -valMATE))
				break;
			//
			//	Depth-restricted search.  Check for "timeout", meaning target
			//	depth met.
			//
			if (((pcon->smode == smodeTHINK) ||
				(pcon->smode == smodePONDER)) &&
				(pcon->tc.tct == tctFIXED_DEPTH) &&
				(pcon->plyDepth >= pcon->tc.plyDepth)) {
				pcon->fTimeout = fTRUE;
				break;
			}
			//	Set up for next iteration, which will use a window centered
			//	around the current position value.
			//
			valLow = valCur - 50;
			valHigh = valCur + 50;
		}
		//	If abort or doing pondering, I'm done, because I don't need to
		//	make any moves or set up the next search.
		//
		if ((pcon->fAbort) || (pcon->smode != smodeTHINK))
			break;
		//
		//	If there is a move in the first element of the PV, I can make it.
		//	I don't make it now, because I want to look at the second element
		//	of the PV for a pondering move, and if I execute the first move,
		//	it will destroy the PV.
		//
		aszMove[0] = aszPonder[0] = '\0';
		if (pcon->argste[0].ccmPv >= 1)
			VCmToSz(&pcon->argste[0].argcmPv[0], aszMove);
		if ((pcon->argste[0].ccmPv >= 2) && (pcon->fPonder)) {
			VCmToSz(&pcon->argste[0].argcmPv[1], aszPonder);
			cmHint = pcon->argste[0].argcmPv[1];
		}
		//	As of this point, I have move to make in "aszMove", move to ponder
		//	in "aszPonder".  If I can't move, or I can't ponder, these strings
		//	are nulled out.
		//
		//	Check to see if I'm mated or stalemated, which will be the case
		//	if I can't move.
		//
		if (aszMove[0] == '\0') {
			if ((!pcon->fTimeout) && (valCur == -valMATE)) {
				VMates(pcon->argste[0].coUs ^ 1);
				break;
			}
			VPrSendResult("1/2-1/2", "Stalemate");
			break;
		}
		//	Make the move.
		//
		f = FAdvance(pcon, aszMove);
		Assert(f);
		VPrSendMove(aszMove);
		//
		//	The score from the last search might indicate that this is mate.
		//
		if ((!pcon->fTimeout) && (valCur == valMATE - 1)) {
			VMates(pcon->argste[0].coUs ^ 1);
			break;
		}
		//	Check for 50-move or 3x repetition draws.
		//
		VDrawCheck(pcon);
		//
		//	Check to see if I stalemated my opponent, and report the result as
		//	appropriate.
		//
		if (FStalemated(pcon))
			VPrSendResult("1/2-1/2", "Stalemate");
		//
		//	If I can't ponder, I'm done, otherwise set up pondering and loop
		//	back around.
		//
		if (aszPonder[0] == '\0')
			break;
		//
		//	Get the algebraic so I can send a hint.
		//
		VCmToSan(pcon, pcon->argste, &cmHint, &sanHint);
		VSanToSz(&sanHint, aszSanHint);
		strcpy(pcon->aszPonder, aszPonder);
		//
		//	Execute the move.  I'm now one move ahead of the game, which is
		//	tricky.  After that, send the hint move.
		//
		f = FAdvance(pcon, aszPonder);
		Assert(f);
		VPrSendHint(aszSanHint);
		pcon->smode = smodePONDER;
	}
	//	If I broke out, and I'm in ponder mode, back up the search and pretend
	//	that I never pondered.  I could do some sneaky stuff having to do with
	//	remembering the move that would have been made if the pondered search
	//	had been converted into a normal search soon enough, but this would be
	//	annoying to do and it's kind of rare and pointless.
	//
	if (pcon->smode == smodePONDER)
		VUndoMove(pcon);
	pcon->smode = smodeIDLE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
