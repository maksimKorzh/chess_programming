//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#include "eval.h"
#include "moves.h"
#include "notation.h"
#include "search.h"
#include "utils.h"

extern Position g_pos;
extern deque<string> g_queue;
extern bool g_console;
extern bool g_xboard;
extern bool g_uci;

const bool USE_FUTILITY = true;
const bool USE_ROOT_WINDOW = true;

const bool USE_LMR = true;
const int LMR_MIN_DEPTH = 3;
const int LMR_MIN_QUIET_MOVE = 4;
const int LMR_QUIET_MOVES_DIVISOR = 12;

const int QCHECKS = 1;

int g_stHard = 2000;
int g_stSoft = 2000;

int g_inc = 0;
int g_restTime = 0;
int g_restMoves = 0;
int g_movesPerSession = 0;

int g_sd = 0;
NODES g_sn = 0;
U32 g_level = 100;
double g_knps = 0;

static U8           g_flags = 0;
static HashEntry*   g_hash = NULL;
static U8           g_hashAge = 0;
static int          g_hashSize = 0;
static int          g_history[64][14];
static int          g_historyMax = 0;
static int          g_iter = 0;
static vector<Move> g_iterPV;
static Move         g_killers[MAX_PLY];
static MoveList     g_lists[MAX_PLY];
static NODES        g_nodes = 0;
static vector<Move> g_pv[MAX_PLY];
static EVAL         g_score = 0;
static U32          g_t0 = 0;

const int SORT_HASH        = 3000000;
const int SORT_CAPTURE     = 2000000;
const int SORT_KILLER      = 1000000;
const int SORT_HISTORY_MAX = 1000000;

const EVAL SORT_VALUE[14] = { 0, 0, 100, 100, 400, 400, 400, 400, 600, 600, 1200, 1200, 20000, 20000 };

EVAL       AlphaBetaRoot(Position& pos, EVAL alpha, EVAL beta, int depth);
EVAL       AlphaBetaPV(Position& pos, EVAL alpha, EVAL beta, int depth, int ply);
EVAL       AlphaBetaNonPV(Position& pos, EVAL beta, int depth, int ply);
EVAL       AlphaBetaQ(Position& pos, EVAL alpha, EVAL beta, int ply, int qply);
void       CheckInput();
void       CheckLimits();
void       ClearKillers();
int        Extensions(bool inCheck, Move mv, Move lastMove, bool onPV);
Move       GetNextBest(MoveList& mvlist, size_t i);
void       ProcessInput(const string& s);
HashEntry* ProbeHash(const Position& pos);
void       RecordHash(const Position& pos, Move mv, EVAL score, int depth, int ply, U8 type);
EVAL       SEE(const Position& pos, Move mv);
void       UpdateHistory(Move mv, int depth);
void       UpdatePV(Move mv, int ply);
void       UpdateSortScores(MoveList& mvlist, Move hashMove, int ply);

void AdjustSpeed()
{
	if (g_flags & SEARCH_TERMINATED)
		return;

	U32 dt = GetProcTime() - g_t0;
	if (g_knps > 0 && g_iter > 1)
	{
		double expectedTime = g_nodes / g_knps;
		while (dt < expectedTime)
		{
			SleepMillisec(1);
			CheckInput();
			CheckLimits();
			if (g_flags & SEARCH_TERMINATED)
				return;

			dt = GetProcTime() - g_t0;
		}
	}
}
////////////////////////////////////////////////////////////////////////////////

