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

// search.cpp

// includes

#include "attack.h"
#include "board.h"
#include "book.h"
#include "colour.h"
#include "egbb.h"
#include "list.h"
#include "material.h"
#include "move.h"
#include "move_do.h"
#include "move_gen.h"
#include "option.h"
#include "pawn.h"
#include "protocol.h"
#include "pv.h"
#include "search.h"
#include "search_full.h"
#include "smp.h"
#include "sort.h"
#include "tb.h"
#include "trans.h"
#include "util.h"
#include "value.h"

// constants

static const bool UseCpuTime = false; // false
static bool UseEvent = true; // true

static const bool UseShortSearch = true;
static const int ShortSearchDepth = 1;

static bool DispSearch = true;

static bool DispBest = true; // true
static bool DispDepthStart = true; // true
static bool DispDepthEnd = true; // true
static bool DispRoot = true; // true
static bool DispStat = true; // true

static const bool UseEasy = true; // singular move
static const int EasyThreshold = 150;
static const double EasyRatio = 0.20;

static const bool UseEarly = true; // early iteration end
static const double EarlyRatio = 0.60;

static const bool UseBad = true;
static const int BadThreshold = 50; // 50
static const bool UseExtension = true;

// variables

search_input_t   SearchInput[1];
search_info_t    SearchInfo[1];
search_root_t    SearchRoot[1];
search_current_t SearchCurrent[1];
search_best_t    SearchBest[MultiMax];

// prototypes

static void search_send_stat    ();

static void search_update_depth (int depth);
static void search_update_sort  ();

// functions

// depth_is_ok()

bool depth_is_ok(int depth) {

   return depth > -128 && depth < DepthMax * HeightFull;
}

// height_is_ok()

bool height_is_ok(int height) {

   return height >= 0 && height < HeightMax;
}

// search_disp()

void search_disp(bool disp) {

   ASSERT(disp==true||disp==false);
   DispSearch = disp;
}

// search_clear()

void search_clear() {

   int thread;

   // SearchInput

   SearchInput->infinite = false;
   SearchInput->depth_is_limited = false;
   SearchInput->depth_limit = 0;
   SearchInput->time_is_limited = false;
   SearchInput->time_limit_1 = 0.0;
   SearchInput->time_limit_2 = 0.0;
   SearchInput->time_limit_3 = 0.0;
   SearchInput->nodes_is_limited = false;
   SearchInput->nodes = 0;

   // SearchInfo

   SearchInfo->search_stop = false;
   SearchInfo->can_stop = false;
   SearchInfo->stop = false;
   SearchInfo->check_nb = 10000; // was 100000
   SearchInfo->check_inc = 10000; // was 100000
   SearchInfo->last_time = 0.0;

   // SearchRoot

   SearchRoot->depth = 0;
   SearchRoot->move = MoveNone;
   SearchRoot->move_pos = 0;
   SearchRoot->move_nb = 0;
   SearchRoot->last_value = 0;
   SearchRoot->bad_1 = false;
   SearchRoot->bad_2 = false;
   SearchRoot->change = false;
   SearchRoot->easy = false;
   SearchRoot->flag = false;
   SearchRoot->research = false;

   // SearchCurrent

   for (thread = 0; thread < ThreadMax; thread++) {
      SearchCurrent->max_depth[thread] = 0;
      SearchCurrent->node_nb[thread] = 0;
   }

   SearchCurrent->time = 0.0;
   SearchCurrent->speed = 0.0;
   SearchCurrent->cpu = 0.0;

   search_clear_best(true);

   // SearchDisp

   if (DispSearch) {
      UseEvent = true;
      DispBest = true;
      DispDepthStart = true;
      DispDepthEnd = true;
      DispRoot = true;
      DispStat = true;
   } else {
      UseEvent = false;
      DispBest = false;
      DispDepthStart = false;
      DispDepthEnd = false;
      DispRoot = false;
      DispStat = false;
   }
}

// search_clear_best()

