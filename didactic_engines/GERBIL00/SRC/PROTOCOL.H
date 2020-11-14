
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

//	The following functions must be implemented in the interface side.  They
//	are called by the engine.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This should be called often when thinking.  A decent place to put it is in
//	the eval function, or at the top of "search" or "qsearch".  After it
//	returns, you should check to see if the search has been aborted.  If the
//	search has been aborted, you shouldn't call this function anymore until
//	thinking has started again.

void VPrCallback(void * pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This allows the engine to send a command to the interface.  Most common
//	commands have more specific functions, but this could send an error
//	message, a version, or something else, as desired.

void cdecl VPrSendToIface(const char * szFmt, ...);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Send the search result to the interface.  This is the move the program
//	intends to make, in the format "e2e4", "e1g1", "e7e8q", etc.

void VPrSendMove(char * szMove);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	If the engine is going to ponder, after it makes its move it will send
//	the move it is pondering on.

void VPrSendHint(char * szMove);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Sends a line of analysis to the interface.  "tm" is time searched in
//	milliseconds.  "szLine" is not particularly well defined, it can be
//	anything you want.

void VPrSendAnalysis(int ply, int val, unsigned long tm,
	unsigned __int64 nodes, char * szLine);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This should send a result when the game ends.  "szResult" will be
//	something like "1/2-1/2".  "szWhy" will be something like "Drawn by
//	50-move rule".  The "szWhy" pointer can be NULL if desired.

void VPrSendResult(char * szResult, char * szWhy);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	These functions are implemented in the engine.  The interface calls them
//	to make the interface do various things.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will cause the engine to analyze the current position.  It should
//	never make a move, and it should not return from this call until the
//	search has been aborted, although it should periodically call
//	"VPrCallback".

void VPrAnalyze(void * pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will cause the engine to think about the current position, and
//	eventually move.  It should not return from this call until the
//	search has been aborted or it otherwise decides not to think anymore (it
//	can stay in here if it wants to ponder).

void VPrThink(void * pv, unsigned long tmRemaining);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called when the engine is not thinking.  The engine will set up
//	the position specified by the FEN, and continue about its business
//	(without thinking).

BOOL FPrSetboard(void * pv, char * szFen);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called when the engine is thinking about something.  It returns
//	TRUE if the engine is in ponder mode.

BOOL FPrPondering(void * pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called when the engine is thinking about something.  The "szMove"
//	parameter is the move just entered by the user.  The function is supposed
//	to check to see if it is pondering.  If not, return FALSE.  If it is
//	pondering, and it is pondering "szMove", set the engine up to think
//	instead and return TRUE.  Otherwise return FALSE, which will cause the
//	interface to abort the search and start a new one.

BOOL FPrPonderHit(void * pv, unsigned long tmRemaining, char * szMove);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This adds "szMove" to the end of the game move list.  It will be called
//	when the engine is not thinking.

void VPrAdvance(void * pv, char * szMove);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine to turn its analysis output on or off.  The engine
//	may or may not be thinking at the time.

void VPrSetPost(void * pv, BOOL fPost);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine the current time control.  The engine may or may
//	not be thinking at the time.  If in analysis mode, this does not mean that
//	the engine should start thinking and make a move.

void VPrSetTimeControl(void * pv, int cMoves,
	unsigned long tmBase, unsigned long tmIncr);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is analogous to "VPrSetTimeControl".

void VPrSetFixedDepthTimeControl(void * pv, int plyDepth);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will be called if the engine is thinking about something.  If the
//	engine is pondering, it should ignore this.  If the engine is thinking
//	about making a move, it should make it pretty soon (it doesn't have to
//	make it before returning from this call).

void VPrMoveNow(void * pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine to undo the last move in its move list.  It will be
//	called when the engine is not thinking.

void VPrUndoMove(void * pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine whether it should ponder or not.  If pondering is
//	going to be turned off, it will be called when the engine is not pondering
//	(although it might be thinking).

void VPrSetPonder(void * pv, BOOL fPonder);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This tells the engine directly to abort the search.  It will only be
//	called if the engine is thinking about something.

void VPrAbort(void * pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is a request for a hint, which may be ignored.  It will only be
//	sent while the engine is thinking, although it may not be pondering.

void VPrHint(void * pv);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This will be called once at boot.  The engine should return a pointer to
//	its context, which will be passed to all of the above calles, or NULL if
//	it can't initialize itself.

void * PvPrEngineInit(void);

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
