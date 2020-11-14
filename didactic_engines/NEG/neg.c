#include <stdio.h>

#ifdef WIN32 
#    include <windows.h>
#else
#    include <sys/time.h>
#    include <sys/times.h>
#    include <unistd.h>
     int GetTickCount()
     {	struct timeval t;
	gettimeofday(&t, NULL);
	return t.tv_sec*1000 + t.tv_usec/1000;
     }
#endif

#define WHITE 8
#define BLACK 16
#define COLOR (WHITE|BLACK)


typedef void Func(int stm, int from, int to, void *closure);
typedef int Board[128];

int value[8] = { 0, 100, 100, 10000, 325, 350, 500, 950 };
int firstDir[] = { 0, 0, 27, 4, 18, 8, 13, 4 };
int steps[] = { -16, -15, -17, 0, 1, -1, 16, -16, 15, -15, 17, -17, 0, 1, -1, 16, -16, 0, 18, 31, 33, 14, -18, -31, -33, -14, 0, 16, 15, 17, 0 };
Board PST = {
 0, 2, 4, 6, 6, 4, 2, 0,   0,0,0,0,0,0,0,0,
 2, 8,10,12,12,10, 8, 2,   0,0,0,0,0,0,0,0,
 6,12,16,18,18,16,12, 6,   0,0,0,0,0,0,0,0,
 8,14,18,20,20,18,14, 8,   0,0,0,0,0,0,0,0,
 8,14,18,20,20,18,14, 8,   0,0,0,0,0,0,0,0,
 6,12,16,18,18,16,12, 6,   0,0,0,0,0,0,0,0,
 2, 8,10,12,12,10, 8, 2,   0,0,0,0,0,0,0,0,
 0, 2, 4, 6, 6, 4, 2, 0,   0,0,0,0,0,0,0,0
};


//              "abcdefghijklmnopqrstuvwxyz"
char pieces[] = ".5........3..4.176........";
Board board, attacks[2], lva[2];
int bestScore, bestFrom, bestTo, lastMover, lastChecker, randomize, post;

void
MoveGen (Board board, int stm, Func proc, void *cl)
{
  int from, to, piece, victim, type, dir, step;
  for(from=0; from<128; from = from + 9 & ~8) {
    piece = board[from];
    if(piece & stm) {
      type = piece & 7;
      dir = firstDir[type];
      while((step = steps[dir++])) {
        to = from;
        do {
          to += step;
          if(to & 0x88) break;
          victim = board[to];
          (*proc)(piece & COLOR, from, to, cl);
          victim += type < 5;
          if(!(to - from & 7) && type < 3 && (type == 1 ? to > 79 : to < 48)) victim--;
        } while(!victim);
      }
    }
  }
}

int InCheck (int stm)
{
  int k, xstm = COLOR - stm;
  for(k=0; k<128; k++) if( board[k] == stm + 3 ) {
    int dir=4, step;
    int f = (stm == WHITE ? -16 : 16); // forward
    int p = (stm == WHITE ? 2 : 1);
    if(!(k + f + 1 & 0x88) && board[k + f + 1] == xstm + p) return k + f + 2;
    if(!(k + f - 1 & 0x88) && board[k + f - 1] == xstm + p) return k + f;
    while((step = steps[dir])) {
	int from = k + steps[dir + 14];
	if(!(from & 0x88) && board[from] == xstm + 4) return from + 1;
	from = k + step;
	if(!(from & 0x88) && board[from] == xstm + 3) return from + 1;
	from = k;
	while(!((from += step) & 0x88)) if(board[from]) { // occupied
	    if(dir < 8 && (board[from] & COLOR + 6) == xstm + 6) return from + 1; // R or Q and orthogonal
	    if(dir > 7 && (board[from] & COLOR + 5) == xstm + 5) return from + 1; // B or Q and diagonal
	    break;
	}
	dir++;
    }
    break;
  }
  return 0;
}

void
Count (int stm, int from, int to, void *cl)
{
  int s = (stm == BLACK);
  if(!(to - from & 7) && (board[from] & 7) < 3) return; // ignore Pawn non-captures
  attacks[s][to]++;
  if(lva[s][to] > value[board[from] & 7]) lva[s][to] = value[board[from] & 7];
}

