
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
#include <windows.h>

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Returns system time in milliseconds.  If you are porting this away from
//	windows, this might cause you minor trouble, but you should be able to
//	find something.  High clock resolution is a plus, but if you don't have
//	it it's not absolutely fatal.

unsigned TmNow(void)
{
	return GetTickCount();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	If this is called while a search is running, it won't change the time
//	control unless the time control is tctFIXED_DEPTH now, in which case
//	it will probably crash and burn.

void VSetTimeControl(PCON pcon, int cMoves, TM tmBase, TM tmIncr)
{
	switch (cMoves) {
	case 0:
		pcon->tc.tct = tctINCREMENT;
		break;
	case 1:
		pcon->tc.tct = tctFIXED_TIME;
		break;
	default:
		pcon->tc.tct = tctTOURNEY;
		break;
	}
	pcon->tc.cMoves = cMoves;
	pcon->tc.tmBase = tmBase;
	pcon->tc.tmIncr = tmIncr;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	See the caveat regarding "VSetTimeControl".

void VSetFixedDepthTimeControl(PCON pcon, int plyDepth)
{
	pcon->tc.tct = tctFIXED_DEPTH;
	pcon->tc.plyDepth = plyDepth;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called at the start of a search that's expected to output a move
//	at some point.
//
//	"tmStart" is *not* set by this routine.

void VSetTime(PCON pcon)
{
	switch (pcon->tc.tct) {
		TM	tmUse;
		int	movLeft;
		int	movMade;

	case tctINCREMENT:
		if (pcon->tc.tmIncr) {
			tmUse = pcon->tmRemaining / 30 + (pcon->tc.tmIncr * 9) / 10;
			pcon->tmEnd = pcon->tmStart + tmUse;
		} else {
			if (pcon->tmRemaining >= 600000)	// 10 minutes.
				tmUse = pcon->tmRemaining / 30;
			else if (pcon->tmRemaining > 60000)	// 1 minute.
				tmUse = pcon->tmRemaining / 40;
			else
				tmUse = pcon->tmRemaining / 60;
			pcon->tmEnd = pcon->tmStart + tmUse;
		}
		break;
	case tctFIXED_TIME:
		tmUse = pcon->tc.tmBase;		// Use *all* the time.
		pcon->tmEnd = pcon->tmStart + tmUse;
		break;
	case tctFIXED_DEPTH:
		break;		// Do nothing.
	case tctTOURNEY:
		movMade = pcon->gc.ccm / 2 + pcon->gc.movStart - 1;
		while (movMade >= pcon->tc.cMoves)
			movMade -= pcon->tc.cMoves;
		movLeft = pcon->tc.cMoves - movMade;
		if (movLeft >= 30)						// This is a fudge so it will
			movLeft += 10;						//  buffer some time.
		else if (movLeft >= 25)
			movLeft += 8;
		else if (movLeft >= 20)
			movLeft += 6;
		else if (movLeft >= 15)
			movLeft += 5;
		else if (movLeft >= 10)
			movLeft += 4;
		else if (movLeft >= 4)
			movLeft += 3;
		else
			movLeft += 1;
		tmUse = pcon->tmRemaining / movLeft;
		pcon->tmEnd = pcon->tmStart + tmUse;
		break;
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called a whole bunch of times via the engine/interface protocol.
//	Given what is known about the time control, the current time, and the
//	search depth, this may set "ptcon->fTimeout", which will eventually finish
//	the search.

//	This routine has a built-in defense against cases where someone might try
//	to force a move before the first ply has been fully considered.  It will
//	simply ignore such cases.

void VCheckTime(PCON pcon)
{
	if (pcon->plyDepth == 1)	// Can't time out before a one-ply
		return;					//  search is finished.
	if (pcon->smode != smodeTHINK)
		return;
	if (pcon->tc.tct == tctFIXED_DEPTH)
		return;
	if (TmNow() >= pcon->tmEnd)
		pcon->fTimeout = fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This routine takes a "SAN" record and produces a SAN move output string
//	(such as "e4", or "dxe6+", or "Nbd7", or "Qxg7#").

//	The SAN element already has all the information needed to do this, so it's
//	a very simple process.

void VSanToSz(PSAN psan, char * sz)
{
	if (psan->cmf & cmfCASTLE) {
		if ((psan->isqTo == isqG1) || (psan->isqTo == isqG8))
			sz += sprintf(sz, "O-O");
		else
			sz += sprintf(sz, "O-O-O");
	} else {
		if (psan->pc == pcPAWN)
			*sz++ = FilFromIsq(psan->isqFrom) + 'a';
		else {
			*sz++ = s_argbPc[coWHITE][psan->pc];
			if (psan->sanf & sanfRNK_AMBIG)
				*sz++ = FilFromIsq(psan->isqFrom) + 'a';
			if (psan->sanf & sanfFIL_AMBIG)
				*sz++ = RnkFromIsq(psan->isqFrom) + '1';
		}
		if (psan->cmf & cmfCAPTURE) {
			*sz++ = 'x';
			*sz++ = FilFromIsq(psan->isqTo) + 'a';
			*sz++ = RnkFromIsq(psan->isqTo) + '1';
		} else if (psan->pc == pcPAWN)
			*sz++ = RnkFromIsq(psan->isqTo) + '1';
		else {
			*sz++ = FilFromIsq(psan->isqTo) + 'a';
			*sz++ = RnkFromIsq(psan->isqTo) + '1';
		}
		if (psan->cmf & cmfPR_MASK) {
			*sz++ = '=';
			*sz++ = s_argbPc[coWHITE][psan->cmf & cmfPR_MASK];
		}
	}
	if (psan->sanf & sanfMATE)
		*sz++ = '#';
	else if (psan->sanf & sanfCHECK)
		*sz++ = '+';
	*sz = '\0';
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This takes a CM and converts it to a SAN record, which can be used by
//	"VSanToSz" to make something pretty to display.

//	This is a terrible process, in large part because of disambiguation.
//	Disambiguation would be extremely easy, if all I had to do is disambiguate
//	between moves produced by the move generator.  But some of these moves are
//	not actually legal, so I have to delete those.

//	Also, part of the process includes appending "+", or "#" if necessary,
//	which especially in the latter case is annoying.

//	So this routine figures out if a given move is ambiguous, and if so
//	records exactly how, and it also figures out if the move is check or mate.

void VCmToSan(PCON pcon, PSTE pste, PCM pcmSrc, PSAN psan)
{
	PCM	pcm;
	BOOL fAmbiguous;

	//	Easy stuff first.
	//
	psan->pc = pcon->argsq[pcmSrc->isqFrom].ppi->pc;
	psan->isqFrom = pcmSrc->isqFrom;
	psan->isqTo = pcmSrc->isqTo;
	psan->cmf = pcmSrc->cmf;
	psan->sanf = sanfNONE;
	//
	//	Disambiguate.
	//
	VGenMoves(pcon, pste, fFALSE);
	fAmbiguous = fFALSE;
	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++)
		if ((pcm->isqTo == pcmSrc->isqTo) &&
			(pcm->isqFrom != pcmSrc->isqFrom) &&
			(pcon->argsq[pcm->isqFrom].ppi->pc == psan->pc)) {
			VMakeMove(pcon, pste, pcm);
			if (!FAttacked(pcon, pcon->argpi[pste->coUs][0].isq,
				pste->coUs ^ 1)) {
				if (RnkFromIsq(pcm->isqFrom) == RnkFromIsq(pcmSrc->isqFrom))
					psan->sanf |= sanfRNK_AMBIG;
				if (FilFromIsq(pcm->isqFrom) == FilFromIsq(pcmSrc->isqFrom))
					psan->sanf |= sanfFIL_AMBIG;
				fAmbiguous = fTRUE;
			}
			VUnmakeMove(pcon, pste, pcm);
		}
	if ((fAmbiguous) && (!(psan->sanf & (sanfRNK_AMBIG | sanfFIL_AMBIG))))
		psan->sanf |= sanfRNK_AMBIG;	// "Nbd2" rather than "N1d2".
	//
	//	The gross disambiguation has already been done.  Now I'm going to
	//	execute the move and see if it is check or mate.
	//
	VMakeMove(pcon, pste, pcmSrc);
	if (FAttacked(pcon, pcon->argpi[pste->coUs ^ 1][0].isq, pste->coUs)) {
		pste++;
		psan->sanf |= sanfCHECK;
		VGenMoves(pcon, pste, fFALSE);
		for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
			VMakeMove(pcon, pste, pcm);
			if (!FAttacked(pcon, pcon->argpi[pste->coUs][0].isq,
				pste->coUs ^ 1)) {
				VUnmakeMove(pcon, pste, pcm);
				break;
			}
			VUnmakeMove(pcon, pste, pcm);
		}
		if (pcm == (pste + 1)->pcmFirst)
			psan->sanf |= sanfMATE;
		pste--;
	}
	VUnmakeMove(pcon, pste, pcmSrc);
	//
	//	That was painful.
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This spits a PV out to the interface.  The only gotcha in here is that
//	I have a "ch" parameter that indicates whether the search is in fail-high
//	(+) or fail-low (-).

void VDumpPv(PCON pcon, int ply, int val, int ch)
{
	char asz[256];
	char * sz;
	int	i;

	if (!pcon->fPost)	// The interface doesn't want PV's.
		return;
	sz = asz;
	if (ch != '\0')
		*sz++ = ch;
	//
	//	In order to do the SAN thing, I need to actually execute the moves
	//	on the board, so I call "VMakeMove" at the end of this.
	//
	for (i = 0; i < pcon->argste[0].ccmPv; i++) {
		SAN	san;
		PCM	pcmPv;
		PSTE	pste;

		pste = pcon->argste + i;
		pcmPv = &pcon->argste[0].argcmPv[i];
		VCmToSan(pcon, pste, pcmPv, &san);
		VSanToSz(&san, sz);
		sz += strlen(sz);
		if (i + 1 < pcon->argste[0].ccmPv)
			*sz++ = ' ';
		VMakeMove(pcon, pste, pcmPv);
	}
	//	Undo all of the make-move's.
	//
	for (; --i >= 0;)
		VUnmakeMove(pcon, pcon->argste + i, &pcon->argste[0].argcmPv[i]);
	*sz++ = '\0';
	//
	//	Ship it.
	//
	VPrSendAnalysis(ply, val, TmNow() - pcon->tmStart, pcon->nodes, asz);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called before a search in order to tell the search which moves
//	to try first when iterating through them.

void VSetPv(PCON pcon)
{
	int	i;
	
	for (i = 0; i < pcon->argste[0].ccmPv; i++)
		pcon->argste[i].cmPv = pcon->argste[0].argcmPv[i];
	for (; i < csteMAX; i++) {
		pcon->argste[i].cmPv.isqFrom = isqNIL;
		pcon->argste[i].cmPv.isqTo = isqNIL;
		pcon->argste[i].cmPv.cmf = cmfNONE;
		pcon->argste[i].cmPv.cmk = cmkNONE;
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	"rand()" produces 15-bit values (0..32767).  I want 64 bits.

static U64 U64Rand(void)
{
	return (U64)rand() ^ ((U64)rand() << 15) ^ ((U64)rand() << 30);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Fills up the hash key table with random gibberish.  These will be XOR'd
//	together in order to create the semi-unique hash key for any given
//	position.  They need to be as random as possible otherwise there will be
//	too many hash collisions.

//	I've never cared about hash collisions.  If they are a bigger problem
//	than I think, you might want to make this more random.

void VInitHashk(void)
{
	int	co;
	int	pc;
	int	isq;
	int	fil;
	int	cf;
	
	for (pc = pcPAWN; pc <= pcKING; pc++)
		for (co = coWHITE; co <= coBLACK; co++)
			for (isq = 0; isq < csqMAX; isq++)
				if (FilFromIsq(isq) <= filH)
					s_arghashkPc[pc][co][isq] = U64Rand();
	for (fil = filA; fil <= filH; fil++)
		s_arghashkEnP[fil] = U64Rand();
	for (cf = 0; cf <= cfMAX; cf++)
		s_arghashkCf[cf] = U64Rand();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This converts a CM to "e2e4" (or "e7e8q") format.  I'm writing this with
//	Winboard in mind, and that is a format that Winboard will accept.  It is
//	also very easy to generate.

void VCmToSz(PCM pcm, char * sz)
{
	sz += sprintf(sz, "%c%c",
		FilFromIsq(pcm->isqFrom) + 'a',
		RnkFromIsq(pcm->isqFrom) + '1');
	sz += sprintf(sz, "%c%c",
		FilFromIsq(pcm->isqTo) + 'a',
		RnkFromIsq(pcm->isqTo) + '1');
	if ((pcm->cmf & cmfPR_MASK) != pcPAWN)
		sz += sprintf(sz, "%c", s_argbPc[coBLACK][pcm->cmf & cmfPR_MASK]);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Execute "szMove" on the board, update game context ("pcon->gc"), then set
//	the other "pcon" fields up so it's ready to search the new position.

//	If it doesn't find the move, it returns FALSE.

BOOL FAdvance(PCON pcon, char * szMove)
{
	PSTE	pste;
	PCM	pcm;
	
	pste = pcon->argste;
	VGenMoves(pcon, pste, fFALSE);
	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
		char	asz[16];

		VCmToSz(pcm, asz);
		if (!strcmp(asz, szMove)) {
			VMakeMove(pcon, pste, pcm);
			memcpy(pste, pste + 1, sizeof(STE));
			VFixSte(pcon);
			pcon->gc.argcm[pcon->gc.ccm] = *pcm;
			pcon->gc.arghashk[++pcon->gc.ccm] = pste->hashk;
			break;
		}
	}
	return (pcm != (pste + 1)->pcmFirst);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This backs up one move in the current game, and deletes all trace of the
//	backed up move.

//	It works by going back to the beginning and going forward.

//	This routine is a little evil because it assumes that parts of the current
//	game context aren't blown out by "FInitCon".

void VUndoMove(PCON pcon)
{
	int	i;
	GAME_CONTEXT	gc;
	BOOL	f;

	gc = pcon->gc;
	if (!gc.ccm)
		return;
	f = FInitCon(pcon, gc.aszFen);
	Assert(f);
	for (i = 0, gc.ccm--; i < gc.ccm; i++) {
		char	asz[16];
		
		VCmToSz(&gc.argcm[i], asz);
		f = FAdvance(pcon, asz);
		Assert(f);
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A simple routine that checks for draws based upon the game history, and by
//	that I mean repetition draws and 50-move draws.  It does nothing about
//	stalemate, insuffient material, etc.

void VDrawCheck(PCON pcon)
{
	if (pcon->smode != smodeTHINK)
		return;
	if ((pcon->smode == smodeTHINK) && (pcon->argste[0].plyFifty >= 50 * 2))
		VPrSendResult("1/2-1/2", "Drawn by 50-move rule");
	else {
		int	i;
		int	cReps;

		cReps = 1;
		for (i = 0; i < pcon->gc.ccm; i++)
			if (pcon->gc.arghashk[i] == pcon->gc.arghashk[pcon->gc.ccm])
				if (++cReps >= 3) {
					VPrSendResult("1/2-1/2", "Drawn by repetition");
					break;
				}
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This checks to see if the side to move at the root of "pcon" is
//	stalemated.

BOOL FStalemated(PCON pcon)
{
	PSTE	pste;
	PCM	pcm;
	
	pste = pcon->argste;
	if (FAttacked(pcon, pcon->argpi[pste->coUs][0].isq, pste->coUs ^ 1))
		return fFALSE;
	VGenMoves(pcon, pste, fFALSE);
	for (pcm = pste->pcmFirst; pcm < (pste + 1)->pcmFirst; pcm++) {
		BOOL	fIllegal;

		VMakeMove(pcon, pste, pcm);
		fIllegal = FAttacked(pcon,
			pcon->argpi[pste->coUs][0].isq, pste->coUs ^ 1);
		VUnmakeMove(pcon, pste, pcm);
		if (!fIllegal)
			return fFALSE;
	}
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	"coWin" just checkmated.  Tell the interface.

void VMates(int coWin)
{
	char	aszResult[64];
	char	aszReason[64];
				
	sprintf(aszResult, "%d-%d", coWin ^ 1, coWin);
	sprintf(aszReason, "%s mates", s_argszCo[coWin]);
	VPrSendResult(aszResult, aszReason);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VReadIni(PCON pcon)
{
	static char const s_aszKey[] = "settings";
	char	aszPath[256];
	int	i;

	GetModuleFileName(GetModuleHandle(NULL), aszPath, sizeof(aszPath));
	for (i = strlen(aszPath) - 1; i >= 0; i--)
		if (aszPath[i] == '\\') {
			strcpy(aszPath + i + 1, "gerbil.ini");
			break;
		}
	Assert(i >= 0);
	GetPrivateProfileString(s_aszKey, "BookFile",
		"gerbil.opn", pcon->aszBook, sizeof(pcon->aszBook), aszPath);
	pcon->fUseBook = GetPrivateProfileInt(s_aszKey, "UseBook",
		fTRUE, aszPath);
	pcon->cbMaxHash = GetPrivateProfileInt(s_aszKey, "MaxHash",
		10000000, aszPath);
	pcon->cbMaxBook = GetPrivateProfileInt(s_aszKey, "MaxBook",
		65536, aszPath);
	pcon->bdm = GetPrivateProfileInt(s_aszKey, "BookDump",
		bdmNONE, aszPath);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Initialization function.  Returns pointer to main context.

//	If this routine fails, it leaks some memory.  This shouldn't be a problem
//	if the app shuts down as a consequence.

PCON PconInitEngine(void)
{
	static	CON s_con;
	SYSTEMTIME	st;

	Assert(sizeof(HASH) == 16);
	VReadIni(&s_con);
	s_con.smode = smodeIDLE;
	VInitAttackData();
	VInitHashk();
	if (!FInitHashe(&s_con))
		return NULL;
	if ((s_con.fUseBook) && (!FGenBook(&s_con, s_con.aszBook, s_con.bdm)))
		return NULL;
	GetSystemTime(&st);
	srand((st.wHour ^ st.wMinute ^ st.wSecond ^ st.wMilliseconds) & 0x7FFF);
	return &s_con;
}
