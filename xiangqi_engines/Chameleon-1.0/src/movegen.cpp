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

#include <cassert>
#include <iostream>
#include <sstream>
#include <string>
#include "misc.h"

#include "movegen.h"
#include "position.h"
#include "bitcount.h"
#include "init.h"

#define SERIALIZE(b) while(b) *moveList++ = make_move(from, pop_lsb(&b))

static const char* PieceMap[COLOR_NB] = { " PABNCRK", " pabncrk" };

static std::string piece_to_chinese(char p)
{
	switch (p)
	{
	case ' ': return std::string(" ");
	case 'P': return std::string("兵");
	case 'p': return std::string("卒");
	case 'B': return std::string("相");
	case 'b': return std::string("象");
	case 'A': return std::string("士");
	case 'a': return std::string("侍");
	case 'N': return std::string("马");
	case 'n': return std::string("馬");
	case 'C': return std::string("炮");
	case 'c': return std::string("包");
	case 'R': return std::string("车");
	case 'r': return std::string("車");
	case 'K': return std::string("帅");
	case 'k': return std::string("将");
	}

	return std::string();
}

std::string move_to_chinese(const Position& pos, Move m)
{
	Color us = pos.side_to_move();
	Square from = from_sq(m);
	Square to = to_sq(m);
	Piece pc = pos.piece_on(from);
	PieceType pt = type_of(pc);
	char p = PieceMap[us][pt];

	std::string move = "\n";
	move += piece_to_chinese(p);
	move += square_to_string(from);
	move += square_to_string(to);

	return move;
}

static ExtMove* gen_rook_moves(const Position& pos, ExtMove* moveList)
{
	Color us = pos.side_to_move();
	Bitboard target = ~pos.pieces(us);
	const Square* pl = pos.squares<ROOK>(us);

	for (Square from = *pl; from != SQ_NONE; from = *++pl)
	{
		Bitboard att = pos.attacks_from<ROOK>(from)&target;

		SERIALIZE(att);
	}
	return moveList;
}

static ExtMove* gen_knight_moves(const Position& pos, ExtMove* moveList)
{
	Color us = pos.side_to_move();
	Bitboard target = ~pos.pieces(us);
	const Square* pl = pos.squares<KNIGHT>(us);

	for (Square from = *pl; from != SQ_NONE; from = *++pl)
	{
		Bitboard att = pos.attacks_from<KNIGHT>(from)&target;

		SERIALIZE(att);
	}
	return moveList;
}

static ExtMove* gen_cannon_moves(const Position& pos, ExtMove* moveList)
{
	Color us = pos.side_to_move();
	Bitboard target = pos.pieces(~us);
	Bitboard empty = ~pos.pieces();
	const Square* pl = pos.squares<CANNON>(us);

	for (Square from = *pl; from != SQ_NONE; from = *++pl)
	{
		Bitboard att = pos.attacks_from<CANNON>(from)&target;

		SERIALIZE(att);

		Bitboard natt = pos.attacks_from<ROOK>(from)&empty;

		SERIALIZE(natt);
	}
	return moveList;
}

static ExtMove* gen_pawn_moves(const Position& pos, ExtMove* moveList)
{
	Color    us = pos.side_to_move();
	Bitboard target = ~pos.pieces(us);
	Bitboard pawns = pos.pieces(us, PAWN);

	const Square Up = (us == WHITE ? DELTA_N : DELTA_S);
	const Square Right = (us == WHITE ? DELTA_E : DELTA_W);
	const Square Left = (us == WHITE ? DELTA_W : DELTA_E);
	const Bitboard MaskBB = PawnMask[us];

	Bitboard attup = shift_bb(pawns, Up) & MaskBB & target;
	Bitboard attleft = shift_bb(pawns, Left) & MaskBB & target;
	Bitboard attright = shift_bb(pawns, Right) & MaskBB & target;

	while (attup)
	{
		Square to = pop_lsb(&attup);
		Square from = to - (Up);
		*moveList++ = make_move(from, to);
	}

	while (attleft)
	{
		Square to = pop_lsb(&attleft);
		Square from = to - (Left);
		*moveList++ = make_move(from, to);
	}

	while (attright)
	{
		Square to = pop_lsb(&attright);
		Square from = to - (Right);
		*moveList++ = make_move(from, to);
	}
	return moveList;
}