EVAL AlphaBetaRoot(Position& pos, EVAL alpha, EVAL beta, int depth)
{
	int ply = 0;
	g_pv[ply].clear();

	//
	//   PROBING HASH
	//

	Move hashMove = 0;
	HashEntry* pEntry = ProbeHash(pos);
	if (pEntry != NULL)
	{
		hashMove = pEntry->GetMove();
	}

	int legalMoves = 0;
	Move bestMove = 0;
	bool inCheck = pos.InCheck();
	U8 hashType = HASH_ALPHA;

	MoveList& mvlist = g_lists[ply];
	if (inCheck)
		GenMovesInCheck(pos, mvlist);
	else
		GenAllMoves(pos, mvlist);
	UpdateSortScores(mvlist, hashMove, ply);

	Move lastMove = pos.LastMove();
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = GetNextBest(mvlist, i);
		if (pos.MakeMove(mv))
		{
			++g_nodes;
			++legalMoves;

			if (g_uci && GetProcTime() - g_t0 > 1000)
				cout << "info currmove " << MoveToStrLong(mv) << " currmovenumber " << legalMoves << endl;

			int newDepth = depth - 1;

			//
			//   EXTENSIONS
			//

			newDepth += Extensions(inCheck, mv, lastMove, true);

			EVAL e;
			if (legalMoves == 1)
				e = -AlphaBetaPV(pos, -beta, -alpha, newDepth, ply + 1);
			else
			{
				e = -AlphaBetaNonPV(pos, -alpha, newDepth, ply + 1);
				if (e > alpha && e < beta)
					e = -AlphaBetaPV(pos, -beta, -alpha, newDepth, ply + 1);
			}
			pos.UnmakeMove();

			CheckLimits();
			CheckInput();

			if (g_flags & SEARCH_TERMINATED)
				break;

			if (e > alpha)
			{
				alpha = e;
				bestMove = mv;
				hashType = HASH_EXACT;
				UpdatePV(mv, ply);
				if (!mv.Captured() && !mv.Promotion())
					UpdateHistory(mv, depth);
			}

			if (alpha >= beta)
			{
				hashType = HASH_BETA;
				if (!mv.Captured() && !mv.Promotion())
					g_killers[ply] = mv;
				break;
			}
		}
	}

	if (legalMoves == 0)
	{
		if (inCheck)
			alpha = -CHECKMATE_SCORE + ply;
		else
			alpha = DRAW_SCORE;
	}

	RecordHash(pos, bestMove, alpha, depth, ply, hashType);
	AdjustSpeed();

	return alpha;
}
////////////////////////////////////////////////////////////////////////////////

EVAL AlphaBetaPV(Position& pos, EVAL alpha, EVAL beta, int depth, int ply)
{
	if (ply > MAX_PLY - 2)
		return DRAW_SCORE;

	g_pv[ply].clear();
	CheckLimits();
	CheckInput();

	if (g_flags & SEARCH_TERMINATED)
		return alpha;

	if (pos.Repetitions() >= 2)
		return DRAW_SCORE;

	//
	//   PROBING HASH
	//

	Move hashMove = 0;
	HashEntry* pEntry = ProbeHash(pos);
	if (pEntry != NULL)
	{
		hashMove = pEntry->GetMove();
		if (pEntry->GetDepth() >= depth)
		{
			EVAL score = pEntry->GetScore(ply);
			U8 type = pEntry->GetType();
			if (type == HASH_EXACT)
				return score;
			if (type == HASH_ALPHA && score <= alpha)
				return alpha;
			if (type == HASH_BETA && score >= beta)
				return beta;
		}
	}

	bool inCheck = pos.InCheck();

	//
	//   QSEARCH
	//

	if (!inCheck && depth <= 0)
		return AlphaBetaQ(pos, alpha, beta, ply, 0);

	//
	//   IID
	//

	if (hashMove == 0 && depth > 2)
	{
		AlphaBetaPV(pos, alpha, beta, depth - 2, ply);
		if (!g_pv[ply].empty())
			hashMove = g_pv[ply][0];
	}

	MoveList& mvlist = g_lists[ply];
	if (inCheck)
		GenMovesInCheck(pos, mvlist);
	else
		GenAllMoves(pos, mvlist);
	UpdateSortScores(mvlist, hashMove, ply);

	int legalMoves = 0;
	Move bestMove = 0;
	U8 hashType = HASH_ALPHA;

	Move lastMove = pos.LastMove();
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = GetNextBest(mvlist, i);
		if (pos.MakeMove(mv))
		{
			++g_nodes;
			++legalMoves;

			int newDepth = depth - 1;

			//
			//   EXTENSIONS
			//

			newDepth += Extensions(inCheck, mv, lastMove, true);

			EVAL e;
			if (legalMoves == 1)
				e = -AlphaBetaPV(pos, -beta, -alpha, newDepth, ply + 1);
			else
			{
				e = -AlphaBetaNonPV(pos, -alpha, newDepth, ply + 1);
				if (e > alpha && e < beta)
					e = -AlphaBetaPV(pos, -beta, -alpha, newDepth, ply + 1);
			}
			pos.UnmakeMove();

			if (g_flags & SEARCH_TERMINATED)
				break;

			if (e > alpha)
			{
				alpha = e;
				bestMove = mv;
				hashType = HASH_EXACT;
				UpdatePV(mv, ply);
				if (!mv.Captured() && !mv.Promotion())
					UpdateHistory(mv, depth);
			}

			if (alpha >= beta)
			{
				hashType = HASH_BETA;
				if (!mv.Captured() && !mv.Promotion())
					g_killers[ply] = mv;
				break;
			}
		}
	}

	if (legalMoves == 0)
	{
		if (inCheck)
			alpha = -CHECKMATE_SCORE + ply;
		else
			alpha = DRAW_SCORE;
	}
	else
	{
		if (pos.Fifty() >= 100)
			alpha = DRAW_SCORE;
	}

	RecordHash(pos, bestMove, alpha, depth, ply, hashType);
	AdjustSpeed();

	return alpha;
}
////////////////////////////////////////////////////////////////////////////////

