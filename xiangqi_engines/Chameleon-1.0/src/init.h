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

#ifndef INIT_H_INCLUDED
#define INIT_H_INCLUDED

#include <string>

#include "types.h"

namespace Bitboards
{
	void init();
	const std::string pretty(Bitboard b);
}

#define POW_2(x)   ( (Bitboard(1, 0)<<(x)) )

const Bitboard  A0 = POW_2(SQ_A0), B0 = POW_2(SQ_B0), C0 = POW_2(SQ_C0), D0 = POW_2(SQ_D0), E0 = POW_2(SQ_E0), F0 = POW_2(SQ_F0), G0 = POW_2(SQ_G0), H0 = POW_2(SQ_H0), I0 = POW_2(SQ_I0),
A1 = POW_2(SQ_A1), B1 = POW_2(SQ_B1), C1 = POW_2(SQ_C1), D1 = POW_2(SQ_D1), E1 = POW_2(SQ_E1), F1 = POW_2(SQ_F1), G1 = POW_2(SQ_G1), H1 = POW_2(SQ_H1), I1 = POW_2(SQ_I1),
A2 = POW_2(SQ_A2), B2 = POW_2(SQ_B2), C2 = POW_2(SQ_C2), D2 = POW_2(SQ_D2), E2 = POW_2(SQ_E2), F2 = POW_2(SQ_F2), G2 = POW_2(SQ_G2), H2 = POW_2(SQ_H2), I2 = POW_2(SQ_I2),
A3 = POW_2(SQ_A3), B3 = POW_2(SQ_B3), C3 = POW_2(SQ_C3), D3 = POW_2(SQ_D3), E3 = POW_2(SQ_E3), F3 = POW_2(SQ_F3), G3 = POW_2(SQ_G3), H3 = POW_2(SQ_H3), I3 = POW_2(SQ_I3),
A4 = POW_2(SQ_A4), B4 = POW_2(SQ_B4), C4 = POW_2(SQ_C4), D4 = POW_2(SQ_D4), E4 = POW_2(SQ_E4), F4 = POW_2(SQ_F4), G4 = POW_2(SQ_G4), H4 = POW_2(SQ_H4), I4 = POW_2(SQ_I4),
A5 = POW_2(SQ_A5), B5 = POW_2(SQ_B5), C5 = POW_2(SQ_C5), D5 = POW_2(SQ_D5), E5 = POW_2(SQ_E5), F5 = POW_2(SQ_F5), G5 = POW_2(SQ_G5), H5 = POW_2(SQ_H5), I5 = POW_2(SQ_I5),
A6 = POW_2(SQ_A6), B6 = POW_2(SQ_B6), C6 = POW_2(SQ_C6), D6 = POW_2(SQ_D6), E6 = POW_2(SQ_E6), F6 = POW_2(SQ_F6), G6 = POW_2(SQ_G6), H6 = POW_2(SQ_H6), I6 = POW_2(SQ_I6),
A7 = POW_2(SQ_A7), B7 = POW_2(SQ_B7), C7 = POW_2(SQ_C7), D7 = POW_2(SQ_D7), E7 = POW_2(SQ_E7), F7 = POW_2(SQ_F7), G7 = POW_2(SQ_G7), H7 = POW_2(SQ_H7), I7 = POW_2(SQ_I7),
A8 = POW_2(SQ_A8), B8 = POW_2(SQ_B8), C8 = POW_2(SQ_C8), D8 = POW_2(SQ_D8), E8 = POW_2(SQ_E8), F8 = POW_2(SQ_F8), G8 = POW_2(SQ_G8), H8 = POW_2(SQ_H8), I8 = POW_2(SQ_I8),
A9 = POW_2(SQ_A9), B9 = POW_2(SQ_B9), C9 = POW_2(SQ_C9), D9 = POW_2(SQ_D9), E9 = POW_2(SQ_E9), F9 = POW_2(SQ_F9), G9 = POW_2(SQ_G9), H9 = POW_2(SQ_H9), I9 = POW_2(SQ_I9);

const Bitboard DarkSquares(0x0000000000000000, 0x1FFFFFFFFFFF);

