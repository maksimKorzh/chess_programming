/* OliThink5 (c) Oliver Brausch 17.Sep.2020, ob112@web.de, http://brausch.org */
#define VER "5.7.9"
#include <stdio.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <conio.h>
#include <sys/timeb.h>
#include <intrin.h>
struct _timeb tv;
#define _bitcnt(x) __popcnt64(x)
#define getLsb(x) _tzcnt_u64(x)
#else
#include <sys/time.h>
struct timeval tv;
struct timezone tz;
#define _bitcnt(x) __builtin_popcountll(x)
#define getLsb(x) __builtin_ctzll(x)
#endif
typedef unsigned long long u64;
typedef unsigned long u32;
typedef int Move;

#define PAWN 1
#define KNIGHT 2
#define KING 3
#define ENP 4
#define BISHOP 5
#define ROOK 6
#define QUEEN 7

#define CNODES 0x3FFF
const int pval[] = {0, 100, 290, 0, 100, 310, 500, 980};
const int fval[] = {0, 0, 2, 0, 0, 3, 5, 9};
const int pawnrun[] = {0, 0, 1, 8, 16, 32, 64, 128};

#define FROM(x) ((x) & 63)
#define TO(x) (((x) >> 6) & 63)
#define ONMV(x) (((x) >> 12) & 1)
#define PROM(x) (((x) >> 13) & 7)
#define PIECE(x) (((x) >> 16) & 7)
#define CAP(x) (((x) >> 19) & 7)
#define MAXSCORE 16384

#define _TO(x) ((x) << 6)
#define _ONMV(x) ((x) << 12)
#define _PROM(x) ((x) << 13)
#define _PIECE(x) ((x) << 16)
#define _CAP(x) ((x) << 19)
#define PREMOVE(f, p) ((f) | _ONMV(c) | _PIECE(p))

#define RATT1(f) rays[((f) << 7) | key000(BOARD, f)]
#define RATT2(f) rays[((f) << 7) | key090(BOARD, f) | 0x2000]
#define BATT3(f) rays[((f) << 7) | key045(BOARD, f) | 0x4000]
#define BATT4(f) rays[((f) << 7) | key135(BOARD, f) | 0x6000]
#define RXRAY1(f) rays[((f) << 7) | key000(BOARD, f) | 0x8000]
#define RXRAY2(f) rays[((f) << 7) | key090(BOARD, f) | 0xA000]
#define BXRAY3(f) rays[((f) << 7) | key045(BOARD, f) | 0xC000]
#define BXRAY4(f) rays[((f) << 7) | key135(BOARD, f) | 0xE000]

#define ROCC1(f) (RATT1(f) & BOARD)
#define ROCC2(f) (RATT2(f) & BOARD)
#define BOCC3(f) (BATT3(f) & BOARD)
#define BOCC4(f) (BATT4(f) & BOARD)
#define RMOVE1(f) (RATT1(f) & (~BOARD))
#define RMOVE2(f) (RATT2(f) & (~BOARD))
#define BMOVE3(f) (BATT3(f) & (~BOARD))
#define BMOVE4(f) (BATT4(f) & (~BOARD))
#define RCAP1(f, c) (RATT1(f) & colorb[(c)^1])
#define RCAP2(f, c) (RATT2(f) & colorb[(c)^1])
#define BCAP3(f, c) (BATT3(f) & colorb[(c)^1])
#define BCAP4(f, c) (BATT4(f) & colorb[(c)^1])
#define ROCC(f) (ROCC1(f) | ROCC2(f))
#define BOCC(f) (BOCC3(f) | BOCC4(f))
#define RMOVE(f) (RMOVE1(f) | RMOVE2(f))
#define BMOVE(f) (BMOVE3(f) | BMOVE4(f))
#define RCAP(f, c) (ROCC(f) & colorb[(c)^1])
#define BCAP(f, c) (BOCC(f) & colorb[(c)^1])

#define SHORTMOVE(x) ((x) & ((x)^BOARD))
#define SHORTCAP(x, c) ((x) & colorb[(c)^1])

#define NMOVE(x) (SHORTMOVE(nmoves[x]))
#define KMOVE(x) (SHORTMOVE(kmoves[x]))
#define PMOVE(x, c) (pmoves[(x) | ((c)<<6)] & (~BOARD))
#define POCC(x, c) (pcaps[(x) | ((c)<<6)] & BOARD)
#define NCAP(x, c) (SHORTCAP(nmoves[x], (c)))
#define KCAP(x, c) (SHORTCAP(kmoves[x], (c)))
#define PCAP(x, c) (pcaps[(x) | ((c)<<6)] & colorb[(c)^1])
#define PCA3(x, c) (pcaps[(x) | ((c)<<6) | 128] & (colorb[(c)^1] | (BIT[ENPASS] & (c ? 0xFF0000LL : 0xFF0000000000LL))))
#define PCA4(x, c) (pcaps[(x) | ((c)<<6) | 256] & (colorb[(c)^1] | (BIT[ENPASS] & (c ? 0xFF0000LL : 0xFF0000000000LL))))

#define RANK(x, y) (((x) & 0x38) == (y))
#define TEST(f, b) (BIT[f] & (b))
#define ENPASS (flags & 63)
#define CASTLE (flags & 960)
#define COUNT (count & 0x3FF)
#define MEVAL(w) (w > MAXSCORE-500 ? (200000+MAXSCORE+1-w)/2 : (w < 500-MAXSCORE ? (-200000-MAXSCORE-w)/2 : w))
#define NOMATEMAT(c) ((sf[c] <= 4 || (sf[c] <= 8 && sf[c] <= sf[c^1] + 3)) && (pieceb[PAWN] & colorb[c]) == 0)

#define HSIZE 0x800000LL
#define HMASK (HSIZE-1)

struct entry {
	u64 key;
	Move move;
	short value;
	char depth;
	char type;
};

struct entry hashDB[HSIZE];
u64 hashb;
u64 hstack[0x800];
u64 mstack[0x800];

static u64 hashxor[4096];
static u64 rays[0x10000];
static u64 pmoves[128];
static u64 pcaps[384];
static u64 nmoves[64];
static u64 kmoves[64];
static int _knight[8] = {-17,-10,6,15,17,10,-6,-15};
static int _king[8] = {-9,-1,7,8,9,1,-7,-8};
static u64 BIT[64];
static int crevoke[64];
static int nmobil[64];
static int kmobil[64];
static int pawnprg[128];
static u64 pawnfree[128];
static u64 pawnfile[128];
static u64 pawnhelp[128];
static int cornbase[] = {4, 4, 2, 1, 0, 0 ,0};
static int bishcorn[64];
u64 whitesq;

struct Movep {
	int n;
	Move list[128];
};
typedef struct Movep Movep;
Move pv[128][128];
int value[128];
int iter;
const char pieceChar[] = "*PNK.BRQ";
u64 maxtime, starttime;
int sabort, ics = 0, ponder = 0, pondering = 0, analyze = 0;
Move pon = 0;
int count, flags, mat, onmove, engine =-1;
int sd = 64;
int kingpos[2];
u64 pieceb[8];
u64 colorb[2];
char irbuf[256];
#define BOARD (colorb[0] | colorb[1])
#define RQU (pieceb[QUEEN] | pieceb[ROOK])
#define BQU (pieceb[QUEEN] | pieceb[BISHOP])

void setBit(int f, u64 *b) {
	*b |= BIT[f];
}

void xorBit(int f, u64 *b) {
	*b ^= BIT[f];
}

int _getpiece(char s, int *c) {
	int i;
	for (i = 1; i < 8; i++) 
		if (pieceChar[i] == s) { *c = 0; return i; } 
		else if (pieceChar[i] == s-32) { *c = 1; return i; }
	return 0;
}

int sf[2];
int changeMat(int m, int c, int d) {
	int dm = pval[CAP(m)];
	if (PROM(m)) dm += -pval[PAWN] + pval[PROM(m)];
	sf[c] += d*fval[PROM(m)];
	sf[c^1] -= d*fval[CAP(m)];
	return c ? -d*dm : d*dm;
}

