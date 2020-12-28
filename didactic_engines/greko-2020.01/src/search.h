//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef SEARCH_H
#define SEARCH_H

#include "position.h"

extern int g_stHard;
extern int g_stSoft;

extern int g_inc;
extern int g_restTime;
extern int g_restMoves;
extern int g_movesPerSession;

extern int g_sd;
extern NODES g_sn;

const int MAX_PLY = 64;

const U8 TERMINATED_BY_USER  = 0x01;
const U8 TERMINATED_BY_LIMIT = 0x02;
const U8 SEARCH_TERMINATED = TERMINATED_BY_USER | TERMINATED_BY_LIMIT;

const U8 MODE_PLAY    = 0x04;
const U8 MODE_ANALYZE = 0x08;
const U8 MODE_SILENT  = 0x10;

const U8 HASH_ALPHA = 0x01;
const U8 HASH_BETA  = 0x02;
const U8 HASH_EXACT = 0x03;

void ClearHash();
void ClearHistory();
void ClearKillers();
Move GetRandomMove(Position& pos);
bool IsGameOver(Position& pos, string& result, string& comment);
EVAL SEE(const Position& pos, Move mv);
void SetHashSize(double mb);
void SetStrength(int level);
void StartPerft(Position& pos, int depth);
Move StartSearch(const Position& startpos, U8 flags);

class HashEntry
{
public:
	HashEntry()
	{
		memset(&m_data, 0, sizeof(m_data));
	}

	Move GetMove() const { return m_data.mv; }
	I8 GetDepth() const { return m_data.depth; }
	EVAL GetScore(int ply)
	{
		EVAL score = m_data.score;
		if (score > CHECKMATE_SCORE - 50 && score <= CHECKMATE_SCORE)
			score -= ply;
		if (score < -CHECKMATE_SCORE + 50 && score >= -CHECKMATE_SCORE)
			score += ply;
		return score;
	}
	U8 GetType() const { return m_data.type; }

	bool Fits(U64 hash) const { return m_data.hashLock == (U32)(hash >> 32); }
	void LockHash(U64 hash) { m_data.hashLock = (U32)(hash >> 32); }

	void SetMove(Move mv) { m_data.mv = mv; }
	void SetDepth(I8 depth) { m_data.depth = depth; }
	void SetScore(EVAL score, int ply)
	{
		if (score > CHECKMATE_SCORE - 50 && score <= CHECKMATE_SCORE)
			score += ply;
		if (score < -CHECKMATE_SCORE + 50 && score >= -CHECKMATE_SCORE)
			score -= ply;
		m_data.score = score;
	}
	void SetType(U8 type) { m_data.type = type; }

private:

	struct
	{
		unsigned hashLock : 32;   // 4
		unsigned mv       : 32;   // 4
		signed   score    : 32;   // 4
		signed   depth    :  8;   // 1
		unsigned type     :  8;   // 1
	}
	m_data;
};
////////////////////////////////////////////////////////////////////////////////

#endif