static ExtMove* gen_bishop_moves(const Position& pos, ExtMove* moveList)
{
	Color us = pos.side_to_move();
	Bitboard target = ~pos.pieces(us);
	const Square* pl = pos.squares<BISHOP>(us);

	for (Square from = *pl; from != SQ_NONE; from = *++pl)
	{
		Bitboard att = pos.attacks_from<BISHOP>(from)&target;
		SERIALIZE(att);
	}
	return moveList;
}

static ExtMove* gen_advisor_moves(const Position& pos, ExtMove* moveList)
{
	Color us = pos.side_to_move();
	Bitboard target = ~pos.pieces(us);
	const Square* pl = pos.squares<ADVISOR>(us);

	for (Square from = *pl; from != SQ_NONE; from = *++pl)
	{
		Bitboard att = pos.attacks_from<ADVISOR>(from)&target;
		SERIALIZE(att);
	}
	return moveList;
}

static ExtMove* gen_king_moves(const Position& pos, ExtMove* moveList)
{
	Color us = pos.side_to_move();
	Bitboard target = ~pos.pieces(us);
	const Square* pl = pos.squares<KING>(us);

	for (Square from = *pl; from != SQ_NONE; from = *++pl)
	{
		Bitboard att = pos.attacks_from<KING>(from)&target;
		SERIALIZE(att);
	}
	return moveList;
}

static ExtMove* gen_all_moves(const Position& pos, ExtMove* moveList)
{
	moveList = gen_rook_moves(pos, moveList);
	moveList = gen_knight_moves(pos, moveList);
	moveList = gen_cannon_moves(pos, moveList);
	moveList = gen_pawn_moves(pos, moveList);
	moveList = gen_bishop_moves(pos, moveList);
	moveList = gen_advisor_moves(pos, moveList);
	moveList = gen_king_moves(pos, moveList);

	return moveList;
}

bool move_is_legal(const Position& pos, Move move)
{
	Move m = move;
	assert(is_ok(m));

	Color us = pos.side_to_move();
	Square from = from_sq(m);
	Square to = to_sq(m);

	assert(color_of(pos.moved_piece(m)) == us);
	assert(pos.piece_on(pos.square<KING>(us)) == make_piece(us, KING));

	PieceType pfr = type_of(pos.piece_on(from));
	PieceType pto = type_of(pos.piece_on(to));

	Bitboard pawns = pos.pieces(~us, PAWN);
	Bitboard knights = pos.pieces(~us, KNIGHT);
	Bitboard cannons = pos.pieces(~us, CANNON);
	Bitboard rooks = pos.pieces(~us, ROOK);
	Bitboard occ = pos.pieces();

	occ ^= from;
	if (pto == NO_PIECE_TYPE)
		occ ^= to;
	Square ksq = pos.square<KING>(us);
	if (ksq == from)
		ksq = to;

	if (pto != NO_PIECE_TYPE)
	{
		switch (pto)
		{
		case PAWN:
			pawns ^= to;
			break;
		case KNIGHT:
			knights ^= to;
			break;
		case ROOK:
			rooks ^= to;
			break;
		case CANNON:
			cannons ^= to;
			break;
		}
	}

	if ((RookAttackMask[ksq] & cannons) && (cannon_attacks_bb(ksq, occ) & cannons))
		return false;
	if ((RookAttackMask[ksq] & rooks) && (rook_attacks_bb(ksq, occ)& rooks))
		return false;
	if ((KnightAttackMask[ksq] & knights) && (knight_eye_attacks_bb(ksq, occ) & knights))
		return false;
	if ((PawnAttackTo[~us][ksq] & pawns))
		return false;
	if ((RookAttackMask[ksq] & pos.square<KING>(~us)) && (rook_attacks_bb(ksq, occ) & pos.square<KING>(~us)))
		return false;

	return true;
}

bool move_is_check(const Position& pos, Move move)
{
	Color us = pos.side_to_move();
	Square from = from_sq(move);
	Square to = to_sq(move);
	Square ksq = pos.square<KING>(~us);

	PieceType pfr = type_of(pos.piece_on(from));
	PieceType pto = type_of(pos.piece_on(to));

	Bitboard pawns = pos.pieces(us, PAWN);
	Bitboard knights = pos.pieces(us, KNIGHT);
	Bitboard cannons = pos.pieces(us, CANNON);
	Bitboard rooks = pos.pieces(us, ROOK);
	Bitboard occ = pos.pieces();

	occ ^= from;
	if (pto == NO_PIECE_TYPE)
		occ ^= to;

	switch (pfr)
	{
	case ROOK:
		rooks ^= from;
		rooks ^= to;
		break;
	case CANNON:
		cannons ^= from;
		cannons ^= to;
		break;
	case KNIGHT:
		knights ^= from;
		knights ^= to;
		break;
	case PAWN:
		pawns ^= from;
		pawns ^= to;
		break;
	default:break;
	}

	if ((RookAttackMask[ksq] & cannons) && (cannon_attacks_bb(ksq, occ) & cannons)) return true;
	if ((RookAttackMask[ksq] & rooks) && (rook_attacks_bb(ksq, occ)& rooks)) return true;
	if ((KnightAttackMask[ksq] & knights) && (knight_eye_attacks_bb(ksq, occ) & knights)) return true;
	if ((PawnAttackTo[us][ksq] & pawns)) return true;

	return false;
}

