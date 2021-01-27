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
#include <cstring>
#include "init.h"
#include "bitcount.h"
#include "magics.h"

int SquareDistance[SQUARE_NB][SQUARE_NB];

Bitboard  RookAttackMask[SQUARE_NB];
Bitboard  RookMasks[SQUARE_NB];
Bitboard  RookMagics[SQUARE_NB];
Bitboard* RookAttacks[SQUARE_NB];
unsigned  RookShifts[SQUARE_NB];

Bitboard  CannonMasks[SQUARE_NB];
Bitboard* CannonAttacks[SQUARE_NB];
Bitboard  CannonMagics[SQUARE_NB];
unsigned  CannonShifts[SQUARE_NB];

Bitboard  SuperCannonMasks[SQUARE_NB];
Bitboard* SuperCannonAttacks[SQUARE_NB];
Bitboard  SuperCannonMagics[SQUARE_NB];
unsigned  SuperCannonShifts[SQUARE_NB];

Bitboard KnightAttackMask[SQUARE_NB];
Bitboard KnightLeg[SQUARE_NB];
Bitboard KnightEye[SQUARE_NB];
Bitboard KnightAttackTo[SQUARE_NB][16];
Bitboard KnightAttackFrom[SQUARE_NB][16];
uint64_t KnightLegMagic[SQUARE_NB];
uint64_t KnightEyeMagic[SQUARE_NB];

Bitboard BishopEye[SQUARE_NB];
Bitboard BishopAttack[SQUARE_NB][16];
uint64_t BishopEyeMagic[SQUARE_NB];

Bitboard AdvisorAttack[SQUARE_NB];

Bitboard KingAttack[SQUARE_NB];

Bitboard PawnAttackTo[COLOR_NB][SQUARE_NB];
Bitboard PawnAttackFrom[COLOR_NB][SQUARE_NB];
Bitboard PawnMask[COLOR_NB];
Bitboard PassedRiverBB[COLOR_NB];

Bitboard CityBB[COLOR_NB];
Bitboard AdvisorCityBB[COLOR_NB];
Bitboard BishopCityBB[COLOR_NB];

Bitboard SquareBB[SQUARE_NB];
Bitboard FileBB[FILE_NB];
Bitboard RankBB[RANK_NB];
Bitboard AdjacentFilesBB[FILE_NB];
Bitboard InFrontBB[COLOR_NB][RANK_NB];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];
Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard DistanceRingBB[SQUARE_NB][8];
Bitboard ForwardBB[COLOR_NB][SQUARE_NB];
Bitboard PassedPawnMask[COLOR_NB][SQUARE_NB];
Bitboard PawnAttackSpan[COLOR_NB][SQUARE_NB];
// Bitboard PseudoAttacks[PIECE_TYPE_NB][SQUARE_NB];

namespace
{
	// De Bruijn sequences. See chessprogramming.wikispaces.com/BitScan
	const uint64_t DeBruijn64 = 0x3F79D71B4CB0A89ULL;
	const uint32_t DeBruijn32 = 0x783A9B23;

	int MSBTable[256];      // To implement software msb()
	Square BSFTable[128];   // To implement software bitscan

	Bitboard RookTable[1081344];//To store rook attacks
	Bitboard CannonTable[1081344];
	Bitboard SuperCannonTable[1081344];

	typedef unsigned (IndexFn)(Square sq, const Bitboard& occupied);
	typedef Bitboard(SliderAttackFun)(Square deltas[], Square sq, const Bitboard& occupied);
	typedef Bitboard(KnightAttackFun)(Square sq, const Bitboard& occupied);

	void init_slider_magics(Bitboard table[],
		Bitboard* attacks[],
		Bitboard magics[],
		Bitboard masks[],
		unsigned shifts[],
		Square deltas[],
		IndexFn index,
		SliderAttackFun attackfun,
		Bitboard magicsdata[],
		unsigned shiftsdata[]);

	void init_knight_maigcs(Bitboard attack[SQUARE_NB][16],
		Bitboard legoreye[SQUARE_NB],
		uint64_t imagic[SQUARE_NB],
		IndexFn index,
		KnightAttackFun attackfun,
		uint64_t magicsdata[]
	);

	void init_bishop_magics(uint64_t magicsdata[]);

	/// bsf_index() returns the index into BSFTable[] to look up the bitscan. Uses
	/// Matt Taylor's folding for 32 bit case, extended to 64 bit by Kim Walisch.

