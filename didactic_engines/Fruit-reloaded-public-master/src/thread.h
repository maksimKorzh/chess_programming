/* Fruit reloaded, a UCI chess playing engine derived from Fruit 2.1
 *
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

// thread.h

#ifndef THREAD_H
#define THREAD_H

// includes

#include "board.h"
#include "move.h"
#include "mutex.h"
#include "smp.h"
#include "sort.h"

// constants

const int ThreadMain = 0;

// types

struct split_t {

   const board_t * board;
   split_t * split_nb;
   sort_t * sort;

   mutex_t lock[1];

   volatile int alpha;
   volatile int best_move;
   volatile int best_value;
   volatile int played_nb;

   int beta;
   int depth;
   int eval_value;
   int height;
   int node_type;
   bool in_check;

   mv_t * played;
   mv_t * pv;

   int master;
   volatile bool slave[ThreadMax];
   volatile int thread_nb;

   volatile bool cut; // cutoff info
   volatile bool end; // move_gen info
};

// functions

extern bool thread_is_free       (int master);
extern bool thread_is_stop       (int id);

extern bool do_split             (const board_t * board, int * alpha, int old_alpha, int beta, int depth, int height, mv_t pv[],
                                  int node_type, int * best_value, int * played_nb, int * best_move, sort_t * sort,
                                  mv_t played[],int eval_value, bool in_check, int master);

#endif // !defined THREAD_H

// end of thread.h
