
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

//#define	QUIESCENT_PRUNING	// This was an attempt to prune crap out of
								//  MVV/LVA.  Maybe it works, maybe not.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The values in the arrays correspond to the number of squares you have to
//	move in order to go in a particular "chess-useful" direction.

//	For instance, on a 16 x 8 board, 16 is up one rank, and 17 is up one rank
//	and to the right one file.

int	const s_argvecKnight[] = { 14, 31, 33, 18, -14, -31, -33, -18, 0 };
int	const s_argvecBishop[] = { 15, 17, -15, -17, 0 };
int	const s_argvecRook[] = { -1, 1, 16, -16, 0 };
int	const s_argvecQueen[] = { -1, 1, 16, -16, 15, 17, -15, -17, 0 };

int const * s_argpvecPiece[] = {
	NULL,			// Pawn isn't handled through here.
	s_argvecKnight,
	s_argvecBishop,
	s_argvecRook,
	s_argvecQueen,
	s_argvecQueen,	// King and queen use the same ones.
};

//	The first two values are pawn captures, the last is a single-space move.

int	const s_argvecWhitePawn[] = { 15, 17, 16, };	// <-- Ordering important.
int	const s_argvecBlackPawn[] = { -15, -17, -16, };	// <-- Ordering important.

int const * s_argpvecPawn[] = {
	s_argvecWhitePawn,
	s_argvecBlackPawn,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This stuff helps avoid messy computation when dealing with castling, which
//	is already messy enough.

typedef	struct	tagCSTL {
	int	cf;			// Castling flag that's being handled here.
	int	isqRookTo;	// Where the rook goes.
	int	isqKnight;	// Where the knight started out.
	int	isqKingTo;	// Where the king goes.
}	CSTL;

CSTL const c_argcstl[coMAX][2] = {
	cfE1G1,	isqF1,	isqG1,	isqG1,
	cfE1C1,	isqD1,	isqB1,	isqC1,
	cfE8G8,	isqF8,	isqG8,	isqG8,
	cfE8C8,	isqD8,	isqB8,	isqC8,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This generates moves and adds them to the move list starting at
//	"pste->pcmFirst".  If "fCapsOnly" is set, this performs as a quiescent
//	capture generator.  This could be broken into two functions for speed,
//	but the extra code is not worth it in this simple application.
//
//	Captures are ordered by MVV/LVA, meaning that the sort key is:
//
//		VALUE(captured piece) - INDEX(capturing piece).
//
//	This means that PxK gets the highest value, and KxP gets the lowest.
//
//	MVV/LVA kind of sucks a bit.  If a static-exchange evaluator is written,
//	and pruning is done based upon the result of the static exchange test, the
//	number of nodes processed per ply will drop dramatically, with little or
//	no loss in effective strength per ply.

void VGenMoves(PCON pcon, PSTE pste, BOOL fCapsOnly)
{
	PCM	pcm = pste->pcmFirst;
	int	ipi;
#ifdef	QUIESCENT_PRUNING
	int	valMin;
#endif

	Assert(pcm + 500 <			// Dummy check to see if I'm getting close to
		pcon->argcm + ccmMAX);	//  blowing out the move list.
	//
	//	In captures-only mode I'm going to do some pruning I haven't heard
	//	described along with MVV/LVA.  If a capture can't possibly drive the
	//	material score to within 100 centipawns of alpha, prune it out.
	//
#ifdef	QUIESCENT_PRUNING
	valMin = (fCapsOnly) ? pste->valAlpha - (pste->valPcUs + pste->valPnUs -
		pste->valPcThem - pste->valPnThem) - 100 : 0;
#endif
	for (ipi = 0; ipi < pcon->argcpi[pste->coUs]; ipi++) {
		PPI	ppiFrom;
		int	isqFrom;
		
		ppiFrom = &pcon->argpi[pste->coUs][ipi];
		if (ppiFrom->fDead)	// Ignore pieces that don't exist anymore.
			continue;
		isqFrom = ppiFrom->isq;
		switch (ppiFrom->pc) {
			int	i;
			int const * pvec;
			int	isqTo;
			PPI	ppiTo;

		case pcPAWN:
			//
			//	Pawn captures, including en-passant.
			//
			pvec = s_argpvecPawn[ppiFrom->co];
			for (i = 0; i < 2; i++)		// Handle two captures first.
				if (!((isqTo = isqFrom + *pvec++) & 0x88))
					if ((ppiTo = pcon->argsq[isqTo].ppi) == NULL) {
						//
						//	En-passant case.
						//
						if (pste->isqEnP == isqTo) {
							pcm->isqFrom = isqFrom;
							pcm->isqTo = isqTo;
							pcm->cmk = cmkCAPTURE | (valPAWN - pcPAWN);
							pcm++->cmf = cmfMAKE_ENP | cmfCAPTURE;
						}
					} else if (ppiTo->co != pste->coUs)
						if ((isqTo >= isqA8) || (isqTo <= isqH1)) {
							int	pc;

							//	Promotion + capturing case.
							//
							for (pc = pcQUEEN; pc >= pcKNIGHT; pc--) {
								pcm->isqFrom = isqFrom;
								pcm->isqTo = isqTo;
								pcm->cmk = cmkCAPTURE | (ppiTo->val - pcPAWN);
								pcm++->cmf = cmfCAPTURE | pc;
							}
#ifdef	QUIESCENT_PRUNING
						} else if (ppiTo->val >= valMin) {
#else
						} else {
#endif
							//
							//	Regular capture.
							//
							pcm->isqFrom = isqFrom;
							pcm->isqTo = isqTo;
							pcm->cmk = cmkCAPTURE | (ppiTo->val - pcPAWN);
							pcm++->cmf = cmfCAPTURE;
						}
			if (fCapsOnly)
				break;
			//
			//	Non-capturing pawn-advance.
			//
			isqTo = isqFrom + *pvec;
			if (pcon->argsq[isqTo].ppi == NULL)
				if ((isqTo >= isqA8) || (isqTo <= isqH1)) {
					int	pc;

					//	Non-capturing pawn promotion.
					//
					for (pc = pcQUEEN; pc >= pcKNIGHT; pc--) {
						pcm->isqFrom = isqFrom;
						pcm->isqTo = isqTo;
						pcm->cmk = (pc == pcQUEEN) ? cmkQUEEN : cmkNONE;
						pcm++->cmf = pc;
					}
				} else {
					//
					//	This is the normal single-step move.
					//
					pcm->isqFrom = isqFrom;
					pcm->isqTo = isqTo;
					pcm->cmk = cmkNONE;
					pcm++->cmf = cmfNONE;
					//
					//	Check for double-step on first move.
					//
					if (((pste->coUs == coWHITE) &&
						(ppiFrom->isq <= isqH2)) ||
						((pste->coUs == coBLACK) &&
						(ppiFrom->isq >= isqA7))) {
						isqTo += *pvec;
						if (pcon->argsq[isqTo].ppi == NULL) {
							pcm->isqFrom = isqFrom;
							pcm->isqTo = isqTo;
							pcm->cmk = cmkNONE;
							pcm++->cmf = cmfSET_ENP;
						}
					}
				}
			break;
		case pcKING:
			//
			//	The king has some special castling code, after which it falls
			//	into the knight code.
			//
			//	It's assumed that if a castling flag is on, the king is on its
			//	original square, and there is a rook on the right square, too.
			//
			//	This code checks to see if other appropriate squares are
			//	empty.  It does *not* check to see if the king move puts us
			//	into check.  It *does* check to see if the king move crossed
			//	check, which is kind of expensive, but that's life.  It could
			//	check elsewhere, but you can make *that* change.
			//
			if ((!pste->fInCheck) && (!fCapsOnly))
				for (i = 0; i < 2; i++)
					if (pste->cf & c_argcstl[ppiFrom->co][i].cf) 
						if ((pcon->argsq[c_argcstl[
							ppiFrom->co][i].isqRookTo].ppi == NULL) &&
							(pcon->argsq[c_argcstl[
							ppiFrom->co][i].isqKnight].ppi == NULL) &&
							(pcon->argsq[c_argcstl[
							ppiFrom->co][i].isqKingTo].ppi == NULL) &&
							(!FAttacked(pcon, c_argcstl[ppiFrom->co][
							i].isqRookTo, ppiFrom->co ^ 1))) {
							pcm->isqFrom = isqFrom;
							pcm->isqTo = c_argcstl[ppiFrom->co][i].isqKingTo;
							pcm->cmk = cmkNONE;
							pcm++->cmf = cmfCASTLE;
						}
		case pcKNIGHT:	// No "break" statement above.
			//
			//	Knight is a non-sliding piece.  The king also drops into this
			//	code when it gets done messing around with castling.
			//
			for (pvec = s_argpvecPiece[ppiFrom->pc]; *pvec; pvec++)
				if (!((isqTo = isqFrom + *pvec) & 0x88))
					if ((ppiTo = pcon->argsq[isqTo].ppi) == NULL) {
						if (!fCapsOnly) {
							pcm->isqFrom = isqFrom;
							pcm->isqTo = isqTo;
							pcm->cmk = cmkNONE;
							pcm++->cmf = cmfNONE;
						}
#ifdef	QUIESCENT_PRUNING
					} else if ((ppiTo->co != ppiFrom->co) &&
						(ppiTo->val >= valMin)) {
#else
					} else if (ppiTo->co != ppiFrom->co) {
#endif
						pcm->isqFrom = isqFrom;
						pcm->isqTo = isqTo;
						pcm->cmk = cmkCAPTURE | (ppiTo->val - ppiFrom->pc);
						pcm++->cmf = cmfCAPTURE;
					}
			break;
		default:
			Assert(fFALSE);	// Corrupt data check.
		case pcBISHOP:
		case pcROOK:
		case pcQUEEN:
			//
			//	Sliding piece.  This is all there is to it.
			//
			for (pvec = s_argpvecPiece[ppiFrom->pc]; *pvec; pvec++)
				for (isqTo = isqFrom + *pvec; !(isqTo & 0x88); isqTo += *pvec)
					if ((ppiTo = pcon->argsq[isqTo].ppi) == NULL) {
						if (!fCapsOnly) {
							pcm->isqFrom = isqFrom;
							pcm->isqTo = isqTo;
							pcm->cmk = cmkNONE;
							pcm++->cmf = cmfNONE;
						}
					} else {
#ifdef	QUIESCENT_PRUNING
						if ((ppiTo->co != ppiFrom->co) &&
							(ppiTo->val >= valMin)) {
#else
						if (ppiTo->co != ppiFrom->co) {
#endif
							pcm->isqFrom = isqFrom;
							pcm->isqTo = isqTo;
							pcm->cmk = cmkCAPTURE |
								(ppiTo->val - ppiFrom->pc);
							pcm++->cmf = cmfCAPTURE;
						}
						break;
					}
			break;
		}
	}
	(pste + 1)->pcmFirst = pcm;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