char *sfen = "rnbqkbnr/pppppppp/////PPPPPPPP/RNBQKBNR w KQkq - 0 1";
void reseth(int level);
int book;
void _parse_fen(char *fen) {
	char s, mv, pos[128], cas[5], enps[3];
	int c, i, halfm = 0, fullm = 1, col = 0, row = 7;
	for (i = 0; i < 8; i++) pieceb[i] = 0LL;
	colorb[0] = colorb[1] = hashb = 0LL;
	mat = book = i = c = cas[0] = enps[0] = sf[0] = sf[1] = 0;
	sscanf(fen, "%s %c %s %s %d %d", pos, &mv, cas, enps, &halfm, &fullm); if (fullm < 1) fullm = 1;
	while ((s = pos[i++])) {
		if (s == '/') {
			row--;
			col = 0;
		} else if (s >= '1' && s <= '8') {
			col += s - '0';
		} else {
			int p = _getpiece(s, &c);
			if (p == KING) kingpos[c] = row*8 + col;
			else mat += changeMat(_CAP(p) | _TO(row*8 + col), c^1, -1);
			hashb ^= hashxor[col | row << 3 | p << 6 | c << 9];
			setBit(row*8 + col, pieceb + p);
			setBit(row*8 + (col++), colorb + c);
		}
	}
	onmove = mv == 'b' ? 1 : 0;
	flags = i = 0;
	while ((s = cas[i++])) {
		if (s == 'K') flags |= BIT[6];
		if (s == 'k') flags |= BIT[7];
		if (s == 'Q') flags |= BIT[8];
		if (s == 'q') flags |= BIT[9];
	}
	if (enps[0] >= 'a' && enps[0] <= 'h' && enps[1] >= '1' && enps[1] <= '8') flags |= 8*(enps[1] -'1') + enps[0] -'a';
	count = (fullm - 1)*2 + onmove + (halfm << 10);
	for (i = 0; i < COUNT; i++) hstack[i] = 0LL;
	reseth(fen == sfen ? 2 : 3);
}

void doMove(Move, int c);
int parseMove(char*, int, Move);
int protV2(char*,int);
#define BKSIZE 8192
Move bkmove[BKSIZE*32];
int bkflag[BKSIZE];
int bkcount[3];

void _readbook(char *bk) {
	char buf[256], s0[64], s1[64], s2[64];
	int k, n = 0;
	FILE *in = fopen(bk, "r");
	bkcount[0] = bkcount[1] = 0;
	for (k = 0; k < BKSIZE; k++) bkflag[k] = 2;
	if (in != NULL) {
		while (!feof(in)) {
			fgets(buf, 255, in);
			if (buf[0] == '[') {
				sscanf(buf,"%s %s", s1, s2);
				if (!strncmp(s2, "\"OliThink", 8)) bkflag[n] = strncmp(s1, "[Black", 6) ? 0 : 1;
				else if (!strncmp(s1, "[Result", 7)) {
					if (bkflag[n]) {
						if (!strncmp(s2, "\"0-1", 4)) bkflag[n] = 2;
					} else if (strncmp(s2, "\"1-0", 4)) bkflag[n] = 2;
				}
			} else if (buf[0] == '1' && buf[1] == '.' && bkflag[n] < 2) {
				u32 i = 0, j = 0;
				_parse_fen(sfen);
				for (;;) {
					if (strchr(buf+i, ' ') == NULL) break;
					sscanf(buf+i,"%s %s", s0, s2);
					if (s0[0] < '1' || s0[0] > '9') break;
					i += (int)(strlen(s0) + strlen(s2) + 2);
					if (s0[strlen(s0)-1] == '.') {
						strcpy(s1, s2); s2[0] = 0;
						sscanf(buf+i,"%s", s2);
						i += (int)(strlen(s2) + 1);
					} else sscanf(s0,"%*[^.].%[^.]", s1);
					doMove(bkmove[n*32+ (j++)] = parseMove(s1, 0, 0), 0);
					if (s2[0] == 0 || s2[0] == '*') break;
					doMove(bkmove[n*32+ (j++)] = parseMove(s2, 1, 0), 1);
					if (j > 30 || i >= strlen(buf)) break;
				}
				bkmove[n*32 + j] = 0;
				if (j) bkcount[bkflag[n]]++;
				if (++n == BKSIZE) break;
			} 
		}
		fclose(in);
	}
	_parse_fen(sfen);
	if (bkcount[0] > 0 || bkcount[1] > 0) book = 1;
}

u64 getTime() {
#ifdef _WIN32
	_ftime(&tv);
	return(tv.time * 1000LL + tv.millitm);
#else
	gettimeofday (&tv, &tz);
	return(tv.tv_sec * 1000LL + (tv.tv_usec / 1000));
#endif
}

int pullLsb(u64* bit) {
	int f = getLsb(*bit);
	*bit &= *bit - 1;
	return f;
}

int identPiece(int f) {
	int i;
	for (i = PAWN; i <= QUEEN; i++) if (i != ENP && TEST(f, pieceb[i])) return i;
	return ENP;
}

u64 bmask45[64];
u64 bmask135[64];
int key000(u64 b, int f) {
	return (int) ((b >> (f & 56)) & 0x7E);
}

int key090(u64 b, int f) {
	u64 _b = (b >> (f&7)) & 0x0101010101010101LL;
	_b = _b * 0x0080402010080400LL;
	return (int)(_b >> 57);
}

int keyDiag(u64 _b) {
	_b = _b * 0x0202020202020202LL;
	return (int)(_b >> 57);
}

int key045(u64 b, int f) {
	return keyDiag(b & bmask45[f]);
}

int key135(u64 b, int f) {
	return keyDiag(b & bmask135[f]);
}

#define DUALATT(x, y, c) (battacked(x, c, pieceb[QUEEN]) || battacked(y, c, pieceb[QUEEN]))
int battacked(int f, int c, u64 q) {
	if (PCAP(f, c) & pieceb[PAWN]) return 1;
	if (NCAP(f, c) & pieceb[KNIGHT]) return 1;
	if (KCAP(f, c) & pieceb[KING]) return 1;
	if (RCAP1(f, c) & (pieceb[ROOK] | q)) return 1;
	if (RCAP2(f, c) & (pieceb[ROOK] | q)) return 1;
	if (BCAP3(f, c) & (pieceb[BISHOP] | q)) return 1;
	if (BCAP4(f, c) & (pieceb[BISHOP] | q)) return 1;
	return 0;
}

u64 reach(int f, int c) {
	return (NCAP(f, c) & pieceb[KNIGHT])
		| (RCAP1(f, c) & RQU)
		| (RCAP2(f, c) & RQU)
		| (BCAP3(f, c) & BQU)
		| (BCAP4(f, c) & BQU);
}

u64 attacked(int f, int c) {
	return (PCAP(f, c) & pieceb[PAWN]) | reach(f, c);
}

void _init_pawns(u64* moves, u64* caps, u64* freep, u64* filep, u64* helpp, int c) {
	int i, j;
	for (i = 0; i < 64; i++) {
		int rank = i/8;
		int file = i&7;
		int m = i + (c ? -8 : 8);
		pawnprg[i + (c << 6)] = pawnrun[c ? 7-rank : rank];
		for (j = 0; j < 64; j++) {
			int jrank = j/8;
			int jfile = j&7;
			int dfile = (jfile - file)*(jfile - file);
			if (dfile > 1) continue;
			if ((c && jrank < rank) || (!c && jrank > rank)) {//The not touched part of the board
				if (dfile == 0) setBit(j, filep + i);
				setBit(j, freep + i);
			} else if (dfile != 0 && (jrank - rank)*(jrank - rank) <= 1) {
				setBit(j, helpp + i);
			}
		}
		if (m < 0 || m > 63) continue;
		setBit(m, moves + i);
		if (file > 0) {
			m = i + (c ? -9 : 7);
			if (m < 0 || m > 63) continue;
			setBit(m, caps + i);
			setBit(m, caps + i + 128*(2 - c));
		}
		if (file < 7) {
			m = i + (c ? -7 : 9);
			if (m < 0 || m > 63) continue;
			setBit(m, caps + i);
			setBit(m, caps + i + 128*(c + 1));
		}
	}
}

void _init_shorts(u64* moves, int* m) {
	int i, j, n;
	for (i = 0; i < 64; i++) {
		for (j = 0; j < 8; j++) {
			n = i + m[j];
			if (n < 64 && n >= 0 && ((n & 7)-(i & 7))*((n & 7)-(i & 7)) <= 4) {
				setBit(n, moves+i);
			}
		}
	}
}

u64 _occ_free_board(int bc, int del, u64 free) {
	int i;
	u64 low, perm = free;
	for (i = 0; i < bc; i++) {
		low = free & (-free); // Lowest bit 
		free &= (~low);
		if (!TEST(i, del)) perm &= (~low);
	}
	return perm;
}

