//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval.h"
#include "eval_params.h"
#include "utils.h"

const bool USE_LAZY_EVAL = true;

const int MAX_KNIGHT_MOBILITY = 8;
const int MAX_BISHOP_MOBILITY = 13;
const int MAX_ROOK_MOBILITY = 14;
const int MAX_QUEEN_MOBILITY = 27;
const int MAX_KING_PAWN_SHIELD = 9;
const int MAX_KING_PAWN_STORM = 9;
const int MAX_KING_EXPOSED = 27;
const int MAX_DISTANCE = 9;
const int MAX_ATTACK_KING = 8;
const int MAX_BOARD_CONTROL = 64;
const int NUM_PIECE_TYPES_EXCEPT_KP = 4;  // N, B, R, Q

Pair PSQ[14][64];

Pair PAWN_PASSED_BLOCKED[64];
Pair PAWN_PASSED_FREE[64];
Pair PAWN_PASSED_KING_DISTANCE[MAX_DISTANCE + 1];
Pair PAWN_PASSED_SQUARE[64];
Pair PAWN_DOUBLED[64];
Pair PAWN_ISOLATED[64];
Pair PAWN_BACKWARDS[64];
Pair PAWN_PHALANGUE[64];
Pair PAWN_RAM[64];
Pair PAWN_LAST;

Pair KNIGHT_MOBILITY[MAX_KNIGHT_MOBILITY + 1];
Pair KNIGHT_KING_DISTANCE[MAX_DISTANCE + 1];
Pair KNIGHT_STRONG[64];
Pair KNIGHT_ATTACK[NUM_PIECE_TYPES_EXCEPT_KP];

Pair BISHOP_MOBILITY[MAX_BISHOP_MOBILITY + 1];
Pair BISHOP_KING_DISTANCE[MAX_DISTANCE + 1];
Pair BISHOP_STRONG[64];
Pair BISHOP_ATTACK[NUM_PIECE_TYPES_EXCEPT_KP];

Pair ROOK_MOBILITY[MAX_ROOK_MOBILITY + 1];
Pair ROOK_KING_DISTANCE[MAX_DISTANCE + 1];
Pair ROOK_OPEN;
Pair ROOK_SEMI_OPEN;
Pair ROOK_7TH;
Pair ROOK_ATTACK[NUM_PIECE_TYPES_EXCEPT_KP];

Pair QUEEN_MOBILITY[MAX_QUEEN_MOBILITY + 1];
Pair QUEEN_KING_DISTANCE[MAX_DISTANCE + 1];
Pair QUEEN_7TH;
Pair QUEEN_ATTACK[NUM_PIECE_TYPES_EXCEPT_KP];

Pair KING_PAWN_SHIELD[MAX_KING_PAWN_SHIELD + 1];
Pair KING_PAWN_STORM[MAX_KING_PAWN_STORM + 1];
Pair KING_EXPOSED[MAX_KING_EXPOSED + 1];

Pair PIECE_PAIRS[NUM_PIECE_TYPES_EXCEPT_KP][NUM_PIECE_TYPES_EXCEPT_KP];
Pair ATTACK_KING[MAX_ATTACK_KING + 1];
Pair BOARD_CONTROL[MAX_BOARD_CONTROL + 1];
Pair TEMPO;

struct PawnHashEntry
{
	PawnHashEntry() : m_pawnHash(0) {}
	void Read(const Position& pos);

	bool FileIsSemiOpen(FLD f, COLOR side) const { return (m_ranks[side][Col(f) + 1] == 0 + 7 * side); }
	bool FileIsOpen(FLD f) const { return (FileIsSemiOpen(f, WHITE) && FileIsSemiOpen(f, BLACK)); }
	static bool RankIs7th(FLD f, COLOR side) { return (Row(f) == 1 + 5 * side); }

	U32 m_pawnHash;

	I8  m_ranks[2][10];

	U64 m_passed;
	U64 m_doubled;
	U64 m_isolated;
	U64 m_backwards;

	U64 m_attackedBy[2];
	U64 m_canBeAttackedBy[2];
};
////////////////////////////////////////////////////////////////////////////////

static PawnHashEntry* g_pawnHash = NULL;
static int            g_pawnHashSize = 0;

bool IsDrawKBPK(const Position& pos);
bool IsDrawKPK(const Position& pos);
bool IsDrawLowMaterial(const Position& pos);
int  PawnShieldPenalty(FLD f, COLOR side, const PawnHashEntry& pentry);
int  PawnStormPenalty(FLD f, COLOR side, const PawnHashEntry& pentry);
Pair PsqFunction(const vector<Pair>& v, FLD f);

int Distance(FLD f1, FLD f2)
{
	static const int dist[100] =
	{
		0, 1, 1, 1, 2, 2, 2, 2, 2, 3,
		3, 3, 3, 3, 3, 3, 4, 4, 4, 4,
		4, 4, 4, 4, 4, 5, 5, 5, 5, 5,
		5, 5, 5, 5, 5, 5, 6, 6, 6, 6,
		6, 6, 6, 6, 6, 6, 6, 6, 6, 7,
		7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
		7, 7, 7, 7, 8, 8, 8, 8, 8, 8,
		8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
		8, 9, 9, 9, 9, 9, 9, 9, 9, 9,
		9, 9, 9, 9, 9, 9, 9, 9, 9, 9
	};

	int drow = Row(f1) - Row(f2);
	int dcol = Col(f1) - Col(f2);
	return dist[drow * drow + dcol * dcol];
}
////////////////////////////////////////////////////////////////////////////////

