
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
#include <ctype.h>

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Once the first STE record is primed, this cleans up a few details in the
//	record, and sets up the rest of the array properly.

void VFixSte(PCON pcon)
{
	int	i;

	pcon->argste[0].pcmFirst = pcon->argcm;
#ifdef	NULL_MOVE
	pcon->argste[0].fNull = fFALSE;
#endif
	pcon->argste[0].fInCheck = FAttacked(pcon,
		pcon->argpi[pcon->argste[0].coUs][0].isq, pcon->argste[0].coUs ^ 1);
	for (i = 1; i < csteMAX; i++)
		pcon->argste[i].coUs = pcon->argste[0].coUs ^ (i & 1);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Inits context ("pcon") from FEN string.  Returns FALSE if the position
//	is illegal.

//	"VUndoMove" makes a few assumptions about how this routine works.  If you
//	go messing with the "pcon->gc" info in here, check that routine to make
//	sure you don't break it.

BOOL FInitCon(PCON pcon, char const * szFen)
{
	int	rnk;
	int	fil;
	int	i;
	int	argcKing[coMAX];
	int	argvalPc[coMAX];
	int	argvalPn[coMAX];
	PSTE	pste = pcon->argste;

	strcpy(pcon->gc.aszFen, szFen);	// Remember FEN.
	pcon->argcpi[coWHITE] = 0;		// Number of pieces per side.
	pcon->argcpi[coBLACK] = 0;
	for (i = 0; i < csqMAX; i++) {	// Clear board.
		pcon->argsq[i].ppi = NULL;
		pcon->argsq[i].isq = i;
	}
	pste->hashk = 0;		// Going to XOR the pieces into this.
	argvalPc[coWHITE] = argvalPc[coBLACK] = 0;
	argvalPn[coWHITE] = argvalPn[coBLACK] = 0;
	//
	//	Fill in pcon from FEN string.  Part one of the FEN is piece locations,
	//	so this fills in the board and the piece list.
	//
	fil = filA;
	rnk = rnk8;
	argcKing[coWHITE] = argcKing[coBLACK] = 0;
	for (;; szFen++) {
		if (*szFen == ' ') {
			szFen++;
			break;
		}
		switch (*szFen) {
			PPI	ppi;
			int	pc;
			int	co;

		default:			// Includes '\0' (end of input).
			return fFALSE;
		case 'p':
			pc = pcPAWN;
			co = coBLACK;
lblSetPawn:	if ((rnk == rnk1) ||			// Pawn in a stupid place.
				(rnk == rnk8))
				return fFALSE;
lblSet:		if (pcon->argcpi[co] >= cpiMAX)	// Too many pieces of this color.
				return fFALSE;
			if (rnk < rnk1)					// Ran off the board.
				return fFALSE;
			if (fil > filH)
				return fFALSE;
			ppi = &pcon->argpi[co][pcon->argcpi[co]++];
			//
			//	Patch king so it's at the front of the list.
			//
			if ((pc == pcKING) && (pcon->argcpi[co] > 1)) {
				PI pi = pcon->argpi[co][0];

				*ppi = pi;
				pcon->argsq[ppi->isq].ppi = ppi;
				ppi = &pcon->argpi[co][0];
			}
			//	Create new piece.
			//
			ppi->pc = pc;
			ppi->co = co;
			ppi->fDead = fFALSE;
			ppi->isq = IsqFromRnkFil(rnk, fil);
			ppi->val = s_argvalPc[pc];
			argvalPc[co] += s_argvalPcOnly[pc];
			argvalPn[co] += s_argvalPnOnly[pc];
			pcon->argsq[ppi->isq].ppi = ppi;
			pste->hashk ^= s_arghashkPc[pc][co][ppi->isq];
			fil++;
			break;
		case 'P':
			pc = pcPAWN;
			co = coWHITE;
			goto lblSetPawn;
		case 'n':
			pc = pcKNIGHT;
			co = coBLACK;
			goto lblSet;
		case 'N':
			pc = pcKNIGHT;
			co = coWHITE;
			goto lblSet;
		case 'b':
			pc = pcBISHOP;
			co = coBLACK;
			goto lblSet;
		case 'B':
			pc = pcBISHOP;
			co = coWHITE;
			goto lblSet;
		case 'r':
			pc = pcROOK;
			co = coBLACK;
			goto lblSet;
		case 'R':
			pc = pcROOK;
			co = coWHITE;
			goto lblSet;
		case 'q':
			pc = pcQUEEN;
			co = coBLACK;
			goto lblSet;
		case 'Q':
			pc = pcQUEEN;
			co = coWHITE;
			goto lblSet;
		case 'k':
			argcKing[coBLACK]++;
			pc = pcKING;
			co = coBLACK;
			goto lblSet;
		case 'K':
			argcKing[coWHITE]++;
			pc = pcKING;
			co = coWHITE;
			goto lblSet;
		case '/':
			fil = filA;
			rnk--;
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
			fil += *szFen - '0';
			break;
		}
	}
	if ((argcKing[coWHITE] != 1) || (argcKing[coBLACK] != 1))
		return fFALSE;		// Too many or not enough kings.  I could check
							//  for other cases of too many pieces ( > 9 Q's),
							//  but that doesn't seem necessary yet.
							// I'm also allowing > 8 pawns.
	//
	//	At this point I'm sitting beyond the first ' '.  Collect color, which
	//	must be either 'w' or 'b'.
	//
	switch (*szFen++) {
	case 'w':
		pste->coUs = coWHITE;
		pcon->gc.coStart = coWHITE;
		break;
	case 'b':
		pste->coUs = coBLACK;
		pcon->gc.coStart = coBLACK;
		pste->hashk = HashkSwitch(pste->hashk);
		break;
	default:			// Includes '\0' (end of input).
		return fFALSE;
	}
	pste->valPcUs = argvalPc[pste->coUs];
	pste->valPcThem = argvalPc[pste->coUs ^ 1];
	pste->valPnUs = argvalPn[pste->coUs];
	pste->valPnThem = argvalPn[pste->coUs ^ 1];
	//
	//	I'm sitting on the ' ' beyond color.  Force it, then get castling
	//	flags.
	//
	if (*szFen++ != ' ')
		return fFALSE;
	pste->cf = cfNONE;
	if (*szFen == '-') {
		szFen++;
		if (*szFen++ != ' ')
			return fFALSE;
	} else {
		for (;;) {
			if (*szFen == ' ') {
				szFen++;
				break;
			}
			switch (*szFen++) {
			default:			// Includes '\0' (end of input).
				return fFALSE;
			case 'k':
				pste->cf |= cfE8G8;
				break;
			case 'q':
				pste->cf |= cfE8C8;
				break;
			case 'K':
				pste->cf |= cfE1G1;
				break;
			case 'Q':
				pste->cf |= cfE1C1;
				break;
			}
		}
		//	There are a lot of FEN's with bogus castling flags.  Rather than
		//	be anal about this I'm going to do some quick checks and eliminate
		//	bogus flags.
		//
		if ((pcon->argsq[isqE1].ppi == NULL) ||
			(pcon->argsq[isqE1].ppi->pc != pcKING) ||
			(pcon->argsq[isqE1].ppi->co != coWHITE))
			pste->cf &= ~(cfE1G1 | cfE1C1);
		if ((pcon->argsq[isqA1].ppi == NULL) ||
			(pcon->argsq[isqA1].ppi->pc != pcROOK) ||
			(pcon->argsq[isqA1].ppi->co != coWHITE))
			pste->cf &= ~cfE1C1;
		if ((pcon->argsq[isqH1].ppi == NULL) ||
			(pcon->argsq[isqH1].ppi->pc != pcROOK) ||
			(pcon->argsq[isqH1].ppi->co != coWHITE))
			pste->cf &= ~cfE1G1;
		if ((pcon->argsq[isqE8].ppi == NULL) ||
			(pcon->argsq[isqE8].ppi->pc != pcKING) ||
			(pcon->argsq[isqE8].ppi->co != coBLACK))
			pste->cf &= ~(cfE8G8 | cfE8C8);
		if ((pcon->argsq[isqA8].ppi == NULL) ||
			(pcon->argsq[isqA8].ppi->pc != pcROOK) ||
			(pcon->argsq[isqA8].ppi->co != coBLACK))
			pste->cf &= ~cfE8C8;
		if ((pcon->argsq[isqH8].ppi == NULL) ||
			(pcon->argsq[isqH8].ppi->pc != pcROOK) ||
			(pcon->argsq[isqH8].ppi->co != coBLACK))
			pste->cf &= ~cfE8G8;
	}
	//	I'm sitting after the space after castling flags.  Time to process
	//	en-passant square.
	//
	//	I'll check to see if the square is in the range a1..h8, and if not
	//	I'll return FALSE.
	//
	//	If the square seems okay, I'll check to see if a capture might be
	//	possible, because sometimes people put bogus EnP squares in their
	//	FEN string.  If the capture looks impossible, I'll null-out the
	//	square.  "Might be possible" means that the target square is empty,
	//	there is a pawn that could capture, and a pawn that can be captured.
	//
	//	I'm not going to check to see if an en-passant capture is strictly
	//	legal.
	//
	pste->isqEnP = isqNIL;
	if (*szFen == '-')
		szFen++;
	else {
		int	isqFromL;
		int	isqFromR;
		int	isqCapture;
		int	isqEnP;

		if ((*szFen < 'a') || (*szFen > 'h'))
			return fFALSE;
		if ((*(szFen + 1) < '1') || (*(szFen + 1) > '8'))
			return fFALSE;
		//
		//	If here, square is in the range a1..h8.
		//
		isqEnP = isqCapture = isqNIL;
		if (pste->coUs == coWHITE) {
			if (*(szFen + 1) == '6') {
				isqEnP = *szFen - 'a' + (*(szFen + 1) - '1') * filIBD;
				isqCapture = isqEnP - filIBD;
			}
		} else {
			if (*(szFen + 1) == '3') {
				isqEnP = *szFen - 'a' + (*(szFen + 1) - '1') * filIBD;
				isqCapture = isqEnP + filIBD;
			}
		}
		//	"isqCapture" is the square of the captured pawn, given that
		//	"isqEnP" is where the capturing pawn ends up.  For instance, in
		//	the en-passant capture "exd6", the "isqEnP" square is e6, since
		//	that is where the capturing pawn goes, and "isqCapture" is d5,
		//	since that's the square the pawn is removed from.
		//
		isqFromL = (*szFen == 'a') ? isqNIL : isqCapture - 1;
		isqFromR = (*szFen == 'h') ? isqNIL : isqCapture + 1;
		if (isqEnP != isqNIL) {
			PPI	ppiCapture = pcon->argsq[isqCapture].ppi;

			//	Check to see if there is an appropriate pawn on the capture
			//	square and if there is an enemy pawn to the left and/or right
			//	of that square.
			//
			//	If these conditions are not true, this is a bogus en-passant
			//	square, and I'm going to null it out before it causes some
			//	sort of problem.
			//
			//	The same conditions apply during the search.  I only set
			//	en-passant squares if these conditions are met.
			//
			if ((ppiCapture == NULL) || (ppiCapture->pc != pcPAWN) ||
				(ppiCapture->co == pste->coUs))
				isqEnP = isqNIL;
			else if (((isqFromL == isqNIL) ||
				(pcon->argsq[isqFromL].ppi == NULL) ||
				(pcon->argsq[isqFromL].ppi->pc != pcPAWN) ||
				(pcon->argsq[isqFromL].ppi->co != pste->coUs)) &&
				((isqFromR == isqNIL) ||
				(pcon->argsq[isqFromR].ppi == NULL) ||
				(pcon->argsq[isqFromR].ppi->pc != pcPAWN) ||
				(pcon->argsq[isqFromR].ppi->co != pste->coUs)))
				isqEnP = isqNIL;
		}
		pste->isqEnP = isqEnP;
		szFen += 2;
	}
	if (*szFen++ != ' ')
		return fFALSE;
	//
	//	Get fifty-move counter.
	//
	pste->plyFifty = 0;
	for (;;) {
		if (!isdigit(*szFen))
			break;
		pste->plyFifty *= 10;
		pste->plyFifty += *szFen++ - '0';
	}
	if (*szFen++ != ' ')
		return fFALSE;
	//
	//	Get start move counter.
	//
	pcon->gc.movStart = 0;
	for (;;) {
		if (!isdigit(*szFen))
			break;
		pcon->gc.movStart *= 10;
		pcon->gc.movStart += *szFen++ - '0';
	}
	if (pcon->gc.movStart < 1)	// Start move must be 1 or more.
		return fFALSE;
	//
	//	Look for extraneous crap at the end of the FEN.
	//
	while (*szFen == ' ')
		szFen++;
	if (*szFen != '\0')
		return fFALSE;
	//
	//	Sets number of moves in this game (zero), and record the current
	//	position's hash key for rep checking.
	//
	pcon->gc.ccm = 0;				// Zero moves made so far.
	pcon->gc.arghashk[0] = pste->hashk;
	//
	//	"pcon->argste" contains some precomputed stuff, and this sets that
	//	up properly.
	//
	VFixSte(pcon);
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