void _init_rays(u64* rays, u64 (*rayFunc)(int, u64, int), int (*key)(u64, int)) {
	int i, f, iperm, bc, index; 
	u64 board, mmask, occ, move, xray;
	for (f = 0; f < 64; f++) {
		mmask = (*rayFunc)(f, 0LL, 0) | BIT[f];
		iperm = 1 << (bc = _bitcnt(mmask));
		for (i = 0; i < iperm; i++) {
			board = _occ_free_board(bc, i, mmask);
			move = (*rayFunc)(f, board, 1);
			occ = (*rayFunc)(f, board, 2);
			xray = (*rayFunc)(f, board, 3);
			index = (*key)(board, f);
			rays[(f << 7) + index] = occ | move;
			rays[(f << 7) + index + 0x8000] = xray;
		}
	}
}

#define RAYMACRO { if (TEST(i, board)) \
	{ if (b) {setBit(i, &xray); break; } else { setBit(i, &occ); b = 1; }} if (!b) setBit(i, &free); }
u64 _rook0(int f, u64 board, int t) {
	u64 free = 0LL, occ = 0LL, xray = 0LL;
	int i, b;
	for (b = 0, i = f+1; i < 64 && i%8 != 0; i++) RAYMACRO
	for (b = 0, i = f-1; i >= 0 && i%8 != 7; i--) RAYMACRO
	return (t < 2) ? free : (t == 2 ? occ : xray);
}

u64 _rook90(int f, u64 board, int t) {
	u64 free = 0LL, occ = 0LL, xray = 0LL;
	int i, b;
	for (b = 0, i = f-8; i >= 0; i-=8) RAYMACRO
	for (b = 0, i = f+8; i < 64; i+=8) RAYMACRO
	return (t < 2) ? free : (t == 2 ? occ : xray);
}

u64 _bishop45(int f, u64 board, int t) {
	u64 free = 0LL, occ = 0LL, xray = 0LL;
	int i, b;
	for (b = 0, i = f+9; i < 64 && (i%8 != 0); i+=9) RAYMACRO
	for (b = 0, i = f-9; i >= 0 && (i%8 != 7); i-=9) RAYMACRO
	return (t < 2) ? free : (t == 2 ? occ : xray);
}

u64 _bishop135(int f, u64 board, int t) {
	u64 free = 0LL, occ = 0LL, xray = 0LL;
	int i, b;
	for (b = 0, i = f-7; i >= 0 && (i%8 != 0); i-=7) RAYMACRO
	for (b = 0, i = f+7; i < 64 && (i%8 != 7); i+=7) RAYMACRO
	return (t < 2) ? free : (t == 2 ? occ : xray);
}

void displaym(Move m) {
	printf("%c%c%c%c", 'a' + FROM(m) % 8, '1' + FROM(m) / 8, 'a' + TO(m) % 8, '1' + TO(m) / 8);
	if (PROM(m)) printf("%c", pieceChar[PROM(m)]+32);
}

/* This one is the same as in OliThink 4 */
int bioskey() {
#ifndef _WIN32
	fd_set readfds;

	FD_ZERO (&readfds);
	FD_SET (fileno(stdin), &readfds);
	tv.tv_sec=0; tv.tv_usec=0;
	select(16, &readfds, 0, 0, &tv);

	return (FD_ISSET(fileno(stdin), &readfds));
#else
	static int init = 0, pipe;
	static HANDLE inh;
	DWORD dw;

	if (!init) {
		init = 1;
		inh = GetStdHandle(STD_INPUT_HANDLE);
		pipe = !GetConsoleMode(inh, &dw);
		if (!pipe) {
			SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
			FlushConsoleInputBuffer(inh);
		}
	}
	if (pipe) {
		if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
		return dw;
	} else {
		GetNumberOfConsoleInputEvents(inh, &dw);
		return dw <= 1 ? 0 : dw;
	}
#endif
}

void displaypv() {
	int i;
	if (pon && pondering) { printf("("); displaym(pon); printf(") "); }
	for (i = 0; pv[0][i]; i++) {
		displaym(pv[0][i]); printf(" ");
	}
}

int isDraw(u64 hp, int nrep) {
	if (count > 0xFFF) { //fifty > 3
		int i, c = 0, n = COUNT - (count >> 10);
		if (count >= 0x400*100) return 2; //100 plies
		for (i = COUNT - 2; i >= n; i--) 
			if (hstack[i] == hp && ++c == nrep) return 1; 
	} else if (!(pieceb[PAWN] | RQU)) { //Check for mating material
		if (_bitcnt(BOARD) <= 3) return 3;
	}
	return 0;
}

u64 pinnedPieces(int f, int oc) {
	u64 pin = 0LL;
	u64 b = ((RXRAY1(f) | RXRAY2(f)) & colorb[oc]) & RQU;
	while (b) {
		int t = pullLsb(&b);
		pin |= RCAP(t, oc) & ROCC(f);
	}
	b = ((BXRAY3(f) | BXRAY4(f)) & colorb[oc]) & BQU;
	while (b) {
		int t = pullLsb(&b);
		pin |= BCAP(t, oc) & BOCC(f);
	}
	return pin;
}

/* precondition: f and t are on common rank (8), file (16), diagonal (32) or antidiagonal (64) */
char getDir(int f, int t) {
	if (!((f ^ t) & 56)) return 8;
	if (!((f ^ t) & 7)) return 16;
	return (!((f - t) % 9)) ? 32 : 64;
}

#define XORHASH(f, p, c) hashb ^= hashxor[(f) | (p) << 6 | (c) << 9]
/* move is for both doMove and undoMove, only for undo the globalflags have to be restored (counter, castle, enpass..)*/
void move(Move m, int c, int d) {
	int f = FROM(m);
	int t = TO(m);
	int p = PIECE(m);
	int a = CAP(m);

	xorBit(f, colorb+c);
	xorBit(f, pieceb+p);

	xorBit(t, colorb+c);
	xorBit(t, pieceb+p);
	XORHASH(f, p, c);
	XORHASH(t, p, c);

	flags &= 960;
	count += 0x401;
	if (a) {
		if (a == ENP) { // Enpassant Capture
			t = (t&7) | (f&56);
			a = PAWN;
		} else if (a == ROOK && CASTLE) { //Revoke castling rights.
			flags &= crevoke[t];
		}
		xorBit(t, pieceb+a);
		xorBit(t, colorb+(c^1));
		XORHASH(t, a, c^1);
		count &= 0x3FF; //Reset Fifty Counter
		mat += changeMat(m, c, d);
	}
	if (p == PAWN) {
		if (((f^t)&8) == 0) flags |= f^24; //Enpassant
		else if ((t&56) == 0 || (t&56) == 56) {
			xorBit(t, pieceb+PAWN);
			xorBit(t, pieceb+PROM(m));
			XORHASH(t, PAWN, c);
			XORHASH(t, PROM(m), c);
			if (!a) mat += changeMat(m, c, d);
		}
		count &= 0x3FF; //Reset Fifty Counter
	} else if (p == KING) {
		if (kingpos[c] == f) kingpos[c] = t; else kingpos[c] = f;
		flags &= ~(320 << c); // Lose castling rights
		if (((f^t)&3) == 2) { // Castle
			if (t == 6) { f = 7; t = 5; }
			else if (t == 2) { f = 0; t = 3; }
			else if (t == 62) { f = 63; t = 61; }
			else { f = 56; t = 59; }
			xorBit(f, colorb+c);
			xorBit(f, pieceb+ROOK);
			xorBit(t, colorb+c);
			xorBit(t, pieceb+ROOK);
			XORHASH(f, ROOK, c);
			XORHASH(t, ROOK, c);
		}
	} else if (p == ROOK && CASTLE) {
		flags &= crevoke[f];
	}
}

void doMove(Move m, int c) {
	mstack[COUNT] = count | (flags << 17) | (((u64)m) << 27);
	move(m, c, 1);
}

void undoMove(Move m, int c) {
	u64 u = mstack[COUNT - 1];
	move(m, c, -1);
	count = u & 0x1FFFF;
	flags = (u >> 17L) & 0x3FF;
}

void regMoves(Move m, u64 bt, Movep* mp, int cap) {
	while (bt) {
		int t = pullLsb(&bt);
		mp->list[mp->n++] = m | _TO(t) | (cap ? _CAP(identPiece(t)) : 0LL);
	}
}

void regMovesCaps(Move m, u64 bc, u64 bm, Movep* mp) {
	regMoves(m, bc, mp, 1);
	regMoves(m, bm, mp, 0);
}