const Bitboard FileABB = A0 | A1 | A2 | A3 | A4 | A5 | A6 | A7 | A8 | A9;
const Bitboard FileBBB = B0 | B1 | B2 | B3 | B4 | B5 | B6 | B7 | B8 | B9;
const Bitboard FileCBB = C0 | C1 | C2 | C3 | C4 | C5 | C6 | C7 | C8 | C9;
const Bitboard FileDBB = D0 | D1 | D2 | D3 | D4 | D5 | D6 | D7 | D8 | D9;
const Bitboard FileEBB = E0 | E1 | E2 | E3 | E4 | E5 | E6 | E7 | E8 | E9;
const Bitboard FileFBB = F0 | F1 | F2 | F3 | F4 | F5 | F6 | F7 | F8 | F9;
const Bitboard FileGBB = G0 | G1 | G2 | G3 | G4 | G5 | G6 | G7 | G8 | G9;
const Bitboard FileHBB = H0 | H1 | H2 | H3 | H4 | H5 | H6 | H7 | H8 | H9;
const Bitboard FileIBB = I0 | I1 | I2 | I3 | I4 | I5 | I6 | I7 | I8 | I9;

const Bitboard Rank0BB = A0 | B0 | C0 | D0 | E0 | F0 | G0 | H0 | I0;
const Bitboard Rank1BB = A1 | B1 | C1 | D1 | E1 | F1 | G1 | H1 | I1;
const Bitboard Rank2BB = A2 | B2 | C2 | D2 | E2 | F2 | G2 | H2 | I2;
const Bitboard Rank3BB = A3 | B3 | C3 | D3 | E3 | F3 | G3 | H3 | I3;
const Bitboard Rank4BB = A4 | B4 | C4 | D4 | E4 | F4 | G4 | H4 | I4;
const Bitboard Rank5BB = A5 | B5 | C5 | D5 | E5 | F5 | G5 | H5 | I5;
const Bitboard Rank6BB = A6 | B6 | C6 | D6 | E6 | F6 | G6 | H6 | I6;
const Bitboard Rank7BB = A7 | B7 | C7 | D7 | E7 | F7 | G7 | H7 | I7;
const Bitboard Rank8BB = A8 | B8 | C8 | D8 | E8 | F8 | G8 | H8 | I8;
const Bitboard Rank9BB = A9 | B9 | C9 | D9 | E9 | F9 | G9 | H9 | I9;

const Bitboard WhiteCityBB = D0 | E0 | F0 | D1 | E1 | F1 | D2 | E2 | F2;
const Bitboard BlackCityBB = D7 | E7 | F7 | D8 | E8 | F8 | D9 | E9 | F9;

const Bitboard WhiteAdvisorCityBB = D0 | F0 | E1 | D2 | F2;
const Bitboard BlackAdvisorCityBB = D7 | F7 | E8 | D9 | F9;
const Bitboard WhiteBishopCityBB = C0 | G0 | A2 | E2 | I2 | C4 | G4;
const Bitboard BlackBishopCityBB = C9 | G9 | A7 | E7 | I7 | C5 | G5;

const Bitboard WhitePawnMaskBB = Rank9BB | Rank8BB | Rank7BB | Rank6BB | Rank5BB | A3 | A4 | C3 | C4 | E3 | E4 | G3 | G4 | I3 | I4;
const Bitboard BlackPawnMaskBB = Rank0BB | Rank1BB | Rank2BB | Rank3BB | Rank4BB | A5 | A6 | C5 | C6 | E5 | E6 | G5 | G6 | I5 | I6;

extern int SquareDistance[SQUARE_NB][SQUARE_NB];

extern Bitboard CityBB[COLOR_NB];
extern Bitboard AdvisorCityBB[COLOR_NB];
extern Bitboard BishopCityBB[COLOR_NB];

extern Bitboard  RookAttackMask[SQUARE_NB];

extern Bitboard  RookMasks[SQUARE_NB];
extern Bitboard  RookMagics[SQUARE_NB];
extern Bitboard* RookAttacks[SQUARE_NB];
extern unsigned  RookShifts[SQUARE_NB];

extern Bitboard  CannonMasks[SQUARE_NB];
extern Bitboard* CannonAttacks[SQUARE_NB];
extern Bitboard  CannonMagics[SQUARE_NB];
extern unsigned  CannonShifts[SQUARE_NB];

extern Bitboard  SuperCannonMasks[SQUARE_NB];
extern Bitboard* SuperCannonAttacks[SQUARE_NB];
extern Bitboard  SuperCannonMagics[SQUARE_NB];
extern unsigned  SuperCannonShifts[SQUARE_NB];

