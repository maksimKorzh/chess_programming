
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

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

int const s_argcf[] = {
	~cfE1C1,~0,		~0,		~0,		~(cfE1G1 | cfE1C1),
											~0,		~0,		~cfE1G1,
		0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,
		0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,
		0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,
		0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,
		0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,
		0,0,0,0,0,0,0,0,
	~0,		~0,		~0,		~0,		~0,		~0,		~0,		~0,
		0,0,0,0,0,0,0,0,
	~cfE8C8,~0,		~0,		~0,		~(cfE8G8 | cfE8C8),
											~0,		~0,		~cfE8G8,
		0,0,0,0,0,0,0,0,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#ifdef	NULL_MOVE

//	Making a null move does not mean doing nothing.  It means doing this.

#define	plyR	2

void VMakeNullMove(PCON pcon, PSTE pste)
{
	(pste + 1)->isqEnP = isqNIL;	// No en-passant square.
	(pste + 1)->fNull = fTRUE;		// Null move not allowed.
	(pste + 1)->plyRem =			// Reduced-depth search.
		pste->plyRem - plyR - 1;
	(pste + 1)->cf = pste->cf;		// Castling flags are passed.
	(pste + 1)->pcmFirst =			// This is like a null move-generation.
		pste->pcmFirst;
	(pste + 1)->plyFifty =			// Advance fifty-move counter.  This is a
		(pste + 1)->plyFifty + 1;	//  little strange but probably okay.
	(pste + 1)->hashk =				// Switch colors, but other than that,
		HashkSwitch(pste->hashk);	//  hash key doesn't change.
	(pste + 1)->valPcUs = pste->valPcThem;
	(pste + 1)->valPcThem = pste->valPcUs;
	(pste + 1)->valPnUs = pste->valPnThem;
	(pste + 1)->valPnThem = pste->valPnUs;
}

#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The following makes a move ("pcm") on the internal board, and sets up the
//	next move stack element ("pste + 1") to reflect the changes.

void VMakeMove(PCON pcon, PSTE pste, PCM pcm)
{
	PSQ	psqFrom;
	PSQ	psqTo;

	(pste + 1)->isqEnP = isqNIL;	// By default, no en-passant square.
#ifdef	NULL_MOVE
	(pste + 1)->fNull = fFALSE;		// By default, null-move is allowed.
#endif
	(pste + 1)->cf = pste->cf &		// Some castling flags might get turned
		s_argcf[pcm->isqFrom] &		//  off if I'm moving a king or rook.
		s_argcf[pcm->isqTo];
	//
	//	Initialize various values for the next ply.
	//
	(pste + 1)->plyFifty = pste->plyFifty + 1;
	(pste + 1)->hashk = HashkSwitch(pste->hashk);
	(pste + 1)->valPcUs = pste->valPcThem;
	(pste + 1)->valPcThem = pste->valPcUs;
	(pste + 1)->valPnUs = pste->valPnThem;
	(pste + 1)->valPnThem = pste->valPnUs;
	//
	//	This section handles cleaning up a captured piece.  The nastiest stuff
	//	is in the en-passant section.
	//
	psqTo = &pcon->argsq[pcm->isqTo];
	if (pcm->cmf & cmfCAPTURE) {
		int	isqTook;

		if (pcm->cmf & cmfMAKE_ENP) {			// This is gross.
			Assert(pste->isqEnP != isqNIL);
			isqTook = pste->isqEnP +
				((pste->coUs == coWHITE) ? -filIBD : filIBD);
			pste->ppiTook = pcon->argsq[isqTook].ppi;
			Assert(pste->ppiTook != NULL);
			Assert(pste->ppiTook->pc == pcPAWN);
			Assert(pste->ppiTook->co != pste->coUs);
			pcon->argsq[isqTook].ppi = NULL;
		} else {
			isqTook = pcm->isqTo;				// This is normal.
			pste->ppiTook = psqTo->ppi;			// This field will be used
		}										//  when it's time to unmake.
		Assert(pste->ppiTook != NULL);
		pste->ppiTook->fDead = fTRUE;			// <-- Mark it dead.
		(pste + 1)->hashk ^=					// XOR the piece out of the
			s_arghashkPc[pste->ppiTook->pc][	//  hash key.
			pste->ppiTook->co][isqTook];
		(pste + 1)->plyFifty = 0;				// Capture resets this.
		(pste + 1)->valPcUs -=
			s_argvalPcOnly[pste->ppiTook->pc];
		(pste + 1)->valPnUs -=
			s_argvalPnOnly[pste->ppiTook->pc];
	}
 	//	The next few lines move the piece, and clear out the "from" square,
	//	modify the new hash key, and deal with the fifty-move counter if this
	//	is a pawn move.
 	//
	psqFrom = &pcon->argsq[pcm->isqFrom];
	if (psqFrom->ppi->pc == pcPAWN)
		(pste + 1)->plyFifty = 0;
	psqTo->ppi = psqFrom->ppi;
	psqFrom->ppi = NULL;
	psqTo->ppi->isq = pcm->isqTo;
	(pste + 1)->hashk ^= s_arghashkPc[psqTo->ppi->pc][
		pste->coUs][pcm->isqFrom];
	(pste + 1)->hashk ^= s_arghashkPc[psqTo->ppi->pc][
		pste->coUs][pcm->isqTo];
	//
	//	The rest of the function handles dumb special cases like castling
	//	and promotion and setting the en-passant square behind a two-square
	//	pawn move.
	//
	if (pcm->cmf & cmfCASTLE) {
		int	isqRookFrom;
		int	isqRookTo;
		PSQ	psqRookFrom;
		PSQ	psqRookTo;
		PPI	ppiRook;

		switch (pcm->isqTo) {
		case isqC1:
			isqRookFrom = isqA1;
			isqRookTo = isqD1;
			break;
		case isqG1:
			isqRookFrom = isqH1;
			isqRookTo = isqF1;
			break;
		case isqC8:
			isqRookFrom = isqA8;
			isqRookTo = isqD8;
			break;
		default:
			Assert(fFALSE);
		case isqG8:
			isqRookFrom = isqH8;
			isqRookTo = isqF8;
			break;
		}
		psqRookFrom = &pcon->argsq[isqRookFrom];
		psqRookTo = &pcon->argsq[isqRookTo];
		ppiRook = psqRookFrom->ppi;
		Assert(ppiRook != NULL);
		ppiRook->isq = isqRookTo;
		Assert(psqRookTo->ppi == NULL);
		psqRookTo->ppi = ppiRook;
		psqRookFrom->ppi = NULL;
		(pste + 1)->hashk ^= s_arghashkPc[pcROOK][pste->coUs][isqRookFrom];
		(pste + 1)->hashk ^= s_arghashkPc[pcROOK][pste->coUs][isqRookTo];
	} else if (pcm->cmf & cmfPR_MASK) {
		psqTo->ppi->pc = (int)(pcm->cmf & cmfPR_MASK);
		psqTo->ppi->val = s_argvalPc[psqTo->ppi->pc];
		(pste + 1)->valPcThem += s_argvalPcOnly[psqTo->ppi->pc];
		(pste + 1)->valPnThem -= valPAWN;
	} else if (pcm->cmf & cmfSET_ENP) {
		int	isq;

		//	I'm only going to set the en-passant square if there is a pawn
		//	that could conceivably execute an en-passant capture.
		//
		isq = pcm->isqTo;
		if ((!((isq + 1) & 0x88)) &&
			(pcon->argsq[isq + 1].ppi != NULL) &&
			(pcon->argsq[isq + 1].ppi->pc == pcPAWN) &&
			(pcon->argsq[isq + 1].ppi->co != pste->coUs))
			(pste + 1)->isqEnP = isq + ((pste->coUs == coWHITE) ?
				-filIBD : filIBD);
		else if ((!((isq - 1) & 0x88)) &&
			(pcon->argsq[isq - 1].ppi != NULL) &&
			(pcon->argsq[isq - 1].ppi->pc == pcPAWN) &&
			(pcon->argsq[isq - 1].ppi->co != pste->coUs))
			(pste + 1)->isqEnP = isq + ((pste->coUs == coWHITE) ?
				-filIBD : filIBD);
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is just "VMakeMove" in reverse.  It's easier since a lot of the stuff
//	I put in "pste + 1" just gets popped off.

void VUnmakeMove(PCON pcon, PSTE pste, PCM pcm)
{
	PSQ	psqTo;
	PSQ	psqFrom;

	psqTo = &pcon->argsq[pcm->isqTo];
	Assert(psqTo->ppi != NULL);
	psqFrom = &pcon->argsq[pcm->isqFrom];
	psqTo->ppi->isq = psqFrom->isq;
	psqFrom->ppi = psqTo->ppi;
	psqTo->ppi = NULL;
	if (pcm->cmf & cmfCAPTURE) {
		pste->ppiTook->fDead = fFALSE;
		pcon->argsq[pste->ppiTook->isq].ppi = pste->ppiTook;
	}
	if (pcm->cmf & cmfCASTLE) {
		PSQ	psqRookFrom;
		PSQ	psqRookTo;
		PPI	ppiRook;

		switch (pcm->isqTo) {
		case isqC1:
			psqRookFrom = &pcon->argsq[isqA1];
			psqRookTo = &pcon->argsq[isqD1];
			break;
		case isqG1:
			psqRookFrom = &pcon->argsq[isqH1];
			psqRookTo = &pcon->argsq[isqF1];
			break;
		case isqC8:
			psqRookFrom = &pcon->argsq[isqA8];
			psqRookTo = &pcon->argsq[isqD8];
			break;
		default:
			Assert(fFALSE);
		case isqG8:
			psqRookFrom = &pcon->argsq[isqH8];
			psqRookTo = &pcon->argsq[isqF8];
			break;
		}
		ppiRook = psqRookTo->ppi;
		ppiRook->isq = psqRookFrom->isq;
		psqRookFrom->ppi = ppiRook;
		psqRookTo->ppi = NULL;
	} else if (pcm->cmf & cmfPR_MASK) {
		pcon->argsq[pcm->isqFrom].ppi->pc = pcPAWN;
		pcon->argsq[pcm->isqFrom].ppi->val = s_argvalPc[pcPAWN];
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