	unsigned bsf_index(Bitboard b)
	{
		if (b.bb[0] > 0)
			return (((b.bb[0] ^ (b.bb[0] - 1))* DeBruijn64) >> 58);
		else if (b.bb[1] > 0)
			return (((b.bb[1] ^ (b.bb[1] - 1))* DeBruijn64) >> 58) + 64;
		return 0;
	}

	Bitboard sliding_attack(Square deltas[], Square sq, const Bitboard& occupied)
	{
		Bitboard attack(0, 0);
		for (int i = 0; i < 4; ++i)
		{
			for (Square s = sq + deltas[i]; is_ok(s) && distance(s, s - deltas[i]) == 1;
				s += deltas[i])
			{
				attack |= s;

				if (occupied & s)
					break;
			}
		}
		return attack;
	}

	Bitboard cannon_sliding_control(Square deltas[], Square sq, const Bitboard& occupied)
	{
		Bitboard attack(0, 0);
		for (int i = 0; i < 4; ++i)
		{
			int count = 0;
			for (Square s = sq + deltas[i];	is_ok(s) && distance(s, s - deltas[i]) == 1;
				s += deltas[i])
			{
				if (count == 1)
					attack |= s;
				else if (count >= 2)
					break;
				if (occupied & s)
					count++;
			}
		}
		return attack;
	}

	Bitboard supercannon_sliding_control(Square deltas[], Square sq, const Bitboard& occupied)
	{
		Bitboard attack(0, 0);
		for (int i = 0; i < 4; ++i)
		{
			int count = 0;
			for (Square s = sq + deltas[i];	is_ok(s) && distance(s, s - deltas[i]) == 1;
				s += deltas[i])
			{
				/*if (count == 1)
					attack |= s;
				else */if (count == 2)
					attack |= s;
				else if (count >= 3)
					break;

				if (occupied & s)
					count++;
			}
		}
		return attack;
	}

	Bitboard knight_attack_from(Square sq, const Bitboard& leg)
	{
		Bitboard attack;

		Square KnightLegDeltas[4] = { DELTA_N, DELTA_E,DELTA_S,DELTA_W };
		Square KnightStep[4][2] = { { DELTA_NNW,DELTA_NNE },{ DELTA_EEN, DELTA_EES },{ DELTA_SSE, DELTA_SSW },{ DELTA_WWS,DELTA_WWN } };

		for (int dir = 0; dir < 4; ++dir)
		{
			for (int foot = 0; foot < 2; ++foot)
			{
				int to = (int)sq + (int)KnightStep[dir][foot];
				if (is_ok(to) && distance(Square(sq), Square(to)) < 3)
				{
					int legsq = int(sq) + KnightLegDeltas[dir];
					if (is_ok(legsq) && distance(Square(sq), Square(legsq)) == 1)
					{
						if (!(leg & SquareBB[legsq]))
						{
							attack |= SquareBB[to];
						}
					}
				}
			}
		}
		return attack;
	}

	Bitboard knight_attack_to(Square sq, const Bitboard& eye)
	{
		Bitboard attack;

		Square KnightEyeDeltas[4] = { DELTA_NW, DELTA_NE, DELTA_SE, DELTA_SW };
		Square KnightStepFrom[4][2] = { { DELTA_WWN,DELTA_NNW },{ DELTA_NNE,DELTA_EEN },{ DELTA_EES,DELTA_SSE },{ DELTA_SSW, DELTA_WWS } };

		for (int dir = 0; dir < 4; ++dir)
		{
			for (int fr = 0; fr < 2; ++fr)
			{
				int from = (int)sq + (int)KnightStepFrom[dir][fr];
				if (is_ok(from) && distance(Square(sq), Square(from)) < 3)
				{
					int sqeye = int(sq) + int(KnightEyeDeltas[dir]);
					if (is_ok(sqeye) && distance(Square(sq), Square(sqeye)) == 1)
					{
						if (!(eye&SquareBB[sqeye]))
						{
							attack |= SquareBB[from];
						}
					}
				}
			}
		}
		return attack;
	}

