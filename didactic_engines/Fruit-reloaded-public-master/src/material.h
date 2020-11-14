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

// material.h

#ifndef MATERIAL_H
#define MATERIAL_H

// includes

#include "board.h"
#include "colour.h"
#include "util.h"

// constants

enum mat_dummy_t {
   MAT_NONE,
   MAT_KK,
   MAT_KBK, MAT_KKB,
   MAT_KNK, MAT_KKN,
   MAT_KPK, MAT_KKP,
   MAT_KRK, MAT_KKR,
   MAT_KQK, MAT_KKQ,
   MAT_KQKQ, MAT_KQKR, MAT_KRKQ, MAT_KQKP, MAT_KPKQ,
   MAT_KRKR, MAT_KRKP, MAT_KPKR,
   MAT_KNBK, MAT_KKNB,
   MAT_KBKB, MAT_KBBK, MAT_KKBB, MAT_KBKP, MAT_KPKB, MAT_KBPK, MAT_KKBP,
   MAT_KNKN, MAT_KNKP, MAT_KPKN, MAT_KNPK, MAT_KKNP,
   MAT_KRKB, MAT_KBKR, MAT_KRKN, MAT_KNKR,
   MAT_KRPKR, MAT_KRKRP,
   MAT_KBPKB, MAT_KBKBP,
   MAT_NB
};

enum mat_type_t {
   MAT_PAWN,
   MAT_KNIGHT,
   MAT_BISHOP,
   MAT_ROOK,
   MAT_QUEEN,
   MAT_BISHOP_PAIR
};

const int DrawNodeFlag    = 1 << 0;
const int DrawBishopFlag  = 1 << 1;

const int MatRookPawnFlag = 1 << 0;
const int MatBishopFlag   = 1 << 1;
const int MatKnightFlag   = 1 << 2;
const int MatKingFlag     = 1 << 3;

// types

struct material_info_t {
   uint32 lock;
   uint8 recog;
   uint8 flags;
   uint8 cflags[ColourNb];
   uint8 mul[ColourNb];
   sint16 phase;
   sint16 opening;
   sint16 endgame;
};

// functions

extern void material_init       ();

extern void material_clear      ();

extern void material_set_option (int weight);
extern void piece_set_option    (int weight, int piece);

extern void material_get_info   (material_info_t * info, const board_t * board, int thread);

#endif // !defined MATERIAL_H

// end of material.h