extern Bitboard KnightAttackMask[SQUARE_NB];
extern Bitboard KnightLeg[SQUARE_NB];
extern Bitboard KnightEye[SQUARE_NB];
extern Bitboard KnightAttackTo[SQUARE_NB][16];
extern Bitboard KnightAttackFrom[SQUARE_NB][16];
extern uint64_t KnightLegMagic[SQUARE_NB];
extern uint64_t KnightEyeMagic[SQUARE_NB];

extern Bitboard BishopEye[SQUARE_NB];
extern Bitboard BishopAttack[SQUARE_NB][16];
extern uint64_t BishopEyeMagic[SQUARE_NB];

extern Bitboard AdvisorAttack[SQUARE_NB];

extern Bitboard KingAttack[SQUARE_NB];

extern Bitboard PawnAttackTo[COLOR_NB][SQUARE_NB];
extern Bitboard PawnAttackFrom[COLOR_NB][SQUARE_NB];
extern Bitboard PawnMask[COLOR_NB];
extern Bitboard PassedRiverBB[COLOR_NB];

extern Bitboard SquareBB[SQUARE_NB];
extern Bitboard FileBB[FILE_NB];
extern Bitboard RankBB[RANK_NB];
extern Bitboard AdjacentFilesBB[FILE_NB];
extern Bitboard InFrontBB[COLOR_NB][RANK_NB];
//extern Bitboard StepAttacksBB[PIECE_NB][SQUARE_NB];
extern Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];
extern Bitboard DistanceRingBB[SQUARE_NB][8];
extern Bitboard ForwardBB[COLOR_NB][SQUARE_NB];
extern Bitboard PassedPawnMask[COLOR_NB][SQUARE_NB];
extern Bitboard PawnAttackSpan[COLOR_NB][SQUARE_NB];
// extern Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];

// Overloads of bitwise operators between a Bitboard and a Square for testing
// whether a given bit is set in a bitboard, and for setting and clearing bits.
inline Bitboard operator&(const Bitboard& b, Square s)
{
	return b & SquareBB[s];
}

inline Bitboard& operator&=(Bitboard& b, Square s)
{
	return b &= SquareBB[s];
}

inline Bitboard operator|(const Bitboard& b, Square s)
{
	return b | SquareBB[s];
}

inline Bitboard& operator|=(Bitboard& b, Square s)
{
	return b |= SquareBB[s];
}

inline Bitboard operator^(const Bitboard& b, Square s)
{
	return b ^ SquareBB[s];
}

inline Bitboard& operator^=(Bitboard& b, Square s)
{
	return b ^= SquareBB[s];
}

inline bool more_than_one(const Bitboard& b)
{
	return b.more_than_one();
}

/// rank_bb() and file_bb() return a bitboard representing all the squares on
/// the given file or rank.

inline Bitboard rank_bb(Rank r)
{
	return RankBB[r];
}

inline Bitboard rank_bb(Square s)
{
	return RankBB[rank_of(s)];
}

inline Bitboard file_bb(File f)
{
	return FileBB[f];
}

inline Bitboard file_bb(Square s)
{
	return FileBB[file_of(s)];
}

/// shift_bb() moves a bitboard one step along direction Delta. Mainly for pawns

inline Bitboard shift_bb(Bitboard b, Square Delta)
{
	return Delta == DELTA_N ? b << 9 : Delta == DELTA_S ? b >> 9
		: Delta == DELTA_W ? (b & ~FileABB) >> 1 : Delta == DELTA_E ? (b & ~FileIBB) << 1
		: Bitboard();
}

/// adjacent_files_bb() returns a bitboard representing all the squares on the
/// adjacent files of the given one.

inline Bitboard adjacent_files_bb(File f)
{
	return AdjacentFilesBB[f];
}

/// between_bb() returns a bitboard representing all the squares between the two
/// given ones. For instance, between_bb(SQ_C4, SQ_F7) returns a bitboard with
/// the bits for square d5 and e6 set. If s1 and s2 are not on the same rank, file
/// or diagonal, 0 is returned.

inline Bitboard between_bb(Square s1, Square s2)
{
	return BetweenBB[s1][s2];
}

/// in_front_bb() returns a bitboard representing all the squares on all the ranks
/// in front of the given one, from the point of view of the given color. For
/// instance, in_front_bb(BLACK, RANK_3) will return the squares on ranks 1 and 2.

