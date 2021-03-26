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

#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

// When compiling with provided Makefile (e.g. for Linux and OSX), configuration
// is done automatically. To get started type 'make help'.
//
// When Makefile is not used (e.g. with Microsoft Visual Studio) some switches
// need to be set manually:
//
// -DNDEBUG      | Disable debugging mode. Always use this for release.
//
// -DNO_PREFETCH | Disable use of prefetch asm-instruction. You may need this to
//               | run on some very old machines.
//
// -DUSE_POPCNT  | Add runtime support for use of popcnt asm-instruction. Works
//               | only in 64-bit mode and requires hardware with popcnt support.
//
// -DUSE_PEXT    | Add runtime support for use of pext asm-instruction. Works
//               | only in 64-bit mode and requires hardware with pext support.

#include <cassert>
#include <cctype>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <string>
#include "bitboard.h"

#if defined(_MSC_VER)
// Disable some silly and noisy warning from MSVC compiler
#pragma warning(disable: 4127) // Conditional expression is constant
#pragma warning(disable: 4146) // Unary minus operator applied to unsigned type
#pragma warning(disable: 4800) // Forcing value to bool 'true' or 'false'
#endif

// Predefined macros hell:
//
// __GNUC__           Compiler is gcc, Clang or Intel on Linux
// __INTEL_COMPILER   Compiler is Intel
// _MSC_VER           Compiler is MSVC or Intel on Windows
// _WIN32             Building on Windows (any)
// _WIN64             Building on Windows 64 bit

#if defined(_WIN64) && defined(_MSC_VER) // No Makefile used
#  include <intrin.h> // MSVC popcnt and bsfq instrinsics
#endif

#if defined(USE_POPCNT) && defined(__INTEL_COMPILER) && defined(_MSC_VER)
#  include <nmmintrin.h> // Intel header for _mm_popcnt_u64() intrinsic
#endif

#if !defined(NO_PREFETCH) && (defined(__INTEL_COMPILER) || defined(_MSC_VER))
#  include <xmmintrin.h> // Intel and Microsoft header for _mm_prefetch()
#endif

#if defined(USE_PEXT)
#  include <immintrin.h> // Header for _pext_u64() intrinsic
#  define pext(b, m) _pext_u64(b, m)
#else
#  define pext(b, m) (0)
#endif

const int MAX_PLY = 128;
const int MAX_MOVES = 128;
const int MAX_HASH_MB = 131072;
const int MAX_THREAD_COUNT = 64;
const int MIN_HASH_MB = 16;
const int MIN_THREAD_COUNT = 1;
const int DEFAULT_HASH_MB = 512;
const int DEFAULT_THREAD_COUNT = 1;

// A move needs 16 bits to be stored
//
// 0 - 7: Destination square (from 0 to 255)
// 8 -15: Origin square (from 0 to 255)

// Special cases are MOVE_NONE and MOVE_NULL. We can sneak these in because in
// any normal move destination square is always different from origin square
// while MOVE_NONE and MOVE_NULL have the same origin and destination square.

enum Move
{
	MOVE_NONE,
	MOVE_NULL = 89
};

enum MoveType
{
	NORMAL,
};

enum Color
{
	WHITE, BLACK, NO_COLOR, COLOR_NB = 2
};

enum Phase
{
	PHASE_ENDGAME,
	PHASE_MIDGAME = 128,
	MG = 0, EG = 1, PHASE_NB = 2
};

enum ScaleFactor
{
	SCALE_FACTOR_DRAW = 0,
	SCALE_FACTOR_ONEPAWN = 48,
	SCALE_FACTOR_NORMAL = 64,
	SCALE_FACTOR_MAX = 128,
	SCALE_FACTOR_NONE = 255
};

enum Bound
{
	BOUND_NONE,
	BOUND_UPPER,
	BOUND_LOWER,
	BOUND_EXACT = BOUND_UPPER | BOUND_LOWER
};

enum Value : int
{
	VALUE_ZERO = 0,
	VALUE_DRAW = 0,
	VALUE_KNOWN_WIN = 10000,
	VALUE_REPEAT = 25000,
	VALUE_MATE = 30000,
	VALUE_INFINITE = 30001,
	VALUE_NONE = 30002,
	VALUE_MATE_IN_MAX_PLY = VALUE_MATE - 2 * MAX_PLY,
	VALUE_MATED_IN_MAX_PLY = -VALUE_MATE + 2 * MAX_PLY,

