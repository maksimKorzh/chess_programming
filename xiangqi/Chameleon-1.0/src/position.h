/*
  Chameleon, a UCI chinese chess playing engine derived from Stockfish
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2017 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2017 Wilbert Lee

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <cstddef>  // For offsetof()
#include <string>

#include "bitboard.h"
#include "types.h"
#include "init.h"

class Position;
class Thread;

namespace Zobrist
{
	extern uint64_t side;
	extern uint64_t exclusion;
	extern uint64_t psq[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];
}

namespace PSQT
{
	extern Score psq[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];

	void init();
}

// CheckInfo struct is initialized at constructor time and keeps info used to
// detect if a move gives check.

struct CheckInfo
{
	explicit CheckInfo(const Position&);

	// You can take the mate of king
	Bitboard dcCandidates;

	// As the location of the gun rack, we cannon with the other side king face to face
	Bitboard dcCannonCandidates;

	// We are pinned down
	Bitboard pinned;

	// We can take the next direct general's position
	Bitboard checkSquares[PIECE_TYPE_NB];
	Square   ksq;
};

// StateInfo struct stores information needed to restore a Position object to
// its previous state when we retract a move. Whenever a move is made on the
// board (by calling Position::do_move), a StateInfo object must be passed.
struct StateInfo
{
	// Copied when making a move
	uint64_t    pawnKey;
	uint64_t    materialKey;
	Value  nonPawnMaterial[COLOR_NB];

	int    rule50;
	int    pliesFromNull;
	Score  psq;

	// Not copied when making a move
	uint64_t        key;
	Bitboard   checkersBB;
	PieceType  capturedType;
	StateInfo* previous;

};

// Position class stores information regarding the board representation as
// pieces, side to move, hash keys, castling info, etc. Important methods are
// do_move() and undo_move(), used by the search to update node info when
// traversing the search tree.
class Position
{
public:
	static void init();

	// To define the global object RootPos
	Position() = default;
	Position(const Position&) = delete;
	Position(const Position& pos, Thread* th) { *this = pos; thisThread = th; }
	Position(const std::string& f, bool c960, Thread* th) { set(f, c960, th); }

	// To assign RootPos from UCI
	Position& operator=(const Position&);

	// FEN string input/output
	void set(const std::string& fenStr, bool isChess960, Thread* th);
	const std::string fen() const;

	// Position representation
	Bitboard pieces() const;
	Bitboard pieces(PieceType pt) const;
	Bitboard pieces(PieceType pt1, PieceType pt2) const;
	Bitboard pieces(Color c) const;
	Bitboard pieces(Color c, PieceType pt) const;
	Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
	Piece piece_on(Square s) const;

	bool empty(Square s) const;
	template<PieceType Pt> int count(Color c) const;
	template<PieceType Pt> const Square* squares(Color c) const;
	template<PieceType Pt> Square square(Color c) const;

	// Checking
	Bitboard checkers() const;
	Bitboard discovered_check_candidates() const;
	Bitboard discovered_cannon_check_candidates() const;
	Bitboard discovered_cannon_face_king() const;
	Bitboard pinned_pieces(Color c) const;
	Bitboard get_checkers(Color checker, Square ksq)const;
	bool in_check(Color c);

	// Attacks to/from a given square
	Bitboard attackers_to(Square s) const;
	Bitboard attackers_to(Square s, Bitboard occupied) const;
	Bitboard attacks_from(Piece pc, Square s) const;
	template<PieceType> Bitboard attacks_from(Square s) const;
	template<PieceType> Bitboard attacks_from(Square s, Color c) const;

	// Properties of moves
	bool legal(Move m, Bitboard pinned) const;
	bool pseudo_legal(const Move m) const;
	bool capture(Move m) const;

	bool gives_check(Move m, const CheckInfo& ci) const;
	bool advanced_pawn_push(Move m) const;
	Piece moved_piece(Move m) const;
	PieceType captured_piece_type() const;

	// Piece specific
	//bool pawn_passed(Color c, Square s) const;
	bool opposite_bishops() const;

	// Doing and undoing moves
	void do_move(Move m, StateInfo& st, bool givesCheck);
	void undo_move(Move m);
	void do_null_move(StateInfo& st);
	void undo_null_move();

	// Static exchange evaluation
	Value see(Move m) const;
	Value see_sign(Move m) const;

	// Accessing hash keys
	uint64_t key() const;
	uint64_t key_after(Move m) const;
	uint64_t exclusion_key() const;
	uint64_t material_key() const;
	uint64_t pawn_key() const;

	// Other properties of the position
	Color side_to_move() const;
	Phase game_phase() const;
	int game_ply() const;

	Thread* this_thread() const;
	uint64_t nodes_searched() const;
	void set_nodes_searched(uint64_t n);
	bool is_draw() const;
	int  is_repeat()const;
	int rule50_count() const;
	Score psq_score() const;
	Value non_pawn_material(Color c) const;

	// Position consistency check, for debugging
	bool pos_is_ok(int* failedStep = nullptr) const;
	void flip();

private:
	// Initialization helpers (used while setting up a position)
	void clear();
	void set_state(StateInfo* si) const;

	// Other helpers
	Bitboard check_blockers(Color c, Color kingColor) const;
	void put_piece(Color c, PieceType pt, Square s);
	void remove_piece(Color c, PieceType pt, Square s);
	void move_piece(Color c, PieceType pt, Square from, Square to);

	// Data members
	Piece board[SQUARE_NB];
	Bitboard byTypeBB[PIECE_TYPE_NB];
	Bitboard byColorBB[COLOR_NB];
	int pieceCount[COLOR_NB][PIECE_TYPE_NB];
	Square pieceList[COLOR_NB][PIECE_TYPE_NB][16];
	int index[SQUARE_NB];

	StateInfo startState;
	uint64_t nodes;
	int gamePly;
	Color sideToMove;
	Thread* thisThread;
	StateInfo* st;
};

extern std::ostream& operator<<(std::ostream& os, const Position& pos);

inline Color Position::side_to_move() const
{
	return sideToMove;
}

inline bool Position::empty(Square s) const
{
	return board[s] == NO_PIECE;
}

inline Piece Position::piece_on(Square s) const
{
	return board[s];
}

inline Piece Position::moved_piece(Move m) const
{
	return board[from_sq(m)];
}

inline Bitboard Position::pieces() const
{
	return byTypeBB[ALL_PIECES];
}

inline Bitboard Position::pieces(PieceType pt) const
{
	return byTypeBB[pt];
}

inline Bitboard Position::pieces(PieceType pt1, PieceType pt2) const
{
	return byTypeBB[pt1] | byTypeBB[pt2];
}

inline Bitboard Position::pieces(Color c) const
{
	return byColorBB[c];
}

inline Bitboard Position::pieces(Color c, PieceType pt) const
{
	return byColorBB[c] & byTypeBB[pt];
}

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2) const
{
	return byColorBB[c] & (byTypeBB[pt1] | byTypeBB[pt2]);
}

template<PieceType Pt> inline int Position::count(Color c) const
{
	return pieceCount[c][Pt];
}

template<PieceType Pt> inline const Square* Position::squares(Color c) const
{
	return pieceList[c][Pt];
}

template<PieceType Pt> inline Square Position::square(Color c) const
{
	assert(pieceCount[c][Pt] == 1);
	return pieceList[c][Pt][0];
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s) const
{
	if (Pt == ROOK)	 return rook_attacks_bb(s, byTypeBB[ALL_PIECES]);
	else if (Pt == CANNON)	return cannon_attacks_bb(s, byTypeBB[ALL_PIECES]);
	else if (Pt == KNIGHT)  return knight_leg_attacks_bb(s, byTypeBB[ALL_PIECES]);
	else if (Pt == BISHOP)  return bishop_attacks_bb(s, byTypeBB[ALL_PIECES]);
	else if (Pt == ADVISOR)	return AdvisorAttack[s];
	else if (Pt == KING) return  KingAttack[s];
	assert(false);
	return Bitboard();
}

template<>
inline Bitboard Position::attacks_from<PAWN>(Square s, Color c) const
{
	return PawnAttackFrom[c][s];
}

inline Bitboard Position::attacks_from(Piece pc, Square s) const
{
	return attacks_bb(pc, s, byTypeBB[ALL_PIECES]);
}

inline Bitboard Position::attackers_to(Square s) const
{
	return attackers_to(s, byTypeBB[ALL_PIECES]);
}

inline Bitboard Position::checkers() const
{
	return st->checkersBB;
}

inline Bitboard Position::pinned_pieces(Color c) const
{
	return check_blockers(c, c);
}

/*inline bool Position::pawn_passed(Color c, Square s) const
{
  return !(moved_piece(~c, PAWN) & passed_pawn_mask(c, s));
}*/