EVAL AlphaBetaNonPV(Position& pos, EVAL beta, int depth, int ply)
{
	if (ply > MAX_PLY - 2)
		return DRAW_SCORE;

	g_pv[ply].clear();
	CheckLimits();
	CheckInput();

	EVAL alpha = beta - 1;

	if (g_flags & SEARCH_TERMINATED)
		return alpha;

	if (pos.Repetitions() >= 2)
		return DRAW_SCORE;

	//
	//   PROBING HASH
	//

	Move hashMove = 0;
	HashEntry* pEntry = ProbeHash(pos);
	if (pEntry != NULL)
	{
		hashMove = pEntry->GetMove();
		if (pEntry->GetDepth() >= depth)
		{
			EVAL score = pEntry->GetScore(ply);
			U8 type = pEntry->GetType();
			if (type == HASH_EXACT)
				return score;
			if (type == HASH_ALPHA && score <= alpha)
				return alpha;
			if (type == HASH_BETA && score >= beta)
				return beta;
		}
	}

	Move lastMove = pos.LastMove();
	bool isNull = (lastMove == 0);
	bool inCheck = pos.InCheck();

	//
	//   QSEARCH
	//

	if (!inCheck && depth <= 0)
		return AlphaBetaQ(pos, alpha, beta, ply, 0);

	if (!inCheck)
	{
		EVAL staticScore = Evaluate(pos);

		//
		//   FUTILITY
		//

		const EVAL MARGIN_ALPHA[4] = { 0, 50, 350, 550 };
		const EVAL MARGIN_BETA[4]  = { 0, 50, 350, 550 };

		if (USE_FUTILITY && depth >= 1 && depth <= 3)
		{
			if (staticScore < alpha - MARGIN_ALPHA[depth])
				return AlphaBetaQ(pos, alpha, beta, ply, 0);
			if (staticScore > beta + MARGIN_BETA[depth])
				return beta;
		}

		//
		//   NULL MOVE
		//

		if (!isNull && pos.MatIndex(pos.Side()) > 0 && depth > 1)
		{
			const int R = 3 + depth / 6 + std::max(0, (staticScore - beta) / 300);

			pos.MakeNullMove();
			EVAL nullScore = -AlphaBetaNonPV(pos, -alpha, depth - 1 - R, ply + 1);
			pos.UnmakeNullMove();

			if (nullScore >= beta)
				return beta;
		}
	}

	int legalMoves = 0;
	int quietMoves = 0;
	Move bestMove = 0;
	U8 hashType = HASH_ALPHA;

	MoveList& mvlist = g_lists[ply];
	if (inCheck)
		GenMovesInCheck(pos, mvlist);
	else
		GenAllMoves(pos, mvlist);
	UpdateSortScores(mvlist, hashMove, ply);

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = GetNextBest(mvlist, i);
		if (pos.MakeMove(mv))
		{
			++g_nodes;
			++legalMoves;

			int newDepth = depth - 1;

			//
			//   EXTENSIONS
			//

			newDepth += Extensions(inCheck, mv, lastMove, false);

			//
			//   LMR
			//

			int reduction = 0;
			if (USE_LMR &&
				depth >= LMR_MIN_DEPTH &&
				!inCheck &&
				!mv.Captured() &&
				!mv.Promotion() &&
				!pos.InCheck())
			{
				++quietMoves;
				if (quietMoves >= LMR_MIN_QUIET_MOVE)
					reduction = 1 + quietMoves / LMR_QUIET_MOVES_DIVISOR;
			}

			EVAL e = -AlphaBetaNonPV(pos, -alpha, newDepth - reduction, ply + 1);

			if (reduction && e > alpha)
				e = -AlphaBetaNonPV(pos, -alpha, newDepth, ply + 1);

			pos.UnmakeMove();

			if (g_flags & SEARCH_TERMINATED)
				break;

			if (e > alpha)
			{
				alpha = e;
				bestMove = mv;
				hashType = HASH_EXACT;
				UpdatePV(mv, ply);
				if (!mv.Captured() && !mv.Promotion())
					UpdateHistory(mv, depth);
			}

			if (alpha >= beta)
			{
				hashType = HASH_BETA;
				if (!mv.Captured() && !mv.Promotion())
					g_killers[ply] = mv;
				break;
			}
		}
	}

	if (legalMoves == 0)
	{
		if (inCheck)
			alpha = -CHECKMATE_SCORE + ply;
		else
			alpha = DRAW_SCORE;
	}
	else
	{
		if (pos.Fifty() >= 100)
			alpha = DRAW_SCORE;
	}

	RecordHash(pos, bestMove, alpha, depth, ply, hashType);
	AdjustSpeed();

	return alpha;
}
////////////////////////////////////////////////////////////////////////////////

