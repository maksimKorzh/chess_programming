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

// search_full.cpp

// includes

#include <cmath> // for sqrt()

#include "attack.h"
#include "bit.h"
#include "board.h"
#include "colour.h"
#include "egbb.h"
#include "eval.h"
#include "list.h"
#include "move.h"
#include "move_check.h"
#include "move_do.h"
#include "mutex.h"
#include "option.h"
#include "pawn.h"
#include "piece.h"
#include "pst.h"
#include "pv.h"
#include "recog.h"
#include "search.h"
#include "search_full.h"
#include "see.h"
#include "smp.h"
#include "sort.h"
#include "tb.h"
#include "thread.h"
#include "trans.h"
#include "util.h"
#include "value.h"

// constants and variables

static const bool UseDebug = false;

// root search

static bool UseAspiration = true;
static const bool UseFailLow = true;
static int AspirationDepth = 4;
static int AspirationWindow = 16;
static const int WidthFactor = 2;
static const int WidthStart = 1;

static int MoveNb = 1;

// main search

static const bool UseDistancePruning = true;

// draw value

static int ValueDraw = 0;

// transposition table

static const bool UseMateValues = true; // use mate values from shallower searches?

// endgame tablebase

static bool UseTB = true;
static int TBDepth = 8 * HeightFull;
static int TBpieceNb = 0;

static bool UseBB = false;
static int BBDepth = 4 * HeightFull;
static int BBHeight = 0;
static int BBpieceNb = 0;

// razor pruning

static const bool UseRazor = true;
static const int RazorDepth = 4 * HeightFull;
static const int RazorMargin[8] = { 0, 100, 200, 300, 400, 500, 600, 700 };

// static null move pruning

static const bool UseStaticNull = true;
static const int StaticNullDepth = 4 * HeightFull;
static const int StaticNullMargin[8] = { 0, 100, 200, 300, 400, 500, 600, 700 };

// null move

static const bool UseNull = true;
static const int NullDepth = 2 * HeightFull;
static const int NullReduction = 3 * HeightFull;

static const bool UseNullTune = true;
static const int NullMargin = 100;

static const bool UseVer = true;
static const int VerReduction = 5 * HeightFull;

static const bool UseFail = false;
static const int FailDepth = 4 * HeightFull;

// move ordering

static const bool UseIID = true;
static const int IIDDepth = 3 * HeightFull;
static const int IIDReduction = 2 * HeightFull;

// extensions

static const bool ExtendSingleReply = true;
static const bool ExtendPawnEndgame = true;

// LMR

static const bool UseLMR = true;
static const int LMRDepthMax = DepthMax * HeightFull;
static uint8 LMRList[NodeNb][LMRDepthMax][ListSize];

static const bool UseLMRPruning = true;
static const int LMRDepth = 4 * HeightFull;
static const int LMRvalue = 0;

static const bool LMRReSearch = true;

// futility pruning

static const bool UseFutility = true;
static const int FutilityDepth = 5 * HeightFull;
static const int FutilityMargin[16] = {
   0, 0, 0, 0, 50, 75, 100, 200, 300, 450, 600, +ValueInf, +ValueInf, +ValueInf, +ValueInf, +ValueInf
};

// thread

static const bool UseThread = true;
static const int ThreadDepth = 4 * HeightFull;
static int ThreadNb = 1;

// quiescence search

static const bool UseTransQS = true;

static const bool UseDelta = true;
static const int DeltaMargin = 50;

static const bool UseDeltaDelta = false;

// macros

#define NODE_OPP(type)               (-(type))
#define NODE_FLAG(type)              (ABS(type))

#define DEPTH_MATCH(d1,d2)           ((d1)>=(d2))
#define DEPTH_NEW(d1,d2,d3)          ((d1)>=3*HeightFull?MAX((d2)-(d3),HeightFull):(d2))

#define MOVE_IS_PRIME(value)         ((value)>32768)
#define MOVE_IS_KILLER(value)        ((value)==32768)

// prototypes

static int  full_root            (list_t * list, board_t * board, int alpha, int beta, int depth, int height, int search_type);

static int  full_search          (board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int node_type, int thread, bool null_move);
static int  full_quiescence      (board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int node_type, int thread);

static int  full_new_depth       (int depth, int move, board_t * board, bool single_reply, bool in_pv);
static int  full_next_move       (board_t * board, split_t * split, int * move_value, int thread);

static bool do_null              (const board_t * board);

static void pv_fill              (const mv_t pv[], board_t * board);

static bool move_is_dangerous    (int move, const board_t * board);
static bool move_is_neighbour    (int move, const board_t * board);
static bool capture_is_dangerous (int move, const board_t * board);

static bool simple_stalemate     (const board_t * board);

static int  tune_eval            (int trans_min_value, int trans_max_value, int alpha, int beta, int height, int static_eval);

static bool pawn_is_passed       (int move, const board_t * board);
static bool pawn_endgame         (int move, const board_t * board);

// functions

// search_full_init()

void search_full_init(list_t * list, board_t * board) {

   int depth, move_nb;
   int trans_move, trans_min_depth, trans_max_depth, trans_min_value, trans_max_value, trans_eval;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));

   // init

   for (depth = 0; depth < LMRDepthMax; depth++) {

      for (move_nb = 0; move_nb < ListSize; move_nb++) {

        LMRList[0][depth][move_nb] = uint8((sqrt(double(depth-1)) + sqrt(double(move_nb-1))) * 2 / 3);
        LMRList[1][depth][move_nb] = uint8( sqrt(double(depth-1)) + sqrt(double(move_nb-1)));
      }
   }

   // aspiration search options

   UseAspiration = option_get_bool("Aspiration Search");
   AspirationDepth = option_get_int("Aspiration Depth");
   AspirationWindow = option_get_int("Aspiration Window");

   // search best options

   MoveNb = option_get_int("MultiPV");

   // thread options

   ThreadNb = option_get_int("Threads");

   // contempt options

   ValueDraw = option_get_int("Contempt Factor");

   // endgame tablebase options

   UseTB = option_get_bool("EGTB");
   TBDepth = option_get_int("EGTB Depth");
   TBpieceNb = tb_piece_nb();

   UseBB = option_get_bool("EGBB");
   BBDepth = option_get_int("EGBB Depth");
   BBpieceNb = egbb_piece_nb();

   // standard sort

   list_note(list);
   list_sort(list);

   // basic sort

   trans_move = MoveNone;
   if (UseTrans) trans_retrieve(board->key,&trans_move,&trans_min_depth,&trans_max_depth,&trans_min_value,&trans_max_value,&trans_eval,true);

   note_moves(list,board,0,trans_move,ThreadMain);
   list_sort(list);
}