void regPromotions(int f, int c, u64 bt, Movep* mp, int cap, int queen) {
	while (bt) {
		int t = pullLsb(&bt);
		Move m = f | _ONMV(c) | _PIECE(PAWN) | _TO(t) | (cap ? _CAP(identPiece(t)) : 0);
		if (queen) mp->list[mp->n++] = m | _PROM(QUEEN);
		mp->list[mp->n++] = m | _PROM(KNIGHT);
		mp->list[mp->n++] = m | _PROM(ROOK);
		mp->list[mp->n++] = m | _PROM(BISHOP);
	}
}

void regKings(Move m, u64 bt, Movep* mp, int c, int cap) {
	while (bt) {
		int t = pullLsb(&bt);
		if (battacked(t, c, pieceb[QUEEN])) continue;
		mp->list[mp->n++] = m | _TO(t) | (cap ? _CAP(identPiece(t)) : 0LL);
	}
}

int generateCheckEsc(u64 ch, u64 apin, int c, int k, Movep *mp) {
	u64 cc, fl;
	int d, bf = _bitcnt(ch);
	xorBit(k, colorb+c);
	regKings(PREMOVE(k, KING), KCAP(k, c), mp, c, 1);
	regKings(PREMOVE(k, KING), KMOVE(k), mp, c, 0);
	xorBit(k, colorb+c);
	if (bf > 1) return bf; //Doublecheck
	bf = getLsb(ch);

	cc = attacked(bf, c^1) & apin;  //Can we capture the checker?
	while (cc) {
		int cf = pullLsb(&cc);
		int p = identPiece(cf);
		if (p == PAWN && RANK(cf, c ? 0x08 : 0x30)) 
			regPromotions(cf, c, ch, mp, 1, 1);
		else 
			regMovesCaps(PREMOVE(cf, p), ch, 0LL, mp);
	}
	if (ENPASS && (ch & pieceb[PAWN])) { //Enpassant capture of attacking Pawn
		cc = PCAP(ENPASS, c^1) & pieceb[PAWN] & apin;
		while (cc) {
			int cf = pullLsb(&cc);
			regMovesCaps(PREMOVE(cf, PAWN), BIT[ENPASS], 0LL, mp);
		}
	}
	if (ch & (nmoves[k] | kmoves[k])) return 1; //We can't move anything between!

	d = getDir(bf, k);
	if (d & 8) fl = RMOVE1(bf) & RMOVE1(k);
	else if (d & 16) fl = RMOVE2(bf) & RMOVE2(k);
	else if (d & 32) fl = BMOVE3(bf) & BMOVE3(k);
	else fl = BMOVE4(bf) & BMOVE4(k);

	while (fl) {
		int f = pullLsb(&fl);
		cc = reach(f, c^1) & apin;
		while (cc) {
			int cf = pullLsb(&cc);
			int p = identPiece(cf);
			regMovesCaps(PREMOVE(cf, p), 0LL, BIT[f], mp);
		}
		bf = c ? f+8 : f-8;
		if (bf < 0 || bf > 63) continue;
		if (BIT[bf] & pieceb[PAWN] & colorb[c] & apin) {
			if (RANK(bf, c ? 0x08 : 0x30)) 
				regPromotions(bf, c, BIT[f], mp, 0, 1);
			else
				regMovesCaps(PREMOVE(bf, PAWN), 0LL, BIT[f], mp);
		}
		if (RANK(f, c ? 0x20 : 0x18) && !(BOARD & BIT[bf]) && (BIT[c ? f+16 : f-16] & pieceb[PAWN] & colorb[c] & apin))
			regMovesCaps(PREMOVE(c ? f+16 : f-16, PAWN), 0LL, BIT[f], mp);
	}
	return 1;
}

void generateNonCaps(int c, int f, u64 pin, Movep* mp) {
	u64 m, b, cb = colorb[c] & (~pin);

	regKings(PREMOVE(f, KING), KMOVE(f), mp, c, 0);

	b = pieceb[PAWN] & cb;
	while (b) {
		f = pullLsb(&b);
		m = PMOVE(f, c);
		if (m && RANK(f, c ? 0x30 : 0x08)) m |= PMOVE(c ? f-8 : f+8, c);
		if (RANK(f, c ? 0x08 : 0x30)) {
			u64 a = PCAP(f, c);
			if (a) regPromotions(f, c, a, mp, 1, 0);
			regPromotions(f, c, m, mp, 0, 0);
		} else {
			regMoves(PREMOVE(f, PAWN), m, mp, 0);
		}
	}

	b = pin & pieceb[PAWN]; 
	while (b) {
		f = pullLsb(&b);
		int t = getDir(f, kingpos[c]);
		if (t & 8) continue;
		m = 0LL;
		if (t & 16) {
			m = PMOVE(f, c);         
			if (m && RANK(f, c ? 0x30 : 0x08)) m |= PMOVE(c ? f-8 : f+8, c);
		}
		if (RANK(f, c ? 0x08 : 0x30)) {
			u64 a = (t & 32) ? PCA3(f, c) : ((t & 64) ? PCA4(f, c) : 0LL);
			if (a) regPromotions(f, c, a, mp, 1, 0);
		} else {
			regMoves(PREMOVE(f, PAWN), m, mp, 0);
		}
	}

	b = pieceb[KNIGHT] & cb;
	while (b) {
		f = pullLsb(&b);
		regMoves(PREMOVE(f, KNIGHT), NMOVE(f), mp, 0);
	}

	b = pieceb[ROOK] & cb;
	while (b) {
		f = pullLsb(&b);
		regMoves(PREMOVE(f, ROOK), RMOVE(f), mp, 0);
		if (CASTLE) {
			if (c) {
				if ((flags & 128) && (f == 63) && (RMOVE1(63) & BIT[61]))
					if (!DUALATT(61, 62, c)) regMoves(PREMOVE(60, KING), BIT[62], mp, 0);
				if ((flags & 512) && (f == 56) && (RMOVE1(56) & BIT[59]))
					if (!DUALATT(59, 58, c)) regMoves(PREMOVE(60, KING), BIT[58], mp, 0);
			} else {
				if ((flags & 64) && (f == 7) && (RMOVE1(7) & BIT[5]))
					if (!DUALATT(5, 6, c)) regMoves(PREMOVE(4, KING), BIT[6], mp, 0);
				if ((flags & 256) && (f == 0) && (RMOVE1(0) & BIT[3]))
					if (!DUALATT(3, 2, c)) regMoves(PREMOVE(4, KING), BIT[2], mp, 0);
			}
		}
	}

	b = pieceb[BISHOP] & cb;
	while (b) {
		f = pullLsb(&b);
		regMoves(PREMOVE(f, BISHOP), BMOVE(f), mp, 0);
	}

	b = pieceb[QUEEN] & cb;
	while (b) {
		f = pullLsb(&b);
		regMoves(PREMOVE(f, QUEEN), RMOVE(f) | BMOVE(f), mp, 0);
	}

	b = pin & (pieceb[ROOK] | pieceb[BISHOP] | pieceb[QUEEN]); 
	while (b) {
		f = pullLsb(&b);
		int p = identPiece(f);
		int t = p | getDir(f, kingpos[c]);
		if ((t & 10) == 10) regMoves(PREMOVE(f, p), RMOVE1(f), mp, 0);
		if ((t & 18) == 18) regMoves(PREMOVE(f, p), RMOVE2(f), mp, 0);
		if ((t & 33) == 33) regMoves(PREMOVE(f, p), BMOVE3(f), mp, 0);
		if ((t & 65) == 65) regMoves(PREMOVE(f, p), BMOVE4(f), mp, 0);
	}
}