	PawnValueMg = 76, PawnValueEg = 150,
	AdvisorValueMg = 76, AdvisorValueEg = 86,
	BishopValueMg = 96, BishopValueEg = 106,
	KnightValueMg = 102, KnightValueEg = 131,
	CannonValueMg = 131, CannonValueEg = 102,
	RookValueMg = 304, RookValueEg = 323,

	MgLimit = 1298, EgLimit = 333,
};

enum PieceType
{
	NO_PIECE_TYPE, PAWN, ADVISOR, BISHOP, KNIGHT, CANNON, ROOK, KING,
	ALL_PIECES = 0,
	PIECE_TYPE_NB = 8,
	SUPER_CANNON
};

enum Piece
{
	NO_PIECE,
	W_PAWN = 1, W_ADVISOR, W_BISHOP, W_KNIGHT, W_CANNON, W_ROOK, W_KING,
	B_PAWN = 9, B_ADVISOR, B_BISHOP, B_KNIGHT, B_CANNON, B_ROOK, B_KING,
	PIECE_NB = 16
};

enum Depth
{
	ONE_PLY = 1,

	DEPTH_ZERO = 0,
	DEPTH_QS_CHECKS = 0,
	DEPTH_QS_NO_CHECKS = -1,
	DEPTH_QS_RECAPTURES = -5,

	DEPTH_NONE = -6,
	DEPTH_MAX = 8 * MAX_PLY
};

enum BlockType
{
	KNIGHT_LEG = 0,
	KNIGHT_EYE,
	BISHOP_EYE
};

enum RepeatType
{
	REPEATE_NONE = 0,
	REPEATE_TRUE = 1,
	REPEATE_ME_CHECK = 2,
	REPEATE_OPP_CHECK = 4
};

enum Square
{
	SQ_A0, SQ_B0, SQ_C0, SQ_D0, SQ_E0, SQ_F0, SQ_G0, SQ_H0, SQ_I0,
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1, SQ_I1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2, SQ_I2,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3, SQ_I3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4, SQ_I4,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5, SQ_I5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6, SQ_I6,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7, SQ_I7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8, SQ_I8,
	SQ_A9, SQ_B9, SQ_C9, SQ_D9, SQ_E9, SQ_F9, SQ_G9, SQ_H9, SQ_I9,
	SQ_NONE,

	SQUARE_NB = 90,

	DELTA_N = 9,
	DELTA_E = 1,
	DELTA_S = -9,
	DELTA_W = -1,

	DELTA_NN = DELTA_N + DELTA_N,
	DELTA_NE = DELTA_N + DELTA_E,
	DELTA_SE = DELTA_S + DELTA_E,
	DELTA_SS = DELTA_S + DELTA_S,
	DELTA_SW = DELTA_S + DELTA_W,
	DELTA_NW = DELTA_N + DELTA_W,

	DELTA_EE = DELTA_E + DELTA_E,
	DELTA_WW = DELTA_W + DELTA_W,

	DELTA_NNW = DELTA_NN + DELTA_W,
	DELTA_NNE = DELTA_NN + DELTA_E,
	DELTA_EEN = DELTA_EE + DELTA_N,
	DELTA_EES = DELTA_EE + DELTA_S,
	DELTA_SSE = DELTA_SS + DELTA_E,
	DELTA_SSW = DELTA_SS + DELTA_W,
	DELTA_WWN = DELTA_WW + DELTA_N,
	DELTA_WWS = DELTA_WW + DELTA_S,

	DELTA_NNWW = DELTA_NN + DELTA_WW,
	DELTA_NNEE = DELTA_NN + DELTA_EE,
	DELTA_SSEE = DELTA_SS + DELTA_EE,
	DELTA_SSWW = DELTA_SS + DELTA_WW,
};

enum File
{
	FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H, FILE_I, FILE_NB
};

enum Rank
{
	RANK_0, RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8, RANK_9, RANK_NB
};

// Score enum stores a middlegame and an endgame value in a single integer
// (enum). The least significant 16 bits are used to store the endgame value
// and the upper 16 bits are used to store the middlegame value.
enum Score : int { SCORE_ZERO };

const int32_t VerticalFlip[SQUARE_NB] =
{
	SQ_A9, SQ_B9, SQ_C9, SQ_D9, SQ_E9, SQ_F9, SQ_G9, SQ_H9, SQ_I9,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8, SQ_I8,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7, SQ_I7,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6, SQ_I6,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5, SQ_I5,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4, SQ_I4,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3, SQ_I3,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2, SQ_I2,
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1, SQ_I1,
	SQ_A0, SQ_B0, SQ_C0, SQ_D0, SQ_E0, SQ_F0, SQ_G0, SQ_H0, SQ_I0,
};

