//   GreKo chess engine
//   (c) 2002-2020 Vladimir Medvedev <vrm@bk.ru>
//   http://greko.su

#ifndef MOVES_H
#define MOVES_H

#include "position.h"

struct SMove
{
	SMove() : m_mv(0), m_score(0) {}
	SMove(Move mv) : m_mv(mv), m_score(0) {}
	SMove(Move mv, EVAL score) : m_mv(mv), m_score(score) {}

	Move m_mv;
	int  m_score;
};
////////////////////////////////////////////////////////////////////////////////

class MoveList
{
public:

	void Add(Move mv)
	{
		m_data.push_back(SMove(mv));
	}

	void Add(FLD from, FLD to, PIECE piece)
	{
		m_data.push_back(SMove(Move(from, to, piece)));
	}

	void Add(FLD from, FLD to, PIECE piece, PIECE captured)
	{
		m_data.push_back(SMove(Move(from, to, piece, captured)));
	}

	void Add(FLD from, FLD to, PIECE piece, PIECE captured, PIECE promotion)
	{
		m_data.push_back(SMove(Move(from, to, piece, captured, promotion)));
	}

	void   Clear() { m_data.clear(); }
	size_t Size() const { return m_data.size(); }

	const SMove& operator[](size_t i) const { return m_data[i]; }
	SMove& operator[](size_t i) { return m_data[i]; }

	void Swap(size_t i, size_t j) { std::swap(m_data[i], m_data[j]); }

private:
	vector<SMove> m_data;
};
////////////////////////////////////////////////////////////////////////////////

void GenAllMoves(const Position& pos, MoveList& mvlist);
void GenCapturesAndPromotions(const Position& pos, MoveList& mvlist);
void AddSimpleChecks(const Position& pos, MoveList& mvlist);
void GenMovesInCheck(const Position& pos, MoveList& mvlist);

#endif