// search_full_root()

int search_full_root(list_t * list, board_t * board, int depth, int search_type) {

   int value;
   int alpha, beta;
   int delta;
   int recursion;
   int move;
   int flag;

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));
   ASSERT(depth_is_ok(depth));
   ASSERT(search_type==SearchNormal||search_type==SearchShort||search_type==SearchMulti);

   ASSERT(list==SearchRoot->list);
   ASSERT(!LIST_IS_EMPTY(list));
   ASSERT(board==SearchCurrent->board);
   ASSERT(board_is_legal(board));
   ASSERT(depth>=1);

   // EGBB init

   BBHeight = 3 * depth / BBDepth;

   // init

   delta = AspirationWindow;

   if (depth < AspirationDepth || !UseAspiration || search_type == SearchMulti) {
      alpha = -ValueInf;
      beta =  +ValueInf;
   } else {

      if (!value_is_win(SearchRoot->last_value)) {
         alpha = SearchRoot->last_value - delta;
         beta = SearchRoot->last_value + delta;
      } else {
         alpha = -ValueInf;
         beta = +ValueInf;
      }
   }

   ASSERT(value_is_ok(alpha));
   ASSERT(value_is_ok(beta));
   ASSERT(range_is_ok(alpha,beta));

   // more init

   recursion = WidthStart;
   flag = SearchUnknown;
   move = MoveNone;
   value = 0; // HACK

   // loop

   do {

      // bound check

      if (!value_is_win(value)) {
         if (SearchBest->flags == SearchUpper) alpha -= (delta * recursion);
         if (SearchBest->flags == SearchLower) beta += (delta * recursion);
      } else {
         alpha = -ValueInf;
         beta =  +ValueInf;
      }

      ASSERT(value_is_ok(alpha));
      ASSERT(value_is_ok(beta));
      ASSERT(range_is_ok(alpha,beta));

      // search

      if (UseDebug) printf("info string d%d: b[%d,%d] value: %d delta: %d\n",depth,alpha,beta,value,delta);
      value = full_root(list,board,alpha,beta,depth*HeightFull,0,search_type);

      // search info

      if (SearchInfo->search_stop) {
         ASSERT(SearchInfo->can_stop);
         ASSERT(SearchBest->move!=MoveNone);
         return ValueDraw;
      }

      // verification

      if ((recursion == WidthStart || (flag == SearchBest->flags && move == SearchBest->move))) {
         recursion += WidthFactor;
      } else {
         recursion = WidthStart;
      }

      flag = SearchBest->flags;
      move = SearchBest->move;

   } while (UseAspiration && SearchBest->flags != SearchExact && depth >= AspirationDepth);

   ASSERT(value_is_ok(value));
   ASSERT(LIST_VALUE(list,0)==value);
   ASSERT(SearchInfo->search_stop==false);

   return value;
}

// full_root()

static int full_root(list_t * list, board_t * board, int alpha, int beta, int depth, int height, int search_type) {

   int old_alpha;
   int value, best_value;
   int i, move;
   int new_depth, root_depth;
   undo_t undo[1];
   mv_t new_pv[HeightMax];

   ASSERT(list_is_ok(list));
   ASSERT(board_is_ok(board));
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(search_type==SearchNormal||search_type==SearchShort||search_type==SearchMulti);

   ASSERT(list==SearchRoot->list);
   ASSERT(!LIST_IS_EMPTY(list));
   ASSERT(board==SearchCurrent->board);
   ASSERT(board_is_legal(board));
   ASSERT(depth>=HeightFull);

   // init

   SearchCurrent->node_nb[ThreadMain]++;
   SearchInfo->check_nb--;

   for (i = 0; i < LIST_SIZE(list); i++) list->value[i] = ValueNone;
   search_clear_best(false); // TODO: fix me!

   old_alpha = alpha;
   best_value = ValueNone;
   root_depth = depth / HeightFull;

   // history eval

   if (board_is_check(board)) {
      board->eval = ValueNone;
   } else {
      board->eval = eval(board,ThreadMain);
   }

   board->move = MoveNone; // HACK

   // move loop

   for (i = 0; i < LIST_SIZE(list); i++) {

      move = LIST_MOVE(list,i);

      SearchRoot->depth = root_depth;
      SearchRoot->move = move;
      SearchRoot->move_pos = i;
      SearchRoot->move_nb = LIST_SIZE(list);

      search_update_root();

      new_depth = full_new_depth(depth,move,board,board_is_check(board)&&LIST_SIZE(list)==1,true);

      board->reduced = false;
      move_do(board,move,undo);

      // search

      if (search_type == SearchShort || best_value == ValueNone || i < MoveNb) { // first move or multi pv
         value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NodePV,ThreadMain,false);
      } else { // other moves
         value = -full_search(board,-alpha-1,-alpha,new_depth,height+1,new_pv,NodeCut,ThreadMain,true);
         if (value > alpha) { // && value < beta
            SearchRoot->change = true;
            SearchRoot->easy = false;
            SearchRoot->flag = false;
            search_update_root();
            value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NodePV,ThreadMain,false);
         }
      }

      move_undo(board,move,undo);

      // search info

      if (SearchInfo->search_stop) {
         ASSERT(SearchInfo->can_stop);
         ASSERT(SearchBest->move!=MoveNone);
         return ValueDraw;
      }

      // list

      if (value <= alpha) { // upper bound
         list->value[i] = old_alpha;
      } else if (value >= beta) { // lower bound
         list->value[i] = beta;
      } else { // alpha < value < beta => exact value
         list->value[i] = value;
      }

      // search best

      SearchBest[i].move = move;
      SearchBest[i].value = value;
      SearchBest[i].depth = root_depth;
      SearchBest[i].node_nb = search_update_node_nb();
      SearchBest[i].time = SearchCurrent->time;

      if (value <= alpha) { // upper bound
         SearchBest[i].flags = SearchUpper;
      } else if (value >= beta) { // lower bound
         SearchBest[i].flags = SearchLower;
      } else { // alpha < value < beta => exact value
         SearchBest[i].flags = SearchExact;
      }

      pv_cat(SearchBest[i].pv,new_pv,move);

      // search update

      if (search_type == SearchMulti) {
         if (value > alpha) {
            if (depth > HeightFull || i + 1 >= MoveNb) search_update_best();
         }
      } else {
         if (value > best_value) {
            if (best_value == ValueNone || value > alpha) search_update_best();
         }
      }

      // search window

      if (value > best_value) best_value = value;

      if (value > alpha) {

         if (search_type == SearchNormal) alpha = value;

         if (search_type == SearchMulti && i+1 >= MoveNb) {
            alpha = SearchBest[MoveNb-1].value;
            ASSERT(value_is_ok(alpha));
            ASSERT(range_is_ok(alpha,beta));
         }

         if (value >= beta) {
            SearchRoot->research = true;
            ASSERT(UseAspiration);
            ASSERT(MoveNb==1);
            break;
         }
         SearchRoot->research = false;

      } else if (UseFailLow && SearchRoot->move_pos == 0) { // first move
         SearchRoot->research = true;
         ASSERT(UseAspiration);
         ASSERT(MoveNb==1);
         break;
      }
   }

   ASSERT(value_is_ok(best_value));

   list_sort(list);

   if (DEBUG) {
       for (i = 0; i < MoveNb; i++) {
          ASSERT(SearchBest[i].move==LIST_MOVE(list,i));
          ASSERT(SearchBest[i].value==LIST_VALUE(list,i)||SearchBest[i].flags!=SearchExact);
       }
   }

   ASSERT(SearchBest->value==best_value);
   ASSERT(SearchInfo->search_stop==false);

   if (UseTrans && best_value > old_alpha && best_value < beta) {
      pv_fill(SearchBest->pv,board);
   }

   return best_value;
}