void generateCaps(int c, int f, u64 pin, Movep *mp) {
	u64 m, b, a, cb = colorb[c] & (~pin);

	regKings(PREMOVE(f, KING), KCAP(f, c), mp, c, 1);

	b = pieceb[PAWN] & cb;
	while (b) {
		f = pullLsb(&b);
		a = PCAP(f, c);
		if (RANK(f, c ? 0x08 : 0x30)) {
			regMovesCaps(PREMOVE(f, PAWN) | _PROM(QUEEN), a, PMOVE(f, c), mp);
		} else {
			if (ENPASS && (BIT[ENPASS] & pcaps[(f) | ((c)<<6)])) {
				xorBit(ENPASS^8, colorb+(c^1));
				if (!(ROCC1(f) & BIT[kingpos[c]]) || !(ROCC1(f) & colorb[c^1] & RQU)) {
					a = a | BIT[ENPASS];
				}
				xorBit(ENPASS^8, colorb+(c^1));
			}
			regMoves(PREMOVE(f, PAWN), a, mp, 1);
		}
	}

	b = pin & pieceb[PAWN]; 
	while (b) {
		f = pullLsb(&b);
		int t = getDir(f, kingpos[c]);
		if (t & 8) continue;
		m = a = 0LL;
		if (t & 16) {
			m = PMOVE(f, c);         
		} else if (t & 32) {
			a = PCA3(f, c);
		} else {
			a = PCA4(f, c);
		}
		if (RANK(f, c ? 0x08 : 0x30)) {
			regMovesCaps(PREMOVE(f, PAWN) | _PROM(QUEEN), a, m, mp);
		} else {
			regMoves(PREMOVE(f, PAWN), a, mp, 1);
		}
	}

	b = pieceb[KNIGHT] & cb;
	while (b) {
		f = pullLsb(&b);
		regMoves(PREMOVE(f, KNIGHT), NCAP(f, c), mp, 1);
	}

	b = pieceb[BISHOP] & cb;
	while (b) {
		f = pullLsb(&b);
		regMoves(PREMOVE(f, BISHOP), BCAP(f, c), mp, 1);
	}

	b = pieceb[ROOK] & cb;
	while (b) {
		f = pullLsb(&b);
		regMoves(PREMOVE(f, ROOK), RCAP(f, c), mp, 1);
	}

	b = pieceb[QUEEN] & cb;
	while (b) {
		f = pullLsb(&b);
		regMoves(PREMOVE(f, QUEEN), RCAP(f, c) | BCAP(f,c), mp, 1);
	}

	b = pin & (pieceb[ROOK] | pieceb[BISHOP] | pieceb[QUEEN]); 
	while (b) {
		f = pullLsb(&b);
		int p = identPiece(f);
		int t = p | getDir(f, kingpos[c]);
		if ((t & 10) == 10) regMoves(PREMOVE(f, p), RCAP1(f, c), mp, 1);
		if ((t & 18) == 18) regMoves(PREMOVE(f, p), RCAP2(f, c), mp, 1);
		if ((t & 33) == 33) regMoves(PREMOVE(f, p), BCAP3(f, c), mp, 1);
		if ((t & 65) == 65) regMoves(PREMOVE(f, p), BCAP4(f, c), mp, 1);
	}
}

#define GENERATE(c, mp) generate(attacked(kingpos[c], c), c, mp, 1, 1)
int generate(u64 ch, int c, Movep *mp, int cap, int noncap) {
	int f = kingpos[c];
	u64 pin = pinnedPieces(f, c^1);
	mp->n = 0;
	if (ch) return generateCheckEsc(ch, ~pin, c, f, mp);
	if (cap) generateCaps(c, f, pin, mp);
	if (noncap) generateNonCaps(c, f, pin, mp);
	return 0;
}

int swap(Move m) { //SEE
	int s_list[32], f = FROM(m), t = TO(m), c = ONMV(m);
	int a_piece = pval[CAP(m)], piece = PIECE(m), nc = 1;
	u64 temp = 0, colstore0 = colorb[0], colstore1 = colorb[1];

	u64 attacks = ((PCAP(t, 0) | PCAP(t, 1)) & pieceb[PAWN])
		| (nmoves[t] & pieceb[KNIGHT]) | (kmoves[t] & pieceb[KING]);
	s_list[0] = a_piece;
	a_piece = pval[piece];
	colorb[c] &= ~BIT[f];

	do {
		c ^= 1;
		attacks |= (BOCC(t) & BQU) | (ROCC(t) & RQU);
		attacks &= BOARD;

		if ((temp = pieceb[PAWN] & colorb[c] & attacks)) piece = PAWN;
		else if ((temp = pieceb[KNIGHT] & colorb[c] & attacks)) piece = KNIGHT;
		else if ((temp = pieceb[BISHOP] & colorb[c] & attacks)) piece = BISHOP;
		else if ((temp = pieceb[ROOK] & colorb[c] & attacks)) piece = ROOK;
		else if ((temp = pieceb[QUEEN] & colorb[c] & attacks)) piece = QUEEN;
		else if ((temp = pieceb[KING] & colorb[c] & attacks)) { piece = KING; if (colorb[c^1] & attacks) break; }
		else break;

		temp &= -(long long)temp;
		colorb[c] ^= temp;

		s_list[nc] = -s_list[nc - 1] + a_piece;
		a_piece = pval[piece];
		if (a_piece < s_list[++nc - 1]) break;
	} while (1);

	while (--nc)
		if (s_list[nc] > -s_list[nc - 1])
			s_list[nc - 1] = -s_list[nc];

	colorb[0] = colstore0;
	colorb[1] = colstore1;
	return s_list[0];
}

/* In quiesce the list are ordered just for the value of the captured piece */
Move qpick(Movep* mp, int s) {
	Move m;
	int i, t, pi = 0, vmax = -9999;
	for (i = s; i < mp->n; i++) {
		m = mp->list[i];
		t = pval[CAP(m)];
		if (t > vmax) {
			vmax = t;
			pi = i;
		}
	}
	m = mp->list[pi];
	if (pi != s) mp->list[pi] = mp->list[s];
	return m;
}

Move killer[128];
long long history[0x2000];
/* In normal search some basic move ordering heuristics are used */
Move spick(Movep* mp, int s, int ply) {
	Move m;
	int i, pi = 0; long long vmax = -9999L;
	for (i = s; i < mp->n; i++) {
		m = mp->list[i];
		if (m == killer[ply]) {
			pi = i;
			break;
		}
		if (vmax < history[m & 0x1FFF]) {
			vmax = history[m & 0x1FFF];
			pi = i;
		}
	}
	m = mp->list[pi];
	if (pi != s) mp->list[pi] = mp->list[s];
	return m;
}

/* The eval for Color c. It's almost only mobility. Pinned pieces are still awarded for limiting opposite's king */
int evalc(int c) {
	int t, f, mn = 0, katt = 0, egf = 5200/(40 + sf[c]);
	int oc = c^1;
	u64 b, a, cb, ocb = colorb[oc];
	u64 kn = kmoves[kingpos[oc]] & (~pieceb[PAWN]);
	u64 pin = pinnedPieces(kingpos[c], oc);

	b = pieceb[PAWN] & colorb[c];
	while (b) {
		f = pullLsb(&b);
		t = f + (c << 6);

		/* The only non-mobility eval is the detection of free pawns/hanging pawns */
		int ppos = pawnprg[t]* egf / 100;
		if (!(pawnfree[t] & pieceb[PAWN] & ocb)) ppos <<= 1; //Free run?

		if (!(pawnhelp[t] & pieceb[PAWN] & colorb[c])) { // No support
			int openfile = !(pawnfile[t] & pieceb[PAWN] & ocb);
			ppos -= (openfile ? 32 : 10); // Open file
		}

		a = POCC(f, c);
		if (a) {
			ppos += _bitcnt(a & pieceb[PAWN] & colorb[c]) << 2;
		}
		if (a & kn) katt += _bitcnt(a & kn) << 4;
		mn += ppos;
	}

	cb = colorb[c] & (~pin);
	b = pieceb[KNIGHT] & cb;
	while (b) {
		f = pullLsb(&b);
		a = nmoves[f];
		if (a & kn) katt += _bitcnt(a & kn) << 4;
		mn += nmobil[f];
	}

	b = pieceb[KNIGHT] & pin;
	while (b) {
		f = pullLsb(&b);
		a = nmoves[f];
		if (a & kn) katt += _bitcnt(a & kn) << 4;
	}

	xorBit(kingpos[oc], colorb+oc); //Opposite King doesn't block mobility at all
	colorb[c] ^= pieceb[QUEEN] & cb; //Own non-pinned Queen doesn't block mobility for anybody.
	b = pieceb[QUEEN] & cb;
	while (b) {
		f = pullLsb(&b);

		a = BATT3(f) | BATT4(f) | RATT1(f) | RATT2(f);
		if (a & kn) katt += _bitcnt(a & kn) << 4;
		mn += (_bitcnt(a) << 1) * egf / 75;
	}

	colorb[oc] ^= RQU & ocb; //Opposite Queen & Rook doesn't block mobility for bishop
	b = pieceb[BISHOP] & cb;
	while (b) {
		f = pullLsb(&b);
		a = BATT3(f) | BATT4(f);
		if (a & kn) katt += _bitcnt(a & kn) << 4;
		mn += _bitcnt(a) << 3;
	}

	colorb[oc] ^= pieceb[ROOK] & ocb; //Opposite Queen doesn't block mobility for rook.
	colorb[c] ^= pieceb[ROOK] & cb; //Own non-pinned Rook doesn't block mobility for rook.
	b = pieceb[ROOK] & cb;
	while (b) {
		f = pullLsb(&b);
		a = RATT1(f) | RATT2(f);
		if (a & kn) katt += _bitcnt(a & kn) << 4;
		mn += (_bitcnt(a) << 2) * egf / 75;
	}

	colorb[c] ^= RQU & cb; // Back
	b = pin & (pieceb[ROOK] | pieceb[BISHOP] | pieceb[QUEEN]); 
	while (b) {
		f = pullLsb(&b);
		int p = identPiece(f);
		if (p == BISHOP) a = BATT3(f) | BATT4(f);
		else if (p == ROOK) a = RATT1(f) | RATT2(f);
		else a = RATT1(f) | RATT2(f) | BATT3(f) | BATT4(f);

		if (a & kn) katt += _bitcnt(a & kn) << 4;
		t = p | getDir(f, kingpos[c]);
		if ((t & 10) == 10) mn += _bitcnt(RATT1(f));
		if ((t & 18) == 18) mn += _bitcnt(RATT2(f));
		if ((t & 33) == 33) mn += _bitcnt(BATT3(f));
		if ((t & 65) == 65) mn += _bitcnt(BATT4(f));
	}

	colorb[oc] ^= pieceb[QUEEN] & ocb; //Back
	xorBit(kingpos[oc], colorb+oc); //Back
	if (sf[c] < 14) katt = katt * sf[c] / 14; //Reduce the bonus for attacking king squares
	return mn + katt;
}