EVAL EvalMate(const Position& pos)
{
	EVAL e =
		100 * (pos.Count(PW) - pos.Count(PB)) +
		400 * (pos.Count(NW) - pos.Count(NB)) +
		400 * (pos.Count(BW) - pos.Count(BB)) +
		600 * (pos.Count(RW) - pos.Count(RB)) +
		1200 * (pos.Count(QW) - pos.Count(QB));

	static const EVAL CENTER[64] =
	{
		-30, -20, -10,   0,   0, -10, -20, -30,
		-20, -10,   0,  10,  10,   0, -10, -20,
		-10,   0,  10,  20,  20,  10,   0, -10,
		  0,  10,  20,  30,  30,  20,  10,   0,
		  0,  10,  20,  30,  30,  20,  10,   0,
		-10,   0,  10,  20,  20,  10,   0, -10,
		-20, -10,   0,  10,  10,   0, -10, -20,
		-30, -20, -10,   0,   0, -10, -20, -30
	};

	static const EVAL MATE_ON_A1_H8[64] =
	{
		 0, 10, 20, 30, 40, 50, 60, 70,
		10, 10, 20, 30, 40, 50, 60, 60,
		20, 20, 20, 30, 40, 50, 50, 50,
		30, 30, 30, 30, 40, 40, 40, 40,
		40, 40, 40, 40, 30, 30, 30, 30,
		50, 50, 50, 40, 30, 20, 20, 20,
		60, 60, 50, 40, 30, 20, 10, 10,
		70, 60, 50, 40, 30, 20, 10,  0
	};

	static const EVAL MATE_ON_A8_H1[64] =
	{
		70, 60, 50, 40, 30, 20, 10,  0,
		60, 60, 50, 40, 30, 20, 10, 10,
		50, 50, 50, 40, 30, 20, 20, 20,
		40, 40, 40, 40, 30, 30, 30, 30,
		30, 30, 30, 30, 40, 40, 40, 40,
		20, 20, 20, 30, 40, 50, 50, 50,
		10, 10, 20, 30, 40, 50, 60, 60,
		 0, 10, 20, 30, 40, 50, 60, 70
	};

	if (pos.MatIndex(WHITE) > 3)
	{
		// stronger is white
		FLD K = pos.King(BLACK);
		if (pos.MatIndex(WHITE) == 6 && pos.Count(BW) == 1 && pos.Count(NW) == 1)
		{
			if (pos.Bits(BW) & BB_WHITE_FIELDS)
				e += MATE_ON_A8_H1[K];
			else
				e += MATE_ON_A1_H8[K];
		}
		else
			e -= CENTER[K];
		e -= Distance(pos.King(WHITE), K);
	}
	else
	{
		// stronger is black
		FLD K = pos.King(WHITE);
		if (pos.MatIndex(BLACK) == 6 && pos.Count(BB) == 1 && pos.Count(NB) == 1)
		{
			if (pos.Bits(BB) & BB_WHITE_FIELDS)
				e -= MATE_ON_A8_H1[K];
			else
				e -= MATE_ON_A1_H8[K];
		}
		else
			e += CENTER[K];
		e += Distance(pos.King(BLACK), K);
	}

	return (pos.Side() == WHITE)? e : -e;
}
////////////////////////////////////////////////////////////////////////////////

