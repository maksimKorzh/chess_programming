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

// pawn.h

#ifndef PAWN_H
#define PAWN_H

// includes

#include "board.h"
#include "colour.h"
#include "util.h"

// constants

const int BackRankFlag = 1 << 0;
const int SevenRankFlag = 1 << 1;

// types

struct pawn_info_t {
   uint32 lock;
   sint16 opening;
   sint16 endgame;
   uint8 flags[ColourNb];
   uint8 passed_bits[ColourNb];
   uint8 single_file[ColourNb];
   uint16 pad;
};

// functions

extern void pawn_init_bit             ();
extern void pawn_init                 ();

extern void pawn_clear                ();

extern void pawn_structure_set_option (int weight);

extern void pawn_get_info             (pawn_info_t * info, const board_t * board, int thread);

extern int  quad                      (int y_min, int y_max, int x);

#endif // !defined PAWN_H

// end of pawn.h

