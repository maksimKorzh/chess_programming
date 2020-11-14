
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Epd2Wb
//
//	Copyright (c) 2001, Bruce Moreland.  All rights reserved.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Epd2Wb is free software; you can redistribute it and/or modify it under
//	the terms of the GNU General Public License as published by the Free
//	Software Foundation; either version 2 of the License, or (at your option)
//	any later version.
//
//	Epd2Wb is distributed in the hope that it will be useful, but WITHOUT ANY
//	WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
//	FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
//	details.
//
//	You should have received a copy of the GNU General Public License along
//	with Epd2Wb; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-
//
//	Parts of this program have been adapted from Tim Mann's Winboard project,
//	and are used with permission.
//
//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	fTRUE	1
#define	fFALSE	0

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Here's how this thing works.

//	The engine is started, and this harness drops into "FEATURE_TIMEOUT" mode.
//	During this time, the harness is waiting for "feature" commands from the
//	engine.  It will wait a total of two seconds, while listening to whatever
//	the engine has to say during this time.

//	After two seconds, the harness assumes the engine is done sending feature
//	commands, drops into "MODE_NORMAL" and sends the engine a few initial
//	commands.

//	It's possible that during the FEATURE_TIMEOUT period, the engine could
//	send a "feature done=0" command.  This is used to indicate that feature
//	commands will be coming from the engine until further notice.  If this
//	happens, the harness goes into "FEATURE_NOTIMEOUT" mode, which is like
//	the normal timeout mode except that it will stay in this mode until it
//	gets a "feature done=1" command, which is handled exactly like a normal
//	timeout.

//	Once the harness has entered normal mode and send the initial commands
//	to the engine, the first EPD string will be read from the EPD file.  The
//	harness will process the string, put the engine into "force" mode, and
//	use the "setboard" command to feed the engine the EPD's FEN.

//	Once this has been accomplished, the harness will enter "WAITING" mode
//	and send the engine a ping.

//	The harness will just sit there and do nothing until it receives a "pong"
//	command with the same argument as was passed with the previous "ping"
//	command.  While it is waiting for the "pong", the harness will ignore any
//	analysis coming from the engine.

//	Once it has received the "pong", it will enter "TESTING" mode and tell
//	the engine to analyze.  It will remain in this mode until the test period
//	has elapsed, at which point it will read the next EPD string and repeat
//	the process from that point.

#define	modeFEATURE_TIMEOUT		0
#define	modeFEATURE_NOTIMEOUT	1
#define	modeWAITING				2
#define	modeTESTING				3

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

typedef	unsigned long	TM;

//	Time is measured in milliseconds.  This returns time since system boot.