EVAL AlphaBetaQ(Position& pos, EVAL alpha, EVAL beta, int ply, int qply)
{
	if (ply > MAX_PLY - 2)
		return alpha;

	g_pv[ply].clear();
	CheckLimits();
	CheckInput();

	if (g_flags & SEARCH_TERMINATED)
		return alpha;

	bool inCheck = pos.InCheck();
	if (!inCheck)
	{
		EVAL staticScore = Evaluate(pos, alpha, beta);
		if (staticScore > alpha)
			alpha = staticScore;
		if (alpha >= beta)
			return beta;
	}

	MoveList& mvlist = g_lists[ply];
	if (inCheck)
		GenMovesInCheck(pos, mvlist);
	else
	{
		GenCapturesAndPromotions(pos, mvlist);
		if (qply < QCHECKS)
			AddSimpleChecks(pos, mvlist);
	}
	UpdateSortScores(mvlist, 0, ply);

	int legalMoves = 0;
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = GetNextBest(mvlist, i);

		if (!inCheck)
		{
			bool isGoodMove = SORT_VALUE[mv.Captured()] >= SORT_VALUE[mv.Piece()];
			if (!isGoodMove && SEE(pos, mv) < 0)
				continue;
		}

		if (pos.MakeMove(mv))
		{
			++g_nodes;
			++legalMoves;

			EVAL e = -AlphaBetaQ(pos, -beta, -alpha, ply + 1, qply + 1);
			pos.UnmakeMove();

			if (g_flags & SEARCH_TERMINATED)
				break;

			if (e > alpha)
			{
				alpha = e;
				UpdatePV(mv, ply);
			}
			if (alpha >= beta)
				break;
		}
	}

	if (legalMoves == 0)
	{
		if (inCheck)
			alpha = -CHECKMATE_SCORE + ply;
	}

	return alpha;
}
////////////////////////////////////////////////////////////////////////////////

void CheckInput()
{
	if (g_nodes % 8192)
		return;

	if (InputAvailable())
	{
		string s;
		getline(cin, s);
		Log("> " + s);
		ProcessInput(s);
	}
}
////////////////////////////////////////////////////////////////////////////////

void CheckLimits()
{
	if (g_flags & SEARCH_TERMINATED)
		return;

	int dt = GetProcTime() - g_t0;
	if (g_flags & MODE_PLAY)
	{
		if (g_stHard > 0 && dt >= g_stHard)
		{
			g_flags |= TERMINATED_BY_LIMIT;

			stringstream ss;
			ss << "Search stopped by stHard, dt = " << dt;
			Log(ss.str());
		}
		if (g_sn > 0 && g_nodes >= g_sn)
			g_flags |= TERMINATED_BY_LIMIT;
	}
}
////////////////////////////////////////////////////////////////////////////////

void ClearHash()
{
	assert(g_hash != NULL);
	assert(g_hashSize > 0);
	memset(g_hash, 0, g_hashSize * sizeof(HashEntry));
}
////////////////////////////////////////////////////////////////////////////////

void ClearHistory()
{
	memset(g_history, 0, 64 * 14 * sizeof(int));
	g_historyMax = 0;
}
////////////////////////////////////////////////////////////////////////////////

void ClearKillers()
{
	memset(g_killers, 0, MAX_PLY * sizeof(Move));
}
////////////////////////////////////////////////////////////////////////////////

int Extensions(bool inCheck, Move mv, Move lastMove, bool onPV)
{
	if (inCheck)
		return 1;
	else if (mv.Piece() == PW && Row(mv.To()) == 1)
		return 1;
	else if (mv.Piece() == PB && Row(mv.To()) == 6)
		return 1;
	else if (onPV && mv.To() == lastMove.To() && mv.Captured() && lastMove.Captured())
		return 1;
	else
		return 0;
}
////////////////////////////////////////////////////////////////////////////////

Move FirstLegalMove(Position& pos)
{
	MoveList mvlist;
	GenAllMoves(pos, mvlist);
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			pos.UnmakeMove();
			return mv;
		}
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////

Move GetNextBest(MoveList& mvlist, size_t i)
{
	if (i == 0 && mvlist[0].m_score == SORT_HASH)
		return mvlist[0].m_mv;

	for (size_t j = i + 1; j < mvlist.Size(); ++j)
	{
		if (mvlist[j].m_score > mvlist[i].m_score)
			swap(mvlist[i], mvlist[j]);
	}
	return mvlist[i].m_mv;
}
////////////////////////////////////////////////////////////////////////////////

