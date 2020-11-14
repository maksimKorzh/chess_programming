/*
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// move_gen.h

#ifndef MOVE_GEN_H
#define MOVE_GEN_H

// includes

#include "attack.h"
#include "board.h"
#include "list.h"
#include "util.h"

// functions

extern void gen_legal_moves (list_t * list, board_t * board);

extern void gen_moves       (list_t * list, const board_t * board);
extern void gen_captures    (list_t * list, const board_t * board);
extern void gen_quiet_moves (list_t * list, const board_t * board);

extern void gen_promotes    (list_t * list, const board_t * board, int turn);

extern void add_pawn_move   (list_t * list, int from, int to);
extern void add_promote     (list_t * list, int move);

#endif // !defined MOVE_GEN_H

// end of move_gen.h

