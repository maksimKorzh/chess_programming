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

// search_full.h

#ifndef SEARCH_FULL_H
#define SEARCH_FULL_H

// includes

#include "board.h"
#include "thread.h"
#include "util.h"

// constants

const bool UseTrans = true;
const int TransDepth = 1 * HeightFull;

// functions

extern void search_full_init (list_t * list, board_t * board);
extern int  search_full_root (list_t * list, board_t * board, int depth, int search_type);

extern void full_thread      (board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int node_type, int thread, split_t * split);

#endif // !defined SEARCH_FULL_H

// end of search_full.h