Move GetRandomMove(Position& pos)
{
	EVAL e0 = AlphaBetaRoot(pos, -INFINITY_SCORE, INFINITY_SCORE, 1);

	MoveList mvlist;
	GenAllMoves(pos, mvlist);
	vector<Move> cand_moves;

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			EVAL e = -AlphaBetaQ(pos, -INFINITY_SCORE, INFINITY_SCORE, 0, 0);
			pos.UnmakeMove();

			if (e >= e0 - 100)
				cand_moves.push_back(mv);
		}
	}

	if (cand_moves.empty())
		return 0;
	else
	{
		size_t ind = Rand32() % cand_moves.size();
		return cand_moves[ind];
	}
}
////////////////////////////////////////////////////////////////////////////////

bool HaveSingleMove(Position& pos)
{
	MoveList mvlist;
	GenAllMoves(pos, mvlist);

	int legalMoves = 0;
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			pos.UnmakeMove();
			++legalMoves;
			if (legalMoves > 1)
				break;
		}
	}

	return (legalMoves == 1);
}
////////////////////////////////////////////////////////////////////////////////

bool IsGameOver(Position& pos, string& result, string& comment)
{
	if (pos.Count(PW) == 0 && pos.Count(PB) == 0)
	{
		if (pos.MatIndex(WHITE) < 5 && pos.MatIndex(BLACK) < 5)
		{
			result = "1/2-1/2";
			comment = "{Insufficient material}";
			return true;
		}
	}

	Move mv = FirstLegalMove(pos);
	if (mv == 0)
	{
		if (pos.InCheck())
		{
			if (pos.Side() == WHITE)
			{
				result = "0-1";
				comment = "{Black mates}";
			}
			else
			{
				result = "1-0";
				comment = "{White mates}";
			}
		}
		else
		{
			result = "1/2-1/2";
			comment = "{Stalemate}";
		}
		return true;
	}

	if (pos.Fifty() >= 100)
	{
		result = "1/2-1/2";
		comment = "{Fifty moves rule}";
		return true;
	}

	if (pos.Repetitions() >= 3)
	{
		result = "1/2-1/2";
		comment = "{Threefold repetition}";
		return true;
	}

	return false;
}
////////////////////////////////////////////////////////////////////////////////

NODES Perft(Position& pos, int depth, int ply)
{
	if (depth <= 0)
		return 1;

	NODES total = 0;

	MoveList& mvlist = g_lists[ply];
	GenAllMoves(pos, mvlist);

	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			total += Perft(pos, depth - 1, ply + 1);
			pos.UnmakeMove();
		}
	}

	return total;
}
////////////////////////////////////////////////////////////////////////////////

void PrintPV(const Position& pos, int iter, EVAL score, const vector<Move>& pv, const string& sign)
{
	if (pv.empty())
		return;

	U32 dt = GetProcTime() - g_t0;
	stringstream ss;

	if (!g_uci)
	{
		ss <<
			setw(2) << iter <<
			setw(8) << score <<
			setw(10) << dt / 10 <<
			setw(12) << g_nodes;
		ss << "   ";
		Position tmp = pos;
		int plyCount = tmp.Ply();
		if (tmp.Side() == BLACK)
		{
			if (plyCount == 0)
				++plyCount;
			ss << plyCount / 2 + 1 << ". ... ";
		}
		for (size_t m = 0; m < pv.size(); ++m)
		{
			Move mv = pv[m];
			MoveList mvlist;
			GenAllMoves(tmp, mvlist);
			if (tmp.Side() == WHITE)
				ss << plyCount / 2 + 1 << ". ";
			++plyCount;
			ss << MoveToStrShort(mv, tmp, mvlist);
			if (!tmp.MakeMove(mv))
				break;
			if (tmp.InCheck())
			{
				if (score + m + 1 == CHECKMATE_SCORE)
					ss << "#";
				else if (score - (int)m + 1 == -CHECKMATE_SCORE)
					ss << "#";
				else
					ss << "+";
			}
			if (!sign.empty() && m == 0)
			{
				ss << sign;
				if (sign == "!")
					break;
			}
			if (m < pv.size() - 1)
				ss << " ";
		}
	}
	else
	{
		ss << "info depth " << iter;
		ss << " score cp " << score;
		ss << " time " << dt;
		ss << " nodes " << g_nodes;
		if (!pv.empty())
		{
			ss << " pv";
			for (size_t i = 0; i < pv.size(); ++i)
				ss << " " << MoveToStrLong(pv[i]);
		}
	}

	out(ss.str());
}
////////////////////////////////////////////////////////////////////////////////