	Bitboard bishop_attack_from(Square sq, const Bitboard& eye)
	{
		Bitboard attack;

		Square EyeDeltas[4] = { DELTA_NW, DELTA_NE, DELTA_SE, DELTA_SW };
		Square Step[4] = { DELTA_NNWW, DELTA_NNEE, DELTA_SSEE,DELTA_SSWW };

		for (int dir = 0; dir < 4; ++dir)
		{
			int to = (int)sq + (int)Step[dir];
			if (is_ok(to) && distance(Square(sq), Square(to)) < 3 && square_same_color(Square(sq), Square(to)) && bishop_in_city(Square(to)) && bishop_in_city(Square(sq))) {	 //在棋盘上，且没有发生回绕
				int bishopeye = int(sq) + int(EyeDeltas[dir]);
				if (is_ok(bishopeye) && distance(Square(sq), Square(bishopeye)) == 1 && square_same_color(Square(sq), Square(to)))
				{
					if (!(eye&SquareBB[bishopeye]))
					{
						attack |= SquareBB[to];
					}
				}
			}
		}
		return attack;
	}
}

/// Bitboards::pretty() returns an ASCII representation of a bitboard suitable
/// to be printed to standard output. Useful for debugging.

const std::string Bitboards::pretty(Bitboard b)
{
	std::string s;

	int shift[90] =
	{
			81, 82, 83, 84, 85, 86, 87, 88, 89,
			72, 73, 74, 75, 76, 77, 78, 79, 80,
			63, 64, 65, 66, 67, 68, 69, 70, 71,
			54, 55, 56, 57, 58, 59, 60, 61, 62,
			45, 46, 47, 48, 49, 50, 51, 52, 53,
			36, 37, 38, 39, 40, 41, 42, 43, 44,
			27, 28, 29, 30, 31, 32, 33, 34, 35,
			18, 19, 20, 21, 22, 23, 24, 25, 26,
			9,  10, 11, 12, 13, 14, 15, 16, 17,
			0,  1,  2,  3,  4,  5,  6,  7,  8,
	};

	for (int i = 0; i < 90; ++i)
	{
		Bitboard one(0x1, 0);
		Bitboard t = one << shift[i];

		if ((t&(b))) s += "1";
		else         s += "0";

		if ((i + 1) % 9 == 0)
			s += "\n";

		if (i + 1 == 45)
			s += "---------\n";
	}
	return s;
}

/// Bitboards::init() initializes various bitboard tables. It is called at
/// startup and relies on global objects to be already zero-initialized.