Pair EvalSide(const Position& pos, COLOR side, const PawnHashEntry& pentry)
{
	Pair score(0, 0);
	U64 x, y, y1, occ = pos.BitsAll();
	FLD f, f1;
	COLOR opp = side ^ 1;
	FLD Kopp = pos.King(opp);
	U64 zoneK = BB_KING_ATTACKS[Kopp];
	int attK = 0;
	U64 boardControl = 0;

	//
	//   PAWNS
	//

	boardControl |= pentry.m_attackedBy[side];

	// passed
	x = pentry.m_passed & pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		if (pos[f - 8 + 16 * side])
			score += PAWN_PASSED_BLOCKED[FLIP_SIDE[side][f]];
		else
			score += PAWN_PASSED_FREE[FLIP_SIDE[side][f]];
		int dist = Distance(f, Kopp);
		score += PAWN_PASSED_KING_DISTANCE[dist];
		if ((BB_PAWN_SQUARE[f][side] & pos.Bits(KING | opp)) == 0)
			score += PAWN_PASSED_SQUARE[FLIP_SIDE[side][f]];
	}

	// doubled
	x = pentry.m_doubled & pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		score += PAWN_DOUBLED[FLIP_SIDE[side][f]];
	}

	// isolated
	x = pentry.m_isolated & pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		score += PAWN_ISOLATED[FLIP_SIDE[side][f]];
	}

	// backwards
	x = pentry.m_backwards & pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		score += PAWN_BACKWARDS[FLIP_SIDE[side][f]];
	}

	// phalangues
	x = pos.Bits(PAWN | side);
	x = (x & Left(x)) | (x & Right(x));
	while (x)
	{
		f = PopLSB(x);
		score += PAWN_PHALANGUE[FLIP_SIDE[side][f]];
	}

	// rams
	x = (side == WHITE)?
		pos.Bits(PW) & Down(pos.Bits(PB)) :
		pos.Bits(PB) & Up(pos.Bits(PW));
	while (x)
	{
		f = PopLSB(x);
		score += PAWN_RAM[FLIP_SIDE[side][f]];
	}

	// last
	if (pos.Count(PAWN | side) > 0)
		score += PAWN_LAST;

	//
	//   KNIGHTS
	//

	x = pos.Bits(KNIGHT | side);
	while (x)
	{
		f = PopLSB(x);

		y = BB_KNIGHT_ATTACKS[f];
		score += KNIGHT_MOBILITY[CountBits(y)];
		boardControl |= y;

		if (y & zoneK)
			++attK;

		y1 = y & pos.BitsAll(opp);
		while (y1)
		{
			f1 = PopLSB(y1);
			int p = pos[f1] / 2 - 2;
			if (p >= 0 && p <= 3)
				score += KNIGHT_ATTACK[p];
		}

		int dist = Distance(f, Kopp);
		score += KNIGHT_KING_DISTANCE[dist];
	}

	// strong
	x = pos.Bits(KNIGHT | side) & pentry.m_attackedBy[side] & ~pentry.m_canBeAttackedBy[opp];
	while (x)
	{
		f = PopLSB(x);
		score += KNIGHT_STRONG[FLIP_SIDE[side][f]];
	}

	//
	//   BISHOPS
	//

	x = pos.Bits(BISHOP | side);
	while (x)
	{
		f = PopLSB(x);

		y = BishopAttacks(f, occ);
		score += BISHOP_MOBILITY[CountBits(y)];
		boardControl |= y;

		if (y & zoneK)
			++attK;

		y1 = y & pos.BitsAll(opp);
		while (y1)
		{
			f1 = PopLSB(y1);
			int p = pos[f1] / 2 - 2;
			if (p >= 0 && p <= 3)
				score += BISHOP_ATTACK[p];
		}

		int dist = Distance(f, Kopp);
		score += BISHOP_KING_DISTANCE[dist];
	}

	// strong
	x = pos.Bits(BISHOP | side) & pentry.m_attackedBy[side] & ~pentry.m_canBeAttackedBy[opp];
	while (x)
	{
		f = PopLSB(x);
		score += BISHOP_STRONG[FLIP_SIDE[side][f]];
	}

	//
	//   ROOKS
	//

	x = pos.Bits(ROOK | side);
	while (x)
	{
		f = PopLSB(x);

		y = RookAttacks(f, occ & ~pos.Bits(ROOK | side));
		score += ROOK_MOBILITY[CountBits(y)];
		boardControl |= y;

		if (y & zoneK)
			++attK;

		y1 = y & pos.BitsAll(opp);
		while (y1)
		{
			f1 = PopLSB(y1);
			int p = pos[f1] / 2 - 2;
			if (p >= 0 && p <= 3)
				score += ROOK_ATTACK[p];
		}

		int dist = Distance(f, Kopp);
		score += ROOK_KING_DISTANCE[dist];

		if (pentry.FileIsOpen(f))
			score += ROOK_OPEN;
		else if (pentry.FileIsSemiOpen(f, side))
			score += ROOK_SEMI_OPEN;

		if (pentry.RankIs7th(f, side))
			score += ROOK_7TH;
	}

	//
	//   QUEENS
	//

	x = pos.Bits(QUEEN | side);
	while (x)
	{
		f = PopLSB(x);

		y = QueenAttacks(f, occ);
		score += QUEEN_MOBILITY[CountBits(y)];
		boardControl |= y;

		if (y & zoneK)
			++attK;

		y1 = y & pos.BitsAll(opp);
		while (y1)
		{
			f1 = PopLSB(y1);
			int p = pos[f1] / 2 - 2;
			if (p >= 0 && p <= 3)
				score += QUEEN_ATTACK[p];
		}

		int dist = Distance(f, Kopp);
		score += QUEEN_KING_DISTANCE[dist];

		if (pentry.RankIs7th(f, side))
			score += QUEEN_7TH;
	}

	//
	//   KINGS
	//

	f = pos.King(side);

	int shield = PawnShieldPenalty(f, side, pentry);
	score += KING_PAWN_SHIELD[shield];
	
	int storm = PawnStormPenalty(f, side, pentry);
	score += KING_PAWN_STORM[storm];
	
	y = QueenAttacks(f, occ);
	score += KING_EXPOSED[CountBits(y)];

	boardControl |= BB_KING_ATTACKS[f];

	//
	//   PIECE PAIRS
	//

	for (int i = 0; i < 4; ++i)
	{
		PIECE p1 = (KNIGHT + 2 * i) | side;
		for (int j = 0; j < 4; ++j)
		{
			PIECE p2 = (KNIGHT + 2 * j) | side;
			if (p1 == p2 && pos.Count(p1) == 2)
				score += PIECE_PAIRS[i][j];
			else if (p1 != p2 && pos.Count(p1) == 1 && pos.Count(p2) == 1)
				score += PIECE_PAIRS[i][j];
		}
	}

	//
	//   ATTACKS ON KING ZONE
	//

	if (attK > MAX_ATTACK_KING)
		attK = MAX_ATTACK_KING;
	score += ATTACK_KING[attK];

	//
	//   BOARD CONTROL
	//

	score += BOARD_CONTROL[CountBits(boardControl)];

	//
	//   TEMPO
	//

	if (pos.Side() == side)
		score += TEMPO;

	return score;
}
////////////////////////////////////////////////////////////////////////////////