inline bool Position::advanced_pawn_push(Move m) const
{
	return   type_of(moved_piece(m)) == PAWN
		&& relative_rank(sideToMove, from_sq(m)) > RANK_4;
}

inline uint64_t Position::key() const
{
	return st->key;
}

inline uint64_t Position::pawn_key() const
{
	return st->pawnKey;
}

inline uint64_t Position::exclusion_key() const
{
	return st->key ^ Zobrist::exclusion;
}

inline uint64_t Position::material_key() const
{
	return st->materialKey;
}

inline Score Position::psq_score() const
{
	return st->psq;
}

inline Value Position::non_pawn_material(Color c) const
{
	return st->nonPawnMaterial[c];
}

inline int Position::game_ply() const
{
	return gamePly;
}

inline int Position::rule50_count() const
{
	return st->rule50;
}

inline uint64_t Position::nodes_searched() const
{
	return nodes;
}

inline void Position::set_nodes_searched(uint64_t n)
{
	nodes = n;
}

inline bool Position::opposite_bishops() const
{
	return   pieceCount[WHITE][BISHOP] == 1
		&& pieceCount[BLACK][BISHOP] == 1
		&& opposite_colors(square<BISHOP>(WHITE), square<BISHOP>(BLACK));
}

inline bool Position::capture(Move m) const
{
	assert(is_ok(m));
	return !empty(to_sq(m));
}

