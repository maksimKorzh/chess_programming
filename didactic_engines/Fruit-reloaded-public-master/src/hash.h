/* Fruit reloaded, a UCI chess playing engine derived from Fruit 2.1
 *
 * Copyright (C) 2004-2005 Fabien Letouzey
 * Copyright (C) 2012-2014 Daniel Mehrmann
 * Copyright (C) 2013-2014 Ryan Benitez
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// hash.h

#ifndef HASH_H
#define HASH_H

// includes

#include "board.h"
#include "util.h"

// macros

#define KEY_INDEX(key) (uint32(key))
#define KEY_LOCK(key)  (uint32((key)>>32))

// constants

const int RandomPiece     =   0; // 12 * 64
const int RandomCastle    = 768; // 4
const int RandomEnPassant = 772; // 8
const int RandomTurn      = 780; // 1

// variables

extern uint64 Castle64[16];

// functions

extern void   hash_init         ();

extern uint64 hash_key          (const board_t * board);
extern uint64 hash_pawn_key     (const board_t * board);
extern uint64 hash_material_key (const board_t * board);

extern uint64 hash_piece_key    (int piece, int square);
extern uint64 hash_castle_key   (int flags);
extern uint64 hash_ep_key       (int square);
extern uint64 hash_turn_key     (int colour);

#endif // !defined HASH_H

// end of hash.h

