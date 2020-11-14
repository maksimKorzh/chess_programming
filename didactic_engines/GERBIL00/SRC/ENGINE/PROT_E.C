
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
#include <stdarg.h>
#include <windows.h>

#ifdef	DEBUG
static char const s_aszModule[] = __FILE__;
#endif

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is how my program implements my engine/Winboard protocol.  If you
//	want to use my code to hook your engine up to Winboard, all you should
//	have to do is write these functions.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Tells the engine to think forever on this position.

void VPrAnalyze(void * pv)
{
	PCON pcon = pv;

	Assert(pcon->smode == smodeIDLE);
	VThink(pcon, tmANALYZE);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Tells the engine to think for "tmRemaining", then emit a move, and start
//	pondering if that flag is on.

void VPrThink(void * pv, unsigned long tmRemaining)
{
	PCON pcon = pv;

	Assert(pcon->smode == smodeIDLE);
	VThink(pcon, tmRemaining);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Clears the current game and starts a new one based upon "szFen".

BOOL FPrSetboard(void * pv, char * szFen)
{
	PCON pcon = pv;

	Assert(pcon->smode == smodeIDLE);
	return FInitCon(pcon, szFen);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Returns TRUE or FALSE depending upon whether the engine is currently
//	pondering.

BOOL FPrPondering(void * pv)
{
	PCON pcon = pv;

	return (pcon->smode == smodePONDER);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A move is passed in, and if the engine is pondering, the engine goes
//	into normal "think" mode.

//	At the time the engine started pondering, "tmStart" was set.  This
//	routine will call "VSetTime", which will set the end time based upon
//	the time pondering started.

BOOL FPrPonderHit(void * pv, unsigned long tmRemaining, char * szMove)
{
	PCON pcon = pv;

	if (pcon->smode != smodePONDER)
		return fFALSE;
	if (strcmp(pcon->aszPonder, szMove))
		return fFALSE;
	pcon->smode = smodeTHINK;
	pcon->tmRemaining = tmRemaining;
	VSetTime(pcon);
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Adds this move to the current game.

void VPrAdvance(void * pv, char * szMove)
{
	PCON pcon = pv;

	Assert(pcon->smode == smodeIDLE);
	if (!FAdvance(pcon, szMove))
		VPrSendToIface("Illegal move: %s", szMove);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Tells the engine whether or not to post analysis while thinking.

void VPrSetPost(void * pv, BOOL fPost)
{
	PCON pcon = pv;

	pcon->fPost = fPost;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrSetTimeControl(void * pv, int cMoves,
	unsigned long tmBase, unsigned long tmIncr)
{
	PCON pcon = pv;

	VSetTimeControl(pcon, cMoves, tmBase, tmIncr);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VPrSetFixedDepthTimeControl(void * pv, int plyDepth)
{
	PCON pcon = pv;

	Assert(plyDepth >= 1);	// 0 will cause bad things.
	VSetFixedDepthTimeControl(pcon, plyDepth);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This initiates a request to move now.  The engine is thinking but may
//	be pondering.

void VPrMoveNow(void * pv)
{
	PCON pcon = pv;

	pcon->tmEnd = pcon->tmStart;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Back up in the game list.

void VPrUndoMove(void * pv)
{
	PCON pcon = pv;

	Assert(pcon->smode == smodeIDLE);
	VUndoMove(pcon);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Set ponder flag TRUE or FALSE.

void VPrSetPonder(void * pv, BOOL fPonder)
{
	PCON pcon = pv;

	pcon->fPonder = fPonder;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Abort the search immediately.

void VPrAbort(void * pv)
{
	PCON pcon = pv;

	pcon->fAbort = fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This passes back the move I'm pondering on.

void VPrHint(void * pv)
{
	PCON pcon = pv;

	if (pcon->smode != smodePONDER)
		return;
	if (pcon->aszPonder[0] != '\0')
		VPrSendHint(pcon->aszPonder);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Inits engine and returns pointer to context.

void * PvPrEngineInit(void)
{
	return (void *)PconInitEngine();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