void search_clear_best(bool all) {

   int move_nb;

   ASSERT(all==true||all==false);

   // SearchBest

   if (all) {
      move_nb = 0;
   } else {
      move_nb = option_get_int("MultiPV");
   }

   for ( ; move_nb < MultiMax; move_nb++) {
      SearchBest[move_nb].move = MoveNone;
      SearchBest[move_nb].value = ValueNone;
      SearchBest[move_nb].depth = 0;
      SearchBest[move_nb].flags = SearchUnknown;
      SearchBest[move_nb].time = 0.0;
      PV_CLEAR(SearchBest[move_nb].pv);
   }
}

// search()

void search() {

   int move;
   int move_nb;
   int depth;

   ASSERT(board_is_ok(SearchInput->board));

   // opening book

   if (option_get_bool("OwnBook") && !SearchInput->infinite) {

      move = book_move(SearchInput->board);

      if (move != MoveNone) {

         // play book move

         SearchBest->move = move;
         SearchBest->value = 1;
         SearchBest->flags = SearchExact;
         SearchBest->depth = 1;
         SearchBest->pv[0] = move;
         SearchBest->pv[1] = MoveNone;

         SearchRoot->move_nb = 1;

         search_update_best();

         return;
      }
   }

   // SearchInput

   gen_legal_moves(SearchInput->list,SearchInput->board);

   if (LIST_SIZE(SearchInput->list) <= 1) {
      SearchInput->depth_is_limited = true;
      SearchInput->depth_limit = 4; // was 1
   }

   // SearchRoot

   list_copy(SearchRoot->list,SearchInput->list);

   // SearchBest

   move_nb = option_get_int("MultiPV");

   // SearchCurrent

   board_copy(SearchCurrent->board,SearchInput->board);
   my_timer_reset(SearchCurrent->timer);
   my_timer_start(SearchCurrent->timer);

   // init

   trans_inc_date();

   sort_init();
   search_full_init(SearchRoot->list,SearchCurrent->board);

   // thread

   smp_wakeup();

   // iterative deepening

   for (depth = 1; depth < DepthMax; depth++) {

      if (DispDepthStart) send("info depth %d",depth);

      SearchRoot->bad_1 = false;
      SearchRoot->change = false;
      SearchRoot->research = false;

      board_copy(SearchCurrent->board,SearchInput->board);

      if (move_nb > 1) {
         search_full_root(SearchRoot->list,SearchCurrent->board,depth,SearchMulti);
      } else {
         if (UseShortSearch && depth <= ShortSearchDepth) {
            search_full_root(SearchRoot->list,SearchCurrent->board,depth,SearchShort);
         } else {
            search_full_root(SearchRoot->list,SearchCurrent->board,depth,SearchNormal);
         }
      }

      search_update_current();

      if (DispDepthEnd) search_update_depth(depth);

      // search info

      if (depth >= 1) SearchInfo->can_stop = true;

      if (SearchInfo->search_stop) {
         ASSERT(SearchBest->move!=MoveNone);
         break;
      }

      if (depth == 1
       && LIST_SIZE(SearchRoot->list) >= 2
       && LIST_VALUE(SearchRoot->list,0) >= LIST_VALUE(SearchRoot->list,1) + EasyThreshold) {
         SearchRoot->easy = true;
      }

      if (UseBad && depth > 1) {
         SearchRoot->bad_2 = SearchRoot->bad_1;
         SearchRoot->bad_1 = false;
         ASSERT(SearchRoot->bad_2==(SearchBest->value<=SearchRoot->last_value-BadThreshold));
      }

      SearchRoot->last_value = SearchBest->value;

      // stop search?

      if (SearchInput->depth_is_limited
       && depth >= SearchInput->depth_limit) {
         SearchRoot->flag = true;
      }

      if (SearchInput->time_is_limited
       && SearchCurrent->time >= SearchInput->time_limit_1
       && !SearchRoot->bad_2) {
         SearchRoot->flag = true;
      }

      if (SearchInput->nodes_is_limited
       && search_update_node_nb() >= SearchInput->nodes) {
          SearchRoot->flag = true;
      }

      if (UseEasy
       && SearchInput->time_is_limited
       && SearchCurrent->time >= SearchInput->time_limit_1 * EasyRatio
       && SearchRoot->easy) {
         ASSERT(!SearchRoot->bad_2);
         ASSERT(!SearchRoot->change);
         SearchRoot->flag = true;
      }

      if (UseEarly
       && SearchInput->time_is_limited
       && SearchCurrent->time >= SearchInput->time_limit_1 * EarlyRatio
       && !SearchRoot->bad_2
       && !SearchRoot->change) {
         SearchRoot->flag = true;
      }

      if (SearchInfo->can_stop
       && (SearchInfo->stop || (SearchRoot->flag && !SearchInput->infinite))) {
         break;
      }
   }

   // thread

   smp_sleep();
}