template<Color Us, GenType Type>
ExtMove* generate_pawn_moves(const Position& pos,
	ExtMove* moveList, Bitboard target, Square exclued, const CheckInfo* ci)
{
	Color us = Us;
	Bitboard pawns = pos.pieces(us, PAWN);

	const Square   Up = (us == WHITE ? DELTA_N : DELTA_S);
	const Square   Right = (us == WHITE ? DELTA_E : DELTA_W);
	const Square   Left = (us == WHITE ? DELTA_W : DELTA_E);
	const Bitboard MaskBB = PawnMask[us];

	Bitboard attup = shift_bb(pawns, Up) & MaskBB & target;
	Bitboard attleft = shift_bb(pawns, Left) & MaskBB & target;
	Bitboard attright = shift_bb(pawns, Right) & MaskBB & target;

	while (attup)
	{
		Square to = pop_lsb(&attup);
		Square from = to - (Up);
		if (from == exclued)	continue;
		*moveList++ = make_move(from, to);
	}

	while (attleft)
	{
		Square to = pop_lsb(&attleft);
		Square from = to - (Left);
		if (from == exclued)	continue;
		*moveList++ = make_move(from, to);
	}

	while (attright)
	{
		Square to = pop_lsb(&attright);
		Square from = to - (Right);
		if (from == exclued)	continue;
		*moveList++ = make_move(from, to);
	}
	return 	moveList;
}

template<PieceType Pt, bool Checks>
ExtMove* generate_moves(const Position& pos, ExtMove* moveList,
	Color us, Bitboard target, Square exclued, const CheckInfo* ci)
{
	assert(Pt != KING && Pt != PAWN);

	Bitboard empty = ~pos.pieces();

	const Square* pl = pos.squares<Pt>(us);

	for (Square from = *pl; from != SQ_NONE; from = *++pl)
	{
		if (from == exclued)	continue;

		if (Pt == ROOK)
		{
			Bitboard att = pos.attacks_from<ROOK>(from)&target;
			while (att) *moveList++ = make_move(from, pop_lsb(&att));
		}
		else if (Pt == CANNON)
		{
			Bitboard att = pos.attacks_from<CANNON>(from)&target&pos.pieces(~us);
			while (att) *moveList++ = make_move(from, pop_lsb(&att));

			Bitboard natt = pos.attacks_from<ROOK>(from)&empty&target;
			while (natt) *moveList++ = make_move(from, pop_lsb(&natt));
		}
		else if (Pt == KNIGHT)
		{
			Bitboard att = pos.attacks_from<KNIGHT>(from)&target;
			while (att) *moveList++ = make_move(from, pop_lsb(&att));
		}
		else if (Pt == ADVISOR)
		{
			Bitboard att = pos.attacks_from<ADVISOR>(from)&target;
			while (att) *moveList++ = make_move(from, pop_lsb(&att));
		}
		else if (Pt == BISHOP)
		{
			Bitboard att = pos.attacks_from<BISHOP>(from)&target;
			while (att) *moveList++ = make_move(from, pop_lsb(&att));
		}
	}
	return moveList;
}

template<Color Us, GenType Type>
ExtMove* generate_all(const Position& pos, ExtMove* moveList,
	Bitboard target, Square exclued, const CheckInfo* ci = nullptr)
{
	const bool Checks = Type == QUIET_CHECKS;

	moveList = generate_moves<  ROOK, Checks>(pos, moveList, Us, target, exclued, ci);
	moveList = generate_moves<CANNON, Checks>(pos, moveList, Us, target, exclued, ci);
	moveList = generate_moves<KNIGHT, Checks>(pos, moveList, Us, target, exclued, ci);
	moveList = generate_pawn_moves<Us, Type>(pos, moveList, target, exclued, ci);
	moveList = generate_moves<BISHOP, Checks>(pos, moveList, Us, target, exclued, ci);
	moveList = generate_moves<ADVISOR, Checks>(pos, moveList, Us, target, exclued, ci);

	if (Type != EVASIONS)
	{
		Square ksq = pos.square<KING>(Us);
		Bitboard b = pos.attacks_from<KING>(ksq) & target;
		while (b)
			*moveList++ = make_move(ksq, pop_lsb(&b));
	}
	return moveList;
}

