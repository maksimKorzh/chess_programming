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

#include <algorithm>
#include <cassert>

#include "bitboard.h"
#include "bitcount.h"
#include "pawns.h"
#include "position.h"
#include "thread.h"

#define V Value
#define S(mg, eg) make_score(mg, eg)

// Isolated pawn penalty by opposed flag and file
const Score Isolated[2][FILE_NB] =
{
	{ S(37, 45), S(54, 52), S(60, 52), S(60, 52),
		S(60, 52), S(60, 52), S(54, 52), S(37, 45) },
	{ S(25, 30), S(36, 35), S(40, 35), S(40, 35),
		S(40, 35), S(40, 35), S(36, 35), S(25, 30) }
};

// Backward pawn penalty by opposed flag
const Score Backward[2] = { S(67, 42), S(49, 24) };

// Unsupported pawn penalty, for pawns which are neither isolated or backward
const Score Unsupported = S(20, 10);

// Connected pawn bonus by opposed, phalanx, twice supported and rank
Score Connected[2][2][2][RANK_NB];

// Doubled pawn penalty by file
const Score Doubled[FILE_NB] =
{
	  S(13, 43), S(20, 48), S(23, 48), S(23, 48),
	  S(23, 48), S(23, 48), S(20, 48), S(13, 43)
};

// Lever bonus by rank
const Score Lever[RANK_NB] =
{
	  S(0,  0), S(0,  0), S(0, 0), S(0, 0),
	  S(20, 20), S(40, 40), S(0, 0), S(0, 0)
};

// Center bind bonus, when two pawns controls the same central square
const Score CenterBind = S(16, 0);

// Weakness of our pawn shelter in front of the king by [distance from edge][rank]
const Value ShelterWeakness[][RANK_NB] =
{
	  { V(97), V(21), V(26), V(51), V(87), V(89), V(99) },
	  { V(120), V(0), V(28), V(76), V(88), V(103), V(104) },
	  { V(101), V(7), V(54), V(78), V(77), V(92), V(101) },
	  { V(80), V(11), V(44), V(68), V(87), V(90), V(119) }
};

// Danger of enemy pawns moving toward our king by [type][distance from edge][rank]
const Value StormDanger[][4][RANK_NB] =
{
	  { { V(0),  V(67), V(134), V(38), V(32) },
		{ V(0),  V(57), V(139), V(37), V(22) },
		{ V(0),  V(43), V(115), V(43), V(27) },
		{ V(0),  V(68), V(124), V(57), V(32) } },
	  { { V(20),  V(43), V(100), V(56), V(20) },
		{ V(23),  V(20), V(98), V(40), V(15) },
		{ V(23),  V(39), V(103), V(36), V(18) },
		{ V(28),  V(19), V(108), V(42), V(26) } },
	  { { V(0),  V(0), V(75), V(14), V(2) },
		{ V(0),  V(0), V(150), V(30), V(4) },
		{ V(0),  V(0), V(160), V(22), V(5) },
		{ V(0),  V(0), V(166), V(24), V(13) } },
	  { { V(0),  V(-283), V(-281), V(57), V(31) },
		{ V(0),  V(58), V(141), V(39), V(18) },
		{ V(0),  V(65), V(142), V(48), V(32) },
		{ V(0),  V(60), V(126), V(51), V(19) } }
};

// Max bonus for king safety. Corresponds to start position with all the pawns
// in front of the king and no enemy pawn on the horizon.
const Value MaxSafetyBonus = V(258);

#undef S
#undef V

template<Color Us>
Score evaluate(const Position& pos, Pawns::Entry* e)
{
	return SCORE_ZERO;
}

namespace Pawns
{
	/// Pawns::init() initializes some tables needed by evaluation. Instead of using
	/// hard-coded tables, when makes sense, we prefer to calculate them with a formula
	/// to reduce independent parameters and to allow easier tuning and better insight.

	void init()
	{
		static const int Seed[RANK_NB] = { 0, 6, 15, 10, 57, 75, 135, 258 , 258 , 0 };

		for (int opposed = 0; opposed <= 1; ++opposed)
		{
			for (int phalanx = 0; phalanx <= 1; ++phalanx)
			{
				for (int apex = 0; apex <= 1; ++apex)
				{
					for (Rank r = RANK_0; r < RANK_9; ++r)
					{
						int v = (Seed[r] + (phalanx ? (Seed[r + 1] - Seed[r]) / 2 : 0)) >> opposed;
						v += (apex ? v / 2 : 0);
						Connected[opposed][phalanx][apex][r] = make_score(3 * v / 2, v);
					}
				}
			}
		}
	}

	/// Pawns::probe() looks up the current position's pawns configuration in
	/// the pawns hash table. It returns a pointer to the Entry if the position
	/// is found. Otherwise a new Entry is computed and stored there, so we don't
	/// have to recompute all when the same pawns configuration occurs again.

	Entry* probe(const Position& pos)
	{
		uint64_t key = pos.pawn_key();
		Entry* e = pos.this_thread()->pawnsTable[key];

		if (e->key == key)
			return e;

		e->key = key;
		e->score = evaluate<WHITE>(pos, e) - evaluate<BLACK>(pos, e);
		e->asymmetry = popcount<CNT_64>(e->semiopenFiles[WHITE] ^ e->semiopenFiles[BLACK]);
		return e;
	}

	/// Entry::shelter_storm() calculates shelter and storm penalties for the file
	/// the king is on, as well as the two adjacent files.

	template<Color Us>
	Score Entry::shelter_storm(const Position& pos, Square ksq)
	{
		return SCORE_ZERO;
	}

	/// Entry::do_king_safety() calculates a bonus for king safety. It is called only
	/// when king square changes, which is about 20% of total king_safety() calls.

	template<Color Us>
	Score Entry::do_king_safety(const Position& pos, Square ksq)
	{
		return SCORE_ZERO;
	}

	// Explicit template instantiation
	template Score Entry::do_king_safety<WHITE>(const Position& pos, Square ksq);
	template Score Entry::do_king_safety<BLACK>(const Position& pos, Square ksq);

} // namespace Pawns