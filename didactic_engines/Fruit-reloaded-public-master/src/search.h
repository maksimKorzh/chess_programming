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

// search.h

#ifndef SEARCH_H
#define SEARCH_H

// includes

#include "board.h"
#include "list.h"
#include "move.h"
#include "smp.h"
#include "util.h"

// constants

const int HeightFull = 2;
const int HeightHalf = 1;

const int DepthMax = 64;
const int HeightMax = 256;
const int MultiMax = 256;

const int SearchNormal = 0;
const int SearchShort  = 1;
const int SearchMulti = 2;

const int SearchUnknown = 0;
const int SearchUpper   = 1;
const int SearchLower   = 2;
const int SearchExact   = 3;

const int NodeNb = 2;

const int NodeAll = -1;
const int NodePV  =  0;
const int NodeCut = +1;

// types

struct search_input_t {
   board_t board[1];
   list_t list[1];
   bool infinite;
   bool depth_is_limited;
   int depth_limit;
   bool time_is_limited;
   double time_limit_1;
   double time_limit_2;
   double time_limit_3;
   bool nodes_is_limited;
   sint64 nodes;
};

struct search_info_t {
   bool search_stop;
   bool can_stop;
   bool stop;
   int check_nb;
   int check_inc;
   double last_time;
};

struct search_root_t {
   list_t list[1];
   int depth;
   int move;
   int move_pos;
   int move_nb;
   int last_value;
   bool bad_1;
   bool bad_2;
   bool change;
   bool easy;
   bool flag;
   bool research;
};

struct search_best_t {
   int move;
   int value;
   int flags;
   int depth;
   sint64 node_nb;
   double time;
   mv_t pv[HeightMax];
};

struct search_current_t {
   board_t board[1];
   my_timer_t timer[1];
   int max_depth[ThreadMax];
   sint64 node_nb[ThreadMax];
   double time;
   double speed;
   double cpu;
};

// variables

extern search_input_t SearchInput[1];
extern search_info_t SearchInfo[1];
extern search_root_t SearchRoot[1];
extern search_current_t SearchCurrent[1];
extern search_best_t SearchBest[MultiMax];

// functions

extern bool   depth_is_ok             (int depth);
extern bool   height_is_ok            (int height);

extern void   search_disp             (bool disp);

extern void   search_clear            ();
extern void   search_clear_best       (bool all);

extern void   search                  ();

extern sint64 search_update_node_nb   ();
extern int    search_update_max_depth ();

extern void   search_update_best      ();
extern void   search_update_root      ();
extern void   search_update_current   ();

extern void   search_check            ();

#endif // !defined SEARCH_H

// end of search.h

