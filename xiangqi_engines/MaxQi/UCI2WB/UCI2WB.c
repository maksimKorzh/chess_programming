/************************* UCI2WB by H.G.Muller ****************************/

#define VERSION "1.1"

#include <stdio.h>
#include <windows.h>
#include <io.h>
#include <fcntl.h>

#ifdef _MSC_VER 
#define SLEEP() Sleep(1) 
#else 
#define SLEEP() usleep(10) 
#endif 

// Set VARIANTS for in WinBoard variant feature. (With -s option this will always be reset to use "shogi".)
#  define VARIANTS "normal,xiangqi"

#define WHITE 0
#define BLACK 1
#define NONE  2
#define ANALYZE 3

char move[2000][10], checkOptions[8192], iniPos[256], hashOpt[20], pause, pondering, ponder, post, hasHash, c, sc='c';
int mps, tc, inc, stime, depth, myTime, hisTime, stm, computer = NONE, memory, oldMem=0, cores, moveNr, lastDepth, lastScore, startTime;
int statDepth, statScore, statNodes, statTime, currNr; char currMove[20]; // for analyze mode

FILE *toE, *fromE;
HANDLE process;
int pid;
DWORD thread_id;

void
StartSearch(char *ponder)
{	// send the 'go' command to engine. Suffix by ponder.
	int nr = moveNr + (ponder[0] != 0); // we ponder for one move ahead!
	fprintf(toE, "\ngo wtime %d btime %d", stm == WHITE ^ sc=='s' ? myTime : hisTime, stm == BLACK ^ sc=='s' ? myTime : hisTime);
	printf(    "\n# go wtime %d btime %d", stm == WHITE ^ sc=='s' ? myTime : hisTime, stm == BLACK ^ sc=='s' ? myTime : hisTime);
	if(stime > 0) fprintf(toE, " movetime %d", stime),printf(" movetime %d", stime); else
	if(mps) fprintf(toE, " movestogo %d", mps*(nr/(2*mps)+1)-nr/2),printf(" movestogo %d", mps*(nr/(2*mps)+1)-nr/2);
	if(inc) fprintf(toE, " winc %d binc %d", inc, inc),printf(" winc %d binc %d", inc, inc);
	if(depth > 0) fprintf(toE, " depth %d", depth),printf(" depth %d", depth);
	fprintf(toE, "%s\n", ponder);
	printf("%s\n", ponder);
}

void
StopPonder(int pondering)
{
	if(!pondering) return;
	pause = 1;
	fprintf(toE, "stop\n");printf("# stop\n"); // note: 'pondering' remains set until engine acknowledges 'stop' with 'bestmove'
	while(pause) SLEEP(); // wait for engine to acknowledge 'stop' with 'bestmove'.
}

void
LoadPos(int moveNr)
{
	int j;
	fprintf(toE, "%s moves", iniPos);
	printf(    "# %s moves", iniPos);
	for(j=0; j<moveNr; j++) fprintf(toE, " %s", move[j]),printf(" %s", move[j]);
}