// search_update_node_nb()

sint64 search_update_node_nb() {

   sint64 node_nb;
   int thread;

   for (node_nb = 0, thread = 0; thread < ThreadMax; thread++) {
      node_nb += SearchCurrent->node_nb[thread];
   }

   ASSERT(node_nb>=0);
   return node_nb;
}

// search_update_max_depth()

int search_update_max_depth() {

   int max_depth;
   int thread;

   for (max_depth = 0, thread = 0; thread < ThreadMax; thread++) {
      if (SearchCurrent->max_depth[thread] > max_depth) {
         max_depth = SearchCurrent->max_depth[thread];
      }
   }

   ASSERT(height_is_ok(max_depth));
   return max_depth;
}

// search_update_best()

void search_update_best() {

   int move_nb, i;
   int move, value, flags, depth, max_depth;
   const mv_t * pv;
   double time;
   sint64 node_nb;
   int mate;
   char move_string[256], pv_string[512];

   search_update_current();
   search_update_sort();

   if (DispBest) {

      move_nb = option_get_int("MultiPV");

      for (i = 0; i < move_nb; i++) {

         move = SearchBest[i].move;
         value = SearchBest[i].value;
         flags = SearchBest[i].flags;
         depth = SearchBest[i].depth;
         node_nb = SearchBest[i].node_nb;
         time = SearchBest[i].time;
         pv = SearchBest[i].pv;

         max_depth = search_update_max_depth();

         move_to_string(move,move_string,256);
         pv_to_string(pv,pv_string,512);

         mate = value_to_mate(value);

         ASSERT(flags==SearchExact||move_nb==1);

         if (mate == 0) {

            // normal evaluation

            if (false) {
            } else if (flags == SearchExact) {
               send("info multipv %d depth %d seldepth %d score cp %d time %.0f nodes " S64_FORMAT " pv %s",i+1,depth,max_depth,value,time*1000.0,node_nb,pv_string);
            } else if (flags == SearchLower) {
               send("info multipv %d depth %d seldepth %d score cp %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",i+1,depth,max_depth,value,time*1000.0,node_nb,pv_string);
            } else if (flags == SearchUpper) {
               send("info multipv %d depth %d seldepth %d score cp %d upperbound time %.0f nodes " S64_FORMAT " pv %s",i+1,depth,max_depth,value,time*1000.0,node_nb,pv_string);
            }

         } else {

            // mate announcement

            if (false) {
            } else if (flags == SearchExact) {
               send("info multipv %d depth %d seldepth %d score mate %d time %.0f nodes " S64_FORMAT " pv %s",i+1,depth,max_depth,mate,time*1000.0,node_nb,pv_string);
            } else if (flags == SearchLower) {
               send("info multipv %d depth %d seldepth %d score mate %d lowerbound time %.0f nodes " S64_FORMAT " pv %s",i+1,depth,max_depth,mate,time*1000.0,node_nb,pv_string);
            } else if (flags == SearchUpper) {
               send("info multipv %d depth %d seldepth %d score mate %d upperbound time %.0f nodes " S64_FORMAT " pv %s",i+1,depth,max_depth,mate,time*1000.0,node_nb,pv_string);
            }
         }
      }
   }

   // update time-management info

   if (UseBad && SearchBest->depth > 1) {
      if (SearchBest->value <= SearchRoot->last_value - BadThreshold) {
         SearchRoot->bad_1 = true;
         SearchRoot->easy = false;
         SearchRoot->flag = false;
      } else {
         SearchRoot->bad_1 = false;
      }
   }
}

// search_update_root()

void search_update_root() {

   int move, move_pos;
   char move_string[256];

   if (DispRoot) {

      search_update_current();

      if (SearchCurrent->time >= 1.0) {

         move = SearchRoot->move;
         move_pos = SearchRoot->move_pos;

         move_to_string(move,move_string,256);

         send("info currmove %s currmovenumber %d",move_string,move_pos+1);
      }
   }
}