EVAL Evaluate(const Position& pos, EVAL alpha, EVAL beta)
{
	int mid = pos.MatIndex(WHITE) + pos.MatIndex(BLACK);
	int end = 64 - mid;

	Pair score = pos.Score();

	//
	//   LAZY EVAL
	//

	if (USE_LAZY_EVAL)
	{
		EVAL lazy = (score.mid * mid + score.end * end) / 64;
		lazy *= (1 - 2 * pos.Side());

		if (lazy < alpha - 250)
			return alpha;
		if (lazy > beta + 250)
			return beta;
	}

	int index = pos.PawnHash() % g_pawnHashSize;
	PawnHashEntry& pentry = g_pawnHash[index];
	if (pentry.m_pawnHash != pos.PawnHash())
		pentry.Read(pos);

	score += EvalSide(pos, WHITE, pentry);
	score -= EvalSide(pos, BLACK, pentry);

	EVAL e = (score.mid * mid + score.end * end) / 64;

	if (e > 0 && pos.MatIndex(WHITE) < 5 && pos.Count(PW) == 0)
		e = 0;
	if (e < 0 && pos.MatIndex(BLACK) < 5 && pos.Count(PB) == 0)
		e = 0;

	if (pos.Side() == BLACK)
		e = -e;

	return e;
}
////////////////////////////////////////////////////////////////////////////////

void InitEval(const vector<int>& x)
{
	g_eval.SetValues(x);

	for (FLD f = 0; f < 64; ++f)
	{
		PSQ[PW][f] = g_eval.GetPSQ(Pawn, f);
		PSQ[NW][f] = g_eval.GetPSQ(Knight, f);
		PSQ[BW][f] = g_eval.GetPSQ(Bishop, f);
		PSQ[RW][f] = g_eval.GetPSQ(Rook, f);
		PSQ[QW][f] = g_eval.GetPSQ(Queen, f);
		PSQ[KW][f] = g_eval.GetPSQ(King, f);

		PSQ[PB][FLIP[f]] = -PSQ[PW][f];
		PSQ[NB][FLIP[f]] = -PSQ[NW][f];
		PSQ[BB][FLIP[f]] = -PSQ[BW][f];
		PSQ[RB][FLIP[f]] = -PSQ[RW][f];
		PSQ[QB][FLIP[f]] = -PSQ[QW][f];
		PSQ[KB][FLIP[f]] = -PSQ[KW][f];

		PAWN_PASSED_BLOCKED[f] = g_eval.GetPSQ(PawnPassedBlocked, f);
		PAWN_PASSED_FREE[f] = g_eval.GetPSQ(PawnPassedFree, f);
		PAWN_PASSED_SQUARE[f] = g_eval.GetPSQ(PawnPassedSquare, f);
		PAWN_DOUBLED[f] = g_eval.GetPSQ(PawnDoubled, f);
		PAWN_ISOLATED[f] = g_eval.GetPSQ(PawnIsolated, f);
		PAWN_BACKWARDS[f] = g_eval.GetPSQ(PawnBackwards, f);
		PAWN_PHALANGUE[f] = g_eval.GetPSQ(PawnPhalangue, f);
		PAWN_RAM[f] = g_eval.GetPSQ(PawnRam, f);

		KNIGHT_STRONG[f] = g_eval.GetPSQ(KnightStrong, f);
		BISHOP_STRONG[f] = g_eval.GetPSQ(BishopStrong, f);
	}

	PAWN_LAST = g_eval[PawnLast][0];
	ROOK_OPEN = g_eval[RookOpen][0];
	ROOK_SEMI_OPEN = g_eval[RookSemiOpen][0];
	ROOK_7TH = g_eval[Rook7th][0];
	QUEEN_7TH = g_eval[Queen7th][0];
	TEMPO = g_eval[Tempo][0];

	for (int i = 0; i <= MAX_KNIGHT_MOBILITY; ++i)
		KNIGHT_MOBILITY[i] = g_eval.GetScaled(KnightMobility, i, MAX_KNIGHT_MOBILITY);

	for (int i = 0; i <= MAX_BISHOP_MOBILITY; ++i)
		BISHOP_MOBILITY[i] = g_eval.GetScaled(BishopMobility, i, MAX_BISHOP_MOBILITY);

	for (int i = 0; i <= MAX_ROOK_MOBILITY; ++i)
		ROOK_MOBILITY[i] = g_eval.GetScaled(RookMobility, i, MAX_ROOK_MOBILITY);

	for (int i = 0; i <= MAX_QUEEN_MOBILITY; ++i)
		QUEEN_MOBILITY[i] = g_eval.GetScaled(QueenMobility, i, MAX_QUEEN_MOBILITY);

	for (int i = 0; i <= MAX_KING_PAWN_SHIELD; ++i)
		KING_PAWN_SHIELD[i] = g_eval.GetScaled(KingPawnShield, i, MAX_KING_PAWN_SHIELD);

	for (int i = 0; i <= MAX_KING_PAWN_STORM; ++i)
		KING_PAWN_STORM[i] = g_eval.GetScaled(KingPawnStorm, i, MAX_KING_PAWN_STORM);

	for (int i = 0; i <= MAX_KING_EXPOSED; ++i)
		KING_EXPOSED[i] = g_eval.GetScaled(KingExposed, i, MAX_KING_EXPOSED);

	for (int i = 0; i <= MAX_DISTANCE; ++i)
	{
		PAWN_PASSED_KING_DISTANCE[i] = g_eval.GetScaled(PawnPassedKingDistance, i, MAX_DISTANCE);
		KNIGHT_KING_DISTANCE[i] = g_eval.GetScaled(KnightKingDistance, i, MAX_DISTANCE);
		BISHOP_KING_DISTANCE[i] = g_eval.GetScaled(BishopKingDistance, i, MAX_DISTANCE);
		ROOK_KING_DISTANCE[i] = g_eval.GetScaled(RookKingDistance, i, MAX_DISTANCE);
		QUEEN_KING_DISTANCE[i] = g_eval.GetScaled(QueenKingDistance, i, MAX_DISTANCE);
	}

	for (int i = 0; i < 4; ++i)
		for (int j = 0; j < 4; ++j)
			PIECE_PAIRS[i][j] = g_eval[PiecePairs][4 * i + j];

	for (int i = 0; i <= MAX_ATTACK_KING; ++i)
		ATTACK_KING[i] = g_eval.GetScaled(AttackKing, i, MAX_ATTACK_KING);

	for (int i = 0; i <= MAX_BOARD_CONTROL; ++i)
		BOARD_CONTROL[i] = g_eval.GetScaled(BoardControl, i, MAX_BOARD_CONTROL);

	for (int i = 0; i < 4; ++i)
	{
		KNIGHT_ATTACK[i] = g_eval[KnightAttack][i];
		BISHOP_ATTACK[i] = g_eval[BishopAttack][i];
		ROOK_ATTACK[i] = g_eval[RookAttack][i];
		QUEEN_ATTACK[i] = g_eval[QueenAttack][i];
	}
}
////////////////////////////////////////////////////////////////////////////////