int kmobilf(int c) {
	if (sf[c^1] >= 12) return 0;
	int km = kmobil[kingpos[c]];
	if (!sf[c] && sf[c^1] == 5 && pieceb[BISHOP] && !pieceb[PAWN]) { // BNK_vs_k
		int bc = bishcorn[kingpos[c]] << 3;
		if (pieceb[BISHOP] & whitesq) km += bc; else km -= bc; 
	}
	return km << 2;
}

int evallazy(int c, int matrl) {
		int ev = kmobilf(c) - kmobilf(c^1);

		if ((matrl < 0 && NOMATEMAT(1)) || (matrl > 0 && NOMATEMAT(0)))
				matrl = 0;

		return ev + (c ? -matrl : matrl);
}

u64 eval1 = 0;
int eval(int c, int matrl) {
	int ev = evalc(c) - evalc(c^1);
	eval1++;

	return ev + evallazy(c, matrl);
}

u64 nodes, qnodes;
int quiesce(u64 ch, int c, int ply, int alpha, int beta) {
	int i, best = -MAXSCORE;

	if (ply == 127) return eval(c, mat);
	if (!ch) do {
		int cmat = evallazy(c, mat);
		if (cmat - 125 >= beta) return beta;
		if (cmat + 85 <= alpha) break;
		best = eval(c, mat);
		if (best > alpha) {
			alpha = best;
			if (best >= beta) return beta;
		}
	} while(0);

	Movep mp; generate(ch, c, &mp, 1, 0);
	if (ch && mp.n == 0) return -MAXSCORE+ply;

	for (i = 0; i < mp.n; i++) {
		Move m = qpick(&mp, i);
		if (!ch && !PROM(m) && pval[PIECE(m)] > pval[CAP(m)] && swap(m) < 0) continue;

		doMove(m, c);
		qnodes++;

		int w = -quiesce(attacked(kingpos[c^1], c^1), c^1, ply+1, -beta, -alpha);

		undoMove(m, c);

		if (w > best) {
			best = w;
			if (w > alpha) {
				alpha = w;
				if (w >= beta) return beta;
			}
		}
	}
	return best > -MAXSCORE ? best : eval(c, mat);
}

int retPVMove(int c, int ply) {
	int i;
	Movep mp; GENERATE(c, &mp);
	for (i = 0; i < mp.n; i++) {
		Move m = mp.list[i];
		if (m == pv[0][ply]) return m;
	}
	return 0;
}

int ttime = 30000, mps = 0, inc = 0, post = 1, st = 0; char base[16];

int inputSearch() {
	int ex;
	fgets(irbuf,255,stdin);
	ex = protV2(irbuf, 0);
	if (analyze) { if (ex <= -1) return ex; else ex = 0; }
	if (!ponder || ex || engine != onmove) pondering = analyze;
	if (!ex) irbuf[0] = 0;
	if (ex < -1) return ex;
	if (ex != -1) return !pondering;
	ex = parseMove(irbuf, ONMV(pon), pon);
	if (!ex || ex == -1) return -1;
	irbuf[0] = 0; pon = 0;
	return -(ttime < 50);
}

const int nullvar[] = {13, 43, 149, 519, 1809, 6311, 22027};
static int nullvariance(int delta) {
	int r = 0;
	if (delta >= 4) for (r = 1; r <= 7; r++) if (delta < nullvar[r - 1]) break;
	return r;
}

#define NO_MOVE 0
#define ANY_MOVE 1
#define GOOD_MOVE 2
#define HASHP (hashb ^ hashxor[flags | 1024 | c << 11])
int search(u64 ch, int c, int d, int ply, int alpha, int beta, int pvnode, int null) {
	int i, j, n, w;
	u64 hp, hismax = 0LL;

	if (ply) pv[ply][ply] = 0;
	if ((++nodes & CNODES) == 0) {
		while (bioskey() && !sabort) sabort = inputSearch();
		if (!pondering && getTime() - starttime > maxtime) sabort = 1;
	}
	if (sabort) return alpha;

	hp = HASHP;
	if (ply && isDraw(hp, 1)) return 0;

	if (d == 0 || ply > 100) return quiesce(ch, c, ply, alpha, beta);
		
	if (alpha < -MAXSCORE+ply) alpha = -MAXSCORE+ply;
	if (beta > MAXSCORE-ply-1) beta = MAXSCORE-ply-1;
	if (alpha >= beta) return alpha;

	hstack[COUNT] = hp;
	Move hmove = ply ? 0 : retPVMove(c, 0);

	struct entry he = hashDB[hp & HMASK];
	if (he.key == hp) {
		if (he.depth >= d) {
			if (he.type <= 1 && he.value >= beta) return beta;
			if (he.type >= 1 && he.value <= alpha) return alpha;
		}
		if (!hmove) hmove = he.move;
	} 

	if (!ch && !pvnode && d <= 8) {
		w = evallazy(c, mat);
		if (w > beta + 85*d) return w;
	}

	//Null Move - pvnode => null == 0
	if (!ch && null && d > 1 && (n = _bitcnt(colorb[c] & (~pieceb[PAWN]) & (~pinnedPieces(kingpos[c], c^1)))) > 1) {
		int flagstore = flags;
		int R = (10 + d + nullvariance(evallazy(c, mat) - alpha))/4;
		if (R > d) R = d;
		flags &= 960;
		count += 0x401;
		w = -search(0LL, c^1, d-R, ply+1, -beta, 1-beta, 0, 0);
		flags = flagstore;
		count -= 0x401;
		if (d >= 6 && n <= 2 && w >= beta) w = search(ch, c, d-5, ply, beta-1, beta, 0, 0);
		if (!sabort && w >= beta) return beta;
	}

	if (d >= 5 && !hmove) { // Internal Iterative Reduction (IIR)
		d--;
	}

	int evilqueen = 0;
	if (pieceb[QUEEN] & colorb[c^1]) evilqueen = getLsb(pieceb[QUEEN] & colorb[c^1]);
	if (evilqueen && battacked(evilqueen, c^1, 0)) evilqueen = 0;

	Movep mp;
	int first = NO_MOVE;
	for (n = 1; n <= (ch ? 2 : 3); n++) {
		if (n == 1) {
		if (hmove == 0) continue;
			mp.n = 1;
		} else if (n == 2) {
			generate(ch, c, &mp, 1, 0);
		} else {
			generate(ch, c, &mp, 0, 1);
		}
		for (i = 0; i < mp.n; i++) {
			Move m;
			u64 nch;
			int ext = 0;
			if (n == 1) {
				m = hmove;
			} else {
				if (n == 2) m = qpick(&mp, i);
				else m = spick(&mp, i, ply);
				if (m == hmove) continue;
			}

			doMove(m, c);
			nch = attacked(kingpos[c^1], c^1);
			if (nch) ext++; // Check Extension
			else if (pvnode || ch);
			else if (n == 2 && d >= 2 && !PROM(m) && swap(m) < 0) ext-= (d + 1)/3; //Reduce bad exchanges
			else if (n == 3) { //LMR
				if (m == killer[ply]); //Don't reduce killers
				else if (PIECE(m) == PAWN && !(pawnfree[TO(m) + (c << 6)] & pieceb[PAWN] & colorb[c^1])); //Free pawns
				else if (evilqueen && battacked(evilqueen, c^1, 0) && swap(m) >= 0); //Don't reduce queen attacks
				else {
					u32 his = history[m & 0x1FFF];
					if (his > hismax) { hismax = his;}
					else if (d <= 5 && his*his < hismax) { undoMove(m, c); continue; }
					else if (d >= 2) ext-= (d + 1)/3;
				}
			}
			if (PROM(m) == QUEEN) ext++;

			if (first == NO_MOVE && pvnode) {
				w = -search(nch, c^1, d-1+ext, ply+1, -beta, -alpha, 1, 0);
			} else {
				w = -search(nch, c^1, d-1+ext, ply+1, -alpha-1, -alpha, 0, 1);
				if (w > alpha && ext < 0) w = -search(nch, c^1, d-1, ply+1, -alpha-1, -alpha, 0, 1);
				if (w > alpha && w < beta) w = -search(nch, c^1, d-1+ext, ply+1, -beta, -alpha, 1, 0);
			}
			undoMove(m, c);
			if (sabort) return alpha;

			if (w > alpha) {
				alpha = w;
				first = GOOD_MOVE;
				pv[ply][ply] = m;
				for (j = ply +1; pv[ply +1][j]; j++) pv[ply][j] = pv[ply +1][j];
				pv[ply][j] = 0;

				if (w == MAXSCORE-ply-1) { n = 3; break; }
				if (w >= beta) {
					if (CAP(m) == 0) {
						killer[ply] = m;
						history[m & 0x1FFF]+=(d+ext)*(d+ext);
					}
					n = 3; break;
				}
			} else if (first == NO_MOVE) first = ANY_MOVE;
	    }
	}
	if (sabort) return alpha;
	if (first == NO_MOVE) alpha = ch ? -MAXSCORE+ply : 0;

	char type = 2; // 2 = upper bound               lower = 0 : 1 = exact bound
	if (first == GOOD_MOVE) { type = (char)(alpha >= beta ? 0 : 1); hmove = pv[ply][ply]; } // Found a good move
	
	hashDB[hp & HMASK] = (struct entry) {.key = hp, .move = hmove, .value = alpha, .depth = d, .type = type};

	return alpha;
}

