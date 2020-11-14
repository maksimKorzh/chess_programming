
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

static char const s_aszModule[] = __FILE__;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	wmodeANALYZE	0
#define	wmodeTHINK		1

typedef	struct	tagIVARS {
	int	wmode;
	BOOL	fForce;
	BOOL	fDidInitEngine;
	unsigned long tmMyTime;
}	IVARS;

IVARS	s_ivars;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is just a normal printf with a "fflush" after it.

void cdecl VSendToWinboard(const char * szFmt, ...)
{
	char	aszBuf[256];
	va_list	lpArgPtr;

	va_start(lpArgPtr, szFmt);
	vsprintf(aszBuf, szFmt, lpArgPtr);
	printf("%s\n", aszBuf);
	fflush(stdout);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VAssertFailedW(const char * szMod, int iLine)
{
	VSendToWinboard("Assert Failed: %s+%d\n", szMod, iLine);
	exit(1);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VStripWb(char * sz)
{
	int	i;

	for (i = 0; sz[i]; i++)
		if (sz[i] == '\n') {
			sz[i] = '\0';
			break;
		}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

int	CszVectorizeWb(char * sz, char * rgsz[], int * pibFirst)
{
	int	i;
	int	csz;

	for (csz = 0, i = 0; sz[i]; i++)
		if (sz[i] != ' ') {
			if (csz == 1)
				*pibFirst = i;
			rgsz[csz++] = sz + i;
			for (;; i++) {
				if (sz[i] == ' ')
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

void VErrorWb(char * szMsg, char * szCmd)
{
	VSendToWinboard("Error (%s): %s", szMsg, szCmd);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	I'm going to init the engine after sending "done=0".  I'm a little
//	worried about doing this at some other time.  The engine I'm working on
//	now isn't that likely to take two seconds to boot, but another engine
//	might, and that's how long WinBoard waits before deciding that you are
//	protover one.

BOOL FCmdProtover(char * sz, char * rgsz[], int csz)
{
	static char const * s_argszFeatures[] = {
		"done=0",		// <-- Must be first.
		"ping=1",
		"setboard=1",
		"sigint=0",
		"sigterm=0",
		"colors=0",
		"name=0",
		"analyze=1",
		"done=1",
		NULL,
	};
	int	i;

	//	If I received a "protover" command it implies level two or higher.
	//
	for (i = 0; s_argszFeatures[i] != NULL; i++) {
		VSendToWinboard("feature %s", s_argszFeatures[i]);
		if ((i == 0) && (!s_ivars.fDidInitEngine)) {
			if (!FInitEngineThread())
				return fFALSE;
			s_ivars.fDidInitEngine = fTRUE;
		}
	}
	return fTRUE;
}

BOOL FCmdNew(char * sz, char * rgsz[], int csz)
{
	s_ivars.fForce = fFALSE;
	VSendToEngine("setboard "
		"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
	return fTRUE;
}

BOOL FCmdQuit(char * sz, char * rgsz[], int csz)
{
	return fFALSE;
}

BOOL FCmdForce(char * sz, char * rgsz[], int csz)
{
	s_ivars.fForce = fTRUE;
	return fTRUE;
}

BOOL FCmdGo(char * sz, char * rgsz[], int csz)
{
	s_ivars.fForce = fFALSE;
	s_ivars.wmode = wmodeTHINK;
	VSendToEngine("think %ld", s_ivars.tmMyTime);
	return fTRUE;
}

BOOL FCmdLevel(char * szCmd, char * rgsz[], int csz)
{
	int	iMoves;
	unsigned	tmMinutes;
	unsigned	tmSeconds;
	unsigned	tmIncr;
	char * sz;

	Assert(csz >= 4);
	iMoves = atoi(rgsz[1]);
	sz = rgsz[2];
	tmMinutes = tmSeconds = 0;
	for (; *sz; sz++) {
		if (*sz == ':')
			break;
		Assert(isdigit(*sz));
		tmMinutes = tmMinutes * 10 + *sz - '0';
	}
	if (*sz == ':')
		for (; *sz; sz++) {
			Assert(isdigit(*sz));
			tmSeconds = tmSeconds * 10 + *sz - '0';
		}
	tmIncr = atol(rgsz[3]);
	VSendToEngine("level %d %ld %ld",
		iMoves, (tmMinutes * 60 + tmSeconds) * 1000, tmIncr * 1000);
	return fTRUE;
}

BOOL FCmdSt(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("st %lu", atol(sz) * 1000);
	return fTRUE;
}

BOOL FCmdSd(char * sz, char * rgsz[], int csz)
{
	int	plyDepth;

	if (csz < 2)
		return fTRUE;
	plyDepth = atoi(rgsz[1]);
	if (plyDepth < 1)
		plyDepth = 1;
	VSendToEngine("sd %d", plyDepth);
	return fTRUE;
}

BOOL FCmdTime(char * sz, char * rgsz[], int csz)
{
	if (csz <= 1)
		return fTRUE;
	s_ivars.tmMyTime = atol(rgsz[1]) * 10;
	return fTRUE;
}

BOOL FCmdHook(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("movenow");
	return fTRUE;
}

BOOL FCmdPing(char * sz, char * rgsz[], int csz)
{
	VSendToWinboard("pong %s", sz);
	return fTRUE;
}

BOOL FCmdSetboard(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("setboard %s", sz);
	return fTRUE;
}

BOOL FCmdHint(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("hint");
	return fTRUE;
}

BOOL FCmdUndo(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("undo");
	if (s_ivars.wmode == wmodeANALYZE)
		VSendToEngine("analyze");
	return fTRUE;
}

BOOL FCmdRemove(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("remove");
	return fTRUE;
}

BOOL FCmdHard(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("ponder");
	return fTRUE;
}

BOOL FCmdEasy(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("noponder");
	return fTRUE;
}

BOOL FCmdPost(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("post");
	return fTRUE;
}

BOOL FCmdNopost(char * sz, char * rgsz[], int csz)
{
	VSendToEngine("nopost");
	return fTRUE;
}

BOOL FCmdAnalyze(char * sz, char * rgsz[], int csz)
{
	s_ivars.fForce = fFALSE;
	s_ivars.wmode = wmodeANALYZE;
	VSendToEngine("analyze");
	return fTRUE;
}

BOOL FCmdExit(char * sz, char * rgsz[], int csz)
{
	s_ivars.wmode = wmodeTHINK;
	VSendToEngine("abort");
	return fTRUE;
}

BOOL FCmdUnimplemented(char * sz, char * rgsz[], int csz)
{
	Assert(csz);
	VErrorWb("Unimplemented", rgsz[0]);
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	struct	tagCMD {
	char * sz;
	BOOL (* pfn)(char * sz, char * rgsz[], int csz);
}	CMD, * PCMD;

CMD const c_argcmd[] = {
	"protover",		FCmdProtover,
	"xboard",		NULL,
	"accepted",		NULL,
	"rejected",		NULL,
	"new",			FCmdNew,
	"variant",		FCmdUnimplemented,
	"quit",			FCmdQuit,
	"random",		NULL,
	"force",		FCmdForce,
	"go",			FCmdGo,
	"playother",	FCmdUnimplemented,
	"white",		FCmdUnimplemented,
	"black",		FCmdUnimplemented,
	"level",		FCmdLevel,
	"st",			FCmdSt,
	"sd",			FCmdSd,
	"time",			FCmdTime,
	"otim",			NULL,
	"usermove",		FCmdUnimplemented,
	"?",			FCmdHook,
	".",			NULL,
	"ping",			FCmdPing,
	"draw",			NULL,
	"result",		NULL,
	"setboard",		FCmdSetboard,
	"edit",			FCmdUnimplemented,
	"hint",			FCmdHint,
	"bk",			NULL,
	"undo",			FCmdUndo,
	"remove",		FCmdRemove,
	"hard",			FCmdHard,
	"easy",			FCmdEasy,
	"post",			FCmdPost,
	"nopost",		FCmdNopost,
	"analyze",		FCmdAnalyze,
	"exit",			FCmdExit,
	"name",			NULL,
	"rating",		NULL,
	"ics",			NULL,
	"computer",		NULL,
	"pause",		FCmdUnimplemented,
	"resume",		FCmdUnimplemented,
	NULL,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

void VExecuteMove(char * sz, char * rgsz[], int csz)
{
	Assert(csz);
	if (s_ivars.fForce)
		VSendToEngine("move %s", rgsz[0]);
	else
		switch (s_ivars.wmode) {
		case wmodeANALYZE:
			VSendToEngine("analyze %s", rgsz[0]);
			break;
		case wmodeTHINK:
			VSendToEngine("think %ld %s", s_ivars.tmMyTime, rgsz[0]);
			break;
		}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is the input thread.  The program will spin in here until it gets a
//	command that tells it to exit.

void VReadFromWinboard(void)
{
	s_ivars.wmode = wmodeTHINK;
	for (;;) {
		char	aszBuf[256];
		char	aszVec[256];
		char *	argsz[256];
		int	ibFirst;
		int	csz;
		int	i;

		if (fgets(aszBuf, sizeof(aszBuf), stdin) != aszBuf)
			break;
		VStripWb(aszBuf);
		strcpy(aszVec, aszBuf);
		csz = CszVectorizeWb(aszVec, argsz, &ibFirst);
		if (csz != 0)
			if ((isdigit(argsz[0][1])) && (isdigit(argsz[0][3]))) {
				if (!s_ivars.fDidInitEngine) {
					if (!FInitEngineThread())
						return;
					s_ivars.fDidInitEngine = fTRUE;
				}
				VExecuteMove(aszBuf + ibFirst, argsz, csz);
			} else {
				for (i = 0; c_argcmd[i].sz != NULL; i++)
					if (!strcmp(c_argcmd[i].sz, argsz[0])) {
						if (c_argcmd[i].pfn != NULL) {
							//
							//	If I haven't initialized the engine, check to
							//	see if this is the "protover" command, which
							//	is where I am supposed to do this.  If I get
							//	a real command before "protover", something
							//	weird happened, but I can still handle it.
							//
							if ((!s_ivars.fDidInitEngine) &&
								(strcmp(c_argcmd[i].sz, "protover"))) {
								if (!FInitEngineThread())
									return;
								s_ivars.fDidInitEngine = fTRUE;
							}
							if (!(*c_argcmd[i].pfn)(aszBuf + ibFirst,
								argsz, csz))
								return;
						}
						break;
					}
				if (c_argcmd[i].sz == NULL)
					VErrorWb("Unknown command", argsz[0]);
			}
	}
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

int main(int argc, char * argv[])
{
	s_ivars.fDidInitEngine = fFALSE;
	VReadFromWinboard();
	return 1;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
