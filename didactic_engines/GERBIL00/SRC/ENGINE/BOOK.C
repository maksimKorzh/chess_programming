
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
#include <ctype.h>

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	The program's book is read from a text file at startup.  Positions that
//	can be reached by following the lines in the book are hashed.

//	When the program wants to play a book move, it checks to see if it can
//	reach any positions that are in the hash table.

//	If it finds any, it picks one at random.

//	Note that the program won't try to reach book positions that are in the
//	game history already.  This prevents it from going for a draw after
//	1. e4 e5 2. Nf3 Nc6 3. Ng1.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	struct	tagBN {
	HASHK	hashk;
}	BN, * PBN;

PBN s_rgbn;
int	s_cbnMax;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Sticks this position into the book hash table, unless it is already there.
//	Returns TRUE if it added a node.

BOOL FHashBn(HASHK hashk)
{
	unsigned	ihashk;

	ihashk = (unsigned)hashk % s_cbnMax;
	for (;;) {
		if (s_rgbn[ihashk].hashk == hashk)
			return fFALSE;
		if (!s_rgbn[ihashk].hashk) {
			s_rgbn[ihashk].hashk = hashk;
			return fTRUE;
		}
		ihashk = (ihashk + 1) % s_cbnMax;
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Checks to see if this position is known.

BOOL FBnHashSet(HASHK hashk)
{
	unsigned	ihashk;

	ihashk = (unsigned)hashk % s_cbnMax;
	for (;;) {
		if (s_rgbn[ihashk].hashk == hashk)
			return fTRUE;
		if (!s_rgbn[ihashk].hashk)
			return fFALSE;
		ihashk = (ihashk + 1) % s_cbnMax;
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Takes the '\n' off the end of a string.

void VStrip(char * sz)
{
	int	i;

	for (i = 0; sz[i]; i++)
		if (sz[i] == '\n') {
			sz[i] = '\0';
			break;
		}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Breaks a string up into pieces.

int	CszVectorize(char * sz, char * rgsz[])
{
	int	i;
	int	csz;

	for (csz = 0, i = 0; sz[i]; i++)
		if (sz[i] == '(') {
			rgsz[csz++] = sz + i;
			break;
		} else if ((sz[i] != ' ') && (sz[i] != '\t')) {
			rgsz[csz++] = sz + i;
			for (;; i++) {
				if ((sz[i] == ' ') || (sz[i] == '\t'))
					break;
				if (sz[i] == '\0')
					break;
			}
			if (sz[i] == '\0')
				break;
			sz[i] = '\0';
		}
	return csz;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	If I find an error while loading the book, I'll spit it out with this
//	function.

void VBookError(int cLine, char * sz)
{
	VPrSendToIface("Book error (line %d) : %s.", cLine, sz);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Load the book from a text file.

//	The book format is a line of opening description followed by zero or more
//	lines, each of which has one or two moves in e2e4 format, followed by a
//	line containing only a "!".

//	I took the book format TSP.  A quirk is that rather than storing castling
//	as e1g1, it stores it as "o-o" (lower case).  I handle this and "O-O"
//	(upper case) properly.

//	I also added an enhancement in that if a move is followed by a "?", it
//	and the rest of the moves for that side in this line will be not be
//	hashed, which will cause the program to not play any move moves for that
//	side in this line, although it will still play book moves in response.

//	If the "?" move appears in any subsequent lines, without a "?", it will
//	be hashed like normal.

char const s_aszFenDefault[] =
	"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

BOOL FGenBook(PCON pcon, char * szFile, int bdm)
{
	FILE * pfI;
	int	cLine;
	BOOL	f;
	CM	argcmPrev[256];
	BOOL argfAvoid[256];
	int	cplyPrev;
	BOOL	fResult;
	int	cbn = 0;

	s_cbnMax = pcon->cbMaxBook / sizeof(BN);
	if (s_cbnMax < 1)
		s_cbnMax = 1;
	if ((s_rgbn = malloc(s_cbnMax * sizeof(BN))) == NULL) {
		VPrSendToIface("Can't allocate book memory: %d bytes",
			s_cbnMax * sizeof(BN));
		return fFALSE;
	}
	if ((pfI = fopen(szFile, "r")) == NULL) {
		VPrSendToIface("Can't read book: '%s'", szFile);
		return fFALSE;
	}
	cplyPrev = 0;
	fResult = fTRUE;
	for (cLine = 0;;) {
		char	aszBuf[512];
		char	aszOut[512];
		char *	szOut = aszOut;
		char *	argsz[256];
		int	csz;
		PSTE	pste = pcon->argste;
		int	isz;
		int	i;
		BOOL	fFollowing;
		int	cply;

		//	Get a line from the file.
		//
		if (fgets(aszBuf, sizeof(aszBuf), pfI) != aszBuf)	// Opening name.
			break;
		cLine++;
		VStrip(aszBuf);
		if ((aszBuf[0] == '/') || (aszBuf[0] == '\0')) {
			if (bdm != bdmNONE)
				printf("%s\n", aszBuf);
			continue;
		}
		//	Set up the board.
		//
		f = FInitCon(pcon, s_aszFenDefault);
		Assert(f);
		//
		//	Break the line up.
		//
		csz = CszVectorize(aszBuf, argsz);
		isz = cply = 0;
		//
		//	Look at the first string and see if it's a move number.  Set
		//	"cply" appropriately based upon this.  If the move is of the
		//	form "1" or "1.", I assume it's white to move.  If it's "1.."
		//	I assume it's black to move.
		//
		if ((isz < csz) && (isdigit(argsz[isz][0]))) {
			int	cmov;

			cmov = 0;
			for (i = 0; argsz[isz][i]; i++)
				if (isdigit(argsz[isz][i]))
					cmov = cmov * 10 + argsz[isz][i] - '0';
				else
					break;
			cply = (cmov - 1) * 2;
			if ((argsz[isz][i++] == '.') && (argsz[isz][i] == '.'))
				cply++;
			isz++;
		} else
			cply = 0;
		//
		//	If white is to move, check the next string for "...", which
		//	indicates that it's black's move.
		//
		//	The basic deal here is that I'm trapping the cases "1.." and
		//	"1. ..." and flagging these as black to move.
		//
		if ((isz < csz) && (!(cply & 1)))
			if (!strcmp(argsz[isz], "...")) {
				cply++;
				isz++;
			}
		//
		//	If I set this up to start other than on move 1, I have to have
		//	enough previous context lying around from the previous line.
		//
		if (cplyPrev < cply) {
			VBookError(cLine, "Discontiguous opening line");
			fResult = fFALSE;
			goto lblDone;
		}
		//	Skip over any previous context.  If I'm outputting in "full" mode,
		//	write this stuff to stdout.
		//
		for (i = 0; i < cply; i++) {
			char	aszMov[32];
			BOOL	f;

			if (bdm == bdmFULL) {
				char	aszSan[32];
				SAN	san;

				VCmToSan(pcon, pste, &argcmPrev[i], &san);
				VSanToSz(&san, aszSan);
				if (!(i & 1))
					szOut += sprintf(szOut, "%d. ", i / 2 + 1);
				szOut += sprintf(szOut, "%s", aszSan);
				if (argfAvoid[i])
					*szOut++ = '?';
				*szOut++ = ' ';
			} else if (bdm != bdmNONE)
				szOut += sprintf(szOut, "  ");
			VCmToSz(&argcmPrev[i], aszMov);
			f = FAdvance(pcon, aszMov);
			Assert(f);
		}
		//	I'm going to take care of the new stuff now.
		//
		fFollowing = fTRUE;
		for (; isz < csz; isz++) {
			char	aszSan[32];
			char	aszMov[32];
			int	j;
			PCM	pcm;
			SAN	san;
			CM	cm;

			if (atoi(argsz[isz]))		// Skip move numbers.
				continue;
			if (argsz[isz][0] == '(')	// Variation name is last arg.
				break;
			//
			//	If the move some even number of plies back was a "?" move,
			//	this is a "?" move, too, otherwise it isn't.
			//
			if (cply < 2)
				argfAvoid[cply] = fFALSE;
			else
				argfAvoid[cply] = argfAvoid[cply - 2];
			//
			//	Look for an explicit "?".
			//
			for (j = 0; argsz[isz][j]; j++)
				if (argsz[isz][j] == '?') {
					argsz[isz][j] = '\0';
					argfAvoid[cply] = fTRUE;
					break;
				}
			//
			//	Find the move.
			//
			VGenMoves(pcon, pste, fFALSE);
			pcm = pste->pcmFirst;
			for (; pcm < (pste + 1)->pcmFirst; pcm++) {
				VCmToSan(pcon, pste, pcm, &san);
				VSanToSz(&san, aszSan);
				if (!strcmp(aszSan, argsz[isz])) {
					cm = *pcm;
					break;
				}
			}
			Assert(pcm < (pste + 1)->pcmFirst);
			//
			//	Output the move if necssary.  This is some tricky and nasty
			//	code.
			//
			if (bdm != bdmNONE) {
				if (fFollowing)
					if ((cplyPrev <= cply) || (memcmp(&cm,
						&argcmPrev[cply], sizeof(CM)))) {
						if ((cply & 1) && (bdm == bdmCOMPACT))
							szOut += sprintf(szOut, "%d. ... ", cply / 2 + 1);
						fFollowing = fFALSE;
					} else if (bdm == bdmCOMPACT)
						szOut += sprintf(szOut, "  ");
				if ((!fFollowing) || (bdm == bdmFULL)) {
					if (!(cply & 1))
						szOut += sprintf(szOut, "%d. ", cply / 2 + 1);
					szOut += sprintf(szOut, "%s", aszSan);
					if (argfAvoid[cply])
						*szOut++ = '?';
					*szOut++ = ' ';
				}
			}
			//	Make the move on the internal board.
			//
			VCmToSz(&cm, aszMov);
			if (!FAdvance(pcon, aszMov)) {
				sprintf(aszBuf, "Illegal move for %s, move %d on line",
					(pste->coUs == coWHITE) ? "white" : "black", isz + 1);
				VBookError(cLine, aszBuf);
				fResult = fFALSE;
				goto lblDone;
			}
			//	Hash it unless it is a "?" move.
			//
			if (!argfAvoid[cply])
				if (FHashBn(pcon->argste[0].hashk))
					if (++cbn >= s_cbnMax * 3 / 4) {
						VBookError(cLine, "Hash table full");
						fResult = fFALSE;
						goto lblDone;
					}
			//
			//	Remember the line.
			//
			argcmPrev[cply++] = cm;
		}
		cplyPrev = cply;
		//
		//	Deal with the variation name field at the end of the line.
		//
		if (bdm != bdmNONE) {
			if (isz < csz) {
				while (szOut - aszOut < 128)
					*szOut++ = ' ';
				szOut += sprintf(szOut, "%s", argsz[isz]);
			}
			*szOut = '\0';
			VStrip(szOut);
			printf("%s\n", aszOut);
		}
	}
	if (bdm != bdmNONE)
		fResult = fFALSE;
lblDone:
	printf("%d book nodes\n", cbn);
	fclose(pfI);
	return fResult;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This just makes a list of book moves the program is considering choosing
//	from, and spits it out to the interface.

void VDumpBookPv(PCON pcon, PCM rgcm, int ccm, int icm)
{
	PSTE pste = pcon->argste;
	char asz[256];
	char * sz;
	SAN	san;
	int	i;

	if (!pcon->fPost)
		return;
	sz = asz;
	*sz++ = '(';
	for (i = 0; i < ccm; i++) {
		if (i) {
			*sz++ = ',';
			*sz++ = ' ';
		}
		VCmToSan(pcon, pste, &rgcm[i], &san);
		VSanToSz(&san, sz);
		sz += strlen(sz);
	}
	*sz++ = ')';
	*sz++ = '\0';
	VPrSendAnalysis(0, 0, 0, 0, asz);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Finds possible book positions.  If it finds any, it picks one at random
//	and returns it.  It won't pick a position that's already appeared on the
//	board in this game.  This is done to keep the engine from taking draws
//	in situations where the opponent tries to get cute.

BOOL FBookMove(PCON pcon, PCM pcmMove)
{
	PCM	pcm;
	PSTE pste = &pcon->argste[0];
	CM	argcm[256];
	int	ccm;

	VGenMoves(pcon, pste, fFALSE);
	ccm = 0;
	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
		VMakeMove(pcon, pste, pcm);
		if (FAttacked(pcon, pcon->argpi[pste->coUs][0].isq, pste->coUs ^ 1)) {
			VUnmakeMove(pcon, pste, pcm);
			continue;
		}
		if (FBnHashSet((pste + 1)->hashk)) {
			int	i;

			for (i = 0; i <= pcon->gc.ccm; i++)
				if (pcon->gc.arghashk[i] == (pste + 1)->hashk)
					break;
			if (i >= pcon->gc.ccm)
				argcm[ccm++] = *pcm;
		}
		VUnmakeMove(pcon, pste, pcm);
	}
	if (ccm) {
		int	icm = rand() % ccm;

		VDumpBookPv(pcon, argcm, ccm, icm);
		*pcmMove = argcm[icm];
		return fTRUE;
	}
	return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