void InitEval()
{
	vector<int> x;
	if (!g_eval.Read(WEIGHTS_FILE))
	{
		SetDefaultValues(x);
		g_eval.SetValues(x);
		g_eval.Write(WEIGHTS_FILE);
	}

	g_eval.GetValues(x);
	InitEval(x);
}
////////////////////////////////////////////////////////////////////////////////

bool IsDrawKBPK(const Position& pos)
{
	if (pos.Endgame_KBP(WHITE) && pos.Endgame_K(BLACK))
	{
		FLD f = LSB(pos.Bits(PW));
		if (Col(f) == 0)
		{
			// A8
			if (pos.Bits(BW) & BB_BLACK_FIELDS)
				if (pos.Bits(KB) & LL(0xc0c0000000000000))
					return true;
		}
		else if (Col(f) == 7)
		{
			// H8
			if (pos.Bits(BW) & BB_WHITE_FIELDS)
				if (pos.Bits(KB) & LL(0x0303000000000000))
					return true;
		}
	}
	else if (pos.Endgame_KBP(BLACK) && pos.Endgame_K(WHITE))
	{
		FLD f = LSB(pos.Bits(PB));
		if (Col(f) == 0)
		{
			// A1
			if (pos.Bits(BB) & BB_WHITE_FIELDS)
				if (pos.Bits(KW) & LL(0x000000000000c0c0))
					return true;
		}
		else if (Col(f) == 7)
		{
			// H1
			if (pos.Bits(BB) & BB_BLACK_FIELDS)
				if (pos.Bits(KW) & LL(0x0000000000000303))
					return true;
		}
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////

bool IsDrawKPK(const Position& pos)
{
	if (pos.Endgame_KP(WHITE) && pos.Endgame_K(BLACK))
	{
		U64 drawZone = 0;
		FLD f = LSB(pos.Bits(PW));
		if (Col(f) == 0)
			drawZone |= BB_DIR[f][DIR_U] | BB_SINGLE[B8] | BB_SINGLE[B7];
		else if (Col(f) == 7)
			drawZone |= BB_DIR[f][DIR_U] | BB_SINGLE[G8] | BB_SINGLE[G7];
		else
		{
			drawZone |= BB_SINGLE[f - 8];
			if (Row(f) > 2)
				drawZone |= BB_SINGLE[f - 16];
		}
		if (pos.Bits(KB) & drawZone)
			return true;
	}
	else if(pos.Endgame_KP(BLACK) && pos.Endgame_K(WHITE))
	{
		U64 drawZone = 0;
		FLD f = LSB(pos.Bits(PB));
		if (Col(f) == 0)
			drawZone |= BB_DIR[f][DIR_D] | BB_SINGLE[B1] | BB_SINGLE[B2];
		else if (Col(f) == 7)
			drawZone |= BB_DIR[f][DIR_D] | BB_SINGLE[G1] | BB_SINGLE[G2];
		else
		{
			drawZone |= BB_SINGLE[f + 8];
			if (Row(f) < 5)
				drawZone |= BB_SINGLE[f + 16];
		}
		if (pos.Bits(KW) & drawZone)
			return true;
	}

	return false;
}
////////////////////////////////////////////////////////////////////////////////

bool IsDrawLowMaterial(const Position& pos)
{
	if (pos.Count(PW) == 0 && pos.Count(PB) == 0)
	{
		if (pos.MatIndex(WHITE) < 5 && pos.MatIndex(BLACK) < 5)
			return true;
		if (pos.MatIndex(WHITE) == 6 && pos.Count(NW) == 2 && pos.MatIndex(BLACK) < 5)
			return true;
		if (pos.MatIndex(BLACK) == 6 && pos.Count(NB) == 2 && pos.MatIndex(WHITE) < 5)
			return true;
	}
	return false;
}
////////////////////////////////////////////////////////////////////////////////

int PawnShieldPenalty(FLD f, COLOR side, const PawnHashEntry& pEntry)
{
	static const int delta[2][8] =
	{
		{ 3, 3, 3, 3, 2, 1, 0, 3 },
		{ 3, 0, 1, 2, 3, 3, 3, 3 }
	};

	int fileK = Col(f) + 1;
	int penalty = 0;

	for (int file = fileK - 1; file < fileK + 2; ++file)
	{
		int rank = pEntry.m_ranks[side][file];
		penalty += delta[side][rank];
	}

	if (penalty < 0)
		penalty = 0;
	if (penalty > 9)
		penalty = 9;

	return penalty;
}
////////////////////////////////////////////////////////////////////////////////

int PawnStormPenalty(FLD f, COLOR side, const PawnHashEntry& pEntry)
{
	COLOR opp = side ^ 1;

	static const int delta[2][8] =
	{
		{ 0, 0, 0, 0, 1, 2, 3, 3 },
		{ 3, 3, 2, 1, 0, 0, 0, 0 }
	};

	int fileK = Col(f) + 1;
	int penalty = 0;

	for (int file = fileK - 1; file < fileK + 2; ++file)
	{
		int rank = pEntry.m_ranks[opp][file];
		penalty += delta[side][rank];
	}

	if (penalty < 0)
		penalty = 0;
	if (penalty > 9)
		penalty = 9;

	return penalty;
}
////////////////////////////////////////////////////////////////////////////////

void SetPawnHashSize(double mb)
{
	if (g_pawnHash)
		delete[] g_pawnHash;

	g_pawnHashSize = int(1024 * 1024 * mb / sizeof(PawnHashEntry));
	g_pawnHash = new PawnHashEntry[g_pawnHashSize];
}
////////////////////////////////////////////////////////////////////////////////

void PawnHashEntry::Read(const Position& pos)
{
	m_pawnHash = pos.PawnHash();

	memset(m_ranks[WHITE], 0, 10);
	memset(m_ranks[BLACK], 7, 10);

	m_passed = 0;
	m_doubled = 0;
	m_isolated = 0;
	m_backwards = 0;

	m_attackedBy[WHITE] = m_attackedBy[BLACK] = 0;
	m_canBeAttackedBy[WHITE] = m_canBeAttackedBy[BLACK] = 0;

	U64 x, y;
	FLD f;

	// first pass
	x = pos.Bits(PW);
	while (x)
	{
		f = PopLSB(x);

		int file = Col(f) + 1;
		int rank = Row(f);

		if (rank > m_ranks[WHITE][file])
			m_ranks[WHITE][file] = rank;

		y = BB_PAWN_ATTACKS[f][WHITE];
		m_attackedBy[WHITE] |= y;
		while (y)
		{
			m_canBeAttackedBy[WHITE] |= y;
			y = Up(y);
		}
	}
	x = pos.Bits(PB);
	while (x)
	{
		f = PopLSB(x);

		int file = Col(f) + 1;
		int rank = Row(f);

		if (rank < m_ranks[BLACK][file])
			m_ranks[BLACK][file] = rank;

		y = BB_PAWN_ATTACKS[f][BLACK];
		m_attackedBy[BLACK] |= y;
		while (y)
		{
			m_canBeAttackedBy[BLACK] |= y;
			y = Down(y);
		}
	}

	// second pass
	x = pos.Bits(PW);
	while (x)
	{
		f = PopLSB(x);
		int file = Col(f) + 1;
		int rank = Row(f);
		if (m_ranks[BLACK][file] == 7 && m_ranks[BLACK][file - 1] >= rank && m_ranks[BLACK][file + 1] >= rank)
			m_passed |= BB_SINGLE[f];
		if (rank != m_ranks[WHITE][file])
			m_doubled |= BB_SINGLE[f];
		if (m_ranks[WHITE][file - 1] == 0 && m_ranks[WHITE][file + 1] == 0)
			m_isolated |= BB_SINGLE[f];
		else if (rank > m_ranks[WHITE][file - 1] && rank > m_ranks[WHITE][file + 1])
			m_backwards |= BB_SINGLE[f];
	}
	x = pos.Bits(PB);
	while (x)
	{
		f = PopLSB(x);
		int file = Col(f) + 1;
		int rank = Row(f);
		if (m_ranks[WHITE][file] == 0 && m_ranks[WHITE][file - 1] <= rank && m_ranks[WHITE][file + 1] <= rank)
			m_passed |= BB_SINGLE[f];
		if (rank != m_ranks[BLACK][file])
			m_doubled |= BB_SINGLE[f];
		if (m_ranks[BLACK][file - 1] == 7 && m_ranks[BLACK][file + 1] == 7)
			m_isolated |= BB_SINGLE[f];
		else if (rank < m_ranks[BLACK][file - 1] && rank < m_ranks[BLACK][file + 1])
			m_backwards |= BB_SINGLE[f];
	}
}
////////////////////////////////////////////////////////////////////////////////

void Features::AddFeatureLinear(size_t tagEnum, int value, int maxValue)
{
	double z = double(value) / maxValue;
	size_t offset = g_eval.GetOffset(tagEnum);
	m_features[offset] += m_mid * z;
	m_features[offset + NUM_FEATURES / 2] += m_end * z;
}
////////////////////////////////////////////////////////////////////////////////

void Features::AddFeatureSquare(size_t tagEnum, int value, int maxValue)
{
	double z = double(value) / maxValue;
	size_t offset = g_eval.GetOffset(tagEnum);
	m_features[offset] += m_mid * z;
	m_features[offset + NUM_FEATURES / 2] += m_end * z;
	m_features[offset + 1] += m_mid * z * z;
	m_features[offset + 1 + NUM_FEATURES / 2] += m_end * z * z;
}
////////////////////////////////////////////////////////////////////////////////

void Features::AddFeatureScaled(size_t tagEnum, int value, int maxValue)
{
	if (g_eval[tagEnum].size() == 2)
		AddFeatureSquare(tagEnum, value, maxValue);
	else
		AddFeatureLinear(tagEnum, value, maxValue);
}
////////////////////////////////////////////////////////////////////////////////

void Features::AddFeatureVector(size_t tagEnum, size_t arrayIndex)
{
	size_t offset = g_eval.GetOffset(tagEnum) + arrayIndex;
	m_features[offset] += m_mid;
	m_features[offset + NUM_FEATURES / 2] += m_end;
}
////////////////////////////////////////////////////////////////////////////////

void Features::AddPsqFeatures(size_t tagEnum, FLD f, COLOR side)
{
	size_t offset = g_eval.GetOffset(tagEnum);
	f = FLIP_SIDE[side][f];

	double x = (Col(f) - 3.5) / 3.5;
	double y = (3.5 - Row(f)) / 3.5;

	m_features[offset] += m_mid;
	m_features[offset + 1] += m_mid * x * x;
	m_features[offset + 2] += m_mid * x;
	m_features[offset + 3] += m_mid * y * y;
	m_features[offset + 4] += m_mid * y;
	m_features[offset + 5] += m_mid * x * y;

	m_features[offset + m_features.size() / 2] += m_end;
	m_features[offset + 1 + m_features.size() / 2] += m_end * x * x;
	m_features[offset + 2 + m_features.size() / 2] += m_end * x;
	m_features[offset + 3 + m_features.size() / 2] += m_end * y * y;
	m_features[offset + 4 + m_features.size() / 2] += m_end * y;
	m_features[offset + 5 + m_features.size() / 2] += m_end * x * y;
}
////////////////////////////////////////////////////////////////////////////////

void Features::GetFeatures()
{
	m_features.resize(NUM_FEATURES);

	int index = m_pos.PawnHash() % g_pawnHashSize;
	PawnHashEntry& pentry = g_pawnHash[index];
	if (pentry.m_pawnHash != m_pos.PawnHash())
		pentry.Read(m_pos);

	GetFeaturesSide(WHITE, pentry);
	GetFeaturesSide(BLACK, pentry);
}
////////////////////////////////////////////////////////////////////////////////

void Features::GetFeaturesSide(COLOR side, const PawnHashEntry& pentry)
{
	assert(m_features.size() == NUM_FEATURES);

	m_mid = (m_pos.MatIndex(WHITE) + m_pos.MatIndex(BLACK)) / 64.0;
	m_end = 1.0 - m_mid;

	m_mid *= (1 - 2 * side);
	m_end *= (1 - 2 * side);

	FLD f, f1;
	U64 x, y, y1, occ = m_pos.BitsAll();
	COLOR opp = side ^ 1;
	FLD Kopp = m_pos.King(opp);
	U64 zoneK = BB_KING_ATTACKS[Kopp];
	int attK = 0;
	U64 boardControl = 0;

	//
	//   PAWNS
	//

	boardControl |= pentry.m_attackedBy[side];

	x = m_pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(Pawn, f, side);
	}

	// passed
	x = pentry.m_passed & m_pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		if (m_pos[f - 8 + 16 * side])
			AddPsqFeatures(PawnPassedBlocked, f, side);
		else
			AddPsqFeatures(PawnPassedFree, f, side);
		int dist = Distance(f, Kopp);
		AddFeatureScaled(PawnPassedKingDistance, dist, MAX_DISTANCE);
		if ((BB_PAWN_SQUARE[f][side] & m_pos.Bits(KING | opp)) == 0)
			AddPsqFeatures(PawnPassedSquare, f, side);
	}

	// doubled
	x = pentry.m_doubled & m_pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(PawnDoubled, f, side);
	}

	// isolated
	x = pentry.m_isolated & m_pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(PawnIsolated, f, side);
	}

	// backwards
	x = pentry.m_backwards & m_pos.Bits(PAWN | side);
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(PawnBackwards, f, side);
	}

	// phalangues
	x = m_pos.Bits(PAWN | side);
	x = (x & Left(x)) | (x & Right(x));
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(PawnPhalangue, f, side);
	}

	// rams
	x = (side == WHITE)?
		m_pos.Bits(PW) & Down(m_pos.Bits(PB)) :
		m_pos.Bits(PB) & Up(m_pos.Bits(PW));
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(PawnRam, f, side);
	}

	// last
	if (m_pos.Count(PAWN | side) > 0)
		AddFeatureScaled(PawnLast, 1, 1);

	//
	//   KNIGHTS
	//

	x = m_pos.Bits(KNIGHT | side);
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(Knight, f, side);

		y = BB_KNIGHT_ATTACKS[f];
		AddFeatureScaled(KnightMobility, CountBits(y), MAX_KNIGHT_MOBILITY);
		boardControl |= y;

		if (y & zoneK)
			++attK;

		y1 = y & m_pos.BitsAll(opp);
		while (y1)
		{
			f1 = PopLSB(y1);
			int p = m_pos[f1] / 2 - 2;
			if (p >= 0 && p <= 3)
				AddFeatureVector(KnightAttack, p);
		}

		int dist = Distance(f, Kopp);
		AddFeatureScaled(KnightKingDistance, dist, MAX_DISTANCE);
	}

	// strong
	x = m_pos.Bits(KNIGHT | side) & pentry.m_attackedBy[side] & ~pentry.m_canBeAttackedBy[opp];
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(KnightStrong, f, side);
	}

	//
	//   BISHOPS
	//

	x = m_pos.Bits(BISHOP | side);
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(Bishop, f, side);

		y = BishopAttacks(f, occ);
		AddFeatureScaled(BishopMobility, CountBits(y), MAX_BISHOP_MOBILITY);
		boardControl |= y;

		if (y & zoneK)
			++attK;

		y1 = y & m_pos.BitsAll(opp);
		while (y1)
		{
			f1 = PopLSB(y1);
			int p = m_pos[f1] / 2 - 2;
			if (p >= 0 && p <= 3)
				AddFeatureVector(BishopAttack, p);
		}

		int dist = Distance(f, Kopp);
		AddFeatureScaled(BishopKingDistance, dist, MAX_DISTANCE);
	}

	// strong
	x = m_pos.Bits(BISHOP | side) & pentry.m_attackedBy[side] & ~pentry.m_canBeAttackedBy[opp];
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(BishopStrong, f, side);
	}

	//
	//   ROOKS
	//

	x = m_pos.Bits(ROOK | side);
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(Rook, f, side);

		y = RookAttacks(f, occ);
		AddFeatureScaled(RookMobility, CountBits(y), MAX_ROOK_MOBILITY);
		boardControl |= y;

		if (y & zoneK)
			++attK;

		y1 = y & m_pos.BitsAll(opp);
		while (y1)
		{
			f1 = PopLSB(y1);
			int p = m_pos[f1] / 2 - 2;
			if (p >= 0 && p <= 3)
				AddFeatureVector(RookAttack, p);
		}

		int dist = Distance(f, Kopp);
		AddFeatureScaled(RookKingDistance, dist, MAX_DISTANCE);

		if (pentry.FileIsOpen(f))
			AddFeatureScaled(RookOpen, 1, 1);
		else if (pentry.FileIsSemiOpen(f, side))
			AddFeatureScaled(RookSemiOpen, 1, 1);

		if (pentry.RankIs7th(f, side))
			AddFeatureScaled(Rook7th, 1, 1);
	}

	//
	//   QUEENS
	//

	x = m_pos.Bits(QUEEN | side);
	while (x)
	{
		f = PopLSB(x);
		AddPsqFeatures(Queen, f, side);

		y = QueenAttacks(f, occ);
		AddFeatureScaled(QueenMobility, CountBits(y), MAX_QUEEN_MOBILITY);
		boardControl |= y;

		if (y & zoneK)
			++attK;

		y1 = y & m_pos.BitsAll(opp);
		while (y1)
		{
			f1 = PopLSB(y1);
			int p = m_pos[f1] / 2 - 2;
			if (p >= 0 && p <= 3)
				AddFeatureVector(QueenAttack, p);
		}

		int dist = Distance(f, Kopp);
		AddFeatureScaled(QueenKingDistance, dist, MAX_DISTANCE);

		if (pentry.RankIs7th(f, side))
			AddFeatureScaled(Queen7th, 1, 1);
	}

	//
	//   KINGS
	//

	f = m_pos.King(side);
	AddPsqFeatures(King, f, side);

	int shield = PawnShieldPenalty(f, side, pentry);
	AddFeatureScaled(KingPawnShield, shield, MAX_KING_PAWN_SHIELD);
	
	int storm = PawnStormPenalty(f, side, pentry);
	AddFeatureScaled(KingPawnStorm, storm, MAX_KING_PAWN_STORM);
	
	y = QueenAttacks(f, occ);
	AddFeatureScaled(KingExposed, CountBits(y), MAX_KING_EXPOSED);

	boardControl |= BB_KING_ATTACKS[f];

	//
	//   PIECE PAIRS
	//

	for (int i = 0; i < 4; ++i)
	{
		PIECE p1 = (KNIGHT + 2 * i) | side;
		for (int j = 0; j < 4; ++j)
		{
			PIECE p2 = (KNIGHT + 2 * j) | side;
			if (p1 == p2 && m_pos.Count(p1) == 2)
				AddFeatureVector(PiecePairs, 4 * i + j);
			else if (p1 != p2 && m_pos.Count(p1) == 1 && m_pos.Count(p2) == 1)
				AddFeatureVector(PiecePairs, 4 * i + j);
		}
	}

	//
	//   ATTACKS ON KING ZONE
	//

	if (attK > MAX_ATTACK_KING)
		attK = MAX_ATTACK_KING;
	AddFeatureScaled(AttackKing, attK, MAX_ATTACK_KING);

	//
	//   BOARD CONTROL
	//

	AddFeatureScaled(BoardControl, CountBits(boardControl), MAX_BOARD_CONTROL);

	//
	//   TEMPO
	//

	if (m_pos.Side() == side)
		AddFeatureScaled(Tempo, 1, 1);
}
////////////////////////////////////////////////////////////////////////////////