void ProcessInput(const string& s)
{
	vector<string> tokens;
	Split(s, tokens);
	if (tokens.empty())
		return;

	string cmd = tokens[0];

	if (g_flags & MODE_ANALYZE)
	{
		if (CanBeMove(cmd))
		{
			Move mv = StrToMove(cmd, g_pos);
			if (mv)
			{
				g_pos.MakeMove(mv);
				g_queue.push_back("analyze");
				g_flags |= TERMINATED_BY_USER;
			}
		}
		else if (Is(cmd, "undo", 1) && (g_flags & MODE_ANALYZE))
		{
			g_pos.UnmakeMove();
			g_queue.push_back("analyze");
			g_flags |= TERMINATED_BY_USER;
		}
	}

	if (g_flags & MODE_PLAY)
	{
		if (cmd == "?")
			g_flags |= TERMINATED_BY_USER;
		else if (Is(cmd, "result", 1))
		{
			g_iterPV.clear();
			g_flags |= TERMINATED_BY_USER;
		}
	}

	if (Is(cmd, "board", 1))
		g_pos.Print();
	else if (Is(cmd, "exit", 1))
		g_flags |= TERMINATED_BY_USER;
	else if (Is(cmd, "quit", 1))
		exit(0);
	else if (Is(cmd, "stop", 1))
		g_flags |= TERMINATED_BY_USER;
}
////////////////////////////////////////////////////////////////////////////////

HashEntry* ProbeHash(const Position& pos)
{
	assert(g_hash != NULL);
	assert(g_hashSize > 0);

	U64 hash = pos.Hash();

	int index = hash % g_hashSize;
	HashEntry* pEntry = g_hash + index;

	if (pEntry->Fits(hash))
		return pEntry;
	else
		return NULL;
}
////////////////////////////////////////////////////////////////////////////////

void RecordHash(const Position& pos, Move mv, EVAL score, int depth, int ply, U8 type)
{
	assert(g_hash != NULL);
	assert(g_hashSize > 0);

	if (g_flags & SEARCH_TERMINATED)
		return;

	U64 hash = pos.Hash();

	int index = hash % g_hashSize;
	HashEntry& entry = g_hash[index];

	entry.SetMove(mv);
	entry.SetScore(score, ply);
	entry.SetDepth(depth);
	entry.SetType(type);
	entry.LockHash(hash);
}
////////////////////////////////////////////////////////////////////////////////

EVAL SEE_Exchange(
	EVAL currScore,
	EVAL target,
	vector<EVAL>& attackers,
	vector<EVAL>& defenders)
{
	if (attackers.empty())
		return currScore;

	if (defenders.empty())
		return currScore + target;

	EVAL newTarget = attackers.back();
	attackers.pop_back();

	EVAL score = -SEE_Exchange(
		-(currScore + target),
		newTarget,
		defenders,
		attackers);

	return (score > currScore)? score : currScore;
}
////////////////////////////////////////////////////////////////////////////////

EVAL SEE(const Position& pos, Move mv)
{
	FLD from = mv.From();
	FLD to = mv.To();
	PIECE piece = mv.Piece();
	PIECE captured = mv.Captured();
	PIECE promotion = mv.Promotion();
	COLOR side = GetColor(piece);
	COLOR opp = side ^ 1;

	U64 x;
	U64 mask = ~BB_SINGLE[from];
	U64 occ = pos.BitsAll() & mask;

	U64 opp_queens = pos.Bits(QUEEN | opp);
	U64 opp_rooks = pos.Bits(ROOK | opp);
	U64 opp_bishops = pos.Bits(BISHOP | opp);

	vector<EVAL> defenders;
	for (x = BB_KING_ATTACKS[to] & pos.Bits(KING | opp); x != 0; x &= (x - 1))
		defenders.push_back(SORT_VALUE[KING]);
	for (x = QueenAttacks(to, occ & ~opp_rooks & ~opp_bishops) & opp_queens; x != 0; x &= (x - 1))
		defenders.push_back(SORT_VALUE[QUEEN]);
	for (x = RookAttacks(to, occ & ~opp_rooks) & opp_rooks; x != 0; x &= (x - 1))
		defenders.push_back(SORT_VALUE[ROOK]);
	for (x = BishopAttacks(to, occ) & opp_bishops; x != 0; x &= (x - 1))
		defenders.push_back(SORT_VALUE[BISHOP]);
	for (x = BB_KNIGHT_ATTACKS[to] & pos.Bits(KNIGHT | opp); x != 0; x &= (x - 1))
		defenders.push_back(SORT_VALUE[KNIGHT]);
	for (x = BB_PAWN_ATTACKS[to][side] & pos.Bits(PAWN | opp); x != 0; x &= (x - 1))
		defenders.push_back(SORT_VALUE[PAWN]);

	EVAL currScore = SORT_VALUE[captured];
	EVAL newTarget = SORT_VALUE[piece];
	if (promotion)
	{
		currScore += SORT_VALUE[promotion] - SORT_VALUE[piece];
		newTarget = SORT_VALUE[promotion];
	}

	if (defenders.empty())
		return currScore;

	U64 side_queens = pos.Bits(QUEEN | side);
	U64 side_rooks = pos.Bits(ROOK | side);
	U64 side_bishops = pos.Bits(BISHOP | side);

	vector<EVAL> attackers;
	for (x = BB_KING_ATTACKS[to] & pos.Bits(KING | side) & mask; x != 0; x &= (x - 1))
		attackers.push_back(SORT_VALUE[KING]);
	for (x = QueenAttacks(to, occ & ~side_rooks & ~side_bishops) & side_queens & mask; x != 0; x &= (x - 1))
		attackers.push_back(SORT_VALUE[QUEEN]);
	for (x = RookAttacks(to, occ & ~side_rooks) & side_rooks & mask; x != 0; x &= (x - 1))
		attackers.push_back(SORT_VALUE[ROOK]);
	for (x = BishopAttacks(to, occ) & side_bishops & mask; x != 0; x &= (x - 1))
		attackers.push_back(SORT_VALUE[BISHOP]);
	for (x = BB_KNIGHT_ATTACKS[to] & pos.Bits(KNIGHT | side) & mask; x != 0; x &= (x - 1))
		attackers.push_back(SORT_VALUE[KNIGHT]);
	for (x = BB_PAWN_ATTACKS[to][opp] & pos.Bits(PAWN | side) & mask; x != 0; x &= (x - 1))
		attackers.push_back(SORT_VALUE[PAWN]);

	EVAL score = -SEE_Exchange(
		-currScore,
		newTarget,
		defenders,
		attackers);

	return score;
}
////////////////////////////////////////////////////////////////////////////////

