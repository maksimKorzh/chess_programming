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

// sort.h

#ifndef SORT_H
#define SORT_H

// includes

#include "attack.h"
#include "board.h"
#include "list.h"
#include "util.h"

// types

struct sort_t {
   int depth;
   int height;
   int trans_killer;
   int killer_1;
   int killer_2;
   int gen;
   int test;
   int pos;
   int value;
   board_t * board;
   const attack_t * attack;
   list_t list[1];
   list_t bad[1];
};

// functions

extern void sort_init    ();

extern void move_eval    (const board_t * board, int eval);

extern void sort_init    (sort_t * sort, board_t * board, const attack_t * attack, int depth, int height, int trans_killer, int thread);
extern int  sort_next    (sort_t * sort);

extern void sort_init_qs (sort_t * sort, board_t * board, const attack_t * attack, int depth, int node_type, int trans_killer);
extern int  sort_next_qs (sort_t * sort);

extern void good_move    (int move, const board_t * board, int depth, int height, int thread, bool cut);
extern void bad_move     (int move, const board_t * board, int depth);

extern void killer_copy  (int dst, int src, int height);

extern void note_moves   (list_t * list, const board_t * board, int height, int trans_killer, int thread);

#endif // !defined SORT_H

// end of sort.h