inline Bitboard in_front_bb(Color c, Rank r)
{
	return InFrontBB[c][r];
}

/// forward_bb() returns a bitboard representing all the squares along the line
/// in front of the given one, from the point of view of the given color:
///        ForwardBB[c][s] = in_front_bb(c, s) & file_bb(s)

inline Bitboard forward_bb(Color c, Square s)
{
	return ForwardBB[c][s];
}

/// pawn_attack_span() returns a bitboard representing all the squares that can be
/// attacked by a pawn of the given color when it moves along its file, starting
/// from the given square:
/// PawnAttackSpan[c][s] = in_front_bb(c, s) & adjacent_files_bb(s);

inline Bitboard pawn_attack_span(Color c, Square s)
{
	return PawnAttackSpan[c][s];
}

/// passed_pawn_mask() returns a bitboard mask which can be used to test if a
/// pawn of the given color and on the given square is a passed pawn:
/// PassedPawnMask[c][s] = pawn_attack_span(c, s) | forward_bb(c, s)

inline Bitboard passed_pawn_mask(Color c, Square s)
{
	return PassedPawnMask[c][s];
}

/// aligned() returns true if the squares s1, s2 and s3 are aligned either on a
/// straight or on a diagonal line.

inline bool aligned(Square s1, Square s2, Square s3)
{
	return LineBB[s1][s2] & s3;
}

inline bool square_same_color(Square s1, Square s2)
{
	return square_color(s1) == square_color(s2);
}

inline bool square_in_city(Square sq)
{
	return CityBB[square_color(sq)] & sq;
}

inline bool bishop_in_city(Square sq)
{
	return BishopCityBB[square_color(sq)] & sq;
}

inline bool advisor_in_city(Square sq)
{
	return AdvisorCityBB[square_color(sq)] & sq;
}

/// distance() functions return the distance between x and y, defined as the
/// number of steps for a king in x to reach y. Works with squares, ranks, files.

template<typename T> inline int distance(T x, T y) { return x < y ? y - x : x - y; }
template<> inline int distance<Square>(Square x, Square y) { return SquareDistance[x][y]; }

template<typename T1, typename T2> inline int distance(T2 x, T2 y);
template<> inline int distance<File>(Square x, Square y) { return distance(file_of(x), file_of(y)); }
template<> inline int distance<Rank>(Square x, Square y) { return distance(rank_of(x), rank_of(y)); }


/// attacks_bb() returns a bitboard representing all the squares attacked by a
/// piece of type Pt (bishop or rook) placed on 's'. The helper magic_index()
/// looks up the index using the 'magic bitboards' approach.

template<PieceType Pt>
inline unsigned slider_magic_index(Square s, const Bitboard& occ)
{
	Bitboard* const Masks = Pt == ROOK ? RookMasks : Pt == CANNON ? CannonMasks : SuperCannonMasks;
	Bitboard* const Magics = Pt == ROOK ? RookMagics : Pt == CANNON ? CannonMagics : SuperCannonMagics;
	unsigned* const Shifts = Pt == ROOK ? RookShifts : Pt == CANNON ? CannonShifts : SuperCannonShifts;

	uint64_t bb[2];
	bb[0] = occ.bb[0] & Masks[s].bb[0];
	bb[1] = occ.bb[1] & Masks[s].bb[1];

	//very important, must << 18,
	return  (((bb[0] * Magics[s].bb[0]) << 18) ^ ((bb[1] * Magics[s].bb[1])) << 18) >> Shifts[s];
}

template<BlockType Bt>
inline unsigned block_magic_index(Square s, const Bitboard& occ)
{
	Bitboard* const Masks = Bt == KNIGHT_LEG ? KnightLeg : Bt == KNIGHT_EYE ? KnightEye : BishopEye;
	uint64_t* const Magics = Bt == KNIGHT_LEG ? KnightLegMagic : Bt == KNIGHT_EYE ? KnightEyeMagic : BishopEyeMagic;

	Bitboard t = Masks[s] & occ;
	return ((t.bb[0] << 18 ^ t.bb[1] << 18)*Magics[s]) >> 60;
}

inline Bitboard rook_attacks_bb(Square s, const Bitboard& occupied)
{
	return RookAttacks[s][slider_magic_index<ROOK>(s, occupied)];
}