void
Score (int stm, int from, int to, void *cl)
{
  int score = PST[to] - PST[from];
  int piece = board[from];
  int victim = board[to];
  int myVal = value[piece & 7];
  int hisVal = value[victim & 7];
  int push = (piece & 7) < 3 && to - from & 7;// Pawn non-capture
  int s = (stm == BLACK);
  int check;
  if((piece & 7) < 3 && !(to - from & 7) != !victim) return; // weed out illegal pawn modes
  if((piece & 7) == 3) score -= score;        // keep King out of center
  else if(myVal > 400) score = 0;             // no centralization for R, Q
  if((piece & ~7) == (victim & ~7)) return;   // self capture
  board[from] = 0; board[to] = piece;
  if(from != lastChecker && InCheck(COLOR - stm)) score += 50; // bonus for checking with new piece
  check = InCheck(stm);                       // in check after move?
  board[to] = victim; board[from] = piece;
  if(check) return;                           // illegal
  score += ((rand()>>8 & 31) - 16)*randomize; // randomize
  if(from == lastMover) score -= 10;          // discourage moving same piece twice
  if(hisVal && hisVal < 400) score += PST[to];// centralization bonus of victim
  score += hisVal;                            // captured piece
  if(attacks[!s][to]) {                       // to-square was attacked
    if(attacks[s][to] - 1 + push < attacks[!s][to] ) score -= myVal; else // not sufficiently protected
    if(myVal > lva[!s][to]) score += lva[!s][to] - myVal; // or protected, but more valuable
  }
  if((piece & 7) != 3 && attacks[!s][from]) { // from-square was attacked (and not King)
    if(attacks[s][from] < attacks[!s][from] ) score += myVal; else // not sufficiently protected
    if(myVal > lva[!s][from]) score -= lva[!s][from] - myVal; // or protected, but more valuable
  }
  if((piece & 7) == 1 && to < 48) score += 50;
  if((piece & 7) == 2 && to > 79) score += 50;

  if(score > bestScore) bestScore = score, bestFrom = from, bestTo = to; // remember best move
  if(post) printf("2 %d 0 1 %c%d%c%d\n", score, (from&7) + 'a', 8 - (from >> 4), (to&7) + 'a', 8 - (to >> 4));
}

int
Setup (char *fen)
{
  char c;
  int i;
  for(i=0; i<128; i++) board[i] = 0;
  i = 0;
  while((c = *fen++)) {
    if(c == 'p') board[i++] = BLACK + 2; else
    if(c >= '0' && c <= '9') i += c - '0'; else
    if(c >= 'a' && c <= 'z') board[i++] = BLACK + pieces[c - 'a'] - '0'; else
    if(c >= 'A' && c <= 'Z') board[i++] = WHITE + pieces[c - 'A'] - '0'; else
    if(c == '/') i = (i | 15) + 1; else break;
  }
  for(i=0; i<128; i = i + 9 & ~8) printf(i&7 ? " %2d" : "\n# %2d", board[i]); printf("\n");
  return (*fen == 'w' ? WHITE : BLACK);
}

int
main ()
{
  int stm = WHITE, engineSide = 0;
  char line[256], command[20];
  srand(GetTickCount());
  while(1) {
    int i, c;
    if(stm == engineSide) {
	char *promo = "";
	for(i=0; i<128; i++) lva[0][i] = lva[1][i] = 30000, attacks[0][i] = attacks[1][i] = 0;
	MoveGen(board, COLOR, &Count, NULL);
	bestScore = -30000;
	MoveGen(board, stm, &Score, NULL);
	board[bestTo] = board[bestFrom]; board[bestFrom] = 0; stm ^= COLOR;
	if((board[bestTo] & 7) < 3 && (stm == BLACK ? bestTo < 16 : bestTo > 111)) board[bestTo] |= 7, promo = "q"; // always promote to Q
	lastMover = bestTo;
	lastChecker = InCheck(stm) ? bestTo : -1 ;
	printf("move %c%d%c%d%s\n", (bestFrom&7) + 'a', 8 - (bestFrom >> 4), (bestTo&7) + 'a', 8 - (bestTo >> 4), promo);
    }
    fflush(stdout); i = 0;
    while((line[i++] = c = getchar()) != '\n') if(c == EOF) printf("# EOF\n"), exit(1); line[i] = '\0';
    if(*line == '\n') continue;
    sscanf(line, "%s", command);
    printf("# command: %s\n", command);
    if(!strcmp(command, "usermove")) {
	int from, to; char c, d, promo, ep;
	sscanf(line, "usermove %c%d%c%d%c", &c, &from, &d, &to, &promo);
	from = (8 - from)*16 + c - 'a'; to = (8 - to)*16 + d - 'a';
	if((board[from] & 7) == 3 && to - from == 2) board[from + 1] = board[to + 1], board[to + 1] = 0; // K-side castling
	if((board[from] & 7) == 3 && from - to == 2) board[from - 1] = board[to - 2], board[to - 2] = 0; // Q-side castling
	ep = ((board[from] & 7) < 3 && !board[to]); // recognize e.p. capture
	board[to] = board[from]; if(ep) board[from & 0x70 | to & 7] = 0; board[from] = 0;
	if(promo == 'q') board[to] = board[to] | 7; // promote
	stm ^= COLOR;	
    }
    else if(!strcmp(command, "protover")) printf("feature myname=\"N.E.G. 1.1\" setboard=1 usermove=1 analyze=0 colors=0 sigint=0 sigterm=0 done=1\n");
    else if(!strcmp(command, "new"))      stm = Setup("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"), randomize = 0, engineSide = BLACK;
    else if(!strcmp(command, "go"))       engineSide = stm;
    else if(!strcmp(command, "result"))   engineSide = 0;
    else if(!strcmp(command, "force"))    engineSide = 0;
    else if(!strcmp(command, "setboard")) stm = Setup(line+9);
    else if(!strcmp(command, "random"))   randomize = !randomize;
    else if(!strcmp(command, "post"))     post = 1;
    else if(!strcmp(command, "nopost"))   post = 0;
    else if(!strcmp(command, "quit"))     break;
  }
  return 0;
}
