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

// test.cpp

// includes

#include <cstring>

#include "board.h"
#include "egbb.h"
#include "engine.h"
#include "eval.h"
#include "fen.h"
#include "list.h"
#include "material.h"
#include "move_do.h"
#include "move_gen.h"
#include "option.h"
#include "pawn.h"
#include "protocol.h"
#include "pv.h"
#include "recog.h"
#include "search.h"
#include "search_full.h"
#include "smp.h"
#include "sort.h"
#include "tb.h"
#include "test.h"
#include "thread.h"
#include "trans.h"
#include "value.h"
#include "util.h"

// constants

static const bool DispSetup = true;
static const bool DispMatch = true;
static const bool DispGame = true;

static const int BenchMax = 256;
static const int FenMax = 16;
static const int SuiteMax = 47;

static const char * UCIPosStart = "position startpos moves ";

static const char * TestFen[FenMax] = {
   "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
   "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -",
   "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -",
   "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
   "rq3rk1/ppp2ppp/1bnpb3/3N2B1/3NP3/7P/PPPQ1PP1/2KR3R w - - 7 14",
   "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4Pp2/1BNP4/PPP2PPP/3R1RK1 w - - 2 14",
   "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
   "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
   "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
   "4r1k1/r1q2ppp/ppp2n2/4P3/5Rb1/1N1BQ3/PPP3PP/R5K1 w - - 1 17",
   "2rqkb1r/ppp2p2/2npb1p1/1N1Nn2p/2P1PP2/8/PP2B1PP/R1BQK2R b KQ - 0 11",
   "r1bq1r1k/b1p1npp1/p2p3p/1p6/3PP3/1B2NN2/PP3PPP/R2Q1RK1 w - - 1 16",
   "3r1rk1/p5pp/bpp1pp2/8/q1PP1P2/b3P3/P2NQRPP/1R2B1K1 b - - 6 22",
   "r1q2rk1/2p1bppp/2Pp4/p6b/Q1PNp3/4B3/PP1R1PPP/2K4R w - - 2 18",
   "4k2r/1pb2ppp/1p2p3/1R1p4/3P4/2r1PN2/P4PPP/1R4K1 b - - 3 22",
   "3q2k1/pb3p1p/4pbp1/2r5/PpN2N2/1P2P2P/5PP1/Q2R2K1 b - - 4 26"
};