/// generate<CAPTURES> generates all pseudo-legal captures
/// promotions. Returns a pointer to the end of the move list.
///
/// generate<QUIETS> generates all pseudo-legal non-captures and
/// underpromotions. Returns a pointer to the end of the move list.
///
/// generate<NON_EVASIONS> generates all pseudo-legal captures and
/// non-captures. Returns a pointer to the end of the move list.

template<GenType Type>
ExtMove* generate(const Position& pos, ExtMove* moveList)
{
	assert(Type == CAPTURES || Type == QUIETS || Type == NON_EVASIONS);
	assert(!pos.checkers());

	Color us = pos.side_to_move();

	Bitboard target = Type == CAPTURES ? pos.pieces(~us)
		: Type == QUIETS ? ~pos.pieces()
		: Type == NON_EVASIONS ? ~pos.pieces(us) : Bitboard();

	return us == WHITE ? generate_all<WHITE, Type>(pos, moveList, target, SQ_NONE)
		: generate_all<BLACK, Type>(pos, moveList, target, SQ_NONE);
}

// Explicit template instantiations
template ExtMove* generate<CAPTURES>(const Position&, ExtMove*);
template ExtMove* generate<QUIETS>(const Position&, ExtMove*);
template ExtMove* generate<NON_EVASIONS>(const Position&, ExtMove*);

/// generate<QUIET_CHECKS> generates all pseudo-legal non-captures and knight
/// underpromotions that give check. Returns a pointer to the end of the move list.

template<>
ExtMove* generate<QUIET_CHECKS>(const Position& pos, ExtMove* moveList)
{
	assert(!pos.checkers());

	Color us = pos.side_to_move();
	CheckInfo ci(pos);
	Bitboard dc = ci.dcCandidates;
	Bitboard cannonsforbid = pos.discovered_cannon_face_king();
	ExtMove* cur = moveList;

	// Because of the problems with cannon, the problems are complicated
	// There is room for further optimization
	moveList = us == WHITE ? generate_all<WHITE, QUIET_CHECKS>(pos, moveList, ~pos.pieces(), SQ_NONE, &ci)
		: generate_all<BLACK, QUIET_CHECKS>(pos, moveList, ~pos.pieces(), SQ_NONE, &ci);

	while (cur != moveList)
	{
		if (move_is_check(pos, cur->move))
			++cur;
		else
			*cur = (--moveList)->move;
	}
	return moveList;
}


/// generate<EVASIONS> generates all pseudo-legal check evasions when the side
/// to move is in check. Returns a pointer to the end of the move list.

