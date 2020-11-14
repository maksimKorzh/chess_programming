
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "winboard.h"
#include "protocol.h"

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

static char const s_aszModule[] = __FILE__;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is the engine thread.  The following documentation is very important
//	and unless you understand it, you won't understand how any of this works.

//	Commands are sent to the engine thread by the input thread by use of
//	"VSendEngineCommand".  This function sits (maybe for a while) on an event
//	called "s_evars.heventCmdFinished".  If this is not set, it assumes
//	that the engine is working on a command, and waits.

//	The engine thread sits in a loop ("DwEngine"), processing engine commands
//	from the input thread, via use of "VProcessEngineCommand".  This function
//	waits for "s_evars.heventCmdPresent" to be set, and if it finds it,
//	it reads it and processes it, then it sets "s_evars.heventCmdFinished",
//	so the input thread can send another command.

//  If it gets a command that tells it to go think, it sets the command
//	finished event and calls "VPrThink" or "VPrAnalyze".

//	If it calls either of these, it's gone until these functions end, and that
//	could be a very, very long time.  But it is still possible to issue engine
//	commands and have them responded to.

//	How it works is that the engine is required to call "VPrCallback" every
//	once in a while, while it is thinking.

//	And what "VPrCallback" does is call into here ("VProcessEngineCommand")
//	and polls to see if there is an engine command ready, and if so it
//	executes it.

//	Many of the engine commands can be handled while the engine is thinking.
//	For those that can be handled, they are just handled normally.

//	For those that can't be handled, something special needs to be done.  The
//	current state is that we're sitting in "VProcessEngineCommand", with a
//	whole bunch of chess engine thinking context in the backtrace, and then
//	the original called "VProcessEngineComand" further back in the trace.  The
//	input thread is either off doing its own thing or it's blocked, waiting
//	for us to finish this command so it can send another one.

//	What is done is that the engine is told to abort its thinking, and then
//	the "VProcessEngineCommand" function returns.  The current command is
//	still executing, so the input thread can't talk to us.

//	The chess engine will then unwind its stack and return, and we end up
//	sitting in the original "VProcessEngine" function, which then loops back
//	up to the top and grabs another engine command, which is, not by
//	coincidence, guaranteed to be the same one it just tried to process.

//	It then processes it and sets the command finished event.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	struct tagEVARS {
	HANDLE	heventCmdPresent;		// These are described above.
	HANDLE	heventCmdFinished;
	char	aszEngineInBuf[256];	// Command input buffer.
	BOOL	fLegal;					// If we get an illegal "setboard"
									//  command, until we get another one
									//  we respond to every attempt to move
									//  by emitting an error message.
									// It should respond to "undo" commands
									//  and so on by emitting and error
									//  message, but I didn't architect this
									//  properly.
}	EVARS;

EVARS s_evars;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Wait until the engine is ready, and then say what you want to say.