void Bitboards::init()
{
	for (int k = 0, i = 0; i < 8; ++i)
		while (k < (2 << i))
			MSBTable[k++] = i;

	for (int i = 0; i < SQUARE_NB; ++i)
		BSFTable[bsf_index(Bitboard(1, 0) << i)] = Square(i);

	for (int i = 0; i < SQUARE_NB; ++i)
		SquareBB[i] = (Bitboard(1, 0) << (i));

	FileBB[FILE_A] = FileABB;
	FileBB[FILE_B] = FileBBB;
	FileBB[FILE_C] = FileCBB;
	FileBB[FILE_D] = FileDBB;
	FileBB[FILE_E] = FileEBB;
	FileBB[FILE_F] = FileFBB;
	FileBB[FILE_G] = FileGBB;
	FileBB[FILE_H] = FileHBB;
	FileBB[FILE_I] = FileIBB;

	RankBB[RANK_0] = Rank0BB;
	RankBB[RANK_1] = Rank1BB;
	RankBB[RANK_2] = Rank2BB;
	RankBB[RANK_3] = Rank3BB;
	RankBB[RANK_4] = Rank4BB;
	RankBB[RANK_5] = Rank5BB;
	RankBB[RANK_6] = Rank6BB;
	RankBB[RANK_7] = Rank7BB;
	RankBB[RANK_8] = Rank8BB;
	RankBB[RANK_9] = Rank9BB;

	CityBB[WHITE] = WhiteCityBB;
	CityBB[BLACK] = BlackCityBB;

	AdvisorCityBB[WHITE] = WhiteAdvisorCityBB;
	AdvisorCityBB[BLACK] = BlackAdvisorCityBB;

	BishopCityBB[WHITE] = WhiteBishopCityBB;
	BishopCityBB[BLACK] = BlackBishopCityBB;

	PawnMask[WHITE] = WhitePawnMaskBB;
	PawnMask[BLACK] = BlackPawnMaskBB;

	PassedRiverBB[WHITE] = DarkSquares;
	PassedRiverBB[BLACK] = ~DarkSquares;

	for (File f = FILE_A; f <= FILE_I; ++f)
		AdjacentFilesBB[f] = (f > FILE_A ? FileBB[f - 1] : Bitboard(0, 0)) | (f < FILE_I ? FileBB[f + 1] : Bitboard(0, 0));

	for (Rank r = RANK_0; r < RANK_9; ++r)
		InFrontBB[WHITE][r] = ~(InFrontBB[BLACK][r + 1] = InFrontBB[BLACK][r] | RankBB[r]);

	for (Color c = WHITE; c <= BLACK; ++c)
	{
		for (Square s = SQ_A0; s <= SQ_I9; ++s)
		{
			ForwardBB[c][s] = InFrontBB[c][rank_of(s)] & FileBB[file_of(s)];
			// PawnAttackSpan[c][s] = InFrontBB[c][rank_of(s)] & AdjacentFilesBB[file_of(s)];
			// PassedPawnMask[c][s] = ForwardBB[c][s] | PawnAttackSpan[c][s];

			PawnAttackSpan[c][s] = (InFrontBB[c][rank_of(s)] | RankBB[rank_of(s)]) & PawnMask[c];  //?

			PassedPawnMask[c][s] = (InFrontBB[c][rank_of(s)] | RankBB[rank_of(s)]) & PassedRiverBB[c];//?
		}
	}

	for (Square s1 = SQ_A0; s1 <= SQ_I9; ++s1)
	{
		for (Square s2 = SQ_A0; s2 <= SQ_I9; ++s2)
		{
			if (s1 != s2)
			{
				SquareDistance[s1][s2] = std::max(distance<File>(s1, s2), distance<Rank>(s1, s2));
				DistanceRingBB[s1][SquareDistance[s1][s2] - 1] |= s2;
			}
		}
	}

	Square RDeltas[] = { DELTA_N,  DELTA_E,  DELTA_S,  DELTA_W };

	for (Square s = SQ_A0; s <= SQ_I9; ++s)
		RookAttackMask[s] = sliding_attack(RDeltas, s, Bitboard());

	Square KnightLegDeltas[4] = { DELTA_N, DELTA_E,DELTA_S,DELTA_W };
	Square KnightStep[4][2] = { { DELTA_NNW,DELTA_NNE },{ DELTA_EEN, DELTA_EES },{ DELTA_SSE, DELTA_SSW },{ DELTA_WWS,DELTA_WWN } };

	for (Square s = SQ_A0; s <= SQ_I9; ++s)
	{
		KnightLeg[s] = Bitboard();
		for (int i = 0; i < 4; ++i)
		{
			int to = (int)s + (int)KnightLegDeltas[i];
			if (is_ok(to) && distance(Square(s), Square(to)) == 1)
				KnightLeg[s] |= SquareBB[to];
		}

		for (int j = 0; j < 4; ++j)
		{
			for (int k = 0; k < 2; ++k)
			{
				int to = int(s) + KnightStep[j][k];
				if (is_ok(to) && distance(Square(s), Square(to)) < 3)
					KnightAttackMask[s] |= SquareBB[to];
			}
		}
	}

	Square KnightEyeDeltas[4] = { DELTA_NW, DELTA_NE, DELTA_SE, DELTA_SW };
	// Square KnightStepFrom[4][2] = { { DELTA_WWN,DELTA_NNW },{ DELTA_NNE,DELTA_EEN },{ DELTA_EES,DELTA_SSE },{ DELTA_SSW, DELTA_WWS } };
	for (Square s = SQ_A0; s <= SQ_I9; ++s)
	{
		for (int i = 0; i < 4; ++i)
		{
			int to = (int)s + (int)KnightEyeDeltas[i];
			if (is_ok(to) && distance(Square(s), Square(to)) == 1)
				KnightEye[s] |= SquareBB[to];
		}
	}

	Square BishopEyeDeltas[4] = { DELTA_NW, DELTA_NE, DELTA_SE, DELTA_SW };
	// Square BishopStepDeltas[4] = { DELTA_NNWW, DELTA_NNEE, DELTA_SSEE, DELTA_SSWW };
	for (Square s = SQ_A0; s <= SQ_I9; ++s)
	{
		BishopEye[s] = Bitboard();
		for (int i = 0; i < 4; ++i)
		{
			int to = (int)s + (int)BishopEyeDeltas[i];
			if (is_ok(to) && distance(Square(s), Square(to)) == 1 && square_same_color(Square(s), Square(to)))
				BishopEye[s] |= SquareBB[to];
		}
	}

	Square AdvisorDetas[4] = { DELTA_NW, DELTA_NE, DELTA_SE, DELTA_SW };
	for (Square s = SQ_A0; s <= SQ_I9; ++s)
	{
		AdvisorAttack[s] = Bitboard();
		for (int i = 0; i < 4; ++i)
		{
			int to = (int)s + AdvisorDetas[i];
			if (is_ok(to) && distance(Square(s), Square(to)) == 1 && advisor_in_city(Square(to)) && advisor_in_city(Square(s)))
				AdvisorAttack[s] |= SquareBB[to];
		}
	}

	Square PawnDetas[COLOR_NB][3] = { { DELTA_N,DELTA_W, DELTA_E },{ DELTA_S,DELTA_E,DELTA_W } };
	for (Color c = WHITE; c < COLOR_NB; ++c)
	{
		for (Square s = SQ_A0; s <= SQ_I9; ++s)
		{
			PawnAttackFrom[c][s] = Bitboard();
			for (int i = 0; i < 3; ++i)
			{
				int to = int(s) + PawnDetas[c][i];
				if (is_ok(to) && distance(Square(s), Square(to)) == 1)
					if (PawnMask[c] & SquareBB[s])
						if (PawnMask[c] & SquareBB[to])
							PawnAttackFrom[c][s] |= SquareBB[to];
			}

			Color opp = ~c;

			PawnAttackTo[opp][s] = Bitboard();

			for (int i = 0; i < 3; ++i)
			{
				int to = int(s) + PawnDetas[c][i];
				if (is_ok(to) && distance(Square(s), Square(to)) == 1)
					if (PawnMask[opp] & SquareBB[s])
						if (PawnMask[opp] & SquareBB[to])
							PawnAttackTo[opp][s] |= SquareBB[to];
			}
		}
	}

	Square KingDetas[4] = { DELTA_N, DELTA_E, DELTA_S,DELTA_W };
	for (Square s = SQ_A0; s <= SQ_I9; ++s)
	{
		KingAttack[s] = Bitboard();
		for (int i = 0; i < 4; ++i)
		{
			int to = (int)s + KingDetas[i];
			if (is_ok(to) && distance(Square(s), Square(to)) == 1)
				if (square_in_city(Square(to)) && square_in_city(Square(s)))
					KingAttack[s] |= SquareBB[to];
		}
	}

	for (Square s1 = SQ_A0; s1 <= SQ_I9; ++s1)
	{
		for (Square s2 = SQ_A0; s2 <= SQ_I9; ++s2)
		{
			if (RookAttackMask[s1] & s2)
			{
				Square delta = (s2 - s1) / distance(s1, s2);
				for (Square s = s1 + delta; s != s2; s += delta)
					BetweenBB[s1][s2] |= s;
				LineBB[s1][s2] = (RookAttackMask[s1] & RookAttackMask[s2]) | s1 | s2;
			}
		}
	}


	init_slider_magics(RookTable, RookAttacks, RookMagics, RookMasks, RookShifts, RDeltas, slider_magic_index<ROOK>, sliding_attack, RookMagicsData, RookShiftsData);
	init_slider_magics(CannonTable, CannonAttacks, CannonMagics, CannonMasks, CannonShifts, RDeltas, slider_magic_index<CANNON>, cannon_sliding_control, CannonMagicsData, CannonShiftsData);
	init_slider_magics(SuperCannonTable, SuperCannonAttacks, SuperCannonMagics, SuperCannonMasks, SuperCannonShifts, RDeltas, slider_magic_index<SUPER_CANNON>, supercannon_sliding_control, SuperCannonMagicsData, SuperCannonShiftsData);

	init_knight_maigcs(KnightAttackFrom, KnightLeg, KnightLegMagic, block_magic_index<KNIGHT_LEG>, knight_attack_from, KnightLegMagicsData);
	init_knight_maigcs(KnightAttackTo, KnightEye, KnightEyeMagic, block_magic_index<KNIGHT_EYE>, knight_attack_to, KnightEyeMagicsData);

	init_bishop_magics(BishopLegMagicsData);
}


