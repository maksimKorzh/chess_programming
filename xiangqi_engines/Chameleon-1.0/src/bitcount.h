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

#ifndef BITCOUNT_H_INCLUDED
#define BITCOUNT_H_INCLUDED

#include <cassert>

#include "types.h"

enum BitCountType
{
	CNT_64,
	CNT_HW_POPCNT
};

/// popcount() counts the number of non-zero bits in a bitboard

template<BitCountType> inline int popcount(uint64_t);

template<>
inline int popcount<CNT_64>(uint64_t b)
{
	b -= (b >> 1) & 0x5555555555555555ULL;
	b = ((b >> 2) & 0x3333333333333333ULL) + (b & 0x3333333333333333ULL);
	b = ((b >> 4) + b) & 0x0F0F0F0F0F0F0F0FULL;
	return (b * 0x0101010101010101ULL) >> 56;
}

template<>
inline int popcount<CNT_HW_POPCNT>(uint64_t b)
{
#ifndef USE_POPCNT

	assert(false);
	return b != 0; // Avoid 'b not used' warning

#elif defined(_MSC_VER) && defined(__INTEL_COMPILER)

	return _mm_popcnt_u64(b);

#elif defined(_MSC_VER)

	return (int)__popcnt64(b);

#else // Assumed gcc or compatible compiler

	return __builtin_popcountll(b);

#endif
}

inline int popcount(const Bitboard& b)
{
	return popcount<CNT_64>(b.bb[0]) + popcount<CNT_64>(b.bb[1]);
}

#endif // #ifndef BITCOUNT_H_INCLUDED