void cdecl VSendToEngine(const char * szFmt, ...)
{
	char	aszBuf[2048];
	va_list	lpArgPtr;

	WaitForSingleObject(s_evars.heventCmdFinished, INFINITE);
	va_start(lpArgPtr, szFmt);
	vsprintf(aszBuf, szFmt, lpArgPtr);
	VStripWb(aszBuf);
	strcpy(s_evars.aszEngineInBuf, aszBuf);
	SetEvent(s_evars.heventCmdPresent);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

BOOL VEcmdSetboard(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if (fThinking)
		return fTRUE;
	if (!(s_evars.fLegal = FPrSetboard(pv, sz)))
		VSendToWinboard("tellusererror Illegal position");
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdHint(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if (fThinking)
		VPrHint(pv);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdThink(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if (fThinking) {
		if ((csz < 3) || (!FPrPonderHit(pv, atol(rgsz[1]), rgsz[2])))
			return fTRUE;
		SetEvent(s_evars.heventCmdFinished);
		return fFALSE;
	}
	if (csz > 2)
		if (s_evars.fLegal)
			VPrAdvance(pv, rgsz[2]);
		else
			VSendToWinboard("Illegal move: %s", rgsz[1]);
	SetEvent(s_evars.heventCmdFinished);
	if (s_evars.fLegal)
		VPrThink(pv, atol(rgsz[1]));
	return fFALSE;
}

BOOL VEcmdAnalyze(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if (fThinking)
		return fTRUE;
	if (csz > 1) {
		if (s_evars.fLegal)
			VPrAdvance(pv, rgsz[1]);
		else
			VSendToWinboard("Illegal move: %s", rgsz[1]);
	}
	SetEvent(s_evars.heventCmdFinished);
	if (s_evars.fLegal)
		VPrAnalyze(pv);
	return fFALSE;
}

BOOL VEcmdAbort(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if (fThinking)
		return fTRUE;
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdPost(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	VPrSetPost(pv, fTRUE);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdNoPost(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	VPrSetPost(pv, fFALSE);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdMove(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if (fThinking)
		return fTRUE;
	if (s_evars.fLegal)
		VPrAdvance(pv, rgsz[1]);
	else
		VSendToWinboard("Illegal move: %s", rgsz[1]);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdLevel(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	Assert(csz >= 4);
	VPrSetTimeControl(pv, atoi(rgsz[1]), atol(rgsz[2]), atol(rgsz[3]));
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdSt(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	Assert(csz >= 1);
	VPrSetTimeControl(pv, 1, atol(rgsz[1]), 0);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdSd(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	Assert(csz >= 1);
	VPrSetFixedDepthTimeControl(pv, atoi(rgsz[1]));
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdMovenow(void * pv, char * sz, char * rgsz[],
	int csz, BOOL fThinking)
{
	if (fThinking)
		VPrMoveNow(pv);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdUndo(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if (fThinking)
		return fTRUE;
	if (s_evars.fLegal)
		VPrUndoMove(pv);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdRemove(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if (fThinking)
		return fTRUE;
	if (s_evars.fLegal) {
		VPrUndoMove(pv);
		VPrUndoMove(pv);
	}
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdPonder(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	VPrSetPonder(pv, fTRUE);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

BOOL VEcmdNoponder(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking)
{
	if ((fThinking) && (FPrPondering(pv)))
		return fTRUE;
	VPrSetPonder(pv, fFALSE);
	SetEvent(s_evars.heventCmdFinished);
	return fFALSE;
}

typedef	struct	tagECMD {
	char * sz;
	int (* pfn)(void * pv, char * sz, char * rgsz[], int csz, BOOL fThinking);
}	ECMD, * PECMD;

ECMD const c_argecmdEngine[] = {
	"setboard",		VEcmdSetboard,
	"hint",			VEcmdHint,
	"think",		VEcmdThink,
	"analyze",		VEcmdAnalyze,
	"abort",		VEcmdAbort,
	"post",			VEcmdPost,
	"nopost",		VEcmdNoPost,
	"move",			VEcmdMove,
	"level",		VEcmdLevel,
	"movenow",		VEcmdMovenow,
	"undo",			VEcmdUndo,
	"remove",		VEcmdRemove,
	"ponder",		VEcmdPonder,
	"noponder",		VEcmdNoponder,
	"st",			VEcmdSt,
	"sd",			VEcmdSd,
	NULL,
};

//	If this is called from "DwEngine", "fThinking" will be FALSE.  If it's
//	called from "VPrCallback", it will be TRUE.

void VProcessEngineCommand(void * pv, BOOL fThinking)
{
	int	ibFirst;
	char	aszVec[256];
	char *	argsz[256];
	int	i;
	int	csz;

	if (WaitForSingleObject(s_evars.heventCmdPresent,
		(fThinking) ? 0 : INFINITE) == WAIT_TIMEOUT)
		return;
	strcpy(aszVec, s_evars.aszEngineInBuf);
	csz = CszVectorizeWb(aszVec, argsz, &ibFirst);
	Assert(csz != 0);
	for (i = 0; c_argecmdEngine[i].sz != NULL; i++)
		if (!strcmp(c_argecmdEngine[i].sz, argsz[0])) {
			if ((*c_argecmdEngine[i].pfn)(pv,
				s_evars.aszEngineInBuf + ibFirst, argsz, csz, fThinking)) {
				VPrAbort(pv);
				SetEvent(s_evars.heventCmdPresent);
				return;
			}
			break;
		}
	Assert(c_argecmdEngine[i].sz != NULL);
}

DWORD WINAPI DwEngine(void * pv)
{
	SetEvent(s_evars.heventCmdFinished);
	for (;;)
		VProcessEngineCommand(pv, fFALSE);
	return 0;
}

BOOL FInitEngineThread(void)
{
	void * pv;
	DWORD	dw;
	HANDLE	hthread;

	s_evars.heventCmdPresent = CreateEvent(NULL, FALSE, FALSE, NULL);
	s_evars.heventCmdFinished = CreateEvent(NULL, FALSE, FALSE, NULL);
	if ((pv = PvPrEngineInit()) == NULL)
		return FALSE;
	hthread = CreateThread(NULL, 0, DwEngine, pv, 0, &dw);
	if (hthread == NULL)
		return fFALSE;
	//
	//	I'm going to set the thread priority way down because otherwise the
	//	engine seems to thrash Winboard on a single-processor machine.
	//	Symptoms of this were very slow response when I clicked on the console
	//	window, and win I looked in "winboard.debug" it turns out that the
	//	amount of time between the program's move and the subsequent pondering
	//	hint was very large.  Setting the priority has fixed this, but of
	//	course if someone runs a match between this and another program, with
	//	pondering enable, this one will get trashed.
	//
	SetThreadPriority(hthread, THREAD_PRIORITY_BELOW_NORMAL);
	return fTRUE;
}