// full_search()

static int full_search(board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int node_type, int thread, bool null_move) {

   bool in_check;
   bool single_reply;
   bool trans_hit;
   int trans_move, trans_depth, trans_min_depth, trans_max_depth, trans_min_value, trans_max_value, trans_eval;
   int min_value, max_value;
   int old_alpha;
   int value, best_value;
   int eval_value, tune_value;
   int move, best_move;
   int new_depth, search_depth;
   int reduction;
   int played_nb;
   int i;
   int opt_value;
   bool reduced;
   const uint8 * lmr_ptr;
   bool use_fp;
   attack_t attack[1];
   sort_t sort[1];
   undo_t undo[1];
   pawn_info_t pawn_info[1];
   mv_t new_pv[HeightMax];
   mv_t played[256];

   ASSERT(board!=NULL);
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(pv!=NULL);
   ASSERT(node_type==NodePV||node_type==NodeCut||node_type==NodeAll);
   ASSERT(thread_is_ok(thread));
   ASSERT(null_move==true||null_move==false);

   ASSERT(board_is_legal(board));

   // horizon?

   if (depth <= 1) return full_quiescence(board,alpha,beta,0,height,pv,node_type,thread);

   // init

   SearchCurrent->node_nb[thread]++;
   PV_CLEAR(pv);

   if (height > SearchCurrent->max_depth[thread]) SearchCurrent->max_depth[thread] = height;

   // search info & thread

   if (thread == ThreadMain && --SearchInfo->check_nb <= 0) {
      SearchInfo->check_nb += SearchInfo->check_inc;
      search_check();
   }

   if (SearchInfo->search_stop || (ThreadNb > 1 && thread_is_stop(thread))) return ValueDraw;

   // draw?

   if (board_is_repetition(board) || recog_draw(board,thread)) return ValueDraw;

   // mate-distance pruning

   if (UseDistancePruning) {

      // lower bound

      value = VALUE_MATE(height+2); // does not work if the current position is mate
      if (value > alpha && board_is_mate(board)) value = VALUE_MATE(height);

      if (value > alpha) {
         alpha = value;
         if (value >= beta) return value;
      }

      // upper bound

      value = -VALUE_MATE(height+1);

      if (value < beta) {
         beta = value;
         if (value <= alpha) return value;
      }
   }

   // transposition table

   trans_move = MoveNone;
   trans_hit = false;

   if (UseTrans && depth >= TransDepth) {

      if (trans_retrieve(board->key,&trans_move,&trans_min_depth,&trans_max_depth,&trans_min_value,&trans_max_value,&trans_eval,true)) {

         // trans_move is now updated

         trans_hit = true;

         if (node_type != NodePV) {

            if (UseMateValues) {

               if (trans_min_value > +ValueEvalInf && trans_min_depth < depth) {
                  trans_min_depth = depth;
               }

               if (trans_max_value < -ValueEvalInf && trans_max_depth < depth) {
                  trans_max_depth = depth;
               }
            }

            min_value = -ValueInf;

            if (DEPTH_MATCH(trans_min_depth,depth)) {
               min_value = value_from_trans(trans_min_value,height);
               if (min_value >= beta) return min_value;
            }

            max_value = +ValueInf;

            if (DEPTH_MATCH(trans_max_depth,depth)) {
               max_value = value_from_trans(trans_max_value,height);
               if (max_value <= alpha) return max_value;
            }

            if (min_value == max_value) return min_value; // exact match
         }
      }
   }

   // endgame tablebase

   if (UseTB && board->piece_nb <= TBpieceNb) {

      if ((beta != alpha + 1) || (node_type != NodePV && depth >= TBDepth)) {

         if (thread == ThreadMain && tb_probe(board,&value)) {

            if (UseTrans && depth >= TransDepth) {
               trans_eval = ValueNone;
               trans_move = MoveNone;
               trans_depth = depth;
               trans_min_value = trans_max_value = value_from_trans(value,height);

               trans_store(board->key,trans_move,trans_depth,trans_min_value,trans_max_value,trans_eval);
            }

            return value_from_trans(value,height);
         }
      }
   }

   // height limit

   if (height >= HeightMax-1) return eval(board,thread);

   // more init

   old_alpha = alpha;
   best_value = ValueNone;
   best_move = MoveNone;
   new_pv[0] = MoveNone;
   played_nb = 0;
   reduced = board->reduced;

   attack_set(attack,board);
   in_check = ATTACK_IN_CHECK(attack);

   // evaluation

   if (!in_check) {

      if (trans_hit && trans_eval != ValueNone) {
         eval_value = trans_eval;
#if DEBUG
         if (ThreadNb == 1) ASSERT(eval(board,thread)==eval_value); // exceptions are extremely rare
#endif
      } else {
         eval_value = tune_value = eval(board,thread);
      }

      // move evaluation

      if (move_is_ok(board->move) && board->eval != ValueNone) {
         move_eval(board,-(board->eval+eval_value));
      }

      if (trans_hit) tune_value = tune_eval(trans_min_value,trans_max_value,alpha,beta,height,eval_value);

      ASSERT(value_is_ok(eval_value));
      ASSERT(value_is_ok(tune_value));

      if (UseRazor) pawn_get_info(pawn_info,board,thread);

   } else {
      eval_value = tune_value = ValueNone;
   }

   // endgame bitbases

   if (!in_check) {

      // WDL bitbases can't handle mate
      // Let's use Shawul bitbases like he wants

      if (UseBB && board->piece_nb <= BBpieceNb) {

         if (height >= BBHeight || board->cap_sq != Empty) {

            if (!value_is_mate(beta) && egbb_probe(board,&value)) {

               if (value > 0) { // win
                  value = +ValueWin - (50 * height) + eval_value;
               } else if (value < 0) { // loss
                  value = -ValueWin + (50 * height) + eval_value;
               }

               ASSERT(!value_is_mate(value));
               return value;
            }
         }
      }
   }

   // eval history

   board->eval = ValueNone; // HACK

   // razor pruning

   if (UseRazor && depth < RazorDepth
    && node_type != NodePV
    && !in_check
    && !null_move
    && !value_is_mate(beta)
    && trans_move == MoveNone
    && tune_value + RazorMargin[depth] < beta
    && (pawn_info->flags[board->turn] & SevenRankFlag) == 0) {

      ASSERT(node_type!=NodePV);
      ASSERT(!in_check);
      ASSERT(!value_is_mate(beta))
      ASSERT(value_is_ok(tune_value));
      ASSERT(pawn_info!=NULL);

      opt_value = beta - RazorMargin[depth];

      ASSERT(opt_value<beta);
      ASSERT(range_is_ok(opt_value-1,opt_value));

      // quiescence search

      value = full_quiescence(board,opt_value-1,opt_value,0,height,pv,node_type,thread);

      if (value < opt_value) {
         ASSERT(value_is_ok(value));
         return value;
      }
   }

   // static null-move pruning

   if (UseStaticNull && depth < StaticNullDepth
    && node_type != NodePV
    && !in_check
    && null_move
    && !value_is_mate(beta)
    && do_null(board)
    && tune_value - StaticNullMargin[depth] >= beta
    && (pawn_info->flags[COLOUR_OPP(board->turn)] & SevenRankFlag) == 0) {

      ASSERT(node_type!=NodePV);
      ASSERT(pawn_info!=NULL);
      ASSERT(!in_check);
      ASSERT(!value_is_mate(beta))
      ASSERT(value_is_ok(tune_value));
      ASSERT(value_is_ok(tune_value-StaticNullMargin[depth]));

      return tune_value - StaticNullMargin[depth];
   }

   // null-move pruning

   if (UseNull && null_move && depth >= NullDepth && node_type != NodePV) {

      if (!in_check && !value_is_mate(beta) && do_null(board) && tune_value >= beta) {

         ASSERT(value_is_ok(tune_value));

         // null-move search

         new_depth = depth - NullReduction - HeightFull;

         if (UseNullTune) {
            if (depth > VerReduction && tune_value - NullMargin > beta) new_depth -= HeightFull;
         }

         move_do_null(board,undo);
         value = -full_search(board,-beta,-beta+1,new_depth,height+1,new_pv,NODE_OPP(node_type),thread,false);
         move_undo_null(board,undo);

         // pruning

         if (value >= beta) {

            if (value > +ValueEvalInf) value = +ValueEvalInf; // do not return unproven mates
            ASSERT(!value_is_mate(value));

            // verification search

            if (UseVer && depth > VerReduction) {

               new_depth = depth - VerReduction;
               ASSERT(new_depth>0);

               value = full_search(board,alpha,beta,new_depth,height,new_pv,NodeCut,thread,false);
            }

            if (value >= beta) {
               best_move = MoveNone;
               best_value = value;
               goto cut;
            }

         } else if (UseFail) {

            move = new_pv[0];

            if (depth < FailDepth && move != MoveNone && reduced && tune_value >= beta) {

               if (move_is_neighbour(move,board)) {

                  return alpha;
               }
            }
         }
      }
   }

   // Internal Iterative Deepening

   if (UseIID && depth >= IIDDepth && node_type == NodePV && trans_move == MoveNone) {

      new_depth = depth - IIDReduction;
      ASSERT(new_depth>0);

      value = full_search(board,alpha,beta,new_depth,height,new_pv,node_type,thread,node_type!=NodePV);

      if (!UseAspiration && value <= alpha) {
         value = full_search(board,-ValueInf,beta,new_depth,height,new_pv,node_type,thread,node_type!=NodePV);
         trans_move = new_pv[0];
      } else if (new_pv[0] != MoveNone) {
         trans_move = new_pv[0];
      }
   }

   // more init

   board->eval = eval_value;
   board->move = MoveNone;

   use_fp = false;
   if (depth <= FutilityDepth && node_type != NodePV && !in_check) use_fp = true;
   lmr_ptr = LMRList[NODE_FLAG(node_type)][depth];

  // move generation

   sort_init(sort,board,attack,depth,height,trans_move,thread);

   single_reply = false;
   if (in_check && LIST_SIZE(sort->list) == 1) single_reply = true; // HACK

   // move loop

   while ((move=sort_next(sort)) != MoveNone) {

      // extensions

      new_depth = full_new_depth(depth,move,board,single_reply,node_type==NodePV);

      // LMR

      reduced = false;
      reduction = 0;

      if (UseLMR && node_type != NodePV && !in_check) {

         value = sort->value;

         if (played_nb >= 3 && !MOVE_IS_PRIME(value) && !MOVE_IS_KILLER(value)) {

            ASSERT(best_value!=ValueNone);
            ASSERT(value<32768);
            ASSERT(played_nb>0);
            ASSERT(sort->pos>0&&move==LIST_MOVE(sort->list,sort->pos-1));
            ASSERT(move!=trans_move);
            ASSERT(!MOVE_IS_TACTICAL(move,board));

            if (new_depth >= depth) { // TODO: testing
               reduction = MIN(lmr_ptr[played_nb],HeightFull);
            } else {
               reduction = lmr_ptr[played_nb];
            }
            ASSERT(reduction>=0);

            // pruning

            if (UseLMRPruning && depth < LMRDepth) {

               if (depth - reduction < LMRvalue && new_depth < depth && !move_is_dangerous(move,board)) {

                  ASSERT(!move_is_check(move,board));

                  // TODO: more conditions

                  continue;
               }
            }

            if (reduction != 0) reduced = true;
         }
      }

      // futility pruning

      if (UseFutility && use_fp && new_depth < depth && !MOVE_IS_TACTICAL(move,board) && !move_is_dangerous(move,board)) {

         ASSERT(!move_is_check(move,board));
         ASSERT(value_is_ok(eval_value));

         // optimistic evaluation and depth

         opt_value = depth;

         if (played_nb >= 1 + depth) {

            value = sort->value;

            if (false) {
            } else if (value >= 16384) {
            } else if (value <   2054) { opt_value -= 3 * HeightFull;
            } else if (value <   4108) { opt_value -= 2 * HeightFull;
            } else if (value <   8217) { opt_value -= 1 * HeightFull; }
         }

         ASSERT(opt_value+2*HeightFull>=0&&opt_value<8*HeightFull);
         value = eval_value + FutilityMargin[opt_value+2*HeightFull];

         // pruning

         if (value <= alpha) {

            if (value > best_value) {
               best_value = value;
               PV_CLEAR(pv);
            }

            continue;
         }
      }

      // recursive search

      board->reduced = reduced;
      move_do(board,move,undo);

      search_depth = DEPTH_NEW(depth,new_depth,reduction);
      ASSERT(search_depth>=0||UseLMR);

      if (node_type != NodePV || best_value == ValueNone) { // first move
         value = -full_search(board,-beta,-alpha,search_depth,height+1,new_pv,NODE_OPP(node_type),thread,true);
      } else { // other moves
         value = -full_search(board,-alpha-1,-alpha,search_depth,height+1,new_pv,NodeCut,thread,true);
         if (value > alpha) { // && value < beta
            value = -full_search(board,-beta,-alpha,search_depth,height+1,new_pv,NodePV,thread,false);
         }
      }

      // history-pruning re-search

      if (LMRReSearch && reduced && value >= beta) {

         ASSERT(node_type!=NodePV);

         board->reduced = false;

         value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NODE_OPP(node_type),thread,true);
      }

      move_undo(board,move,undo);

      played[played_nb++] = move;

      if (value > best_value) {
         best_value = value;
         pv_cat(pv,new_pv,move);
         if (value > alpha) {
            alpha = value;
            best_move = move;
            if (value >= beta) goto cut;
         }
      }

      if (node_type == NodeCut) node_type = NodeAll;

      // thread

      if (UseThread && ThreadNb > 1 && depth >= ThreadDepth && !single_reply && !value_is_mate(beta)) {

         ASSERT(best_value!=ValueNone);
         ASSERT(played_nb>0);

         if (!thread_is_stop(thread) && thread_is_free(thread)) {
            if (do_split(board,&alpha,old_alpha,beta,depth,height,pv,node_type,&best_value,&played_nb,&best_move,sort,played,eval_value,in_check,thread)) {

               ASSERT(value_is_ok(best_value));

               return best_value;
            }
         }
      }
   }

   // ALL node

   if (best_value == ValueNone) { // no legal move
      if (in_check) {
         ASSERT(board_is_mate(board));
         return VALUE_MATE(height);
      } else {
         ASSERT(board_is_stalemate(board));
         return ValueDraw;
      }
   }