inline Bitboard cannon_attacks_bb(Square s, const Bitboard& occupied)
{
	return CannonAttacks[s][slider_magic_index<CANNON>(s, occupied)];
}

inline Bitboard suppercannon_attacks_bb(Square s, const Bitboard& occupied)
{
	return SuperCannonAttacks[s][slider_magic_index<SUPER_CANNON>(s, occupied)];
}

inline Bitboard knight_leg_attacks_bb(Square s, const Bitboard& occupied)
{
	return  KnightAttackFrom[s][block_magic_index<KNIGHT_LEG>(s, occupied)];
}

inline Bitboard knight_eye_attacks_bb(Square s, const Bitboard& occupied)
{
	return  KnightAttackTo[s][block_magic_index<KNIGHT_EYE>(s, occupied)];
}

inline Bitboard bishop_attacks_bb(Square s, const Bitboard& occupied)
{
	return  BishopAttack[s][block_magic_index<BISHOP_EYE>(s, occupied)];
}

inline Bitboard attacks_bb(Piece pc, Square s, const Bitboard& occupied)
{
	switch (type_of(pc))
	{
	case ROOK: return rook_attacks_bb(s, occupied);
	case CANNON: return cannon_attacks_bb(s, occupied);
	case KNIGHT: return knight_leg_attacks_bb(s, occupied);
	case BISHOP: return bishop_attacks_bb(s, occupied);
	case ADVISOR: return AdvisorAttack[s];
	case PAWN:   return PawnAttackFrom[color_of(pc)][s];
	case KING:   return KingAttack[s];
	default:    return Bitboard();
	}
}

/// lsb() and msb() return the least/most significant bit in a non-zero bitboard
#  if defined(_MSC_VER) && !defined(__INTEL_COMPILER)

inline Square lsb(uint64_t b)
{
	unsigned long idx;

	_BitScanForward64(&idx, b);
	return (Square)idx;
}

inline Square lsb(Bitboard b)
{
	unsigned long idx;

	if (b.bb[0]) _BitScanForward64(&idx, b.bb[0]);
	else if (b.bb[1])
	{
		_BitScanForward64(&idx, b.bb[1]);
		idx += 45;
	}
	return (Square)idx;
}

inline Square msb(uint64_t b)
{
	unsigned long idx;

	_BitScanReverse64(&idx, b);
	return (Square)idx;
}

inline Square msb(Bitboard b)
{
	unsigned long idx;

	if (b.bb[1])
	{
		_BitScanReverse64(&idx, b.bb[1]);
		idx += 45;
	}
	else if (b.bb[0])	 _BitScanReverse64(&idx, b.bb[0]);
	return (Square)idx;
}

#  elif defined(__arm__)

Square lsb(Bitboard b);
Square msb(Bitboard b);

#  else // Assumed gcc or compatible compiler

inline Square lsb(uint64_t b) {
	return Square(__builtin_ctzll(b));
}

inline Square msb(uint64_t b) {
	return Square(63 ^ __builtin_clzll(b));
}

inline Square lsb(Bitboard b) { // Assembly code by Heinz van Saanen
	//Bitboard idx;
	unsigned long idx;

	if (b.bb[0]) { idx = __builtin_ctzll(b.bb[0]); }
	else if (b.bb[1])
	{

		idx = __builtin_ctzll(b.bb[1]);
		idx += 45;
	}

	return (Square)idx;
}

inline Square msb(Bitboard b)
{

	//Bitboard idx;
	unsigned long idx;

	if (b.bb[1])
	{

		idx = 127 ^ __builtin_clzll(b.bb[1]);
		idx += 45;
	}
	else if (b.bb[0]) { idx = 127 ^ __builtin_clzll(b.bb[0]); }

	return (Square)idx;
}

#endif

/// pop_lsb() finds and clears the least significant bit in a non-zero bitboard

inline Square pop_lsb(Bitboard* b)
{
	const Square s = lsb(*b);
	b->pop_lsb();
	return s;
}

/// frontmost_sq() and backmost_sq() return the square corresponding to the
/// most/least advanced bit relative to the given color.

inline Square frontmost_sq(Color c, Bitboard b) { return c == WHITE ? msb(b) : lsb(b); }
inline Square  backmost_sq(Color c, Bitboard b) { return c == WHITE ? lsb(b) : msb(b); }

#endif // #ifndef INIT_H_INCLUDED