void reseth(int level) {
	int i, istart = level < 0 ? 1 : 0;
	memset(history, 0, sizeof(history));
	for (i = istart; i < 127; i++) killer[i] = level <= 1 ? killer[i+level] : 0;
	for (i = istart; i < 127; i++) pv[0][i] = level <= 1 ? pv[0][i+level] : 0;
	if (level <= 1) return; else pv[0][0] = 0;
	if (level >= 3) memset(hashDB, 0, sizeof(hashDB));
}

int execMove(Move m) {
	doMove(m, onmove);
	onmove ^= 1; 
	int i, c = onmove;
	if (book) for (i = 0; i < BKSIZE; i++) {
		if (bkflag[i] < 2 && (bkmove[i*32 + COUNT - 1] != m || bkmove[i*32 + COUNT] == 0)) {
			bkcount[bkflag[i]]--;
			bkflag[i] = 2;
		}
	}
	hstack[COUNT] = HASHP;
	reseth(1);
	Movep mp; i = GENERATE(c, &mp);
	if (pondering) return (mp.n == 0);
	if (mp.n == 0) {
		if (!i) {
			printf("1/2-1/2 {Stalemate}\n"); return 4;
		} else {
			printf(c ? "1-0 {White mates}\n" : "0-1 {Black mates}\n"); return 5 + c;
		}
	}
	switch (isDraw(HASHP, 2)) {
		case 1: printf("1/2-1/2 {Draw by Repetition}\n"); return 1;
		case 2: printf("1/2-1/2 {Draw by Fifty Move Rule}\n"); return 2;
		case 3: printf("1/2-1/2 {Insufficient material}\n"); return 3;
	}
	return 0;
}

#define ISRANK(c) (c >= '1' && c <= '8')
#define ISFILE(c) (c >= 'a' && c <= 'h')
int ismove(Move m, int to, int from, int piece, int prom, int h) {
	if (TO(m) != to) return 0;
	if (from < 0 && PIECE(m) != piece) return 0;
	if (from >= 0 && FROM(m) != from) return 0;
	if (ISFILE(h) && (FROM(m) & 7) != h - 'a') return 0;
	if (ISRANK(h) && (FROM(m) & 56) != 8*(h - '1')) return 0;
	if (prom && PROM(m) != prom) return 0;
	return 1;
}

int parseMove(char *s, int c, Move p) {
	int i, to, from = -1, piece = PAWN, prom = 0;
	char h = 0, c1, c2;
	if (!strncmp(s, "O-O-O", 5)) strcpy(s, c ? "Kc8" : "Kc1");
	else if (!strncmp(s, "O-O", 3)) strcpy(s, c ? "Kg8" : "Kg1");
	if (s[0] >= 'A' && s[0] <= 'Z') if ((piece = _getpiece(*s++, &i)) < 1) return -1;
	if (s[0] == 'x') s++;
	if (ISRANK(s[0])) {h = *s++; if (s[0] == 'x') s++; }
	if (!ISFILE(s[0])) return -1;
	c1 = *s++;
	if (s[0] == 'x') s++;
	if (ISFILE(s[0])) {h = c1; c1 = *s++;}
	c2 = *s++;
	if (!ISRANK(c2)) return -1;
	if (*s >= '0') {
		if (*s == '=') prom = _getpiece(s[1], &i);
		else if (*s == '+');
		else { // Algebraic Notation
			from = c1 - 'a' + 8*(c2 - '1');
			c1 = *s++; c2 = *s++;
			if (!ISFILE(c1) || !ISRANK(c2)) return -1;
			prom = _getpiece(*s, &i);
		}
	}
	to = c1 - 'a' + 8*(c2 - '1');
	if (p) {
		if (ismove(p, to, from, piece, prom, h)) return p;
		return 0;
	}
	Movep mp; GENERATE(c, &mp);
	for (i = 0; i < mp.n; i++) if (ismove(mp.list[i], to, from, piece, prom, h)) return mp.list[i];
	return 0;
}

int parseMoveNExec(char *s, int c) {
	Move m = parseMove(s, c, 0);
	if (m == -1) printf("UNKNOWN COMMAND: %s\n", s);
	else if (m == 0) fprintf(stderr,"Illegal move: %s\n",s);
	else return execMove(m);
	return -1;
}

void undo() {
	int cnt = COUNT - 1;
	if (cnt < 0) return;
	onmove ^= 1;
	Move m = mstack[cnt] >> 27L;
	undoMove(m, onmove);
	reseth(-1);
	pv[0][0] = m;
}

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
int calc(int sd, int tm) {
	int i, j, t1 = 0, m2go = mps == 0 ? 32 : 1 + mps - ((COUNT/2) % mps);
	long tmsh = MAX(tm*8L-50-m2go*5, 10);
	long searchtime = MIN(tm*6L/m2go + inc*500L, tmsh);
	long extendtime = MIN(tm*25L/m2go + inc*1000L, tmsh);
	if (st > 0) maxtime = searchtime = st*1000LL;

	u64 ch = attacked(kingpos[onmove], onmove); 
	maxtime = extendtime;
	starttime = getTime();

	sabort = iter = value[0] = 0;
	eval1 = qnodes = nodes = 0LL;
	if (book) {
		if (!bkcount[onmove]) book = 0;
		else {
			j = hashxor[starttime % 4095] % bkcount[onmove];
			for (i = 0; i < BKSIZE; i++) {
				if (bkflag[i] == onmove && j == t1++) { pv[0][0] = bkmove[i*32 + COUNT]; break; }
			}
		}
	}
	if (!book || analyze) for (iter = 1; iter <= sd; iter++) {
		value[iter] = search(ch, onmove, iter, 0, -MAXSCORE, MAXSCORE, 1, 0);
		t1 = (int)(getTime() - starttime);
		if (post && pv[0][0] && (!sabort || (!analyze && sabort>=1)) && value[iter] > -MAXSCORE) { 
			printf("%2d %5d %6d %9llu  ", iter, MEVAL(value[iter]), t1/10, nodes + qnodes);
			displaypv(); printf("\n"); 
		}
		if (sabort) break;
		if (pondering) continue;
		if (iter >= MAXSCORE-value[iter]) break;
		if (t1 < searchtime || iter == 1) continue;

		if (value[iter] - value[iter-1] < -40 && (long)maxtime == extendtime && extendtime < tmsh) {
			maxtime = MIN(extendtime*3L, tmsh-1);
			continue;
		}
		break;
	}
	if (analyze) return 1;
	pondering = 0;
	if (sabort < -1) { pon = 0; return sabort; }
	if (pon) {
		undo();
		pon = 0;
		return engine != onmove;
	}
	printf("move "); displaym(pv[0][0]); printf("\n");

	if (post && ics) printf("kibitz W: %d Nodes: %llu QNodes: %llu Evals: %llu cs: %d knps: %llu\n",
						MEVAL(value[iter > sd ? sd : iter]), nodes, qnodes, eval1, t1/10, (nodes+qnodes)/(t1+1));

	return execMove(pv[0][0]);
}