static const char * TestSuite[SuiteMax] = {
   "c2c4 c7c5 g1f3 g8f6 g2g3 b7b6 f1g2 c8b7 b1c3 e7e6 e1g1 f8e7 d2d4 c5d4 d1d4 d7d6 f1d1 a7a6",
   "c2c4 e7e5 b1c3 g8f6 g1f3 b8c6",
   "d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 d5c4 a2a4 c8f5",
   "d2d4 d7d5 c2c4 e7e6 b1c3 c7c5 c4d5 e6d5 g1f3 b8c6 g2g3 g8f6 f1g2 f8e7 e1g1 e8g8 c1g5",
   "d2d4 d7d5 c2c4 c7c6 g1f3 g8f6 b1c3 e7e6 e2e3 b8d7 f1d3",
   "d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 c1g5 f8e7 e2e3 e8g8 g1f3 h7h6 g5h4 b7b6",
   "d2d4 f7f5 g2g3 g8f6 f1g2 e7e6 c2c4 d7d5 g1f3 c7c6 e1g1 f8d6 b2b3 d8e7",
   "d2d4 g8f6 c2c4 c7c5 d4d5 b7b5 c4b5 a7a6 b5a6 g7g6 b1c3 c8a6",
   "d2d4 g8f6 c2c4 c7c5 d4d5 e7e6 b1c3 e6d5 c4d5 d7d6 e2e4 g7g6",
   "d2d4 g8f6 c2c4 g7g6 g2g3 f8g7 f1g2 e8g8 g1f3 d7d5 c4d5 f6d5 e1g1",
   "d2d4 g8f6 c2c4 g7g6 b1c3 d7d5 c4d5 f6d5 e2e4 d5c3 b2c3 f8g7 f1c4 c7c5 g1e2",
   "d2d4 g8f6 c2c4 e7e6 g1f3 d7d5 g2g3 f8e7 f1g2 e8g8",
   "d2d4 g8f6 c2c4 e7e6 g1f3 f8b4 c1d2 d8e7 g2g3",
   "d2d4 g8f6 c2c4 e7e6 g1f3 b7b6 g2g3",
   "d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 d1c2 e8g8 a2a3 b4c3 c2c3",
   "d2d4 g8f6 c2c4 e7e6 b1c3 f8b4 e2e3 e8g8 f1d3 d7d5 g1f3 c7c5 e1g1",
   "d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 g2g3 e8g8 f1g2 d7d6 g1f3 b8d7 e1g1 e7e5 e2e4",
   "d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 f2f3 e8g8 c1e3 e7e5 g1e2 c7c6",
   "d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 g1f3 e8g8 f1e2 e7e5 e1g1 b8c6 d4d5 c6e7 f3e1 f6d7 c1e3 f7f5 f2f3 f5f4 e3f2 g6g5",
   "e2e4 d7d5 e4d5 d8d5 b1c3 d5a5 d2d4 g8f6 g1f3 c7c6 f1c4",
   "e2e4 g8f6 e4e5 f6d5 d2d4 d7d6 g1f3",
   "e2e4 d7d6 d2d4 g8f6 b1c3 g7g6 g1f3 f8g7 f1e2 e8g8 e1g1 c7c6",
   "e2e4 d7d6 d2d4 g8f6 b1c3 g7g6 f2f4 f8g7 g1f3 e8g8",
   "e2e4 c7c6 d2d4 d7d5 e4e5 c8f5 g1f3 e7e6 f1e2 c6c5",
   "e2e4 c7c6 d2d4 d7d5 e4d5 c6d5 c2c4 g8f6 b1c3 e7e6 g1f3",
   "e2e4 c7c6 d2d4 d7d5 b1c3 d5e4 c3e4 c8f5 e4g3 f5g6 h2h4 h7h6 g1f3 b8d7 h4h5 g6h7 f1d3 h7d3 d1d3",
   "e2e4 c7c5 c2c3 d7d5 e4d5 d8d5 d2d4 g8f6 g1f3 e7e6",
   "e2e4 c7c5 b1c3 b8c6 g2g3 g7g6 f1g2 f8g7 d2d3 d7d6 f2f4 e7e6 g1f3 g8e7 e1g1 e8g8",
   "e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g8f6 b1c3 e7e5 d4b5 d7d6 c1g5 a7a6 b5a3 b7b5 g5f6 g7f6 c3d5 f6f5",
   "e2e4 c7c5 g1f3 b8c6 d2d4 c5d4 f3d4 g7g6 c2c4 g8f6 b1c3 d7d6 f1e2",
   "e2e4 c7c5 g1f3 e7e6 d2d4 c5d4 f3d4 b8c6 b1c3 a7a6",
   "e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 b8c6 c1g5 e7e6 d1d2 a7a6 e1c1 h7h6",
   "e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 g7g6 c1e3 f8g7 f2f3 e8g8 d1d2 b8c6 f1c4 c8d7 e1c1 c6e5 c4b3 a8c8",
   "e2e4 e7e6 d2d4 d7d5 e4e5 c7c5 c2c3 b8c6 g1f3 d8b6 a2a3 c5c4",
   "e2e4 e7e6 d2d4 d7d5 b1d2 g8f6 e4e5 f6d7 c2c3 c7c5 f1d3 b8c6",
   "e2e4 e7e6 d2d4 d7d5 b1c3 f8b4 e4e5 c7c5 a2a3 b4c3 b2c3 g8e7",
   "e2e4 e7e5 g1f3 d7d6 d2d4 e5d4 f3d4 g8f6 b1c3 f8e7",
   "e2e4 e7e5 g1f3 g8f6 f3e5 d7d6 e5f3 f6e4 d2d4 d6d5 f1d3",
   "e2e4 e7e5 g1f3 b8c6 d2d4 e5d4 f3d4",
   "e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 d2d3 g8f6 c2c3 d7d6 b2b4 c5b6 a2a4 a7a6",
   "e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5c6 d7c6 d2d4 e5d4 d1d4 d8d4 f3d4",
   "e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 b7b5 a4b3 f8c5",
   "e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f6e4 d2d4 b7b5 a4b3 d7d5 d4e5 c8e6",
   "e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5 a4b3 d7d6 c2c3 e8g8 h2h3 c8b7 d2d4 f8e8 b1d2",
   "f2f4 d7d5 g1f3 g8f6 b2b3 g7g6 c1b2 f8g7 e2e3 e8g8 f1e2 c7c5 e1g1 b8c6 f3e5",
   "g1f3 g8f6 d2d4 d7d5 e2e3 e7e6 c2c3 c7c5 f1d3",
   "g1f3 g8f6 g2g3 g7g6 f1g2 f8g7 e1g1 e8g8 d2d3 d7d6 e2e4",
};