const int32_t HorizontalFlip[SQUARE_NB] =
{
	SQ_I0, SQ_H0, SQ_G0, SQ_F0, SQ_E0, SQ_D0, SQ_C0, SQ_B0, SQ_A0,
	SQ_I1, SQ_H1, SQ_G1, SQ_F1, SQ_E1, SQ_D1, SQ_C1, SQ_B1, SQ_A1,
	SQ_I2, SQ_H2, SQ_G2, SQ_F2, SQ_E2, SQ_D2, SQ_C2, SQ_B2, SQ_A2,
	SQ_I3, SQ_H3, SQ_G3, SQ_F3, SQ_E3, SQ_D3, SQ_C3, SQ_B3, SQ_A3,
	SQ_I4, SQ_H4, SQ_G4, SQ_F4, SQ_E4, SQ_D4, SQ_C4, SQ_B4, SQ_A4,
	SQ_I5, SQ_H5, SQ_G5, SQ_F5, SQ_E5, SQ_D5, SQ_C5, SQ_B5, SQ_A5,
	SQ_I6, SQ_H6, SQ_G6, SQ_F6, SQ_E6, SQ_D6, SQ_C6, SQ_B6, SQ_A6,
	SQ_I7, SQ_H7, SQ_G7, SQ_F7, SQ_E7, SQ_D7, SQ_C7, SQ_B7, SQ_A7,
	SQ_I8, SQ_H8, SQ_G8, SQ_F8, SQ_E8, SQ_D8, SQ_C8, SQ_B8, SQ_A8,
	SQ_I9, SQ_H9, SQ_G9, SQ_F9, SQ_E9, SQ_D9, SQ_C9, SQ_B9, SQ_A9,
};

//SQUARE
const int32_t SquareMake[RANK_NB][FILE_NB] =
{
	SQ_A0, SQ_B0, SQ_C0, SQ_D0, SQ_E0, SQ_F0, SQ_G0, SQ_H0, SQ_I0,
	SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1, SQ_I1,
	SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2, SQ_I2,
	SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3, SQ_I3,
	SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4, SQ_I4,
	SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5, SQ_I5,
	SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6, SQ_I6,
	SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7, SQ_I7,
	SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8, SQ_I8,
	SQ_A9, SQ_B9, SQ_C9, SQ_D9, SQ_E9, SQ_F9, SQ_G9, SQ_H9, SQ_I9,
};

//SQUARE_FILE
const int32_t SquareFile[SQUARE_NB] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
	0, 1, 2, 3, 4, 5, 6, 7, 8,
};

//SQUARE_RANK
const int32_t SquareRank[SQUARE_NB] =
{
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2,
	3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8,
	9, 9, 9, 9, 9, 9, 9, 9, 9,
};

inline Score make_score(int mg, int eg)
{
	return Score((mg << 16) + eg);
}

// Extracting the signed lower and upper 16 bits is not so trivial because
// according to the standard a simple cast to short is implementation defined
// and so is a right shift of a signed integer.
inline Value mg_value(Score s)
{
	union { uint16_t u; int16_t s; } mg = { uint16_t(unsigned(s + 0x8000) >> 16) };
	return Value(mg.s);
}

inline Value eg_value(Score s)
{
	union { uint16_t u; int16_t s; } eg = { uint16_t(unsigned(s)) };
	return Value(eg.s);
}

#define ENABLE_BASE_OPERATORS_ON(T)                             \
inline T operator+(T d1, T d2) { return T(int(d1) + int(d2)); } \
inline T operator-(T d1, T d2) { return T(int(d1) - int(d2)); } \
inline T operator*(int i, T d) { return T(i * int(d)); }        \
inline T operator*(T d, int i) { return T(int(d) * i); }        \
inline T operator-(T d) { return T(-int(d)); }                  \
inline T& operator+=(T& d1, T d2) { return d1 = d1 + d2; }      \
inline T& operator-=(T& d1, T d2) { return d1 = d1 - d2; }      \
inline T& operator*=(T& d, int i) { return d = T(int(d) * i); }

#define ENABLE_FULL_OPERATORS_ON(T)                             \
ENABLE_BASE_OPERATORS_ON(T)                                     \
inline T& operator++(T& d) { return d = T(int(d) + 1); }        \
inline T& operator--(T& d) { return d = T(int(d) - 1); }        \
inline T operator/(T d, int i) { return T(int(d) / i); }        \
inline int operator/(T d1, T d2) { return int(d1) / int(d2); }  \
inline T& operator/=(T& d, int i) { return d = T(int(d) / i); }