int doponder(int c) {
	pon = retPVMove(c, 0);
	if (pon) {
		pondering = 1;
		if (execMove(pon)) {
			pondering = 0;
			undo();
			pon = 0;
		}
	}
	return pondering ? 0 : -1;
}

u64 perft(int c, int d, int div) {
	int i, ply = 63 - d;
	u64 n, cnt = 0LL;
	Movep mp; generate(attacked(kingpos[c], c), c, &mp, 1, 1);
	if (d == 1 || bioskey()) return (u64)mp.n;
	for (i = 0; i < mp.n; i++) {
		Move m = mp.list[i];
		doMove(m, c);
		cnt += n = perft(c^1, d - 1, 0);
		if (div) { displaym(m); printf(" %llu\n", n); }
		undoMove(m, c);
	}
	return cnt;
}

void newGame() {
	_readbook("olibook.pgn");
	reseth(3);
	engine = 1;
	sd = 64;
}

int protV2(char* buf, int parse) {
	if (!strncmp(buf,"protover",8))
		printf("feature setboard=1 myname=\"OliThink " VER "\" colors=0 analyze=1 ping=1 sigint=0 sigterm=0 done=1\n");
	else if (!strncmp(buf,"xboard",6));
	else if (!strncmp(buf,"ping",4)) { buf[1] = 'o'; printf("%s", buf); }
	else if (!strncmp(buf,"quit",4)) return -2;
	else if (!strncmp(buf,"new",3)) return -3;
	else if (!strncmp(buf,"remove",6)) return -4;
	else if (!strncmp(buf,"force",5)) return -7;
	else if (!strncmp(buf,"go",2)) engine = pondering ? onmove^1 : onmove;
	else if (!strncmp(buf,"setboard",8)) if (parse) _parse_fen(buf+9); else return -9;
	else if (!strncmp(buf,"undo",4)) return -8;
	else if (!strncmp(buf,"easy",4)) ponder = 0;
	else if (!strncmp(buf,"hard",4)) ponder = 1;
	else if (!strncmp(buf,"analyze",7)) return -5;
	else if (!strncmp(buf,"exit",4)) return -6;
	else if (!strncmp(buf,"sd",2)) sscanf(buf+3,"%d",&sd);
	else if (!strncmp(buf,"time",4)) sscanf(buf+5,"%d",&ttime);
	else if (!strncmp(buf,"level",4)) sscanf(buf+6,"%d %s %d",&mps, base, &inc);
	else if (!strncmp(buf,"post",4)) post = 1;
	else if (!strncmp(buf,"nopost",6)) post = 0;
	else if (!strncmp(buf,"result",6)) return -6; //result 0-1 {Black mates}
	else if (!strncmp(buf,"otim",4));//otim <optime>
	else if (!strncmp(buf,"draw",4));//draw offer
	else if (!strncmp(buf,"st",2)) sscanf(buf+3,"%d",&st);
	else if (!strncmp(buf,"?",1)) return 1;
	else if (!strncmp(buf,".",1));
	else if (!strncmp(buf,"bk",2));
	else if (!strncmp(buf,"hint",4));
	else if (!strncmp(buf,"computer",8));
	else if (!strncmp(buf,"accepted",8) || !strncmp(buf,"rejected",8));//accepted/rejected <feature>
	else if (!strncmp(buf,"random",6));
	else if (!strncmp(buf,"rating",6)) ics = 1;//ICS: rating <myrat> <oprat>
	else if (!strncmp(buf,"name",4));//ICS: name <opname>
	else if (!strncmp(buf,"perft",5)) { int i; for (i = 1; i <= sd && !bioskey(); i++)
		printf("Depth: %d Nodes: %lld%c\n", i, perft(onmove, i, 0), bioskey() ? '+' : ' '); }
	else if (!strncmp(buf,"divide",5)) perft(onmove, sd, 1);
	else if (strchr(buf, '/') != NULL && strlen(buf)>16) {
		engine = -1; analyze = pondering = 1; if (parse) _parse_fen(buf); else return -9; }
	else if (strlen(buf) == 0);
	else return -1;
	return 0;
}

int input(int c) {
	int ex;
	char buf[256];
	if (irbuf[0]) strcpy(buf,irbuf); else fgets(buf,255,stdin);
	irbuf[0] = 0;
	ex = protV2(buf, 1);
	if (ex == -1) return parseMoveNExec(buf, c);
	return ex;
}

int main(int argc, char **argv) {
	int i, ex = -1; u64 m, n;

	setbuf(stdout, NULL);
	setbuf(stdin, NULL);
	for (i = 4096, n = 1, m = 6364136223846793005LL; i--; hashxor[4095-i] = n = n*m +1LL);
	for (i = 0; i < 64; i++) BIT[i] = 1LL << i;
	for (i = 0; i < 128; i++) pmoves[i] = pawnfree[i] = pawnfile[i] = pawnhelp[i] = 0LL;
	for (i = 0; i < 384; i++) pcaps[i] = 0LL;
	for (i = 0; i < 64; i++) bmask45[i] = _bishop45(i, 0LL, 0) | BIT[i];
	for (i = 0; i < 64; i++) bmask135[i] = _bishop135(i, 0LL, 0) | BIT[i];
	for (i = 0; i < 64; i++) crevoke[i] = 0x3FF;
	for (i = 0; i < 64; i++) kmoves[i] = nmoves[i] = 0LL;
	for (i = 0; i < 64; i++) if ((i/8)%2 != (i&7)%2) whitesq |= BIT[i];
	crevoke[7] ^= BIT[6];
	crevoke[63] ^= BIT[7];
	crevoke[0] ^= BIT[8];
	crevoke[56] ^= BIT[9];

	_init_rays(rays, _rook0, key000);
	_init_rays(rays + 0x2000, _rook90, key090);
	_init_rays(rays + 0x4000, _bishop45, key045);
	_init_rays(rays + 0x6000, _bishop135, key135);
	_init_shorts(nmoves, _knight);
	_init_shorts(kmoves, _king);
	_init_pawns(pmoves, pcaps, pawnfree, pawnfile, pawnhelp, 0);
	_init_pawns(pmoves + 64, pcaps + 64, pawnfree + 64, pawnfile + 64, pawnhelp + 64, 1);

	for (i = 0; i < 64; i++) nmobil[i] = (_bitcnt(nmoves[i]))*8;
	for (i = 0; i < 64; i++) kmobil[i] = (_bitcnt(nmoves[i]));
	for (i = 0; i < 32; i++) bishcorn[i] = bishcorn[63-i] = (i&7) < 4 ? cornbase[(i&7) +i/8] : -cornbase[7 -(i&7) +i/8];
	newGame();

	if (argc > 1 && !strncmp(argv[1],"-sd",3)) {
		ttime = 99999999;
		if (argc > 2) {
			sscanf(argv[2], "%d", &sd);
			if (argc > 3) { _parse_fen(argv[3]); calc(sd, ttime); return 0; }
		}
	}
	irbuf[0] = 0;

	for (;;) {
		if (engine == onmove || analyze) ex = calc(sd, ttime);
		else if (ex == 0 && ponder && engine != -1 && !book) ex = doponder(onmove);

		if (!ponder || book || engine == -1 || ex != 0) ex = input(onmove);
		if (ex == -2) break;
		if (ex == -3) newGame();
		if (ex == -4) { undo(); undo(); }
		if (ex == -5) { analyze = pondering = 1; engine = -1; }
		if (ex == -6) analyze = pondering = 0;
		if (ex == -7) { pondering = 0; engine = -1; }
		if (ex == -8) undo();
	}
	return 0;
}