cut:

   ASSERT(value_is_ok(best_value));

   // search info

   if (SearchInfo->search_stop || (ThreadNb > 1 && thread_is_stop(thread))) return best_value;

   // move ordering

   if (best_move != MoveNone) {

      good_move(best_move,board,depth,height,thread,best_value>=beta);

      if (best_value >= beta && !MOVE_IS_TACTICAL(best_move,board)) {

         ASSERT(played_nb>0&&played[played_nb-1]==best_move);

         for (i = 0; i < played_nb-1; i++) {
            move = played[i];
            ASSERT(move!=best_move);

            bad_move(move,board,depth);
         }
      }
   }

   // transposition table

   if (UseTrans && depth >= TransDepth) {

      trans_eval = eval_value;
      trans_move = best_move;
      trans_depth = depth;
      trans_min_value = (best_value > old_alpha) ? value_to_trans(best_value,height) : -ValueInf;
      trans_max_value = (best_value < beta)      ? value_to_trans(best_value,height) : +ValueInf;

      trans_store(board->key,trans_move,trans_depth,trans_min_value,trans_max_value,trans_eval);
   }

   return best_value;
}

// full_thread()

void full_thread(board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int node_type, int thread, split_t * split) {

   bool in_check;
   int value;
   int opt_value, eval_value;
   int move;
   int new_depth, search_depth;
   int reduction;
   bool reduced;
   const uint8 * lmr_ptr;
   bool use_fp;
   undo_t undo[1];
   mv_t new_pv[HeightMax];

   ASSERT(board!=NULL);
   ASSERT(value_is_ok(alpha));
   ASSERT(value_is_ok(beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(pv!=NULL);
   ASSERT(node_type==NodePV||node_type==NodeCut||node_type==NodeAll);
   ASSERT(thread_is_ok(thread));
   ASSERT(split!=NULL);

   ASSERT(range_is_ok(alpha,beta)||split->cut);
   ASSERT(depth>=ThreadDepth);
   ASSERT(split->best_value!=ValueNone);
   ASSERT(split->played_nb>0);
   ASSERT(depth>=ThreadDepth);

   // init

   in_check = split->in_check;
   eval_value = split->eval_value;

   ASSERT(in_check==true||in_check==false);
   ASSERT(in_check||value_is_ok(eval_value));

   ASSERT(board_is_ok(board));
   ASSERT(board_is_legal(board));

   // more init

   use_fp = false;
   if (depth <= FutilityDepth && node_type != NodePV && !in_check) use_fp = true;
   lmr_ptr = LMRList[NODE_FLAG(node_type)][depth];

   // move loop

   MUTEX_LOCK(split->lock);

   while ((move=full_next_move(board,split,&value,thread)) != MoveNone) {

      MUTEX_FREE(split->lock);

      // extensions

      new_depth = full_new_depth(depth,move,board,false,node_type==NodePV);

      // LMR

      reduced = false;
      reduction = 0;

      if (UseLMR && node_type != NodePV && !in_check) {

         if (split->played_nb >= 3 && !MOVE_IS_PRIME(value) && !MOVE_IS_KILLER(value)) {

            ASSERT(value<32768);
            ASSERT(!MOVE_IS_TACTICAL(move,board));

            if (new_depth >= depth) { // TODO: testing
               reduction = MIN(lmr_ptr[split->played_nb],HeightFull);
            } else {
               reduction = lmr_ptr[split->played_nb];
            }
            ASSERT(reduction>=0);

            if (reduction != 0) reduced = true;
         }
      }

      // futility pruning

      if (UseFutility && use_fp && new_depth < depth && !MOVE_IS_TACTICAL(move,board) && !move_is_dangerous(move,board)) {

         ASSERT(!move_is_check(move,board));
         ASSERT(value_is_ok(eval_value));

         // optimistic evaluation and depth

         opt_value = depth;

         if (split->played_nb >= 1 + depth) {

            ASSERT((value>=0&&value<=16384)||value==32768);

            if (false) {
            } else if (value >= 16384) {
            } else if (value <   2054) { opt_value -= 3 * HeightFull;
            } else if (value <   4108) { opt_value -= 2 * HeightFull;
            } else if (value <   8217) { opt_value -= 1 * HeightFull; }
         }

         ASSERT(opt_value+2*HeightFull>=0&&opt_value<8*HeightFull);
         value = eval_value + FutilityMargin[opt_value+2*HeightFull];

         // pruning

         if (value <= alpha) {

            MUTEX_LOCK(split->lock);
            alpha = split->alpha;

            if (value > split->best_value) {
               split->best_value = value;
               PV_CLEAR(pv);
            }

            continue;
         }
      }

      // recursive search

      board->reduced = reduced;
      move_do(board,move,undo);

      search_depth = DEPTH_NEW(depth,new_depth,reduction);
      ASSERT(search_depth>=0||UseLMR);

      if (node_type != NodePV) {
         value = -full_search(board,-beta,-alpha,search_depth,height+1,new_pv,NODE_OPP(node_type),thread,true);
      } else {
         value = -full_search(board,-alpha-1,-alpha,search_depth,height+1,new_pv,NodeCut,thread,true);
         if (value > alpha) { // && value < beta
            value = -full_search(board,-beta,-alpha,search_depth,height+1,new_pv,NodePV,thread,false);
         }
      }

      // history-pruning re-search

      if (LMRReSearch && reduced && value >= beta) {

         ASSERT(node_type!=NodePV);

         board->reduced = false;

         value = -full_search(board,-beta,-alpha,new_depth,height+1,new_pv,NODE_OPP(node_type),thread,true);
      }

      move_undo(board,move,undo);

      MUTEX_LOCK(split->lock);

      if (thread_is_stop(thread)) goto cut;
      ASSERT(split->cut==false);

      split->played[split->played_nb++] = move;

      if (value > split->best_value) {
         ASSERT(value_is_ok(value));
         split->best_value = value;
         pv_cat(pv,new_pv,move);

         if (value > split->alpha) {
            split->alpha = alpha = value;
            split->best_move = move;
            if (value >= beta) {
               split->cut = true;
               goto cut;
            }
         }
      }

      alpha = split->alpha;
      ASSERT(range_is_ok(alpha,beta));
   }

   // thread update

cut:

   split->thread_nb--;
   split->slave[thread] = false;

   ASSERT(split->thread_nb>=0);
   MUTEX_FREE(split->lock);
}

// full_quiescence()

static int full_quiescence(board_t * board, int alpha, int beta, int depth, int height, mv_t pv[], int node_type, int thread) {

   bool in_check;
   int trans_move, trans_depth, trans_min_depth, trans_max_depth, trans_min_value, trans_max_value, trans_eval;
   int min_value, max_value;
   int old_alpha;
   int value, best_value;
   int best_move;
   int move;
   int opt_value, delta_value;
   int eval_value;
   attack_t attack[1];
   sort_t sort[1];
   undo_t undo[1];
   mv_t new_pv[HeightMax];

   ASSERT(board!=NULL);
   ASSERT(range_is_ok(alpha,beta));
   ASSERT(depth_is_ok(depth));
   ASSERT(height_is_ok(height));
   ASSERT(pv!=NULL);
   ASSERT(node_type==NodePV||node_type==NodeCut||node_type==NodeAll);
   ASSERT(thread_is_ok(thread));

   ASSERT(board_is_legal(board));
   ASSERT(depth<=0);

   // init

   SearchCurrent->node_nb[thread]++;
   PV_CLEAR(pv);

   if (height > SearchCurrent->max_depth[thread]) SearchCurrent->max_depth[thread] = height;

   // search info & thread

   if (thread == ThreadMain && --SearchInfo->check_nb <= 0) {
      SearchInfo->check_nb += SearchInfo->check_inc;
      search_check();
   }

   if (SearchInfo->search_stop || (ThreadNb > 1 && thread_is_stop(thread))) return ValueDraw;

   // draw?

   if (board_is_repetition(board) || recog_draw(board,thread)) return ValueDraw;

   // mate-distance pruning

   if (UseDistancePruning) {

      // lower bound

      value = VALUE_MATE(height+2); // does not work if the current position is mate
      if (value > alpha && board_is_mate(board)) value = VALUE_MATE(height);

      if (value > alpha) {
         alpha = value;
         if (value >= beta) return value;
      }

      // upper bound

      value = -VALUE_MATE(height+1);

      if (value < beta) {
         beta = value;
         if (value <= alpha) return value;
      }
   }

   // more init

   attack_set(attack,board);
   in_check = ATTACK_IN_CHECK(attack);

   if (in_check) {
      ASSERT(depth<0);
      depth++; // in-check extension
      trans_depth = 0;
   } else {
      trans_depth = depth >= 0 ? -63 : -64;
   }

   // transposition table

   trans_eval = ValueNone;
   trans_move = MoveNone;

   if (UseTransQS) {

      if (trans_retrieve(board->key,&trans_move,&trans_min_depth,&trans_max_depth,&trans_min_value,&trans_max_value,&trans_eval,false)) {

         // trans_move is now updated

         min_value = -ValueInf;

         if (DEPTH_MATCH(trans_min_depth,trans_depth)) {
            min_value = value_from_trans(trans_min_value,height);
            if (min_value >= beta && node_type != NodePV) return min_value;
         }

         max_value = +ValueInf;

         if (DEPTH_MATCH(trans_max_depth,trans_depth)) {
            max_value = value_from_trans(trans_max_value,height);
            if (max_value <= alpha && node_type != NodePV) return max_value;
         }

         if (min_value == max_value && node_type == NodePV) return min_value; // exact match
      }
   }

   // height limit

   if (height >= HeightMax-1) return eval(board,thread);

   // more init

   old_alpha = alpha;
   best_value = ValueNone;
   best_move = MoveNone;
   opt_value = +ValueInf;
   delta_value = +ValueInf;
   eval_value = ValueNone;

   if (!in_check) {

      // lone-king stalemate?

      if (simple_stalemate(board)) return ValueDraw;

      // evaluation

      if (trans_eval == ValueNone) {
         eval_value = eval(board,thread);
      } else {
         eval_value = trans_eval;
#if DEBUG
         if (ThreadNb == 1) ASSERT(eval(board,thread)==eval_value); // exceptions are extremely rare
#endif
      }

      // stand pat

      ASSERT(eval_value>best_value);
      best_value = eval_value;

      if (best_value > alpha) {
         alpha = best_value;
         if (best_value >= beta) {
            trans_depth = -126;
            goto cut;
         }
      }

      if (UseDelta) {
         opt_value = eval_value + DeltaMargin;
         delta_value = opt_value + DeltaMargin;

         ASSERT(opt_value<+ValueInf);
         ASSERT(delta_value<+ValueInf);
      }
   }

   // move loop

   sort_init_qs(sort,board,attack,depth,node_type,trans_move);

   while ((move=sort_next_qs(sort)) != MoveNone) {

      // delta pruning

      if (UseDelta && node_type != NodePV) {

         if (!in_check && move != trans_move && !move_is_check(move,board) && !capture_is_dangerous(move,board)) {

            ASSERT(MOVE_IS_TACTICAL(move,board));
            ASSERT(!MOVE_IS_PROMOTE(move));

            // optimistic evaluation

            value = opt_value;

            int to = MOVE_TO(move);
            int capture = board->square[to];

            if (capture != Empty) {
               value += VALUE_PIECE(capture);
            } else if (MOVE_IS_EN_PASSANT(move)) {
               value += ValuePawn;
            }

            // pruning

            if (value <= alpha) {

               if (value > best_value) { // needs see_move() >= 0
                  best_value = value;
                  PV_CLEAR(pv);
               }

               continue;
            }

            // delta-delta pruning

            if (UseDeltaDelta && delta_value <= alpha) {

               if (see_move(move,board,beta-delta_value) <= 0) {

                  if (opt_value > best_value) { // HACK: Surprisely, but this is better
                     best_value = opt_value;
                     PV_CLEAR(pv);
                  }

                  continue;
               }
            }
         }
      }

      move_do(board,move,undo);
      value = -full_quiescence(board,-beta,-alpha,depth-1,height+1,new_pv,node_type,thread);
      move_undo(board,move,undo);

      if (value > best_value) {
         best_value = value;
         pv_cat(pv,new_pv,move);
         if (value > alpha) {
            alpha = value;
            best_move = move;
            if (value >= beta) goto cut;
         }
      }
   }

   // ALL node

   if (best_value == ValueNone) { // no legal move
      ASSERT(board_is_mate(board));
      return VALUE_MATE(height);
   }

cut:

   ASSERT(value_is_ok(best_value));
   ASSERT(value_is_ok(eval_value)||(in_check&&eval_value==ValueNone));
   ASSERT(move_is_ok(best_move)||best_move==MoveNone);
   ASSERT(trans_depth<0||in_check);

   // search info

    if (SearchInfo->search_stop || (ThreadNb > 1 && thread_is_stop(thread))) return best_value;

   // transposition table

   if (UseTransQS) {

      trans_eval = eval_value;
      trans_move = best_move;
      trans_min_value = (best_value > old_alpha) ? value_to_trans(best_value,height) : -ValueInf;
      trans_max_value = (best_value < beta)      ? value_to_trans(best_value,height) : +ValueInf;

      trans_store(board->key,trans_move,trans_depth,trans_min_value,trans_max_value,trans_eval);
   }

   return best_value;
}

// full_new_depth()

static int full_new_depth(int depth, int move, board_t * board, bool single_reply, bool in_pv) {

   int new_depth;

   ASSERT(depth_is_ok(depth));
   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);
   ASSERT(single_reply==true||single_reply==false);
   ASSERT(in_pv==true||in_pv==false);

   ASSERT(depth>0);

   new_depth = depth - HeightFull;

   if ((single_reply && ExtendSingleReply)
    || (in_pv && MOVE_IS_TACTICAL(move,board)
              && see_move(move,board,0) > 0)
    || (in_pv && PIECE_IS_PAWN(MOVE_PIECE_FROM(move,board))
              && !MOVE_IS_TACTICAL(move,board)
              && pawn_is_passed(move,board)
              && (PAWN_RANK(MOVE_TO(move),board->turn) >= Rank6
              || see_move(move,board,0) >= 0))
    || (ExtendPawnEndgame && board->square[MOVE_TO(move)] != Empty
              && pawn_endgame(move,board))
    || move_is_check(move,board)) {
      new_depth += HeightFull;
   }

   ASSERT(new_depth>=0&&new_depth<=depth);

   return new_depth;
}

// full_next_move()

static int full_next_move (board_t * board, split_t * split, int * value, int thread) {

   int move;

   ASSERT(board!=NULL);
   ASSERT(split!=NULL);
   ASSERT(value!=NULL);
   ASSERT(thread_is_ok(thread));

   // end

   if (split->end || thread_is_stop(thread)) return MoveNone;

   // init

   split->sort->board = board; // HACK

   // next move

   move = sort_next(split->sort);
   *value = split->sort->value;

   if (move == MoveNone) split->end = true;

   ASSERT(move==MoveNone||move_is_ok(move));
   ASSERT((*value>=0&&*value<=16384)||*value==32768||*value==65536);

   return move;
}

// do_null()

static bool do_null(const board_t * board) {

   ASSERT(board!=NULL);

   // use null move if the side-to-move has at least one piece

   return board->piece_size[board->turn] >= 2; // king + one piece
}

// pv_fill()

static void pv_fill(const mv_t pv[], board_t * board) {

   int move;
   int trans_move, trans_depth, trans_min_value, trans_max_value, trans_eval;
   undo_t undo[1];

   ASSERT(pv!=NULL);
   ASSERT(board!=NULL);

   ASSERT(UseTrans);

   move = *pv;

   if (move != MoveNone && move != MoveNull) {

      move_do(board,move,undo);
      pv_fill(pv+1,board);
      move_undo(board,move,undo);

      trans_move = move;
      trans_depth = -127; // HACK
      trans_min_value = -ValueInf;
      trans_max_value = +ValueInf;

      if (board_is_check(board)) {
         trans_eval = ValueNone;
      } else {
         trans_eval = eval(board,ThreadMain);
      }

      trans_store(board->key,trans_move,trans_depth,trans_min_value,trans_max_value,trans_eval);
   }
}

// move_is_dangerous()

static bool move_is_dangerous(int move, const board_t * board) {

   int piece;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(!MOVE_IS_TACTICAL(move,board));

   piece = MOVE_PIECE_FROM(move,board);

   if (PIECE_IS_PAWN(piece)) {

      if (PAWN_RANK(MOVE_TO(move),board->turn) >= Rank6
       || pawn_is_passed(move,board)) return true;
   }

   return false;
}

// move_is_neighbour()

static bool move_is_neighbour(int move, const board_t * board) {

   int last_move, last_from, last_to;
   int from, to;
   int piece;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   last_move = board->move;
   ASSERT(move_is_ok(last_move));

   from = MOVE_FROM(move);
   to = MOVE_TO(move);
   last_from = MOVE_FROM(last_move);
   last_to = MOVE_TO(last_move);

   if (last_to == from) return true;
   if (to == last_from) return true;

   piece = board->square[from];

   if (PIECE_IS_SLIDER(piece)) {
      if (line_is_touched(board,from,to,piece,last_from)) return true;
   }

   // defender

   piece = board->square[last_to];

   if (piece_is_touched(board,last_to,from,to,piece,COLOUR_OPP(board->turn))) return true;

   // check

   if (PIECE_IS_SLIDER(piece)) {
      if (piece_is_touched(board,last_to,from,KING_POS(board,board->turn),piece,board->turn)) return true;
   }

   return false;
}

// capture_is_dangerous()

static bool capture_is_dangerous(int move, const board_t * board) {

   int piece, capture;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(MOVE_IS_TACTICAL(move,board));

   piece = MOVE_PIECE_FROM(move,board);

   if (PIECE_IS_PAWN(piece)) {

      if (PAWN_RANK(MOVE_TO(move),board->turn) >= Rank6
       || pawn_is_passed(move,board)) return true;
   }

   capture = move_capture(move,board);

   if (PIECE_IS_QUEEN(capture)) return true;

   if (PIECE_IS_PAWN(capture)
    && PAWN_RANK(MOVE_TO(move),board->turn) <= Rank2) {
      return true;
   }

   return false;
}

// simple_stalemate()

static bool simple_stalemate(const board_t * board) {

   int me, opp;
   int king;
   int opp_flag;
   int from, to;
   int capture;
   const inc_t * inc_ptr;
   int inc;

   ASSERT(board!=NULL);

   ASSERT(board_is_legal(board));
   ASSERT(!board_is_check(board));

   // lone king?

   me = board->turn;
   if (board->piece_size[me] != 1 || board->pawn_size[me] != 0) return false; // no

   // king in a corner?

   king = KING_POS(board,me);
   if (king != A1 && king != H1 && king != A8 && king != H8) return false; // no

   // init

   opp = COLOUR_OPP(me);
   opp_flag = COLOUR_FLAG(opp);

   // king can move?

   from = king;

   for (inc_ptr = KingInc; (inc=*inc_ptr) != IncNone; inc_ptr++) {
      to = from + inc;
      capture = board->square[to];
      if (capture == Empty || FLAG_IS(capture,opp_flag)) {
         if (!is_attacked(board,to,opp)) return false; // legal king move
      }
   }

   // no legal move

   ASSERT(board_is_stalemate((board_t*)board));

   return true;
}

// tune_eval()

static int tune_eval(int trans_min_value, int trans_max_value, int alpha, int beta, int height, int eval) {

   int min_value, max_value;

   ASSERT(value_is_ok(alpha));
   ASSERT(value_is_ok(beta));
   ASSERT(height_is_ok(height));
   ASSERT(value_is_ok(eval));

   ASSERT(value_is_ok(trans_min_value));
   ASSERT(value_is_ok(trans_max_value));

   // mate bounds (remove me?)

   min_value = value_from_trans(trans_min_value,height);
   max_value = value_from_trans(trans_max_value,height);

   // bound check

   if (min_value >= beta && min_value > eval) return min_value;
   if (max_value <= alpha && max_value < eval) return max_value;

   // fallback

   return eval;
}

// pawn_is_passed

static bool pawn_is_passed(int move, const board_t * board) {

   int bit_file;
   int square;
   int me, opp;
   int file, rank;

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(PIECE_IS_PAWN(board->square[MOVE_FROM(move)]));

   // init

   square = MOVE_TO(move);
   me = board->turn;
   opp = COLOUR_OPP(me);
   file = SQUARE_FILE(square);
   rank = PAWN_RANK(square,me);

   ASSERT(file>=FileA&&file<=FileH);
   ASSERT(rank>=Rank3&&rank<=Rank8);

   // special cases

   if (rank == Rank8) return false; // promote
   if (rank == Rank7) return true;

   // pawn file

   bit_file = board->pawn_file[me][file] | BitRev[board->pawn_file[opp][file]];

   // file neighbours

   if ((bit_file & BitGT[rank]) == 0) {
      if (((BitRev[board->pawn_file[opp][file-1]] | BitRev[board->pawn_file[opp][file+1]]) & BitGT[rank]) == 0) return true;
   }

   return false;
}

// enter_pawn_ending()

static bool pawn_endgame(int move, const board_t * board) {

   ASSERT(move_is_ok(move));
   ASSERT(board!=NULL);

   ASSERT(move_is_capture(move,board));

   // special cases

   if (PIECE_IS_PAWN(MOVE_PIECE_TO(move,board))) return false;
   if (MOVE_IS_EN_PASSANT(move)) return false;
   if (MOVE_IS_PROMOTE(move)) return false;

   if (board->piece_size[White] + board->piece_size[Black] - 1 == 2) return true;

   return false;
}

// end of search_full.cpp
