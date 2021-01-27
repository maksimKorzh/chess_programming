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

#include "init.h"
#include "bitcount.h"
#include "endgame.h"
#include "movegen.h"

using std::string;

// Table used to drive the king towards the edge of the board
// in KX vs K and KQ vs KR endgames.
const int PushToEdges[SQUARE_NB] =
{
	  100, 90, 80, 70, 70, 80, 90, 100,
	   90, 70, 60, 50, 50, 60, 70,  90,
	   80, 60, 40, 30, 30, 40, 60,  80,
	   70, 50, 30, 20, 20, 30, 50,  70,
	   70, 50, 30, 20, 20, 30, 50,  70,
	   80, 60, 40, 30, 30, 40, 60,  80,
	   90, 70, 60, 50, 50, 60, 70,  90,
	  100, 90, 80, 70, 70, 80, 90, 100
};

// Table used to drive the king towards a corner square of the
// right color in KBN vs K endgames.
const int PushToCorners[SQUARE_NB] =
{
	  200, 190, 180, 170, 160, 150, 140, 130,
	  190, 180, 170, 160, 150, 140, 130, 140,
	  180, 170, 155, 140, 140, 125, 140, 150,
	  170, 160, 140, 120, 110, 140, 150, 160,
	  160, 150, 140, 110, 120, 140, 160, 170,
	  150, 140, 125, 140, 140, 155, 170, 180,
	  140, 130, 140, 150, 160, 170, 180, 190,
	  130, 140, 150, 160, 170, 180, 190, 200
};

// Tables used to drive a piece towards or away from another piece
const int PushClose[8] = { 0, 0, 100, 80, 60, 40, 20, 10 };
const int PushAway[8] = { 0, 5, 20, 40, 60, 80, 90, 100 };

// Pawn Rank based scaling factors used in KRPPKRP endgame
const int KRPPKRPScaleFactors[RANK_NB] = { 0, 9, 10, 14, 21, 44, 0, 0 };

#ifndef NDEBUG
bool verify_material(const Position& pos, Color c, Value npm, int pawnsCnt)
{
	return pos.non_pawn_material(c) == npm && pos.count<PAWN>(c) == pawnsCnt;
}
#endif

/// normalize() map the square as if strongSide is white and strongSide's only pawn
/// is on the left half of the board.
Square normalize(const Position& pos, Color strongSide, Square sq)
{
	assert(pos.count<PAWN>(strongSide) == 1);

	if (file_of(pos.square<PAWN>(strongSide)) >= FILE_E)
		sq = Square(sq ^ 7); // Mirror SQ_H1 -> SQ_A1

	if (strongSide == BLACK)
		sq = ~sq;

	return sq;
}

/// key() get the material key of Position out of the given endgame key code
/// like "KBPKN". The trick here is to first forge an ad-hoc FEN string
/// and then let a Position object do the work for us.

uint64_t key(const string& code, Color c)
{
	assert(code.length() > 0 && code.length() < 8);
	assert(code[0] == 'K');

	string sides[] = { code.substr(code.find('K', 1)),      // Weak
					   code.substr(0, code.find('K', 1)) }; // Strong

	std::transform(sides[c].begin(), sides[c].end(), sides[c].begin(), tolower);

	string fen = sides[0] + char(8 - sides[0].length() + '0') + "/8/8/8/8/8/8/"
		+ sides[1] + char(8 - sides[1].length() + '0') + " w - - 0 10";

	return Position(fen, false, nullptr).material_key();
}