void
Engine2GUI()
{
    char line[1024], command[256];

    while(1) {
	int i=0, x; char *p;

	fflush(stdout); fflush(toE);
	while((line[i] = x = fgetc(fromE)) != EOF && line[i] != '\n') i++;
	line[++i] = 0;
	if(x == EOF) exit(0);
printf("# engine said: %s", line);
	sscanf(line, "%s", command);
	if(!strcmp(command, "bestmove")) {
	    if(pause) { pondering = pause = 0; continue; } // bestmove was reply to ponder miss or analysis result; ignore.
	    // move was a move to be played
	    if(strstr(line+9, "resign")) { printf("resign\n"); computer = NONE; }
	    if(strstr(line+9, "(none)") || strstr(line+9, "null") ||
	       strstr(line+9, "0000")) { printf("%s\n", lastScore < -99999 ? "resign" : "1/2-1/2 {stalemate}"); computer = NONE; }
	    sscanf(line, "bestmove %s", move[moveNr++]);
	    myTime -= (GetTickCount() - startTime)*1.02 + inc; // update own clock, so we can give correct wtime, btime with ponder
	    if(mps && ((moveNr+1)/2) % mps == 0) myTime += tc; if(stime) myTime = stime; // new session or move starts
	    // first start a new ponder search, if pondering is on and we have a move to ponder on
	    if(p = strstr(line+9, "ponder")) {
	      if(computer != NONE && ponder) {
		sscanf(p+7, "%s", move[moveNr]);
printf("# ponder on %s\n", move[moveNr]);
		LoadPos(moveNr+1); // load position
		// and set engine pondering
		pondering = 1; lastDepth = 1;
		StartSearch(" ponder");
	      }
	      p[-1] = '\n'; *p = 0; // strip off ponder move
	    }
	    if(sc == 's') {
	      // convert USI move to WB format
	      line[11] = 'a' + '9' - line[11];
	      line[12] = 'a' + '9' - line[12];
	      if(line[10] == '*') { // drop
		line[10] = '@';
	      } else {
		line[9]  = 'a' + '9' - line[9];
		line[10] = 'a' + '9' - line[10];
		if((stm == WHITE ? (line[10]>'6' || line[12]>'6') : (line[10] < '4' || line[12] < '4')) && line[13] != '+')
		     line[13] = '=', line[14] = 0;
	      }
	    }
	    printf("move %s\n", line+9); // send move to GUI
	    if(lastScore == 100001)      { printf("%s {mate}\n", stm == WHITE ? "1-0" : "0-1"); computer = NONE; }
	    stm = WHITE+BLACK - stm;
	}
	else if(!strcmp(command, "info")) {
	    int d=0, s=0, t=0, n=0;
	    char *pv;
	    if(!post) continue;
	    if(strstr(line, "info string ") == line) printf("%d 0 0 0 %s", lastDepth, line+12); else {
		if(p = strstr(line+4, " depth "))      sscanf(p+7, "%d", &d), statDepth = d;
		if(p = strstr(line+4, " score cp "))   sscanf(p+10, "%d", &s), statScore = s; else
		if(p = strstr(line+4, " score mate ")) sscanf(p+12, "%d", &s), s += s>0 ? 100000 : -100000, statScore = s; else
		if(p = strstr(line+4, " score "))      sscanf(p+7, "%d", &s), statScore = s;
		if(p = strstr(line+4, " nodes "))      sscanf(p+7, "%d", &n), statNodes = n;
		if(p = strstr(line+4, " time "))       sscanf(p+6, "%d", &t), t /= 10, statTime = t;
		if(p = strstr(line+4, " currmove "))   sscanf(p+10,"%s", currMove);
		if(p = strstr(line+4, " currmovenumber ")) sscanf(p+16,"%d", &currNr);
		if(pv = strstr(line+4, " pv ")) // convert PV info to WB thinking output
		    printf("%3d  %6d %6d %10d %s", lastDepth=d, lastScore=s, t, n, pv+4);
	    }
	}
	else if(!strcmp(command, "option")) { // USI option: extract data fields
	    char name[80], type[80], buf[1024], val[256], *q;
	    int min=0, max=1e9;
	    if(p = strstr(line+6, " type ")) sscanf(p+1, "type %s", type), *p = '\n';
	    if(p = strstr(line+6, " min "))  sscanf(p+1, "min %d", &min), *p = '\n';
	    if(p = strstr(line+6, " max "))  sscanf(p+1, "max %d", &max), *p = '\n';
	    if(p = strstr(line+6, " default "))  sscanf(p+1, "default %[^\n]*", val), *p = '\n';
	    if(p = strstr(line+6, " name ")) sscanf(p+1, "name %[^\n]*", name);
	    if(!strcmp(name, "Hash") || !strcmp(name, "USI_Hash")) {
		memory = oldMem = atoi(val); hasHash = 1; 
		strcpy(hashOpt, name);
		continue;
	    }
	    // pass on engine-defined option as WB option feature
	    if(!strcmp(type, "filename")) type[4] = 0;
	    sprintf(buf, "feature option=\"%s -%s", name, type); q = buf + strlen(buf);
	    if(     !strcmp(type, "file")
	         || !strcmp(type, "string")) sprintf(q, " %s\"\n", val);
	    else if(!strcmp(type, "spin"))   sprintf(q, " %d %d %d\"\n", atoi(val), min, max);
	    else if(!strcmp(type, "check"))  sprintf(q, " %d\"\n", strcmp(val, "true") ? 0 : 1), strcat(checkOptions, name);
	    else if(!strcmp(type, "button")) sprintf(q, "\"\n");
	    else if(!strcmp(type, "combo")) {
		if(p = strstr(line+6, " default "))  sscanf(p+1, "default %s", type); // current setting
		min = 0; p = line+6;
		while(p = strstr(p, " var ")) {
		    sscanf(p += 5, "%s", val); // next choice
		    sprintf(buf + strlen(buf), "%s%s%s", min++ ? " /// " : " ", strcmp(type, val) ? "" : "*", val);
		}
		strcat(q, "\"\n");
	    }
	    else buf[0] = 0; // ignore unrecognized option types
	    if(buf[0]) printf("%s", buf);
	}
	else if(!strcmp(command, "id")) {
	    char name[256];
	    if(sscanf(line, "id name %s", name) == 1) printf("feature myname=\"%s (U%cI2WB)\"\n", name, sc-32);
	}
	else if(!strcmp(command, "readyok")) pause = 0; // resume processing of GUI commands
	else if(sscanf(command, "u%ciok", &c)==1 && c==sc)   printf("feature smp=1 memory=%d done=1\n", hasHash); // done with options
    }
}