ENABLE_FULL_OPERATORS_ON(Value)
ENABLE_FULL_OPERATORS_ON(PieceType)
ENABLE_FULL_OPERATORS_ON(Piece)
ENABLE_FULL_OPERATORS_ON(Color)
ENABLE_FULL_OPERATORS_ON(Depth)
ENABLE_FULL_OPERATORS_ON(Square)
ENABLE_FULL_OPERATORS_ON(File)
ENABLE_FULL_OPERATORS_ON(Rank)
ENABLE_BASE_OPERATORS_ON(Score)

#undef ENABLE_FULL_OPERATORS_ON
#undef ENABLE_BASE_OPERATORS_ON

// Additional operators to add integers to a Value
inline Value operator+(Value v, int i) { return Value(int(v) + i); }
inline Value operator-(Value v, int i) { return Value(int(v) - i); }
inline Value& operator+=(Value& v, int i) { return v = v + i; }
inline Value& operator-=(Value& v, int i) { return v = v - i; }

// Division of a Score must be handled separately for each term
inline Score operator/(Score s, int i)
{
	return make_score(mg_value(s) / i, eg_value(s) / i);
}

inline Color operator~(Color c)
{
	return Color(c ^ BLACK);
}

inline Square operator~(Square s)
{
	return Square(VerticalFlip[s]); // Vertical flip SQ_A0 -> SQ_A9 
}

inline Value mate_in(int ply)
{
	return VALUE_MATE - ply;
}

inline Value mated_in(int ply)
{
	return -VALUE_MATE + ply;
}

inline Value repeat_value(int ply, int reptype)
{
	int v;
	v = (reptype & REPEATE_ME_CHECK) ? (-VALUE_REPEAT + ply) : 0 + (reptype & REPEATE_OPP_CHECK) ? (VALUE_REPEAT - ply) : 0;
	return Value(v == 0 ? VALUE_DRAW : v);
}

inline Square make_square(File f, Rank r)
{
	return Square(SquareMake[r][f]);
}

inline Piece make_piece(Color c, PieceType pt)
{
	return Piece((c << 3) | pt);
}

inline PieceType type_of(Piece pc)
{
	return PieceType(pc & 7);
}

inline Color color_of(Piece pc)
{
	assert(pc != NO_PIECE);
	return Color(pc >> 3);
}

inline bool is_ok(Square s)
{
	return s >= SQ_A0 && s <= SQ_I9;
}

inline bool is_ok(int s)
{
	return s >= SQ_A0 && s <= SQ_I9;
}

inline File file_of(Square s)
{
	return File(SquareFile[s]);
}

inline Rank rank_of(Square s)
{
	return Rank(SquareRank[s]);
}

inline Square mirror(Square s)
{
	return Square(HorizontalFlip[s]);
}

inline Square relative_square(Color c, Square s)
{
	return c ? (Square(VerticalFlip[s])) : (s);
}

inline Rank relative_rank(Color c, Rank r)
{
	return c ? (Rank(9 - r)) : (r);
}

inline Rank relative_rank(Color c, Square s)
{
	return relative_rank(c, rank_of(s));
}

inline Color square_color(Square s)
{
	return s > SQ_I4 ? BLACK : WHITE;
}

inline bool opposite_colors(Square s1, Square s2)
{
	return  square_color(s1) != square_color(s2);
}

inline Square pawn_push(Color c)
{
	return c == WHITE ? DELTA_N : DELTA_S;
}

inline Square from_sq(Move m)
{
	return Square((m >> 8));
}

inline Square to_sq(Move m)
{
	return Square(m & 0xFF);
}

inline MoveType type_of(Move m)
{
	return NORMAL;
}

inline Move make_move(Square from, Square to)
{
	return Move(to | (from << 8));
}

template<MoveType T>
inline Move make(Square from, Square to, PieceType pt = KNIGHT)
{
	return Move(to | (from << 8));
}

inline bool is_ok(Move m)
{
	return from_sq(m) != to_sq(m); // Catch MOVE_NULL and MOVE_NONE
}
inline char file_to_char(File f, bool tolower = true)
{
	return char(f - FILE_A + (tolower ? 'a' : 'A'));
}

inline char rank_to_char(Rank r)
{
	return char(r - RANK_0 + '0');
}

inline const std::string square_to_string(Square s)
{
	char ch[] = { file_to_char(file_of(s)), rank_to_char(rank_of(s)), 0 };
	return ch;
}

extern Value PieceValue[PHASE_NB][PIECE_NB];

#endif // #ifndef TYPES_H_INCLUDED