namespace
{
	void init_slider_magics(Bitboard table[],
		Bitboard* attacks[],
		Bitboard magics[],
		Bitboard masks[],
		unsigned shifts[],
		Square deltas[],
		IndexFn index,
		SliderAttackFun attackfun,
		Bitboard magicsdata[],
		unsigned datashifts[])
	{

		Bitboard occupancy[1 << 15], reference[1 << 15], edges, b;
		int i, size;

		// Attacks[s] is a pointer to the beginning of the attacks table for square 's'
		attacks[SQ_A0] = table;

		for (Square s = SQ_A0; s <= SQ_I9; ++s)
		{
			// Board edges are not considered in the relevant occupancies
			edges = ((Rank0BB | Rank9BB) & ~rank_bb(s)) | ((FileABB | FileIBB) & ~file_bb(s));
			masks[s] = sliding_attack(deltas, s, Bitboard()) & ~edges;
			shifts[s] = 64 - popcount(masks[s]);
			size = 0;
			b = Bitboard();
			do
			{
				do
				{
					occupancy[size] = b;
					reference[size] = attackfun(deltas, s, b);

					size++;
					b.bb[0] = (b.bb[0] - masks[s].bb[0])&masks[s].bb[0];
				} while (b.bb[0]);
				b.bb[1] = (b.bb[1] - masks[s].bb[1])&masks[s].bb[1];
			} while (b.bb[1]);
			if (s < SQ_I9)
				attacks[s + 1] = attacks[s] + size;
			if (shifts[s] != datashifts[s])
				printf("sq%d shift error\n", s);
			do
			{
				magics[s] = magicsdata[s];
				std::memset(attacks[s], 0, size * sizeof(Bitboard));
				for (i = 0; i < size; ++i)
				{
					Bitboard& attack = attacks[s][index(s, occupancy[i])];

					if (attack && attack != reference[i])
						break;

					attack = reference[i];
				}
			} while (i < size);
		}
	}