void SetHashSize(double mb)
{
	if (g_hash)
		delete[] g_hash;

	g_hashSize = int(1024 * 1024 * mb / sizeof(HashEntry));
	g_hash = new HashEntry[g_hashSize];
}
////////////////////////////////////////////////////////////////////////////////

void SetStrength(int level)
{
	if (level < 0)
		level = 0;
	if (level > 100)
		level = 100;

	g_level = level;

	if (level < 100)
	{
		double r = level / 20.;      // 0 - 5
		g_knps = 0.1 * pow(10, r);   // 100 nps - 10000 knps
	}
	else
		g_knps = 0;
}
////////////////////////////////////////////////////////////////////////////////

void StartPerft(Position& pos, int depth)
{
	NODES total = 0;
	U32 t0 = GetProcTime();

	MoveList& mvlist = g_lists[0];
	GenAllMoves(pos, mvlist);

	cout << endl;
	for (size_t i = 0; i < mvlist.Size(); ++i)
	{
		Move mv = mvlist[i].m_mv;
		if (pos.MakeMove(mv))
		{
			NODES delta = Perft(pos, depth - 1, 1);
			total += delta;
			cout << " " << MoveToStrLong(mv) << " - " << delta << endl;
			pos.UnmakeMove();
		}
	}
	U32 t1 = GetProcTime();
	double dt = (t1 - t0) / 1000.;

	cout << endl;
	cout << " Nodes: " << total << endl;
	cout << " Time:  " << dt << endl;
	if (dt > 0) cout << " Knps:  " << total / dt / 1000. << endl;
	cout << endl;
}
////////////////////////////////////////////////////////////////////////////////