static const option_t TestSetup[] = { // example

   { "Null Fail",             "true" },
   { "Fail Depth",            "4"    },
   { "LMR Pruning",           "true" },
   { "LMR Pruning Depth",     "4"    },
   { "LMR Pruning Threshold", "0"    },
   { "Delta-Delta Pruning",   "true" },
   { NULL, NULL }
};

// types

struct bench_info_t {
   search_best_t best[1];
   double speed;
};

struct match_info_t {
   int depth;
   int nodes;
   int time;
   int inc;
   int score;
   int draw;
   int tot;
   int level;
};

struct game_info_t {
   double time[EngineNb]; // sec
   double inc;
   int value;
};

// variables

bench_info_t BenchInfo[BenchMax];
match_info_t MatchInfo[1];

// prototypes

static void bench_clear      ();
static void bench_disp       ();

static void match_clear      ();
static void match_disp       ();

static void search_perft     (board_t * board, int depth);
static void search_bench     (board_t * board);

static void match_game_play  (const char * string, board_t * board);

static void game_clear       (game_info_t * game);
static void game_setup       (game_info_t * game, int player);

static bool game_is_finished (game_info_t * game, int player);
static bool game_is_draw     (game_info_t * game);

static void test_clear       ();

// functions

// perft()

void perft(int depth) {

   int i;
   board_t board[1];

   ASSERT(depth>0&&depth<DepthMax);

   // init

   search_clear();
   board_copy(board,SearchInput->board);

   my_timer_reset(SearchCurrent->timer);
   my_timer_start(SearchCurrent->timer);

   // iterative deepening

   for (i = 1; i <= depth; i++) {

      search_perft(board,i);
      search_update_current();

      send("info depth %d time %.0f nodes " S64_FORMAT " nps %.0f", i,SearchCurrent->time*1000.0,SearchCurrent->node_nb,SearchCurrent->speed);
   }
}

void bench(int depth) {

   int pos;
   board_t board[1];

   ASSERT(depth>0&&depth<DepthMax);

   // init

   bench_clear();
   search_disp(false);

   // thread

   smp_wakeup();

   // fen loop

   for (pos = 0; pos < FenMax; pos++) {

      // init

      test_clear();

      SearchInput->depth_is_limited = true;
      SearchInput->depth_limit = depth;

      ASSERT(TestFen[pos]!=NULL);
      board_from_fen(board,TestFen[pos]);

      // search

      search_bench(board);

      BenchInfo[pos].best[0] = SearchBest[0];
      BenchInfo[pos].speed = SearchCurrent->speed;
   }

   // thread

   smp_sleep();

   // info

   bench_disp();
   search_disp(true);
}