void
GUI2Engine()
{
    char line[256], command[256];

    while(1) {
	int i, x;

	if(computer == stm || computer == ANALYZE) {
printf("# start search\n");
	    LoadPos(moveNr); // load position
	    // and set engine thinking (note USI swaps colors!)
	    startTime = GetTickCount();
	    if(computer == ANALYZE) fprintf(toE, "\ngo infinite\n"), printf("\ngo infinite\n");
	    else StartSearch("");
	}
      nomove:
	fflush(toE); fflush(stdout);
	i = 0; while((x = getchar()) != EOF && (line[i] = x) != '\n') i++;
	line[++i] = 0; if(x == EOF) { printf("# EOF\n"); exit(-1); }
	sscanf(line, "%s", command);
	while(pause) SLEEP(); // wait for readyok
	if(!strcmp(command, "new")) {
	    computer = BLACK; moveNr = 0; depth = -1;
	    stm = WHITE; strcpy(iniPos, "position startpos");
	    if(memory != oldMem && hasHash) fprintf(toE, "setoption name %s value %d\n", hashOpt, memory);
	    oldMem = memory;
	    // we can set other options here
	    pause = 1; // wait for option settings to take effect
	    fprintf(toE, "isready\n");
	    fprintf(toE, "u%cinewgame\n", sc);
	}
	else if(!strcmp(command, "usermove")) {
	    if(sc == 's') {
	      // convert input move to USI format
	      if(line[10] == '@') { // drop
		line[10] = '*';
	      } else {
		line[9]  = 'a' + '9' - line[9];
		line[10] = 'a' + '9' - line[10];
	      }
	      line[11] = 'a' + '9' - line[11];
	      line[12] = 'a' + '9' - line[12];
	      if(line[13] == '=') line[13] = 0; // no '=' in USI format!
	      else if(line[13] != '\n') line[13] = '+'; // cater to WB 4.4 bug :-(
	    }
	    sscanf(line, "usermove %s", command); // strips off linefeed
	    stm = WHITE+BLACK - stm;
	    // when pondering we either continue the ponder search as normal search, or abort it
	    if(pondering || computer == ANALYZE) {
		if(pondering && !strcmp(command, move[moveNr])) { // ponder hit
		    pondering = 0; moveNr++; startTime = GetTickCount(); // clock starts running now
		    fprintf(toE, "ponderhit\n"); printf("# ponderhit\n");
		    goto nomove;
		}
		StopPonder(1);
	    }
	    sscanf(line, "usermove %s", move[moveNr++]); // possibly overwrites ponder move
	}
	else if(!strcmp(command, "level")) {
	    int sec = 0;
	    sscanf(line, "level %d %d:%d %d", &mps, &tc, &sec, &inc) == 4 ||
	    sscanf(line, "level %d %d %d", &mps, &tc, &inc);
	    tc = (60*tc + sec)*1000; inc *= 1000; stime = 0;
	}
	else if(!strcmp(command, "option")) {
	    char name[80], *p;
	    if(p = strchr(line, '=')) {
		*p++ = 0;
		if(strstr(checkOptions, line+7)) sprintf(p, "%s\n", atoi(p) ? "true" : "false");
		fprintf(toE, "setoption name %s value %s", line+7, p), printf("# setoption name %s value %s", line+7, p);
	    } else fprintf(toE, "setoption name %s\n", line+7), printf("# setoption name %s\n", line+7);
	}
	else if(!strcmp(command, "protover")) {
	    printf("feature variants=\"%s\" setboard=1 usermove=1 debug=1 reuse=0 done=0\n", sc=='s' ? "shogi" : VARIANTS);
	    fprintf(toE, "u%ci\n", sc); // this prompts UCI engine for options
	}
	else if(!strcmp(command, "setboard")) {
		if(strstr(line+9, " b ")) stm = BLACK;
		sprintf(iniPos, "%s%sfen %s", iniPos[0]=='p' ? "position " : "", sc=='s' ? "s" : "", line+9);
		iniPos[strlen(iniPos)-1] = 0;
	}
	else if(!strcmp(command, "variant")) {
		if(!strcmp(line+8, "shogi\n")) strcpy(iniPos, "position startpos");
		if(!strcmp(line+8, "xiangqi\n")) strcpy(iniPos, "fen rnbakabnr/9/1c5c1/p1p1p1p1p/9/9/P1P1P1P1P/1C5C1/9/RNBAKABNR r");
	}
	else if(!strcmp(command, "undo") && (i=1) || !strcmp(command, "remove") && (i=2)) {
	    if(pondering || computer == ANALYZE) StopPonder(1);
	    moveNr = moveNr > i ? moveNr - i : 0;
	}
	else if(!strcmp(command, ".")) {
	    printf("stat01: %d %d %d %d 100 %s\n", statTime, statNodes, statDepth, 100-currNr, currMove);
	    goto nomove;
	}
	else if(!strcmp(command, "xboard")) ;
	else if(!strcmp(command, "analyze"))computer = ANALYZE;
	else if(!strcmp(command, "exit"))   computer = NONE, StopPonder(1);
	else if(!strcmp(command, "force"))  computer = NONE, StopPonder(pondering);
	else if(!strcmp(command, "go"))     computer = stm;
	else if(!strcmp(command, "time"))   sscanf(line+4, "%d", &myTime),  myTime  *= 10;
	else if(!strcmp(command, "otim"))   sscanf(line+4, "%d", &hisTime), hisTime *= 10;
	else if(!strcmp(command, "post"))   post = 1;
	else if(!strcmp(command, "nopost")) post = 0;
	else if(!strcmp(command, "easy"))   ponder = 0;
	else if(!strcmp(command, "hard"))   ponder = 1;
	else if(!strcmp(command, "memory")) sscanf(line, "memory %d", &memory);
	else if(!strcmp(command, "cores"))  sscanf(line, "cores %d", &cores);
	else if(!strcmp(command, "sd"))     sscanf(line, "sd %d", &depth);
	else if(!strcmp(command, "st"))     sscanf(line, "st %d", &stime), stime *= 1000, inc = 0;
	else if(!strcmp(command, "quit"))   fprintf(toE, "quit\n"), fflush(toE), exit(0);
    }
}