template<>
ExtMove* generate<EVASIONS>(const Position& pos, ExtMove* moveList)
{
	assert(pos.checkers());

	Color us = pos.side_to_move();
	Square ksq = pos.square<KING>(us);
	Bitboard b;
	Bitboard target;
	Bitboard sliderAttacks;
	Bitboard sliders = pos.checkers() & ~pos.pieces(KNIGHT, PAWN);
	Bitboard occ = pos.pieces();

	// Find all the squares attacked by slider checkers. We will remove them from
	// the king evasions in order to skip known illegal moves, which avoids any
	// useless legality checks later on.
	while (sliders)
	{
		Square  checksq = pop_lsb(&sliders);

		switch (type_of(pos.piece_on(checksq)))
		{
		case ROOK:
			sliderAttacks |= RookAttackMask[checksq];
			break;
		case CANNON:
			sliderAttacks |= cannon_attacks_bb(checksq, occ);
			sliderAttacks |= suppercannon_attacks_bb(checksq, occ);
			break;
		default:
			break;
		}
	}

	b = pos.attacks_from<KING>(ksq)&~pos.pieces(us)&~sliderAttacks;
	while (b)
		*moveList++ = make_move(ksq, pop_lsb(&b));

	if (pos.checkers().more_than_one())
	{
		assert(popcount(pos.checkers()) > 1);

		ExtMove* cur = moveList;
		ExtMove* end = us == WHITE ? generate_all<WHITE, EVASIONS>(pos, moveList, ~pos.pieces(us), SQ_NONE)
			: generate_all<BLACK, EVASIONS>(pos, moveList, ~pos.pieces(us), SQ_NONE);

		// That's a problem: Many will, can only file
		// The intersection of multiple files is not correct
		// The union of multiple files, and then according to the set generation method, the set range will not be very large
		// Because many will, the situation is very rare, can produce all legal action law set
		while (cur != end)
		{
			if (!move_is_legal(pos, cur->move))
				*cur = (--end)->move;
			else
				++cur;
		}
		return end;
	}
	else
	{
		assert(popcount(pos.checkers()) == 1);

		Square checksq = lsb(pos.checkers());
		PieceType pt = type_of(pos.piece_on(checksq));
		if (pt == CANNON)
		{
			Bitboard att;
			Bitboard natt;
			Square midsq = lsb(between_bb(checksq, ksq)&occ);
			if (color_of(pos.piece_on(midsq)) == us)
			{
				switch (type_of(pos.piece_on(midsq)))
				{
				case ROOK:
					att = pos.attacks_from<ROOK>(midsq)&(~between_bb(checksq, ksq)) & ~pos.pieces(us);
					while (att)  *moveList++ = make_move(midsq, pop_lsb(&att));
					break;
				case CANNON:
					att = pos.attacks_from<CANNON>(midsq)&pos.pieces(~us);
					while (att)  *moveList++ = make_move(midsq, pop_lsb(&att));
					natt = pos.attacks_from<ROOK>(midsq)&(~pos.pieces())&(~between_bb(checksq, ksq));
					while (natt)  *moveList++ = make_move(midsq, pop_lsb(&natt));
					break;
				case KNIGHT:
					att = pos.attacks_from<KNIGHT>(midsq)& ~pos.pieces(us);
					while (att)  *moveList++ = make_move(midsq, pop_lsb(&att));
					break;
				case PAWN:
					att = pos.attacks_from<PAWN>(midsq, us)& ~pos.pieces(us)&(~between_bb(checksq, ksq));
					while (att)  *moveList++ = make_move(midsq, pop_lsb(&att));
					break;
				case BISHOP:
					att = pos.attacks_from<BISHOP>(midsq)&~pos.pieces(us);
					while (att)  *moveList++ = make_move(midsq, pop_lsb(&att));
					break;
				case ADVISOR:
					att = pos.attacks_from<ADVISOR>(midsq)&~pos.pieces(us);
					while (att)  *moveList++ = make_move(midsq, pop_lsb(&att));
					break;
				case KING:
				default:
					break;
				}
			}

			target = (between_bb(checksq, ksq)&~occ) | checksq;

			if (target)
			{
				moveList = us == WHITE ? generate_all<WHITE, EVASIONS>(pos, moveList, target, midsq)
					: generate_all<BLACK, EVASIONS>(pos, moveList, target, midsq);
			}

			return moveList;
		}
		else
		{
			switch (pt)
			{
			case ROOK:
				target |= between_bb(checksq, ksq) | checksq;
				break;
			case KNIGHT:
				// Knight leg is checksq leg, which is the same as king's leg
				target |= (KnightLeg[checksq] & KnightEye[ksq]) | checksq;
				break;
			case PAWN:
				target |= checksq;
				break;
			case CANNON:
			default:
				break;
			}

			if (target)
			{
				moveList = us == WHITE ? generate_all<WHITE, EVASIONS>(pos, moveList, target, SQ_NONE)
					: generate_all<BLACK, EVASIONS>(pos, moveList, target, SQ_NONE);
			}
			return moveList;
		}
	}
}

/// generate<LEGAL> generates all the legal moves in the given position

template<>
ExtMove* generate<LEGAL>(const Position& pos, ExtMove* moveList)
{
	ExtMove* cur = moveList;
	Square ksq = pos.square<KING>(pos.side_to_move());
	Bitboard pinned = pos.pinned_pieces(pos.side_to_move());
	Bitboard cannonsforbid = pos.discovered_cannon_face_king();

	moveList = pos.checkers() ? generate<EVASIONS    >(pos, moveList)
		: generate<NON_EVASIONS>(pos, moveList);
	while (cur != moveList)
	{
		// There are several cases where filtering can significantly improve efficiency
		if ((cannonsforbid & to_sq(cur->move)) && from_sq(cur->move) != ksq)
			*cur = (--moveList)->move;
		else if ((pinned & from_sq(cur->move)) && !move_is_legal(pos, cur->move))
			*cur = (--moveList)->move;
		else if ((from_sq(cur->move) == ksq) && !move_is_legal(pos, cur->move))
			*cur = (--moveList)->move;
		else
			++cur;
	}
	return moveList;
}