void match(int depth, int node_nb, int time, int inc) {

   int pos;
   const char * moves;
   const option_t * opt_std, * opt_test;
   char string[32768];
   board_t board[1];

   ASSERT(depth_is_ok(depth));
   ASSERT(node_nb>=0);
   ASSERT(time>=0);
   ASSERT(inc>=0);

   // init

   search_disp(false);

   match_clear();

   MatchInfo->depth = depth;
   MatchInfo->nodes = node_nb;
   MatchInfo->time = time;
   MatchInfo->inc = inc;

   board_from_fen(board,TestFen[0]);

   // setup

   engine_set_type(ENGINE_TEST);

   for (opt_test = &TestSetup[0]; opt_test->var != NULL; opt_test++) {
      option_set(opt_test->var,opt_test->val);
   }

   if (DispSetup) {

      opt_std =  engine_get_opt(ENGINE_DEFAULT);
      opt_test = engine_get_opt(ENGINE_TEST);

      for ( ; opt_std->var != NULL; opt_std++, opt_test++) {
         if (!my_string_equal(opt_std->val,opt_test->val)) {
           send("EngineDefault [var = %s val = %s]", opt_std->var, opt_std->val);
           send("EngineTest    [var = %s val = %s]", opt_test->var, opt_test->val);
         }
      }
   }

   // opening loop

   for (pos = 0; pos < SuiteMax; pos++) {

      // disp

      if (DispMatch) {
         send("Running game %d of %d (win = %d draw = %d)",
            MatchInfo->tot,SuiteMax*2,MatchInfo->score,MatchInfo->draw);
      }

      // init

      memset(string,0,sizeof(string));

      moves = TestSuite[pos];
      ASSERT(moves!=NULL);

      // moves

      strcat(string,UCIPosStart);
      strcat(string,moves);
      strcat(string,"\n");

      match_game_play(string,board);
   }

   match_disp();
   search_disp(true);
}

// bench_clear()

static void bench_clear() {

   int i;

   for (i = 0; i < BenchMax; i++) {

      BenchInfo[i].best->move = MoveNone;
      BenchInfo[i].best->value = 0;
      BenchInfo[i].best->flags = SearchUnknown;
      BenchInfo[i].best->depth = 0;
      BenchInfo[i].best->node_nb = 0;
      BenchInfo[i].best->time = 0.0;
      PV_CLEAR(BenchInfo[i].best->pv);

      BenchInfo[i].speed = 0.0;
   }
}

// bench_disp()

static void bench_disp() {

   int i;
   int move, value, depth;
   const mv_t * pv;
   double time, speed;
   double time_all, speed_all;
   char move_string[256], pv_string[512];

   // init

   time_all = 0.0;
   speed_all = 0.0;

   send("");
   send("*** Benchmark result ***");
   send("");

   // fen loop

   for (i = 0; i < 16; i++) {

      move  = BenchInfo[i].best->move;
      value = BenchInfo[i].best->value;
      depth = BenchInfo[i].best->depth;
      pv    = BenchInfo[i].best->pv;
      time  = BenchInfo[i].best->time;
      speed = BenchInfo[i].speed;

      move_to_string(move,move_string,256);
      pv_to_string(pv,pv_string,512);

      send("Fen<%d> depth %d time %.2fs speed %.0f nps score %d move %s pv %s",i+1,depth,time,speed,value,move_string,pv_string);

      time_all += time;
      speed_all += speed;
   }

   send("");
   send("Overall time %.2fs speed %.0f nps", time_all,speed_all/(i-1));
}

// match_clear()

static void match_clear() {

   MatchInfo->depth = 0;
   MatchInfo->nodes = 0;
   MatchInfo->time = 0;
   MatchInfo->inc = 0;
   MatchInfo->draw = 0;
   MatchInfo->score = 0;
   MatchInfo->tot = 0;
   MatchInfo->level = 0;
}

// match_disp()

static void match_disp() {

   send("TestEngine: win = %d draw = %d \n",MatchInfo->score,MatchInfo->draw);
}

// search_perft()

static void search_perft(board_t * board, int depth) {

   int i, move;
   list_t list[1];
   undo_t undo[1];

   ASSERT(board_is_ok(board));
   ASSERT(depth_is_ok(depth));

   // init

   SearchCurrent->node_nb[ThreadMain]++;

   // horizon?

   if (depth == 0) return;

   // move generation

   gen_legal_moves(list,board);

   // move loop

   for (i = 0; i < LIST_SIZE(list); i++) {

      move = LIST_MOVE(list,i);

      move_do(board,move,undo);
      search_perft(board,depth-1);
      move_undo(board,move,undo);
   }
}

// search_bench()

static void search_bench(board_t * board) {

   int depth;
   list_t list[1];

   ASSERT(board!=NULL);

   // init

   sort_init();

   gen_legal_moves(list,board);

   list_copy(SearchRoot->list,list);
   board_copy(SearchCurrent->board,board);

   search_full_init(SearchRoot->list,board);

   trans_inc_date();

   my_timer_reset(SearchCurrent->timer);
   my_timer_start(SearchCurrent->timer);

   // iterative deepening

   for (depth = 1; depth <= SearchInput->depth_limit; depth++) {

      if (depth <= 1) {
         search_full_root(SearchRoot->list,SearchCurrent->board,depth,SearchShort);
      } else {
         search_full_root(SearchRoot->list,SearchCurrent->board,depth,SearchNormal);
      }

      search_update_current();
   }

   my_timer_stop(SearchCurrent->timer);
}