	void init_knight_maigcs(Bitboard attacktable[SQUARE_NB][16],
		Bitboard legoreye[SQUARE_NB],
		uint64_t imagic[SQUARE_NB],
		IndexFn index,
		KnightAttackFun attackfun,
		uint64_t magicsdata[])
	{
		Bitboard occupancy[1 << 4], reference[1 << 4], edges, b;
		int i, size;

		for (Square s = SQ_A0; s <= SQ_I9; ++s)
		{
			size = 0;
			b = Bitboard();
			do
			{
				do
				{
					occupancy[size] = b;
					reference[size] = attackfun(s, b);
					size++;
					b.bb[0] = (b.bb[0] - legoreye[s].bb[0])&legoreye[s].bb[0];
				} while (b.bb[0]);
				b.bb[1] = (b.bb[1] - legoreye[s].bb[1])&legoreye[s].bb[1];

			} while (b.bb[1]);

			do
			{
				imagic[s] = magicsdata[s];
				std::memset(attacktable[s], 0, size * sizeof(Bitboard));

				for (i = 0; i < size; ++i)
				{
					Bitboard& attack = attacktable[s][index(s, occupancy[i])];
					if (attack && attack != reference[i])
						break;
					attack = reference[i];
				}
			} while (i < size);
		}
	}

	void init_bishop_magics(uint64_t magicsdata[])
	{

		Bitboard occupancy[1 << 4], reference[1 << 4], b;
		int i, size;

		for (Square s = SQ_A0; s <= SQ_I9; ++s)
		{
			if (!bishop_in_city(s))
				continue;
			size = 0;
			b = Bitboard();
			do
			{
				do
				{
					occupancy[size] = b;
					reference[size] = bishop_attack_from(s, b);
					size++;
					b.bb[0] = (b.bb[0] - BishopEye[s].bb[0])&BishopEye[s].bb[0];
				} while (b.bb[0]);
				b.bb[1] = (b.bb[1] - BishopEye[s].bb[1])&BishopEye[s].bb[1];

			} while (b.bb[1]);

			do
			{
				BishopEyeMagic[s] = magicsdata[s];
				std::memset(BishopAttack[s], 0, size * sizeof(Bitboard));
				for (i = 0; i < size; ++i)
				{
					Bitboard& attack = BishopAttack[s][block_magic_index<BISHOP_EYE>(s, occupancy[i])];
					if (attack && attack != reference[i])
						break;
					attack = reference[i];
				}
			} while (i < size);
		}
	}
}