int
StartEngine(char *cmdLine, char *dir)
{
  HANDLE hChildStdinRd, hChildStdinWr,
    hChildStdoutRd, hChildStdoutWr;
  SECURITY_ATTRIBUTES saAttr;
  BOOL fSuccess;
  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  DWORD err;

  /* Set the bInheritHandle flag so pipe handles are inherited. */
  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  /* Create a pipe for the child's STDOUT. */
  if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) return GetLastError();

  /* Create a pipe for the child's STDIN. */
  if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) return GetLastError();

  SetCurrentDirectory(dir); // go to engine directory

  /* Now create the child process. */
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

  fSuccess = CreateProcess(NULL,
			   cmdLine,	   /* command line */
			   NULL,	   /* process security attributes */
			   NULL,	   /* primary thread security attrs */
			   TRUE,	   /* handles are inherited */
			   DETACHED_PROCESS|CREATE_NEW_PROCESS_GROUP,
			   NULL,	   /* use parent's environment */
			   NULL,
			   &siStartInfo, /* STARTUPINFO pointer */
			   &piProcInfo); /* receives PROCESS_INFORMATION */

  if (! fSuccess) return GetLastError();

//  if (0) { // in the future we could trigger this by an argument
//    SetPriorityClass(piProcInfo.hProcess, GetWin32Priority(appData.niceEngines));
//  }

  /* Close the handles we don't need in the parent */
  CloseHandle(piProcInfo.hThread);
  CloseHandle(hChildStdinRd);
  CloseHandle(hChildStdoutWr);

  process = piProcInfo.hProcess;
  pid = piProcInfo.dwProcessId;
  fromE = (FILE*) _fdopen( _open_osfhandle((long)hChildStdoutRd, _O_TEXT|_O_RDONLY), "r");
  toE   = (FILE*) _fdopen( _open_osfhandle((long)hChildStdinWr, _O_WRONLY), "w");

  return NO_ERROR;
}

main(int argc, char **argv)
{
	char *dir = NULL, *p, *q; int e;

	if(argc == 2 && !strcmp(argv[1], "-v")) { printf("UCI2WB " VERSION " by H.G.Muller\n"); exit(0); }
	if(argc > 1 && argv[1][0] == '-') { sc = argv[1][1]; argc--; argv++; }
	if(argc < 2) { printf("usage is: U%cI2WB [-s] <engine.exe> [<engine directory>]", sc-32); exit(-1); }
	if(argc > 2) dir = argv[2];

	// spawn engine proc
	if((errno = StartEngine(argv[1], dir)) != NO_ERROR) { perror(argv[1]), exit(-1); }

	// create separate thread to handle engine->GUI traffic
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) Engine2GUI, (LPVOID) NULL, 0, &thread_id);

	// handle GUI->engine traffic in original thread
	GUI2Engine();
}