Move StartSearch(const Position& startpos, U8 flags)
{
	g_t0 = GetProcTime();
	Log("SEARCH: " + startpos.FEN());

	g_nodes = 0;
	g_flags = flags;
	g_iterPV.clear();
	Position pos = startpos;

	Move bestMove = FirstLegalMove(pos);
	if (bestMove == 0)
		return 0;

	++g_hashAge;
	ClearKillers();
	ClearHistory();

	EVAL alpha = -INFINITY_SCORE;
	EVAL beta = INFINITY_SCORE;
	EVAL ROOT_WINDOW = 100;

	string result, comment;
	if (IsGameOver(pos, result, comment))
	{
		cout << result << " " << comment << endl << endl;
	}
	else
	{
		if (g_console || g_xboard)
		{
			if (!(flags & MODE_SILENT))
				cout << endl;
		}

		bool singleMove = HaveSingleMove(pos);
		for (g_iter = 1; g_iter < MAX_PLY; ++g_iter)
		{
			g_score = AlphaBetaRoot(pos, alpha, beta, g_iter);
			int dt = GetProcTime() - g_t0;

			if (g_flags & SEARCH_TERMINATED)
				break;

			if (g_score > alpha && g_score < beta)
			{
				if (USE_ROOT_WINDOW)
				{
					alpha = g_score - ROOT_WINDOW / 2;
					beta = g_score + ROOT_WINDOW / 2;
				}

				g_iterPV = g_pv[0];

				if (!g_iterPV.empty() && g_iterPV[0] != 0)
					bestMove = g_iterPV[0];

				if (!(flags & MODE_SILENT))
					PrintPV(pos, g_iter, g_score, g_pv[0], "");

				int dt = GetProcTime() - g_t0;
				if (g_stSoft > 0 && dt >= g_stSoft)
				{
					g_flags |= TERMINATED_BY_LIMIT;

					stringstream ss;
					ss << "Search stopped by stSoft, dt = " << dt;
					Log(ss.str());

					break;
				}

				if ((flags & MODE_ANALYZE) == 0)
				{
					if (singleMove)
					{
						g_flags |= TERMINATED_BY_LIMIT;
						break;
					}

					if (g_score + g_iter >= CHECKMATE_SCORE)
					{
						g_flags |= TERMINATED_BY_LIMIT;
						break;
					}
				}
			}
			else if (g_score <= alpha)
			{
				alpha = -INFINITY_SCORE;
				beta = INFINITY_SCORE;

				if (!(flags & MODE_SILENT))
					PrintPV(pos, g_iter, g_score, g_pv[0], "?");
				--g_iter;
			}
			else if (g_score >= beta)
			{
				alpha = -INFINITY_SCORE;
				beta = INFINITY_SCORE;

				g_iterPV = g_pv[0];
				if (!g_iterPV.empty() && g_iterPV[0] != 0)
					bestMove = g_iterPV[0];

				if (!(flags & MODE_SILENT))
					PrintPV(pos, g_iter, g_score, g_pv[0], "!");
				--g_iter;
			}

			if (g_uci && dt > 0)
			{
				cout << "info time " << dt << " nodes " << g_nodes << " nps " << 1000 * g_nodes / dt << endl;
			}

			dt = GetProcTime() - g_t0;
			if (g_stSoft > 0 && dt >= g_stSoft)
			{
				g_flags |= TERMINATED_BY_LIMIT;

				stringstream ss;
				ss << "Search stopped by stSoft, dt = " << dt;
				Log(ss.str());

				break;
			}

			if ((flags & MODE_ANALYZE) == 0)
			{
				if (singleMove)
				{
					g_flags |= TERMINATED_BY_LIMIT;
					break;
				}

				if (g_score + g_iter >= CHECKMATE_SCORE)
				{
					g_flags |= TERMINATED_BY_LIMIT;
					break;
				}
			}

			if (g_sd > 0 && g_iter >= g_sd)
			{
				g_flags |= TERMINATED_BY_LIMIT;
				break;
			}
		}

		if (g_console || g_xboard)
		{
			if (!(flags & MODE_SILENT))
				cout << endl;
		}
	}

	if (g_flags & MODE_ANALYZE)
	{
		while ((g_flags & SEARCH_TERMINATED) == 0)
		{
			string s;
			getline(cin, s);
			ProcessInput(s);
		}
	}

	return bestMove;
}
////////////////////////////////////////////////////////////////////////////////

void UpdateHistory(Move mv, int depth)
{
	int& value = g_history[mv.To()][mv.Piece()];
	value += depth;

	if (value > g_historyMax)
		g_historyMax = value;

	if (value > SORT_HISTORY_MAX)
	{
		g_historyMax /= 2;
		for (FLD f = 0; f < 64; ++f)
			for (PIECE p = 0; p < 14; ++p)
				g_history[f][p] /= 2;
	}
}
////////////////////////////////////////////////////////////////////////////////

void UpdatePV(Move mv, int ply)
{
	g_pv[ply].clear();
	g_pv[ply].push_back(mv);
	g_pv[ply].insert(g_pv[ply].end(), g_pv[ply + 1].begin(), g_pv[ply + 1].end());
}
////////////////////////////////////////////////////////////////////////////////

void UpdateSortScores(MoveList& mvlist, Move hashMove, int ply)
{
	for (size_t j = 0; j < mvlist.Size(); ++j)
	{
		Move mv = mvlist[j].m_mv;
		if (mv == hashMove)
		{
			mvlist[j].m_score = SORT_HASH;
			mvlist.Swap(j, 0);
		}
		else if (mv.Captured() || mv.Promotion())
		{
			EVAL s_piece = SORT_VALUE[mv.Piece()];
			EVAL s_captured = SORT_VALUE[mv.Captured()];
			EVAL s_promotion = SORT_VALUE[mv.Promotion()];

			mvlist[j].m_score = SORT_CAPTURE + 10 * (s_captured + s_promotion) - s_piece;
		}
		else if (mv == g_killers[ply])
			mvlist[j].m_score = SORT_KILLER;
		else
			mvlist[j].m_score = g_history[mv.To()][mv.Piece()];
	}
}
////////////////////////////////////////////////////////////////////////////////