inline PieceType Position::captured_piece_type() const
{
	return st->capturedType;
}

inline Thread* Position::this_thread() const
{
	return thisThread;
}

inline void Position::put_piece(Color c, PieceType pt, Square s)
{
	board[s] = make_piece(c, pt);
	byTypeBB[ALL_PIECES] |= s;
	byTypeBB[pt] |= s;
	byColorBB[c] |= s;
	index[s] = pieceCount[c][pt]++;
	pieceList[c][pt][index[s]] = s;
	pieceCount[c][ALL_PIECES]++;
}

inline void Position::remove_piece(Color c, PieceType pt, Square s)
{
	// WARNING: This is not a reversible operation. If we remove a piece in
	// do_move() and then replace it in undo_move() we will put it at the end of
	// the list and not in its original place, it means index[] and pieceList[]
	// are not guaranteed to be invariant to a do_move() + undo_move() sequence.
	byTypeBB[ALL_PIECES] ^= s;
	byTypeBB[pt] ^= s;
	byColorBB[c] ^= s;
	Square lastSquare = pieceList[c][pt][--pieceCount[c][pt]];
	index[lastSquare] = index[s];
	pieceList[c][pt][index[lastSquare]] = lastSquare;
	pieceList[c][pt][pieceCount[c][pt]] = SQ_NONE;
	pieceCount[c][ALL_PIECES]--;
}

inline void Position::move_piece(Color c, PieceType pt, Square from, Square to)
{
	// index[from] is not updated and becomes stale. This works as long as index[]
	// is accessed just by known occupied squares.
	Bitboard from_to_bb = SquareBB[from] ^ SquareBB[to];
	byTypeBB[ALL_PIECES] ^= from_to_bb;
	byTypeBB[pt] ^= from_to_bb;
	byColorBB[c] ^= from_to_bb;
	board[from] = NO_PIECE;
	board[to] = make_piece(c, pt);
	index[to] = index[from];
	pieceList[c][pt][index[to]] = to;
}

#endif // #ifndef POSITION_H_INCLUDED