unsigned TmNow(void)
{
	return GetTickCount();
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	A struct containing info about a child process, in particular the engine.
//	This stuff was adapted (with permission) from similar code in Tim Mann's
//	Winboard.

typedef struct tagCHILDPROC {
	HANDLE hProcess;
	DWORD pid;
	HANDLE hTo;
	HANDLE hFrom;
}	CHILDPROC, * PCHILDPROC;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	isolMAX		1000
#define	cbINPUT_BUF	2048

//	Global variables.

typedef	struct	tagIVARS {
	HANDLE	heventStdinReady;	// Events used by the asynchronous engine
	HANDLE	heventStdinAck;		//  input system.
	char	aszId[256];			// EPD "id" for current test.
	char	aszAnswer[256];		// EPD "bm" for current test, this is double
								//  null-terminated:
								//  <answer1></0><answer2></0></0>
	char	aszAvoid[256];		// EPD "am" for current test, this is double
								//  null-terminated:
								//  <answer1></0><answer2></0></0>
	int	movStart;				// EPD doesn't return the full FEN, it drops
								//  the last two fields.  EPD defines fields
								//  specifically for these, and I have
								//  implemented those fields.  If they aren't
								//  present I use sensible defaults ("0 1").
	int	plyFifty;				// EPD fifty-move counter, see my comments
								//  about "movStart".
	int	mode;					// Harness mode, see definitiions above.
	CHILDPROC	cp;				// The engine.
	TM	tmEnd;					// Time the current wait period is over,
								//  either the feature wait period, or the
								//  current test.
	TM	tmPerMove;				// Amount of time in milliseconds I am going
								//  to spend per test.
	FILE *	pfI;				// File pointer of the EPD file.
	int	cPing;					// Ping counter.  I'll increment this, ping
								//  the engine with it, then wait for a
								//  corresponding pong with the same argument.
	BOOL	fCorrectAnswer;		// TRUE if the last analysis line I got for
								//  this test indicated a correct answer.
	int	cSolved;				// Number of positions solved.
	int	cFailed;				// Number of positions not solved.
	int	cError;					// Number of bogus FEN's found.
	TM	tmFoundIn;				// If "fCorrectAnswer", this is how many
								//  milliseconds, as reported by the engine,
								//  it took to find the correct answer.  This
								//  is "find and hold", meaning if it changes
								//  its mind later, this value is reset.
	int	argsol[isolMAX];		// This is an array of the number of answers
								//  found in array index seconds, rounded
								//  down.  So element zero is the number of
								//  answers found in between 0 an 999 ms.
	char	aszInBuf[			// Input buffer from the engine.
		cbINPUT_BUF];
	int	cSkip;
	int	cLine;					// Line of the EPD file I'm processing.
	BOOL	fError;				// This is true if the EPD FEN seems to be
								//  bogus.
}	IVARS;

IVARS	s_ivars;	// The one instance of this struct.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	cbLINE	80	// Assumed width of the output console.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This converts the last Windows error into a text string, displays it along
//	with some harness-specific error text, and exits.

//	This code was once again adapted (with permission) from code in Tim Mann's
//	Winboard.

void VDisplayLastError(char * sz)
{
	int	len;
	char	aszBuf[512];
	DWORD	dw = GetLastError();

    len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dw, LANG_NEUTRAL, aszBuf, sizeof(aszBuf), NULL);
    if (len > 0) {
		fprintf(stderr, "%s: %s", sz, aszBuf);
		exit(1);
	}
	fprintf(stderr, "%s: error = %ld\n", sz, dw);
	exit(1);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is supposed to kill the engine, but when I call it, it hangs, so I
//	don't call it.

void DestroyChildProcess(PCHILDPROC pcp)
{
	CloseHandle(pcp->hTo);
    if (pcp->hFrom)
		CloseHandle(pcp->hFrom);
    CloseHandle(pcp->hProcess);
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This code was adapted (with permission) from code in Tim Mann's Winboard.
//	It starts the engine while hooking the engine's input and output.

//	This function returns FALSE if it fails, and "GetLastError()" might
//	explain a little more about why it failed.

BOOL FStartProcess(PCHILDPROC pcp, char * szEngine)
{
	HANDLE	hChildStdinRd;
	HANDLE	hChildStdinWr;
	HANDLE	hChildStdoutRd;
	HANDLE	hChildStdoutWr;
	HANDLE	hChildStdinWrDup;
	HANDLE	hChildStdoutRdDup;
	PROCESS_INFORMATION	piProcInfo;
	STARTUPINFO	siStartInfo;
	SECURITY_ATTRIBUTES saAttr;

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	if (!CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
		return fFALSE;
	if (!DuplicateHandle(GetCurrentProcess(), hChildStdoutRd,
		GetCurrentProcess(), &hChildStdoutRdDup, 0,
		FALSE, DUPLICATE_SAME_ACCESS))
		return fFALSE;
	CloseHandle(hChildStdoutRd);
	if (!CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0))
		return fFALSE;
	if (!DuplicateHandle(GetCurrentProcess(), hChildStdinWr,
		GetCurrentProcess(), &hChildStdinWrDup, 0, FALSE,
		DUPLICATE_SAME_ACCESS))
		return fFALSE;
	CloseHandle(hChildStdinWr);
	siStartInfo.cb = sizeof(STARTUPINFO);
	siStartInfo.lpReserved = NULL;
	siStartInfo.lpDesktop = NULL;
	siStartInfo.lpTitle = NULL;
	siStartInfo.dwFlags = STARTF_USESTDHANDLES;
	siStartInfo.cbReserved2 = 0;
	siStartInfo.lpReserved2 = NULL;
	siStartInfo.hStdInput = hChildStdinRd;
	siStartInfo.hStdOutput = hChildStdoutWr;
	siStartInfo.hStdError = hChildStdoutWr;
	if (!CreateProcess(NULL, szEngine, NULL, NULL, TRUE,
		DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP,
		NULL, NULL, &siStartInfo, &piProcInfo))
		return fFALSE;
	CloseHandle(hChildStdinRd);
	CloseHandle(hChildStdoutWr);
	pcp->hProcess = piProcInfo.hProcess;
	pcp->pid = piProcInfo.dwProcessId;
	pcp->hFrom = hChildStdoutRdDup;
	pcp->hTo = hChildStdinWrDup;
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is a thread function.  It will sit here waiting for permission to
//	send the next line.  Once it gets it, it will read a line from the engine,
//	then it will break it up into chunks (delimited by \r\n) and pass them
//	back to the main thread one chunk at a time via "s_ivars.aszInBuf".

DWORD WINAPI DwInput(void * pv)
{
	char	argb[cbINPUT_BUF];
	int	ib = 0;
	int	cb = 0;
	int	ibOut = 0;

	for (;;) {
		WaitForSingleObject(s_ivars.heventStdinAck, INFINITE);
		for (;;)
			if (ib == cb) {		// Empty buffer, read a new line.
				DWORD	dw;

				if (!ReadFile(s_ivars.cp.hFrom,
					argb, sizeof(argb), &dw, NULL))
					VDisplayLastError("Can't read from engine");
				ib = 0;
				cb = dw;
			} else if (argb[ib] == '\r') {	// End of line, return line.
				if (argb[++ib] == '\n')
					ib++;
				s_ivars.aszInBuf[ibOut] = '\0';
				ibOut = 0;
//				printf("I:%s\n", s_ivars.aszInBuf);
				SetEvent(s_ivars.heventStdinReady);
				break;
			} else
				s_ivars.aszInBuf[ibOut++] = argb[ib++];
	}
	return 0;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This writes a line of stuff to the engine, with a '\n' appended.

void cdecl VSendToEngine(const char * szFmt, ...)
{
	char	aszBuf[2048];
	va_list	lpArgPtr;
	int	cb;
	DWORD	dw;

	va_start(lpArgPtr, szFmt);
	vsprintf(aszBuf, szFmt, lpArgPtr);
	cb = strlen(aszBuf);
	aszBuf[cb++] = '\n';
	aszBuf[cb] = '\0';
	if (!WriteFile(s_ivars.cp.hTo, aszBuf, cb, &dw, NULL))
		VDisplayLastError("Can't write to engine");
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This breaks a space-delimited line ("sz") up into pieces and returns a
//	count of the pieces.  The line has null characters poked into it as part
//	of the process.
//
//	"rgsz" is an array of pointers to the pieces.
//
//	"*pibSecond" is the index in "sz" of the second piece.
//
//	This function handles double-quoted matter by deleting the quotes and
//	ignoring spaces that occur within the quoted matter.  So:
//
//		id "tough position"
//
//	is turned into:
//
//		0: id
//		1: tough position

int	CszVectorize(char * sz, char * rgsz[], int * pibSecond)
{
	int	i;
	int	csz;

	for (csz = 0, i = 0; sz[i]; i++)
		if (sz[i] != ' ') {
			BOOL	fInQuote;

			if (sz[i] == '"') {
				fInQuote = fTRUE;
				i++;
			} else
				fInQuote = fFALSE;
			if (csz == 1)
				*pibSecond = i;
			rgsz[csz++] = sz + i;
			for (;; i++) {
				if ((sz[i] == ' ') && (!fInQuote))
					break;
				if ((sz[i] == '"') && (fInQuote))
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

//	This removes the newline at the end of "sz", if it is there.

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

//	These commands are sent to the engine once at the start of the run, after
//	the engine seems to be done sending the harness "feature" commands.

void VPrepEngine(void)
{
	VSendToEngine("new");
	VSendToEngine("level 0 5 0");
	VSendToEngine("post");
	VSendToEngine("easy");
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	In a few places I am going to vectorize a string, then search for the
//	first vectorized element in a command table, then call a function with
//	the original string and the vector.  This structure facilitates this.

typedef	struct	tagCMD {
	char * sz;
	BOOL (* pfn)(char * sz, char * rgsz[], int csz);
}	CMD, * PCMD;

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	EPD processing commands.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This one processes "bm".  It converts the arguments into a double null-
//	terminated string.

BOOL FCmdBm(char * sz, char * rgsz[], int csz)
{
	int	ib;
	int	i;

	ib = 0;
	for (i = 1; i < csz; i++)
		ib += sprintf(s_ivars.aszAnswer + ib, "%s", rgsz[i]) + 1;
	s_ivars.aszAnswer[ib] = '\0';
	return fTRUE;
}

//	This one processes "am".  It converts the arguments into a double null-
//	terminated string.

BOOL FCmdAm(char * sz, char * rgsz[], int csz)
{
	int	ib;
	int	i;

	ib = 0;
	for (i = 1; i < csz; i++)
		ib += sprintf(s_ivars.aszAvoid + ib, "%s", rgsz[i]) + 1;
	s_ivars.aszAvoid[ib] = '\0';
	return fTRUE;
}

//	This one processes "id".  It just remembers the argument.

BOOL FCmdId(char * sz, char * rgsz[], int csz)
{
	if (csz < 2)
		return fTRUE;
	strcpy(s_ivars.aszId, rgsz[1]);
	return fTRUE;
}

//	This one processes "fmvn", which stands for "first move number".

BOOL FCmdFmvn(char * sz, char * rgsz[], int csz)
{
	if (csz < 2)
		return fTRUE;
	s_ivars.movStart = atoi(rgsz[1]);
	if (s_ivars.movStart < 1)
		s_ivars.movStart = 1;
	return fTRUE;
}

//	This one processes "hmvc", which is the fifty-move counter (number of
//	moves since the last reversible move.

BOOL FCmdHmvc(char * sz, char * rgsz[], int csz)
{
	if (csz < 2)
		return fTRUE;
	s_ivars.plyFifty = atoi(rgsz[1]);
	if (s_ivars.plyFifty < 0)
		s_ivars.plyFifty = 0;
	return fTRUE;
}

//	EPD command table.

CMD const c_argcmdEpd[] = {
	"bm",			FCmdBm,		// Best move
	"id",			FCmdId,		// Avoid move.
	"fmvn",			FCmdFmvn,	// Full move number.
	"hmvc",			FCmdHmvc,	// "Half-move clock".  Fifty-move counter.
	NULL,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This gets a line from the EPD file, breaks it up and processes it, gets
//	the engine ready to go, and sends a ping.

//	Later on, something else will see the "pong" from the engine, and the
//	engine will be told to analyze.

//	If there are no more EPD lines, or if this EPD line is seriously broken,
//	this returns FALSE.

BOOL FNextTest(void)
{
	char	aszBuf[256];
	char	aszFen[256];
	int	i;
	int	cSpaces;

	//	Get the line.
	//
	if (fgets(aszBuf, sizeof(aszBuf), s_ivars.pfI) != aszBuf)
		return fFALSE;
	s_ivars.cLine++;
	VStrip(aszBuf);
	//
	//	Break the FEN out of the line.  For some unknown and probably silly
	//	reason, the EPD standard doesn't include the list two fields of the
	//	FEN, so I'll put in some obvious default values, and remember the FEN.
	//
	for (i = cSpaces = 0; aszBuf[i]; i++) {
		if (aszBuf[i] == ' ')
			if (++cSpaces == 4)
				break;
		aszFen[i] = aszBuf[i];
	}
	if (aszBuf[i] == '\0') {	// If I didn't find the FEN I assume I'm
								//  broken.
		printf("Obviously bogus FEN, line %d\n", s_ivars.cLine);
		return fFALSE;
	}
	aszFen[i] = '\0';
	//
	//	I'm sitting at the space after the FEN now.  I'm going to eat EPD
	//	semi-colon delimited fields.  First step is to zero some fields that
	//	I might not find.
	//
	s_ivars.aszId[0] = '\0';
	s_ivars.aszAnswer[0] = '\0';
	s_ivars.aszAvoid[0] = '\0';
	s_ivars.movStart = 1;
	s_ivars.plyFifty = 0;
	//
	//	One pass through this loop for every EPD filed.
	//
	for (;;) {
		char	aszCmd[256];
		int	j;
		char *	argsz[256];
		int	ibSecond;
		int	csz;

		while (aszBuf[i] == ' ')	// Strip preceding spaces.
			i++;
		if (aszBuf[i] == '\0')		// A null here indicates no more fields.
			break;
		//
		//	Grab everything from here until the semi-colon.
		//
		for (j = 0; aszBuf[i]; i++) {
			if (aszBuf[i] == ';') {
				i++;
				break;
			}
			aszCmd[j++] = aszBuf[i];
		}
		aszCmd[j] = '\0';
		//
		//	Break the argument up, then try to process it via the command
		//	table.
		//
		csz = CszVectorize(aszCmd, argsz, &ibSecond);
		if (csz)
			for (j = 0; c_argcmdEpd[j].sz != NULL; j++)
				if (!strcmp(c_argcmdEpd[j].sz, argsz[0])) {
					(*c_argcmdEpd[j].pfn)(aszBuf + ibSecond, argsz, csz);
					break;
				}
	}
	//	At this point the entire EPD is eaten.  Tell the engine to get ready,
	//	display a little output, and put the harness into "WAITING" mode.
	//
	sprintf(aszFen + strlen(aszFen), " %d %d",
		s_ivars.plyFifty, s_ivars.movStart);
	s_ivars.mode = modeWAITING;
	VSendToEngine("force");
	s_ivars.fCorrectAnswer = fFALSE;
	s_ivars.tmFoundIn = -1;
	s_ivars.fError = fFALSE;
	printf("Id:  %s\n", (s_ivars.aszId[0] == '\0') ?
		"(none)" : s_ivars.aszId);
	printf("Fen: %s\n", aszFen);
	if (s_ivars.aszAnswer[0] != '\0') {
		printf("Bm: ");
		for (i = 0; s_ivars.aszAnswer[i] != '\0';) {
			printf(" %s", s_ivars.aszAnswer + i);
			i += strlen(s_ivars.aszAnswer + i) + 1;
		}
		putchar('\n');
	}
	if (s_ivars.aszAvoid[i] != '\0') {
		printf("Am: ");
		for (i = 0; s_ivars.aszAvoid[i] != '\0';) {
			printf(" %s", s_ivars.aszAvoid + i);
			i += strlen(s_ivars.aszAvoid + i) + 1;
		}
		putchar('\n');
	}
	putchar('\n');
	//
	//	Ping the engine, then tell it to analyze.  Until the ping is responded
	//	to, I'll ignore any new analysis lines from the engine.  I don't want
	//	any stupid race conditions, since there is no guarantee that the thing
	//	won't have output some analysis while I'm in here doing this stuff.
	//
	VSendToEngine("ping %d", ++s_ivars.cPing);
	VSendToEngine("setboard %s", aszFen);
	s_ivars.tmEnd = TmNow() + s_ivars.tmPerMove;	// Target end time.
	VSendToEngine("analyze");
	return fTRUE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	When I am done processing "feature" commands from the engine, I'll do
//	this.  The funtion sends a few simple commands to the engine, then tries
//	to set up the first EPD line.

BOOL FFeatureTimeout(void)
{
	VPrepEngine();
	if (!FNextTest())
		return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	Engine commands.

//	These functions are called in reaction to command received from the
//	engine.

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	"feature" command.  All I care about are "feature done=0", which tells the
//	engine to ignore the two-second timeout, and "feature done=1", which is
//	treated as a feature timeout.

BOOL FCmdFeature(char * sz, char * rgsz[], int csz)
{
	if (csz > 1)
		if (!strcmp(rgsz[1], "done=0")) {
			if (s_ivars.mode == modeFEATURE_TIMEOUT)
				s_ivars.mode = modeFEATURE_NOTIMEOUT;
		} else if (!strcmp(rgsz[1], "done=1"))
			if ((s_ivars.mode == modeFEATURE_NOTIMEOUT) ||
				(s_ivars.mode == modeFEATURE_TIMEOUT))
				if (!FFeatureTimeout())
					return fFALSE;
	return fTRUE;
}

//	A "pong" indicates that the engine is listening to me and is ready to
//	start analyzing the current position, so I change the mode to TESTING.

BOOL FCmdPong(char * sz, char * rgsz[], int csz)
{
	if (csz == 0)
		return fTRUE;
	if (atoi(rgsz[1]) == s_ivars.cPing)
		s_ivars.mode = modeTESTING;
	return fTRUE;
}

//	This will be received if I send a bogus FEN to the engine.  I'm not going
//	to look at the error message at all.  It is supposed to be
//	"Illegal position", but I am not going to count on that.

BOOL FCmdTellusererror(char * sz, char * rgsz[], int csz)
{
	if (s_ivars.mode != modeTESTING)
		return fTRUE;
	s_ivars.fError = fTRUE;
	return fTRUE;
}

//	Engine command table.

CMD const c_argcmdEngine[] = {
	"feature",			FCmdFeature,
	"pong",				FCmdPong,
	"tellusererror",	FCmdTellusererror,
	NULL,
};

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

BOOL FInteger(char * sz)
{
	if ((*sz == '-') || (*sz == '+'))
		sz++;
	while (isdigit(*sz))
		sz++;
	return (*sz == '\0');
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This checks to see if "sz" is a correct answer to the current test
//	position.

BOOL FCorrectAnswer(char * sz)
{
	int	i;

	if ((*sz == '-') || (*sz == '+'))	// Ignore my fail-high and fail-low
		sz++;							//  indicators.
	while (isdigit(*sz))
		sz++;
	while (*sz == '.')
		sz++;
	if (s_ivars.aszAnswer[0] != '\0') {
		for (i = 0; s_ivars.aszAnswer[i] != '\0';) {
			if (!strcmp(s_ivars.aszAnswer + i, sz))
				return fTRUE;
			i += strlen(s_ivars.aszAnswer + i) + 1;
		}
		return fFALSE;
	}
	if (s_ivars.aszAvoid[0] != '\0') {
		for (i = 0; s_ivars.aszAvoid[i] != '\0';) {
			if (!strcmp(s_ivars.aszAvoid + i, sz))
				return fFALSE;
			i += strlen(s_ivars.aszAvoid + i) + 1;
		}
		return fTRUE;
	}
	return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

BOOL FIsMove(char * sz)
{
	if (*sz == '+')
		sz++;
	else if (*sz == '-')
		sz++;
	while (isdigit(*sz))
		sz++;
	while (*sz == '.')
		sz++;
	if (*sz != '\0')
		return fTRUE;
	return fFALSE;
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This function handles a line of analysis nonsense sent by the engine.  At
//	the start of the function I don't know for sure that it's actually a line
//	of analysis.  I do some dummy checking to see if it starts with four
//	integers, and if so I assume it's an analysis line.

void VAnalysisLine(char * rgsz[], int csz)
{
	int	plyDepth;
	int	valScore;
	TM	tm;
	long	nodes;
	int	i;
	char	aszResult[16];
	int	cb;
	int	cbStats;

	if (csz < 4)
		return;
	for (i = 0; i < 4; i++)
		if (!FInteger(rgsz[i]))
			return;
	//
	//	It had four integers, break them out.
	//
	plyDepth = atoi(rgsz[0]);
	valScore = atoi(rgsz[1]);
	tm = atol(rgsz[2]) * 10;
	nodes = atol(rgsz[3]);
	//
	//	Check to see if the answer is correct.  If so, record the time, if
	//	not, clear the time.
	//
	//	This loop is a little gross, because I might decide that the first
	//	"move" or two aren't really moves (they might be move numbers or some
	//	symbol that indicates that black is on move), so I'll skip them.
	//
	for (i = 4 + s_ivars.cSkip;; i++)
		if (i >= csz) {
lblWrong:	s_ivars.fCorrectAnswer = fFALSE;
			strcpy(aszResult, "no");
			s_ivars.tmFoundIn = -1;
			break;
		} else if (FIsMove(rgsz[i])) {
			if (!FCorrectAnswer(rgsz[i]))
				goto lblWrong;
			s_ivars.fCorrectAnswer = fTRUE;
			strcpy(aszResult, "yes");
			if (s_ivars.tmFoundIn == -1)
				s_ivars.tmFoundIn = tm;
			break;
		}
	//
	//	Output the line in a somewhat prettied up format.
	//
	cb = cbStats = printf("%-3s %2d %9ld %+6d %12ld",
		aszResult, plyDepth, tm, valScore, nodes);
	for (i = 4; i < csz; i++) {
		int	cbCur = strlen(rgsz[i]) + 1;

		if (cb + cbCur >= cbLINE) {
			putchar('\n');
			for (cb = 0; cb < cbStats; cb++)
				putchar(' ');
		}
		printf(" %s", rgsz[i]);
		cb += cbCur;
	}
	putchar('\n');
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

//	This is called at the end, and it just dumps out the number solved, and
//	a breakdown of how many problems were solved in less than N seconds.

void VDumpResults(void)
{
	int	isolHi;
	int	i;
	
	for (isolHi = isolMAX - 1; isolHi >= 0; isolHi--)
		if (s_ivars.argsol[isolHi])
			break;
	printf("Results:\n\n");
	if (s_ivars.cSolved) {
		if (isolHi < 0)
			printf("No problems solved in < %d seconds\n", isolMAX);
		else {
			int	cTotal = 0;

			printf("<Sec Solved Total\n");
			printf("---- ------ ------\n");
			for (i = 0; i <= isolHi; i++) {
				cTotal += s_ivars.argsol[i];
				printf("%4d %6d %6d\n", i + 1, s_ivars.argsol[i], cTotal);
			}
		}
		putchar('\n');
	}
	printf("%d problem%s solved.\n", s_ivars.cSolved,
		(s_ivars.cSolved == 1) ? "" : "s");
	printf("%d problem%s unsolved.\n", s_ivars.cFailed,
		(s_ivars.cFailed == 1) ? "" : "s");
	if (s_ivars.cError)
		printf("%d error%s found!\n",
			s_ivars.cError, (s_ivars.cError == 1) ? "" : "s");
}

//	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-	-

#define	uHOUR_FROM_MILLI(tm)	(unsigned)((TM)(tm) / 3600000)
#define	uMIN_FROM_MILLI(tm)		(unsigned)(((TM)(tm) / 60000) % 60)
#define	uSEC_FROM_MILLI(tm)		(unsigned)(((TM)(tm) / 1000) % 60)
#define	uMILLI_FROM_MILLI(tm)	(unsigned)((TM)(tm) % 1000)

//	This is this program's main loop.

void VProcess(void)
{
	int	i;

	//	The function starts in FEATURE_TIMEOUT mode.  It's going to sit here
	//	and collect feature commands for two seconds or perhaps longer or
	//	shorter if directed by the feature commands.
	//
	VSendToEngine("xboard");
	VSendToEngine("protover 2");
	s_ivars.mode = modeFEATURE_TIMEOUT;
	s_ivars.tmEnd = TmNow() + 2000;
	//
	//	Tell the input thread that I'm ready for a line.
	//
	SetEvent(s_ivars.heventStdinAck);
	//
	//	Forever loop.  If this exits, it will be because the EPD file is
	//	exhausted.
	//
	for (;;) {
		int	csz;
		char	aszBuf[512];
		char	aszVec[512];
		char *	argsz[256];
		int	ibSecond;

		if (s_ivars.mode == modeFEATURE_TIMEOUT) {
			TM	tm;

			//	In FEATURE_TIMEOUT mode I'll wait some small amount of time
			//	for a line, and if this expires, the features are done and
			//	I start processing the EPD.
			//
			tm = TmNow();
			if ((tm >= s_ivars.tmEnd) ||
				(WaitForSingleObject(s_ivars.heventStdinReady,
				s_ivars.tmEnd - tm) == WAIT_TIMEOUT)) {
				if (!FFeatureTimeout())
					return;
				continue;	// There's no command, so back to the top of the
							//  loop.
			}
		} else if (s_ivars.mode == modeTESTING) {
			TM	tm;

			//	In TESTING mode, I'm running a test, so I'll wait for input
			//	only until the text time has expired.
			//
			//	If I run out of time I'll write some output then try to start
			//	a new test.
			//
			tm = TmNow();
			if ((tm >= s_ivars.tmEnd) ||
				(WaitForSingleObject(s_ivars.heventStdinReady,
				s_ivars.tmEnd - tm) == WAIT_TIMEOUT)) {
				printf("\nResult:   ");
				if (s_ivars.fError) {
					printf("ERROR!!");
					s_ivars.cError++;
				} else if (s_ivars.fCorrectAnswer) {
					printf("Success");
					s_ivars.cSolved++;
				} else {
					printf("Failure");
					s_ivars.cFailed++;
				}
				putchar('\n');
				if (s_ivars.fCorrectAnswer) {
					int	isol;

					printf("Found in: %ld ms (%02d:%02d:%02d.%03d)\n",
						s_ivars.tmFoundIn,
						uHOUR_FROM_MILLI(s_ivars.tmFoundIn),
						uMIN_FROM_MILLI(s_ivars.tmFoundIn),
						uSEC_FROM_MILLI(s_ivars.tmFoundIn),
						uMILLI_FROM_MILLI(s_ivars.tmFoundIn));
					isol = s_ivars.tmFoundIn / 1000;
					if (isol < isolMAX)
						s_ivars.argsol[isol]++;
				}
				putchar('\n');
				for (i = 0; i < cbLINE - 1; i++)
					putchar('-');
				putchar('\n');
				putchar('\n');
				if (!FNextTest())
					return;
				continue;	// There's no command, so back to the top of the
							//  loop.
			}
		} else
			//	In any other mode I'll wait forever for input.
			//
			WaitForSingleObject(s_ivars.heventStdinReady, INFINITE);
		//
		//	If I'm here, it's because the input request did not time out, so
		//	I received an input line, which is presumed to contain a command
		//	or some analysis.
		//
		//	Vectorize the line and try to process it as a command.
		//
		strcpy(aszBuf, s_ivars.aszInBuf);
		strcpy(aszVec, s_ivars.aszInBuf);
		csz = CszVectorize(aszVec, argsz, &ibSecond);
		if (csz) {
			for (i = 0; c_argcmdEngine[i].sz != NULL; i++)
				if (!strcmp(c_argcmdEngine[i].sz, argsz[0])) {
					if (!(*c_argcmdEngine[i].pfn)(aszBuf + ibSecond,
						argsz, csz))
						return;
					break;
				}
			if (s_ivars.mode == modeTESTING)
				//
				//	This line didn't contain a command, but it might contain
				//	some analysis, so treat it as such.
				//
				VAnalysisLine(argsz, csz);
		}
		//	Tell the input thread that I'm ready for another line of stuff.
		//
		SetEvent(s_ivars.heventStdinAck);
	}
}

int main(int argc, char * argv[])
{
	DWORD	dw;

	if (argc < 4) {
		fprintf(stderr, "Usage: epd2wb <engine> <EPD> <seconds> [skip]\n");
		exit(1);
	}
	if ((s_ivars.pfI = fopen(argv[2], "r")) == NULL) {
		perror(argv[2]);
		exit(1);
	}
	s_ivars.tmPerMove = atoi(argv[3]) * 1000;
	if (argc >= 5)
		s_ivars.cSkip = atoi(argv[4]);
	if (!FStartProcess(&s_ivars.cp, argv[1]))
		VDisplayLastError("Can't start engine");
	s_ivars.heventStdinReady = CreateEvent(NULL, FALSE, FALSE, NULL);
	s_ivars.heventStdinAck = CreateEvent(NULL, FALSE, FALSE, NULL);
	CreateThread(NULL, 0, DwInput, NULL, 0, &dw);
	VProcess();
	VDumpResults();
//	DestroyChildProcess(&s_ivars.cp);	// This hangs so it's commented out.
	return 1;
}