// search_update_current()

void search_update_current() {

   my_timer_t *timer;
   sint64 node_nb;
   double time, speed, cpu;

   timer = SearchCurrent->timer;

   node_nb = search_update_node_nb();
   time = (UseCpuTime) ? my_timer_elapsed_cpu(timer) : my_timer_elapsed_real(timer);
   speed = (time >= 1.0) ? double(node_nb) / time : 0.0;
   cpu = my_timer_cpu_usage(timer);

   SearchCurrent->time = time;
   SearchCurrent->speed = speed;
   SearchCurrent->cpu = cpu;
}

// search_check()

void search_check() {

   search_send_stat();

   if (UseEvent) event();

   if (SearchInput->depth_is_limited
    && SearchRoot->depth > SearchInput->depth_limit) {
      SearchRoot->flag = true;
   }

   if (SearchInput->nodes_is_limited
    && SearchRoot->depth > 1
    && search_update_node_nb() >= SearchInput->nodes) {
      SearchRoot->flag = true;
   }

   if (SearchInput->time_is_limited) {

      if (SearchCurrent->time >= SearchInput->time_limit_2) {
         SearchRoot->flag = true;
      } else if (SearchCurrent->time >= SearchInput->time_limit_3
       && SearchRoot->research) {
         SearchRoot->flag = true;
      } else if (SearchCurrent->time >= SearchInput->time_limit_1
       && !SearchRoot->bad_1
       && !SearchRoot->bad_2
       && !SearchRoot->research
       && (!UseExtension || SearchRoot->move_pos == 0)) {
         SearchRoot->flag = true;
      }
   }

   if (SearchInfo->can_stop
    && (SearchInfo->stop || (SearchRoot->flag && !SearchInput->infinite))) {
      SearchInfo->search_stop = true;
   }
}

// search_send_stat()

static void search_send_stat() {

   double time, speed, cpu;
   sint64 node_nb, tb_hit;

   search_update_current();

   if (DispStat && SearchCurrent->time >= SearchInfo->last_time + 1.0) { // at least one-second gap

      SearchInfo->last_time = SearchCurrent->time;

      time = SearchCurrent->time;
      speed = SearchCurrent->speed;
      cpu = SearchCurrent->cpu;
      node_nb = search_update_node_nb();
      tb_hit = tb_stats() + egbb_stats();

      send("info time %.0f nodes " S64_FORMAT " nps %.0f tbhits " S64_FORMAT " cpuload %.0f",time*1000.0,node_nb,speed,tb_hit,cpu*1000.0);

      trans_stats();
   }
}

// search_update_depth()

static void search_update_depth(int depth) {

   int max_depth;
   double time, speed;
   sint64 node_nb, tb_hit;

   ASSERT(depth_is_ok(depth));

   time = SearchCurrent->time;
   speed = SearchCurrent->speed;
   node_nb = search_update_node_nb();
   max_depth = search_update_max_depth();
   tb_hit = tb_stats() + egbb_stats();

   send("info depth %d seldepth %d time %.0f nodes " S64_FORMAT " nps %.0f tbhits " S64_FORMAT, depth,max_depth,time*1000.0,node_nb,speed,tb_hit);
}

// search_update_sort()

static void search_update_sort() {

   int size;
   int i, j;
   search_best_t swap;

   ASSERT(SearchRoot->move_nb>0);

   // init

   size = SearchRoot->move_nb;
   SearchBest[size].value = -32768; // HACK: sentinel
   SearchBest[size].depth = -128;

   // insert sort (stable)

   for (i = size-1; i >= 0; i--) {

      swap = SearchBest[i];

      for (j = i; swap.value < SearchBest[j+1].value && swap.depth <= SearchBest[j+1].depth; j++) {
         SearchBest[j] = SearchBest[j+1];
      }

      ASSERT(j<size);

      SearchBest[j] = swap;
   }

   // debug

   if (DEBUG) {
      for (i = 0; i < size-1; i++) {
         ASSERT(SearchBest[i].value>=SearchBest[i+1].value||
                SearchBest[i].depth>SearchBest[i+1].depth);
      }
   }
}

// end of search.cpp