// match_game_play()

static void match_game_play(const char * string, board_t * board) {

   int colour;
   int player;
   int move;
   game_info_t game_info[1];
   undo_t undo[1];

   ASSERT(string!=NULL);
   ASSERT(board!=NULL);

   for (colour = 0; colour < ColourNb; colour++) {

      // init

      player = colour;

      game_clear(game_info);
      board_copy(SearchInput->board,board);

      // opening

      loop_step(string);

      while(!game_is_finished(game_info,player)) {

         if (DispGame) {
            printf("Game %d of %d move %d \r", MatchInfo->tot+1,SuiteMax*2,SearchInput->board->sp/2);
         }

         player ^= 1;
         engine_set_type(player);

         test_clear(); // TODO: Use seperate hash tables
         game_setup(game_info,player);

         eval_init_options();

         // search

         search();

         move = SearchBest->move;
         game_info->value = SearchBest->value;

         if (MatchInfo->time > 0) {
            game_info->time[player] -= SearchCurrent->time;
            game_info->time[player] += game_info->inc;
         }

         move_do(SearchInput->board,move,undo);
      }

      // update

      MatchInfo->tot++;

      if (false) {
      } else if (game_info->value > 0) {
         MatchInfo->score += (colour == 0 ? +1 : -1);
      } else if (game_info->value < 0) {
         MatchInfo->score += (colour == 0 ? -1 : +1);
      } else {
         MatchInfo->draw++;
      }
   }
}

// game_clear()

static void game_clear(game_info_t * game) {

   int colour;

   ASSERT(game!=NULL);

   for (colour = 0; colour < ColourNb; colour++) {
      game->time[colour] = double(MatchInfo->time);
   }

   game->inc = double(MatchInfo->inc);
   game->value = 0;
}

// game_setup()

static void game_setup(game_info_t * game, int player) {

   ASSERT(game!=NULL);
   ASSERT(player==ENGINE_DEFAULT||player==ENGINE_TEST);

   if (false) {

   } else if (MatchInfo->depth > 0) {

      SearchInput->depth_is_limited = true;
      SearchInput->depth_limit = MatchInfo->depth;

   } else if (MatchInfo->nodes > 0) {

      SearchInput->nodes_is_limited = true;
      SearchInput->nodes = MatchInfo->nodes;

   } else if (MatchInfo->time > 0) {

      SearchInput->time_is_limited = true;
      time_allocation(game->time[player],game->inc,30);

   } else {

      ASSERT(false);
   }
}

// game_is_finished()

static bool game_is_finished(game_info_t * game, int player) {

   ASSERT(game!=NULL);
   ASSERT(player==ENGINE_DEFAULT||player==ENGINE_TEST);

   ASSERT(board_is_ok(SearchInput->board));
   ASSERT(board_is_legal(SearchInput->board));

   // mate

   if (board_is_mate(SearchInput->board)) {
      ASSERT(value_is_mate(game->value));
      return true;
   }

   // stalemate

   if (board_is_stalemate(SearchInput->board)) {
      ASSERT(game->value==0);
      return true;
   }

   // draw

   if (game_is_draw(game)) {
      game->value = 0;
      return true;
   }

   // time

   if (game->time[player] < 0) {

      // TODO: do more!

      send("game lost on time");
      game->value = 0; // HACK
      return true;
   }

   // TODO: Add score feature

   return false;
}

// game_is_draw()

static bool game_is_draw(game_info_t * game) {

   ASSERT(game!=NULL);

   if (board_is_repetition(SearchInput->board)) return true;
   if (recog_draw(SearchInput->board,ThreadMain)) return true;

   return false;
}

// test_clear()

static void test_clear() {

   pawn_clear();
   search_clear();
   material_clear();
   trans_clear();
   tb_clear();
   egbb_clear();
};

// end of test.cpp
